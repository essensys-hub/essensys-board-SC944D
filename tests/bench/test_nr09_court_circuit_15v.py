"""NR-board-sc944d-9 — Court-circuit du +15 V d'un port (tâche 7.11).

Exigences : i2c-bus-surge-protection › « Protection de l'alimentation +15 V du bus » (scénarios
« Court-circuit du +15 V à la sortie du BP », « Écrêteur d'un BA en court-circuit ») ;
aux-box-interconnect › « Distribution protégée du +15 V » (scénarios « Court-circuit sur une
sortie », « Retour après court-circuit »).

Vérification (tâche 7.11) : la sortie +15 V d'un port est court-circuitée pendant 60 s, puis J31-4
est court-circuité ; les autres ports retrouvent 14 V ou plus dans les 5 s et les gardent, les
autres BA reprennent moins de 10 s après l'ouverture du cavalier du port en court-circuit, le retour
se fait en 120 s au plus, et aucune dégradation n'apparaît.

Les deux scénarios de l'interconnexion sont essayés séparément :

- court-circuit de 60 s **cavalier fermé**, puis suppression : les autres sorties tiennent 14 V, la
  protection du BP ne déclenche pas, et la sortie remonte en 120 s au plus, sans intervention ;
- court-circuit, puis **ouverture du cavalier** du port : les autres BA reprennent en moins de 10 s.

Le point ``+15V:port`` est en amont du cavalier, le court-circuit ``port:`` en aval (voir
``instruments.Oscilloscope``). Sur un banc chaîné dont les BA sont retouchés (PTC et écrêteur sur
+Vin, tâches 2.3 et 2.4), le court-circuit de l'écrêteur d'un BA est aussi essayé.

Les courts-circuits sont faits sur les échantillons du banc, jamais sur une carte d'installation.
Un redémarrage du BP pendant le court-circuit de J31-4 est admis (design.md D3).
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


def _echantillonner(banc, points: list[str], duree_s: float) -> dict[str, list[tuple[float, float]]]:
    """Tensions continues aux ``points``, relevées toutes les ``PERIODE_ECHANTILLON_TENSION_S``."""
    series: dict[str, list[tuple[float, float]]] = {point: [] for point in points}
    debut = banc.maintenant()
    while banc.maintenant() - debut < duree_s:
        for point, serie in series.items():
            serie.append((banc.maintenant(), banc.oscillo.tension_continue(point)))
        banc.attendre(S.PERIODE_ECHANTILLON_TENSION_S)
    return series


def _tensions_basses(series: dict[str, list[tuple[float, float]]], t_court: float) -> list[str]:
    """Points passés sous 14 V au-delà des 5 s qui suivent le court-circuit."""
    defauts = []
    for point, serie in series.items():
        basses = [
            (round(t - t_court, 1), v)
            for t, v in serie
            if t >= t_court + S.DELAI_RETOUR_AUTRES_PORTS_S and v < S.TENSION_MIN_PORT_15V_V
        ]
        if basses:
            defauts.append(f"{point} sous {S.TENSION_MIN_PORT_15V_V} V après {S.DELAI_RETOUR_AUTRES_PORTS_S} s : {basses[:3]}")
    return defauts


# NR: NR-board-sc944d-9 #1
@pytest.mark.nr("NR-board-sc944d-9", "#1")
@pytest.mark.banc_requiert("court_circuit_15v", "sorties_15v_par_port")
def test_court_circuit_du_15v_d_un_port_cavalier_ferme(banc):
    """Scénarios « Court-circuit sur une sortie » (tensions, protection du BP) et « Retour après
    court-circuit ». Le BA privé d'alimentation peut bloquer le bus pendant le court-circuit : la
    communication n'est jugée qu'après le retour de sa sortie."""
    reference = banc.mesures_repos()
    ba = banc.ba_le_plus_eloigne
    cible = f"port:{S.carte_ba(ba)}"
    points = [S.point_port_15v(autre) for autre in banc.autres_ba(ba)] + [S.POINT_J31_4]

    t_court = banc.maintenant()
    banc.relais.court_circuiter_15v(cible)
    try:
        series = _echantillonner(banc, points, S.DUREE_COURT_CIRCUIT_S)
    finally:
        banc.relais.supprimer_court_circuit(cible)
    t_suppression = banc.maintenant()
    retour = banc.attendre_que(
        lambda: banc.oscillo.tension_continue(S.point_port_15v(ba)) >= S.TENSION_MIN_PORT_15V_V,
        S.DELAI_RETOUR_APRES_COURT_CIRCUIT_S,
    )
    reprise = banc.reprise(banc.ba) if retour is not None else {}

    defauts = _tensions_basses(series, t_court)
    if retour is None:
        defauts.append(
            f"port {ba:#04x} sous {S.TENSION_MIN_PORT_15V_V} V {S.DELAI_RETOUR_APRES_COURT_CIRCUIT_S} s "
            "après la suppression du court-circuit"
        )
    for autre, duree in reprise.items():
        if duree is None:
            defauts.append(f"{autre:#04x} sans reprise {S.DELAI_REPRISE_S} s après le retour du port")
    defauts += analyse.degradations(reference, banc.mesures_repos(), S.TOLERANCE_COURANT_REPOS)
    assert not defauts, f"court-circuit du port {ba:#04x} à {t_suppression - t_court:.0f} s :\n" + "\n".join(defauts)


# NR: NR-board-sc944d-9 #1
@pytest.mark.nr("NR-board-sc944d-9", "#1")
@pytest.mark.banc_requiert("court_circuit_15v", "sorties_15v_par_port", "cavaliers_par_port")
def test_ouverture_du_cavalier_d_un_port_en_court_circuit(banc):
    """Scénario « Court-circuit sur une sortie », dernière clause : après l'ouverture du cavalier du
    port en court-circuit, les autres BA reprennent en moins de 10 s."""
    ba = banc.ba_le_plus_eloigne
    autres = banc.autres_ba(ba)
    cible = f"port:{S.carte_ba(ba)}"

    banc.relais.court_circuiter_15v(cible)
    try:
        banc.attendre(S.DELAI_RETOUR_AUTRES_PORTS_S)
        t_cavalier = banc.maintenant()
        banc.relais.deconnecter_ba(ba, par="cavalier")
        reprise_autres = banc.reprise(autres, depuis=t_cavalier)
    finally:
        banc.relais.supprimer_court_circuit(cible)
        banc.relais.reconnecter_ba(ba, par="cavalier")
    reprise_ba = banc.reprise([ba], delai_s=S.DELAI_RETOUR_APRES_COURT_CIRCUIT_S + S.DELAI_REPRISE_S)[ba]

    defauts = [
        f"{autre:#04x} sans reprise {S.DELAI_REPRISE_S} s après l'ouverture du cavalier"
        for autre, duree in reprise_autres.items()
        if duree is None
    ]
    if reprise_ba is None:
        defauts.append(f"{ba:#04x} sans reprise après la suppression du court-circuit et la fermeture du cavalier")
    assert not defauts, "\n".join(defauts)


# NR: NR-board-sc944d-9 #1
@pytest.mark.nr("NR-board-sc944d-9", "#1")
@pytest.mark.banc_requiert("court_circuit_vin_ba")
def test_ecreteur_d_un_ba_en_court_circuit(banc):
    """Scénario « Écrêteur d'un BA en court-circuit » : l'écrêteur +15 V du BA le plus éloigné est
    remplacé par un court-circuit, en aval de sa PTC. BA_ALIM revient à 14 V ou plus à l'entrée
    des autres BA dans les 5 s et y reste ; après la déconnexion du port de ce BA, les autres BA
    reprennent en moins de 10 s. La PTC du BP (L31) ne doit pas déclencher avant celle du BA."""
    ba, par = banc.ba_deconnectable_seul(), banc.moyen_deconnexion()
    autres = banc.autres_ba(ba)
    cible = f"+Vin:{S.carte_ba(ba)}"
    points = [S.point_entree_15v(autre) for autre in autres]

    t_court = banc.maintenant()
    banc.relais.court_circuiter_15v(cible)
    try:
        series = _echantillonner(banc, points, S.DUREE_COURT_CIRCUIT_S)
        t_deconnexion = banc.maintenant()
        banc.relais.deconnecter_ba(ba, par=par)
        reprise_autres = banc.reprise(autres, depuis=t_deconnexion)
    finally:
        banc.relais.supprimer_court_circuit(cible)
        banc.relais.reconnecter_ba(ba, par=par)

    defauts = _tensions_basses(series, t_court)
    defauts += [
        f"{autre:#04x} sans reprise {S.DELAI_REPRISE_S} s après la déconnexion de {ba:#04x} par {par}"
        for autre, duree in reprise_autres.items()
        if duree is None
    ]
    assert not defauts, "\n".join(defauts)


# NR: NR-board-sc944d-9 #1
@pytest.mark.nr("NR-board-sc944d-9", "#1")
@pytest.mark.banc_requiert("court_circuit_15v")
def test_court_circuit_de_j31_4(banc):
    reference = banc.mesures_repos()
    eloigne = banc.ba_le_plus_eloigne

    banc.relais.court_circuiter_15v("J31-4")
    try:
        banc.attendre(S.DUREE_COURT_CIRCUIT_S)
    finally:
        banc.relais.supprimer_court_circuit("J31-4")
    t_suppression = banc.maintenant()
    retour = banc.attendre_que(
        lambda: banc.oscillo.tension_continue(S.point_entree_15v(eloigne)) >= S.TENSION_MIN_PORT_15V_V,
        S.DELAI_RETOUR_APRES_COURT_CIRCUIT_S,
    )
    assert retour is not None, (
        f"BA_ALIM sous {S.TENSION_MIN_PORT_15V_V} V au dernier BA {S.DELAI_RETOUR_APRES_COURT_CIRCUIT_S} s "
        "après la suppression du court-circuit de J31-4"
    )
    reprise = banc.reprise(banc.ba, depuis=t_suppression + retour)
    sans_reprise = [f"{ba:#04x}" for ba, duree in reprise.items() if duree is None]
    assert not sans_reprise, f"BA sans reprise {S.DELAI_REPRISE_S} s après le retour de BA_ALIM : {sans_reprise}"
    degradations = analyse.degradations(reference, banc.mesures_repos(), S.TOLERANCE_COURANT_REPOS)
    assert not degradations, "dégradation permanente après le court-circuit de J31-4 :\n" + "\n".join(degradations)
