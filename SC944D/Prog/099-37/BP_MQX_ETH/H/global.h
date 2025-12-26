
//---------------------------------- VARIABLES APPLICATIVES GENERALES

	// Identifiants SOFT : Version save + version serveur
	#define uc_VERSION_BP_SOFT_SAVE		99			// N° de sauvegarde du soft (fichier ZIP)
	#define us_BP_VERSION_SERVEUR		37			// Version serveur a laquelle le soft va être publiée

	#define uc_VERSION_TABLE_ECHANGE	30			// Version table d'échange

	extern unsigned char uc_VersionHard;	// Détecté au démarrage : 0 première version (FP1 : avec un timer) - 1 : seconde version (2016 FP2 : avec deux timers)

	extern unsigned char uc_PortSPIouvert;	// A 1 si port SPI ouvert
	extern MQX_FILE_PTR fd_SPI_EEPROM;
	
	#define uc_BP_OK			0x00
	#define uc_BP_PB_ACCES		0x01
	#define uc_BP_PB_VALEUR		0x02

	extern unsigned char uc_CadencementTraitement_50ms;
	
	extern unsigned char uc_CompteurLED_50ms;				// Cadencement led état
	#define uc_LED_CADENCEMENT_CYCLE_50ms		20			// Cadencement flash led état : 20 x 50 -> 1 seconde
	#define uc_LED_DUREE_ALLUMEE_50_ms			1			// Durée led allumée : 2 x 50 -> 100 ms

	extern DATE_STRUCT st_DateHeure;		// Contient la date / heure pour l'applicatif - Rafraichie et utilisée dans tache principale
	extern unsigned char uc_JourSemaine;	// jour de la semaine - 0 : Lundi, 1 : Mardi, ..., 6 : Dimanche
	extern unsigned char uc_JourSemainePrecedent;
	
	extern unsigned char uc_ControlerDateRetourVacances;		// A 1 après réception de la nouvelle date de retour vacances -> date à contrôler par MQX

	extern DATE_STRUCT st_DateHeurePourEcran;			// Contient la date / heure pour la tache ECRAN
	extern MUTEX_STRUCT st_Mutex_DateHeurePourEcran;	// Protection acces a st_DateHeurePourEcran durant lecture / écriture
	
	extern DATE_STRUCT st_DateHeureEnvoyeeParEcran;	// Date / heure reçue de l'écran -> MAJ RTC BP
	extern	LWSEM_STRUCT lwsem_wr_date;		// Indique qu'une nouvelle date / heure a été envoyée par l'écran -> PRENDRE EN COMPTE	
	extern unsigned char uc_FlagMajDate;	// Réception d'une nouvelle date / heure -> déclenche lwsem_wr_date à la fin du traitement de la trame
	
	extern unsigned char uc_MinutePrecedente;	// Traitement effectué toutes les minutes
	
	extern unsigned char uc_SecondePrecedente;	// Traitement effectué toutes les secondes
	
	// Gestion batterie
	#define uc_VBAT_MOYENNE_GLISSANTE	10		// Traitement effectué chaque seconde -> 10 valeurs -> moyenne sur 10 secondes 
	extern unsigned short us_VBat[uc_VBAT_MOYENNE_GLISSANTE];
	#define us_VBAT_MIN_pts				1500	// Vbat min = batterie déchargée -> couper BP - 1500 points -> 10.5 Volts
	#define us_VBAT_MIN_DISPLAY_pts		1600	// EN dessous, affichage variation 10 points
	#define uc_TEMPO_AVANT_COUPURE_sec	10		// Tempo avant coupure sur détection seuil min
	
	// Gestion mode secouru
	#define uc_TEMPO_AVANT_MODE_SECOURU_sec			10		// Tempo avant prise en compte mode secouru
	#define uc_TEMPO_AVANT_SUPP_MODE_SECOURU_sec	5		// Tempo avant suppression du mode secouru
	
	// Détection présence batterie
	// Ancienne version : détection oscillation VBAT
	/*#define us_OSCILLATION_SEUIL_BAS						1700	// En nb de points d'acquisition
	#define us_OSCILLATION_SEUIL_HAUT						2000	// En nb de points d'acquisition
	#define uc_DUREE_DETECTION_OSCILLATION_sec				5		// 5 secondes
	extern unsigned short us_AinVBatMin;
	extern unsigned short us_AinVBatMax;
	extern unsigned char uc_OscillationCompteurSeuilBas_sec;
	extern unsigned char uc_OscillationCompteurSeuilHaut_sec;*/
	
	#define uc_VBAT_NB_ECHANTILLONS			7		// Pour détection présence batterie ou non
	extern unsigned short us_VBatDernieresValeurs[uc_VBAT_NB_ECHANTILLONS];
	extern unsigned char uc_BatteriePresente;
	extern unsigned char uc_BatteriePresentePrecedent;
	
	// Contient l'état de BP
	// Ici sont résumés certains états positionnés par BP
	// Et utilisés ensuite pour informer l'écran ou les BA, ou conditionner le traitement BP
	// Controle acces multithread (assembleur) :
	//		PB si champ de bit -> supprimé !!! -> ok (lecture + écriture + test)
	extern struct 
	{
		// Gérés par la fonction Alarme, reflète l'état de l'alarme dans le BP
		// Envoyés dans trame status (écran)
		// Utilisés pour piloter sirènes - Utilisés pour forcer BA si alarme déclenchée
		unsigned char uc_AlarmeActivee;					// alarme activée (1) ou désactivée (0)
		unsigned char uc_AlarmeDeclenchee;				// Alarme déclenchée (1) ou non déclenchée (0)
		unsigned char uc_AlarmeProcedureSortieEnCours;
		unsigned char uc_AlarmeProcedureRentreeEnCours;
		
		// Défaut écran
		// Positionné par tache écran, utilisé par alarme pour détection fraude
		unsigned char uc_EcranTimeOUT;
		
		// DIN écran ouvert
		// Positionné par tache écran (état envoyé par écran) - utilisé par alarme pour détection fraude
		unsigned char uc_DinEcranOuvert;

		// Positionné par l'alarme selon la config et l'état - envoyés par la tache BA aux BA
		unsigned char uc_BloquerVolets;		// 1 = Volets roulants bloqués en cas d'alarme
		unsigned char uc_ForcerAllumage;	// 1 = allumage forcé en cas d'alarme

		// Positionné par téléinfo (lecture trame LINKY)
		// Utilisé par fonction chauffage (délestage)
		unsigned char uc_SurConsommation;		// A 1 si surconsommation détectée -> délestage

		// Positionné par téléinfo (lecture trame LINKY)
		// Utilisé pour piloter le cumulus (si mode auto)
		unsigned char uc_HeuresCreuses;
		
		// Utilisé par fonction chauffage (génération fil pilote) / pilotage sorties pour bloquer les sorties si alimentation secourue
		// Envoyé dans trame status (écran)
		// Envoyé aux BA -> save état
		unsigned char uc_Secouru;				// indique si BP sur batterie -> ne plus piloter les sorties
		
		// Envoyés dans trame status (écran) - positionné par fonctions hard détection fuite
		// Utilisés pour pilotage sirène (gestion alerte) et trame status
		unsigned char uc_FuiteLL;				// détection fuite d'eau lave linge
		unsigned char uc_FuiteLV;				// détection fuite d'eau lave vaisselle
		
		// Positionné par téléinfo (lecture trame LINKY)
		// Utilisé par fonction Teleinfo (pilotage led)
		unsigned char uc_DefTeleinfo;			// défaut com compteur ERDF

		// Positionné par tache BA en cas de pb de comm avec les BA ou si tache BA pas démarrée
		// Utilisé pour renseigner Information
		unsigned char uc_Defboitier1;		// défaut com boitiers
		unsigned char uc_Defboitier2;		// défaut com boitiers
		unsigned char uc_Defboitier3;		// défaut com boitiers
		
		// Positionné par l'exécution d'un scénario - Utilise par vd_PiloterSorties()
		// Par défaut à 0 -> coupe les sorties concernées si à 1
		unsigned char uc_SécuOFF;			// coupure des prises de sécurité
		unsigned char uc_MachinesOFF;		// coupure des machines à laver
		
		unsigned char uc_EtatEthernet;		// Etat ethernet -> pour copie dans table echange EtatEthernet
											// Bit 0 = Etat câble (0=OK / 1=HS)
											// Bit 1 = Etat DHCP (0=OK / 1=HS)
											// Bit 2 = Etat DNS (0=OK / 1=HS)
											// Bit 3 = Etat serveur (0=OK / 1=HS)
		
	}st_EchangeStatus, st_EchangeStatusPrecedent;
	
	// Pilotage alarme depuis serveur
	extern unsigned char uc_DemandeServeurActiverAlarme;	// Demande du serveur de mettre l'alarme
	extern unsigned char uc_DemandeServeurCouperAlarme;		// Demande du serveur de couper l'alarme
	
	// Etat entrée DIN_VITESSE_VENT
	extern unsigned char uc_Etat_DIN_VITESSE_VENT;
	
	// Valeurs VENT précédentes pour filtrage
	extern unsigned char uc_VitesseVent[3];	// 3 dernières valeurs
	
	// Etat Ethernet
	extern unsigned char uc_EtatEthernetCablePB;
	extern unsigned char uc_EtatEthernetDHCPPB;
	extern unsigned char uc_EtatEthernetDNSPB;
	extern unsigned char uc_EtatEthernetServeurPB;
	
	// A 1 si reset BP programmé (après DOWNLOAD) -> save état BA
	extern unsigned char uc_FlagResetBP;
	
	// ADC channels
	//extern MQX_FILE_PTR f_adc;
	//extern MQX_FILE_PTR f_adc_vbat;		// Entrée analogique vbat
	//extern MQX_FILE_PTR f_adc_Fuite1;	// Entrée analogique détecteur fuite 1
	//extern MQX_FILE_PTR f_adc_Fuite2;	// Entrée analogique détecteur fuite 2
	
	struct st_AIN
	{
		unsigned char uc_NumeroAIN;
		unsigned short us_AINBrut;
		unsigned short us_AINMin;
		unsigned short us_AINMax;
	};
	extern struct st_AIN st_AIN4_VBat;
	extern struct st_AIN st_AIN5_FuiteLV;
	extern struct st_AIN st_AIN6_FuiteLL;

	
	// Gestion PWM
	extern FILE_PTR fdpwm;
	extern PWM_DEV_ENABLE_PIN_DATA pwm_pin_data;

	enum mode_sirene {	// ratio PWM
		uc_BUZ_TFORT =0,		// PLEINE PUISSANCE - alarme déclenchée (ou fil coupé)
		uc_BUZ_FORT =25,
		uc_BUZ_MOYEN =50,
		uc_BUZ_FAIBLE = 75,
		uc_BUZ_STOP = 100		// arrêté
	};
	
	// Gestion Temporisation sirènes en cas d'alarme
	extern unsigned short us_CompteurPilotageSireneInterieure_sec;
	#define us_DUREE_PILOTAGE_SIRENE_INTERIEURE_sec						300		// 5 minutes
	
	extern unsigned char uc_SireneExterieurePilotee;
	extern unsigned char uc_CompteurNbPilotageSireneExterieure;
	#define uc_NB_MAX_PILOTAGE_SIRENE_EXTERIEURE						3
	
	extern unsigned short us_CompteurPilotageSireneExterieure_sec;
	#define us_DUREE_PILOTAGE_SIRENE_EXTERIEURE_sec						180		// 3 minutes
	
	// Réinit temporisation sirènes si détecteur ou fraude redéclenchent
	extern unsigned char uc_ReinitCompteursSirenes;			// Reinit en meme temps pour les deus sirenes lorsque les deux sont arretees !
	
	// Permet de détection qu'un détecteur ou fraude a redéclenché (front montant)
	extern unsigned char uc_EtatDetecteurs;				// Contient les détecteurs déclenchés (contrairement à uc_AlarmeDetection remis à 0 à chaque début de calcul !)
	extern unsigned char uc_EtatDetecteursPrecedent;
	extern unsigned char uc_EtatFraudes;				// Contient les fraudes déclenchées (contrairement à uc_AlarmeFraude remis à 0 à chaque début de calcul !)
	extern unsigned char uc_EtatFraudesPrecedent;

	// I/O
	extern LWGPIO_STRUCT IO_DIN_OuvertureSireneInterieure;	// UC_OUVERTURE_SIRENE_INT - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DIN_OuvertureSireneExterieure;	// UC_OUVERTURE_SIRENE_EXT - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DIN_OuvertureTableauDominique;	// UC_OUVERTURE_TABLEAU_DOMOTIQUE - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DOUT_Sirene_Exterieure;			// UC_COMMANDE_SIRENE_EXT - Alarme()
	extern LWGPIO_STRUCT IO_DOUT_15VSP_AlimBA;				// UC_+15VSP - Toujours piloté sauf en cas de coupure de l'alimentation générale
	extern LWGPIO_STRUCT IO_DOUT_LEDEtatBP;					// UC_LED - Activité logiciel
	extern LWGPIO_STRUCT IO_DOUT_ControleBatterie;			// UC_BATT_CTRL - A 0 -> coupe alimentation provenant de la batterie
	extern LWGPIO_STRUCT IO_DIN_Detection_Ouverture;		// UC_SIGNAL_DETECT_OUVERTURE_PORTE - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DIN_Detection_presence1;		// UC_SIGNAL_DETECT_PRESENCE_1 - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DIN_Detection_presence2;		// UC_SIGNAL_DETECT_PRESENCE_2 - Alarme() A 1 si > 400 ms (précision 50 ms)
	extern LWGPIO_STRUCT IO_DIN_Detection_Pluie;			// UC_DETECT_PLUIE - Conditionne pilotage EV arrosage
	extern LWGPIO_STRUCT IO_DOUT_TeleInfo_Led;				// UC_TELEINFO_LED - Etat teleinfo
	extern LWGPIO_STRUCT IO_DIN_Etat_AlimPrincipale;		// UC_24V_MON - Alimentation générale présente
	extern LWGPIO_STRUCT IO_DOUT_EcranDirection;			// UC_IHM_POL_TX - Inversion sens branchement écran
	extern LWGPIO_STRUCT IO_DIN_OuvertureDecteurPresence1;	// UC_OUVERTURE_DETECT_PRESENCE_1 - Pas utilisé
	extern LWGPIO_STRUCT IO_DIN_OuvertureDecteurPresence2;	// UC_OUVERTURE_DETECT_PRESENCE_2 - Pas utilisé
	extern LWGPIO_STRUCT IO_DIN_VITESSE_VENT;				// UC_ETOR_RESERVE_1 - Vitesse vent (anémometre)
	extern LWGPIO_STRUCT IO_DIN_BOUTON_MAGIQUE;				// UC_ETOR_RESERVE_2 - Bouton magique
	extern LWGPIO_STRUCT IO_DOUT_DebugJ1;					// Debug JT1
	extern LWGPIO_STRUCT IO_DOUT_DebugJ2;					// Debug JT2
	extern LWGPIO_STRUCT IO_DOUT_DebugJ3;					// Debug JT3
	extern LWGPIO_STRUCT IO_DOUT_DebugJ4;					// Debug JT4
	extern LWGPIO_STRUCT IO_DOUT_DebugJ5;					// Debug JT5
	extern LWGPIO_STRUCT IO_DIN_ErreurEcranHard;			// UC_IHM_ERR - Erreur détectée au niveau Hard (alim écran)

	// CS EEPROM : géré directement par SPI mais init à faire manuellement
	extern LWGPIO_STRUCT IO_DOUT_SPI_CS_EepromAdresseMac;	// CS EEPROM ADRESSE MAC
	extern LWGPIO_STRUCT IO_DOUT_SPI_CS_EepromSoft;			// CS EEPROM SOFT
	
	// Sorties suivantes : forcées à 0 si coupure alim générale
	extern LWGPIO_STRUCT IO_DOUT_VanneArrosage;				// UC_EV - Piloté selon planning arrosage
	extern LWGPIO_STRUCT IO_DOUT_PriseSecurite;				// UC_PRISES_SECURITE - Piloté selon config + scénario
	extern LWGPIO_STRUCT IO_DOUT_MachineALaver;				// UC_MACHINES_LAVER - Piloté selon config ET pas de fuite
	extern LWGPIO_STRUCT IO_DOUT_Cumulus;					// UC_CUMULUS - Piloté en forcé ou selon HC / HP
	
	// Gestion FIL PILOTE -> géré en direct sans passer par MQX sauf l'ouverture
	extern LWGPIO_STRUCT IO_DIN_Secteur_Synchro;			// UC_SECTEUR_SYNC - Etat signal entrée pour génération fil pilote (sous IT)
	extern LWGPIO_STRUCT IO_DOUT_FilPilote_ZJ;				// UC_FP_ZJ - Fil pilote (chauffage)
	extern LWGPIO_STRUCT IO_DOUT_FilPilote_ZN;				// UC_FP_ZN - Fil pilote (chauffage)
	extern LWGPIO_STRUCT IO_DOUT_FilPilote_SDB1;			// UC_FP_SDB1 - Fil pilote (chauffage)
	extern LWGPIO_STRUCT IO_DOUT_FilPilote_SDB2;			// UC_FP_SDB2 - Fil pilote (chauffage)
	
	// UC_COMMANDE_SIRENE_INT : géré par PWM
	// UC_VBATT : état VBat -> AIN
	// UC_DETECT_FUITE_1 : état détecteur fuite 1 -> AIN
	// UC_DETECT_FUITE_2 : état détecteur fuite 2 -> AIN
	
	// Nouvelles entrées Carte V2 2016
	extern LWGPIO_STRUCT IO_DIN_D5;
	extern LWGPIO_STRUCT IO_DIN_D6;
	extern LWGPIO_STRUCT IO_DIN_D7;

	extern unsigned char uc_Cle_Acces_Distance[Cle_Acces_Distance_TAILLE];	// Contient l'état en EEPROM
	
	extern unsigned char uc_CouperEVArrosage;
	
