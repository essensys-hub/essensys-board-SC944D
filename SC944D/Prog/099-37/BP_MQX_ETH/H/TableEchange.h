// Version #3.0

// table de données applicatives
// table d'échange avec l'écran / table d'échange avec les boitiers / table d'échange Ethernet

// légende
// BP : boitier principal
// BA : boitier auxiliaire
// PDV : pièces de vie
// PDE : pièces d'eau
// CHB : chambres
// IHM : interface homme machine (l'écran)

// mode automatique : programmation horaire sur 1 semaine
// 1 ordre pour 1 heure sur 7 jours : 6 modes possibles (4 bits) (cf consigne mode de chauffage)
// soit 84 octets 
#define uc_PLANNING_CHAUFFAGE_TAILLE	84

// arrosage 1 ordre pour 30 minutes sur 7 jours : 2 modes possibles (1 bit), soit 42 octets
#define uc_PLANNING_ARROSAGE_TAILLE		42

// nb variateurs possibles par boitier auxiliaire
#define uc_NB_VARIATEURS_POSSIBLES_PAR_BA	8

// nb lampes possibles par boitier auxiliaure
#define uc_NB_LAMPES_POSSIBLES_PAR_BA		16

// nb volets possibles par boitier auxiliaire
#define uc_NB_VOLETS_POSSIBLES_PAR_BA		8

// Clé d'accès distance : 32 chiffres de 0 à 9 -> 16 octets
#define Cle_Acces_Distance_TAILLE			16	// !!!! Utilisé pour les adresses de stockage en EEPROM SERIE !!!!


// Définition des paramètres de l'alarme
enum enumAlarmeConfig
{
	AlarmeConfig_Code,					// 1 = demander le code de l'alarme pour la mettre en route (0 : mise en route alarme sans code) - Géré par écran
	AlarmeConfig_Detect1,				// détecteur de présence 1 désactivé (0) ou utilisé (1)
	AlarmeConfig_Detect2,				// détecteur de présence 2 désactivé (0) ou utilisé (1)
	AlarmeConfig_DetectOuv,				// détecteur d'ouverture désactivé (0) ou utilisé (1)
	AlarmeConfig_Detect1SurVoieAcces,	// détecteur de présence 1 pas sur voie d'accès (0) ou sur voie d'accès (1)
	AlarmeConfig_Detect2SurVoieAcces,	// détecteur de présence 2 pas sur voie d'accès (0) ou sur voie d'accès (1)
	AlarmeConfig_DetectOuvSurVoieAcces,	// détecteur d'ouverture pas sur voie d'accès (0) ou sur voie d'accès (1)
	AlarmeConfig_SireneInt,				// 0= ne rien faire, 1= sirène intérieure activée
	AlarmeConfig_SireneExt,				// 0= ne rien faire, 1= sirène extérieure activée
	AlarmeConfig_BloqueVolets,			// 0= ne rien faire, 1 = Volets roulants bloqués en cas d'alarme
	AlarmeConfig_ForcerEclairage,		// 0= ne rien faire, 1 = Forcer allumage de toutes les éclairages
	
	AlarmeConfig_NB_VALEURS
};

// Définition des paramètres pour un scénario
enum enumScenario
{
	Scenario_Confirme_Scenario,		// 1 = demander confirmation - Géré par écran
	
	// alarme
	Scenario_Alarme_ON,			// 0 = ne rien faire, 1= mettre l'alarme, 2 = enlever l'alarme - Géré par écran
	Scenario_AlarmeConfig,		// -> voir enumAlarmeConfig

