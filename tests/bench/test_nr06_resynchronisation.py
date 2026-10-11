"""NR-board-sc944d-6 — Resynchronisation des configurations (tâche 7.8).

Exigence : i2c-bus-fault-containment › « Resynchronisation des configurations » (scénarios
« Configuration modifiée pendant l'absence d'un BA », « Redémarrage du BP », « BA dont le PIC a été
remplacé »).

Vérification (tâche 7.8) : la configuration est modifiée pendant l'absence d'un BA, puis le BP est
redémarré, puis un BA revient avec une EEPROM vierge ; la configuration complète est acquittée en
moins de 10 s.

La configuration acquittée est décodée par l'analyseur des trames de configuration (modes des
variateurs, temps d'extinction, temps des volets) et comparée à celle du serveur de banc. La fiche
du banc déclare une variante de configuration dont les modes de variateur diffèrent du mode par
défaut : elle est appliquée avant l'effacement de l'EEPROM, pour qu'un BA resté dans ce mode soit
détecté.

Préalables du banc :

- le BP du banc n'a pas de batterie VBATT raccordée, sinon la coupure ne le redémarre pas ; le test
  vérifie le démarrage par la bannière de la console avant de juger la resynchronisation ;
- le firmware d'essai du BA d'essai est de base 1.8 : un BA 1.7 qui redémarre vite peut ne jamais
  être vu absent, et la spec ne garantit sa resynchronisation que par l'octet ``comm_stat``.
"""

from __future__ import annotations

import pytest

from harnais_banc import seuils as S

pytestmark = pytest.mark.banc


# NR: NR-board-sc944d-6 #1
@pytest.mark.nr("NR-board-sc944d-6", "#1")
def test_configuration_modifiee_pendant_l_absence_d_un_ba(banc):
    ba, par = banc.ba_le_plus_eloigne, banc.moyen_deconnexion()
    origine = banc.maitre.configuration(ba)
    variante = banc.maitre.variante_configuration(ba)
    assert variante != origine, "la fiche du banc doit fournir une variante de configuration différente"

    banc.relais.deconnecter_ba(ba, par=par)
    try:
        banc.maitre.modifier_configuration(ba, variante)
        banc.attendre(S.DUREE_OBSERVATION_RETOUR_S)
    finally:
        banc.relais.reconnecter_ba(ba, par=par)
    t_retour = banc.maintenant()
    try:
        delai = banc.attendre_configuration(ba, t_retour)
    finally:
        banc.maitre.modifier_configuration(ba, origine)

    assert delai is not None, (
        f"nouvelle configuration de {ba:#04x} non acquittée {S.DELAI_RESYNCHRONISATION_S} s après son retour"
    )


# NR: NR-board-sc944d-6 #1
@pytest.mark.nr("NR-board-sc944d-6", "#1")
def test_redemarrage_du_bp(banc):
    banc.alim.couper("bp")
    banc.attendre(S.DUREE_COUPURE_BP_S)
    t_alimentation = banc.maintenant()
    banc.alim.activer("bp")

    demarrage = banc.attendre_que(lambda: bool(banc.console.instants_demarrage(t_alimentation)), S.DELAI_ETAT_NOMINAL_S)
    assert demarrage is not None, (
        "essai invalide : la console ne signale aucun démarrage du BP après la coupure "
        "(batterie VBATT raccordée sur le banc ?)"
    )
    attente = banc.attendre_que(lambda: bool(banc.analyseur.trames(t_alimentation)), S.DELAI_ETAT_NOMINAL_S)
    assert attente is not None, "le BP n'a pas remis le bus en service après son redémarrage"
    t_bus = banc.analyseur.trames(t_alimentation)[0].t_debut

    manquants = [ba for ba in banc.ba if banc.attendre_configuration(ba, t_bus) is None]
    assert not manquants, (
        "configuration complète non acquittée moins de "
        f"{S.DELAI_RESYNCHRONISATION_S} s après la mise en service du bus : "
        + ", ".join(f"{ba:#04x}" for ba in manquants)
    )


# NR: NR-board-sc944d-6 #1
@pytest.mark.nr("NR-board-sc944d-6", "#1")
@pytest.mark.banc_requiert("firmware_essai_ba")
def test_ba_revenu_avec_une_eeprom_vierge(banc):
    """Le firmware d'essai efface la configuration du BA puis le redémarre, comme un PIC remplacé.
    La variante de configuration, hors des modes par défaut, est d'abord appliquée et acquittée."""
    ba = banc.config.ba_essai
    if banc.config.firmware_ba_essai != "1.8":
        pytest.skip("firmware d'essai du BA d'essai de base 1.8 requis (octet comm_stat)")
    origine = banc.maitre.configuration(ba)
    variante = banc.maitre.variante_configuration(ba)
    assert variante != origine, "la fiche du banc doit fournir une variante de configuration différente"

    t_variante = banc.maintenant()
    banc.maitre.modifier_configuration(ba, variante)
    try:
        assert banc.attendre_configuration(ba, t_variante, delai_s=S.DELAI_REPRISE_S) is not None, (
            "essai invalide : variante de configuration non acquittée avant l'effacement"
        )
        # Le défaut d'essai se déclenche à la première transaction du BP vers ce BA : une commande la
        # provoque, et la resynchronisation se mesure depuis cette transaction (retour du BA).
        t_defaut = banc.maintenant()
        banc.relais.declencher_defaut_ba(ba, "eeprom_vierge")
        try:
            banc.envoyer_bascules([ba])
            attente = banc.attendre_que(
                lambda: bool(banc.analyseur.trames(t_defaut, adresse=ba)), S.DELAI_DECLENCHEMENT_DEFAUT_S
            )
            assert attente is not None, "essai invalide : aucune transaction vers le BA après l'ordre d'effacement"
            t_retour = banc.analyseur.trames(t_defaut, adresse=ba)[0].t_debut
            delai = banc.attendre_configuration(ba, t_retour)
        finally:
            banc.relais.annuler_defaut_ba(ba)
    finally:
        banc.maitre.modifier_configuration(ba, origine)

    assert delai is not None, (
        f"configuration complète de {ba:#04x} (variateurs compris) non acquittée en "
        f"{S.DELAI_RESYNCHRONISATION_S} s après le retour avec une EEPROM vierge"
    )