//----------------------------------

//---------------------------------- GESTION ECRAN
	
	extern unsigned char uc_EcranTimeOUT;		// Time out sur non réception écran
	#define us_ECRAN_TIME_OUT_ms	1500		// Emission écran toutes les secondes
	
	extern unsigned char uc_TrameSynchroEtat;

//----------------------------------
	
//---------------------------------- GESTION TELEINFO

	#define us_TIMER_TELEINFO_ms						100			// Cadencement timer teleinfo
	
	extern unsigned short us_TeleinfoCadencementLed;				// Incrémenté sous timer TELEINFO
	#define us_CANDENCEMENT_LED_TELEINFO				5			// 5 * 100 ms -> 500 ms

	extern unsigned short us_TeleinfoTimeOUT;						// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_TIME_OUT						20			// 20 * 100 ms -> 2 secondes
	
	extern unsigned char uc_TeleinfoOptionTarifaire;
	extern unsigned short us_TeleinfoTimeOUTOptionTarifaire;		// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_OPTION_TARIFAIRE_TIME_OUT		20			// 20 * 100 ms -> 2 secondes
	
	extern unsigned char uc_TeleinfoPeriodeTarifaire;
	extern unsigned short us_TeleinfoTimeOUTPeriodeTarifaire;		// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_PERIODE_TARIFAIRE_TIME_OUT		20			// 20 * 100 ms -> 2 secondes

	extern unsigned long ul_TeleinfoPuissanceApp;
	extern unsigned short us_TeleinfoTimeOUTPuissanceApp;			// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_PUISSANCE_APP_TIME_OUT		20				// 20 * 100 ms -> 2 secondes

	extern unsigned long ul_TeleinfoHCHC;
	extern unsigned long ul_TeleinfoHCHCPrecedent;
	extern unsigned short us_TeleinfoTimeOUTHCHC;				// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_HCHC_APP_TIME_OUT			20			// 20 * 100 ms -> 2 secondes

	extern unsigned long ul_TeleinfoHCHP_OU_BASE;
	extern unsigned long ul_TeleinfoHCHP_OU_BASEPrecedent;
	extern unsigned short us_TeleinfoTimeOUTHCHP_OU_BASE;		// Incrémenté sous timer TELEINFO
	#define us_TELEINFO_HCHP_OU_BASE_APP_TIME_OUT	20			// 20 * 100 ms -> 2 secondes
	
	extern unsigned short us_TeleInf_HPB_Global_sans_div_10;	
	extern unsigned short us_TeleInf_HC_Global_sans_div_10;
	extern unsigned short us_TeleInf_HPB_Global_avec_div_10;	
	extern unsigned short us_TeleInf_HC_Global_avec_div_10;

	extern unsigned char uc_UpdateHCHC_HCHP;

//----------------------------------
	
