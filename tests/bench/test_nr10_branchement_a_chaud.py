"""NR-board-sc944d-10 — Branchement à chaud d'un BA (tâche 7.12).

Exigence : i2c-bus-fault-containment › « Branchement à chaud d'un BA » (scénario « Branchement d'un
BA pendant le trafic »).

Vérification (tâche 7.12) : un BA est branché puis débranché pendant un trafic soutenu ; aucune
commande destinée aux autres BA n'est perdue ; chaque trame perturbée (erreur de CRC ou NACK) est
retransmise puis acquittée.

Le trafic inverse la sortie témoin de chaque autre BA à chaque tour, après que le BP a récupéré le
tour précédent (``Banc.trafic``) : une commande est perdue si son état n'est pas appliqué avant que
le BP ne récupère la commande suivante.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


# NR: NR-board-sc944d-10 #1
@pytest.mark.nr("NR-board-sc944d-10", "#1")
def test_branchement_et_debranchement_pendant_un_trafic_soutenu(banc):
    ba, par = banc.ba_le_plus_eloigne, banc.moyen_deconnexion()
    autres = banc.autres_ba(ba)
    commandes: dict[int, list] = {autre: [] for autre in autres}

    banc.relais.deconnecter_ba(ba, par=par)
    t_debut = banc.maintenant()
    try:
        for _ in range(S.CYCLES_BRANCHEMENT):
            for manoeuvre in (banc.relais.reconnecter_ba, banc.relais.deconnecter_ba):
                manoeuvre(ba, par=par)
                for autre, liste in banc.trafic(autres, S.DUREE_PHASE_BRANCHEMENT_S, suite_de=commandes).items():
                    commandes[autre].extend(liste)
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    perdues = banc.commandes_perdues(commandes, t_debut)
    t_fin = banc.maintenant()
    trames = banc.analyseur.trames(t_debut, t_fin)
    non_retransmises = analyse.retransmissions_manquantes(trames, autres, S.DELAI_REPRISE_S)

    assert any(commandes.values()), "aucun trafic émis pendant l'essai"
    assert not perdues, "commandes perdues vers les autres BA : " + "; ".join(
        f"{autre:#04x} : {len(liste)}" for autre, liste in perdues.items()
    )
    assert not non_retransmises, "trames perturbées non retransmises puis acquittées : " + "; ".join(
        f"{t.adresse:#04x} {t.code} {t.erreur} à {t.t_debut - t_debut:.3f} s" for t in non_retransmises
    )
