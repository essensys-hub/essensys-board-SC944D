"""Interfaces abstraites des instruments du banc d'essai SC944D (tâche 7.2).

Les tests de non-régression ne pilotent le banc qu'à travers ces interfaces. Les pilotes concrets
(un par modèle d'instrument) sont écrits avec la fiche du banc (tâche 7.1), placés dans
``harnais_banc/pilotes/`` et référencés par le registre ``tests/bench/benches.yaml``.

Garde-fous communs :

- aucun instrument ne peut être instancié tant que le harnais n'a pas validé et autorisé un banc
  déclaré (``autoriser_banc``) : sans banc, ou avec un banc refusé, ``StimulusInterdit`` est levée ;
- chaque instanciation est consignée dans le journal des stimulus (variable d'environnement
  ``ESSENSYS_BENCH_JOURNAL``), puis chaque appel d'instrument par le harnais (``banc.py``) ;
- ``etat_sur()`` ramène chaque instrument dans un état sans stimulus.

Base de temps : tous les instants échangés sont exprimés dans la base de ``time.monotonic()`` du
processus de test (``Horloge.maintenant()``). Chaque pilote convertit ses propres horodatages.

Le banc n'est jamais une armoire en service : le registre refuse toute carte dont la destination
n'est pas « banc », et toute cible réseau hors d'un réseau de banc isolé déclaré.
"""

from __future__ import annotations

import abc
import json
import os
import time
from dataclasses import dataclass, field
from typing import Any, Literal, Mapping, Sequence

Ligne = Literal["SDA", "SCL"]
Cote = Literal["bus", "ba_mib"]
Deconnexion = Literal["cable", "cavalier"]

ENV_JOURNAL = "ESSENSYS_BENCH_JOURNAL"

#: Défauts que le firmware d'essai d'un BA de banc sait provoquer sur ordre du banc. Chaque ordre
#: provoque le défaut une seule fois, à la première transaction du BP vers ce BA qui suit l'ordre ;
#: ``annuler_defaut_ba`` retire un ordre non encore consommé.
DEFAUTS_BA = frozenset(
    {
        "etirement_max",  # BA 1.7 qui étire l'horloge à sa durée maximale mesurée (NR-3)
        "fige_milieu_octet",  # esclave bloqué au milieu d'un octet, SDA retenue (NR-4, NR-13)
        "pic_fige_scl",  # PIC figé, SCL retenue (NR-8)
        "ssp_fige_sda",  # module SSP figé, boucle principale vivante, SDA retenue (NR-8)
        "debordement_ssp",  # débordement du module SSP (NR-8)
        "tampon_occupe",  # tampon de commande occupé (NR-8)
        "eeprom_vierge",  # effacement de la configuration, puis redémarrage (NR-6)
    }
)

#: Firmwares que le programmateur du banc sait monter (NR-11).
FIRMWARES_BP = ("099-37", "candidat")
FIRMWARES_BA = ("1.7", "1.8")


class StimulusInterdit(RuntimeError):
    """Tentative de piloter un instrument sans banc déclaré, reconnu et autorisé."""


_banc_autorise: str | None = None


def autoriser_banc(identifiant: str) -> None:
    """Autorise l'instanciation des instruments du banc validé par le registre."""
    global _banc_autorise
    _banc_autorise = identifiant
    journaliser("autorisation", banc=identifiant)


def revoquer_autorisation() -> None:
    global _banc_autorise
    _banc_autorise = None


def banc_autorise() -> str | None:
    return _banc_autorise


def journaliser(evenement: str, **details: Any) -> None:
    """Ajoute une ligne JSON au journal des stimulus, si ``ESSENSYS_BENCH_JOURNAL`` est défini."""
    chemin = os.environ.get(ENV_JOURNAL)
    if not chemin:
        return
    ligne = {"t": time.time(), "evenement": evenement, **details}
    with open(chemin, "a", encoding="utf-8") as fichier:
        fichier.write(json.dumps(ligne, ensure_ascii=False, default=repr) + "\n")


# --------------------------------------------------------------------------- données échangées


@dataclass(frozen=True)
class LimitesVoie:
    """Limites d'une voie d'alimentation déclarées par le registre du banc."""

    min_v: float
    max_v: float
    max_a: float

    @property
    def nominale_v(self) -> float:
        return round((self.min_v + self.max_v) / 2, 3)


