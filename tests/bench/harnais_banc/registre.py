"""Registre des bancs d'essai et garde-fous de déclaration (tâche 7.2).

Le banc est déclaré par la variable d'environnement ``ESSENSYS_BENCH=<identifiant>`` et décrit
dans le registre versionné ``tests/bench/benches.yaml``.

- Sans banc déclaré : le registre n'est même pas lu, aucun pilote n'est importé et les tests de
  banc sont « skipped ».
- Banc déclaré absent du registre, banc d'exemple ou banc non conforme : ``BancRefuse`` est levée
  avant tout stimulus ; ``conftest.py`` la convertit en ``pytest.UsageError``.

Un banc est conforme si, entre autres :

- son réseau est inclus dans un réseau de banc isolé déclaré par le registre, lui-même inclus dans
  une plage réservée aux essais : 198.18.0.0/15 (RFC 2544) en IPv4, 2001:2::/48 (RFC 5180) en
  IPv6. Les plages privées des installations (10/8, 172.16/12, 192.168/16…) sont donc refusées ;
- chaque cible réseau est une adresse IP littérale de ce réseau. Sont contrôlés : l'adresse du BP,
  le serveur legacy de banc, et dans les paramètres des instruments, toute valeur rangée sous une
  clé de cible (``adresse``, ``addr``, ``host``, ``hote``, ``ip``, ``serveur``, ``server``,
  ``url``, ``endpoint``, ``cible``…), toute URL, toute ressource VISA TCPIP et toute adresse IPv4
  ou IPv6 citée. Un nom d'hôte est refusé, car il pourrait désigner un serveur de production ;
- l'URL du pilote du serveur legacy (``maitre``) désigne le même hôte et le même port que
  ``reseau.serveur_legacy`` ;
- toutes ses cartes ont la destination « banc » (jamais une carte d'installation) ;
- ses alimentations restent dans les plages du banc (22 à 24 V pour le BP).
"""

from __future__ import annotations

import ipaddress
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterator, Mapping
from urllib.parse import urlsplit

from . import seuils
from .instruments import FIRMWARES_BA, INTERFACES, LimitesVoie

ENV_BANC = "ESSENSYS_BENCH"
CHEMIN_REGISTRE = Path(__file__).resolve().parent.parent / "benches.yaml"
#: Chemin affiché dans les messages : jamais de chemin local absolu (dépôt public).
CHEMIN_REGISTRE_AFFICHE = "tests/bench/benches.yaml"
PREFIXE = "Banc d'essai : "
VERSION_REGISTRE = 1

#: Plages réservées aux essais d'équipements réseau, jamais utilisées par une installation : tout
#: réseau de banc isolé déclaré doit y être inclus.
PLAGES_ESSAI = (
    ipaddress.ip_network("198.18.0.0/15"),  # RFC 2544
    ipaddress.ip_network("2001:2::/48"),  # RFC 5180
)

ROLES_OBLIGATOIRES = ("alimentation", "oscilloscope", "analyseur", "relais", "maitre", "console")
ROLES_OPTIONNELS = ("injection", "programmateur")

#: Capacités qu'un banc peut déclarer ; un test qui en requiert une absente est « skipped ».
CAPACITES_CONNUES = frozenset(
    {
        "cavaliers_par_port",  # déconnexion d'un BA par le cavalier de son port (N2, interconnexion)
        "sorties_15v_par_port",  # sortie +15 V protégée par port (N2, interconnexion)
        "court_circuit_15v",  # relais de court-circuit du +15 V (port, J31-4)
        "firmware_essai_ba",  # BA de banc équipé du firmware d'essai qui provoque les défauts
        "teleinfo",  # source de téléinformation simulée raccordée au BP
        "scenario_temoin",  # scénario local témoin programmé sur le BP du banc
        "mib",  # BA raccordé par un module d'isolement
        "court_circuit_vin_ba",  # BA retouchés (PTC et écrêteur sur +Vin), relais aux bornes de l'écrêteur
        "injection",  # générateur d'injection déclaré
        "programmateur",  # programmateur BDM/ICSP déclaré
    }
)

