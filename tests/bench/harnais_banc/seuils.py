"""Seuils et constantes des tests de non-régression sur banc du SC944D.

Change OpenSpec : ``i2c-surge-protection-2026-10-007`` (issue essensys-hub/essensys-board-SC944D#1).

Chaque seuil cite l'exigence qui le fixe, sous la forme
``<capacité> › <exigence>``, et la tâche de ``tasks.md`` qui le vérifie. Les capacités sont :

- ``surge``       : ``specs/i2c-bus-surge-protection/spec.md`` ;
- ``containment`` : ``specs/i2c-bus-fault-containment/spec.md`` ;
- ``interconnect``: ``specs/aux-box-interconnect/spec.md``.

Les valeurs marquées « choix du harnais » ne viennent pas de la spec : ce sont des paramètres
d'essai (fenêtres d'observation, tolérances de mesure) à confirmer lors de la première campagne
sur le banc (tâche 7.1) et à faire relire.
"""

from __future__ import annotations

# --------------------------------------------------------------------------- bus et adresses

#: surge › Niveaux I2C conservés à 50 kHz : fréquence du bus, inchangée par toute retouche.
FREQUENCE_BUS_HZ = 50_000

#: surge › Niveaux I2C conservés à 50 kHz : adresses des trois types de BA.
ADRESSES_BA = (0x11, 0x12, 0x13)

#: Type de carte pour chaque adresse (containment › Télémétrie de l'état de chaque BA).
TYPE_BA = {0x11: "SC940D", 0x12: "SC942C", 0x13: "SC941C"}

#: containment › Télémétrie de l'état de chaque BA : bit de l'index 12 (``comm_faults``) par BA.
BIT_INDEX12 = {0x11: 0x04, 0x12: 0x08, 0x13: 0x10}

#: containment › Télémétrie : les critères portent sur ``(index 12 & 0x1C)``.
MASQUE_BA_INDEX12 = 0x1C

#: containment › Télémétrie : bits 0 (compteur) et 1 (IHM), indépendants de l'état des BA.
MASQUE_HORS_BA_INDEX12 = 0x03

#: Index de la table d'échange qui porte ``comm_faults`` (champ ``Information``).
INDEX_COMM_FAULTS = 12

#: Noms des trames de configuration du BP vers un BA (firmware 099-37, ``ba.c``) : la
#: resynchronisation renvoie la configuration complète (containment › Resynchronisation des
#: configurations). Le décodage des codes numériques relève du pilote de l'analyseur.
TRAMES_CONFIGURATION = ("conf_sorties", "tps_extinction", "tps_action")

#: Erreurs de trame relevées par l'analyseur qui signalent une trame perturbée
#: (containment › Branchement à chaud d'un BA : « erreur de CRC ou NACK »).
ERREURS_PERTURBATION = frozenset({"crc", "echo", "nack_adresse", "nack_donnees"})

# --------------------------------------------------------------------------- NR-board-sc944d-1 (7.3)

#: surge › Niveaux I2C conservés à 50 kHz : niveau bas lu au MCF quand un BA émet (strictement <).
NIVEAU_BAS_MCF_MAX_V = 0.85

#: surge › Niveaux I2C conservés à 50 kHz : niveau bas lu au PIC quand le BP émet (strictement <).
NIVEAU_BAS_PIC_MAX_V = 1.2

#: surge › Niveaux I2C conservés à 50 kHz : temps de montée de 0 à 3,45 V au BA le plus éloigné.
TEMPS_MONTEE_MAX_S = 4e-6
TEMPS_MONTEE_DEBUT_V = 0.0
TEMPS_MONTEE_FIN_V = 3.45

#: surge › Niveaux I2C conservés à 50 kHz et › Écrêtage coordonné (scénario « Fuite après
#: remplacement d'un écrêteur ») : fuite de chaque ligne, carte seule (≤).
FUITE_LIGNE_MAX_A = 50e-6