@dataclass(frozen=True)
class TrameI2C:
    """Une transaction du BP vers un BA décodée par l'analyseur (écriture puis lecture d'acquittement).

    ``erreur`` vaut ``None`` quand la trame est acquittée (adresse et données acquittées, écho et
    CRC de la réponse corrects). Sinon : ``"nack_adresse"``, ``"nack_donnees"``, ``"crc"``,
    ``"echo"``, ``"attente"`` (abandon sur timeout) ou ``"interrompue"``.
    """

    t_debut: float
    t_fin: float
    adresse: int | None
    code: str | None = None
    donnees: bytes = b""
    erreur: str | None = None

    @property
    def acquittee(self) -> bool:
        return self.erreur is None

    @property
    def duree(self) -> float:
        return self.t_fin - self.t_debut


@dataclass(frozen=True)
class ConfigurationAcquittee:
    """Configuration complète d'un BA, décodée des trames de configuration qu'il a acquittées.

    ``instant`` est la fin de la dernière trame de configuration acquittée.
    """

    configuration: Mapping[str, Any]
    instant: float


@dataclass(frozen=True)
class EchangeHTTP:
    """Un échange HTTP du BP capturé par le serveur legacy de banc.

    ``brut`` est la requête telle que reçue, octet pour octet : ligne de requête, en-têtes dans
    leur ordre, ligne vide et corps. C'est elle que compare NR-board-sc944d-11 : le protocole
    legacy IoT a une sérialisation propre (clés ``k``/``v`` non quotées, en-têtes dans un ordre
    fixe), qu'une comparaison des seuls champs décodés ne verrait pas changer.

    Les autres champs sont un décodage d'aide au diagnostic. ``indices`` porte les couples
    index → valeur encodée de la table d'échange présents dans la requête, tels que le BP les a
    codés (aucune réinterprétation).
    """

    t: float
    methode: str
    chemin: str
    brut: bytes = b""
    parametres: Mapping[str, str] = field(default_factory=dict)
    entetes: Sequence[str] = ()
    indices: Mapping[int, str] = field(default_factory=dict)
    statut: int | None = None


@dataclass(frozen=True)
class CompteursBP:
    """Compteurs de la console de diagnostic du BP (containment › Télémétrie de l'état de chaque BA)."""

    recuperations_bus: int
    cycles_alimentation: int
    defauts_simultanes: int
    commandes_expirees: int


@dataclass(frozen=True)
class CommandeEnAttente:
    """Commande conservée par le BP pour un BA, telle que la montre la console de diagnostic."""

    ba: int
    sortie: int
    etat: Any
    acquittee: bool
    expiree: bool = False


# --------------------------------------------------------------------------- base


class Instrument(abc.ABC):
    """Base de tout instrument : refuse l'instanciation hors d'un banc autorisé."""

    role = "instrument"

    def __init__(self, banc: str, parametres: Mapping[str, Any] | None = None) -> None:
        autorise = banc_autorise()
        if autorise is None or banc != autorise:
            raise StimulusInterdit(
                f"instrument « {self.role} » refusé : aucun banc autorisé pour « {banc} » "
                "(déclarer ESSENSYS_BENCH et un banc reconnu par le registre)."
            )
        self.banc = banc
        self.parametres: dict[str, Any] = dict(parametres or {})
        journaliser("instanciation", role=self.role, classe=type(self).__qualname__, banc=banc)

    def ouvrir(self) -> None:
        """Établit la liaison avec l'instrument, sans appliquer de stimulus."""

    def fermer(self) -> None:
        """Libère la liaison avec l'instrument."""

    @abc.abstractmethod
    def etat_sur(self) -> None:
        """Ramène l'instrument dans un état sans stimulus."""


# --------------------------------------------------------------------------- alimentation