TOPOLOGIES = ("chaine", "etoile")
#: Mots d'une clé de paramètre qui désignent une cible réseau (clé découpée en mots).
MOTS_CLE_CIBLE = frozenset(
    {"adresse", "adresses", "addr", "address", "host", "hote", "hostname", "ip", "ipv4", "ipv6",
     "serveur", "server", "url", "uri", "endpoint", "cible", "target"}
)
PREFIXES_CLE_CIBLE = ("addr", "adress", "host", "hote", "serveur", "server", "endpoint", "url")
MOTIF_VISA_TCPIP = re.compile(r"^TCPIP\d*::([^:]+)::", re.IGNORECASE)
MOTIF_IPV4 = re.compile(r"(?<![\d.])(\d{1,3}(?:\.\d{1,3}){3})(?![\d.])")
MOTIF_HOTE_IPV6_PORT = re.compile(r"^\[([^\]]+)\](?::\d+)?$")
MOTIF_SEPARATEURS = re.compile(r"[\s,;()\"'<>]+")
MOTIF_PILOTE = re.compile(r"^[A-Za-z_][\w.]*:[A-Za-z_]\w*$")
MOTIF_IDENTIFIANT = re.compile(r"^[a-z0-9][a-z0-9-]{1,62}$")
#: Chemins locaux interdits dans un registre publié (dépôt public).
MOTIF_CHEMIN_LOCAL = re.compile(r"(^~|/Users/|/home/|/private/|[A-Za-z]:\\Users\\)")


class BancRefuse(Exception):
    """Banc refusé : le harnais s'arrête en erreur sans appliquer de stimulus."""

    def __init__(self, message: str) -> None:
        super().__init__(PREFIXE + message)


class BancInconnu(BancRefuse):
    """Banc déclaré absent du registre."""


@dataclass(frozen=True)
class InstrumentDeclare:
    role: str
    pilote: str
    parametres: Mapping[str, Any]


@dataclass(frozen=True)
class ConfigBanc:
    """Banc validé par le registre."""

    identifiant: str
    description: str
    fiche: str
    reseau: ipaddress.IPv4Network | ipaddress.IPv6Network
    adresse_bp: str
    serveur_legacy: str
    topologie: str
    ordre_ba: tuple[int, ...]
    ba: Mapping[int, Mapping[str, Any]]
    capacites: frozenset[str]
    alimentations: Mapping[str, LimitesVoie]
    instruments: Mapping[str, InstrumentDeclare]
    cartes: Mapping[str, Mapping[str, Any]]
    ba_essai: int | None
    firmware_ba_essai: str | None
    ba_mib: int | None
    volet_temoin: tuple[int, int]
    sorties_volet: tuple[int, ...]
    delai_expiration_volet_s: float
    indices_http_variables: frozenset[int]
    entetes_http_variables: frozenset[str]
    pullup_bp_ohm: float

    @property
    def ba_le_plus_eloigne(self) -> int:
        return self.ordre_ba[-1]

    @property
    def echantillons_injection(self) -> frozenset[str]:
        return frozenset(nom for nom, carte in self.cartes.items() if carte.get("echantillon_injection"))


# --------------------------------------------------------------------------- déclaration et lecture


def banc_declare(environ: Mapping[str, str] | None = None) -> str | None:
    """Identifiant du banc déclaré par ``ESSENSYS_BENCH``, ou ``None``."""
    valeur = (os.environ if environ is None else environ).get(ENV_BANC, "")
    return valeur.strip() or None


def charger_registre(chemin: Path = CHEMIN_REGISTRE) -> dict[str, Any]:
    try:
        import yaml
    except ImportError:
        raise BancRefuse("PyYAML est requis pour lire le registre des bancs (uv run --with pyyaml …).") from None
    try:
        donnees = yaml.safe_load(chemin.read_text(encoding="utf-8"))
    except (OSError, yaml.YAMLError) as exc:
        raise BancRefuse(f"registre {CHEMIN_REGISTRE_AFFICHE} illisible ({type(exc).__name__}).") from None
    if not isinstance(donnees, dict) or donnees.get("version") != VERSION_REGISTRE:
        raise BancRefuse(f"registre {CHEMIN_REGISTRE_AFFICHE} : version {VERSION_REGISTRE} attendue.")
    return donnees