//---------------------------------- Chauffage
	enum type_chauffage {	// ! ne pas modifier l'ordre de déclaration ! xxx HORS GEL doit être après les modes CONFORT et ECO !
		uc_CHAUFFAGE_OFF,				// mode par défaut
		uc_CHAUFFAGE_CONFORT,
		uc_CHAUFFAGE_ECO,
		uc_CHAUFFAGE_ECO_PLUS,
		uc_CHAUFFAGE_ECO_PLUS_PLUS,
		uc_CHAUFFAGE_HORS_GEL,
		uc_CHAUFFAGE_NB_MODES
	};

	// mode de chauffage
	// b4-b5 : mode 	: 0= automatique / 1= forcé / 2= anticipé
	enum mode_chauffage {
		uc_AUTO,		// mode par défaut
		uc_FORCE,
		uc_ANTICIPE,
		uc_MAX
	};

	// Acces aux octets de la table d'échange Chauf_zj_Mode, ...
	extern MUTEX_STRUCT st_Mutex_ZJ;		// Objectif : pendant un calcul de chauffage, empecher une modification de ces octets par l'écran par exemple
	extern MUTEX_STRUCT st_Mutex_ZN;		// sinon risque d'écraser une nouvelle consigne...
	extern MUTEX_STRUCT st_Mutex_Zsdb1;
	extern MUTEX_STRUCT st_Mutex_Zsdb2;
	
	// Flag forcant le calcul du chauffage si modif mode par ecran ou par scenario ou si changement heure
	extern unsigned char uc_ModeChauffageModifieParEcranOuScenario;
	
	// Etat chauffage pour affichage (trame status)
	extern unsigned char uc_EtatChauffagePourEcran_ZJ;
	extern unsigned char uc_EtatChauffagePourEcran_ZN;
	extern unsigned char uc_EtatChauffagePourEcran_Zsdb1;
	extern unsigned char uc_EtatChauffagePourEcran_Zsdb2;

	typedef struct  
	{
		unsigned char uc_Mode;									// mode en cours (auto, force, ...)
		unsigned char uc_ConsigneFilPiloteEnCours;				// consigne fil pilote calculé (6 possibles) *
		unsigned char uc_DelestageActif;						// Delestage actif pour cette zone si != 0xFF -> contient la consigne à appliquer
																// b0-b3 : consigne : 0 = OFF / 1 = CONFORT / 2 = ECO / 3 = ECO+ / 4 = ECO++ / 5 = HORS GEL
		unsigned char uc_ConsigneFilPiloteNonAnticipe;			// type de chauffage qui aurait du être sans anticipation - permet de sortir du mode anticipation des que la consigne planning change
		unsigned char uc_HeurePassageAnticipe;					// Heure de passage en mode anticité -> permet de sortir de anticipé si pas de mode différent trouvé dans planning
	}struct_chauffage, * pstruct_chauffage;
	extern struct_chauffage	st_chauf_ZJ, st_chauf_ZN, st_chauf_Zsdb1, st_chauf_Zsdb2;
	
	// Contient le niveau de délestage en cours :
	// 0 : AUCUN
	// 1 : ZJ en HG
	// 2 : ZN en HG
	// 3 : SDB1 en OFF
	// 4 : SDB2 en OFF
	// 5 : CUMULUS OFF
	extern unsigned char uc_DelestageNiveauEnCours;
	extern unsigned char uc_DelestageNiveauEnCoursPrecedent;
	
	extern unsigned char uc_DelestageCouperCumulus;				// A 1 si délestage -> couper le cumulus

	// * La 1ere variable est utilisée pendant le calcul et peut changer avant d'avoir l'état définitif
	//   A la fin du calcul elle prend la valeur de délestage si ce dernier est activé pour la zone en cours
	//	 Cette valeur ne doit donc pas etre utilisée directement par le pilotage du fil pilote qui se fait sous IT
	//   A la fin du calcul uc_ConsigneFilPiloteEnCours -> uc_ConsigneFilPilotePourIT_xx
	//   uc_ConsigneFilPilotePourIT_xx est utilisée pour générer le fil pilote sous IT
	
	// Generation fil pilote
	// !!! Suite gestion FIL PILOTE sous IT en dehors MQX -> champ sortie de la structure (pour diminuer la taille de code) et mis dans variables suivantes
	extern unsigned char uc_ConsigneFilPilotePourIT_ZJ;
	extern unsigned char uc_ConsigneFilPilotePourIT_ZN;
	extern unsigned char uc_ConsigneFilPilotePourIT_SDB1;
	extern unsigned char uc_ConsigneFilPilotePourIT_SDB2;

	// Generation fil pilote - Etat précédent permettant d'effectuer le traitement adéquat en cas de changement de la consigne
	extern unsigned char uc_ConsigneFilPilotePourITPrecedent_ZJ;
	extern unsigned char uc_ConsigneFilPilotePourITPrecedent_ZN;
	extern unsigned char uc_ConsigneFilPilotePourITPrecedent_SDB1;
	extern unsigned char uc_ConsigneFilPilotePourITPrecedent_SDB2;

	// Gestion des fils pilotes : temporisation du mode ECO
	extern unsigned char uc_EcoEnCours_ZJ;
	extern unsigned char uc_EcoEnCours_ZN;
	extern unsigned char uc_EcoEnCours_SDB1;
	extern unsigned char uc_EcoEnCours_SDB2;
	
	// Gestion des fils pilotes : temporisation du mode ECO +
	extern unsigned char uc_EcoPlusEnCours_ZJ;
	extern unsigned char uc_EcoPlusEnCours_ZN;
	extern unsigned char uc_EcoPlusEnCours_SDB1;
	extern unsigned char uc_EcoPlusEnCours_SDB2;
	
	// Compteurs permettant de générer les consignes ECO et ECO+
	extern unsigned char uc_CompteurSecondesEcoEtEcoPlus_ZJ;
	extern unsigned char uc_CompteurSecondesEcoEtEcoPlus_ZN;
	extern unsigned char uc_CompteurSecondesEcoEtEcoPlus_SDB1;
	extern unsigned char uc_CompteurSecondesEcoEtEcoPlus_SDB2;
	
	// Flag permettant de forcer la réinitialisation des compteurs ci-dessus
	extern unsigned char uc_RAZCompteurSecondesEcoEtEcoPlus_ZJ;
	extern unsigned char uc_RAZCompteurSecondesEcoEtEcoPlus_ZN;
	extern unsigned char uc_RAZCompteurSecondesEcoEtEcoPlus_SDB1;
	extern unsigned char uc_RAZCompteurSecondesEcoEtEcoPlus_SDB2;
	
	// Compteurs permettant de forcer le mode HG un bref instant lors du passage ECO/ECO+ vers CONFORT
	extern unsigned char uc_CompteurITModeHGForce_ZJ;
	extern unsigned char uc_CompteurITModeHGForce_ZN;
	extern unsigned char uc_CompteurITModeHGForce_SDB1;
	extern unsigned char uc_CompteurITModeHGForce_SDB2;
	
	#define uc_NB_IT_MODE_HG_FORCE			 50		// 50 IT * 10 ms -> 500 ms
	
	
	// Gestion scénario : MODE "Continuer le fonctionnement actuel"
	extern unsigned char uc_Chauf_zj_Mode_EtatLorsDernierScenario;	// Contient l'état du mode de chauffage qui état juste avant l'exécution du dernier scénario
	extern unsigned char uc_Chauf_zn_Mode_EtatLorsDernierScenario;
	extern unsigned char uc_Chauf_zsb1_Mode_EtatLorsDernierScenario;
	extern unsigned char uc_Chauf_zsb2_Mode_EtatLorsDernierScenario;
	extern unsigned char uc_Cumulus_Mode_EtatLorsDernierScenario;
	extern unsigned char uc_Cumulus_ForcerAOnAuRetourVacancesProgramme;
	
	extern unsigned char uc_ChauffageModifieDepuisServeur_zj;	// Annule le retour de vacances programme si modification mode via serveur
	extern unsigned char uc_ChauffageModifieDepuisServeur_zn;
	extern unsigned char uc_ChauffageModifieDepuisServeur_zsb1;
	extern unsigned char uc_ChauffageModifieDepuisServeur_zsb2;
	extern unsigned char uc_CumulusModifieDepuisServeur;
	
//----------------------------------

//---------------------------------- CUMULUS
	//Cumulus_Mode,				// 0 = Autonome (ON) / 1 = gestion heures creuses / 2 = OFF
	enum mode_cumulus {
		uc_CUMULUS_ON,			// ON (autonome)
		uc_CUMULUS_HC,			// gestion heures creuses / heures pleines
		uc_CUMULUS_OFF			// arrêt
	};
//----------------------------------
	
//---------------------------------- Gestion des réveils
	typedef struct  
	{
		unsigned char uc_ChGr;		// grande chambre
		unsigned char uc_Ch1;		// chambre 1
		unsigned char uc_Ch2;		// chambre 2
		unsigned char uc_Ch3;		// chambre 3
		unsigned char uc_Bur;		// bureau
	}struct_reveil_armement;
	extern struct_reveil_armement st_Reveil_Arme;
//----------------------------------
	
//---------------------------------- BA
	enum enum_TYPES_BOITIER
	{
		uc_BOITIER_PIECES_VIE,
		uc_BOITIER_CHAMBRES,
		uc_BOITIER_PIECES_EAU,
		
		uc_NB_BOITIER_AUXILIAIRE
	};
//----------------------------------
	
//---------------------------------- VOLETS
	#define uc_TEMPS_CDE_VOLET_DEFAUT			2*60	// init à 2 minutes par défaut (si nulle)

	// boitier chambre :
	// volets grande chambre : volets 0 et 1
	#define CHAMBRE_GR_VOLETS		0x0003
	// volets chambre 1 : volet 2
	#define CHAMBRE1_VOLETS			0x0004
	// volet chambre 2 : volet 3
	#define CHAMBRE2_VOLETS			0x0008
	// volet chambre 3 : volet 4
	#define CHAMBRE3_VOLETS			0x0010
	// boitier pièces de vie :
	// volet bureau : volet 5
	#define BUREAU_VOLETS			0x0020
	
	// Store
	#define STORE					0x0008
	
//----------------------------------

//---------------------------------- GESTION DIALOGUE BA PAR I2C

	extern MQX_FILE_PTR fd_BA_I2C;	// Dialogue I2C

	#define uc_I2C_BUFFER_SIZE 32

	#define	uc_I2C_NB_REPETE_AV_ERREUR	5		// nombre de répétitions avant signalement de l'erreur
	#define	uc_I2C_NB_REPETE			20		// nombre de répétitions avant acquittement de la demande (la trame sera envoyée uniquement sur un nouveau changement d'état)

	// Erreur d'accès au boitier auxiliaire (retour fonction sl_fct_write_polled)
	// Mettre valeurs différentes des erreur I2C
	enum
	{
		sl_FCT_WRITE_POLLED_AUCUNE_ERREUR,		// Identique I2C_OK
		sl_FCT_WRITE_POLLED_ERREUR_FIN_TRAME,
		sl_FCT_WRITE_POLLED_ERREUR_EMISSION_DATA,
		sl_FCT_WRITE_POLLED_ERREUR_TAILLE_RECEPTION,
		sl_FCT_WRITE_POLLED_ERREUR_CODE_REPONSE,
		sl_FCT_WRITE_POLLED_ERREUR_CRC_REPONSE,
		sl_FCT_WRITE_POLLED_ERREUR_BLOCAGE,
		sl_FCT_WRITE_POLLED_ERREUR_REINIT,
	};

	enum enum_CODE_TRAMES
	{
		uc_TRAME_BA_FORCAGE_SORTIES = 1,
		uc_TRAME_BA_CONF_SORTIES,
		uc_TRAME_BA_TPS_EXTINCTION,
		uc_TRAME_BA_TPS_ACTION,
		uc_TRAME_BA_ACTIONS
	};

	extern LWSEM_STRUCT lock_I2C;	// Verrouillage accès multithread à la liaison I2C // xxx on peut mettre aussi un mutex ?
//----------------------------------
	