class AlimentationLabo(Instrument):
    """Alimentation de laboratoire flottante du banc.

    Voies : ``"bp"`` (22 à 24 V, alimentation du BP) et, si déclarée, ``"ba_seul"`` (15,0 V, BA
    mesuré seul par J1-4/J1-3). Les limites de chaque voie viennent du registre (paramètre
    ``limites``) ; toute consigne hors limites est refusée avant d'atteindre l'instrument.
    """

    role = "alimentation"

    @property
    def limites(self) -> Mapping[str, LimitesVoie]:
        return self.parametres.get("limites", {})

    def regler(self, voie: str, tension_v: float, courant_max_a: float) -> None:
        limites = self.limites.get(voie)
        if limites is None:
            raise StimulusInterdit(f"voie d'alimentation « {voie} » non déclarée pour ce banc")
        if not (limites.min_v <= tension_v <= limites.max_v) or not (0 < courant_max_a <= limites.max_a):
            raise StimulusInterdit(
                f"consigne {tension_v} V / {courant_max_a} A hors des limites de la voie « {voie} » "
                f"({limites.min_v}–{limites.max_v} V, {limites.max_a} A au plus)"
            )
        self._regler(voie, tension_v, courant_max_a)

    @abc.abstractmethod
    def _regler(self, voie: str, tension_v: float, courant_max_a: float) -> None: ...

    @abc.abstractmethod
    def activer(self, voie: str) -> None: ...

    @abc.abstractmethod
    def couper(self, voie: str) -> None: ...

    @abc.abstractmethod
    def mesurer_tension(self, voie: str) -> float: ...

    @abc.abstractmethod
    def mesurer_courant(self, voie: str) -> float: ...

    def etat_sur(self) -> None:
        for voie in self.limites:
            self.couper(voie)


# --------------------------------------------------------------------------- oscilloscope


class Oscilloscope(Instrument):
    """Oscilloscope et voltmètre du banc (sondes isolées).

    Points de mesure nommés (voir ``seuils.point_*``) : ``"MCF"`` (broches 28/29 du MCF52259),
    ``"PIC:0x11"`` (broche RC7/RC6 du PIC), ``"J1:0x11"``, ``"J31:BP"`` (lignes du bus au
    connecteur J31), ``"+5V:0x11"``, ``"+5VS:BP"``, ``"+15V:entree:0x11"`` (BA_ALIM au connecteur
    du BA), ``"+15V:J31-4"``, et ``"+15V:port:0x11"``. Ce dernier est la sortie +15 V du port du
    BA dans l'étoile N2 ou la carte d'interconnexion, **en aval de la PTC et de l'écrêteur du port
    et en amont de son cavalier** : il reste mesurable cavalier ouvert. Le court-circuit
    ``"port:0x11"`` est appliqué de l'autre côté du cavalier, au connecteur du port. La
    correspondance avec les voies de l'instrument est dans la fiche du banc.
    """

    role = "oscilloscope"

    @abc.abstractmethod
    def niveau_bas(self, point: str, ligne: Ligne, emetteur: str) -> float:
        """Niveau bas (V) de ``ligne`` au ``point`` pendant que ``emetteur`` (« BP » ou « 0x11 »)
        tire la ligne, relevé sur une phase du protocole déclenchée par décodage."""

    @abc.abstractmethod
    def temps_montee(self, point: str, ligne: Ligne, debut_v: float, fin_v: float) -> float:
        """Temps de montée (s) de ``ligne`` au ``point``, de ``debut_v`` à ``fin_v``, pire cas
        sur une acquisition de trafic réel."""

    @abc.abstractmethod
    def tension_continue(self, point: str) -> float:
        """Tension continue (V) au ``point``."""

    @abc.abstractmethod
    def capacite_ligne(self, point: str, ligne: Ligne) -> float:
        """Capacité (F) de ``ligne`` vue au ``point``, BP compris (critère du MIB). La méthode
        (pont de mesure, ou constante de temps sous une pull-up connue) est décrite dans la fiche
        du banc ; elle n'applique aucun stimulus aux cartes."""

    def etat_sur(self) -> None:
        """Instrument passif : aucun stimulus à retirer."""


# --------------------------------------------------------------------------- analyseur logique


class AnalyseurLogique(Instrument):
    """Analyseur logique à sondes isolées, en acquisition continue sur le bus.

    ``point="bus"`` désigne le bus côté J31 ; ``point="ba_mib"`` le côté BA d'un module
    d'isolement (MIB).
    """

    role = "analyseur"

    @abc.abstractmethod
    def trames(self, depuis: float, jusqu_a: float | None = None, adresse: int | None = None) -> list[TrameI2C]:
        """Transactions du BP décodées dans la fenêtre, triées par ``t_debut``."""

    @abc.abstractmethod
    def intervalles_bas(
        self, ligne: Ligne, depuis: float, jusqu_a: float | None = None, point: Cote = "bus"
    ) -> list[tuple[float, float]]:
        """Intervalles (début, fin) pendant lesquels ``ligne`` est à l'état bas. Un intervalle en
        cours à ``jusqu_a`` (ou à l'instant présent) se termine à cette borne."""

    @abc.abstractmethod
    def fronts(self, ligne: Ligne, depuis: float, jusqu_a: float | None = None, point: Cote = "bus") -> list[float]:
        """Instants de tous les fronts (montants et descendants) de ``ligne``."""

    @abc.abstractmethod
    def configuration_acquittee(self, ba: int, depuis: float) -> ConfigurationAcquittee | None:
        """Dernière configuration complète (modes des variateurs, temps d'extinction, temps des
        volets) décodée des trames de configuration acquittées par ``ba`` depuis ``depuis``, ou
        ``None`` si l'une des trames de configuration n'a pas été acquittée depuis. Les clés sont
        celles de ``MaitreCommande.configuration``."""

    def etat_sur(self) -> None:
        """Instrument passif : aucun stimulus à retirer."""