	// éclairage
	Scenario_Eteindre_PDV_LSB = Scenario_AlarmeConfig + AlarmeConfig_NB_VALEURS,
							// 0  = ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues
							// 1  = éteindre la lampe de l'entrée
							// 2  = éteindre la lampe du salon
							// 4  = éteindre la lampe du salon
							// 8  = éteindre la lampe du dressing 1
							// 16 = éteindre la lampe du dressing 2
	Scenario_Eteindre_PDV_MSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues 
							// 32 = éteindre le variateur du bureau
							// 64 = éteindre le variateur de la salle à manger
							// 128= éteindre le variateur du salon
	Scenario_Eteindre_CHB_LSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues
							// 1  = éteindre la lampe de l'escalier
							// 2  = éteindre la lampe 1 de la grande chambre
							// 4  = éteindre la lampe 2 de la grande chambre
							// 8  = éteindre la lampe 1 de la petite chambre 1
							// 16 = éteindre la lampe 2 de la petite chambre 1
							// 32 = éteindre la lampe de la petite chambre 2
							// 64 = éteindre la lampe de la petite chambre 3
	Scenario_Eteindre_CHB_MSB,	// 0= ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues
							// 16 = éteindre le variateur de la petite chambre 3
							// 32 = éteindre le variateur de la petite chambre 2
							// 64 = éteindre le variateur de la petite chambre 1
							// 128= éteindre le variateur de la grande chambre
	Scenario_Eteindre_PDE_LSB,	// 0= ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues
							// 1  = éteindre la lampe 1 de la cuisine
							// 2  = éteindre la lampe 2 de la cuisine
							// 4  = éteindre la lampe de la salle de bain 1
							// 8  = éteindre la lampe 1 de la salle de bain 2
							// 16  = éteindre la lampe 2 de la salle de bain 2
							// 32 = éteindre la lampe du WC 1
							// 64 = éteindre la lampe du WC 2
							// 128= éteindre la lampe de service
	Scenario_Eteindre_PDE_MSB,	// 0= ne rien faire / combinaison des valeurs suivantes pour éteindre les lampes voulues
							// 1  = éteindre la lampe du dégagement 1
							// 2  = éteindre la lampe du dégagement 2
							// 4  = éteindre la lampe de la terrasse
							// 8  = éteindre la lampe 1 de l'annexe
							// 16 = éteindre la lampe 2 de l'annexe
							// 128= éteindre le variateur de la salle de bain 1
	Scenario_Allumer_PDV_LSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues
							// 1  = allumer la lampe de l'entrée
							// 2  = allumer la lampe 1 du salon
							// 4  = allumer la lampe 2 du salon
							// 8  = allumer la lampe du dressing 1
							// 16 = allumer la lampe du dressing 2
	Scenario_Allumer_PDV_MSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues 
							// 32 = allumer le variateur du bureau
							// 64 = allumer le variateur de la salle à manger
							// 128= allumer le variateur du salon
	Scenario_Allumer_CHB_LSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues
							// 1  = allumer la lampe de l'escalier
							// 2  = allumer la lampe 1 de la grande chambre
							// 4  = allumer la lampe 2 de la grande chambre
							// 8  = allumer la lampe 1 de la petite chambre 1
							// 16 = allumer la lampe 2 de la petite chambre 1
							// 32 = allumer la lampe de la petite chambre 2
							// 64 = allumer la lampe de la petite chambre 3
	Scenario_Allumer_CHB_MSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues
							// 16 = allumer le variateur de la petite chambre 3
							// 32 = allumer le variateur de la petite chambre 2
							// 64 = allumer le variateur de la petite chambre 1
							// 128= allumer le variateur de la grande chambre
	Scenario_Allumer_PDE_LSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues
							// 1  = allumer la lampe 1 de la cuisine
							// 2  = allumer la lampe 2 de la cuisine
							// 4  = allumer la lampe de la salle de bain 1
							// 8  = allumer la lampe 1 de la salle de bain 2
							// 16  = allumer la lampe 2 de la salle de bain 2
							// 32 = allumer la lampe du WC 1
							// 64 = allumer la lampe du WC 2
							// 128= allumer la lampe de service
	Scenario_Allumer_PDE_MSB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour allumer les lampes voulues
							// 1  = allumer la lampe du dégagement 1
							// 2  = allumer la lampe du dégagement 2
							// 4  = allumer la lampe de la terrasse
							// 8  = allumer la lampe 1 de l'annexe
							// 16 = allumer la lampe 2 de l'annexe
							// 128= allumer le variateur de la salle de bain 1
	// volets & store					
	Scenario_OuvrirVolets_PDV,	// 0  = ne rien faire / combinaison des valeurs suivantes pour ouvrir les volets voulus
							// 1  = ouvrir le volet 1 du salon
							// 2  = ouvrir le volet 2 du salon
							// 4  = ouvrir le volet 3 du salon
							// 8  = ouvrir le volet 1 de la salle à manger
							// 16 = ouvrir le volet 2 de la salle à manger
							// 32 = ouvrir le volet du bureau
	Scenario_OuvrirVolets_CHB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour ouvrir les volets voulus
							// 1  = ouvrir le volet 1 de la grande chambre
							// 2  = ouvrir le volet 2 de la grande chambre
							// 4  = ouvrir le volet de la petite chambre 1
							// 8  = ouvrir le volet de la petite chambre 2
							// 16 = ouvrir le volet de la petite chambre 3
	Scenario_OuvrirVolets_PDE,	// 0  = ne rien faire / combinaison des valeurs suivantes pour ouvrir les volets voulus
							// 1  = ouvrir le volet 1 de la cuisine
							// 2  = ouvrir le volet 2 de la cuisine
							// 4  = ouvrir le volet de la salle de bain 1
							// 8  = remonter le store de la terrasse
	Scenario_FermerVolets_PDV,	// 0  = ne rien faire / combinaison des valeurs suivantes pour fermer les volets voulus
							// 1  = fermer le volet 1 du salon
							// 2  = fermer le volet 2 du salon
							// 4  = fermer le volet 3 du salon
							// 8  = fermer le volet 1 de la salle à manger
							// 16 = fermer le volet 2 de la salle à manger
							// 32 = fermer le volet du bureau
	Scenario_FermerVolets_CHB,	// 0  = ne rien faire / combinaison des valeurs suivantes pour fermer les volets voulus
							// 1  = fermer le volet 1 de la grande chambre
							// 2  = fermer le volet 2 de la grande chambre
							// 4  = fermer le volet de la petite chambre 1
							// 8  = fermer le volet de la petite chambre 2
							// 16 = fermer le volet de la petite chambre 3
	Scenario_FermerVolets_PDE,	// 0  = ne rien faire / combinaison des valeurs suivantes pour fermer les volets voulus
							// 1  = fermer le volet 1 de la cuisine
							// 2  = fermer le volet 2 de la cuisine
							// 4  = fermer le volet de la salle de bain 1
							// 8  = sortir le store de la terrasse
	// sécurité
	Scenario_Securite,					// 0= ne rien faire, 1= couper les prises de sécurité, 2= remettre les prises de sécurité
	Scenario_Machines,					// 0= ne rien faire, 1= couper les machines à laver, 2= remettre les machines à laver
	
