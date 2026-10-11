"""NR-board-sc944d-7 — Télémétrie de l'état de chaque BA (tâche 7.9).

Exigence : i2c-bus-fault-containment › « Télémétrie de l'état de chaque BA » (scénarios « BA
déconnecté sans commande », « BA reconnecté », « Codage de l'index 12 inchangé »).

Vérification (tâche 7.9) : un BA est déconnecté puis reconnecté, sans aucune commande ; le bit de
l'index 12 est positionné, puis effacé, en moins de 120 s, et ``(index 12 & 0x1C)`` vaut ``0x1C``
quand les trois BA sont absents. Le protocole HTTP legacy IoT est inchangé : l'index 12 garde son
codage (bits 0x04, 0x08, 0x10 pour les BA 0x11, 0x12, 0x13).
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


# NR: NR-board-sc944d-7 #1
@pytest.mark.nr("NR-board-sc944d-7", "#1")
def test_ba_deconnecte_puis_reconnecte_sans_commande(banc):
    banc.attendre_index_sain()
    banc.maitre.vider_commandes()
    ba, par = banc.ba_le_plus_eloigne, banc.moyen_deconnexion()

    t_deconnexion = banc.maintenant()
    banc.relais.deconnecter_ba(ba, par=par)
    try:
        pose = banc.attendre_bits_ba(S.BIT_INDEX12[ba], t_deconnexion)
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    t_reconnexion = banc.maintenant()
    effacement = banc.attendre_bits_ba(0, t_reconnexion)
    resynchronisation = banc.attendre_configuration(ba, t_reconnexion, delai_s=S.DELAI_TELEMETRIE_S)

    assert pose is not None, f"bit {S.BIT_INDEX12[ba]:#04x} non positionné en {S.DELAI_TELEMETRIE_S} s sans commande"
    assert effacement is not None, f"bit {S.BIT_INDEX12[ba]:#04x} non effacé en {S.DELAI_TELEMETRIE_S} s"
    assert resynchronisation is not None, f"BA {ba:#04x} reconnecté sans resynchronisation de sa configuration"


# NR: NR-board-sc944d-7 #1
@pytest.mark.nr("NR-board-sc944d-7", "#1")
def test_index_12_vaut_0x1c_quand_les_trois_ba_sont_absents(banc):
    banc.attendre_index_sain()
    avant = banc.valeur_index12()
    assert avant is not None, "aucune valeur de l'index 12 remontée par le BP"
    par = banc.moyen_deconnexion()

    t_deconnexion = banc.maintenant()
    for ba in reversed(banc.ba):
        banc.relais.deconnecter_ba(ba, par=par)
    try:
        delai = banc.attendre_bits_ba(S.MASQUE_BA_INDEX12, t_deconnexion)
        valeurs = [valeur for _, valeur in banc.historique_index12(t_deconnexion)]
    finally:
        for ba in banc.ba:
            banc.relais.reconnecter_ba(ba, par=par)

    assert delai is not None, f"index 12 & 0x1C ≠ 0x1C en {S.DELAI_TELEMETRIE_S} s avec les trois BA absents"
    modifies = [v for v in valeurs if analyse.bits_hors_ba(v) != analyse.bits_hors_ba(avant)]
    assert not modifies, f"bits 0 et 1 de l'index 12 modifiés par l'absence des BA : {modifies}"
