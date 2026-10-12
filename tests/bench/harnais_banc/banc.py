"""Façade du banc d'essai : instruments du banc validé, horloge et opérations communes aux tests NR.

Le banc n'est ouvert que par la fixture de session de ``conftest.py``, après validation par le
registre. Chaque appel d'instrument est consigné dans le journal des stimulus
(``ESSENSYS_BENCH_JOURNAL``), à joindre au rapport ``/checkup``.
"""

from __future__ import annotations

import importlib
import time
from contextlib import contextmanager
from typing import Any, Callable, Iterable, Iterator, Mapping, NamedTuple, Sequence

from . import analyse, seuils
from .instruments import (
    INTERFACES,
    AlimentationLabo,
    AnalyseurLogique,
    ConsoleDiagnostic,
    GenerateurInjection,
    Instrument,
    MaitreCommande,
    Oscilloscope,
    ProgrammateurBanc,
    RelaisBanc,
    journaliser,
)
from .registre import BancRefuse, ConfigBanc, InstrumentDeclare


class BancNonNominal(RuntimeError):
    """Le banc n'a pas retrouvé son état nominal : le test est en erreur, pas en échec."""


class CommandeTrafic(NamedTuple):
    """Commande émise par ``Banc.trafic`` : instant d'envoi, état visé et identifiant attribué par le
    serveur de banc."""

    envoi: float
    etat: bool
    identifiant: str


class Horloge:
    """Base de temps commune au harnais et aux pilotes (``time.monotonic``)."""

    def maintenant(self) -> float:
        return time.monotonic()

    def attendre(self, duree_s: float) -> None:
        if duree_s > 0:
            time.sleep(duree_s)


class _Journalise:
    """Mandataire qui consigne chaque appel de méthode d'un instrument dans le journal."""

    def __init__(self, role: str, instrument: Instrument) -> None:
        self._role = role
        self._instrument = instrument

    def __getattr__(self, nom: str) -> Any:
        attribut = getattr(self._instrument, nom)
        if not callable(attribut) or nom.startswith("_"):
            return attribut

        def appel(*args: Any, **kwargs: Any) -> Any:
            journaliser("appel", role=self._role, methode=nom, args=args, kwargs=kwargs)
            return attribut(*args, **kwargs)

        return appel


def _charger_pilote(declare: InstrumentDeclare) -> type[Instrument]:
    module, _, classe = declare.pilote.partition(":")
    pilote = getattr(importlib.import_module(module), classe)
    interface = INTERFACES[declare.role]
    if not (isinstance(pilote, type) and issubclass(pilote, interface)):
        raise BancRefuse(f"le pilote {declare.pilote} n'implémente pas l'interface {interface.__name__}.")
    return pilote