	// chauffage : consigne à appliquer immédiatement
    // b0-b3 : consigne : 0 = OFF / 1 = CONFORT / 2 = ECO / 3 = ECO+ / 4 = ECO++ / 5 = HORS GEL
    // b4-b5 : mode : 0 = automatique / 1 = forcé / 2 = anticipé
    // b6 : 1 = Reprendre le dernier fonctionnement mémorisé
    // b7 : 1 = Continuer le fonctionnement actuel
    // :=> 0x00 à 0x05 := forçage en automatique (et suivi consigne en cours)
    // :=> 0x10 à 0x15 := forçage en mode forcé et consigne de forçage
    // :=> 0x20 à 0x25 := forçage en mode anticipé (et suivi consigne en cours)
    // :=> 0x40 := Reprendre le dernier fonctionnement mémorisé
    // :=> 0x80 := Continuer le fonctionnement actuel
	Scenario_Chauf_zj,					// consigne de chauffage zone jour
	Scenario_Chauf_zn,					// consigne de chauffage zone nuit
	Scenario_Chauf_zsb1,				// consigne de chauffage zone salle de bain 1
	Scenario_Chauf_zsb2,				// consigne de chauffage zone salle de bain 2
	
	// cumulus
	// 0x00 = Forçage mode autonome (= HP permanentes) <- idem "Cumulus_Mode"
	// 0x01 = Forçage gestion HP/HC <- idem "Cumulus_Mode"
	// 0x02 = Forçage OFF (= HC permanent) <- idem "Cumulus_Mode"
	// 0x40 := Reprendre le dernier fonctionnement mémorisé
	// 0x80 := Continuer le fonctionnement actuel
	Scenario_Cumulus,
	
	// réveil
	Scenario_Reveil_Reglage,			// 1 = dérouler la procédure de réglage des réveils
	
	Scenario_Reveil_ON,					// 0 : ne rien faire – Utilisé par le scénario « je me lève »
										// 1 : armer la fonction réveil – Utilisé par le scénario « je vais me coucher » si la fonction réveil est activée
										// 2 : désactiver la fonction réveil – Utilisé par le scénario « je vais me coucher » si la fonction réveil est désactivée

	Scenario_Efface,					// 1= scénario à effacer (remise à 0 de tous les paramètres du scénario)
										// 2 à 6 = init par défaut de scénario prédéfini
										// 0= ne rien faire / commande prise en compte par le BP
	Scenario_NB_VALEURS
};


