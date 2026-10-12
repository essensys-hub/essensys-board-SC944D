"""Test de garde du harnais de banc (tâche 7.2). Toujours exécuté, sans banc ni instrument.

Exigence : i2c-bus-surge-protection › « Niveaux d'immunité visés sur banc », scénarios « Aucun banc
déclaré » et « Refus d'essai hors banc ». Il vérifie, dans un pytest lancé en sous-processus :

- sans banc déclaré, tous les tests de banc sont « skipped » et aucun stimulus n'est appliqué
  (journal des stimulus vide : aucun instrument instancié) ;
- avec un banc inconnu, ou le banc d'exemple, le harnais sort en erreur sans stimulus.

Il vérifie aussi, sans sous-processus, les garde-fous du registre (réseau de banc isolé, adresses
littérales, cartes de banc, plages d'alimentation), l'impossibilité d'instancier un instrument sans
banc autorisé et le refus d'un serveur legacy qui n'annonce pas le banc déclaré.
"""

from __future__ import annotations

import copy
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from harnais_banc import instruments, registre
from harnais_banc.banc import Banc
from harnais_banc.registre import BancRefuse

DOSSIER_BANC = Path(__file__).resolve().parent
NUMEROS_NR_ATTENDUS = set(range(1, 14))
PREFIXE_NR = "NR-board-sc944d-"


# --------------------------------------------------------------------------- sous-processus


def _lancer_pytest(tmp_path: Path, banc: str | None) -> tuple[subprocess.CompletedProcess, Path, Path]:
    """Lance les seuls tests de banc (``-m banc``) dans un pytest séparé, journal des stimulus actif."""
    journal = tmp_path / "journal-stimulus.jsonl"
    junit = tmp_path / "banc.xml"
    env = {
        cle: valeur
        for cle, valeur in os.environ.items()
        if cle not in (registre.ENV_BANC, instruments.ENV_JOURNAL, "PYTEST_ADDOPTS")
    }
    if banc is not None:
        env[registre.ENV_BANC] = banc
    env[instruments.ENV_JOURNAL] = str(journal)
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    commande = [
        sys.executable, "-m", "pytest", str(DOSSIER_BANC),
        "-m", "banc", "-p", "no:cacheprovider", "-q", "-rs", f"--junitxml={junit}",
    ]
    resultat = subprocess.run(commande, cwd=tmp_path, env=env, capture_output=True, text=True, timeout=300)
    return resultat, journal, junit


def _sortie(resultat: subprocess.CompletedProcess) -> str:
    """Fin de la sortie du sous-processus, identifiants NR masqués pour ne pas fausser la
    consolidation du rapport NR si ce test échoue."""
    return (resultat.stdout + resultat.stderr).replace("NR-", "NR·")[-3000:]


def _stimulus(journal: Path) -> list[str]:
    return journal.read_text(encoding="utf-8").splitlines() if journal.exists() else []


def _cas_junit(junit: Path) -> list[ET.Element]:
    return list(ET.parse(junit).getroot().iter("testcase")) if junit.exists() else []


def _statut(cas: ET.Element) -> str:
    if cas.find("failure") is not None or cas.find("error") is not None:
        return "failed"
    return "skipped" if cas.find("skipped") is not None else "passed"


def test_sans_banc_declare_tests_de_banc_skipped_sans_stimulus(tmp_path):
    resultat, journal, junit = _lancer_pytest(tmp_path, banc=None)

    assert resultat.returncode == 0, _sortie(resultat)
    cas = _cas_junit(junit)
    assert cas, "aucun test de banc collecté"
    non_skipped = [f"{c.get('classname')}::{c.get('name')}" for c in cas if _statut(c) != "skipped"]
    assert not non_skipped, f"tests de banc exécutés sans banc : {non_skipped}"
    assert _stimulus(journal) == [], "stimulus ou instrument consigné sans banc déclaré"

    numeros, issues = set(), set()
    for c in cas:
        for propriete in c.iter("property"):
            if propriete.get("name") == "nr":
                nr_id, issue = propriete.get("value").split()
                assert nr_id.startswith(PREFIXE_NR)
                numeros.add(int(nr_id.removeprefix(PREFIXE_NR)))
                issues.add(issue)
    assert numeros == NUMEROS_NR_ATTENDUS, "identifiants NR du JUnit XML incomplets"
    assert issues == {"#1"}


@pytest.mark.parametrize(
    "banc, motif",
    [
        ("banc-absent-du-registre", "n'est pas dans le registre"),
        ("exemple-fictif", "exemple documenté"),
    ],
)
def test_banc_inconnu_ou_d_exemple_arret_en_erreur_sans_stimulus(tmp_path, banc, motif):
    resultat, journal, junit = _lancer_pytest(tmp_path, banc=banc)

    assert resultat.returncode == pytest.ExitCode.USAGE_ERROR, _sortie(resultat)
    assert registre.PREFIXE in resultat.stderr and motif in resultat.stderr, _sortie(resultat)
    assert _stimulus(journal) == [], "stimulus ou instrument consigné pour un banc refusé"
    executes = [c.get("name") for c in _cas_junit(junit) if _statut(c) != "skipped"]
    assert not executes, f"tests exécutés malgré le refus du banc : {executes}"


