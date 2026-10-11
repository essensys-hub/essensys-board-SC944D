"""NR-board-sc944d-8 — Chien de garde et gestion d'erreur du BA (tâche 7.10).

Exigence : i2c-bus-fault-containment › « Chien de garde et gestion d'erreur du BA » (scénarios « PIC
bloqué avec SCL retenue », « Module SSP figé, boucle principale vivante », « Débordement du module
SSP », « Tampon de commande occupé »).

Vérification (tâche 7.10) : un PIC est figé avec SCL retenue, puis le module SSP est figé avec la
boucle principale vivante, puis un débordement SSP est provoqué, puis le tampon de commande est
occupé ; SCL est relâchée en moins de 2 s, SDA est relâchée moins de 100 ms après le dernier front de
SCL, sans cycle d'alimentation ; le BA redémarré par le chien de garde est resynchronisé en moins de
10 s ; la commande refusée par un NACK est retransmise puis appliquée.

Les défauts sont provoqués par le firmware d'essai du BA de banc (BA 1.8 instrumenté), sur ordre du
banc ; jamais sur une carte d'installation.

Le défaut ne se déclenche qu'à la première transaction du BP vers ce BA, donc après la prochaine
scrutation HTTP du BP. Le test attend le début du blocage, puis observe au moins le délai de
libération plus une marge : un blocage encore en cours à la fin de l'observation est un défaut,
jamais une libération tronquée.
"""

from __future__ import annotations

import pytest

from harnais_banc import analyse
from harnais_banc import seuils as S

pytestmark = [pytest.mark.banc, pytest.mark.banc_requiert("firmware_essai_ba")]


@pytest.fixture(autouse=True)
def _firmware_essai_1_8(_banc_session):
    """Saute le test avant la remise à l'état nominal si le firmware d'essai n'est pas de base 1.8."""
    if _banc_session.config.firmware_ba_essai != "1.8":
        pytest.skip("firmware d'essai du BA d'essai de base 1.8 requis (chien de garde, octet comm_stat)")


def _blocages(banc, ligne: str, debut: float, fin: float) -> list[tuple[float, float]]:
    return analyse.intervalles_significatifs(
        banc.analyseur.intervalles_bas(ligne, debut, fin), S.SEUIL_BAS_SIGNIFICATIF_S
    )


def _observer_blocage(banc, ba: int, defaut: str, ligne: str, delai_liberation_s: float):
    """Provoque ``defaut``, attend le début du blocage de ``ligne`` puis l'observe ``delai_liberation_s``
    plus une marge. Renvoie (début du blocage ou ``None``, fin de l'observation, blocages)."""
    t_defaut = banc.maintenant()
    banc.relais.declencher_defaut_ba(ba, defaut)
    try:
        banc.envoyer_bascules([ba])
        t_blocage = banc.attendre_blocage(ligne, t_defaut, S.DELAI_DECLENCHEMENT_DEFAUT_S)
        if t_blocage is not None:
            banc.attendre(max(0.0, t_blocage + delai_liberation_s + S.MARGE_OBSERVATION_LIBERATION_S - banc.maintenant()))
        t_fin = banc.maintenant()
    finally:
        banc.relais.annuler_defaut_ba(ba)
    return t_blocage, t_fin, _blocages(banc, ligne, t_defaut, t_fin)


# NR: NR-board-sc944d-8 #1
@pytest.mark.nr("NR-board-sc944d-8", "#1")
def test_pic_fige_avec_scl_retenue(banc):
    ba = banc.config.ba_essai
    cycles_avant = banc.console.compteurs().cycles_alimentation
    t_blocage, t_fin, blocages = _observer_blocage(banc, ba, "pic_fige_scl", "SCL", S.DELAI_LIBERATION_SCL_PIC_FIGE_S)

    assert t_blocage is not None and blocages, (
        f"SCL jamais retenue en {S.DELAI_DECLENCHEMENT_DEFAUT_S} s : le défaut d'essai ne s'est pas produit"
    )
    assert not analyse.intervalles_non_relaches(blocages, t_fin), (
        f"SCL toujours retenue {t_fin - t_blocage:.2f} s après le début du blocage"
    )
    assert analyse.duree_max(blocages) < S.DELAI_LIBERATION_SCL_PIC_FIGE_S, (
        f"SCL retenue {analyse.duree_max(blocages):.2f} s (< {S.DELAI_LIBERATION_SCL_PIC_FIGE_S} s attendu)"
    )
    t_liberation = max(fin for _, fin in blocages)
    assert banc.attendre_configuration(ba, t_liberation) is not None, (
        f"BA redémarré par le chien de garde non resynchronisé en {S.DELAI_RESYNCHRONISATION_S} s"
    )
    assert banc.console.compteurs().cycles_alimentation == cycles_avant, "cycle d'alimentation des BA"