enum Tbb_Donnees_Index
{
// ----------------					version					----------------
	Version_SoftBP_Embedded,						// Version soft BP (numéro sauvegarde)
	Version_SoftBP_Web,								// Version Web (envoyée au serveur pour gestion téléchargement - version de publication sur serveur)
	Version_SoftIHM_Majeur,							// Version soft IHM
	Version_SoftIHM_Mineur,							// Version soft IHM
	Version_TableEchange,							// version table de données (pour vérification compatibilité)
	
// ----------------		heure / date du boitier principal	----------------
	Minutes,					// ! ne pas modifié l'ordre de déclaration sans reprendre le code associé !
	Heure,						// ! Minutes est utilisé comme indice de base !
	Jour,						// Mise a jour de ces champs en une fois sous MUTEX xxx
	Mois,
	Annee,
			
// ----------------				Etat du système				----------------
	Status,			
		// bit 0 : 1= heures creuses en cours
		// bit 1 : 1= délestage en cours
		// bit 2 : 1= mode secouru
	Alerte,	
		// bit 0 : 1= déclenchement alarme
		// bit 1 : 1= déclenchement alerte suite à détection fuite d'eau lave linge
		// bit 2 : 1= déclenchement alerte suite à détection fuite d'eau lave vaisselle
	Information,	
		// bit 0 : 1= défaut com compteur ERDF
		// bit 1 : 1= défaut com IHM
		// bit 2 : 1= défaut com BA PDV 
		// bit 3 : 1= défaut com BA CHB
		// bit 4 : 1= défaut com BA PDE

// ----------------				chauffage					----------------
	// mode automatique : programmation horaire sur 1 semaine
	// 1 ordre pour 1 heure sur 7 jours : 6 modes possibles (4 bits)
	// b0-b3 : consigne : 0= OFF / 1= CONFORT / 2= ECO / 3= ECO+ / 4= ECO++ / 5= HORS GEL
	// soit 84 octets 
	Chauf_zj_Auto,														// chauffage zone jour
	Chauf_zn_Auto	= Chauf_zj_Auto + uc_PLANNING_CHAUFFAGE_TAILLE,		// chauffage zone nuit
	Chauf_zsb1_Auto	= Chauf_zn_Auto + uc_PLANNING_CHAUFFAGE_TAILLE,	 	// chauffage salle de bain 1
	Chauf_zsb2_Auto	= Chauf_zsb1_Auto + uc_PLANNING_CHAUFFAGE_TAILLE,	// chauffage salle de bain 2

	// chauffage : consigne à appliquer immédiatement
    // b0-b3 : consigne : 0 = OFF / 1 = CONFORT / 2 = ECO / 3 = ECO+ / 4 = ECO++ / 5 = HORS GEL
    // b4-b5 : mode : 0 = automatique / 1 = forcé / 2 = anticipé
    // :=> 0x00 à 0x05 := forçage en automatique (et suivi consigne en cours)
    // :=> 0x10 à 0x15 := forçage en mode forcé et consigne de forçage
    // :=> 0x20 à 0x25 := forçage en mode anticipé (et suivi consigne en cours)
	Chauf_zj_Mode = Chauf_zsb2_Auto + uc_PLANNING_CHAUFFAGE_TAILLE,		// chauffage zone jour - PROTECTION PAR MUTEX
	Chauf_zn_Mode,														// chauffage zone nuit - PROTECTION PAR MUTEX
	Chauf_zsb1_Mode,													// chauffage salle de bain 1 - PROTECTION PAR MUTEX
	Chauf_zsb2_Mode,													// chauffage salle de bain 2 - PROTECTION PAR MUTEX
	// MUTEX : un nouvel ordre peut provenir de l'écran pendant que BP est en train de traiter cet octet
	//         sans mutex, l'ordre risque d'être effacé
	//		   avec mutex, l'ordre de l'écran sera pris en compte après traitement donc pas de risque de perte...
	
// ----------------				Cumulus						----------------
	Cumulus_Mode,				// 0 = Autonome (ON) / 1 = gestion heures creuses / 2 = OFF

// ----------------					vacances				----------------
	// date et heure de fin de vacances : 
	VacanceFin_H,								// heure //xxx a proteger par mutex
	VacanceFin_Mn,								// minute 
	VacanceFin_J,								// jour 
	VacanceFin_M,								// mois 
	VacanceFin_A,								// année 