#: Pull-ups de ligne (design.md, Context et D2) : 10 kΩ sur un BA ; sur le BP, R155/R156 valent
#: 1 kΩ d'origine, 1,5 kΩ ou 2,2 kΩ en option renforcée (valeur déclarée par le registre du banc).
#: La spec équivaut 50 µA à 0,5 V sous 10 kΩ et à 50 mV sous 1 kΩ : la fuite se déduit de l'écart
#: entre l'alimentation de la pull-up et la ligne au repos.
PULLUP_BA_OHM = 10_000.0
PULLUP_BP_ORIGINE_OHM = 1_000.0
PULLUPS_BP_ADMISES_OHM = (1_000.0, 1_500.0, 2_200.0)

#: Câbles du banc (tâche 7.1) : 3 × 33 cm en cat. 5e AWG26, puis 1 m, puis 3 m. Sur un bus non
#: isolé, NR-board-sc944d-1 applique à chacun les critères du coffret (tâche 7.3) ; les critères
#: du MIB (1 m et 3 m derrière le module) sont vérifiés par NR-board-sc944d-13.
CONFIGURATIONS_CABLE = ("3x33cm", "1m", "3m")

#: Configuration de câble nominale, remontée par ``Banc.remettre_etat_nominal`` avant chaque test :
#: les trois tronçons de 33 cm représentatifs d'un coffret (tâche 7.1, design.md D10).
CABLE_NOMINAL = "3x33cm"

# --------------------------------------------------------------------------- reprise et télémétrie

#: surge › Niveaux d'immunité visés sur banc (critère B) et containment › Isolement d'un BA
#: défaillant, › Récupération d'un bus bloqué : reprise de la communication.
DELAI_REPRISE_S = 10.0

#: containment › Resynchronisation des configurations : acquittement de chaque trame.
DELAI_RESYNCHRONISATION_S = 10.0

#: containment › Isolement d'un BA défaillant et › Télémétrie : index 12 à jour.
DELAI_TELEMETRIE_S = 120.0

# --------------------------------------------------------------------------- NR-board-sc944d-3 (7.5)

#: containment › Attentes I2C bornées dans le temps : durée maximale d'une transaction.
TRANSACTION_MAX_S = 0.020

#: containment › Attentes I2C bornées (scénarios « SDA/SCL forcée basse pendant 60 s »).
DUREE_FORCAGE_LONG_S = 60.0

#: Choix du harnais : un intervalle entre deux échanges HTTP du BP pendant le défaut ne doit pas
#: dépasser 1,5 fois le plus long intervalle observé avant le défaut (« à sa période habituelle »).
FACTEUR_PERIODE_HTTP = 1.5

#: Choix du harnais : durée d'observation de la période habituelle des échanges HTTP.
DUREE_REFERENCE_HTTP_S = 60.0

#: Choix du harnais : période minimale d'un tour du trafic émis pour solliciter le bus. Chaque tour
#: attend en plus que le BP ait récupéré ses commandes (``GET /api/myactions``) : le BP 099-37 ne
#: scrute le serveur que toutes les 2,15 s environ, ou 5,15 s sans utilisateur connecté
#: (``Ethernet/www.h``). Deux ordres contraires récupérés dans le même lot ne donneraient que
#: l'état final, ce que la spec admet (containment › Commandes conservées) : le trafic les espace.
PERIODE_TRAFIC_S = 2.0

# --------------------------------------------------------------------------- NR-board-sc944d-4 (7.6)

#: containment › Récupération d'un bus bloqué (scénario « Ligne forcée basse puis relâchée »).
DUREE_FORCAGE_BREF_S = 0.5

#: containment › Récupération d'un bus bloqué : au plus une tentative par seconde.
PERIODE_MIN_RECUPERATION_S = 1.0

#: Choix du harnais : fenêtre d'observation d'un défaut permanent.
FENETRE_DEFAUT_PERMANENT_S = 30.0

#: Choix du harnais : tolérance d'horodatage entre deux instants mesurés.
TOLERANCE_HORODATAGE_S = 0.010

# --------------------------------------------------------------------------- NR-board-sc944d-5 (7.7)

#: containment › Commandes conservées (scénario « Commande émise pendant l'absence d'un BA »).
DUREE_ABSENCE_BA_S = 300.0

#: containment › Commandes conservées : délai d'expiration d'un volet, 5 min par défaut.
DELAI_EXPIRATION_VOLET_DEFAUT_S = 300.0