class Banc:
    """Banc validé et ses instruments."""

    alim: AlimentationLabo
    oscillo: Oscilloscope
    analyseur: AnalyseurLogique
    relais: RelaisBanc
    maitre: MaitreCommande
    console: ConsoleDiagnostic
    injection: GenerateurInjection | None
    programmateur: ProgrammateurBanc | None

    _ATTRIBUTS = {
        "alimentation": "alim",
        "oscilloscope": "oscillo",
        "analyseur": "analyseur",
        "relais": "relais",
        "maitre": "maitre",
        "console": "console",
        "injection": "injection",
        "programmateur": "programmateur",
    }

    def __init__(self, config: ConfigBanc, horloge: Horloge | None = None) -> None:
        self.config = config
        self.horloge = horloge or Horloge()
        self._instruments: dict[str, Instrument] = {}
        self.injection = None
        self.programmateur = None

    # ------------------------------------------------------------------ ouverture

    @classmethod
    def ouvrir(cls, config: ConfigBanc, horloge: Horloge | None = None) -> "Banc":
        """Instancie les instruments déclarés. Le serveur legacy de banc est ouvert et identifié en
        premier : s'il n'annonce pas le banc déclaré, rien d'autre n'est ouvert."""
        banc = cls(config, horloge)
        ordre = ["maitre"] + [role for role in config.instruments if role != "maitre"]
        try:
            for role in ordre:
                banc._ouvrir_instrument(config.instruments[role])
                if role == "maitre":
                    identite = banc.maitre.verifier_identite_banc()
                    if identite != config.identifiant:
                        raise BancRefuse(
                            f"le serveur legacy joint annonce le banc « {identite} » au lieu de "
                            f"« {config.identifiant} ». Arrêt avant tout stimulus."
                        )
        except BaseException:
            banc.fermer(remettre_en_securite=False)
            raise
        return banc

    def _ouvrir_instrument(self, declare: InstrumentDeclare) -> None:
        parametres = dict(declare.parametres)
        if declare.role == "alimentation":
            parametres["limites"] = dict(self.config.alimentations)
        if declare.role == "injection":
            parametres["echantillons"] = sorted(self.config.echantillons_injection)
        instrument = _charger_pilote(declare)(banc=self.config.identifiant, parametres=parametres)
        instrument.ouvrir()
        self._instruments[declare.role] = instrument
        setattr(self, self._ATTRIBUTS[declare.role], _Journalise(declare.role, instrument))

    def fermer(self, remettre_en_securite: bool = True) -> None:
        """Remet chaque instrument en état sûr (stimulus retirés), puis ferme les liaisons."""
        erreurs = []
        if remettre_en_securite:
            for role in ("injection", "relais", "maitre"):
                if role in self._instruments:
                    try:
                        self._instruments[role].etat_sur()
                    except Exception as exc:  # noqa: BLE001 - on poursuit la mise en sécurité
                        erreurs.append(f"{role}: {exc}")
        for instrument in self._instruments.values():
            try:
                instrument.fermer()
            except Exception as exc:  # noqa: BLE001
                erreurs.append(f"{instrument.role}: {exc}")
        journaliser("fermeture", banc=self.config.identifiant, erreurs=erreurs)
        if erreurs:
            raise RuntimeError("fermeture du banc incomplète : " + "; ".join(erreurs))

    # ------------------------------------------------------------------ temps

    def maintenant(self) -> float:
        return self.horloge.maintenant()

    def attendre(self, duree_s: float) -> None:
        self.horloge.attendre(duree_s)

    def attendre_que(
        self, condition: Callable[[], bool], delai_s: float, periode_s: float = seuils.PERIODE_SCRUTATION_S
    ) -> float | None:
        """Durée écoulée quand ``condition()`` devient vraie, ou ``None`` après ``delai_s``."""
        debut = self.maintenant()
        while True:
            if condition():
                return self.maintenant() - debut
            if self.maintenant() - debut >= delai_s:
                return None
            self.attendre(periode_s)

    # ------------------------------------------------------------------ description du banc

    @property
    def ba(self) -> tuple[int, ...]:
        """BA du banc, du plus proche au plus éloigné du BP."""
        return self.config.ordre_ba

    @property
    def ba_le_plus_eloigne(self) -> int:
        return self.config.ba_le_plus_eloigne

    def autres_ba(self, *exclus: int) -> tuple[int, ...]:
        return tuple(ba for ba in self.ba if ba not in exclus)

    def sortie_temoin(self, ba: int) -> int:
        return int(self.config.ba[ba]["sortie_temoin"])

    def sorties(self, ba: int) -> tuple[int, ...]:
        return tuple(int(sortie) for sortie in self.config.ba[ba]["sorties"])

    def ba_deconnectable_seul(self) -> int:
        """BA dont la déconnexion par son câble ne coupe aucun autre BA : le BA en bout de chaîne,
        ou n'importe quel BA en étoile (le plus éloigné)."""
        return self.ba_le_plus_eloigne

    # ------------------------------------------------------------------ opérations communes

    def remettre_etat_nominal(self, cable: str = seuils.CABLE_NOMINAL) -> None:
        """Retire tout stimulus, monte la configuration de câble ``cable`` (le câble nominal par
        défaut), alimente le BP à sa tension nominale et attend que chaque BA acquitte une commande.
        Lève ``BancNonNominal`` sinon."""
        self.relais.etat_sur()
        self.relais.selectionner_cable(cable)
        if self.injection is not None:
            self.injection.etat_sur()
        self.maitre.vider_commandes()
        voie = self.config.alimentations["bp"]
        self.alim.regler("bp", voie.nominale_v, voie.max_a)
        self.alim.activer("bp")
        resultats = self.reprise(self.ba, delai_s=seuils.DELAI_ETAT_NOMINAL_S)
        absents = [f"{ba:#04x}" for ba, duree in resultats.items() if duree is None]
        if absents:
            raise BancNonNominal(f"BA sans acquittement à l'état nominal : {', '.join(absents)}")

    def moyen_deconnexion(self) -> str:
        """Déconnexion d'un seul BA : par le cavalier de son port si le banc en a, sinon par son câble."""
        return "cavalier" if "cavaliers_par_port" in self.config.capacites else "cable"

    def envoyer_bascules(self, bas: Iterable[int]) -> dict[int, tuple[int, bool]]:
        """Commande à chaque BA l'inversion de sa sortie témoin ; renvoie {BA: (sortie, état visé)}."""
        cibles = {}
        for ba in bas:
            sortie = self.sortie_temoin(ba)
            etat = not self.relais.lire_sortie(ba, sortie)
            self.maitre.commander_sortie(ba, sortie, etat)
            cibles[ba] = (sortie, etat)
        return cibles

    def mesurer_application(
        self, cibles: dict[int, tuple[int, bool]], depuis: float, delai_s: float = seuils.DELAI_REPRISE_S
    ) -> dict[int, float | None]:
        """Durée, depuis ``depuis``, au bout de laquelle chaque charge factice visée a pris son état ;
        ``None`` pour un BA qui ne l'a pas appliqué dans ``delai_s``."""
        resultats: dict[int, float | None] = {ba: None for ba in cibles}

        def toutes_appliquees() -> bool:
            for ba, (sortie, etat) in cibles.items():
                if resultats[ba] is None:
                    historique = self.relais.historique_sortie(ba, sortie, depuis)
                    instant = analyse.instant_application(historique, etat, depuis)
                    if instant is not None:
                        resultats[ba] = instant - depuis
            return all(duree is not None for duree in resultats.values())

        restant = max(0.0, depuis + delai_s - self.maintenant())
        self.attendre_que(toutes_appliquees, restant)
        return {ba: (d if d is not None and d <= delai_s else None) for ba, d in resultats.items()}

    def reprise(
        self, bas: Iterable[int], depuis: float | None = None, delai_s: float = seuils.DELAI_REPRISE_S
    ) -> dict[int, float | None]:
        """Inverse la sortie témoin de chaque BA de ``bas`` et mesure, depuis ``depuis`` (par
        défaut l'instant présent), la durée au bout de laquelle la commande est appliquée."""
        depuis = self.maintenant() if depuis is None else depuis
        return self.mesurer_application(self.envoyer_bascules(bas), depuis, delai_s)

    def recuperees(self, identifiants: Iterable[str]) -> bool:
        """Vrai si le BP a récupéré chacune de ces commandes auprès du serveur de banc."""
        identifiants = list(identifiants)
        instants = self.maitre.instants_recuperation(identifiants)
        return all(instants.get(identifiant) is not None for identifiant in identifiants)

    def commander_sortie_recuperee(
        self, ba: int, sortie: int, etat: bool, delai_s: float = seuils.DELAI_REPRISE_S
    ) -> float | None:
        """Commande une sortie et attend que le BP l'ait récupérée ; renvoie le délai, ou ``None``
        au-delà de ``delai_s``. Deux ordres successifs ainsi émis ne sont jamais récupérés dans le
        même lot."""
        identifiant = self.maitre.commander_sortie(ba, sortie, etat)
        return self.attendre_que(lambda: self.recuperees([identifiant]), delai_s, seuils.PERIODE_SCRUTATION_SERVEUR_S)

    def trafic(
        self,
        bas: Iterable[int],
        duree_s: float,
        periode_s: float = seuils.PERIODE_TRAFIC_S,
        suite_de: Mapping[int, Sequence[CommandeTrafic]] | None = None,
    ) -> dict[int, list[CommandeTrafic]]:
        """Trafic soutenu pendant ``duree_s`` : à chaque tour, inverse la sortie témoin de chaque BA,
        puis attend que le BP ait récupéré ces commandes, et au moins ``periode_s``, avant le tour
        suivant. Ainsi deux ordres contraires ne sont jamais récupérés dans le même lot, ce qui
        ferait sauter l'état intermédiaire (la spec l'admet). Aucune attente ne dépasse la fin de
        l'essai. Renvoie, par BA, les commandes émises, chacune inversant la précédente.

        ``suite_de`` prolonge un trafic précédent : l'état de départ de chaque BA est alors le
        dernier état commandé, et non l'état lu sur la charge, et le premier tour attend que le BP
        ait récupéré ces dernières commandes, qui peuvent être encore en vol."""
        bas = tuple(bas)
        fin = self.maintenant() + duree_s
        etats, en_vol = {}, []
        for ba in bas:
            precedentes = (suite_de or {}).get(ba)
            if precedentes:
                etats[ba] = precedentes[-1].etat
                en_vol.append(precedentes[-1].identifiant)
            else:
                etats[ba] = self.relais.lire_sortie(ba, self.sortie_temoin(ba))
        commandes: dict[int, list[CommandeTrafic]] = {ba: [] for ba in bas}
        self._attendre_recuperation(en_vol, fin)
        while True:
            debut_tour = self.maintenant()
            tour = []
            for ba in bas:
                etats[ba] = not etats[ba]
                instant = self.maintenant()
                identifiant = self.maitre.commander_sortie(ba, self.sortie_temoin(ba), etats[ba])
                commandes[ba].append(CommandeTrafic(instant, etats[ba], identifiant))
                tour.append(identifiant)
            self._attendre_recuperation(tour, fin)
            self.attendre(max(0.0, min(debut_tour + periode_s, fin) - self.maintenant()))
            if self.maintenant() >= fin:
                return commandes

    def _attendre_recuperation(self, identifiants: list[str], fin: float) -> None:
        """Attend que le BP ait récupéré ces commandes, au plus jusqu'à ``fin``."""
        if identifiants:
            self.attendre_que(
                lambda: self.recuperees(identifiants),
                max(0.0, fin - self.maintenant()),
                seuils.PERIODE_SCRUTATION_SERVEUR_S,
            )

    def commandes_perdues(
        self, commandes: dict[int, list[CommandeTrafic]], depuis: float
    ) -> dict[int, list[analyse.CommandeEmise]]:
        """Commandes de ``trafic`` dont l'état n'a pas été appliqué avant que le BP ne récupère la
        commande suivante. La dernière commande de chaque BA dispose de ``DELAI_REPRISE_S`` après
        son envoi, latence de scrutation HTTP comprise : l'analyse attend la fin de cette fenêtre."""
        derniers = [liste[-1].envoi for liste in commandes.values() if liste]
        if not derniers:
            return {}
        fin = max(derniers) + seuils.DELAI_REPRISE_S
        self.attendre(max(0.0, fin - self.maintenant()))
        recuperations = self.maitre.instants_recuperation(
            [commande.identifiant for liste in commandes.values() for commande in liste]
        )
        perdues = {}
        for ba, liste in commandes.items():
            emises = [analyse.CommandeEmise(c.envoi, c.etat, recuperations.get(c.identifiant)) for c in liste]
            historique = self.relais.historique_sortie(ba, self.sortie_temoin(ba), depuis)
            manquees = analyse.commandes_perdues(emises, historique, fin)
            if manquees:
                perdues[ba] = manquees
        return perdues

    def historique_index12(self, depuis: float, jusqu_a: float | None = None) -> list[tuple[float, int]]:
        return self.maitre.historique_index(seuils.INDEX_COMM_FAULTS, depuis, jusqu_a)

    def attendre_bits_ba(self, attendu: int, depuis: float, delai_s: float = seuils.DELAI_TELEMETRIE_S) -> float | None:
        """Durée, depuis ``depuis``, au bout de laquelle ``index 12 & 0x1C`` vaut ``attendu``."""
        instant: list[float] = []

        def atteint() -> bool:
            trouve = analyse.premier_instant_bits_ba(self.historique_index12(depuis), attendu, depuis)
            if trouve is not None:
                instant.append(trouve)
            return trouve is not None

        restant = max(0.0, depuis + delai_s - self.maintenant())
        self.attendre_que(atteint, restant, periode_s=1.0)
        if not instant or instant[0] - depuis > delai_s:
            return None
        return instant[0] - depuis

    def attendre_blocage(self, ligne: str, depuis: float, delai_s: float, point: str = "bus") -> float | None:
        """Début du premier niveau bas significatif de ``ligne`` après ``depuis`` (un défaut d'essai
        qui commence), ou ``None`` s'il n'est pas apparu dans ``delai_s``."""
        debuts: list[float] = []

        def commence() -> bool:
            intervalles = analyse.intervalles_significatifs(
                self.analyseur.intervalles_bas(ligne, depuis, point=point), seuils.SEUIL_BAS_SIGNIFICATIF_S
            )
            if intervalles:
                debuts.append(intervalles[0][0])
            return bool(intervalles)

        self.attendre_que(commence, delai_s)
        return debuts[0] if debuts else None

    def attendre_index_sain(self) -> None:
        """Attend que ``index 12 & 0x1C`` soit nul ; ``BancNonNominal`` sinon."""
        if self.attendre_bits_ba(0, self.maintenant()) is None:
            raise BancNonNominal("index 12 : bits des BA non effacés à l'état nominal")

    def valeur_index12(self) -> int | None:
        """Dernière valeur de l'index 12 remontée par le BP."""
        maintenant = self.maintenant()
        historique = self.historique_index12(maintenant - seuils.DELAI_TELEMETRIE_S, maintenant)
        return historique[-1][1] if historique else None

    def attendre_configuration(
        self, ba: int, depuis: float, delai_s: float = seuils.DELAI_RESYNCHRONISATION_S
    ) -> float | None:
        """Durée, depuis ``depuis``, au bout de laquelle le BA a acquitté sa configuration complète,
        identique à celle du serveur de banc ; ``None`` au-delà de ``delai_s``."""
        attendue = self.maitre.configuration(ba)
        acquittee: list[float] = []

        def conforme() -> bool:
            resultat = self.analyseur.configuration_acquittee(ba, depuis)
            if resultat is not None and dict(resultat.configuration) == dict(attendue):
                acquittee.append(resultat.instant)
                return True
            return False

        restant = max(0.0, depuis + delai_s - self.maintenant())
        self.attendre_que(conforme, restant)
        if not acquittee or acquittee[0] - depuis > delai_s:
            return None
        return acquittee[0] - depuis

    def sorties_a_l_arret(self) -> None:
        """Toutes les sorties tout-ou-rien des BA à l'arrêt ; ``BancNonNominal`` sinon."""
        for ba in self.ba:
            for sortie in self.sorties(ba):
                self.maitre.commander_sortie(ba, sortie, False)

        def toutes_arretees() -> bool:
            return not any(self.relais.lire_sortie(ba, s) for ba in self.ba for s in self.sorties(ba))

        if self.attendre_que(toutes_arretees, seuils.DELAI_REPRISE_S) is None:
            raise BancNonNominal("sorties des BA non revenues à l'arrêt")

    def periode_http_reference(self) -> float:
        """Plus long écart entre deux requêtes du BP sur une fenêtre de référence sans défaut."""
        debut = self.maintenant()
        self.attendre(seuils.DUREE_REFERENCE_HTTP_S)
        fin = self.maintenant()
        return max(analyse.ecarts_periode_http(self.maitre.instants_echanges_bp(debut, fin), debut, fin))

    def mesures_repos(self) -> dict[str, float]:
        """Grandeurs de repos, sorties à l'arrêt, comparées avant et après un essai contraignant."""
        self.sorties_a_l_arret()
        mesures = {"courant_bp_a": self.alim.mesurer_courant("bp")}
        for ba in self.ba:
            mesures[f"entree_15v_{ba:#04x}_v"] = self.oscillo.tension_continue(seuils.point_entree_15v(ba))
        return mesures

    @contextmanager
    def consommation_maximale(self) -> Iterator[None]:
        """Toutes les sorties des BA à l'état actif (relais monostables collés), puis retour."""
        depuis = self.maintenant()
        for ba in self.ba:
            for sortie in self.sorties(ba):
                self.maitre.commander_sortie(ba, sortie, True)

        def toutes_actives() -> bool:
            return all(self.relais.lire_sortie(ba, s) for ba in self.ba for s in self.sorties(ba))

        if self.attendre_que(toutes_actives, seuils.DELAI_REPRISE_S) is None:
            raise BancNonNominal("consommation maximale non atteinte : sorties non actives")
        journaliser("consommation_maximale", depuis=depuis)
        try:
            yield
        finally:
            for ba in self.ba:
                for sortie in self.sorties(ba):
                    self.maitre.commander_sortie(ba, sortie, False)