	// chauffage fin de vacances : consigne forcée
    // b0-b3 : consigne : 0 = OFF / 1 = CONFORT / 2 = ECO / 3 = ECO+ / 4 = ECO++ / 5 = HORS GEL
    // b4-b5 : mode : 0 = automatique / 1 = forcé / 2 = anticipé
    // b6 : 1 = Reprendre le dernier fonctionnement mémorisé
    // b7 : 1 = Continuer le fonctionnement actuel
    // :=> 0x00 à 0x05 := forçage en automatique (et suivi consigne en cours)
    // :=> 0x10 à 0x15 := forçage en mode forcé et consigne de forçage
    // :=> 0x20 à 0x25 := forçage en mode anticipé (et suivi consigne en cours)
    // :=> 0x40 := Reprendre le dernier fonctionnement mémorisé - NON GERE DANS VACANCE FIN !!! -> IDEM 0x80
    // :=> 0x80 := Continuer le fonctionnement actuel
	VacanceFin_zj_Force,						// chauffage zone jour
	VacanceFin_zn_Force,						// chauffage zone nuit
	VacanceFin_zsb1_Force,						// chauffage salle de bain 1
	VacanceFin_zsb2_Force,						// chauffage salle de bain 2
	
// ----------------					arrosage				----------------
	// sélection du mode
	Arrose_Mode,			// 0 = OFF : pas d'arrosage
							// 1 à 254 : durée d'arrosage en mode forcé
							// 255 : mode automatique : pilotage en fonction du planning horaire
	// mode automatique : programmation horaire sur 1 semaine
	Arrose_Auto,			// 1 ordre pour 30 minutes sur 7 jours : 2 modes possibles (1 bit), soit 42 octets
	// détecteur de pluie
	Arrose_Detect= Arrose_Auto + uc_PLANNING_ARROSAGE_TAILLE,	// 0x00 := détecteur inactif (OFF) / 0x01 := détecteur utilisé			
	
// ----------------					alarme					----------------
	Alarme_AccesADistance,		// Autorise (1) ou non (0) la modification de l'état de l'alarme à distance - Renseigné par écran (mis à 1 quand il passe en veille)
	Alarme_Mode,				// Définie le mode de fonctionnement de l'alarme
								// 		0x00	Le système d'alarme n'est pas utilisé
								//  	0x01	Mode réglage
								//  	0x02	Alarme indépendante
								//		0x03	Alarme sur scénario "Je sors"
								//		0x04	Alarme sur scénario "Je vais me coucher"
								//		0x05	Alarme sur scénario "Je pars en vacances"
								//		0x06	Alarme sur scénario "Personnalisé"
	Alarme_Commande,			// 1 Demande de mise sous alarme -> repassé à 0 par BP après prise en compte
	Alarme_CodeSaisiLSB,		// Bit 0-3 : 1er chiffre (le + à gauche à l'écran) - Bit 4-7 : 2eme chiffre
	Alarme_CodeSaisiMSB,		// Bit 0-3 : 3eme chiffre - Bit 4-7 : 4eme chiffre (le + à droite à l'écran)
								// Mettre / désactiver alarme si code LSB / MSB correct -> remis à 0xFFFF par BP après prise en compte
	Alarme_Autorisation,		// 0x00 : traitement en attente
								// 0x01 : code valide
								// 0x02 : code invalide
	Alarme_SuiviAlarme,			// Suit le déroulement des étapes de l'automate	//xxx mettre en lecture seule !!!
								// 		0x00	Etape de départ
								//  	0x01	Mise sous alarme impossible -> problème d'alimentation
								//  	0x02	Mise sous alarme impossible -> intrusion ou vandalisme
								//		0x03	Procédure de sortie
								//		0x04	Régime de croisière
								//		0x05	Procédure d'entrée
								//		0x06	Intrusion ou vandalisme
	Alarme_Detection,			// Par bit - 0 : pas de détection - 1 : détection	//xxx mettre en lecture seule
								//		bit 0 : état du détecteur d'ouverture
								//		bit 1 : état du détecteur présence 1
								//		bit 2 : état du détecteur présence 2
	Alarme_Fraude,				// Par bit - 0 : pas de fraude - 1 : fraude
								//		bit 0 : état de fraude tableau domotique
								//		bit 1 : état de fraude IHM
								//		bit 2 : état de fraude détecteur présence 1
								//		bit 3 : état de fraude détecteur présence 2
								//		bit 4 : état de fraude sirène d'intérieur
								//		bit 5 : état de fraude sirène d'extérieur
								//		bit 6 : état batterie - 0 : présente - 1 : non présente
	Alarme_SuiviChangementCode,	// 0x00 : état de départ
								// 0x01 : 1er envoi nouveau code
								// 0x02 : 2ème envoi nouveau code
	Alarme_CodeUser1LSB,		// Code alarme 1 Bit 0-3 : 1er chiffre (le + à gauche) - Bit 4-7 : 2eme chiffre
	Alarme_CodeUser1MSB,		// Code alarme 1 Bit 0-3 : 3eme chiffre - Bit 4-7 : 4eme chiffre (le + à droite)
	Alarme_CodeUser2LSB,		// Code alarme 2 Bit 0-3 : 1er chiffre (le + à gauche) - Bit 4-7 : 2eme chiffre						- NON UTILISE ACTUELLEMENT !
	Alarme_CodeUser2MSB,		// Code alarme 2 Bit 0-3 : 3eme chiffre - Bit 4-7 : 4eme chiffre (le + à droite)					- NON UTILISE ACTUELLEMENT !
	Alarme_CompteARebours,		// Temps restant en secondes en mode "procédure de sortie" ou "procédure de rentrée"
	Alarme_Reserve,				// NOHN UTILISE ACTUELLEMENT
	Alarme_TestRAZPresence,		// Mode réglage -> remis à 0 par BP après prise en compte demande
								//		0x00 : ne fait rien
								//		0x01 : remet le bit du détecteur de présence 1 du registre Alarme_Detection à "pas de détection"
								//		0x02 : remet le bit du détecteur de présence 2 du registre Alarme_Detection à "pas de détection"
	Alarme_TestSirenes,			// Mode réglage
								//		0x00 : Aucune sirène activée
								//		0x01 : Sirène d'intérieur activée en alarme
								//		0x02 : Sirène d'extérieur activée en alarme

