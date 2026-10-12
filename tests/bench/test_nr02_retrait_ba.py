"""NR-board-sc944d-2 — Retrait d'un BA (tâche 7.4).

Exigences : i2c-bus-fault-containment › « Isolement d'un BA défaillant » (scénario « Retrait d'un BA
dans un coffret ») ; aux-box-interconnect › « Déconnexion individuelle d'un BA ».

Vérification (tâche 7.4) : un BA est retiré par son câble, puis par son cavalier ; les autres BA
reprennent en moins de 10 s, et en moins de 120 s ``(index 12 & 0x1C)`` ne contient que le bit du BA
retiré.

Par son câble, le BA retiré est celui du bout de chaîne (en chaînage, retirer un autre câble coupe
aussi les BA en aval, ce qui est admis avec le renfort 0 V). Par son cavalier, chaque BA est retiré
à tour de rôle ; ce mode demande un banc équipé de cavaliers par port (N2 ou interconnexion).
"""

from __future__ import annotations

import pytest

from harnais_banc import seuils as S

pytestmark = pytest.mark.banc

MODES = [
    pytest.param("cable", id="cable"),
    pytest.param("cavalier", id="cavalier", marks=pytest.mark.banc_requiert("cavaliers_par_port")),
]


# NR: NR-board-sc944d-2 #1
@pytest.mark.nr("NR-board-sc944d-2", "#1")
@pytest.mark.parametrize("par", MODES)
def test_retrait_d_un_ba(banc, par):
    retires = banc.ba if par == "cavalier" else (banc.ba_deconnectable_seul(),)
    defauts: list[str] = []
    for retire in retires:
        banc.attendre_index_sain()
        t_retrait = banc.maintenant()
        banc.relais.deconnecter_ba(retire, par=par)
        try:
            reprise = banc.reprise(banc.autres_ba(retire), depuis=t_retrait)
            delai_index = banc.attendre_bits_ba(S.BIT_INDEX12[retire], depuis=t_retrait)
        finally:
            banc.relais.reconnecter_ba(retire, par=par)

        for ba, duree in reprise.items():
            if duree is None:
                defauts.append(f"retrait de {retire:#04x} par {par} : {ba:#04x} sans reprise en {S.DELAI_REPRISE_S} s")
        if delai_index is None:
            defauts.append(
                f"retrait de {retire:#04x} par {par} : index 12 & 0x1C ≠ {S.BIT_INDEX12[retire]:#04x} "
                f"en {S.DELAI_TELEMETRIE_S} s"
            )
        banc.remettre_etat_nominal()

    assert not defauts, "\n".join(defauts)