# --------------------------------------------------------------------------- registre


@pytest.fixture
def registre_versionne() -> dict:
    pytest.importorskip("yaml")
    return registre.charger_registre()


@pytest.fixture
def entree_valide(registre_versionne) -> dict:
    """Le banc d'exemple du registre, sans son drapeau « exemple » : un banc conforme."""
    entree = copy.deepcopy(registre_versionne["bancs"]["exemple-fictif"])
    entree["exemple"] = False
    return entree


def test_registre_versionne_coherent(registre_versionne):
    reseaux = registre.reseaux_isoles(registre_versionne)
    for nom, entree in registre_versionne["bancs"].items():
        if entree.get("exemple"):
            with pytest.raises(BancRefuse, match="exemple"):
                registre.resoudre(nom, registre_versionne)
            registre.valider_banc(nom, {**entree, "exemple": False}, reseaux)
        else:
            registre.resoudre(nom, registre_versionne)


def test_banc_conforme_accepte(registre_versionne, entree_valide):
    banc = registre.valider_banc("banc-de-garde", entree_valide, registre.reseaux_isoles(registre_versionne))
    assert banc.ordre_ba[-1] == banc.ba_le_plus_eloigne
    assert set(banc.ba) == {0x11, 0x12, 0x13}


def _modifier(entree: dict, chemin: tuple, valeur) -> None:
    cible = entree
    for cle in chemin[:-1]:
        cible = cible[cle]
    cible[chemin[-1]] = valeur


@pytest.mark.parametrize(
    "chemin, valeur, motif",
    [
        (("reseau", "cidr"), "10.200.0.0/24", "hors des réseaux de banc isolés"),
        (("reseau", "bp"), "198.18.11.10", "hors du réseau de banc"),
        (("reseau", "serveur_legacy"), "http://serveur-legacy.example.invalid:8080", "nom d'hôte"),
        (("reseau", "serveur_legacy"), "http://10.200.0.2:8080", "hors du réseau de banc"),
        (("instruments", "maitre", "parametres"), {"url": "https://serveur.example.invalid"}, "nom d'hôte"),
        (("instruments", "oscilloscope", "parametres"), {"ressource": "TCPIP0::10.1.2.3::INSTR"}, "hors du réseau"),
        (("instruments", "analyseur", "parametres"), {"note": "relais vers 10.1.2.3:5025"}, "hors du réseau"),
        (("instruments", "relais", "parametres"), {"adresse": "relais.example.invalid"}, "nom d'hôte"),
        (("instruments", "relais", "parametres"), {"hostname": "relais.example.invalid"}, "nom d'hôte"),
        (("instruments", "analyseur", "parametres"), {"adresse": "2001:db8::5"}, "hors du réseau"),
        (("instruments", "analyseur", "parametres"), {"ip_analyseur": "analyseur:5025"}, "nom d'hôte"),
        (("instruments", "alimentation", "parametres"), {"note": "passerelle [2001:db8::1]:80"}, "hors du réseau"),
        (("instruments", "maitre", "parametres"), {"url": "http://198.18.10.3:8080"}, "même hôte et le même port"),
        (("firmware_ba_essai",), "2.0", "firmware_ba_essai"),
        (("cartes", 0x11), {"destination": "installation"}, "destination « banc » obligatoire"),
        (("alimentations", "bp"), {"min_v": 22.0, "max_v": 30.0, "max_a": 2.0}, "plage du banc"),
        (("fiche",), "/Users/quelquun/fiche.md", "chemin local"),
        (("ba",), {0x11: {"sortie_temoin": 1, "sorties": [1]}}, "trois BA"),
        (("capacites",), ["capacite-inconnue"], "capacités inconnues"),
    ],
)
def test_banc_non_conforme_refuse(registre_versionne, entree_valide, chemin, valeur, motif):
    _modifier(entree_valide, chemin, valeur)
    with pytest.raises(BancRefuse, match=motif):
        registre.valider_banc("banc-de-garde", entree_valide, registre.reseaux_isoles(registre_versionne))


def test_instrument_obligatoire_absent_refuse(registre_versionne, entree_valide):
    del entree_valide["instruments"]["maitre"]
    with pytest.raises(BancRefuse, match="instrument obligatoire absent : maitre"):
        registre.valider_banc("banc-de-garde", entree_valide, registre.reseaux_isoles(registre_versionne))


@pytest.mark.parametrize(
    "cidr",
    [
        "8.8.8.0/24",
        "10.0.0.0/8",
        "10.0.0.0/15",
        "172.16.0.0/15",
        "192.168.0.0/16",
        "100.64.0.0/15",
        "169.254.0.0/16",
        "198.16.0.0/14",
        "fd00::/48",
        "pas-un-reseau",
    ],
)
def test_reseau_isole_non_conforme_refuse(cidr):
    with pytest.raises(BancRefuse):
        registre.reseaux_isoles({"reseaux_banc_isoles": [{"cidr": cidr}]})