# --------------------------------------------------------------------------- générateur d'injection


class GenerateurInjection(Instrument):
    """Générateur d'onde de choc ou d'injection (tâches 7.16 à 7.19, hors NR-1 à NR-13).

    N'injecte que sur une carte déclarée échantillon d'injection dans le registre (paramètre
    ``echantillons``) : jamais sur une carte destinée à une installation.
    """

    role = "injection"

    def armer(self, onde: str, crete: float, point: str, cible: str) -> None:
        if cible not in set(self.parametres.get("echantillons", ())):
            raise StimulusInterdit(f"injection refusée : « {cible} » n'est pas un échantillon d'injection du banc")
        self._armer(onde, crete, point, cible)

    @abc.abstractmethod
    def _armer(self, onde: str, crete: float, point: str, cible: str) -> None: ...

    @abc.abstractmethod
    def declencher(self) -> None: ...

    @abc.abstractmethod
    def desarmer(self) -> None: ...

    def etat_sur(self) -> None:
        self.desarmer()


# --------------------------------------------------------------------------- relais de banc


class RelaisBanc(Instrument):
    """Relais et mires du banc, et lecture des charges factices.

    Le banc commande : la sélection du câble, la déconnexion d'un BA par son câble ou par le
    cavalier de son port, les mires à transistor qui forcent SDA ou SCL à l'état bas, les
    courts-circuits du +15 V, l'isolement d'une carte pour la mesure de fuite, et l'ordre de
    défaut envoyé au firmware d'essai d'un BA de banc. Il lit l'état des charges factices
    raccordées aux sorties des BA (aucune tension secteur sur les sorties).
    """

    role = "relais"

    @abc.abstractmethod
    def selectionner_cable(self, configuration: str) -> None:
        """Monte une configuration de câble (``seuils.CONFIGURATIONS_CABLE*``) :

        - ``"3x33cm"`` (configuration nominale, ``seuils.CABLE_NOMINAL``), ``"1m"`` ou ``"3m"`` :
          bus non isolé en cat. 5e AWG26 ;
        - ``"mib-1m"`` ou ``"mib-3m"`` : liaison AWG20 du BA déporté derrière son MIB (banc avec
          la capacité ``mib``).
        """

    @abc.abstractmethod
    def deconnecter_ba(self, ba: int, par: Deconnexion) -> None:
        """Déconnecte le BA par son câble ou par le cavalier de son port. Idempotente."""

    @abc.abstractmethod
    def reconnecter_ba(self, ba: int, par: Deconnexion) -> None:
        """Reconnecte le BA. Idempotente : sans effet sur un BA déjà connecté."""

    @abc.abstractmethod
    def forcer_ligne_basse(self, ligne: Ligne, point: Cote = "bus") -> None: ...

    @abc.abstractmethod
    def relacher_ligne(self, ligne: Ligne, point: Cote = "bus") -> None: ...

    @abc.abstractmethod
    def court_circuiter_15v(self, cible: str) -> None:
        """Court-circuite au 0 V le +15 V de ``cible`` :

        - ``"J31-4"`` : sortie BA_ALIM du BP, entre J31-4 et J31-3 ;
        - ``"port:0x11"`` : connecteur du port du BA dans l'étoile N2 ou la carte
          d'interconnexion, côté BA du cavalier (capacité ``court_circuit_15v``) ;
        - ``"+Vin:0x11"`` : +Vin d'un BA retouché, aux bornes de son écrêteur, en aval de sa PTC
          (capacité ``court_circuit_vin_ba``).
        """

    @abc.abstractmethod
    def supprimer_court_circuit(self, cible: str) -> None: ...

    @abc.abstractmethod
    def isoler_carte(self, carte: str) -> None:
        """Isole ``carte`` (« BP » ou « 0x11 ») du bus pour une mesure carte seule ; un BA isolé est
        alimenté par la voie ``"ba_seul"``."""

    @abc.abstractmethod
    def reintegrer_carte(self, carte: str) -> None: ...

    def declencher_defaut_ba(self, ba: int, defaut: str) -> None:
        if defaut not in DEFAUTS_BA:
            raise ValueError(f"défaut de BA inconnu : {defaut}")
        self._declencher_defaut_ba(ba, defaut)

    @abc.abstractmethod
    def _declencher_defaut_ba(self, ba: int, defaut: str) -> None: ...

    @abc.abstractmethod
    def annuler_defaut_ba(self, ba: int) -> None: ...

    @abc.abstractmethod
    def lire_sortie(self, ba: int, sortie: int) -> bool:
        """État de la charge factice raccordée à la sortie ``sortie`` du BA."""

    @abc.abstractmethod
    def historique_sortie(self, ba: int, sortie: int, depuis: float) -> list[tuple[float, bool]]:
        """Changements d'état (instant, nouvel état) de la charge factice depuis ``depuis``."""

    @abc.abstractmethod
    def etat_sur(self) -> None:
        """Relâche toutes les lignes, supprime tout court-circuit, annule tout défaut de BA,
        reconnecte tous les BA et réintègre toutes les cartes. Ne change pas la configuration de
        câble : ``Banc.remettre_etat_nominal`` remonte ensuite le câble nominal."""