#: Choix du harnais : marge au-delà du délai d'expiration configuré avant le retour du BA.
MARGE_EXPIRATION_S = 30.0

#: Choix du harnais : observation des sorties après le retour d'un BA.
DUREE_OBSERVATION_RETOUR_S = 30.0

# --------------------------------------------------------------------------- NR-board-sc944d-8 (7.10)

#: containment › Chien de garde et gestion d'erreur du BA : un PIC bloqué relâche SCL.
DELAI_LIBERATION_SCL_PIC_FIGE_S = 2.0

#: containment › Chien de garde (scénario « Module SSP figé, boucle principale vivante ») :
#: SDA relâchée après le dernier front de SCL.
DELAI_LIBERATION_SDA_SSP_S = 0.100

#: Choix du harnais : attente du début d'un défaut d'essai. Le défaut ne se déclenche que lorsque le
#: BP adresse le BA, après sa prochaine scrutation HTTP (5,15 s au plus en 099-37).
DELAI_DECLENCHEMENT_DEFAUT_S = 15.0

#: Choix du harnais : marge d'observation au-delà d'un délai de libération, pour qu'un blocage
#: encore en cours ne soit pas tronqué et pris pour une libération.
MARGE_OBSERVATION_LIBERATION_S = 1.0

# --------------------------------------------------------------------------- NR-board-sc944d-9 (7.11)

#: surge › Protection de l'alimentation +15 V du bus et interconnect › Distribution protégée du
#: +15 V : tension minimale des autres ports pendant un court-circuit.
TENSION_MIN_PORT_15V_V = 14.0

#: surge › Protection du +15 V : les autres ports retrouvent 14 V dans les 5 s.
DELAI_RETOUR_AUTRES_PORTS_S = 5.0

#: surge › Protection du +15 V (scénario « Court-circuit du +15 V à la sortie du BP »).
DUREE_COURT_CIRCUIT_S = 60.0

#: surge › Protection du +15 V et interconnect › Distribution protégée : retour après court-circuit.
DELAI_RETOUR_APRES_COURT_CIRCUIT_S = 120.0

#: Choix du harnais, repris du critère de courant d'alimentation de la tâche 7.16 : un courant
#: de repos qui s'écarte de plus de 10 % de sa référence signale une dégradation.
TOLERANCE_COURANT_REPOS = 0.10

#: Choix du harnais : période d'échantillonnage des tensions pendant un court-circuit.
PERIODE_ECHANTILLON_TENSION_S = 0.5

# --------------------------------------------------------------------------- NR-board-sc944d-10 (7.12)

#: Choix du harnais : nombre de branchements puis débranchements pendant le trafic.
CYCLES_BRANCHEMENT = 5

#: Choix du harnais : durée de chaque phase « branché » ou « débranché ».
DUREE_PHASE_BRANCHEMENT_S = 10.0

# --------------------------------------------------------------------------- NR-board-sc944d-12 (7.14)

#: Tâche 7.14 : endurance de 72 h.
DUREE_ENDURANCE_S = 72 * 3600.0

#: Choix du harnais : une commande par BA toutes les 10 s pendant l'endurance.
PERIODE_COMMANDES_ENDURANCE_S = 10.0

#: Choix du harnais : relevé du tas libre du BP toutes les 10 min.
PERIODE_RELEVE_TAS_S = 600.0

#: Choix du harnais : « tas stable » = le minimum du tas libre sur les dernières 24 h n'est pas
#: inférieur à celui des premières 24 h, à cette tolérance près (à confirmer sur banc).
FENETRE_TAS_S = 24 * 3600.0
TOLERANCE_TAS_OCTETS = 0

# --------------------------------------------------------------------------- NR-board-sc944d-13 (7.15)

#: containment › Libération automatique d'un nœud isolé bloqué : côté bus libéré, et chaque
#: essai de raccordement qui retrouve la ligne basse rend le bus libre, en moins de 150 ms.
DELAI_LIBERATION_MIB_S = 0.150

#: containment › Libération automatique : intervalle des essais de raccordement plafonné à 5 s.
INTERVALLE_MAX_ESSAIS_MIB_S = 5.0