	AlarmeConfig,				// -> voir enumAlarmeConfig
	
// ----------------			alerte et sécurité					----------------
	Alerte_Intensite = AlarmeConfig + AlarmeConfig_NB_VALEURS,	// reglage intensite de la sirène sur alerte : 25= fort / 50= moyen / 75= faible
	Alerte_Duree,					// reglage durée de l'alerte : 0= jusqu'à disparition / 1 à 255= temps en secondes
	Alerte_TestSirene,				// 1 = sirène à activer Intensité alerte (activé tant que cette info est non nulle)
	Alerte_Acquit,					// 1 = acquittement alerte xxx
	Securite_PriseCoupe,			// 1= prises de sécurité coupées, 0 = pilotées
	Securite_FuiteLinge,			// 1= détection de fuite sur le lave-linge activée
	Securite_FuiteVaisselle,		// 1= détection de fuite sur le lave-vaisselle activée
	Securite_FuiteAlerte,			// 1= alerte sonore activée en cas de fuite d'eau
	
	// ----------------				réveil					----------------
	// fonction réveil : provoque l'ouverture des volants roulants
	// grande chambre
	Reveil_ChambreGr_H,		// heure
	Reveil_ChambreGr_Mn,	// minutes
	Reveil_ChambreGr_ON,	// 0 = fonction désactivée / > 0 = fonction active
	// chambre 1
	Reveil_Chambre1_H,		// heure 
	Reveil_Chambre1_Mn,		// minutes
	Reveil_Chambre1_ON,		// 0 = fonction désactivée / > 0 = fonction active
	// chambre 2
	Reveil_Chambre2_H,		// heure 
	Reveil_Chambre2_Mn,		// minutes 
	Reveil_Chambre2_ON,		// 0 = fonction désactivée / > 0 = fonction active
	// chambre 3
	Reveil_Chambre3_H,		// heure 
	Reveil_Chambre3_Mn,		// minutes 
	Reveil_Chambre3_ON,		// 0 = fonction désactivée / > 0 = fonction active
	// bureau
	Reveil_Bureau_H,		// heure 
	Reveil_Bureau_Mn,		// minutes
	Reveil_Bureau_ON,		// 0 = fonction désactivée / > 0 = fonction active
		
// ----------------				délestage					----------------
	Delestage,						// 0 = fonction désactivée / <>0 : fonction active

// ----------------		téléinformation du compteur ERDF	----------------
	TeleInf_OPTARIF,			// option tarifaire du compteur	
	TeleInf_PTEC,				// période tarifaire en cours	
	TeleInf_ADPS,				// avertissement dépassement puissance souscrite	
	TeleInf_PAPP_LSB,			// puissance apparente utilisée	(sur 2 octets)
	TeleInf_PAPP_MSB,
	// puissances consommées heures de base ou heures pleines //xxx a gerer
	TeleInf_HPB_Global_LSB,			// puissance globale
	TeleInf_HPB_Global_MSB,	
	TeleInf_HPB_Chauffage_LSB,		// chauffage
	TeleInf_HPB_Chauffage_MSB,
	TeleInf_HPB_Refroid_LSB,		// refroidissement
	TeleInf_HPB_Refroid_MSB,
	TeleInf_HPB_EauChaude_LSB,		// Eau chaude
	TeleInf_HPB_EauChaude_MSB,
	TeleInf_HPB_Prises_LSB,			// prises
	TeleInf_HPB_Prises_MSB,	
	TeleInf_HPB_Autres_LSB,			// autres
	TeleInf_HPB_Autres_MSB,	
	// puissances consommées heures creuses //xxx a gerer
	TeleInf_HC_Global_LSB,			// puissance globale
	TeleInf_HC_Global_MSB,	
	TeleInf_HC_Chauffage_LSB,		// chauffage
	TeleInf_HC_Chauffage_MSB,
	TeleInf_HC_Refroid_LSB,			// refroidissement
	TeleInf_HC_Refroid_MSB,	
	TeleInf_HC_EauChaude_LSB,		// Eau chaude
	TeleInf_HC_EauChaude_MSB,
	TeleInf_HC_Prises_LSB,			// prises
	TeleInf_HC_Prises_MSB,	
	TeleInf_HC_Autres_LSB,			// autres
	TeleInf_HC_Autres_MSB,	
	// puissances consommées : répartition en % (paramètrage)
	TeleInf_Repartition_Chauffage,	// chauffage
	TeleInf_Repartition_Refroid,	// refroidissement
	TeleInf_Repartition_EauChaude,	// Eau chaude
	TeleInf_Repartition_Prises,		// prises
	TeleInf_Repartition_Autres,		// autres

// ----------------				éclairage 					----------------
// configuration des variateurs : 0= TOR (avec rampe) / 1= gradateur / 2= TOR (sans rampe)
// Zone Variateurs_PDV_Conf -> Volets_PDE_Temps : infos BA envoyées par tache et traitée en "un bloc" -> doit rester compact - ne pas insérer de valeurs dans ce bloc
	Variateurs_PDV_Conf,								// 8 variateurs possibles par boitier
										//    salon
										// +1 salle à manger
										// +2 bureau
										// +3 à + 7 non utilisé
	Variateurs_CHB_Conf = Variateurs_PDV_Conf+uc_NB_VARIATEURS_POSSIBLES_PAR_BA,	
										//    grande chambre
										// +1 petite chambre 1
										// +2 petite chambre 2
										// +3 petite chambre 3
										// +4 à +7 non utilisé
	Variateurs_PDE_Conf = Variateurs_CHB_Conf+uc_NB_VARIATEURS_POSSIBLES_PAR_BA,		
										//    salle de bain 1
										// +1 à +7 non utilisé
// temps d'extinction (lampes simples) : 1 à 255 minutes / 0= pas d'extinction automatique
	Lampes_PDV_Temps = Variateurs_PDE_Conf+uc_NB_VARIATEURS_POSSIBLES_PAR_BA,	// 16 sorties simples possibles par boitier
										// non utilisé
	Lampes_CHB_Temps	= Lampes_PDV_Temps+uc_NB_LAMPES_POSSIBLES_PAR_BA,
										// non utilisé
	Lampes_PDE_Temps	= Lampes_CHB_Temps+uc_NB_LAMPES_POSSIBLES_PAR_BA,
										// +0 à + 4 non utilisé
										// +5 WC 1
										// +6 WC 2
										// +7 service 
										// +8 à + 15 non utilisé
	
