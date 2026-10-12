"""NR-board-sc944d-12 — Endurance de 72 h (tâche 7.14).

Exigences : i2c-bus-fault-containment › « Commandes conservées jusqu'à acquittement » et
« Compatibilité des firmwares et du protocole legacy » (le BP n'est flashé en service qu'après la
suite NR au vert, dont cette endurance : design.md, Migration Plan).

Vérification (tâche 7.14) : 0 NACK d'adresse non expliqué, 0 commande perdue, 0 erreur de CRC et un
tas du BP stable. Aucun défaut n'est provoqué pendant l'endurance : tout NACK d'adresse est donc
inexpliqué.

Exécution seulement avec ``--banc-endurance`` (72 h de banc) ; sinon « skipped ».
« Tas stable » (choix du harnais, voir ``seuils.FENETRE_TAS_S``) : le minimum du tas libre sur les
dernières 24 h n'est pas inférieur à celui des premières 24 h.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = [pytest.mark.banc, pytest.mark.endurance]

BILAN_S = 3600.0


# NR: NR-board-sc944d-12 #1
@pytest.mark.nr("NR-board-sc944d-12", "#1")
def test_endurance_72_h(banc):
    t_debut = banc.maintenant()
    fin = t_debut + S.DUREE_ENDURANCE_S
    commandes: dict[int, list] = {ba: [] for ba in banc.ba}
    tas: list[tuple[float, int]] = []
    nacks = crc = 0
    debut_bilan, prochain_tas = t_debut, t_debut

    while banc.maintenant() < fin:
        for ba, liste in banc.trafic(
            banc.ba, S.PERIODE_COMMANDES_ENDURANCE_S, S.PERIODE_COMMANDES_ENDURANCE_S, suite_de=commandes
        ).items():
            commandes[ba].extend(liste)
        maintenant = banc.maintenant()
        if maintenant >= prochain_tas:
            tas.append((maintenant, banc.console.tas_libre()))
            prochain_tas += S.PERIODE_RELEVE_TAS_S
        if maintenant - debut_bilan >= BILAN_S:
            trames = banc.analyseur.trames(debut_bilan, maintenant)
            nacks += len(analyse.nack_adresse(trames))
            crc += len(analyse.erreurs_crc(trames))
            debut_bilan = maintenant
    perdues = banc.commandes_perdues(commandes, t_debut)
    t_fin = banc.maintenant()
    trames = banc.analyseur.trames(debut_bilan, t_fin)
    nacks += len(analyse.nack_adresse(trames))
    crc += len(analyse.erreurs_crc(trames))
    tas.append((t_fin, banc.console.tas_libre()))
    defauts: list[str] = []
    if nacks:
        defauts.append(f"{nacks} NACK d'adresse non expliqués")
    if crc:
        defauts.append(f"{crc} erreurs de CRC ou d'écho")
    if perdues:
        defauts.append("commandes perdues : " + ", ".join(f"{ba:#04x} : {len(l)}" for ba, l in perdues.items()))
    if not analyse.tas_stable(tas, S.FENETRE_TAS_S, S.TOLERANCE_TAS_OCTETS):
        defauts.append(f"tas du BP instable : {tas[0][1]} octets libres au début, {tas[-1][1]} à la fin")
    assert not defauts, "\n".join(defauts)
