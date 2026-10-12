"""NR-board-sc944d-4 — Récupération d'un bus bloqué (tâche 7.6).

Exigence : i2c-bus-fault-containment › « Récupération d'un bus bloqué » (scénarios « Esclave bloqué
au milieu d'un octet », « Ligne forcée basse puis relâchée », « Défaut permanent »).

Vérification (tâche 7.6) : un esclave est bloqué au milieu d'un octet, puis une ligne est forcée à
l'état bas pendant 500 ms, puis le défaut devient permanent ; reprise en moins de 10 s, et une
tentative de récupération par seconde au plus. Aucune reprise ne passe par un cycle d'alimentation
des BA ni par un redémarrage du BP.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


def _sans_reprise(reprise: dict[int, float | None]) -> list[str]:
    return [f"{ba:#04x} sans reprise en {S.DELAI_REPRISE_S} s" for ba, duree in reprise.items() if duree is None]


def _reprise_apres_relachement(banc, cibles, t_forcage: float, t_relachement: float) -> list[str]:
    """Les commandes envoyées pendant le forçage doivent être appliquées après le relâchement, en
    moins de 10 s ; une commande appliquée pendant le forçage signale un forçage inopérant."""
    duree_forcage = t_relachement - t_forcage
    applications = banc.mesurer_application(cibles, t_forcage, delai_s=duree_forcage + S.DELAI_REPRISE_S)
    defauts = []
    for ba, duree in applications.items():
        if duree is None:
            defauts.append(f"{ba:#04x} sans reprise {S.DELAI_REPRISE_S} s après le relâchement")
        elif duree < duree_forcage:
            defauts.append(f"{ba:#04x} : commande appliquée pendant le forçage (forçage inopérant, essai invalide)")
    return defauts


# NR: NR-board-sc944d-4 #1
@pytest.mark.nr("NR-board-sc944d-4", "#1")
@pytest.mark.banc_requiert("firmware_essai_ba")
def test_esclave_bloque_au_milieu_d_un_octet(banc):
    ba = banc.config.ba_essai
    cycles_avant = banc.console.compteurs().cycles_alimentation
    t_defaut = banc.maintenant()
    banc.relais.declencher_defaut_ba(ba, "fige_milieu_octet")
    try:
        reprise = banc.reprise(banc.ba, depuis=t_defaut)
        t_fin = banc.maintenant()
    finally:
        banc.relais.annuler_defaut_ba(ba)

    blocages = analyse.intervalles_significatifs(
        banc.analyseur.intervalles_bas("SDA", t_defaut, t_fin), S.SEUIL_BAS_SIGNIFICATIF_S
    )
    defauts = _sans_reprise(reprise)
    if not blocages:
        defauts.append("SDA jamais retenue : le défaut d'essai ne s'est pas produit")
    if not banc.console.tentatives_recuperation(t_defaut, t_fin):
        defauts.append("aucune tentative de récupération (impulsions d'horloge et STOP) signalée")
    if banc.console.compteurs().cycles_alimentation != cycles_avant:
        defauts.append("reprise obtenue par un cycle d'alimentation des BA")
    assert not defauts, "\n".join(defauts)


# NR: NR-board-sc944d-4 #1
@pytest.mark.nr("NR-board-sc944d-4", "#1")
@pytest.mark.parametrize("ligne", ["SDA", "SCL"])
def test_ligne_forcee_basse_500_ms_puis_relachee(banc, ligne):
    t_forcage = banc.maintenant()
    banc.relais.forcer_ligne_basse(ligne)
    try:
        cibles = banc.envoyer_bascules(banc.ba)
        banc.attendre(S.DUREE_FORCAGE_BREF_S)
    finally:
        banc.relais.relacher_ligne(ligne)
    t_relachement = banc.maintenant()

    defauts = _reprise_apres_relachement(banc, cibles, t_forcage, t_relachement)
    assert not defauts, f"{ligne} forcée 500 ms :\n" + "\n".join(defauts)


# NR: NR-board-sc944d-4 #1
@pytest.mark.nr("NR-board-sc944d-4", "#1")
@pytest.mark.parametrize("ligne", ["SDA", "SCL"])
def test_defaut_permanent_une_tentative_par_seconde_au_plus(banc, ligne):
    ecart_reference = banc.periode_http_reference()
    cycles_avant = banc.console.compteurs().cycles_alimentation

    t_debut = banc.maintenant()
    banc.relais.forcer_ligne_basse(ligne)
    try:
        cibles = banc.envoyer_bascules(banc.ba)
        banc.attendre(S.FENETRE_DEFAUT_PERMANENT_S)
        t_fin = banc.maintenant()
    finally:
        banc.relais.relacher_ligne(ligne)
    t_relachement = banc.maintenant()

    defauts = _reprise_apres_relachement(banc, cibles, t_debut, t_relachement)
    tentatives = banc.console.tentatives_recuperation(t_debut, t_fin)
    if not tentatives:
        defauts.append("aucune tentative de récupération pendant le défaut permanent")
    ecarts = analyse.ecarts(tentatives)
    if ecarts and min(ecarts) < S.PERIODE_MIN_RECUPERATION_S - S.TOLERANCE_HORODATAGE_S:
        defauts.append(f"tentatives de récupération espacées de {min(ecarts):.3f} s seulement")
    ecarts_http = analyse.ecarts_periode_http(banc.maitre.instants_echanges_bp(t_debut, t_fin), t_debut, t_fin)
    if max(ecarts_http) > ecart_reference * S.FACTEUR_PERIODE_HTTP:
        defauts.append(f"BP non joignable : échanges HTTP interrompus {max(ecarts_http):.1f} s")
    if banc.console.compteurs().cycles_alimentation != cycles_avant:
        defauts.append("cycle d'alimentation des BA pendant 30 s de défaut (dernier recours au-delà de 60 s)")
    assert not defauts, f"{ligne} forcée en permanence :\n" + "\n".join(defauts)