# --------------------------------------------------------------------------- maître de commande


class MaitreCommande(Instrument):
    """Maître de commande : le serveur legacy local du banc, sur le réseau isolé.

    Le BP interroge ce serveur avec le protocole HTTP legacy IoT, inchangé. Le serveur de banc
    met les commandes à disposition du BP, enregistre les index remontés et capture le trafic.
    Il n'a aucun accès au serveur de production.
    """

    role = "maitre"

    @abc.abstractmethod
    def verifier_identite_banc(self) -> str:
        """Identifiant de banc **lu sur le serveur legacy de banc**, à son point d'accès dédié
        configuré côté serveur, puis comparé au banc déclaré par ``Banc.ouvrir``.

        Le pilote ne dérive jamais cette valeur de ``self.banc``, de ses paramètres ni du
        registre : c'est la seule barrière dynamique avant tout stimulus, et un point de la revue
        de PR de chaque pilote."""

    @abc.abstractmethod
    def commander_sortie(self, ba: int, sortie: int, etat: bool) -> str:
        """Met à disposition du BP une commande de sortie maintenue ; renvoie son identifiant."""

    @abc.abstractmethod
    def instants_recuperation(self, identifiants: Sequence[str]) -> Mapping[str, float | None]:
        """Pour chaque commande, instant où le BP l'a récupérée (réponse à ``GET /api/myactions``
        qui la contient), ou ``None`` si elle ne l'a pas encore été."""

    @abc.abstractmethod
    def commander_volet(self, ba: int, volet: int, sens: str) -> str:
        """Met à disposition du BP une commande de volet (``"monter"`` ou ``"descendre"``)."""

    @abc.abstractmethod
    def configuration(self, ba: int) -> dict[str, Any]:
        """Configuration du BA actuellement définie côté serveur de banc."""

    @abc.abstractmethod
    def variante_configuration(self, ba: int) -> dict[str, Any]:
        """Configuration valide du BA, différente de la configuration actuelle, déclarée dans la
        fiche du banc (modes de variateur différents du mode par défaut)."""

    @abc.abstractmethod
    def modifier_configuration(self, ba: int, configuration: Mapping[str, Any]) -> None: ...

    @abc.abstractmethod
    def historique_index(self, index: int, depuis: float, jusqu_a: float | None = None) -> list[tuple[float, int]]:
        """Valeurs (instant, valeur) de l'index remontées par le BP dans la fenêtre."""

    @abc.abstractmethod
    def instants_echanges_bp(self, depuis: float, jusqu_a: float | None = None) -> list[float]:
        """Instants des requêtes HTTP reçues du BP dans la fenêtre."""

    @abc.abstractmethod
    def capture_http(self, depuis: float, jusqu_a: float | None = None) -> list[EchangeHTTP]:
        """Requêtes du BP dans la fenêtre, chacune avec sa requête brute (``EchangeHTTP.brut``)."""

    @abc.abstractmethod
    def teleinfo_transmise(self, depuis: float, jusqu_a: float | None = None) -> bool:
        """Vrai si le BP a transmis des index de téléinformation dans la fenêtre."""

    @abc.abstractmethod
    def scenario_temoin_execute(self, depuis: float, jusqu_a: float | None = None) -> bool:
        """Vrai si le scénario local témoin du banc s'est exécuté dans la fenêtre, d'après la
        table d'échange remontée par le BP."""

    @abc.abstractmethod
    def version_bp(self) -> str: ...

    @abc.abstractmethod
    def vider_commandes(self) -> None:
        """Retire du serveur de banc les commandes non encore récupérées par le BP."""

    def etat_sur(self) -> None:
        self.vider_commandes()