# NR: NR-board-sc944d-8 #1
@pytest.mark.nr("NR-board-sc944d-8", "#1")
def test_module_ssp_fige_boucle_principale_vivante(banc):
    ba = banc.config.ba_essai
    cycles_avant = banc.console.compteurs().cycles_alimentation
    # Les tentatives de récupération du BP (une par seconde au plus) produisent des fronts de SCL :
    # l'observation couvre au moins une tentative, puis le délai de libération.
    t_blocage, t_fin, blocages = _observer_blocage(
        banc, ba, "ssp_fige_sda", "SDA", S.PERIODE_MIN_RECUPERATION_S + S.DELAI_LIBERATION_SDA_SSP_S
    )

    assert t_blocage is not None and blocages, (
        f"SDA jamais retenue en {S.DELAI_DECLENCHEMENT_DEFAUT_S} s : le défaut d'essai ne s'est pas produit"
    )
    assert not analyse.intervalles_non_relaches(blocages, t_fin), (
        f"SDA toujours retenue {t_fin - t_blocage:.2f} s après le début du blocage"
    )
    fronts_scl = banc.analyseur.fronts("SCL", t_blocage, t_fin)
    retards = []
    for debut, fin in blocages:
        dernier = analyse.dernier_front_avant([f for f in fronts_scl if f >= debut], fin)
        retards.append(fin - (dernier if dernier is not None else debut))
    assert max(retards) < S.DELAI_LIBERATION_SDA_SSP_S, (
        f"SDA relâchée {max(retards) * 1e3:.0f} ms après le dernier front de SCL "
        f"(< {S.DELAI_LIBERATION_SDA_SSP_S * 1e3:.0f} ms attendu)"
    )
    reprise = banc.reprise([ba])[ba]
    assert reprise is not None, "BA sans acquittement après la réinitialisation de son module SSP"
    assert banc.console.compteurs().cycles_alimentation == cycles_avant, "cycle d'alimentation des BA"


# NR: NR-board-sc944d-8 #1
@pytest.mark.nr("NR-board-sc944d-8", "#1")
def test_debordement_du_module_ssp(banc):
    ba = banc.config.ba_essai
    cycles_avant = banc.console.compteurs().cycles_alimentation
    banc.relais.declencher_defaut_ba(ba, "debordement_ssp")
    try:
        t_defaut = banc.maintenant()
        reprise = banc.reprise([ba], depuis=t_defaut)[ba]
    finally:
        banc.relais.annuler_defaut_ba(ba)

    assert reprise is not None, f"BA sans acquittement {S.DELAI_REPRISE_S} s après un débordement SSP"
    assert banc.console.compteurs().cycles_alimentation == cycles_avant, "cycle d'alimentation des BA"


# NR: NR-board-sc944d-8 #1
@pytest.mark.nr("NR-board-sc944d-8", "#1")
def test_tampon_de_commande_occupe(banc):
    ba = banc.config.ba_essai
    banc.relais.declencher_defaut_ba(ba, "tampon_occupe")
    try:
        t_commande = banc.maintenant()
        application = banc.mesurer_application(banc.envoyer_bascules([ba]), t_commande)[ba]
        banc.attendre(S.PERIODE_TRAFIC_S)
        t_fin = banc.maintenant()
    finally:
        banc.relais.annuler_defaut_ba(ba)

    trames = banc.analyseur.trames(t_commande, t_fin, adresse=ba)
    refus = [trame for trame in trames if trame.erreur in ("nack_adresse", "nack_donnees")]
    assert refus, "aucun NACK du BA dont le tampon de commande est occupé"
    assert not analyse.retransmissions_manquantes(trames, [ba], S.DELAI_REPRISE_S), (
        "commande refusée par un NACK non retransmise puis acquittée"
    )
    assert application is not None, f"commande refusée puis non appliquée en {S.DELAI_REPRISE_S} s"
