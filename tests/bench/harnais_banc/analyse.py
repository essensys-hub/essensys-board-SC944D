"""Fonctions d'analyse pures des mesures de banc.

Elles n'accèdent à aucun instrument : elles sont vérifiées hors banc par ``test_analyse.py``.
"""

from __future__ import annotations

import re
from bisect import bisect_left, bisect_right
from typing import Iterable, Mapping, NamedTuple, Sequence

from . import seuils
from .instruments import EchangeHTTP, TrameI2C

Intervalle = tuple[float, float]


# --------------------------------------------------------------------------- index 12


def bits_ba(index12: int | None) -> int | None:
    """Bits des BA de l'index 12 : ``index 12 & 0x1C``."""
    return None if index12 is None else index12 & seuils.MASQUE_BA_INDEX12


def bits_hors_ba(index12: int | None) -> int | None:
    """Bits 0 (compteur) et 1 (IHM) de l'index 12."""
    return None if index12 is None else index12 & seuils.MASQUE_HORS_BA_INDEX12


def premier_instant_bits_ba(
    historique: Sequence[tuple[float, int]], attendu: int, depuis: float
) -> float | None:
    """Premier instant, après ``depuis``, où ``index 12 & 0x1C`` vaut ``attendu``."""
    for instant, valeur in historique:
        if instant >= depuis and bits_ba(valeur) == attendu:
            return instant
    return None


# --------------------------------------------------------------------------- temps et intervalles


def ecarts(instants: Iterable[float]) -> list[float]:
    """Écarts successifs entre des instants triés."""
    tries = sorted(instants)
    return [b - a for a, b in zip(tries, tries[1:])]


def intervalles_significatifs(intervalles: Iterable[Intervalle], seuil_s: float) -> list[Intervalle]:
    """Intervalles plus longs que ``seuil_s`` (élimine les bits et étirements normaux)."""
    return [(debut, fin) for debut, fin in intervalles if fin - debut > seuil_s]


def duree_max(intervalles: Iterable[Intervalle]) -> float:
    return max((fin - debut for debut, fin in intervalles), default=0.0)


def intervalle_contenant(intervalles: Iterable[Intervalle], instant: float) -> Intervalle | None:
    """Intervalle qui contient ``instant``, ou à défaut le premier qui commence après lui."""
    suivants = []
    for debut, fin in intervalles:
        if debut <= instant <= fin:
            return debut, fin
        if debut > instant:
            suivants.append((debut, fin))
    return min(suivants, default=None)


def intervalles_non_relaches(
    intervalles: Iterable[Intervalle], fin_observation: float, tolerance_s: float = seuils.TOLERANCE_HORODATAGE_S
) -> list[Intervalle]:
    """Intervalles encore en cours à la fin de l'observation : l'analyseur les tronque à cette borne,
    leur durée n'est donc pas celle du blocage et la ligne n'a pas été relâchée."""
    return [(debut, fin) for debut, fin in intervalles if fin >= fin_observation - tolerance_s]


def dernier_front_avant(fronts: Iterable[float], instant: float) -> float | None:
    anterieurs = [front for front in fronts if front < instant]
    return max(anterieurs, default=None)


def ecarts_periode_http(instants: Iterable[float], debut: float, fin: float) -> list[float]:
    """Écarts entre requêtes du BP dans [debut, fin], bornes de la fenêtre comprises : un BP qui se
    tait en fin de fenêtre produit un grand écart final."""
    dans_fenetre = sorted(t for t in instants if debut <= t <= fin)
    return ecarts([debut, *dans_fenetre, fin])


# --------------------------------------------------------------------------- trames


def retransmissions_manquantes(
    trames: Sequence[TrameI2C], adresses: Iterable[int], delai_s: float
) -> list[TrameI2C]:
    """Trames perturbées (CRC, écho ou NACK) vers ``adresses`` qui ne sont pas suivies, dans
    ``delai_s``, d'une trame identique (même adresse, même code, mêmes données) acquittée."""
    adresses = set(adresses)
    manquantes = []
    for i, trame in enumerate(trames):
        if trame.adresse not in adresses or trame.erreur not in seuils.ERREURS_PERTURBATION:
            continue
        retransmise = any(
            suivante.acquittee
            and suivante.adresse == trame.adresse
            and suivante.code == trame.code
            and suivante.donnees == trame.donnees
            and trame.t_fin <= suivante.t_debut <= trame.t_fin + delai_s
            for suivante in trames[i + 1 :]
        )
        if not retransmise:
            manquantes.append(trame)
    return manquantes