def reseaux_isoles(registre: Mapping[str, Any]) -> tuple[ipaddress.IPv4Network | ipaddress.IPv6Network, ...]:
    """Réseaux de banc isolés déclarés par le registre, après contrôle."""
    reseaux = []
    for entree in registre.get("reseaux_banc_isoles") or []:
        cidr = entree.get("cidr") if isinstance(entree, dict) else entree
        try:
            reseau = ipaddress.ip_network(str(cidr), strict=True)
        except ValueError:
            raise BancRefuse(f"réseau de banc isolé invalide dans le registre : {cidr!r}.") from None
        if not any(reseau.version == plage.version and reseau.subnet_of(plage) for plage in PLAGES_ESSAI):
            raise BancRefuse(
                f"réseau de banc isolé {reseau} refusé : hors des plages réservées aux essais "
                f"({', '.join(str(plage) for plage in PLAGES_ESSAI)}), il pourrait être celui d'une installation."
            )
        reseaux.append(reseau)
    if not reseaux:
        raise BancRefuse("le registre ne déclare aucun réseau de banc isolé.")
    return tuple(reseaux)


def resoudre(identifiant: str, registre: Mapping[str, Any] | None = None) -> ConfigBanc:
    """Banc validé pour ``identifiant`` ; ``BancRefuse`` si inconnu ou non conforme."""
    registre = charger_registre() if registre is None else registre
    bancs = registre.get("bancs") or {}
    if identifiant not in bancs:
        executables = sorted(
            nom for nom, entree in bancs.items() if isinstance(entree, dict) and not entree.get("exemple")
        )
        raise BancInconnu(
            f"le banc « {identifiant} » déclaré par {ENV_BANC} n'est pas dans le registre "
            f"{CHEMIN_REGISTRE_AFFICHE} (bancs exécutables : {', '.join(executables) or 'aucun'}). "
            "Arrêt avant tout stimulus."
        )
    return valider_banc(identifiant, bancs[identifiant], reseaux_isoles(registre))


# --------------------------------------------------------------------------- validation


def _adresse(valeur: Any) -> ipaddress.IPv4Address | ipaddress.IPv6Address | None:
    try:
        return ipaddress.ip_address(str(valeur).strip("[]"))
    except ValueError:
        return None


def _est_cle_cible(cle: str) -> bool:
    """Vrai si la clé de paramètre désigne une cible réseau (``adresse``, ``hostname``, ``ip_bp``…)."""
    mots = [mot for mot in re.split(r"[^a-z0-9]+", cle.lower()) if mot]
    return any(mot in MOTS_CLE_CIBLE or mot.startswith(PREFIXES_CLE_CIBLE) for mot in mots)


def _hote_cible(valeur: str) -> str:
    """Hôte désigné par la valeur d'une clé de cible : URL, ressource VISA TCPIP, ``[ipv6]:port``,
    ``hôte:port`` ou hôte seul. Une URL sans hôte donne une chaîne vide."""
    valeur = valeur.strip()
    if "://" in valeur:
        return urlsplit(valeur).hostname or ""
    visa = MOTIF_VISA_TCPIP.match(valeur)
    if visa:
        return visa.group(1)
    if _adresse(valeur) is not None:
        return valeur
    crochets = MOTIF_HOTE_IPV6_PORT.match(valeur)
    if crochets:
        return crochets.group(1)
    hote, separateur, port = valeur.rpartition(":")
    if separateur and port.isdigit() and hote and ":" not in hote:
        return hote
    return valeur


def _cibles_reseau(valeur: Any, cle: str | None = None) -> Iterator[str]:
    """Hôtes désignés par un paramètre d'instrument : valeur d'une clé de cible, URL, ressource VISA
    TCPIP, adresse IPv4 ou IPv6 citée n'importe où."""
    if isinstance(valeur, Mapping):
        for sous_cle, sous_valeur in valeur.items():
            yield from _cibles_reseau(sous_valeur, str(sous_cle))
        return
    if isinstance(valeur, (list, tuple)):
        for element in valeur:
            yield from _cibles_reseau(element, cle)
        return
    if not isinstance(valeur, str):
        return
    if cle is not None and _est_cle_cible(cle):
        yield _hote_cible(valeur)
    if "://" in valeur:
        hote = urlsplit(valeur).hostname
        if hote:
            yield hote
    visa = MOTIF_VISA_TCPIP.match(valeur)
    if visa:
        yield visa.group(1)
    yield from MOTIF_IPV4.findall(valeur)
    for jeton in MOTIF_SEPARATEURS.split(valeur):
        crochets = MOTIF_HOTE_IPV6_PORT.match(jeton)
        candidat = crochets.group(1) if crochets else jeton
        if candidat.count(":") >= 2 and isinstance(_adresse(candidat), ipaddress.IPv6Address):
            yield candidat