@pytest.mark.parametrize("cidr", ["198.18.0.0/15", "198.19.4.0/24", "2001:2::/48", "2001:2:0:1::/64"])
def test_reseau_isole_dans_les_plages_d_essai_accepte(cidr):
    assert registre.reseaux_isoles({"reseaux_banc_isoles": [{"cidr": cidr}]})


def test_aucun_reseau_isole_refuse():
    with pytest.raises(BancRefuse, match="aucun réseau de banc isolé"):
        registre.reseaux_isoles({"reseaux_banc_isoles": []})


def test_declaration_du_banc():
    assert registre.banc_declare({}) is None
    assert registre.banc_declare({registre.ENV_BANC: "  "}) is None
    assert registre.banc_declare({registre.ENV_BANC: " banc-1 "}) == "banc-1"


# --------------------------------------------------------------------------- instruments


class _OscilloscopeFactice(instruments.Oscilloscope):
    def niveau_bas(self, point, ligne, emetteur):
        return 0.0

    def temps_montee(self, point, ligne, debut_v, fin_v):
        return 0.0

    def tension_continue(self, point):
        return 0.0

    def capacite_ligne(self, point, ligne):
        return 0.0


class _AlimentationFactice(instruments.AlimentationLabo):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.consignes = []

    def _regler(self, voie, tension_v, courant_max_a):
        self.consignes.append((voie, tension_v, courant_max_a))

    def activer(self, voie): ...

    def couper(self, voie): ...

    def mesurer_tension(self, voie):
        return 0.0

    def mesurer_courant(self, voie):
        return 0.0


def _concret(interface: type, **methodes) -> type:
    """Classe concrète d'essai : toute méthode abstraite non fournie ne fait rien."""
    corps = {nom: (lambda self, *a, **k: None) for nom in interface.__abstractmethods__}
    corps.update(methodes)
    return type(f"{interface.__name__}Factice", (interface,), corps)


#: Serveur legacy factice qui annonce un autre banc que celui déclaré.
MaitreAutreBanc = _concret(instruments.MaitreCommande, verifier_identite_banc=lambda self: "autre-banc")


def test_aucun_instrument_sans_banc_autorise(tmp_path, monkeypatch):
    journal = tmp_path / "journal.jsonl"
    monkeypatch.setenv(instruments.ENV_JOURNAL, str(journal))
    monkeypatch.setattr(instruments, "_banc_autorise", None)
    with pytest.raises(instruments.StimulusInterdit):
        _OscilloscopeFactice(banc="exemple-fictif")
    monkeypatch.setattr(instruments, "_banc_autorise", "banc-de-garde")
    with pytest.raises(instruments.StimulusInterdit):
        _OscilloscopeFactice(banc="un-autre-banc")
    assert not journal.exists()


def test_alimentation_refuse_une_consigne_hors_limites(tmp_path, monkeypatch):
    monkeypatch.setenv(instruments.ENV_JOURNAL, str(tmp_path / "journal.jsonl"))
    monkeypatch.setattr(instruments, "_banc_autorise", "banc-de-garde")
    limites = {"bp": instruments.LimitesVoie(22.0, 24.0, 2.0)}
    alimentation = _AlimentationFactice(banc="banc-de-garde", parametres={"limites": limites})
    for voie, tension, courant in [("bp", 30.0, 1.0), ("bp", 23.0, 5.0), ("ba_seul", 15.0, 0.1)]:
        with pytest.raises(instruments.StimulusInterdit):
            alimentation.regler(voie, tension, courant)
    alimentation.regler("bp", 23.0, 1.0)
    assert alimentation.consignes == [("bp", 23.0, 1.0)]


def test_serveur_legacy_qui_n_annonce_pas_le_banc_declare_refuse(tmp_path, monkeypatch, registre_versionne, entree_valide):
    """Seul le serveur legacy est ouvert ; les autres pilotes (inexistants) ne sont jamais importés."""
    journal = tmp_path / "journal.jsonl"
    monkeypatch.setenv(instruments.ENV_JOURNAL, str(journal))
    entree_valide["instruments"]["maitre"]["pilote"] = f"{__name__}:MaitreAutreBanc"
    config = registre.valider_banc("banc-de-garde", entree_valide, registre.reseaux_isoles(registre_versionne))
    monkeypatch.setattr(instruments, "_banc_autorise", "banc-de-garde")

    with pytest.raises(BancRefuse, match="annonce le banc « autre-banc »"):
        Banc.ouvrir(config)

    evenements = [json.loads(ligne) for ligne in _stimulus(journal)]
    instancies = [e["role"] for e in evenements if e["evenement"] == "instanciation"]
    assert instancies == ["maitre"]