def nack_adresse(trames: Iterable[TrameI2C]) -> list[TrameI2C]:
    return [trame for trame in trames if trame.erreur == "nack_adresse"]


def erreurs_crc(trames: Iterable[TrameI2C]) -> list[TrameI2C]:
    return [trame for trame in trames if trame.erreur in ("crc", "echo")]


# --------------------------------------------------------------------------- sorties


def etats_successifs(historique: Iterable[tuple[float, bool]], depuis: float) -> list[bool]:
    """États successifs distincts d'une sortie après ``depuis``."""
    etats: list[bool] = []
    for instant, etat in sorted(historique):
        if instant >= depuis and (not etats or etats[-1] != etat):
            etats.append(etat)
    return etats


def instant_application(historique: Iterable[tuple[float, bool]], etat: bool, depuis: float) -> float | None:
    """Premier instant, après ``depuis``, où la sortie passe à ``etat``."""
    for instant, valeur in sorted(historique):
        if instant >= depuis and valeur == etat:
            return instant
    return None


class CommandeEmise(NamedTuple):
    """Commande de sortie émise par le serveur de banc : instant d'envoi, état visé et instant où le
    BP l'a récupérée (``None`` s'il ne l'a jamais récupérée)."""

    envoi: float
    etat: bool
    recuperation: float | None


def commandes_perdues(
    commandes: Sequence[CommandeEmise], historique: Iterable[tuple[float, bool]], fin: float
) -> list[CommandeEmise]:
    """Commandes successives d'une même sortie, chacune inversant la précédente, dont l'état n'a pas
    été appliqué entre leur envoi et la récupération par le BP de la commande suivante (ou ``fin``
    pour la dernière). Au-delà, l'état est devenu obsolète : le BP n'a pas à le rejouer.

    Une commande jamais récupérée par le BP est perdue. Deux commandes récupérées dans le même lot
    font perdre la première : le trafic du harnais les espace pour que cela n'arrive pas.
    """
    historique = sorted(historique)
    instants = [instant for instant, _ in historique]
    perdues = []
    for i, commande in enumerate(commandes):
        if commande.recuperation is None:
            perdues.append(commande)
            continue
        suivante = commandes[i + 1] if i + 1 < len(commandes) else None
        limite = suivante.recuperation if suivante is not None and suivante.recuperation is not None else fin
        debut, fin_tranche = bisect_left(instants, commande.envoi), bisect_right(instants, limite)
        if not any(valeur == commande.etat for _, valeur in historique[debut:fin_tranche]):
            perdues.append(commande)
    return perdues


# --------------------------------------------------------------------------- tas du BP


def tas_stable(echantillons: Sequence[tuple[float, int]], fenetre_s: float, tolerance_octets: int = 0) -> bool:
    """Le minimum du tas libre sur la dernière fenêtre n'est pas inférieur à celui de la première
    fenêtre, à ``tolerance_octets`` près. Faux si la série couvre moins de deux fenêtres."""
    if not echantillons:
        return False
    tries = sorted(echantillons)
    debut, fin = tries[0][0], tries[-1][0]
    if fin - debut < 2 * fenetre_s:
        return False
    premiers = [octets for instant, octets in tries if instant <= debut + fenetre_s]
    derniers = [octets for instant, octets in tries if instant >= fin - fenetre_s]
    return min(derniers) >= min(premiers) - tolerance_octets


# --------------------------------------------------------------------------- échanges HTTP


_MOTIF_ENTIER = re.compile(r"^-?\d+$")

#: Couple index/valeur sérialisé par le BP 099-37 (``Ethernet/Json.c``) : ``{k:12,v:"0"}``, clés
#: non quotées. Une autre sérialisation n'est pas reconnue, donc reste comparée telle quelle.
_MOTIF_VALEUR_INDEX = re.compile(rb'\{k:(\d+),v:"([^"]*)"\}')
_MOTIF_CONTENT_LENGTH = re.compile(rb"^(content-length:[ \t]*)(\d+)", re.IGNORECASE)

#: Chemins dont le dernier segment est un identifiant attribué par le serveur (GUID d'action).
CHEMINS_A_IDENTIFIANT = (b"/api/done/",)


def forme_valeur(valeur: str) -> str:
    """Codage d'une valeur d'index, indépendamment de sa valeur : entier, vide ou texte."""
    if valeur == "":
        return "vide"
    return "entier" if _MOTIF_ENTIER.match(valeur) else "texte"