	// ----------------			volets / store 				----------------
// temps d'action (volets, store) : 1 à 255 secondes (0 à 4 minutes)
	Volets_PDV_Temps	= Lampes_PDE_Temps + uc_NB_LAMPES_POSSIBLES_PAR_BA,		// 8 volets ou stores possibles par boitier
										//    volet salon
										// +0 volet salon
										// +1 volet salon
										// +2 volet salon
										// +3 volet salle à manger
										// +4 volet salle à manger
										// +5 volet bureau
										// +6 à + 7 non utilisé
	Volets_CHB_Temps	= Volets_PDV_Temps + uc_NB_VOLETS_POSSIBLES_PAR_BA,
										//    volet grande chambre
										// +1 volet grande chambre
										// +2 volet petite chambre 1
										// +3 volet petite chambre 2
										// +4 volet petite chambre 3
										// +5 à +7 non utilisé
	Volets_PDE_Temps	= Volets_CHB_Temps + uc_NB_VOLETS_POSSIBLES_PAR_BA,
										//    volet cuisine
										// +1 volet cuisine
										// +2 volet salle de bain 1
										// +3 store terrasse
										// +4 à +7 non utilisé

// ----------------				scénarios					----------------
	Scenario = Volets_PDE_Temps + uc_NB_VOLETS_POSSIBLES_PAR_BA,	// numéro du scénario à lancer
																	// 0 = aucun / commande prise en compte par le BP
	// Scenario1 = réservé au serveur Internet (ex: descendre les volets roulants / remonter le store)
	// Scenario2 = "Je sors" 
	// Scenario3 = "Je pars en vacances" 
	// Scenario4 = "Je rentre"
	// Scenario5 = "Je vais me coucher" 
	// Scenario6 = "Je me lève"
	// Scenario7 = "Personnalisé 1" 
	// Scenario8 = "Personnalisé 2"
	
	Scenario_DernierLance, 							// Dernier scénario lancé - Utilisé par écran pour connaitre le dernier scénario lancé (en cas de remplacement de l'écran)

	Scenario1,										// -> voir enumScenario
	Scenario2 = Scenario1 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario3 = Scenario2 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario4 = Scenario3 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario5 = Scenario4 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario6 = Scenario5 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario7 = Scenario6 + Scenario_NB_VALEURS,	// -> voir enumScenario
	Scenario8 = Scenario7 + Scenario_NB_VALEURS,	// -> voir enumScenario
	
	// Contiennent l'état du boitier principal (pour le serveur)
	EtatBP1 = Scenario8 + Scenario_NB_VALEURS,
		// bit 0 : alarme activée
		// bit 1 : alarme déclenchée
		// Autres bits : à 0 (pour évolutions futures)
	EtatBP2,
		// A 0 (pour évolutions futures)
	
	Cle_Acces_Distance,	// 16 octets pour stocker la clef à 32 chiffres
						// Octet 0 / bits 0 à 3 = chiffre #1
						// Octet 0 / bits 4 à 7 = chiffre #2
						// ...
						// Octet 15 / bits 0 à 3 = chiffre #31
						// Octet 15 / bits 4 à 7 = chiffre #32
						// Etat initial : tout à 0x00
	
	// ----------------                  store                      ----------------

