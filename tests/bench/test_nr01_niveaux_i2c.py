"""NR-board-sc944d-1 — Niveaux I2C à 50 kHz et fuite par carte (tâche 7.3).

Exigences : i2c-bus-surge-protection › « Niveaux I2C conservés à 50 kHz » (scénario « Réception
après retouche ») et › « Écrêtage coordonné des lignes SDA et SCL » (scénario « Fuite après
remplacement d'un écrêteur »).

Vérification (tâche 7.3), avec 3 × 33 cm, 1 m et 3 m de câble, à consommation maximale :

- niveau bas au MCF inférieur à 0,85 V quand un BA émet (SDA : acquittement et octets lus) ;
- niveau bas au PIC inférieur à 1,2 V quand le BP émet (SDA et SCL) ;
- temps de montée de 0 à 3,45 V inférieur à 4 µs au BA le plus éloigné ;
- fuite de chaque ligne, carte seule, d'au plus 50 µA ;
- chaque BA acquitte une commande.

À 1 m et 3 m, le bus reste non isolé et les critères sont ceux du coffret, comme le demande la
tâche 7.3 : ces cas mesurent la marge d'un BA raccordé sans module. Les critères d'un BA déporté
derrière son MIB (interconnect › Module d'isolement pour BA déportés) sont vérifiés par
NR-board-sc944d-13 (``test_niveaux_du_bus_avec_un_mib``).

Chaque cas monte son câble par ``remettre_etat_nominal(cable=…)`` ; la fixture ``banc`` remonte le
câble nominal (3 × 33 cm) après le test, pour que les tests suivants ne dépendent pas de l'ordre
d'exécution.
"""

from __future__ import annotations

import pytest

from harnais_banc import seuils as S

pytestmark = pytest.mark.banc

LIGNES = ("SDA", "SCL")


# NR: NR-board-sc944d-1 #1
@pytest.mark.nr("NR-board-sc944d-1", "#1")
@pytest.mark.parametrize("cable", S.CONFIGURATIONS_CABLE)
def test_niveaux_et_temps_de_montee_a_consommation_maximale(banc, cable):
    banc.remettre_etat_nominal(cable=cable)
    defauts: list[str] = []
    eloigne = banc.ba_le_plus_eloigne

    with banc.consommation_maximale():
        for ba in banc.ba:
            niveau = banc.oscillo.niveau_bas(S.POINT_MCF, "SDA", emetteur=S.carte_ba(ba))
            if not niveau < S.NIVEAU_BAS_MCF_MAX_V:
                defauts.append(f"niveau bas au MCF, SDA émise par {ba:#04x} : {niveau:.3f} V")
            for ligne in LIGNES:
                niveau = banc.oscillo.niveau_bas(S.point_pic(ba), ligne, emetteur="BP")
                if not niveau < S.NIVEAU_BAS_PIC_MAX_V:
                    defauts.append(f"niveau bas au PIC de {ba:#04x}, {ligne} émise par le BP : {niveau:.3f} V")
        for ligne in LIGNES:
            montee = banc.oscillo.temps_montee(
                S.point_pic(eloigne), ligne, S.TEMPS_MONTEE_DEBUT_V, S.TEMPS_MONTEE_FIN_V
            )
            if not montee < S.TEMPS_MONTEE_MAX_S:
                defauts.append(f"temps de montée de {ligne} au BA {eloigne:#04x} : {montee * 1e6:.2f} µs")
        for ba, duree in banc.reprise(banc.ba).items():
            if duree is None:
                defauts.append(f"BA {ba:#04x} : aucune commande acquittée en {S.DELAI_REPRISE_S} s")

    assert not defauts, f"câble {cable} :\n" + "\n".join(defauts)


# NR: NR-board-sc944d-1 #1
@pytest.mark.nr("NR-board-sc944d-1", "#1")
def test_fuite_de_chaque_ligne_carte_seule(banc):
    """Fuite déduite de l'écart, au repos, entre l'alimentation de la pull-up et la ligne : 50 µA
    valent 0,5 V sous les 10 kΩ d'un BA et 50 mV sous le 1 kΩ d'origine du BP."""
    defauts: list[str] = []
    voie_ba = banc.config.alimentations["ba_seul"]
    for carte in ["BP"] + [S.carte_ba(ba) for ba in banc.ba]:
        pullup = banc.config.pullup_bp_ohm if carte == "BP" else S.PULLUP_BA_OHM
        banc.relais.isoler_carte(carte)
        try:
            if carte != "BP":
                banc.alim.regler("ba_seul", voie_ba.nominale_v, voie_ba.max_a)
                banc.alim.activer("ba_seul")
            alimentation = banc.oscillo.tension_continue(S.point_alim_pullup(carte))
            for ligne in LIGNES:
                repos = banc.oscillo.tension_continue(S.point_ligne_carte(carte, ligne))
                fuite = (alimentation - repos) / pullup
                if fuite > S.FUITE_LIGNE_MAX_A:
                    defauts.append(f"{carte} {ligne} : fuite {fuite * 1e6:.1f} µA ({alimentation - repos:.3f} V)")
        finally:
            if carte != "BP":
                banc.alim.couper("ba_seul")
            banc.relais.reintegrer_carte(carte)

    assert not defauts, "\n".join(defauts)
