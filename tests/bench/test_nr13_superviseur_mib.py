"""NR-board-sc944d-13 — Superviseur du module d'isolement (MIB) (tâche 7.15).

Exigence : i2c-bus-fault-containment › « Libération automatique d'un nœud isolé bloqué » (scénarios
« Ligne forcée basse côté BA », « Esclave bloqué derrière un nœud isolé », « Fin du défaut côté BA »).

Vérification (tâche 7.15) : une ligne est forcée à l'état bas côté BA d'un MIB, puis le PIC est figé
au milieu d'un octet ; le bus est libéré en 150 ms au plus, chaque essai de raccordement le bloque
moins de 150 ms, les autres BA continuent, et le BA reprend en moins de 10 s une fois le défaut
supprimé.

S'y ajoutent les niveaux du bus avec le MIB (aux-box-interconnect › « Module d'isolement pour BA
déportés », scénario « Niveaux avec un câble de 3 m » ; tâche 6.7 : niveaux tenus à 1 m et à 3 m),
que NR-board-sc944d-1 ne couvre pas : il mesure un bus non isolé.

Ne s'exécute que sur un banc équipé d'un BA raccordé par un MIB (capacité ``mib``), sur un bus
préparé : écrêteurs de ligne du BP de 50 pF au plus à la place de D33/D34, et aucun BA non isolé
du bus avec ses écrêteurs d'origine (tâche 6.7).
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = [pytest.mark.banc, pytest.mark.banc_requiert("mib")]

LIGNES = ("SDA", "SCL")


def _blocages_cote_bus(banc, ligne: str, debut: float, fin: float) -> list[tuple[float, float]]:
    return analyse.intervalles_significatifs(
        banc.analyseur.intervalles_bas(ligne, debut, fin, point="bus"), S.SEUIL_BAS_SIGNIFICATIF_S
    )


# NR: NR-board-sc944d-13 #1
@pytest.mark.nr("NR-board-sc944d-13", "#1")
@pytest.mark.parametrize("ligne", ["SDA", "SCL"])
def test_ligne_forcee_basse_cote_ba_d_un_mib(banc, ligne):
    ba = banc.config.ba_mib
    autres = banc.autres_ba(ba)

    t_forcage = banc.maintenant()
    banc.relais.forcer_ligne_basse(ligne, point="ba_mib")
    try:
        commandes = banc.trafic(autres, S.FENETRE_OBSERVATION_MIB_S)
        t_fin = banc.maintenant()
    finally:
        banc.relais.relacher_ligne(ligne, point="ba_mib")
    t_relachement = banc.maintenant()
    reprise = banc.reprise([ba], depuis=t_relachement)[ba]

    blocages = _blocages_cote_bus(banc, ligne, t_forcage, t_fin)
    defauts: list[str] = []
    if not blocages:
        defauts.append("aucun essai de raccordement observé côté bus pendant le forçage")
    elif analyse.duree_max(blocages) > S.DELAI_LIBERATION_MIB_S:
        defauts.append(f"côté bus bloqué {analyse.duree_max(blocages) * 1e3:.0f} ms (≤ 150 ms attendu)")
    ecarts = analyse.ecarts(debut for debut, _ in blocages)
    if ecarts and max(ecarts) > S.INTERVALLE_MAX_ESSAIS_MIB_S + S.DELAI_LIBERATION_MIB_S + S.TOLERANCE_HORODATAGE_S:
        defauts.append(f"essais de raccordement espacés de {max(ecarts):.1f} s (plafond de 5 s)")
    perdues = banc.commandes_perdues(commandes, t_forcage)
    if perdues:
        defauts.append("commandes des autres BA non appliquées : " + ", ".join(f"{b:#04x}" for b in perdues))
    if reprise is None:
        defauts.append(f"BA {ba:#04x} sans reprise {S.DELAI_REPRISE_S} s après la fin du forçage")
    assert not defauts, f"{ligne} forcée côté BA :\n" + "\n".join(defauts)


# NR: NR-board-sc944d-13 #1
@pytest.mark.nr("NR-board-sc944d-13", "#1")
@pytest.mark.banc_requiert("firmware_essai_ba")
def test_pic_fige_au_milieu_d_un_octet_derriere_un_mib(banc):
    ba = banc.config.ba_mib
    if banc.config.ba_essai != ba:
        pytest.skip("le BA raccordé par le MIB n'est pas le BA équipé du firmware d'essai")

    t_defaut = banc.maintenant()
    banc.relais.declencher_defaut_ba(ba, "fige_milieu_octet")
    try:
        application = banc.mesurer_application(banc.envoyer_bascules([ba]), t_defaut)[ba]
        t_fin = banc.maintenant()
    finally:
        banc.relais.annuler_defaut_ba(ba)

    blocages = _blocages_cote_bus(banc, "SDA", t_defaut, t_fin)
    assert blocages, "SDA jamais retenue côté bus : le défaut d'essai ne s'est pas produit"
    assert analyse.duree_max(blocages) <= S.DELAI_LIBERATION_MIB_S, (
        f"côté bus bloqué {analyse.duree_max(blocages) * 1e3:.0f} ms (≤ 150 ms attendu)"
    )
    assert application is not None, f"BA {ba:#04x} sans reprise en {S.DELAI_REPRISE_S} s, sans intervention"


# NR: NR-board-sc944d-13 #1
@pytest.mark.nr("NR-board-sc944d-13", "#1")
@pytest.mark.parametrize("cable", S.CONFIGURATIONS_CABLE_MIB)
def test_niveaux_du_bus_avec_un_mib(banc, cable):
    """Critères du MIB à 1 m et à 3 m de câble AWG20, à consommation maximale : capacité côté bus
    (BP compris) ≤ 400 pF, niveau bas au MCF ≤ 0,85 V, niveau bas côté BA < 0,48 V, temps de montée
    de 30 % à 70 % de la tension des pull-ups ≤ 1 µs."""
    ba = banc.config.ba_mib
    banc.remettre_etat_nominal(cable=cable)
    debut_v, fin_v = S.FRACTION_MONTEE_MIB
    defauts: list[str] = []

    for ligne in LIGNES:
        capacite = banc.oscillo.capacite_ligne(S.POINT_J31, ligne)
        if not capacite <= S.CAPACITE_BUS_MIB_MAX_F:
            defauts.append(f"capacité de {ligne} côté bus : {capacite * 1e12:.0f} pF")
    with banc.consommation_maximale():
        alimentation = banc.oscillo.tension_continue(S.point_alim_pullup("BP"))
        niveau = banc.oscillo.niveau_bas(S.POINT_MCF, "SDA", emetteur=S.carte_ba(ba))
        if not niveau <= S.NIVEAU_BAS_MCF_MIB_MAX_V:
            defauts.append(f"niveau bas au MCF, SDA émise par {ba:#04x} : {niveau:.3f} V")
        for ligne in LIGNES:
            niveau = banc.oscillo.niveau_bas(S.point_pic(ba), ligne, emetteur="BP")
            if not niveau < S.NIVEAU_BAS_COTE_BA_MIB_MAX_V:
                defauts.append(f"niveau bas côté BA, {ligne} émise par le BP : {niveau:.3f} V")
            montee = banc.oscillo.temps_montee(S.POINT_J31, ligne, debut_v * alimentation, fin_v * alimentation)
            if not montee <= S.TEMPS_MONTEE_MIB_MAX_S:
                defauts.append(f"temps de montée de {ligne} (30 à 70 %) : {montee * 1e6:.2f} µs")
        for autre, duree in banc.reprise(banc.ba).items():
            if duree is None:
                defauts.append(f"BA {autre:#04x} : aucune commande acquittée en {S.DELAI_REPRISE_S} s")

    assert not defauts, f"câble {cable} derrière le MIB :\n" + "\n".join(defauts)