//---------------------------------- GESTION FORCAGE DOUT BA PAR APPLICATIF
	#define BA_QUEUE_ID			8		// ID de messagerie utilisée entre tache principale et tache BA
		
	enum Tbb_Donneesboitier_Index
	{
		// boitiers : commandes des sorties (forcage)
		// 16 sorties simples possibles par boitier
		// ecrit à 1 par la tache applicative
		// envoye par message a tache BA pour emission vers BA
		// RAZ par tache principale apres emission par message
		us_SSimples_BA1_CdeEteint,				// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)		
		us_SSimples_BA2_CdeEteint,			
		us_SSimples_BA3_CdeEteint,			
		us_SSimples_BA1_CdeAllume,				// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)		
		us_SSimples_BA2_CdeAllume,			
		us_SSimples_BA3_CdeAllume,			
		// 16 variateurs possibles par boitier
		us_SVariateurs_BA1_CdeEteint,			// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)	
		us_SVariateurs_BA2_CdeEteint,
		us_SVariateurs_BA3_CdeEteint,
		us_SVariateurs_BA1_CdeAllume,			// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)		
		us_SVariateurs_BA2_CdeAllume,			
		us_SVariateurs_BA3_CdeAllume,			
		// 16 volets ou stores possibles par boitier
		us_SVolets_BA1_Cdeouvre,				// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)			
		us_SVolets_BA2_Cdeouvre,		
		us_SVolets_BA3_Cdeouvre,		
		us_SVolets_BA1_CdeFerme,				// commande : 0=ne rien faire / 1=à prendre en compte  (1 bit par sortie)			
		us_SVolets_BA2_CdeFerme,		
		us_SVolets_BA3_CdeFerme,		
		
		us_Nb_Tbb_Donnees_BA
	};

	typedef struct {
		MESSAGE_HEADER_STRUCT HEADER;
		uchar DATA[us_Nb_Tbb_Donnees_BA *2];	// taille tableau x 2 car ce sont des short !!!
	} COMMANDE_MESSAGE, _PTR_ COMMANDE_MESSAGE_PTR;
	extern _pool_id message_pool;		// Messagerie utilisée entre tache principale -> tache BA : envoi ordres forcage suite CC

	// Multitache OK :
	// 		Tableau utilisé uniquement par fonctions appelées par tache principale (reveil & scenario)
	// 		Infos transmises a tache BA par message a partir de la tache principale
	extern unsigned short us_Tb_Echangeboitier[us_Nb_Tbb_Donnees_BA];	// Contient les ordres de forcage des sorties
																		// 0 : aucun forcage - positionné par applicatif
																		// -> transmis a la tache BA par message depuis tache principale

//----------------------------------
	
//---------------------------------- SCENARIO
	enum SCENARIOS  {	// Utilisees par fonction vd_Scenario_Init() et par "scénario à lancer" : Tb_Echange[Scenario]
			uc_SCENARIO_RAZ,		// Pas de scenario pour ce numéro, doit être > à cette valeur !
			uc_SCENARIO_SERVEUR,	
			uc_SCENARIO_JE_SORS,  
			uc_SCENARIO_JE_PARS_EN_VACANCES, 
			uc_SCENARIO_JE_RENTRE,  
			uc_SCENARIO_JE_ME_COUCHE,  
			uc_SCENARIO_JE_ME_LEVE,
			uc_SCENARIO_PERSO,
			uc_SCENARIO_LIBRE,
			uc_SCENARIO_NB
	};
	
	extern unsigned char uc_DernierScenarioLance;	// Contient le dernier scénario lancé
//----------------------------------

//---------------------------------- TABHE ECHANGE
	extern unsigned char Tb_Echange[Nb_Tbb_Donnees];
	extern unsigned char Tb_EchangePrecedent[Nb_Tbb_Donnees];	// Utilisé par RS debug pour loguer tout changement de valeurs
//----------------------------------

//---------------------------------- ALARME
	extern unsigned char uc_AlarmeConfigEnCours[AlarmeConfig_NB_VALEURS];	// Contient la config à utiliser au niveau de l'alarme
	extern unsigned char uc_AlarmeModePrecedent;

	extern unsigned char uc_AlarmeDetection;	// Contient l'état des détections	
	extern unsigned char uc_AlarmeFraude;		// Contient l'état des fraudes

	extern unsigned char uc_AlarmeCompteurSecondes; 
	#define us_TIMER_ALARME_CADENCEMENT_ms							1000
	#define uc_DUREE_PROCEDURE_RENTREE_SORTIE_Sec					45
		
	extern unsigned char uc_Alarme_NouveauCode_1_LSB;
	extern unsigned char uc_Alarme_NouveauCode_1_MSB;
	extern unsigned char uc_Alarme_NouveauCode_2_LSB;
	extern unsigned char uc_Alarme_NouveauCode_2_MSB;

	// Constantes Alarme_Mode
	#define uc_ALARME_MODE_PAS_UTILISEE								0x00		// Le système d'alarme n'est pas utilisé
	#define uc_ALARME_MODE_REGLAGE									0x01		// Mode réglage
	#define uc_ALARME_MODE_ALARME_INDEPENDANTE						0x02		// Alarme indépendante
	#define uc_ALARME_MODE_ALARME_SCENARIO_JE_SORS					0x03		// Alarme sur scénario "Je sors"
	#define uc_ALARME_MODE_ALARME_SCENARIO_JE_VAIS_ME_COUCHER		0x04		// Alarme sur scénario "Je vais me coucher"
	#define uc_ALARME_MODE_ALARME_SCENARIO_JE_PARS_EN_VACANCES		0x05		// Alarme sur scénario "Je pars en vacances"
	#define uc_ALARME_MODE_ALARME_SCENARIO_PERSONNALISE				0x06		// Alarme sur scénario "Personnalisé"
	
	// Constantes Alarme_SuiviAlarme
	#define uc_ALARME_SUIVI_ALARME_ETAPE_DEPART						0x00		// Etape de départ
	#define uc_ALARME_SUIVI_ALARME_PROBLEME_ALIMENTATION			0x01		// Mise sous alarme impossible -> problème d'alimentation
	#define uc_ALARME_SUIVI_ALARME_PROBLEME_DETECTION				0x02		// Mise sous alarme impossible -> intrusion ou vandalisme
	#define uc_ALARME_SUIVI_ALARME_PROCEDURE_SORTIE					0x03		// Procédure de sortie
	#define uc_ALARME_SUIVI_ALARME_REGIME_CROISIERE					0x04		// Régime de croisière
	#define uc_ALARME_SUIVI_ALARME_PROCEDURE_ENTREE					0x05		// Procédure d'entrée
	#define uc_ALARME_SUIVI_ALARME_INTRUSION_OU_VANDALISME			0x06		// Intrusion ou vandalisme
	
	// Constantes Alarme_Autorisation
	#define uc_ALARME_AUTORISATION_CODE_VALIDE						0x01
	#define uc_ALARME_AUTORISATION_CODE_INVALIDE					0x02
		
	// Constantes Alarme_Detection
	#define uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE				0x01	// bit 0 : état du détecteur d'ouverture
	#define uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1			0x02	// bit 1 : état du détecteur présence 1
	#define uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2			0x04	// bit 2 : état du détecteur présence 2

	// Constantes Alarme_Fraude
	#define uc_ALARME_FRAUDE_BIT_TABLEAU_DOMOTIQUE					0x01	// bit 0 : état de fraude tableau domotique
	#define uc_ALARME_FRAUDE_BIT_IHM								0x02	// bit 1 : état de fraude IHM
	#define uc_ALARME_FRAUDE_BIT_DETECTEUR_PRESENCE_1				0x04	// bit 2 : état de fraude détecteur présence 1
	#define uc_ALARME_FRAUDE_BIT_DETECTEUR_PRESENCE_2				0x08	// bit 3 : état de fraude détecteur présence 2
	#define uc_ALARME_FRAUDE_BIT_SIRENE_INTERIEURE					0x10	// bit 4 : état de fraude sirène d'intérieur
	#define uc_ALARME_FRAUDE_BIT_SIRENE_EXTERIEURE					0x20	// bit 5 : état de fraude sirène d'extérieur
	#define uc_ALARME_FRAUDE_BATTERIE_NON_PRESENTE					0x40	// bit 6 : état alarme - 0 : présente - 1 : non présente
	
	// Constantes Alarme_SuiviChangementCode
	#define uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART				0x00	// état de départ
	#define uc_ALARME_SUIVI_CHANGEMENT_CODE_1ER_ENVOI				0x01	// 1er envoi nouveau code
	#define uc_ALARME_SUIVI_CHANGEMENT_CODE_2EME_ENVOI				0x02	// 2ème envoi nouveau code
	
	// Codes retour uc_ControleCodeSaisie()
	#define uc_ControleCodeSaisie_AUCUN_CODE_SAISI					0x00
	#define uc_ControleCodeSaisie_CODE_SAISI_INCORRECT				0x01
	#define uc_ControleCodeSaisie_CODE_SAISI_CORRECT				0x02	
	
	// Constantes Alarme_TestRAZPresence
	#define uc_ALARME_TEST_RAZ_PRESENCE_NE_RIEN_FAIRE				0x00	// ne fait rien
	#define uc_ALARME_TEST_RAZ_PRESENCE_RAZ_DETECTEUR_PRESENCE_1	0x01	// remet le bit du détecteur de présence 1 du registre Alarme_Detection à "pas de détection"
	#define uc_ALARME_TEST_RAZ_PRESENCE_RAZ_DETECTEUR_PRESENCE_2	0x02	// remet le bit du détecteur de présence 2 du registre Alarme_Detection à "pas de détection"

	// Filtrage entrées alarme : prise en compte si à 0 pendant au moins 400 ms
	extern unsigned char uc_CompteurDetect_Ouv_50ms;			// Surveillance IO_Detect_Ouv
	extern unsigned char uc_CompteurDetect_Pres1_50ms;			// Surveillance IO_Detect_pres1
	extern unsigned char uc_CompteurDetect_Pres2_50ms;			// Surveillance IO_Detect_pres2
	extern unsigned char uc_CompteurOuvertureSireneInterieure;	// Surveillance IO_DIN_OuvertureSireneInterieure
	extern unsigned char uc_CompteurOuvertureSireneExterieure;	// Surveillance IO_DIN_ouvertureSireneExterieure
	extern unsigned char uc_CompteurOuverturePanneauDomotique;	// Surveillance IO_DIN_OuvertureTableauDominique
	#define uc_DUREE_ENTREE_ALARME_A_1_50ms		8				// Base de temps 50 ms -> 400 ms
//----------------------------------

//---------------------------------- I2C GESTION PERTURBATION

// Variables utilisees dans le BSP
//		Sortie boucle while par compteur time out (ul_I2C_While_x)
//		Mémorisation temps max boucle : ul_I2C_While_xMax (hors time out)
//		Comptage nb sorties par time out : ul_I2C_While_xTimeOut
	extern unsigned long ul_I2C_While_1;
	extern unsigned long ul_I2C_While_1Max;
	extern unsigned long ul_I2C_While_1TimeOut;
	extern unsigned long ul_I2C_While_2;
	extern unsigned long ul_I2C_While_2Max;
	extern unsigned long ul_I2C_While_2TimeOut;
	extern unsigned long ul_I2C_While_3;
	extern unsigned long ul_I2C_While_3Max;
	extern unsigned long ul_I2C_While_3TimeOut;
	extern unsigned long ul_I2C_While_4;
	extern unsigned long ul_I2C_While_4Max;
	extern unsigned long ul_I2C_While_4TimeOut;
	extern unsigned long ul_I2C_While_5;
	extern unsigned long ul_I2C_While_5Max;
	extern unsigned long ul_I2C_While_5TimeOut;
	
	extern unsigned char uc_FlagBlocageDetecte;		// Différent de 0 si blocage détecté dans BSP
	extern unsigned char uc_FlagReinitialiserI2C;	// En cas de blocage, reinitialiser l'I2C avant prochaine émission
	
	#define uc_NB_REINIT_MAX	50					// Nombre de tentative de reinit 
	