def _hote_port(url: str) -> tuple[str | None, int | None]:
    morceaux = urlsplit(url)
    try:
        port = morceaux.port
    except ValueError:
        return None, None
    defaut = {"http": 80, "https": 443}.get(morceaux.scheme)
    return morceaux.hostname, port if port is not None else defaut


def _chaines(valeur: Any) -> Iterator[str]:
    if isinstance(valeur, Mapping):
        for sous_valeur in valeur.values():
            yield from _chaines(sous_valeur)
    elif isinstance(valeur, (list, tuple)):
        for element in valeur:
            yield from _chaines(element)
    elif isinstance(valeur, str):
        yield valeur


def _verifier_cible(hote: str, reseau, origine: str, erreurs: list[str]) -> None:
    if not hote:
        erreurs.append(f"{origine} : adresse absente")
        return
    adresse = _adresse(hote)
    if adresse is None:
        erreurs.append(
            f"{origine} : nom d'hôte « {hote} » refusé, seules les adresses IP littérales du réseau de banc sont admises"
        )
    elif reseau is not None and adresse not in reseau:
        erreurs.append(f"{origine} : {adresse} hors du réseau de banc {reseau}")


def _flottant(valeur: Any) -> float | None:
    try:
        return float(valeur)
    except (TypeError, ValueError):
        return None


