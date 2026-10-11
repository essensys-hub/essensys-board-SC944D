"""NR-board-sc944d-11 — Compatibilité croisée des firmwares et échanges HTTP (tâche 7.13).

Exigence : i2c-bus-fault-containment › « Compatibilité des firmwares et du protocole legacy »
(scénarios « Matrice de compatibilité croisée », « Échanges HTTP inchangés »).

Vérification (tâche 7.13) : les quatre combinaisons (BP 099-37 ou modifié, avec BA 1.7 ou 1.8)
fonctionnent, et la capture HTTP du serveur de banc ne montre aucune différence.

Les firmwares sont montés par le programmateur du banc (BDM, ICSP), uniquement sur les cartes du
banc. Le « candidat » est le build du BP en cours de qualification (tâche 5.12) : ce test fait
partie de la suite qui doit être au vert avant tout flash d'un BP en service, et ne flashe jamais
un BP d'installation. Le protocole HTTP legacy IoT n'est pas modifié : il est seulement capturé.

Les requêtes sont comparées brutes, octet pour octet (``analyse.normaliser_requete``) : ligne de
requête, ordre des en-têtes, sérialisation ``{k:…,v:"…"}`` et format de version compris. Seuls sont
masqués le GUID de ``/api/done/``, la valeur des index et des en-têtes déclarés variables par le
registre, et la valeur de ``Content-Length``, rapportée à la longueur réelle du corps.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = [pytest.mark.banc, pytest.mark.banc_requiert("programmateur")]

COMBINAISONS = [
    pytest.param("099-37", "1.7", id="bp-099-37_ba-1.7"),
    pytest.param("099-37", "1.8", id="bp-099-37_ba-1.8"),
    pytest.param("candidat", "1.7", id="bp-candidat_ba-1.7"),
    pytest.param("candidat", "1.8", id="bp-candidat_ba-1.8"),
]


@pytest.fixture
def firmwares_d_origine(banc):
    """Remonte, après l'essai, les firmwares présents sur le banc avant lui."""
    origine = banc.programmateur.firmwares_montes()
    yield origine
    banc.programmateur.monter(*origine)


def _monter(banc, bp: str, ba: str) -> None:
    banc.programmateur.monter(bp, ba)
    assert banc.programmateur.firmwares_montes() == (bp, ba), "combinaison de firmwares non montée"


# NR: NR-board-sc944d-11 #1
@pytest.mark.nr("NR-board-sc944d-11", "#1")
@pytest.mark.parametrize("firmware_bp,firmware_ba", COMBINAISONS)
def test_matrice_de_compatibilite_croisee(banc, firmwares_d_origine, firmware_bp, firmware_ba):
    _monter(banc, firmware_bp, firmware_ba)
    defauts: list[str] = []

    for ba, duree in banc.reprise(banc.ba, delai_s=S.DELAI_ETAT_NOMINAL_S).items():
        if duree is None:
            defauts.append(f"{ba:#04x} : aucune commande acquittée")
    for ba in banc.ba:
        origine = banc.maitre.configuration(ba)
        t_modification = banc.maintenant()
        banc.maitre.modifier_configuration(ba, banc.maitre.variante_configuration(ba))
        try:
            if banc.attendre_configuration(ba, t_modification, delai_s=S.DELAI_REPRISE_S) is None:
                defauts.append(f"{ba:#04x} : configuration non acquittée")
        finally:
            banc.maitre.modifier_configuration(ba, origine)

    assert not defauts, f"BP {firmware_bp} avec BA {firmware_ba} :\n" + "\n".join(defauts)


def _capturer_sequence_de_reference(banc) -> list:
    """Même état de départ et même séquence de commandes pour les deux firmwares du BP. Chaque ordre
    est récupéré par le BP et appliqué avant le suivant : les deux captures passent par les mêmes
    états, quel que soit le découpage en lots de la scrutation HTTP."""
    banc.remettre_etat_nominal()
    banc.sorties_a_l_arret()
    t_debut = banc.maintenant()
    for ba in banc.ba:
        sortie = banc.sortie_temoin(ba)
        for etat in (True, False):
            t_ordre = banc.maintenant()
            assert banc.commander_sortie_recuperee(ba, sortie, etat) is not None, "ordre non récupéré par le BP"
            assert banc.mesurer_application({ba: (sortie, etat)}, t_ordre)[ba] is not None, "ordre non appliqué"
    banc.attendre(S.DUREE_REFERENCE_HTTP_S)
    return banc.maitre.capture_http(t_debut, banc.maintenant())


# NR: NR-board-sc944d-11 #1
@pytest.mark.nr("NR-board-sc944d-11", "#1")
def test_echanges_http_inchanges(banc, firmwares_d_origine):
    captures = {}
    for firmware_bp in ("099-37", "candidat"):
        _monter(banc, firmware_bp, "1.7")
        captures[firmware_bp] = _capturer_sequence_de_reference(banc)

    differences = analyse.comparer_echanges_http(
        captures["099-37"],
        captures["candidat"],
        banc.config.indices_http_variables,
        banc.config.entetes_http_variables,
    )
    assert not differences, "échanges HTTP différents de ceux du BP 099-37 :\n" + "\n".join(differences)