# --------------------------------------------------------------------------- console de diagnostic


class ConsoleDiagnostic(Instrument):
    """Console de diagnostic locale du BP (liaison série « Espion »), en lecture."""

    role = "console"

    @abc.abstractmethod
    def compteurs(self) -> CompteursBP: ...

    @abc.abstractmethod
    def instants_demarrage(self, depuis: float, jusqu_a: float | None = None) -> list[float]:
        """Instants des démarrages du BP signalés par la console (bannière de démarrage)."""

    @abc.abstractmethod
    def instants_cycles_alimentation(self, depuis: float, jusqu_a: float | None = None) -> list[float]:
        """Instants des cycles d'alimentation des BA (TE2), datés par le BP."""

    @abc.abstractmethod
    def tentatives_recuperation(self, depuis: float, jusqu_a: float | None = None) -> list[float]:
        """Instants des tentatives de récupération de bus signalées par le BP."""

    @abc.abstractmethod
    def transactions_en_erreur(self, depuis: float, jusqu_a: float | None = None) -> list[tuple[float, float]]:
        """Transactions I2C terminées en erreur, (début, fin), horodatées par le firmware."""

    @abc.abstractmethod
    def erreurs_attente(self, depuis: float, ba: int | None = None) -> int:
        """Nombre de transactions abandonnées sur timeout d'attente depuis ``depuis``."""

    @abc.abstractmethod
    def commandes_en_attente(self, ba: int) -> list[CommandeEnAttente]: ...

    @abc.abstractmethod
    def commandes_expirees(self, depuis: float) -> list[CommandeEnAttente]: ...

    @abc.abstractmethod
    def etat_sante(self, ba: int) -> str:
        """« OK », « SUSPECT » ou « ABSENT »."""

    @abc.abstractmethod
    def tas_libre(self) -> int:
        """Octets libres du tas du BP."""

    @abc.abstractmethod
    def version_firmware(self) -> str: ...

    def etat_sur(self) -> None:
        """Console en lecture : aucun stimulus à retirer."""


# --------------------------------------------------------------------------- programmateur


class ProgrammateurBanc(Instrument):
    """Programmateur des cartes du banc (BDM pour le BP, ICSP pour les BA), pour NR-11.

    Il ne programme que les cartes du banc, dont le registre a vérifié la destination « banc ».
    Il ne sert jamais à flasher un BP en service : le déploiement d'un firmware sur un BP
    d'installation est une opération de maintenance distincte (tâche 5.12).
    """

    role = "programmateur"

    def monter(self, bp: str, ba: str) -> None:
        if bp not in FIRMWARES_BP or ba not in FIRMWARES_BA:
            raise ValueError(f"combinaison de firmwares inconnue : BP {bp}, BA {ba}")
        self._monter(bp, ba)

    @abc.abstractmethod
    def _monter(self, bp: str, ba: str) -> None: ...

    @abc.abstractmethod
    def firmwares_montes(self) -> tuple[str, str]:
        """Couple (firmware BP, firmware BA) actuellement monté sur le banc."""

    def etat_sur(self) -> None:
        """Aucun stimulus à retirer."""


#: Interfaces par rôle du registre.
INTERFACES: dict[str, type[Instrument]] = {
    "alimentation": AlimentationLabo,
    "oscilloscope": Oscilloscope,
    "analyseur": AnalyseurLogique,
    "relais": RelaisBanc,
    "maitre": MaitreCommande,
    "console": ConsoleDiagnostic,
    "injection": GenerateurInjection,
    "programmateur": ProgrammateurBanc,
}
