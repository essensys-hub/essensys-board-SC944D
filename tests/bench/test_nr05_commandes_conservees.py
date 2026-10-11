"""NR-board-sc944d-5 — Commandes conservées jusqu'à acquittement (tâche 7.7).

Exigence : i2c-bus-fault-containment › « Commandes conservées jusqu'à acquittement » (scénarios
« Commande émise pendant l'absence d'un BA », « Commandes successives pendant l'absence »,
« Commande de volet expirée »).

Vérification (tâche 7.7) : une commande d'éclairage est émise pendant les 5 min d'absence d'un BA,
puis la séquence marche, arrêt, marche, puis une commande de volet reste en attente au-delà du délai
configuré ; la commande n'est pas acquittée à tort, seul l'état final est appliqué moins de 10 s
après le retour du BA, et la commande de volet expirée n'est pas appliquée et est signalée.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


# NR: NR-board-sc944d-5 #1
@pytest.mark.nr("NR-board-sc944d-5", "#1")
def test_commande_emise_pendant_l_absence_d_un_ba(banc):
    """Un BA déconnecté perd son +15 V : ses sorties retombent au repos. La commande émise pendant
    l'absence est donc « marche », depuis un état initial à l'arrêt."""
    ba, par = banc.ba_le_plus_eloigne, banc.moyen_deconnexion()
    sortie = banc.sortie_temoin(ba)
    etat = True
    banc.sorties_a_l_arret()

    banc.relais.deconnecter_ba(ba, par=par)
    try:
        banc.attendre(S.DUREE_ABSENCE_BA_S)
        banc.maitre.commander_sortie(ba, sortie, etat)
        banc.attendre(S.DUREE_OBSERVATION_RETOUR_S)
        en_attente = banc.console.commandes_en_attente(ba)
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    t_retour = banc.maintenant()
    application = banc.mesurer_application({ba: (sortie, etat)}, t_retour)[ba]

    attendue = [c for c in en_attente if c.sortie == sortie and c.etat == etat]
    assert attendue, "la console ne montre pas la commande en attente pendant l'absence du BA"
    assert not any(c.acquittee for c in attendue), "commande marquée acquittée alors que le BA est absent"
    assert application is not None, f"commande non appliquée {S.DELAI_REPRISE_S} s après le retour du BA"


# NR: NR-board-sc944d-5 #1
@pytest.mark.nr("NR-board-sc944d-5", "#1")
def test_seul_l_etat_final_est_applique_au_retour(banc):
    """Depuis l'arrêt, marche, arrêt puis marche pendant l'absence : au retour, la sortie ne doit
    passer qu'une fois à « marche », sans rejouer l'arrêt intermédiaire. Chaque ordre est récupéré
    par le BP avant l'envoi du suivant : il reçoit bien trois ordres successifs, pas un seul lot."""
    ba, par = banc.ba_le_plus_eloigne, banc.moyen_deconnexion()
    sortie = banc.sortie_temoin(ba)
    banc.sorties_a_l_arret()

    banc.relais.deconnecter_ba(ba, par=par)
    try:
        recuperations = [banc.commander_sortie_recuperee(ba, sortie, etat) for etat in (True, False, True)]
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    t_retour = banc.maintenant()
    application = banc.mesurer_application({ba: (sortie, True)}, t_retour)[ba]
    banc.attendre(S.DUREE_OBSERVATION_RETOUR_S)
    etats = analyse.etats_successifs(banc.relais.historique_sortie(ba, sortie, t_retour), t_retour)

    assert None not in recuperations, "essai invalide : un ordre n'a pas été récupéré par le BP pendant l'absence"
    assert application is not None, f"état final « marche » non appliqué {S.DELAI_REPRISE_S} s après le retour"
    assert etats == [True], f"états appliqués au retour : {etats} (attendu : le seul état final « marche »)"


# NR: NR-board-sc944d-5 #1
@pytest.mark.nr("NR-board-sc944d-5", "#1")
def test_commande_de_volet_expiree_non_appliquee_et_signalee(banc):
    ba, volet = banc.config.volet_temoin
    par = banc.moyen_deconnexion()
    expirees_avant = banc.console.compteurs().commandes_expirees

    t_debut = banc.maintenant()
    banc.relais.deconnecter_ba(ba, par=par)
    try:
        banc.maitre.commander_volet(ba, volet, "descendre")
        banc.attendre(banc.config.delai_expiration_volet_s + S.MARGE_EXPIRATION_S)
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    t_retour = banc.maintenant()
    banc.attendre(S.DUREE_OBSERVATION_RETOUR_S)

    actionnees = [
        sortie
        for sortie in banc.config.sorties_volet
        if True in analyse.etats_successifs(banc.relais.historique_sortie(ba, sortie, t_retour), t_retour)
    ]
    expirees = [c for c in banc.console.commandes_expirees(t_debut) if c.ba == ba]

    assert not actionnees, f"commande de volet expirée appliquée au retour du BA (sorties {actionnees})"
    assert expirees, "la console ne signale pas la commande de volet comme expirée"
    assert not any(c.acquittee for c in expirees), "commande de volet expirée marquée acquittée"
    assert banc.console.compteurs().commandes_expirees > expirees_avant, "compteur des commandes expirées inchangé"