#: Choix du harnais : fenêtre d'observation du superviseur (plusieurs essais de raccordement).
FENETRE_OBSERVATION_MIB_S = 30.0

#: interconnect › Module d'isolement pour BA déportés (scénario « Niveaux avec un câble de 3 m »)
#: et tâche 6.7 (niveaux tenus à 1 m et à 3 m) : liaison du BA déporté derrière son MIB.
CONFIGURATIONS_CABLE_MIB = ("mib-1m", "mib-3m")

#: interconnect › Module d'isolement : capacité côté bus, BP compris (≤).
CAPACITE_BUS_MIB_MAX_F = 400e-12

#: interconnect › Module d'isolement : niveau bas au MCF (≤).
NIVEAU_BAS_MCF_MIB_MAX_V = 0.85

#: interconnect › Module d'isolement : niveau bas côté BA (strictement <).
NIVEAU_BAS_COTE_BA_MIB_MAX_V = 0.48

#: interconnect › Module d'isolement : temps de montée de 30 % à 70 % de la tension
#: d'alimentation des pull-ups (≤).
TEMPS_MONTEE_MIB_MAX_S = 1e-6
FRACTION_MONTEE_MIB = (0.30, 0.70)

#: Choix du harnais : un niveau bas plus long que ce seuil n'est pas un bit I2C à 50 kHz ni un
#: étirement d'horloge admis (2 ms au plus par attente côté BP).
SEUIL_BAS_SIGNIFICATIF_S = 0.005

# --------------------------------------------------------------------------- banc

#: design.md D10 et tâche 7.1 : alimentation de laboratoire flottante de 22 à 24 V pour le BP.
TENSION_MIN_ALIM_BP_V = 22.0
TENSION_MAX_ALIM_BP_V = 24.0

#: Tâche 2.8 : un BA mesuré seul est alimenté sous 15,0 V par J1-4/J1-3.
TENSION_MAX_ALIM_BA_SEUL_V = 15.0

#: Choix du harnais : délai de retour à l'état nominal du banc (démarrage du BP compris).
DELAI_ETAT_NOMINAL_S = 180.0

#: Choix du harnais : durée de coupure de l'alimentation du BP pour un redémarrage (NR-6). Le BP du
#: banc n'a pas de batterie VBATT raccordée (fiche du banc) : sinon il ne redémarre pas.
DUREE_COUPURE_BP_S = 5.0

#: Choix du harnais : période de scrutation des sorties et des index.
PERIODE_SCRUTATION_S = 0.1

#: Choix du harnais : période de scrutation du serveur de banc (récupération des commandes par le BP).
PERIODE_SCRUTATION_SERVEUR_S = 0.5


def point_pic(ba: int) -> str:
    """Point de mesure : broche SDA/SCL du PIC du BA."""
    return f"PIC:{ba:#04x}"


def point_entree_15v(ba: int) -> str:
    """Point de mesure : entrée +15 V (BA_ALIM) au connecteur du BA."""
    return f"+15V:entree:{ba:#04x}"


def point_port_15v(ba: int) -> str:
    """Point de mesure : sortie +15 V du port dédié au BA (étoile 0 V N2 ou carte d'interconnexion),
    en aval de la PTC et de l'écrêteur du port, en amont de son cavalier."""
    return f"+15V:port:{ba:#04x}"


def point_alim_pullup(carte: str) -> str:
    """Point de mesure : alimentation des pull-ups de ligne (+5 V du BA, +5VS du BP)."""
    return "+5VS:BP" if carte == "BP" else f"+5V:{carte}"


def point_ligne_carte(carte: str, ligne: str) -> str:
    """Point de mesure : ligne au connecteur de bus d'une carte seule (J31 du BP, J1 d'un BA)."""
    return f"J31:BP:{ligne}" if carte == "BP" else f"J1:{carte}:{ligne}"


POINT_MCF = "MCF"
POINT_J31 = "J31:BP"
POINT_J31_4 = "+15V:J31-4"


def carte_ba(ba: int) -> str:
    """Repère d'une carte BA dans le registre et dans les appels de relais (« 0x11 »)."""
    return f"{ba:#04x}"
