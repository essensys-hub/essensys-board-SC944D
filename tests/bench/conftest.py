"""Harnais pytest des tests de non-régression sur banc du SC944D (tâche 7.2).

Change OpenSpec ``i2c-surge-protection-2026-10-007``, issue essensys-hub/essensys-board-SC944D#1.

Comportement (surge › Niveaux d'immunité visés sur banc, scénarios « Aucun banc déclaré » et
« Refus d'essai hors banc ») :

- sans banc déclaré (``ESSENSYS_BENCH`` absente ou vide) : tous les tests marqués ``banc`` sont
  « skipped », le registre n'est pas lu et aucun instrument n'est instancié ;
- banc déclaré absent du registre, banc d'exemple ou banc non conforme : arrêt en erreur
  (``pytest.UsageError``, code de sortie 4) pendant la configuration, avant toute collecte et
  tout stimulus ;
- banc reconnu : les instruments sont instanciés une fois par session, le serveur legacy de banc
  doit annoncer le banc déclaré, et chaque test part de l'état nominal et y revient.

Chaque test NR porte ``@pytest.mark.nr("NR-board-sc944d-<n>", "#1")`` ; le harnais l'écrit dans le
JUnit XML (propriété ``nr``) que ``nonreg_report.py --sources tests/bench`` consolide.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

from harnais_banc import instruments, registre  # noqa: E402
from harnais_banc.registre import ConfigBanc  # noqa: E402

CLE_CONFIG = pytest.StashKey[ConfigBanc | None]()
MOTIF_NR = re.compile(r"^NR-board-sc944d-\d+$")
MOTIF_ISSUE = re.compile(r"^(?:[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)?#\d+$")
RAISON_SANS_BANC = (
    f"aucun banc déclaré ({registre.ENV_BANC}) : test de banc non exécuté, aucun stimulus appliqué"
)


def pytest_addoption(parser: pytest.Parser) -> None:
    groupe = parser.getgroup("banc", "banc d'essai SC944D (tests/bench)")
    groupe.addoption(
        "--banc-endurance",
        action="store_true",
        default=False,
        help="exécute aussi l'endurance de 72 h (NR-board-sc944d-12) sur le banc déclaré",
    )


def pytest_configure(config: pytest.Config) -> None:
    config.addinivalue_line(
        "markers", 'nr(id, issue): test de non-régression, ex. nr("NR-board-sc944d-1", "#1")'
    )
    config.addinivalue_line(
        "markers", "banc: test qui pilote le banc d'essai ; skipped sans banc déclaré (ESSENSYS_BENCH)"
    )
    config.addinivalue_line(
        "markers", "banc_requiert(*capacites): capacités du banc requises (registre benches.yaml)"
    )
    config.addinivalue_line(
        "markers", "endurance: essai de longue durée, exécuté seulement avec --banc-endurance"
    )

    identifiant = registre.banc_declare()
    if identifiant is None:
        instruments.revoquer_autorisation()
        config.stash[CLE_CONFIG] = None
        return
    try:
        banc = registre.resoudre(identifiant)
    except registre.BancRefuse as exc:
        instruments.revoquer_autorisation()
        raise pytest.UsageError(str(exc)) from None
    instruments.autoriser_banc(banc.identifiant)
    config.stash[CLE_CONFIG] = banc


def pytest_unconfigure(config: pytest.Config) -> None:
    instruments.revoquer_autorisation()


def pytest_report_header(config: pytest.Config) -> str:
    banc = config.stash.get(CLE_CONFIG, None)
    if banc is None:
        return "banc d'essai : aucun (tests de banc skipped, aucun stimulus)"
    return f"banc d'essai : {banc.identifiant} — réseau isolé {banc.reseau} — fiche {banc.fiche}"


def _verifier_marqueur_nr(item: pytest.Item, marqueur: pytest.Mark) -> tuple[str, str]:
    if len(marqueur.args) != 2:
        raise pytest.UsageError(f"{item.nodeid} : @pytest.mark.nr(id, issue) attend deux arguments")
    nr_id, issue = (str(arg) for arg in marqueur.args)
    if not MOTIF_NR.match(nr_id) or not MOTIF_ISSUE.match(issue):
        raise pytest.UsageError(
            f"{item.nodeid} : marqueur nr invalide ({nr_id!r}, {issue!r}) ; "
            'attendu nr("NR-board-sc944d-<n>", "#<issue>")'
        )
    return nr_id, issue


def pytest_collection_modifyitems(config: pytest.Config, items: list[pytest.Item]) -> None:
    banc = config.stash.get(CLE_CONFIG, None)
    endurance = config.getoption("--banc-endurance")
    for item in items:
        marqueur_nr = item.get_closest_marker("nr")
        if marqueur_nr is not None:
            nr_id, issue = _verifier_marqueur_nr(item, marqueur_nr)
            item.user_properties.append(("nr", f"{nr_id} {issue}"))
        if item.get_closest_marker("banc") is None:
            continue
        if banc is None:
            item.add_marker(pytest.mark.skip(reason=RAISON_SANS_BANC))
            continue
        item.user_properties.append(("banc", banc.identifiant))
        requises = {str(c) for m in item.iter_markers("banc_requiert") for c in m.args}
        manquantes = sorted(requises - banc.capacites)
        if manquantes:
            item.add_marker(
                pytest.mark.skip(reason=f"banc {banc.identifiant} sans la capacité : {', '.join(manquantes)}")
            )
        elif item.get_closest_marker("endurance") is not None and not endurance:
            item.add_marker(pytest.mark.skip(reason="endurance de 72 h non demandée (--banc-endurance)"))


@pytest.fixture(scope="session")
def _banc_session(request: pytest.FixtureRequest):
    """Banc ouvert une fois par session ; jamais sans banc déclaré et validé."""
    config_banc = request.config.stash.get(CLE_CONFIG, None)
    if config_banc is None:
        pytest.skip(RAISON_SANS_BANC)
    from harnais_banc.banc import Banc

    try:
        banc = Banc.ouvrir(config_banc)
    except registre.BancRefuse as exc:
        pytest.exit(str(exc), returncode=pytest.ExitCode.USAGE_ERROR)
    try:
        yield banc
    finally:
        banc.fermer()


@pytest.fixture
def banc(request: pytest.FixtureRequest):
    """Banc remis à l'état nominal avant et après chaque test de banc."""
    if request.node.get_closest_marker("banc") is None:
        pytest.fail("la fixture « banc » est réservée aux tests marqués @pytest.mark.banc")
    banc_session = request.getfixturevalue("_banc_session")
    banc_session.remettre_etat_nominal()
    yield banc_session
    banc_session.remettre_etat_nominal()
