"""NR-board-sc944d-3 — Attentes I2C bornées (tâche 7.5).

Exigence : i2c-bus-fault-containment › « Attentes I2C bornées dans le temps » (scénarios « SDA
forcée basse pendant 60 s », « SCL forcée basse pendant 60 s », « Étirement d'horloge d'un BA 1.7 »).

Vérification (tâche 7.5) : SDA puis SCL sont forcées à l'état bas pendant 60 s ; chaque transaction
se termine en 20 ms au plus, le BP reste joignable par le serveur de banc, la téléinformation est
lue et transmise, et les scénarios locaux continuent ; un BA 1.7 qui étire l'horloge au maximum ne
provoque aucune erreur d'attente.

Préalable de tout nouveau build du BP (design.md D8) : ce test et NR-board-sc944d-4 doivent être au
vert sur banc avant tout flash d'un BP en service.

Interaction avec le cycle TE2 de dernier recours (containment › Cycle d'alimentation des BA en
dernier recours) : il n'est permis qu'au-delà de 60 s de blocage. La ligne est donc relâchée 60 s
exactement après le début du forçage, et les contrôles de téléinformation et de scénario se font
après le relâchement, sur la fenêtre du forçage. Un cycle daté avant 60 s de forçage est un défaut ;
un cycle daté à 60 s ou plus, à la limite du seuil, est toléré et consigné dans le JUnit XML
(propriété ``cycle_te2`` du cas), car il consomme le budget de 3 cycles par 24 h du BP de banc.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


# NR: NR-board-sc944d-3 #1
@pytest.mark.nr("NR-board-sc944d-3", "#1")
@pytest.mark.banc_requiert("teleinfo", "scenario_temoin")
@pytest.mark.parametrize("ligne", ["SDA", "SCL"])
def test_ligne_forcee_basse_pendant_60_s(banc, ligne, request):
    ecart_reference = banc.periode_http_reference()

    t_debut = banc.maintenant()
    banc.relais.forcer_ligne_basse(ligne)
    try:
        banc.trafic(banc.ba, t_debut + S.DUREE_FORCAGE_LONG_S - banc.maintenant())
        banc.attendre(max(0.0, t_debut + S.DUREE_FORCAGE_LONG_S - banc.maintenant()))
    finally:
        banc.relais.relacher_ligne(ligne)
    t_fin = banc.maintenant()
    mi_parcours = t_debut + S.DUREE_FORCAGE_LONG_S / 2
    teleinfo = banc.maitre.teleinfo_transmise(mi_parcours, t_fin)
    scenario = banc.maitre.scenario_temoin_execute(mi_parcours, t_fin)

    defauts: list[str] = []
    cycles = banc.console.instants_cycles_alimentation(t_debut, t_fin + S.TOLERANCE_HORODATAGE_S)
    precoces = [t - t_debut for t in cycles if t - t_debut < S.DUREE_FORCAGE_LONG_S]
    if precoces:
        defauts.append(f"cycle d'alimentation des BA après {min(precoces):.1f} s de forçage (dernier recours au-delà de 60 s)")
    elif cycles:
        request.node.user_properties.append(("cycle_te2", f"{len(cycles)} cycle(s) à la limite des 60 s, toléré(s)"))
    durees = [trame.duree for trame in banc.analyseur.trames(t_debut, t_fin)]
    durees += [fin - debut for debut, fin in banc.console.transactions_en_erreur(t_debut, t_fin)]
    if not durees:
        defauts.append("aucune tentative de transaction observée pendant le forçage")
    elif max(durees) > S.TRANSACTION_MAX_S:
        defauts.append(f"transaction de {max(durees) * 1e3:.1f} ms (> {S.TRANSACTION_MAX_S * 1e3:.0f} ms)")

    ecarts = analyse.ecarts_periode_http(banc.maitre.instants_echanges_bp(t_debut, t_fin), t_debut, t_fin)
    if max(ecarts) > ecart_reference * S.FACTEUR_PERIODE_HTTP:
        defauts.append(
            f"échanges HTTP interrompus {max(ecarts):.1f} s (référence {ecart_reference:.1f} s) : BP non joignable"
        )
    if not teleinfo:
        defauts.append("téléinformation non transmise pendant le forçage")
    if not scenario:
        defauts.append("scénario local témoin non exécuté pendant le forçage")

    assert not defauts, f"{ligne} forcée basse :\n" + "\n".join(defauts)


# NR: NR-board-sc944d-3 #1
@pytest.mark.nr("NR-board-sc944d-3", "#1")
@pytest.mark.banc_requiert("firmware_essai_ba")
def test_etirement_d_horloge_maximal_d_un_ba_1_7(banc):
    """Le firmware d'essai du BA reproduit l'étirement d'horloge maximal mesuré sur un BA 1.7
    (tâche 5.1), quelle que soit sa version de base."""
    ba = banc.config.ba_essai
    t_debut = banc.maintenant()
    banc.relais.declencher_defaut_ba(ba, "etirement_max")
    try:
        commandes = banc.trafic([ba], S.DUREE_FORCAGE_LONG_S)
        perdues = banc.commandes_perdues(commandes, t_debut)
        t_fin = banc.maintenant()
    finally:
        banc.relais.annuler_defaut_ba(ba)

    trames = banc.analyseur.trames(t_debut, t_fin, adresse=ba)
    attentes = [trame for trame in trames if trame.erreur == "attente"]
    erreurs_console = banc.console.erreurs_attente(t_debut, ba=ba)

    assert trames, "aucune transaction vers le BA d'essai pendant l'étirement"
    assert not attentes and erreurs_console == 0, (
        f"erreurs d'attente : {len(attentes)} à l'analyseur, {erreurs_console} à la console"
    )
    assert not perdues, f"commandes non appliquées : {perdues}"