def normaliser_requete(
    brut: bytes, indices_variables: Iterable[int] = (), entetes_variables: Iterable[str] = ()
) -> bytes:
    """Requête brute dont seuls les champs variables déclarés sont masqués :

    - l'identifiant en fin de chemin de ``/api/done/<GUID>`` ;
    - la valeur des index déclarés variables (date, téléinformation…), remplacée par sa forme
      (entier, texte ou vide) ;
    - la valeur des en-têtes déclarés variables ;
    - la valeur de ``Content-Length``, remplacée par son écart à la longueur réelle du corps : une
      longueur fausse reste une différence.

    Tout le reste est comparé octet pour octet : ligne de requête, ordre, casse et séparateurs des
    en-têtes, guillemets des clés, format de la version.
    """
    tete, separateur, corps = brut.partition(b"\r\n\r\n")
    variables = {int(index) for index in indices_variables}

    def masquer_index(trouve: re.Match) -> bytes:
        if int(trouve.group(1)) not in variables:
            return trouve.group(0)
        forme = forme_valeur(trouve.group(2).decode("latin-1"))
        return b"{k:" + trouve.group(1) + b',v:"<' + forme.encode() + b'>"}'

    ligne_requete, *entetes = tete.split(b"\r\n")
    for prefixe in CHEMINS_A_IDENTIFIANT:
        ligne_requete = re.sub(re.escape(prefixe) + rb"[^ ]+", prefixe + b"<identifiant>", ligne_requete)
    noms_variables = {nom.strip().lower().encode() for nom in entetes_variables}
    normalises = []
    for entete in entetes:
        longueur = _MOTIF_CONTENT_LENGTH.match(entete)
        nom, deux_points, _ = entete.partition(b":")
        if longueur:
            ecart = int(longueur.group(2)) - len(corps)
            entete = longueur.group(1) + f"<longueur du corps {ecart:+d}>".encode() + entete[longueur.end() :]
        elif deux_points and nom.strip().lower() in noms_variables:
            entete = nom + deux_points + b"<variable>"
        normalises.append(entete)
    return b"\r\n".join([ligne_requete, *normalises]) + separateur + _MOTIF_VALEUR_INDEX.sub(masquer_index, corps)


def _apercu(requete: bytes, longueur: int = 300) -> str:
    texte = repr(requete.decode("latin-1"))
    return texte if len(texte) <= longueur else texte[:longueur] + "…"


def comparer_echanges_http(
    reference: Sequence[EchangeHTTP],
    candidat: Sequence[EchangeHTTP],
    indices_variables: Iterable[int] = (),
    entetes_variables: Iterable[str] = (),
) -> list[str]:
    """Différences entre deux captures faites à état identique, comparées sur les requêtes brutes
    normalisées (``normaliser_requete``) ; liste vide si elles sont identiques. Le nombre de
    répétitions d'une même requête n'est pas comparé."""
    indices_variables, entetes_variables = tuple(indices_variables), tuple(entetes_variables)
    differences = []
    for nom, capture in (("référence", reference), ("candidat", candidat)):
        sans_brut = sum(1 for echange in capture if not echange.brut)
        if sans_brut:
            differences.append(f"{nom} : {sans_brut} requête(s) capturée(s) sans requête brute")
    ref = {normaliser_requete(e.brut, indices_variables, entetes_variables) for e in reference if e.brut}
    cand = {normaliser_requete(e.brut, indices_variables, entetes_variables) for e in candidat if e.brut}
    differences += [f"absente du candidat : {_apercu(requete)}" for requete in sorted(ref - cand)]
    differences += [f"nouvelle dans le candidat : {_apercu(requete)}" for requete in sorted(cand - ref)]
    if not reference or not candidat:
        differences.append("capture vide")
    return differences


# --------------------------------------------------------------------------- dégradation


def ecart_relatif(valeur: float, reference: float) -> float:
    if reference == 0:
        return 0.0 if valeur == 0 else float("inf")
    return abs(valeur - reference) / abs(reference)


def degradations(
    reference: Mapping[str, float], apres: Mapping[str, float], tolerance: float
) -> list[str]:
    """Grandeurs de repos qui s'écartent de leur référence de plus de ``tolerance`` (relative)."""
    return [
        f"{nom} : {apres.get(nom)!r} contre {valeur!r} avant l'essai"
        for nom, valeur in reference.items()
        if nom not in apres or ecart_relatif(apres[nom], valeur) > tolerance
    ]
