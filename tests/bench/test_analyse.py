"""Vérification hors banc des fonctions d'analyse pures du harnais (tâche 7.2).

Ces tests n'utilisent aucun instrument : ils sont toujours exécutés, avec ou sans banc.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S
from harnais_banc.instruments import EchangeHTTP, TrameI2C


def test_bits_de_l_index_12():
    assert analyse.bits_ba(0x1F) == 0x1C
    assert analyse.bits_ba(0x03) == 0
    assert analyse.bits_hors_ba(0x1F) == 0x03
    assert analyse.bits_ba(None) is None
    assert {S.BIT_INDEX12[ba] for ba in S.ADRESSES_BA} == {0x04, 0x08, 0x10}
    assert sum(S.BIT_INDEX12.values()) == S.MASQUE_BA_INDEX12


def test_premier_instant_bits_ba():
    historique = [(0.0, 0x00), (30.0, 0x01), (60.0, 0x05), (90.0, 0x04)]
    assert analyse.premier_instant_bits_ba(historique, 0x04, depuis=10.0) == 60.0
    assert analyse.premier_instant_bits_ba(historique, 0x08, depuis=0.0) is None


def test_ecarts_et_intervalles():
    assert analyse.ecarts([3.0, 1.0, 2.0]) == [1.0, 1.0]
    intervalles = [(0.0, 0.00001), (1.0, 1.2), (5.0, 5.004)]
    assert analyse.intervalles_significatifs(intervalles, S.SEUIL_BAS_SIGNIFICATIF_S) == [(1.0, 1.2)]
    assert analyse.duree_max(intervalles) == pytest.approx(0.2)
    assert analyse.duree_max([]) == 0.0
    assert analyse.dernier_front_avant([0.1, 0.5, 0.9], 0.6) == 0.5
    assert analyse.dernier_front_avant([0.7], 0.6) is None
    assert analyse.intervalle_contenant([(1.0, 2.0), (3.0, 4.0)], 2.5) == (3.0, 4.0)


def test_intervalles_non_relaches():
    """Un blocage tronqué par la fin de l'observation n'est pas une libération."""
    intervalles = [(1.0, 1.4), (5.0, 7.0)]
    assert analyse.intervalles_non_relaches(intervalles, fin_observation=7.0) == [(5.0, 7.0)]
    assert analyse.intervalles_non_relaches(intervalles, fin_observation=8.0) == []


def test_ecarts_periode_http_compte_le_silence_final():
    assert max(analyse.ecarts_periode_http([1.0, 2.0, 3.0], 0.0, 4.0)) == 1.0
    assert max(analyse.ecarts_periode_http([1.0, 2.0], 0.0, 30.0)) == 28.0
    assert analyse.ecarts_periode_http([], 0.0, 30.0) == [30.0]


def _trame(t, adresse, erreur=None, code="actions", donnees=b"\x01"):
    return TrameI2C(t_debut=t, t_fin=t + 0.002, adresse=adresse, code=code, donnees=donnees, erreur=erreur)


def test_retransmissions_manquantes():
    retransmise = [_trame(0.0, 0x11, "crc"), _trame(0.1, 0x11)]
    assert analyse.retransmissions_manquantes(retransmise, [0x11], 10.0) == []
    perdue = [_trame(0.0, 0x11, "nack_donnees"), _trame(0.1, 0x11, donnees=b"\x02")]
    assert analyse.retransmissions_manquantes(perdue, [0x11], 10.0) == [perdue[0]]
    trop_tard = [_trame(0.0, 0x12, "nack_adresse"), _trame(20.0, 0x12)]
    assert analyse.retransmissions_manquantes(trop_tard, [0x12], 10.0) == [trop_tard[0]]
    hors_perimetre = [_trame(0.0, 0x13, "crc")]
    assert analyse.retransmissions_manquantes(hors_perimetre, [0x11, 0x12], 10.0) == []


def test_comptage_des_erreurs_de_trame():
    trames = [_trame(0, 0x11, "nack_adresse"), _trame(1, 0x11, "crc"), _trame(2, 0x11, "echo"), _trame(3, 0x11)]
    assert len(analyse.nack_adresse(trames)) == 1
    assert len(analyse.erreurs_crc(trames)) == 2


def test_etats_et_application_des_sorties():
    historique = [(1.0, True), (2.0, False), (3.0, False), (4.0, True)]
    assert analyse.etats_successifs(historique, depuis=0.0) == [True, False, True]
    assert analyse.etats_successifs(historique, depuis=3.5) == [True]
    assert analyse.instant_application(historique, False, depuis=0.0) == 2.0
    assert analyse.instant_application(historique, True, depuis=5.0) is None


def test_commandes_perdues():
    C = analyse.CommandeEmise
    commandes = [C(0.0, True, 1.0), C(3.0, False, 4.0), C(6.0, True, 7.0)]
    toutes_appliquees = [(1.05, True), (4.05, False), (7.05, True)]
    assert analyse.commandes_perdues(commandes, toutes_appliquees, fin=12.0) == []
    une_perdue = [(1.05, True), (7.05, True)]
    assert analyse.commandes_perdues(commandes, une_perdue, fin=12.0) == [commandes[1]]
    assert analyse.commandes_perdues(commandes, [], fin=12.0) == commandes

    # Appliquée après l'envoi de la suivante, mais avant que le BP ne la récupère : non perdue.
    tardive = [C(0.0, True, 1.0), C(3.0, False, 5.0)]
    assert analyse.commandes_perdues(tardive, [(3.5, True), (5.1, False)], fin=12.0) == []

    # Deux ordres contraires récupérés dans le même lot : l'intermédiaire n'est jamais appliqué.
    meme_lot = [C(0.0, True, 2.0), C(1.0, False, 2.0)]
    assert analyse.commandes_perdues(meme_lot, [(2.05, False)], fin=12.0) == [meme_lot[0]]

    # Jamais récupérée par le BP : perdue, même si la sortie a l'état visé.
    jamais = [C(0.0, True, None)]
    assert analyse.commandes_perdues(jamais, [(0.5, True)], fin=12.0) == jamais


def test_tas_stable():
    jour = 24 * 3600.0
    stable = [(t * 600.0, 40_000 - (t % 3) * 100) for t in range(int(3 * jour / 600) + 1)]
    assert analyse.tas_stable(stable, jour)
    fuite = [(t * 600.0, 40_000 - t) for t in range(int(3 * jour / 600) + 1)]
    assert not analyse.tas_stable(fuite, jour)
    assert not analyse.tas_stable(stable[:10], jour)
    assert not analyse.tas_stable([], jour)


ENTETES_LEGACY = (
    b"Accept: application/json\r\n",
    b"Content-Type: application/json\r\n",
    b"Host: 198.18.10.2\r\n",
)


def _status(corps: bytes, entetes=ENTETES_LEGACY, longueur: str | None = None, prefixe=b"Content-Length:") -> EchangeHTTP:
    """POST /api/mystatus sérialisé comme le BP 099-37 (``Ethernet/www.c`` et ``Json.c``)."""
    longueur = str(len(corps)) if longueur is None else longueur
    brut = (
        b"POST /api/mystatus HTTP/1.1\r\n" + b"".join(entetes) + prefixe + b" " + longueur.encode() + b"\r\n"
        + b"Authorization: Basic ZmljdGlm\r\n\r\n" + corps
    )
    return EchangeHTTP(t=0.0, methode="POST", chemin="/api/mystatus", brut=brut)


def _done(guid: str) -> EchangeHTTP:
    brut = b"POST /api/done/" + guid.encode() + b" HTTP/1.1\r\n" + b"".join(ENTETES_LEGACY) + b"Content-Length: 0\r\n\r\n"
    return EchangeHTTP(t=0.0, methode="POST", chemin="/api/done/" + guid, brut=brut)


CORPS = b'{"version":"V37","ek":[{k:12,v:"0"},{k:700,v:"1234"}]}'


def test_comparaison_des_echanges_http_identiques():
    reference = [_status(CORPS), _done("a1b2")]
    assert analyse.comparer_echanges_http(reference, [_status(CORPS), _done("ffff-0001")]) == []
    date_suivante = [_status(CORPS.replace(b"1234", b"129999")), _done("a1b2")]
    assert analyse.comparer_echanges_http(reference, date_suivante, indices_variables=[700]) == []


@pytest.mark.parametrize(
    "candidat",
    [
        pytest.param(_status(CORPS.replace(b'v:"0"', b'v:"4"')), id="valeur-changee"),
        pytest.param(_status(CORPS.replace(b"]}", b',{k:999,v:"0"}]}')), id="index-ajoute"),
        pytest.param(_status(CORPS.replace(b"{k:12,", b'{"k":12,').replace(b',v:"0"', b',"v":"0"')), id="cles-quotees"),
        pytest.param(_status(CORPS.replace(b'"V37"', b'"37"')), id="format-de-version"),
        pytest.param(_status(CORPS.replace(b"1234", b"12h99")), id="index-variable-recode"),
        pytest.param(_status(CORPS, entetes=ENTETES_LEGACY[::-1]), id="ordre-des-entetes"),
        pytest.param(_status(CORPS, prefixe=b"Content-length:"), id="casse-d-un-entete"),
        pytest.param(_status(CORPS, longueur=str(len(CORPS) + 1)), id="content-length-faux"),
    ],
)
def test_comparaison_des_echanges_http_differents(candidat):
    reference = [_status(CORPS)]
    assert analyse.comparer_echanges_http(reference, [candidat], indices_variables=[700])


def test_comparaison_des_echanges_http_incomplets():
    reference = [_status(CORPS)]
    assert analyse.comparer_echanges_http(reference, []) != []
    sans_brut = [EchangeHTTP(t=0.0, methode="POST", chemin="/api/mystatus")]
    assert analyse.comparer_echanges_http(reference, sans_brut) != []
    variable = analyse.normaliser_requete(_status(CORPS).brut, entetes_variables=["Authorization"])
    assert b"Authorization:<variable>" in variable


def test_degradations():
    reference = {"courant_bp_a": 0.50, "entree_15v_0x11_v": 14.8}
    assert analyse.degradations(reference, {"courant_bp_a": 0.53, "entree_15v_0x11_v": 14.7}, 0.10) == []
    assert analyse.degradations(reference, {"courant_bp_a": 0.60, "entree_15v_0x11_v": 14.7}, 0.10)
    assert analyse.degradations(reference, {"courant_bp_a": 0.50}, 0.10)