def valider_banc(
    identifiant: str,
    entree: Any,
    reseaux: tuple[ipaddress.IPv4Network | ipaddress.IPv6Network, ...],
) -> ConfigBanc:
    """Valide une entrée du registre ; lève ``BancRefuse`` avec la liste de tous les motifs."""
    if not isinstance(entree, Mapping):
        raise BancRefuse(f"le banc « {identifiant} » n'a pas de description valide dans le registre.")
    if entree.get("exemple"):
        raise BancRefuse(
            f"le banc « {identifiant} » est un exemple documenté du registre, non exécutable. "
            "Déclarer un banc réel décrit par sa fiche (tâche 7.1). Arrêt avant tout stimulus."
        )

    erreurs: list[str] = []
    if not MOTIF_IDENTIFIANT.match(identifiant):
        erreurs.append("identifiant invalide (minuscules, chiffres et tirets)")
    for chaine in _chaines(entree):
        if MOTIF_CHEMIN_LOCAL.search(chaine):
            erreurs.append("chemin local absolu interdit dans le registre (dépôt public)")
            break
    fiche = entree.get("fiche")
    if not isinstance(fiche, str) or not fiche.strip():
        erreurs.append("fiche du banc (tâche 7.1) non référencée")

    # --- réseau isolé
    reseau_brut = entree.get("reseau") or {}
    reseau = None
    try:
        reseau = ipaddress.ip_network(str(reseau_brut.get("cidr")), strict=True)
    except ValueError:
        erreurs.append(f"réseau du banc invalide : {reseau_brut.get('cidr')!r}")
    if reseau is not None and not any(
        reseau.version == isole.version and reseau.subnet_of(isole) for isole in reseaux
    ):
        erreurs.append(
            f"réseau {reseau} hors des réseaux de banc isolés déclarés "
            f"({', '.join(str(r) for r in reseaux)}) : jamais le réseau d'une installation"
        )
    adresse_bp = str(reseau_brut.get("bp", ""))
    _verifier_cible(adresse_bp, reseau, "adresse du BP", erreurs)
    serveur = str(reseau_brut.get("serveur_legacy", ""))
    morceaux = urlsplit(serveur)
    if morceaux.scheme not in ("http", "https") or not morceaux.hostname:
        erreurs.append("serveur legacy de banc : URL http(s) avec une adresse IP littérale attendue")
    else:
        _verifier_cible(morceaux.hostname, reseau, "serveur legacy de banc", erreurs)

    # --- instruments
    instruments: dict[str, InstrumentDeclare] = {}
    declares = entree.get("instruments") or {}
    for role in ROLES_OBLIGATOIRES:
        if role not in declares:
            erreurs.append(f"instrument obligatoire absent : {role}")
    for role, description in declares.items():
        if role not in INTERFACES:
            erreurs.append(f"rôle d'instrument inconnu : {role}")
            continue
        description = description or {}
        pilote = str(description.get("pilote", ""))
        if not MOTIF_PILOTE.match(pilote):
            erreurs.append(f"{role} : pilote « module:Classe » attendu")
        parametres = description.get("parametres") or {}
        for hote in _cibles_reseau(parametres):
            _verifier_cible(hote, reseau, f"instrument {role}", erreurs)
        instruments[role] = InstrumentDeclare(role, pilote, dict(parametres))
    maitre = instruments.get("maitre")
    if maitre is not None and _hote_port(str(maitre.parametres.get("url", ""))) != _hote_port(serveur):
        erreurs.append(
            "instrument maitre : paramètre url attendu, avec le même hôte et le même port que reseau.serveur_legacy"
        )

    # --- BA, topologie et capacités
    ba_brut = entree.get("ba") or {}
    ba: dict[int, dict[str, Any]] = {}
    for adresse, description in ba_brut.items():
        if adresse not in seuils.ADRESSES_BA:
            erreurs.append(f"adresse de BA inconnue : {adresse!r}")
            continue
        description = dict(description or {})
        sorties = description.get("sorties") or []
        if description.get("sortie_temoin") not in sorties:
            erreurs.append(f"BA {adresse:#04x} : sortie témoin absente de la liste des sorties")
        ba[adresse] = description
    if sorted(ba) != sorted(seuils.ADRESSES_BA):
        erreurs.append("le banc compte les trois BA, un de chaque type (0x11, 0x12, 0x13 ; tâche 7.1)")
    topologie = entree.get("topologie")
    if topologie not in TOPOLOGIES:
        erreurs.append(f"topologie attendue parmi {', '.join(TOPOLOGIES)}")
    ordre_ba = tuple(entree.get("ordre_ba") or ())
    if sorted(ordre_ba) != sorted(ba):
        erreurs.append("ordre_ba doit lister chaque BA déclaré une fois, du plus proche au plus éloigné du BP")

    capacites = set(entree.get("capacites") or ())
    inconnues = capacites - CAPACITES_CONNUES
    if inconnues:
        erreurs.append(f"capacités inconnues : {', '.join(sorted(map(str, inconnues)))}")
    capacites |= {role for role in ROLES_OPTIONNELS if role in instruments}

    ba_essai = entree.get("ba_essai")
    firmware_ba_essai = entree.get("firmware_ba_essai")
    firmware_ba_essai = None if firmware_ba_essai is None else str(firmware_ba_essai)
    if "firmware_essai_ba" in capacites:
        if ba_essai not in ba:
            erreurs.append("ba_essai : BA équipé du firmware d'essai attendu avec la capacité firmware_essai_ba")
        if firmware_ba_essai not in FIRMWARES_BA:
            erreurs.append(
                "firmware_ba_essai : version de base du firmware d'essai (« 1.7 » ou « 1.8 ») attendue "
                "avec la capacité firmware_essai_ba"
            )
    ba_mib = entree.get("ba_mib")
    if "mib" in capacites and ba_mib not in ba:
        erreurs.append("ba_mib : BA raccordé par un MIB attendu avec la capacité mib")
    volet = entree.get("volet_temoin") or {}
    sorties_volet = tuple(volet.get("sorties") or ())
    if volet.get("ba") not in ba or not isinstance(volet.get("volet"), int) or not sorties_volet:
        erreurs.append("volet_temoin : BA, numéro de volet et sorties du moteur (charges factices) attendus")
    pullup_bp = _flottant(entree.get("pullup_bp_ohm", seuils.PULLUP_BP_ORIGINE_OHM))
    if pullup_bp not in seuils.PULLUPS_BP_ADMISES_OHM:
        erreurs.append("pullup_bp_ohm : 1000, 1500 ou 2200 (R155/R156 du BP du banc)")
    delai_volet = _flottant(entree.get("delai_expiration_volet_s", seuils.DELAI_EXPIRATION_VOLET_DEFAUT_S))
    if delai_volet is None or delai_volet <= 0:
        erreurs.append("delai_expiration_volet_s : durée positive attendue")

    # --- cartes : uniquement des cartes de banc
    cartes_brutes = entree.get("cartes") or {}
    cartes: dict[str, dict[str, Any]] = {}
    attendues = ["BP"] + [seuils.carte_ba(adresse) for adresse in ba]
    for nom, description in cartes_brutes.items():
        cle = seuils.carte_ba(nom) if isinstance(nom, int) else str(nom)
        cartes[cle] = dict(description or {})
    for nom in attendues:
        if nom not in cartes:
            erreurs.append(f"carte {nom} non déclarée")
        elif cartes[nom].get("destination") != "banc":
            erreurs.append(f"carte {nom} : destination « banc » obligatoire (jamais une carte d'installation)")

    # --- alimentations
    alimentations: dict[str, LimitesVoie] = {}
    for voie, limites in (entree.get("alimentations") or {}).items():
        limites = limites or {}
        min_v, max_v, max_a = (_flottant(limites.get(cle)) for cle in ("min_v", "max_v", "max_a"))
        if None in (min_v, max_v, max_a) or not (0 < min_v <= max_v) or max_a <= 0:
            erreurs.append(f"alimentation {voie} : min_v, max_v et max_a positifs attendus")
            continue
        if voie == "bp" and not (seuils.TENSION_MIN_ALIM_BP_V <= min_v and max_v <= seuils.TENSION_MAX_ALIM_BP_V):
            erreurs.append(
                f"alimentation bp hors de la plage du banc ({seuils.TENSION_MIN_ALIM_BP_V}–{seuils.TENSION_MAX_ALIM_BP_V} V)"
            )
        if voie == "ba_seul" and max_v > seuils.TENSION_MAX_ALIM_BA_SEUL_V:
            erreurs.append(f"alimentation ba_seul au-delà de {seuils.TENSION_MAX_ALIM_BA_SEUL_V} V")
        alimentations[str(voie)] = LimitesVoie(min_v, max_v, max_a)
    for voie in ("bp", "ba_seul"):
        if voie not in alimentations:
            erreurs.append(f"alimentation {voie} non déclarée")

    indices_variables = entree.get("indices_http_variables") or []
    if not all(isinstance(index, int) for index in indices_variables):
        erreurs.append("indices_http_variables : liste d'index entiers attendue")
    entetes_variables = entree.get("entetes_http_variables") or []
    if not all(isinstance(nom, str) and nom.strip() for nom in entetes_variables):
        erreurs.append("entetes_http_variables : liste de noms d'en-têtes attendue")

    erreurs = list(dict.fromkeys(erreurs))

    if erreurs:
        raise BancRefuse(
            f"le banc « {identifiant} » est refusé, aucun stimulus n'est appliqué :\n  - " + "\n  - ".join(erreurs)
        )

    return ConfigBanc(
        identifiant=identifiant,
        description=str(entree.get("description", "")),
        fiche=str(fiche),
        reseau=reseau,
        adresse_bp=adresse_bp,
        serveur_legacy=serveur,
        topologie=str(topologie),
        ordre_ba=ordre_ba,
        ba=ba,
        capacites=frozenset(capacites),
        alimentations=alimentations,
        instruments=instruments,
        cartes=cartes,
        ba_essai=ba_essai,
        firmware_ba_essai=firmware_ba_essai,
        ba_mib=ba_mib,
        volet_temoin=(int(volet["ba"]), int(volet["volet"])),
        sorties_volet=tuple(int(sortie) for sortie in sorties_volet),
        delai_expiration_volet_s=float(delai_volet),
        indices_http_variables=frozenset(indices_variables),
        entetes_http_variables=frozenset(nom.strip().lower() for nom in entetes_variables),
        pullup_bp_ohm=float(pullup_bp),
    )