	Store_VR = Cle_Acces_Distance + Cle_Acces_Distance_TAILLE, // Ce registre ne sert que pour l'IHM.
	                                            // 0x00 = Store utilisé comme store.
	                                            // 0x01 = Store utilisé comme 15ème VR.
	                                            // ETAT INITIAL = 0x00.
	Store_Vitesse_Vent_Repliage,
	                                            // 0d = Pas de repliage automatique. De 1d à 255d = Vitesse du vent en km/h qui fait replier le store automatiquement.
	                                            // ETAT INITIAL = 0x00.
	Store_Vitesse_Vent_Instantane,
	                                            // Vitesse du vent en km/h de 0d à 255d.               
	                                            // ETAT INITIAL = 0x00.

	// ----------------               Constructeur                  ----------------

	Constructeur_CodeLSB,
	                                            // Code constructeur Bit 0-3 : 1er chiffre (le + à gauche) - Bit 4-7 : 2eme chiffre                  
	                                            // ETAT INITIAL = 0x11
	Constructeur_CodeMSB,
	                                            // Code constructeur Bit 0-3 : 3eme chiffre - Bit 4-7 : 4eme chiffre (le + à droite)
	                                            // ETAT INITIAL = 0x19

	// ----------------                   Test                      ----------------

	Test_ETOR_1,// Read only
				// Etat au fil de l'eau des ETOR de la carte BP.
				// Bit 0 = détecteur d'ouverture
				// Bit 1 = détecteur de présence 1 signal
				// Bit 2 = détecteur de présence 1 fraude
				// Bit 3 = détecteur de présence 2 signal
				// Bit 4 = détecteur de présence 2 fraude
				// Bit 5 = sirène d'intérieur fraude
				// Bit 6 = sirène d'extérieur fraude
	Test_ETOR_2,// Read only
				// Etat au fil de l'eau des ETOR de la carte BP.
				// Bit 0 = détecteur fuite lave linge (à voir si possible)
				// Bit 1 = détecteur fuite lave vaisselle (à voir si possible)
				// Bit 2 = détecteur de pluie
				// Bit 3 = détecteur de vent
				// Bit 4 = bouton magique
				// Bit 5 = détecteur d'ouverture tableau domotique

	EtatEthernet,//Read only
				// Bit 0 = Etat câble (0=OK / 1=HS)
				// Bit 1 = Etat DHCP (0=OK / 1=HS)
				// Bit 2 = Etat DNS (0=OK / 1=HS)
				// Bit 3 = Etat serveur (0=OK / 1=HS)
	 
	Mode_Test,
				// Empêche la sauvegarde des paramètres en cas de coupure d’alim secteur.
				// 0 = Mode de fonctionnement normal.
				// 1 = Mode de fonctionnement de test.
	
	AdresseMAC_1,	// Adresse MAC du BP (6 octets)
	AdresseMAC_2,
	AdresseMAC_3,
	AdresseMAC_4,
	AdresseMAC_5,
	AdresseMAC_6,
	 
	Nb_Tbb_Donnees
};

// CONSTANTES TELEINFO
enum option_tarifaire {		// groupe OPTARIF -> TeleInf_OPTARIF
	uc_OPT_TARIF_NON_RENSEIGNE,
	uc_OPT_TARIF_TIME_OUT,
	uc_OPT_TARIF_BASE,
	uc_OPT_TARIF_HC,		// heures creuses / heures pleines
	uc_OPT_TARIF_EJP,		// EJP
	uc_OPT_TARIF_BBR		// Tempo
};

enum periode_tarifaire {	// groupe PTEC -> TeleInf_PTEC
	uc_TARIF_NON_RENSEIGNE,
	uc_TARIF_TIME_OUT,
	uc_TARIF_TH,		// toutes heures
	uc_TARIF_HC,		// heures creuses 
	uc_TARIF_HP,		// heures pleines
	uc_TARIF_HN,		// heures normales
	uc_TARIF_PM,		// heures de pointe
	uc_TARIF_HCJB,		// heures creuses jours bleus
	uc_TARIF_HCJW,		// heures creuses jours blancs
	uc_TARIF_HCJR,		// heures creuses jours rouges
	uc_TARIF_HPJB,		// heures pleines jours bleus
	uc_TARIF_HPJW,		// heures pleines jours blancs
	uc_TARIF_HPJR		// heures pleines jours rouges
};

// groupe PAPP -> TeleInf_PAPP_LSB / TeleInf_PAPP_MSB
#define uc_PUISSANCE_APPARENTE_TIME_OUT	0xFF

// groupe HCHC / HCHP -> TeleInf_HPB_Global_LSB / TeleInf_HPB_Global_MSB / TeleInf_HC_Global_LSB / TeleInf_HC_Global_MSB
#define uc_HCHC_HCHP_TIME_OUT_BYTE	0xFF
#define uc_HCHC_HCHP_TIME_OUT_SHORT	0xFFFF