//----------------------------------

//---------------------------------- ETHERNET GESTION
	extern unsigned char uc_AdresseMac[6];	// Adresse MAC lue dans l'EEPROM --- !!! DOIT CORRESPONDRE A AdresseMAC_1...AdresseMAC_6 DE LA TABLE D'ECHANGE !!!
	
	extern unsigned char uc_DownloadEnCoursBloquerI2CVersBA;	// Telechargement en cours -> bloquer les émissions I2C
	extern unsigned char uc_DownloadEnCours;					// Telechargement en cours -> bloquer dialogue ecran / mémorisation en EEPROM
	extern unsigned char uc_ResetDemande;						// Reset demande en fin de telechargement (et si NOUVEAU PROGRAMME OK)
	
//----------------------------------

//---------------------------------- VARIABLES ESPION RS

	// Check acces multitaches ok (controle assembleur)
	// -> Positionnes par meme tache / pas de risque de x ecritures en meme temps
	// -> Lecture par autres fcts : ok - au pire ne voit pas le bit en train d'etre positionne
	// -> Seul cas : RAZ par espionRS - si intervient en court écriture -> refaire un RAZ dans ce cas !

	// Espion RS toujours actif en émission -> LOG
	// Commandes activées après saisie mot de passe
	// Désactivation auto après xx secondes d'inutilisation
	#define uc_ESPION_MOT_PASSE_ACTIVATION	"1256"
	extern unsigned char uc_CompteurCheckMdpEsion;
	extern unsigned char uc_EspionActive;
	extern unsigned char uc_CompteurDesactivationEspionAuto_sec;
	#define uc_TEMPO_DESACTIVATION_AUTO_sec	30
	
	extern unsigned char uc_CompteurEspionAIN_sec;
	#define uc_TEMPO_ESPION_AIN_sec			5 

	extern unsigned char uc_TempoStartAINMinMax_sec;
	#define uc_TEMPO_START_AIN_MIN_MAXsec			10

	// Indique la provenance de l'information à traiter (messages evenementiels)
	enum
	{
		uc_ESPION_TACHE_ECRAN_ERREUR,
		uc_ESPION_TACHE_ECRAN_ACTIVITE,
		uc_ESPION_DIALOGUE_BA_ERREUR,
		uc_ESPION_TACHE_BA_ERREUR,
		uc_ESPION_TACHE_BA_ACTIVITE,
		uc_ESPION_SCENARIOS,
		uc_ESPION_ALARME_ERREURS,
		uc_ESPION_ALARME_ACTIVITE,
		uc_ESPION_TACHE_PRINCIPALE_ERREUR,
		uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,
		uc_ESPION_RTC_ERREUR,
		uc_ESPION_RTC_ACTIVITE,
		uc_ESPION_HARD_ERREURS,
		uc_ESPION_CHAUFFAGE_ERREURS,
		uc_ESPION_CHAUFFAGE_ACTIVITE,
		uc_ESPION_TABLE_ECHANGE,
		uc_ESPION_ES,
		uc_ESPION_ETAT_BP,
		uc_ESPION_TACHE_ETHERNET_ACTIVITE,
		uc_ESPION_TACHE_ETHERNET_TXRX,
		uc_ESPION_TACHE_ETHERNET_DOWNLOAD,
		uc_ESPION_FIL_PILOTE,
		uc_ESPION_TELEINFO,
		uc_ESPION_TELEINFO_RX,
		uc_ENTREE_VENT,
	
		uc_ESPION_NB
	};
	
	// Active ou non l'affichage
	enum
	{
		uc_ESPION_OFF,
		uc_ESPION_ON
	};
	// Force affichage (en printf)
	enum
	{
		uc_AFFICHAGE_FORCE_NON,
		uc_AFFICHAGE_FORCE_OUI
	};
		
	// Menus disponibles
	enum
	{
		uc_MENU_AUCUN,
		uc_MENU_PRINCIPAL,
		uc_MENU_MESSAGES_EVENEMENTIELS,
		uc_MENU_AFFICHER_ESPIONS,
		uc_MENU_RAZ_ESPIONS,
		uc_MENU_ACTIONS
	};
	
	extern unsigned char uc_BloquerEmissionVersEcran;	// Permet de bloquer les émissions du BP vers l'écran sur demande de la RS espion

	// Espions Erreurs tache ECRAN
	extern unsigned short us_ErreursTacheEcran;
	extern unsigned short us_ErreursTacheEcranPrecedent;
	
	#define us_ERREUR_TACHE_ECRAN_AUCUNE									0x0000
	#define us_ERREUR_TACHE_ECRAN_TX_RS										0x0001
	#define us_ERREUR_TACHE_ECRAN_SATURATION_RX_ECRAN						0x0002
	#define us_ERREUR_TACHE_ECRAN_PB_TAILLE_TRAME_STATUS					0x0004
	#define us_ERREUR_TACHE_ECRAN_SATURATION_TX_ECRAN						0x0008
	#define us_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT				0x0010
	#define us_ERREUR_TACHE_ECRAN_DEMANDE_ECRAN_INCORRECTE					0x0020
	#define us_ERREUR_TACHE_ECRAN_RS_ECRAN									0x0040
	#define us_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_INIT					0x0080
	#define us_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_CANCEL					0x0100
	#define us_ERREUR_TACHE_ECRAN_CONFIG_VITESSE_RS							0x0200
	#define us_ERREUR_TACHE_ECRAN_MUTEX										0x0400 
	
	#define DETECTION_ERREUR_TACHE_ECRAN_TX_RS								us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_TX_RS
	#define DETECTION_ERREUR_TACHE_ECRAN_SATURATION_RX_ECRAN				us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_SATURATION_RX_ECRAN
	#define DETECTION_ERREUR_TACHE_ECRAN_PB_TAILLE_TRAME_STATUS				us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_PB_TAILLE_TRAME_STATUS
	#define DETECTION_ERREUR_TACHE_ECRAN_SATURATION_TX_ECRAN				us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_SATURATION_TX_ECRAN
	#define DETECTION_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT			us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT
	#define DETECTION_ERREUR_TACHE_ECRAN_DEMANDE_ECRAN_INCORRECTE			us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_DEMANDE_ECRAN_INCORRECTE
	#define DETECTION_ERREUR_TACHE_ECRAN_RS_ECRAN							us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_RS_ECRAN
	#define DETECTION_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_INIT				us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_INIT
	#define DETECTION_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_CANCEL			us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_CANCEL
	#define DETECTION_ERREUR_TACHE_ECRAN_CONFIG_VITESSE_RS					us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_CONFIG_VITESSE_RS
	#define DETECTION_ERREUR_TACHE_ECRAN_MUTEX								us_ErreursTacheEcran |= us_ERREUR_TACHE_ECRAN_MUTEX
	
	// Espion etat de la tacle ECRAN
	extern unsigned long ul_EspionTacheEcranNbOctetsRecus;
	extern unsigned long ul_EspionTacheEcranNbTramesCorrectesRecues;
	extern unsigned long ul_EspionTacheEcranNbTramesIncorrectesRecues;
	extern unsigned long ul_EspionTacheEcranNbTramesStatus;
	extern unsigned long ul_EspionTacheEcranNbTramesLectureDiscrete;
	extern unsigned long ul_EspionTacheEcranNbTramesEcritureDiscrete;
	extern unsigned long ul_EspionTacheEcranNbTramesLectureBloc;
	extern unsigned long ul_EspionTacheEcranNbTramesEcritureBloc;
	extern unsigned long ul_EspionTacheEcranNbRAZBufferReceptionSurTimeOUT;
	extern unsigned long ul_EspionTacheEcranNbDeclenchementTimeOUT;
	extern unsigned long ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur1;
	extern unsigned long ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur3;
	extern unsigned long ul_EspionTacheEcranEnteteTrameIncorrecte;

	extern unsigned char uc_EspionTacheEcranEtat;
	extern unsigned long ul_EspionTacheEcranCompteurActivite;

	// Espions Erreurs Dialogue BA par I2C
	extern unsigned short us_ErreursDialogueBA[uc_NB_BOITIER_AUXILIAIRE];
	extern unsigned short us_ErreursDialogueBAPrecedent[uc_NB_BOITIER_AUXILIAIRE];
	
	#define us_ERREUR_DIALOGUE_BA_AUCUNE									0x0000
	#define us_ERREUR_DIALOGUE_BA_SET_DESTINATION_ADDRESS					0x0001
	#define us_ERREUR_DIALOGUE_BA_CHK										0x0002
	#define us_ERREUR_DIALOGUE_BA_START										0x0004
	#define us_ERREUR_DIALOGUE_BA_EMISSION_DATA								0x0008
	#define us_ERREUR_DIALOGUE_BA_EMISSION_COMPLETE							0x0010
	#define us_ERREUR_DIALOGUE_BA_EMISSION_RESTART							0x0020
	#define us_ERREUR_DIALOGUE_BA_TAILLE_REPONSE							0x0040
	#define us_ERREUR_DIALOGUE_BA_CODE_REPONSE								0x0080
	#define us_ERREUR_DIALOGUE_BA_CRC_REPONSE								0x0100
	#define us_ERREUR_DIALOGUE_BA_STOP_TRANSFERT							0x0200
	#define us_ERREUR_DIALOGUE_BA_CRC_RETOUR								0x0400
	
	#define DETECTION_ERREUR_DIALOGUE_BA_SET_DESTINATION_ADDRESS(uc_NumeroBA)	us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_SET_DESTINATION_ADDRESS
	#define DETECTION_ERREUR_DIALOGUE_BA_CHK(uc_NumeroBA)						us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_CHK
	#define DETECTION_ERREUR_DIALOGUE_BA_START(uc_NumeroBA)						us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_START
	#define DETECTION_ERREUR_DIALOGUE_BA_EMISSION_DATA(uc_NumeroBA)				us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_EMISSION_DATA
	#define DETECTION_ERREUR_DIALOGUE_BA_EMISSION_COMPLETE(uc_NumeroBA)			us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_EMISSION_COMPLETE
	#define DETECTION_ERREUR_DIALOGUE_BA_EMISSION_RESTART(uc_NumeroBA)			us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_EMISSION_RESTART
	#define DETECTION_ERREUR_DIALOGUE_BA_TAILLE_REPONSE(uc_NumeroBA)			us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_TAILLE_REPONSE
	#define DETECTION_ERREUR_DIALOGUE_BA_CODE_REPONSE(uc_NumeroBA)				us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_CODE_REPONSE
	#define DETECTION_ERREUR_DIALOGUE_BA_CRC_REPONSE(uc_NumeroBA)				us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_CRC_REPONSE
	#define DETECTION_ERREUR_DIALOGUE_BA_STOP_TRANSFERT(uc_NumeroBA)			us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_STOP_TRANSFERT
	#define DETECTION_ERREUR_DIALOGUE_BA_CRC_RETOUR(uc_NumeroBA)				us_ErreursDialogueBA[uc_NumeroBA] |= us_ERREUR_DIALOGUE_BA_CRC_RETOUR

	// Espions Erreurs tache dialogue BA
	extern unsigned short us_ErreursTacheBA;
	extern unsigned short us_ErreursTacheBAPrecedent;

	#define us_ERREUR_TACHE_BA_AUCUNE										0x0000
	#define us_ERREUR_TACHE_BA_OUVERTURE_I2C								0x0001
	#define us_ERREUR_TACHE_BA_CONFIG_VITESSE								0x0002
	#define us_ERREUR_TACHE_BA_CONFIG_MAITRE								0x0004
	#define us_ERREUR_TACHE_BA_CONFIG_ADRESSE								0x0008
	#define us_ERREUR_TACHE_BA_CREATION_MESSAGE_QUEUE						0x0010
	#define us_ERREUR_TACHE_BA_CLOSE_MESSAGE_QUEUE							0x0020
	#define us_ERREUR_TACHE_BA_CLOSE_I2C									0x0040

	#define DETECTION_ERREUR_TACHE_BA_OUVERTURE_I2C							us_ErreursTacheBA |= us_ERREUR_TACHE_BA_OUVERTURE_I2C
	#define DETECTION_ERREUR_TACHE_BA_CONFIG_VITESSE						us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CONFIG_VITESSE
	#define DETECTION_ERREUR_TACHE_BA_CONFIG_MAITRE							us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CONFIG_MAITRE
	#define DETECTION_ERREUR_TACHE_BA_CONFIG_ADRESSE						us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CONFIG_ADRESSE
	#define DETECTION_ERREUR_TACHE_BA_CREATION_MESSAGE_QUEUE				us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CREATION_MESSAGE_QUEUE
	#define DETECTION_ERREUR_TACHE_BA_CLOSE_MESSAGE_QUEUE					us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CLOSE_MESSAGE_QUEUE
	#define DETECTION_ERREUR_TACHE_BA_CLOSE_I2C								us_ErreursTacheBA |= us_ERREUR_TACHE_BA_CLOSE_I2C
	
	// Espions etat de la tache BA
	extern unsigned char uc_EspionTacheBAEtat;
	extern unsigned char uc_Espionsl_fct_write_polled;
	extern unsigned long ul_EspionTacheBACompteurActivite;
	extern unsigned long ul_EspionTacheBACompteurEmissionI2C[uc_NB_BOITIER_AUXILIAIRE];
	extern unsigned long ul_EspionTacheBACompteurErreurI2C[uc_NB_BOITIER_AUXILIAIRE];
	extern unsigned long ul_EspionTacheBANbBlocageI2C;
	extern unsigned char uc_NbEssaisReinitMax;
	extern unsigned char uc_NbLoupesReinit;

	// Espions Scenarios
	extern unsigned long ul_ScenariosNbExecution[uc_SCENARIO_NB];
	extern unsigned long ul_ScenariosNumeroIncorrect;
	extern unsigned long ul_ScenariosAdressesValeursIncorrectes;
	extern unsigned long ul_ScenariosAdressesEnDehorsPlage;
	extern unsigned long ul_ScenariosInitAdressesEnDehorsPlage;
	
	// Espions Erreurs alarme
	extern unsigned short us_ErreursAlarme;
	extern unsigned short us_ErreursAlarmePrecedent;
	
	#define us_ERREUR_ALARME_AUCUNE											0x0000
	//#define us_ERREUR_ALARME_PB_TIMER_INIT									0x0001
	
	//#define DETECTION_ERREUR_ALARME_PB_TIMER_INIT							us_ErreursAlarme |= us_ERREUR_ALARME_PB_TIMER_INIT
	
	//extern unsigned long ul_EspionAlarmeNbDeclenchementTimer;
	
	// Espions Erreurs chauffage
	extern unsigned short us_ErreursChauffage;
	extern unsigned short us_ErreursChauffagePrecedent;

	#define us_ERREUR_CHAUFFAGE_AUCUNE								0x0000
	#define us_ERREUR_CHAUFFAGE_MUTEX_ZJ							0x0001
	#define us_ERREUR_CHAUFFAGE_MUTEX_ZN							0x0002
	#define us_ERREUR_CHAUFFAGE_MUTEX_SDB1							0x0004
	#define us_ERREUR_CHAUFFAGE_MUTEX_SDB2							0x0008
	#define us_ERREUR_SCENARIO_MUTEX_ZJ								0x0010
	#define us_ERREUR_SCENARIO_MUTEX_ZN								0x0020
	#define us_ERREUR_SCENARIO_MUTEX_SDB1							0x0040
	#define us_ERREUR_SCENARIO_MUTEX_SDB2							0x0080

	#define DETECTION_ERREUR_CHAUFFAGE_MUTEX_ZJ						us_ErreursChauffage |= us_ERREUR_CHAUFFAGE_MUTEX_ZJ
	#define DETECTION_ERREUR_CHAUFFAGE_MUTEX_ZN						us_ErreursChauffage |= us_ERREUR_CHAUFFAGE_MUTEX_ZN
	#define DETECTION_ERREUR_CHAUFFAGE_MUTEX_SDB1					us_ErreursChauffage |= us_ERREUR_CHAUFFAGE_MUTEX_SDB1
	#define DETECTION_ERREUR_CHAUFFAGE_MUTEX_SDB2					us_ErreursChauffage |= us_ERREUR_CHAUFFAGE_MUTEX_SDB2
	#define DETECTION_ERREUR_SCENARIO_MUTEX_ZJ						us_ErreursChauffage |= us_ERREUR_SCENARIO_MUTEX_ZJ
	#define DETECTION_ERREUR_SCENARIO_MUTEX_ZN						us_ErreursChauffage |= us_ERREUR_SCENARIO_MUTEX_ZN
	#define DETECTION_ERREUR_SCENARIO_MUTEX_SDB1					us_ErreursChauffage |= us_ERREUR_SCENARIO_MUTEX_SDB1
	#define DETECTION_ERREUR_SCENARIO_MUTEX_SDB2					us_ErreursChauffage |= us_ERREUR_SCENARIO_MUTEX_SDB2
	
	// Espions activite chauffage
	extern unsigned long ul_EspionCompteurITAlternanceSecteur;
	extern unsigned long ul_EspionCompteurITAlternanceSecteurMain;
	extern unsigned long ul_EspionCompteurITAlternanceSecteurMainPrecedent;

	extern unsigned long ul_EspionCompteurITTimerFilPilote1;
	extern unsigned long ul_EspionCompteurITTimerFilPilote2;
	extern unsigned long ul_EspionCompteurITTimerFilPilote1Main;
	extern unsigned long ul_EspionCompteurITTimerFilPilote2Main;
	extern unsigned long ul_EspionCompteurITTimerFilPilote1MainPrecedent;
	extern unsigned long ul_EspionCompteurITTimerFilPilote2MainPrecedent;
	
	// Espions Erreurs tache principale
	extern unsigned short us_ErreursTachePrincipale;
	extern unsigned short us_ErreursTachePrincipalePrecedent;

	#define us_ERREUR_TACHE_PRINCIPALE_AUCUNE									0x0000
	#define us_ERREUR_TACHE_PRINCIPALE_CREATION_MESSAGE_POOL					0x0001
	#define us_ERREUR_TACHE_PRINCIPALE_CREATION_SEMAPHORE_DATE					0x0002
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ATTRIBUTS						0x0004
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZJ							0x0008
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZN							0x0010
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB1							0x0020
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB2							0x0040
	#define us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_DATE_HEURE_POUR_ECRAN			0x0080
	#define us_ERREUR_TACHE_PRINCIPALE_MESSAGE_ALLOC							0x0100
	#define us_ERREUR_TACHE_PRINCIPALE_MESAGE_SEND								0x0200
	#define us_ERREUR_TACHE_PRINCIPALE_MESSAGE_AVAILABLE						0x0400
	#define us_ERREUR_TACHE_PRINCIPALE_ARROSAGE_CALCUL							0x0800
	#define us_ERREUR_TACHE_PRINCIPALE_START_TACHE_ECRAN						0x1000
	#define us_ERREUR_TACHE_PRINCIPALE_START_TACHE_BA							0x2000
	#define us_ERREUR_TACHE_PRINCIPALE_START_TACHE_TELEINFO						0x4000
	#define us_ERREUR_TACHE_PRINCIPALE_START_TACHE_ETHERNET						0x8000
	
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_CREATION_MESSAGE_POOL				us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_CREATION_MESSAGE_POOL
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_CREATION_SEMAPHORE_DATE			us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_CREATION_SEMAPHORE_DATE
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ATTRIBUTS				us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ATTRIBUTS
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZJ						us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZJ
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZN						us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZN
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB1					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB1
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB2					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB2
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_DATE_HEURE_POUR_ECRAN	us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_DATE_HEURE_POUR_ECRAN
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_MESSAGE_ALLOC						us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_MESSAGE_ALLOC
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_MESAGE_SEND						us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_MESAGE_SEND
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_MESSAGE_AVAILABLE					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_MESSAGE_AVAILABLE
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_ARROSAGE_CALCUL					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_ARROSAGE_CALCUL
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_ECRAN					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_START_TACHE_ECRAN
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_BA					us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_START_TACHE_BA
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_TELEINFO				us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_START_TACHE_TELEINFO
	#define DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_ETHERNET				us_ErreursTachePrincipale |= us_ERREUR_TACHE_PRINCIPALE_START_TACHE_ETHERNET

	// Espions tache principale
	extern unsigned char uc_EspionTachePrincipaleEtat;
	extern unsigned long ul_EspionTachePrincipaleCompteurActivite;
	extern unsigned long ul_EspionTachePrincipaleNbMessagesBAEnvoyes;
	extern unsigned long ul_EspionTachePrincipaleNbMessagesBAErreurs1;
	extern unsigned long ul_EspionTachePrincipaleNbMessagesBAErreurs2;
	extern unsigned long ul_EspionTachePrincipaleNbMessagesBAErreurs3;
	extern unsigned long ul_EspionTachePrincipaleNbMessagesBARecus;
	//extern unsigned long ul_EspionTachePrincipaleAcquisitionAINEnCours;
	//extern unsigned long ul_EspionTachePrincipaleAcquisitionAINOK;
	extern unsigned long ul_EspionTachePrincipaleNbCoupuresCourant;
	extern unsigned long ul_EspionTachePrincipaleNbDeclenchementTimer50ms;
	extern unsigned long ul_EspionTachePrincipaleNbDeclenchementTimer1sec;
	extern unsigned long ul_EspionTableEchangeNbAccesEcritureRefuse;
	extern unsigned long ul_EspionTableEchangeNbAccesLectureRefuse;

	// Espions erreurs RTC
	extern unsigned short us_ErreursRTC;
	extern unsigned short us_ErreursRTCPrecedent;

	#define us_ERREUR_RTC_AUCUNE											0x0000
	#define us_ERREUR_RTC_INIT_RTC_MQX										0x0001
	#define us_ERREUR_RTC_INIT_CONVERSION_DATE								0x0002
	#define us_ERREUR_RTC_CONVERSION_DATE									0x0004
	#define us_ERREUR_RTC_CONVERSION_DATE2									0x0008
	#define us_ERREUR_RTC_MUTEX												0x0010
	#define us_ERREUR_RTC_CONVERSION_HEURE_ETE								0x0020
	#define us_ERREUR_RTC_CONVERSION_HEURE_HIVER							0x0040
	#define us_ERREUR_RTC_RTC_MQX_HEURE_ETE									0x0080
	#define us_ERREUR_RTC_RTC_MQX_HEURE_HIVER								0x0100
	#define us_ERREUR_RTC_RTC_MQX											0x0200
	
	#define DETECTION_ERREUR_RTC_INIT_RTC_MQX								us_ErreursRTC |= us_ERREUR_RTC_INIT_RTC_MQX
	#define DETECTION_ERREUR_RTC_INIT_CONVERSION_DATE						us_ErreursRTC |= us_ERREUR_RTC_INIT_CONVERSION_DATE
	#define DETECTION_ERREUR_RTC_CONVERSION_DATE							us_ErreursRTC |= us_ERREUR_RTC_CONVERSION_DATE
	#define DETECTION_ERREUR_RTC_CONVERSION_DATE2							us_ErreursRTC |= us_ERREUR_RTC_CONVERSION_DATE2
	#define DETECTION_ERREUR_RTC_MUTEX										us_ErreursRTC |= us_ERREUR_RTC_MUTEX
	#define DETECTION_ERREUR_RTC_CONVERSION_HEURE_ETE						us_ErreursRTC |= us_ERREUR_RTC_CONVERSION_HEURE_ETE
	#define DETECTION_ERREUR_RTC_CONVERSION_HEURE_HIVER						us_ErreursRTC |= us_ERREUR_RTC_CONVERSION_HEURE_HIVER
	#define DETECTION_ERREUR_RTC_RTC_MQX_HEURE_ETE							us_ErreursRTC |= us_ERREUR_RTC_RTC_MQX_HEURE_ETE
	#define DETECTION_ERREUR_RTC_RTC_MQX_HEURE_HIVER						us_ErreursRTC |= us_ERREUR_RTC_RTC_MQX_HEURE_HIVER
	#define DETECTION_ERREUR_RTC_RTC_MQX									us_ErreursRTC |= us_ERREUR_RTC_RTC_MQX

	// Espions RTC
	extern unsigned long ul_EspionRTCNbMAJRTC;
	extern unsigned long ul_EspionRTCNbPassageHeureEte;
	extern unsigned long ul_EspionRTCNbPassageHeureHiver;
	
	// Espions Erreurs HARD
	extern unsigned long ul_ErreursHard1;
	extern unsigned long ul_ErreursHard1Precedent;
	extern unsigned long ul_ErreursHard2;
	extern unsigned long ul_ErreursHard2Precedent;

	#define ul_ERREUR_HARD_AUCUNE									0x00000000
	
	// Constantes pour ul_ErreursHard1
	#define ul_ERREUR_HARD_OPEN_ADC									0x00000001
	#define ul_ERREUR_HARD_OPEN_ADC_FUITE_1							0x00000002
	#define ul_ERREUR_HARD_OPEN_ADC_FUITE_2							0x00000004
	#define ul_ERREUR_HARD_OPEN_PWM									0x00000008
	#define ul_ERREUR_HARD_CONFIG_PWM								0x00000010
	#define ul_ERREUR_HARD_CONFIG_DOUT_PWM							0x00000020
	#define ul_ERREUR_HARD_MODIF_DUTY_PWM							0x00000040
	#define ul_ERREUR_HARD_OPEN_IO_OUVERTURE_SIRENE_INTERIEURE		0x00000080
	#define ul_ERREUR_HARD_OPEN_IO_OUVERTURE_SIRENE_EXTERIEURE		0x00000100
	#define ul_ERREUR_HARD_OPEN_IO_OUVERTURE_TABLEAU_DOMOTIQUE		0x00000200
	#define ul_ERREUR_HARD_OPEN_IO_SIRENE_EXTERIEURE				0x00000400
	#define ul_ERREUR_HARD_OPEN_IO_15VSP_ALIM_BA					0x00000800
	#define ul_ERREUR_HARD_OPEN_IO_LED_ETAT_BP						0x00001000
	#define ul_ERREUR_HARD_OPEN_IO_VANNE_ARROSAGE					0x00002000
	#define ul_ERREUR_HARD_OPEN_IO_PRISE_SECURITE					0x00004000
	#define ul_ERREUR_HARD_OPEN_IO_MACHINE_A_LAVER					0x00008000
	#define ul_ERREUR_HARD_OPEN_IO_CUMULUS							0x00010000
	#define ul_ERREUR_HARD_OPEN_IO_FP_ZJ							0x00020000
	#define ul_ERREUR_HARD_OPEN_IO_FP_ZN							0x00040000
	#define ul_ERREUR_HARD_OPEN_IO_FP_SDB1							0x00080000
	#define ul_ERREUR_HARD_OPEN_IO_FP_SDB2							0x00100000
	#define ul_ERREUR_HARD_OPEN_IO_DETECT_OUV						0x00200000
	#define ul_ERREUR_HARD_OPEN_IO_DETECT_PRES1						0x00400000
	#define ul_ERREUR_HARD_OPEN_IO_DETECT_PRES2						0x00800000
	#define ul_ERREUR_HARD_OPEN_IO_DETECT_PLUIE						0x01000000
	#define ul_ERREUR_HARD_OPEN_IO_LED_TELEINFO						0x02000000
	#define ul_ERREUR_HARD_OPEN_ADC_VBAT							0x04000000
	#define ul_ERREUR_HARD_OPEN_IO_SECTEUR_SYNCHRO					0x08000000
	#define ul_ERREUR_HARD_OPEN_IO_SECTEUR_SYNCHRO_INIT_IT			0x10000000
	#define ul_ERREUR_HARD_OPEN_IO_ETAT_ALIM_PRINCIPALE				0x20000000
	#define ul_ERREUR_HARD_OPEN_IO_ECRAN_DIRECTION					0x40000000
	#define ul_ERREUR_HARD_OPEN_IO_ERREUR_ECRAN_HARD				0x80000000
	
	// Constantes pour ul_ErreursHard2
	#define ul_ERREUR_HARD_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_1	0x00000001
	#define ul_ERREUR_HARD_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_2	0x00000002
	#define ul_ERREUR_HARD_OPEN_IO_DIN_ETOR_reserve1				0x00000004
	#define ul_ERREUR_HARD_OPEN_IO_DIN_ETOR_reserve2				0x00000008
	#define ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ1						0x00000010
	#define ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ2						0x00000020
	#define ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ3						0x00000040
	#define ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ4						0x00000080
	#define ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ5						0x00000100
	#define ul_ERREUR_HARD_OPEN_IO_CTRL_BATTERIE					0x00000200
	#define ul_ERREUR_HARD_OPEN_IO_EEPROM_ADRESSE_MAC				0x00000400
	#define ul_ERREUR_HARD_OPEN_IO_EEPROM_SOFT						0x00000800
	#define ul_ERREUR_HARD_OPEN_IO_DIN_D5							0x00001000
	#define ul_ERREUR_HARD_OPEN_IO_DIN_D6							0x00002000
	#define ul_ERREUR_HARD_OPEN_IO_DIN_D7							0x00004000
	
	#define DETECTION_ERREUR_HARD_OPEN_ADC							ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_ADC
	#define DETECTION_ERREUR_HARD_OPEN_ADC_FUITE_1					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_ADC_FUITE_1
	#define DETECTION_ERREUR_HARD_OPEN_ADC_FUITE_2					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_ADC_FUITE_2
	#define DETECTION_ERREUR_HARD_OPEN_PWM							ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_PWM
	#define DETECTION_ERREUR_HARD_CONFIG_PWM						ul_ErreursHard1 |= ul_ERREUR_HARD_CONFIG_PWM
	#define DETECTION_ERREUR_HARD_CONFIG_DOUT_PWM					ul_ErreursHard1 |= ul_ERREUR_HARD_CONFIG_DOUT_PWM
	#define DETECTION_ERREUR_HARD_MODIF_DUTY_PWM					ul_ErreursHard1 |= ul_ERREUR_HARD_MODIF_DUTY_PWM
	#define DETECTION_ERREUR_OPEN_IO_OUVERTURE_SIRENE_INTERIEURE	ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_OUVERTURE_SIRENE_INTERIEURE
	#define DETECTION_ERREUR_OPEN_IO_OUVERTURE_SIRENE_EXTERIEURE	ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_OUVERTURE_SIRENE_EXTERIEURE
	#define DETECTION_ERREUR_OPEN_IO_OUVERTURE_TABLEAU_DOMOTIQUE	ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_OUVERTURE_TABLEAU_DOMOTIQUE
	#define DETECTION_ERREUR_OPEN_IO_SIRENE_EXTERIEURE				ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_SIRENE_EXTERIEURE
	#define DETECTION_ERREUR_OPEN_IO_15VSP_ALIM_BA					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_15VSP_ALIM_BA
	#define DETECTION_ERREUR_OPEN_IO_LED_ETAT_BP					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_LED_ETAT_BP
	#define DETECTION_ERREUR_OPEN_IO_VANNE_ARROSAGE					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_VANNE_ARROSAGE
	#define DETECTION_ERREUR_OPEN_IO_PRISE_SECURITE					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_PRISE_SECURITE
	#define DETECTION_ERREUR_OPEN_IO_MACHINE_A_LAVER				ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_MACHINE_A_LAVER
	#define DETECTION_ERREUR_OPEN_IO_CUMULUS						ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_CUMULUS
	#define DETECTION_ERREUR_OPEN_IO_FP_ZJ							ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_FP_ZJ
	#define DETECTION_ERREUR_OPEN_IO_FP_ZN							ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_FP_ZN
	#define DETECTION_ERREUR_OPEN_IO_FP_SDB1						ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_FP_SDB1
	#define DETECTION_ERREUR_OPEN_IO_FP_SDB2						ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_FP_SDB2
	#define DETECTION_ERREUR_OPEN_IO_DETECT_OUV						ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_DETECT_OUV
	#define DETECTION_ERREUR_OPEN_IO_DETECT_PRES1					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_DETECT_PRES1
	#define DETECTION_ERREUR_OPEN_IO_DETECT_PRES2					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_DETECT_PRES2
	#define DETECTION_ERREUR_OPEN_IO_DETECT_PLUIE					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_DETECT_PLUIE
	#define DETECTION_ERREUR_OPEN_IO_LED_TELEINFO					ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_LED_TELEINFO
	#define DETECTION_ERREUR_HARD_OPEN_ADC_VBAT						ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_ADC_VBAT
	#define DETECTION_ERREUR_OPEN_IO_SECTEUR_SYNCHRO				ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_SECTEUR_SYNCHRO
	#define DETECTION_ERREUR_OPEN_IO_SECTEUR_SYNCHRO_INIT_IT		ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_SECTEUR_SYNCHRO_INIT_IT
	#define DETECTION_ERREUR_OPEN_IO_ETAT_ALIM_PRINCIPALE			ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_ETAT_ALIM_PRINCIPALE
	#define DETECTION_ERREUR_OPEN_IO_ECRAN_DIRECTION				ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_ECRAN_DIRECTION
	#define DETECTION_ERREUR_OPEN_IO_ERREUR_ECRAN_HARD				ul_ErreursHard1 |= ul_ERREUR_HARD_OPEN_IO_ERREUR_ECRAN_HARD
	
	#define DETECTION_ERREUR_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_1	ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_1
	#define DETECTION_ERREUR_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_2	ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_2
	#define DETECTION_ERREUR_OPEN_IO_DIN_ETOR_reserve1				ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DIN_ETOR_reserve1
	#define DETECTION_ERREUR_OPEN_IO_DIN_ETOR_reserve2				ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DIN_ETOR_reserve2
	#define DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ1					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ1
	#define DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ2					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ2
	#define DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ3					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ3
	#define DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ4					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ4
	#define DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ5					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DOUT_DebugJ5
	#define DETECTION_ERREUR_OPEN_IO_CTRL_BATTERIE					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_CTRL_BATTERIE
	#define DETECTION_ERREUR_OPEN_IO_EEPROM_ADRESSE_MAC				ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_EEPROM_ADRESSE_MAC
	#define DETECTION_ERREUR_OPEN_IO_EEPROM_SOFT					ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_EEPROM_SOFT
	#define DETECTION_ERREUR_OPEN_IO_DIN_D5							ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DIN_D5
	#define DETECTION_ERREUR_OPEN_IO_DIN_D6							ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DIN_D6
	#define DETECTION_ERREUR_OPEN_IO_DIN_D7							ul_ErreursHard2 |= ul_ERREUR_HARD_OPEN_IO_DIN_D7

	// Espions changement état alarme
	extern unsigned char uc_AlarmeDetection_Precedent;
	extern unsigned char uc_AlarmeFraude_Precedent;
	extern unsigned char uc_AlarmeCompteurSecondes_Precedent;
	extern unsigned char uc_Alarme_NouveauCode_1_LSB_Precedent;
	extern unsigned char uc_Alarme_NouveauCode_1_MSB_Precedent;
	extern unsigned char uc_Alarme_NouveauCode_2_LSB_Precedent;
	extern unsigned char uc_Alarme_NouveauCode_2_MSB_Precedent;
	extern unsigned char Tb_Echange_Alarme_Commande_Precedent;
	extern unsigned char Tb_Echange_Alarme_Mode_Precedent;
	extern unsigned char Tb_Echange_Alarme_SuiviChangementCode_Precedent;
	extern unsigned char Tb_Echange_Alarme_Autorisation_Precedent;
	extern unsigned char Tb_Echange_Alarme_SuiviAlarme_Precedent;
	extern unsigned char uc_AlarmeActivee_Precedent;
	extern unsigned char uc_AlarmeDeclenchee_Precedent;
	extern unsigned char uc_AlarmeProcedureSortieEnCours_Precedent;
	extern unsigned char uc_AlarmeProcedureRentreeEnCours_Precedent;
	
	// Espions changement état chauffage
	extern struct_chauffage	st_chauf_ZJ_Precedent, st_chauf_ZN_Precedent, st_chauf_Zsdb1_Precedent, st_chauf_Zsdb2_Precedent;

	// Espion changement état E/S
	extern unsigned char uc_DIN_OuvertureSireneInterieurePrecedent;
	extern unsigned char uc_DIN_OuvertureSireneExterieurePrecedent;
	extern unsigned char uc_DIN_OuvertureTableauDominiquePrecedent;
	extern unsigned char uc_Cde_Sirene_ExterieurePrecedent;
	extern unsigned char uc_Cde_15VSP_AlimBAPrecedent;
	extern unsigned char uc_Cde_VanneArrosagePrecedent;
	extern unsigned char uc_Cde_PriseSecuritePrecedent;
	extern unsigned char uc_Cde_MachineALaverPrecedent;
	extern unsigned char uc_Cde_CumulusPrecedent;
	extern unsigned char uc_DIN_Detection_OuverturePrecedent;
	extern unsigned char uc_DIN_Detection_presence1Precedent;
	extern unsigned char uc_DIN_Detection_presence2Precedent;
	extern unsigned char uc_DIN_Detection_PluiePrecedent;
	extern unsigned char uc_DIN_Etat_AlimPrincipalePrecedent;
	extern unsigned char uc_DOUT_EcranDirectionPrecedent;
	extern unsigned char uc_DIN_OuvertureDecteurPresence1Precedent;
	extern unsigned char uc_DIN_OuvertureDecteurPresence2Precedent;
	extern unsigned char uc_DIN_VitesseVentPrecedent;
	extern unsigned char uc_DIN_BoutonMagiquePrecedent;
	extern unsigned char uc_DIN_ErreurEcranHardPrecedent;

	extern unsigned char uc_EspionTacheEthernetEtat;
	extern unsigned char uc_EspionTacheEthernetInitRTCS;
	extern unsigned char uc_EspionTacheEthernetDialogueAvecServeur;
	extern unsigned long ul_EspionTacheEthernetCompteurActivite;
	extern unsigned char uc_EspionTacheEthernetOpenSocket;
	extern unsigned long ul_EspionTacheEthernetCableOK;
	extern unsigned long ul_EspionTacheEthernetCablePB;
	extern unsigned long ul_EspionTacheEthernetDCHPOK;
	extern unsigned long ul_EspionTacheEthernetDCHPPB;
	extern unsigned long ul_EspionTacheEthernetIPFixeOK;
	extern unsigned long ul_EspionTacheEthernetIPFixePB;
	extern unsigned long ul_EspionTacheEthernetDNSOK;
	extern unsigned long ul_EspionTacheEthernetDNSPB;
	extern unsigned long ul_EspionTacheEthernetDialogueServeurOK;
	extern unsigned long ul_EspionTacheEthernetDialogueServeurPBRTCS;
	extern unsigned long ul_EspionTacheEthernetDialogueServeurPBData;
	extern unsigned long ul_EspionTacheEthernetFonction1OK;
	extern unsigned long ul_EspionTacheEthernetFonction1PB;
	extern unsigned long ul_EspionTacheEthernetFonction2OK;
	extern unsigned long ul_EspionTacheEthernetFonction2PB;
	extern unsigned long ul_EspionTacheEthernetFonctions3et4OK;
	extern unsigned long ul_EspionTacheEthernetFonctions3et4PB;
	extern unsigned char uc_EspionTacheEthernetGetInformationServer;
	extern unsigned char uc_EspionTacheEthernetPostInformationServer;
	extern unsigned char uc_EspionTacheEthernetActionManagment;
	extern unsigned char uc_EspionTacheEthernetTraiterActions;
	extern unsigned char uc_EspionTacheEthernetTraitementAlarme;
	extern unsigned char uc_EspionTacheEthernetTraitementAction;
	extern unsigned long ul_EspionTacheEthernetFonctionDownload;
	extern unsigned char uc_EspionTacheEthernetDownload;
	
	extern unsigned char uc_EspionEepromSoftErreurLectureEtat;
	extern unsigned char uc_EspionEepromSoftErreurAttenteBusy;
	extern unsigned char uc_EspionEepromSoftErreurEnableEcriture;
	extern unsigned char uc_EspionEepromSoftErreurEcriture;
	extern unsigned char uc_EspionEepromSoftErreurLecture;
	extern unsigned char uc_EspionEepromSoftErreurEnableEcritureStatus;
	extern unsigned char uc_EspionEepromSoftErreurStatusEnableEcriture;
	extern unsigned char uc_EspionEepromSoftErreurEffacement;
	
	// Espions TELEINFO
	extern unsigned short us_ErreursTacheTeleinfo;
	extern unsigned short us_ErreursTacheTeleinfoPrecedent;

	#define us_ERREUR_TACHE_TELEINFO_AUCUNE									0x0000
	#define us_ERREUR_TACHE_TELEINFO_RS_OPEN								0x0001
	#define us_ERREUR_TACHE_TELEINFO_RS_VITESSE								0x0002
	#define us_ERREUR_TACHE_TELEINFO_RS_PARITE								0x0004
	#define us_ERREUR_TACHE_TELEINFO_RS_BITS								0x0008
	#define us_ERREUR_TACHE_TELEINFO_RS_PB_TIMER_TIME_OUT_INIT				0x0010
	#define us_ERREUR_TACHE_TELEINFO_SATURATION_RX_ECRAN					0x0020
	#define us_ERREUR_TACHE_TELEINFO_RX_ENABLE								0x0040
	
	#define DETECTION_ERREUR_TACHE_TELEINFO_RS_OPEN							us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RS_OPEN
	#define DETECTION_ERREUR_TACHE_TELEINFO_RS_VITESSE						us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RS_VITESSE
	#define DETECTION_ERREUR_TACHE_TELEINFO_RS_PARITE						us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RS_PARITE
	#define DETECTION_ERREUR_TACHE_TELEINFO_RS_BITS							us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RS_BITS
	#define DETECTION_ERREUR_TACHE_TELEINFO_PB_TIMER_TIME_OUT_INIT			us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RS_PB_TIMER_TIME_OUT_INIT
	#define DETECTION_ERREUR_TACHE_TELEINFO_SATURATION_RX_ECRAN				us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_SATURATION_RX_ECRAN
	#define DETECTION_ERREUR_TACHE_TELEINFO_RX_ENABLE						us_ErreursTacheTeleinfo |= us_ERREUR_TACHE_TELEINFO_RX_ENABLE

	extern unsigned char uc_EspionTacheTeleinfoEtat;
	extern unsigned long ul_EspionTacheTeleinfoCompteurActivite;
	extern unsigned long ul_EspionTacheTeleinfoNbDeclenchementTimeOUT;
	extern unsigned long ul_EspionTacheTeleinfoCompteurRx;
	extern unsigned long ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT;
	extern unsigned long ul_EspionTacheTeleinfoCompteurGroupeInfoDebut;
	extern unsigned long ul_EspionTacheTeleinfoCompteurGroupeInfoFin;
	extern unsigned long ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum;
	extern unsigned long ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC;
	extern unsigned long ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData;
		
//----------------------------------

	
	extern unsigned long ul_CompteurMutexEnvoyerTrameStatus;
	extern unsigned long ul_CompteurMutexEnvoyerTrameStatusErreur;
	extern unsigned long ul_CompteurMutexTacheEcran;
	extern unsigned long ul_CompteurMutexTacheEcranErreur;
	extern unsigned long ul_CompteurMutexRTC;
	extern unsigned long ul_CompteurMutexRTCErreur;

	
