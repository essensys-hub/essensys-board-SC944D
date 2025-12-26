#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include <string.h>
#include <rtcs.h>
#include <ipcfg.h>
#include <stdlib.h>

#include "application.h"	// xxx uniquement pour les essais : à enlever ensuite

#include "main.h"

#include "TableEchange.h"

#define extern
#include "global.h"

#undef extern

#include "TableEchangeAcces.h"
#include "alarme.h"
#include "ba.h"
#include "Ecran.h"
#include "TeleInfo.h"
#include "Hard.h"
#include "Scenario.h"
#include "chauffage.h"
#include "gestiondateheure.h"
#include "reveil.h"
#include "www.h"
#include "eepromadressemac.h"
#include "eepromsoft.h"
#include "eepromspi.h"
#include "filpilote.h"
#include "tableechangeflash.h"
#include "anemo.h"
#include "adcdirect.h"

#include "espionrs.h"


//xxx a verifier
#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This application requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero in user_config.h. Please recompile BSP with this option.
#endif


#ifndef BSP_DEFAULT_IO_CHANNEL_DEFINED
#error This application requires BSP_DEFAULT_IO_CHANNEL to be not NULL. Please set corresponding BSPCFG_ENABLE_TTYx to non-zero in user_config.h and recompile BSP with this option.
#endif

#if ! BSPCFG_ENABLE_I2C0
#error This application requires BSPCFG_ENABLE_I2C0 defined non-zero in user_config.h. Please recompile BSP with this option.
#endif


// Liste des taches de l'application
#define MAIN_TASK   1
#define ECRAN_TASK	2
#define BA_TASK 	3
#define TELE_TASK	4
#define ETH_TASK 	5

TASK_TEMPLATE_STRUCT MQX_template_list[] =
{
/*  Task number, Entry point, Stack, Pri, String, Auto? */
   {MAIN_TASK,	Main_task,		1596,  8,   "Main",		MQX_AUTO_START_TASK},
   {ECRAN_TASK,	vd_Ecran_task,	1500 /*696*/,  8,   "Ecran",	MQX_TIME_SLICE_TASK, 50},	// dialogue avec l'écran
   {BA_TASK,   	Boitiers_task,	1796,  8,   "I2C",		MQX_TIME_SLICE_TASK, 50},	// dialogue avec les boitiers auxiliaires
   {TELE_TASK,  TeleInfo_task, 	1396,  8,   "TeleInf",	MQX_TIME_SLICE_TASK, 30},	// dialogue avec le compteur LINKY //xxx pourquoi 30 ???
   //{ETH_TASK,   Ethernet_task,  2000,  8,   "Ethernet", MQX_TIME_SLICE_TASK, 50}, OVERFLOW A 2000 SUR TRAITEMENT ORDRE ALARME_ON	
   {ETH_TASK,   Ethernet_task,  3000,  8,   "Ethernet", MQX_TIME_SLICE_TASK, 50},
   {0,           0,           0,     0,   0,      0,                 }
};	//xxx revoir les priorites et les tailles de stack


void EtatMemoire(void);
void EtatMemoire(void)
{
	unsigned long xxx = 0;
	static unsigned long xxxprecedent = 0;
	
	if(vd_EspionMainActive() == uc_ESPION_ON)
	{
		xxx = (unsigned long)_mem_get_highwater();
		printf("HHHHHHHHH\t\tHighwater = 0x%lx -> + %d\n\n", xxx, xxx-xxxprecedent);
		xxxprecedent = xxx;
	}
}

// Tache Main : executee en permanence dès le démarrage de MQX
void Main_task(uint_32 initial_data)
{
	unsigned char l_uc_Compteur;
	unsigned char * UTF16LE;
	unsigned char * UTF16LEprecedent;
	static unsigned char compteurxxx = 0;
	static unsigned char compteurstack = 0;
	
	_mem_size stack_size;
	_mem_size stack_used;
	_mqx_uint return_value;


	_time_delay(500);
	EtatMemoire();
	//xxxxx[0]++;
	
//	printf("%d\n", START_OF_USER_HEAP);
//	printf("%d\n", END_OF_USER_HEAP);
//	printf("%d\n", START_OF_USER_HEAP);
//	printf("%d\n", BSP_DEFAULT_START_OF_USER_HEAP);
//	printf("%d\n", __sfb( "USER_HEAP"));
//	printf("%d\n", END_OF_USER_HEAP);
//	printf("%d\n", BSP_DEFAULT_END_OF_USER_HEAP);
//	printf("%d\n", __USER_AREA_END);
	
	/*while(1)
	{
		UTF16LE=malloc(1000);
		_time_delay(500);
		compteurxxx++;
		printf("%d - %x -> %d\n", compteurxxx, UTF16LE, UTF16LE - UTF16LEprecedent);
		UTF16LEprecedent = UTF16LE;

		_klog_get_interrupt_stack_usage(&stack_size, &stack_used);
		printf("Interrupt stack size: 0x%x, Stack used: 0x%x\n", stack_size, stack_used);
		/* Get stack usage for this task: */
		//_klog_get_task_stack_usage(_task_get_id(), &stack_size, &stack_used);
		//printf("Task ID: 0x%lx, Stack size: 0x%x, Stack used: 0x%x\n", _task_get_id(), stack_size, stack_used);
		/* Display all stack usage: */
		/*_klog_show_stack_usage();
		
		printf("\n Highwater = 0x%lx\n\n", _mem_get_highwater());
	}*/
	
	// A faire en tout premier pour autoriser les vd_EspionRS_Printf
#ifdef DEBUG
	vd_EspionRS_Init();
#endif
	uc_EspionTachePrincipaleEtat = 1;
	
	
	vd_TableEchangeLireEnFlash();	// Init table échange - A FAIRE AVANT LECTURE EEPROM ADRESSE MAC (CONTIENT CLE SERVEUR + CODE ALARME)
	
	// printf : pour les avoir tout le temps meme en mode release
    printf("ESSENSYS START\n");
    printf("Version BP : %d\n", uc_VERSION_BP_SOFT_SAVE);
    printf("Version Serveur : %d\n", us_BP_VERSION_SERVEUR);
    printf("Table d'echange : %d\n", uc_VERSION_TABLE_ECHANGE);

    printf("OS RESOLUTION : %d ms\n\n",_time_get_resolution());
   
    // Init des objets MQX et du micro (PWM, ADC, I/O)
    uc_EspionTachePrincipaleEtat = 2;
    vd_InitVariablesGlobales();
    
    uc_EspionTachePrincipaleEtat = 3;
	vd_InitHard();
	
	// Ouverture port SPI pour communication avec les EEPROM
	uc_EspionTachePrincipaleEtat = 33;
	vd_SpiOpen();
	uc_EspionTachePrincipaleEtat = 34;
	vd_SpiSelectEEPROMAdresseMac();
	uc_EspionTachePrincipaleEtat = 35;
	vd_ReadAdresseMac();	// Lecture adresse MAC
	uc_EspionTachePrincipaleEtat = 36;
	vd_ReadCleServeur();	// Lecture clé serveur dans EEPROM
	uc_EspionTachePrincipaleEtat = 37;
	// Recopie valeur clé serveur dans table d'échange
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
	{
		Tb_Echange[Cle_Acces_Distance + l_uc_Compteur] = uc_Cle_Acces_Distance[l_uc_Compteur];
	}
	uc_EspionTachePrincipaleEtat = 38;
	vd_ReadCodeAlarme();
	

	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"AVANT DELAY\n");
	
	_time_delay(1000);

	//xxx forcer au demarrage -> à couper si VBat faible
	lwgpio_set_value(&IO_DOUT_ControleBatterie, LWGPIO_VALUE_HIGH);
	lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_HIGH);

	uc_EspionTachePrincipaleEtat = 4;
    _time_delay(100);		//	positionnement des entrées

//	xxx init de toutes les variables : global.h / echanges.h
//	lecture des paramètres sauvegardé en flash : Tb_Echange
//	if (addr_control_flash == 0x55AA)
//	{
//	// lecture de la flash
//	// xxx 
//	}
//	else
//	{
//	init par défaut des paramètres normalement sauvegardés en flash
//	vd_Init_Echange ();
//	}

	// Init de l'applicatif
	uc_EspionTachePrincipaleEtat = 5;
	vd_Init_Horloge();
	vd_rtc();
	uc_EspionTachePrincipaleEtat = 6;
	vd_Chauffage();			// appel à l'init pour initialiser les valeurs chauffage
	uc_EspionTachePrincipaleEtat = 8;
	vd_AlarmeInit();
	uc_EspionTachePrincipaleEtat = 9;
	vd_Timer50ms_Init();
	vd_Timer1sec_Init();
	
	// Tout est initialisé et pret -> démarrer les autres taches de l'application
	uc_EspionTachePrincipaleEtat = 10;
	EtatMemoire();
	vd_StartTacheEcran();
	EtatMemoire();
	uc_EspionTachePrincipaleEtat = 11;
	vd_StartTacheBA();
	EtatMemoire();
	uc_EspionTachePrincipaleEtat = 12;
	vd_StartTacheTeleinfo();
	EtatMemoire();
	uc_EspionTachePrincipaleEtat = 13;
	vd_StartTacheEthernet();
	EtatMemoire();
	

	// Boucle infinie
	uc_EspionTachePrincipaleEtat = 14;

	
	for (;;)
	{
	   ul_EspionTachePrincipaleCompteurActivite++;
	   
	   // Traitement à faire que si aucune erreur détectée (warning ignorées)
	   // mais faire dans tous les cas EspionRS pour diagnostic
	   uc_EspionTachePrincipaleEtat = 20;
	   //if(uc_ErreurDetectee() == 0)
	   {
		   uc_EspionTachePrincipaleEtat = 21;
		   vd_rtc();	// Met à jour date et heure pour l'applicatif
		   uc_EspionTachePrincipaleEtat = 22;
	   
		   vd_VerifierDateRetourVacances();
		   
		   if(uc_CadencementTraitement_50ms >= 2)
		   {
				uc_CadencementTraitement_50ms = 0;
				vd_ADCAcquisition();	// Avant était appelé chaque seconde -> appelé maintenant 100 ms pour détection présence batterie
				vd_VerifPrecenceBatterie();
				//vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"ACQ AIN\n");
		   }
		   
		   // Traitement chaque seconde xxx a remplacer par timer ??? ou autre tache ???
		   if(uc_SecondePrecedente != st_DateHeure.SECOND)
		   {
				uc_SecondePrecedente = st_DateHeure.SECOND;
				
				uc_EspionTachePrincipaleEtat = 23;
				vd_SurveillanceAlimentations();
				vd_GestionCoupureAlimentation();
				
				if(uc_CompteurDesactivationEspionAuto_sec < uc_TEMPO_DESACTIVATION_AUTO_sec)	uc_CompteurDesactivationEspionAuto_sec++;
				if(uc_CompteurEspionAIN_sec < uc_TEMPO_ESPION_AIN_sec)							uc_CompteurEspionAIN_sec++;
				if(uc_TempoStartAINMinMax_sec < uc_TEMPO_START_AIN_MIN_MAXsec)					uc_TempoStartAINMinMax_sec++;

				if(us_CompteurPilotageSireneInterieure_sec < us_DUREE_PILOTAGE_SIRENE_INTERIEURE_sec)	us_CompteurPilotageSireneInterieure_sec++;
				if(us_CompteurPilotageSireneExterieure_sec < us_DUREE_PILOTAGE_SIRENE_EXTERIEURE_sec)	us_CompteurPilotageSireneExterieure_sec++;
				
				// Espions FIL PILOTE
				ul_EspionCompteurITAlternanceSecteurMain = ul_EspionCompteurITAlternanceSecteur;
				ul_EspionCompteurITTimerFilPilote1Main = ul_EspionCompteurITTimerFilPilote1;
				ul_EspionCompteurITTimerFilPilote2Main = ul_EspionCompteurITTimerFilPilote2;
				vd_EspionRS_Printf(uc_ESPION_FIL_PILOTE,"FP IT : %d -> %d\n", ul_EspionCompteurITAlternanceSecteurMain, ul_EspionCompteurITAlternanceSecteurMain - ul_EspionCompteurITAlternanceSecteurMainPrecedent);
				vd_EspionRS_Printf(uc_ESPION_FIL_PILOTE,"FP TIMER 1 : %d -> %d\n", ul_EspionCompteurITTimerFilPilote1Main, ul_EspionCompteurITTimerFilPilote1Main - ul_EspionCompteurITTimerFilPilote1MainPrecedent);
				vd_EspionRS_Printf(uc_ESPION_FIL_PILOTE,"FP TIMER 2 : %d -> %d\n", ul_EspionCompteurITTimerFilPilote2Main, ul_EspionCompteurITTimerFilPilote2Main - ul_EspionCompteurITTimerFilPilote2MainPrecedent);
				ul_EspionCompteurITAlternanceSecteurMainPrecedent = ul_EspionCompteurITAlternanceSecteurMain;
				ul_EspionCompteurITTimerFilPilote1MainPrecedent = ul_EspionCompteurITTimerFilPilote1Main;
				ul_EspionCompteurITTimerFilPilote2MainPrecedent = ul_EspionCompteurITTimerFilPilote2Main;
				
				vd_GestionFilPilote();
			   
				//xxx forcage EVArrosage pour validation pilotage coupure sous IT FP et timer
				/*if(Tb_Echange[Arrose_Mode] == 0)				Tb_Echange[Arrose_Mode] = 10;
				else											Tb_Echange[Arrose_Mode] = 0;*/
				
				vd_AnemoGestion();	//Gestion anémomètre chaque seconde

				compteurstack++;
				if(compteurstack > 30)
				{
					compteurstack = 0;

					if(vd_EspionMainActive() == uc_ESPION_ON)
					{
						_klog_get_interrupt_stack_usage(&stack_size, &stack_used);
						printf("Interrupt stack size: 0x%x, Stack used: 0x%x\n", stack_size, stack_used);
						/* Get stack usage for this task: */
						_klog_get_task_stack_usage(_task_get_id(), &stack_size, &stack_used);
						printf("Task ID: 0x%lx, Stack size: 0x%x, Stack used: 0x%x\n", _task_get_id(), stack_size, stack_used);
						/* Display all stack usage: */
						_klog_show_stack_usage();
						
						//EtatMemoire();
						
						printf("\n\nMUTEX :\n");
						printf("\t\t ul_CompteurMutexEnvoyerTrameStatus : %d\n", ul_CompteurMutexEnvoyerTrameStatus);
						printf("\t\t ul_CompteurMutexEnvoyerTrameStatusErreur : %d\n", ul_CompteurMutexEnvoyerTrameStatusErreur);
						printf("\t\t ul_CompteurMutexTacheEcran : %d\n", ul_CompteurMutexTacheEcran);
						printf("\t\t ul_CompteurMutexTacheEcranErreur : %d\n", ul_CompteurMutexTacheEcranErreur);
						printf("\t\t ul_CompteurMutexRTC : %d\n", ul_CompteurMutexRTC);
						printf("\t\t ul_CompteurMutexRTCErreur : %d\n", ul_CompteurMutexRTCErreur);
						printf("\n\n");
					}
				}
		   }
		   
		   if(uc_JourSemaine != uc_JourSemainePrecedent)
		   {
			   uc_JourSemainePrecedent = uc_JourSemaine;
			   uc_UpdateHCHC_HCHP = 1;	// Changement de jour -> mémorisation HCHC / HCHP
		   }
		   
		   // Désactivation du réveil pour les pièces dont la fonction est désactivée
		   // Voir échanges emails "pb reveil" datant de 2015/09
		   // Le scénario "je vais me coucher" active pour TOUTES les pièces le réveil sans regarder si Tb_Echange[Reveil_xxx_ON] != 0
		   // Et c'est la fonction vd_Reveil() qui va désactiver le réveil pour les pièces dont Tb_Echange[Reveil_xxx_ON] == 0
		   // Sauf que cette fonction n'est appelée qu'au changement de minute
		   // Et donc si dans une même minute, on fait un "je vais me coucher" et "je me lève", aucun volet ne s'ouvrira même si Tb_Echange[Reveil_xxx_ON] == 0
		   // Donc on va forcer ICI et tout le temps le réveil à 0 pour les pièces où la fonction est désactivée
		   // On aurait pu faire ce traitement dans le traitement du scénario mais là c'est fait tout le temps
		   // Ce qui permettrait à l'écran de désactiver un réveil pour une pièce sans lancer de scénario
		   // Par contre l'activation du réveil se fait toujours par scénario
		   // A noter que vd_Reveil() pourrait être aussi appelé tout le temps, mais on laisse comme ca histoire de ne pas avoir à refaire des tests complets xxx à faire en plus mieux bine par la suite...
		   if(Tb_Echange[Reveil_ChambreGr_ON] == 0)	st_Reveil_Arme.uc_ChGr = 0;
		   if(Tb_Echange[Reveil_Chambre1_ON] == 0)	st_Reveil_Arme.uc_Ch1 = 0;
		   if(Tb_Echange[Reveil_Chambre2_ON] == 0)	st_Reveil_Arme.uc_Ch2 = 0;
		   if(Tb_Echange[Reveil_Chambre3_ON] == 0)	st_Reveil_Arme.uc_Ch3 = 0;
		   if(Tb_Echange[Reveil_Bureau_ON] == 0)	st_Reveil_Arme.uc_Bur = 0;
			   
		   
		   // Traitement chaque changement de minute
		   if(uc_MinutePrecedente != st_DateHeure.MINUTE)	// xxx recalculer aussi si changement date / heure -> uc_ModeChauffageModifieParEcranOuScenario = 1 
		   {
			   uc_MinutePrecedente = st_DateHeure.MINUTE;
		   
			   uc_EspionTachePrincipaleEtat = 24;
			   vd_Reveil();
			   vd_RetourVacances();
			   uc_EspionTachePrincipaleEtat = 25;
			   uc_ModeChauffageModifieParEcranOuScenario = 1;	//xxx a faire aussi en acces internet
		   }
		   if(uc_ModeChauffageModifieParEcranOuScenario != 0)	//xxx rajouter mutex
		   {
			   uc_ModeChauffageModifieParEcranOuScenario = 0;
			   vd_Chauffage();
			   uc_EspionTachePrincipaleEtat = 26;
		   }
		   
		   uc_EspionTachePrincipaleEtat = 27;
		   vd_Alarme();			// traitement de l'alarme xxx a faire tout le temps ???
		   vd_DetectionFuitesEau();
		   vd_GestionScenario();

		   vd_UpdateOrdresPilotageSortiesBA();
		   uc_EspionTachePrincipaleEtat = 28;
		   vd_PiloterSorties();	//xxx si defaut executer ? tout couper ? pilotage sirene a faire tout le temps ?
		   
		   vd_MAJEtatBPDansTableEchange();
		   
		   // Mémorisation clé serveur en EEPROM externe si changement valeur
		   // Traitement à faire si Download pas en cours... xxx mutex
		   if(uc_DownloadEnCours == 0)
		   {
			   vd_UpdateCleServeurDansEEPROM();
		   }
	   }
	   uc_EspionTachePrincipaleEtat = 29;
#ifdef DEBUG
	   vd_EspionRS();
#endif
	    
	   uc_EspionTachePrincipaleEtat = 30;
       _sched_yield();		// passe la main à la tache suivante
       
   }	// fin boucle for infinie
   uc_EspionTachePrincipaleEtat = 50;
   
   vd_Close_PWM ();
   vd_Close_ADC ();
   
   _msgpool_destroy(message_pool);
}

// Init par défaut de la table d'échange
void vd_Init_Echange(void)
{
	unsigned short l_us_Compteur;
	
	// Tout à 0
	for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees-1;l_us_Compteur++)
	{
		Tb_Echange[l_us_Compteur] = 0;
	}
	
	// Puis init pour certains
	Tb_Echange[Version_SoftBP_Embedded] = uc_VERSION_BP_SOFT_SAVE;
	Tb_Echange[Version_SoftBP_Web] = us_BP_VERSION_SERVEUR;
	Tb_Echange[Version_TableEchange] = uc_VERSION_TABLE_ECHANGE;
	
	// Valeurs par défaut (email NG 14/11/2013 04/03/2014)
	//Chauffage zone jour : tout à confort
	//Chauffage zone nuit : tout à confort
	//Chaufage SDB1 : tout à confort
	//Chauffage SDB2 : tout à confort
	//Arrosage : tout à OFF
	for(l_us_Compteur = 0; l_us_Compteur < uc_PLANNING_CHAUFFAGE_TAILLE; l_us_Compteur++)
	{
		Tb_Echange[Chauf_zj_Auto + l_us_Compteur] = 0x11;	// Confort par défaut
		Tb_Echange[Chauf_zn_Auto + l_us_Compteur] = 0x11;
		Tb_Echange[Chauf_zsb1_Auto + l_us_Compteur] = 0x11;
		Tb_Echange[Chauf_zsb2_Auto + l_us_Compteur] = 0x11;
	}

	Tb_Echange[VacanceFin_J] = 1; 
	Tb_Echange[VacanceFin_M] = 1; 
	
	Tb_Echange[VacanceFin_zj_Force] = 0x11;
	Tb_Echange[VacanceFin_zn_Force] = 0x11;
	Tb_Echange[VacanceFin_zsb1_Force] = 0x80;
	Tb_Echange[VacanceFin_zsb2_Force] = 0x80;

	Tb_Echange[Alarme_CodeSaisiLSB] = 0xFF;
	Tb_Echange[Alarme_CodeSaisiMSB] = 0xFF;
	
	Tb_Echange[AlarmeConfig+AlarmeConfig_Code] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_Detect1] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_Detect2] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_DetectOuv] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_Detect1SurVoieAcces] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_Detect2SurVoieAcces] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_DetectOuvSurVoieAcces] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_SireneInt] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_SireneExt] = 1;
	Tb_Echange[AlarmeConfig+AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[AlarmeConfig+AlarmeConfig_ForcerEclairage] = 0;

	Tb_Echange[Alerte_Intensite] = 50;
	Tb_Echange[Alerte_Duree] = 30;

	Tb_Echange[Securite_FuiteLinge] = 1;
	Tb_Echange[Securite_FuiteVaisselle] = 1;
	Tb_Echange[Securite_FuiteAlerte] = 1;
	
	Tb_Echange[Reveil_ChambreGr_H] = 7;
	Tb_Echange[Reveil_ChambreGr_Mn] = 0;
	Tb_Echange[Reveil_ChambreGr_ON] = 0;
	Tb_Echange[Reveil_Chambre1_H] = 7;
	Tb_Echange[Reveil_Chambre1_Mn] = 0;
	Tb_Echange[Reveil_Chambre1_ON] = 0;
	Tb_Echange[Reveil_Chambre2_H] = 7;
	Tb_Echange[Reveil_Chambre2_Mn] = 0;
	Tb_Echange[Reveil_Chambre2_ON] = 0;
	Tb_Echange[Reveil_Chambre3_H] = 7;
	Tb_Echange[Reveil_Chambre3_Mn] = 0;
	Tb_Echange[Reveil_Chambre3_ON] = 0;
	Tb_Echange[Reveil_Bureau_H] = 7;
	Tb_Echange[Reveil_Bureau_Mn] = 0;
	Tb_Echange[Reveil_Bureau_ON] = 0;
	Tb_Echange[Delestage] = 1;
	
	Tb_Echange[TeleInf_Repartition_Chauffage] = 21;
	Tb_Echange[TeleInf_Repartition_Refroid] = 0;
	Tb_Echange[TeleInf_Repartition_EauChaude] = 11;
	Tb_Echange[TeleInf_Repartition_Prises] = 6;
	Tb_Echange[TeleInf_Repartition_Autres] = 62;
	
	for(l_us_Compteur = 0;l_us_Compteur < uc_NB_VARIATEURS_POSSIBLES_PAR_BA;l_us_Compteur++)
	{
		Tb_Echange[Variateurs_PDV_Conf + l_us_Compteur] = 0x01;
		Tb_Echange[Variateurs_CHB_Conf + l_us_Compteur] = 0x01;
		Tb_Echange[Variateurs_PDE_Conf + l_us_Compteur] = 0x01;
	}

	for(l_us_Compteur = 0;l_us_Compteur < uc_NB_VOLETS_POSSIBLES_PAR_BA;l_us_Compteur++)
	{
		Tb_Echange[Volets_PDV_Temps + l_us_Compteur] = 120;
		Tb_Echange[Volets_CHB_Temps + l_us_Compteur] = 120;
		Tb_Echange[Volets_PDE_Temps + l_us_Compteur] = 120;
	}
	
	// Store : mettre au maxi par défaut :
	Tb_Echange[Volets_PDE_Temps + 3] = 255;

	vd_Scenario_Init(Scenario1, uc_SCENARIO_SERVEUR);
	vd_Scenario_Init(Scenario2, uc_SCENARIO_JE_SORS);
	vd_Scenario_Init(Scenario3, uc_SCENARIO_JE_PARS_EN_VACANCES);
	vd_Scenario_Init(Scenario4, uc_SCENARIO_JE_RENTRE);
	vd_Scenario_Init(Scenario5, uc_SCENARIO_JE_ME_COUCHE);
	vd_Scenario_Init(Scenario6, uc_SCENARIO_JE_ME_LEVE);
	vd_Scenario_Init(Scenario7, uc_SCENARIO_PERSO);
	vd_Scenario_Init(Scenario8, uc_SCENARIO_LIBRE);

	Tb_Echange[Constructeur_CodeLSB] = 0x11;
	Tb_Echange[Constructeur_CodeMSB] = 0x19;
}

void vd_InitVariablesGlobales(void)
{
	_mqx_uint l_u_result;
	MUTEX_ATTR_STRUCT l_mutexattr;
	unsigned short l_us_Compteur;

	
	uc_VersionHard = 0;
	
	uc_CadencementTraitement_50ms = 0;
	uc_CompteurLED_50ms = 0;
	
	uc_PortSPIouvert = 0;
	
	// Initialisé par vd_rtc :
	// st_DateHeure
	// st_DateHeurePourEcran
	// uc_JourSemaine
	uc_JourSemainePrecedent = 0xFF;	// Forcer le calcul au démarrage

	st_DateHeureEnvoyeeParEcran.MINUTE = 0xFF;
	st_DateHeureEnvoyeeParEcran.HOUR = 0xFF;
	st_DateHeureEnvoyeeParEcran.DAY = 0xFF;
	st_DateHeureEnvoyeeParEcran.MONTH = 0xFF;
	st_DateHeureEnvoyeeParEcran.YEAR = 0xFF;

	l_u_result = _lwsem_create(&lwsem_wr_date, 0);
	if(l_u_result != MQX_OK)
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_CREATION_SEMAPHORE_DATE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"erreur creation lwsem_wr_date : 0x%X", l_u_result);
	}

	uc_FlagMajDate = 0;
	uc_MinutePrecedente = 0xFF;		// Force le traitement au démarrage
	uc_SecondePrecedente = 0xFF;	// FOrce le traitement au démarrage

	st_EchangeStatus.uc_AlarmeActivee = 0;
	st_EchangeStatus.uc_AlarmeDeclenchee = 0;
	st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 0;
	st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
	st_EchangeStatus.uc_EcranTimeOUT = 0;
	st_EchangeStatus.uc_DinEcranOuvert = 0;
	st_EchangeStatus.uc_SurConsommation = 0;
	st_EchangeStatus.uc_HeuresCreuses = 0;
	st_EchangeStatus.uc_Secouru = 0;
	st_EchangeStatus.uc_FuiteLL = 0;
	st_EchangeStatus.uc_FuiteLV = 0;
	st_EchangeStatus.uc_DefTeleinfo = 0;
	st_EchangeStatus.uc_Defboitier1 = 0;
	st_EchangeStatus.uc_Defboitier2 = 0;
	st_EchangeStatus.uc_Defboitier3 = 0;
	st_EchangeStatus.uc_BloquerVolets = 0;
	st_EchangeStatus.uc_ForcerAllumage = 0;
	st_EchangeStatus.uc_SécuOFF = 0;
	st_EchangeStatus.uc_MachinesOFF = 0;
	st_EchangeStatus.uc_EtatEthernet = 0;

	st_EchangeStatusPrecedent.uc_AlarmeActivee = 0;
	st_EchangeStatusPrecedent.uc_AlarmeDeclenchee = 0;
	st_EchangeStatusPrecedent.uc_AlarmeProcedureSortieEnCours = 0;
	st_EchangeStatusPrecedent.uc_AlarmeProcedureRentreeEnCours = 0;
	st_EchangeStatusPrecedent.uc_EcranTimeOUT = 0;
	st_EchangeStatusPrecedent.uc_DinEcranOuvert = 0;
	st_EchangeStatusPrecedent.uc_SurConsommation = 0;
	st_EchangeStatusPrecedent.uc_HeuresCreuses = 0;
	st_EchangeStatusPrecedent.uc_Secouru = 0;
	st_EchangeStatusPrecedent.uc_FuiteLL = 0;
	st_EchangeStatusPrecedent.uc_FuiteLV = 0;
	st_EchangeStatusPrecedent.uc_DefTeleinfo = 0;
	st_EchangeStatusPrecedent.uc_Defboitier1 = 0;
	st_EchangeStatusPrecedent.uc_Defboitier2 = 0;
	st_EchangeStatusPrecedent.uc_Defboitier3 = 0;
	st_EchangeStatusPrecedent.uc_BloquerVolets = 0;
	st_EchangeStatusPrecedent.uc_ForcerAllumage = 0;
	st_EchangeStatusPrecedent.uc_SécuOFF = 0;
	st_EchangeStatusPrecedent.uc_MachinesOFF = 0;
	st_EchangeStatusPrecedent.uc_EtatEthernet = 0;

	uc_EcranTimeOUT = 0;
	uc_TrameSynchroEtat = 0;
	
	uc_EtatChauffagePourEcran_ZJ = 0;
	uc_EtatChauffagePourEcran_ZN = 0;
	uc_EtatChauffagePourEcran_Zsdb1 = 0;
	uc_EtatChauffagePourEcran_Zsdb2 = 0;
	
	uc_ModeChauffageModifieParEcranOuScenario = 0;

	st_chauf_ZJ.uc_Mode = uc_CHAUFFAGE_CONFORT;	// Confort car aucun signal en sortie...
	st_chauf_ZJ.uc_ConsigneFilPiloteEnCours = uc_CHAUFFAGE_CONFORT;
	st_chauf_ZJ.uc_DelestageActif = 0xFF;
	st_chauf_ZJ.uc_ConsigneFilPiloteNonAnticipe = uc_CHAUFFAGE_CONFORT;
	st_chauf_ZJ.uc_HeurePassageAnticipe = 0;
	st_chauf_ZN.uc_Mode = uc_CHAUFFAGE_CONFORT;	// Confort car aucun signal en sortie...
	st_chauf_ZN.uc_ConsigneFilPiloteEnCours = uc_CHAUFFAGE_CONFORT;
	st_chauf_ZN.uc_DelestageActif = 0xFF;
	st_chauf_ZN.uc_ConsigneFilPiloteNonAnticipe = uc_CHAUFFAGE_CONFORT;
	st_chauf_ZN.uc_HeurePassageAnticipe = 0;
	st_chauf_Zsdb1.uc_Mode = uc_CHAUFFAGE_CONFORT;	// Confort car aucun signal en sortie...
	st_chauf_Zsdb1.uc_ConsigneFilPiloteEnCours = uc_CHAUFFAGE_CONFORT;
	st_chauf_Zsdb1.uc_DelestageActif = 0xFF;
	st_chauf_Zsdb1.uc_ConsigneFilPiloteNonAnticipe = uc_CHAUFFAGE_CONFORT;
	st_chauf_Zsdb1.uc_HeurePassageAnticipe = 0;
	st_chauf_Zsdb2.uc_Mode = uc_CHAUFFAGE_CONFORT;	// Confort car aucun signal en sortie...
	st_chauf_Zsdb2.uc_ConsigneFilPiloteEnCours = uc_CHAUFFAGE_CONFORT;
	st_chauf_Zsdb2.uc_DelestageActif = 0xFF;
	st_chauf_Zsdb2.uc_ConsigneFilPiloteNonAnticipe = uc_CHAUFFAGE_CONFORT;
	st_chauf_Zsdb2.uc_HeurePassageAnticipe = 0;
	
	uc_ConsigneFilPilotePourIT_ZJ = uc_CHAUFFAGE_CONFORT;	// uc_CHAUFFAGE_CONFORT : pas de signal en sortie
	uc_ConsigneFilPilotePourIT_ZN = uc_CHAUFFAGE_CONFORT;
	uc_ConsigneFilPilotePourIT_SDB1 = uc_CHAUFFAGE_CONFORT;
	uc_ConsigneFilPilotePourIT_SDB2 = uc_CHAUFFAGE_CONFORT;
	
	uc_EcoEnCours_ZJ = 0;
	uc_EcoEnCours_ZN = 0;
	uc_EcoEnCours_SDB1 = 0;
	uc_EcoEnCours_SDB2 = 0;
	
	uc_EcoPlusEnCours_ZJ = 0;
	uc_EcoPlusEnCours_ZN = 0;
	uc_EcoPlusEnCours_SDB1 = 0;
	uc_EcoPlusEnCours_SDB2 = 0;
	
	uc_CompteurSecondesEcoEtEcoPlus_ZJ = 0;
	uc_CompteurSecondesEcoEtEcoPlus_ZN = 0;
	uc_CompteurSecondesEcoEtEcoPlus_SDB1 = 0;
	uc_CompteurSecondesEcoEtEcoPlus_SDB2 = 0;
	
	uc_RAZCompteurSecondesEcoEtEcoPlus_ZJ = 0;
	uc_RAZCompteurSecondesEcoEtEcoPlus_ZN = 0;
	uc_RAZCompteurSecondesEcoEtEcoPlus_SDB1 = 0;
	uc_RAZCompteurSecondesEcoEtEcoPlus_SDB2 = 0;
	
	uc_CompteurITModeHGForce_ZJ = 0;
	uc_CompteurITModeHGForce_ZN = 0;
	uc_CompteurITModeHGForce_SDB1 = 0;
	uc_CompteurITModeHGForce_SDB2 = 0;
	
	uc_ConsigneFilPilotePourITPrecedent_ZJ = 0;
	uc_ConsigneFilPilotePourITPrecedent_ZN = 0;
	uc_ConsigneFilPilotePourITPrecedent_SDB1 = 0;
	uc_ConsigneFilPilotePourITPrecedent_SDB2 = 0;
		
	uc_Chauf_zj_Mode_EtatLorsDernierScenario = 0;
	uc_Chauf_zn_Mode_EtatLorsDernierScenario = 0;
	uc_Chauf_zsb1_Mode_EtatLorsDernierScenario = 0;
	uc_Chauf_zsb2_Mode_EtatLorsDernierScenario = 0;
	uc_Cumulus_Mode_EtatLorsDernierScenario = 0;

	st_Reveil_Arme.uc_ChGr = 0;
	st_Reveil_Arme.uc_Ch1 = 0;
	st_Reveil_Arme.uc_Ch2 = 0;
	st_Reveil_Arme.uc_Ch3 = 0;
	st_Reveil_Arme.uc_Bur = 0;
	
	// Initialisé par Boitiers_task
	// fd_BA_I2C
	// lock_I2C
	
	for(l_us_Compteur = 0;l_us_Compteur < us_Nb_Tbb_Donnees_BA;l_us_Compteur++)
	{
		us_Tb_Echangeboitier[l_us_Compteur] = 0;
	}

	// Creation "message pool"
	message_pool = _msgpool_create(sizeof(COMMANDE_MESSAGE), 5, 0, 0);	// 5 messages
	if(message_pool == 0)
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_CREATION_MESSAGE_POOL;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"erreur création message_pool!\n");
	}

	// Initialisé par vd_AlarmeInit
	// uc_AlarmeConfigEnCours
	// uc_AlarmeModePrecedent
	// TimerAlarmeID
	// uc_AlarmeCompteurSecondes
	// uc_Alarme_NouveauCode_1_LSB
	// uc_Alarme_NouveauCode_1_MSB
	// uc_Alarme_NouveauCode_2_LSB
	// uc_Alarme_NouveauCode_2_MSB
	// uc_CompteurDetect_Ouv_50ms
	// uc_CompteurDetect_Pres1_50ms
	// uc_CompteurDetect_Pres2_50ms
	// uc_AlarmeDetection	
	// uc_AlarmeFraude

	// Espions & erreurs -> vd_EspionRS_Init
	
	// Initialize mutex attributes :
	if(_mutatr_init(&l_mutexattr) != MQX_OK)
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ATTRIBUTS;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB init mutex\n");
	}
	else
	{
		// Initialize the mutex :
		if(_mutex_init(&st_Mutex_ZJ, &l_mutexattr) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZJ;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB Init st_Mutex_ZJ\n");
		}
		if(_mutex_init(&st_Mutex_ZN, &l_mutexattr) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_ZN;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB Init st_Mutex_ZN\n");
		}
		if(_mutex_init(&st_Mutex_Zsdb1, &l_mutexattr) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB1;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB Init st_Mutex_SDB1\n");
		}
		if(_mutex_init(&st_Mutex_Zsdb2, &l_mutexattr) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_SDB2;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB Init st_Mutex_SDB2\n");
		}

		if(_mutex_init(&st_Mutex_DateHeurePourEcran, &l_mutexattr) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_PRINCIPALE_INIT_MUTEX_DATE_HEURE_POUR_ECRAN;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"PB Init st_Mutex_DateHeurePourEcran\n");
		}
	}
	
	// Initialisés par vd_Init_ADC
	// f_adc
	// f_adc_Fuite1
	// f_adc_Fuite2	
	
	// Initialisés par vd_Init_PWM
	// fdpwm
	// pwm_pin_data
	
	// I/O -> initialisées par vd_Init_IO()
		
	ul_I2C_While_1 = 0;
	ul_I2C_While_1Max = 0;
	ul_I2C_While_1TimeOut = 0;
	ul_I2C_While_2 = 0;
	ul_I2C_While_2Max = 0;
	ul_I2C_While_2TimeOut = 0;
	ul_I2C_While_3 = 0;
	ul_I2C_While_3Max = 0;
	ul_I2C_While_3TimeOut = 0;
	ul_I2C_While_4 = 0;
	ul_I2C_While_4Max = 0;
	ul_I2C_While_4TimeOut = 0;
	ul_I2C_While_5 = 0;
	ul_I2C_While_5Max = 0;
	ul_I2C_While_5TimeOut = 0;
	uc_FlagBlocageDetecte = 0;
	uc_FlagReinitialiserI2C = 0;

	for(l_us_Compteur = 0;l_us_Compteur < uc_VBAT_MOYENNE_GLISSANTE;l_us_Compteur++)
	{
		us_VBat[l_us_Compteur] = 0;
	}
	
	for(l_us_Compteur = 0;l_us_Compteur < sizeof(uc_AdresseMac);l_us_Compteur++)
	{
		uc_AdresseMac[l_us_Compteur] = 0;
	}
	
	uc_DownloadEnCours = 0;
	uc_DownloadEnCoursBloquerI2CVersBA = 0;
	uc_ResetDemande = 0;
	
	for(l_us_Compteur = 0;l_us_Compteur < sizeof(uc_Cle_Acces_Distance);l_us_Compteur++)
	{
		uc_Cle_Acces_Distance[l_us_Compteur] = 0;
	}
	
	uc_DernierScenarioLance = 0;
	uc_Cumulus_ForcerAOnAuRetourVacancesProgramme = 0;
	uc_ChauffageModifieDepuisServeur_zj = 0;
	uc_ChauffageModifieDepuisServeur_zn = 0;
	uc_ChauffageModifieDepuisServeur_zsb1 = 0;
	uc_ChauffageModifieDepuisServeur_zsb2 = 0;
	uc_CumulusModifieDepuisServeur = 0;

	uc_CouperEVArrosage = 0;
	
	us_TeleinfoCadencementLed = 0;
	us_TeleinfoTimeOUT = us_TELEINFO_TIME_OUT;
	uc_TeleinfoOptionTarifaire = uc_OPT_TARIF_NON_RENSEIGNE;
	us_TeleinfoTimeOUTOptionTarifaire = us_TELEINFO_OPTION_TARIFAIRE_TIME_OUT;
	uc_TeleinfoPeriodeTarifaire = uc_TARIF_NON_RENSEIGNE;
	us_TeleinfoTimeOUTPeriodeTarifaire = us_TELEINFO_PERIODE_TARIFAIRE_TIME_OUT;
	ul_TeleinfoPuissanceApp = 0;
	us_TeleinfoTimeOUTPuissanceApp = us_TELEINFO_PUISSANCE_APP_TIME_OUT;
	
	ul_TeleinfoHCHC = 0;
	ul_TeleinfoHCHCPrecedent = 0;
	us_TeleinfoTimeOUTHCHC = us_TELEINFO_HCHC_APP_TIME_OUT;
	
	ul_TeleinfoHCHP_OU_BASE = 0;
	ul_TeleinfoHCHP_OU_BASEPrecedent = 0;
	us_TeleinfoTimeOUTHCHP_OU_BASE = us_TELEINFO_HCHP_OU_BASE_APP_TIME_OUT;
	
	us_TeleInf_HPB_Global_sans_div_10 = uc_HCHC_HCHP_TIME_OUT_SHORT;
	us_TeleInf_HC_Global_sans_div_10 = uc_HCHC_HCHP_TIME_OUT_SHORT;
	us_TeleInf_HPB_Global_avec_div_10 = uc_HCHC_HCHP_TIME_OUT_SHORT;
	us_TeleInf_HC_Global_avec_div_10 = uc_HCHC_HCHP_TIME_OUT_SHORT;
	
	uc_UpdateHCHC_HCHP = 0;
	
	st_AIN4_VBat.uc_NumeroAIN = 4;
	st_AIN4_VBat.us_AINBrut = 0;
	st_AIN4_VBat.us_AINMin = 0xFFFF;
	st_AIN4_VBat.us_AINMax = 0;

	st_AIN5_FuiteLV.uc_NumeroAIN = 5;
	st_AIN5_FuiteLV.us_AINBrut = 0;
	st_AIN5_FuiteLV.us_AINMin = 0xFFFF;
	st_AIN5_FuiteLV.us_AINMax = 0;

	st_AIN6_FuiteLL.uc_NumeroAIN = 6;
	st_AIN6_FuiteLL.us_AINBrut = 0;
	st_AIN6_FuiteLL.us_AINMin = 0xFFFF;
	st_AIN6_FuiteLL.us_AINMax = 0;
	
	uc_CompteurCheckMdpEsion = 0;
	uc_EspionActive = 0;
	uc_CompteurDesactivationEspionAuto_sec = 0;
	uc_CompteurEspionAIN_sec = 0;
	uc_TempoStartAINMinMax_sec = 0;

	us_CompteurPilotageSireneInterieure_sec = 0;
	us_CompteurPilotageSireneExterieure_sec = 0;
	uc_ReinitCompteursSirenes = 0;
	uc_EtatDetecteurs = 0;
	uc_EtatDetecteursPrecedent = 0;
	uc_EtatFraudes = 0;
	uc_EtatFraudesPrecedent = 0;
	uc_CompteurNbPilotageSireneExterieure = 0;
	uc_SireneExterieurePilotee = 0;
	
	uc_DemandeServeurActiverAlarme = 0;
	uc_DemandeServeurCouperAlarme = 0;
	
	// Tous en defauts au demarrage...
	uc_EtatEthernetCablePB = 1;
	uc_EtatEthernetDHCPPB = 1;
	uc_EtatEthernetDNSPB = 1;
	uc_EtatEthernetServeurPB = 1;
	st_EchangeStatus.uc_EtatEthernet = 0x0F;	// Tout en defaut au démarrage
	
	uc_Etat_DIN_VITESSE_VENT = 0;
	
	for(l_us_Compteur = 0;l_us_Compteur < sizeof(uc_VitesseVent);l_us_Compteur++)
	{
		uc_VitesseVent[l_us_Compteur] = 0;
	}
	
	uc_DelestageNiveauEnCours = 0;
	uc_DelestageNiveauEnCoursPrecedent = 1;	// Pour forcer log état au start...
	vd_DelestageUpdateConsigneZones();
	uc_DelestageCouperCumulus = 0;
	
	//us_AinVBatMin = 65535;
	//us_AinVBatMax = 0;
	//uc_OscillationCompteurSeuilBas_sec = 0;
	//uc_OscillationCompteurSeuilHaut_sec = 0;
	for(l_us_Compteur = 0;l_us_Compteur < uc_VBAT_NB_ECHANTILLONS;l_us_Compteur++)
	{
		us_VBatDernieresValeurs[l_us_Compteur] = 0; 
	}
	uc_BatteriePresente = 0;
	uc_BatteriePresentePrecedent = 2;	// Pour forcer log état au start...
	
	uc_FlagResetBP = 0;
	
	uc_ControlerDateRetourVacances = 0;
}

// Retourne 1 si erreur détectée au niveau applicatif
// Ignore certaines erreurs considérées comme warning...
unsigned char uc_ErreurDetectee(void)
{
	unsigned char l_uc_Retour;
	unsigned char l_uc_Compteur;
	
	
	l_uc_Retour = 0;
	if(us_ErreursTacheEcran != us_ERREUR_TACHE_ECRAN_AUCUNE)						l_uc_Retour = 1;
	/*for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE;l_uc_Compteur++)
	{	Ne pas prendre en compte les erreurs de Comm (peuvent arriver en cas de perturbation, mais ne doivent pas tout bloquer !)
		if(us_ErreursDialogueBA[l_uc_Compteur] != us_ERREUR_DIALOGUE_BA_AUCUNE)		l_uc_Retour = 1;
	}*/
	if(us_ErreursTacheBA != us_ERREUR_TACHE_BA_AUCUNE)								l_uc_Retour = 1;
	if(us_ErreursAlarme != us_ERREUR_ALARME_AUCUNE)									l_uc_Retour = 1;
	if(us_ErreursTachePrincipale != us_ERREUR_TACHE_PRINCIPALE_AUCUNE)				l_uc_Retour = 1;
	if(us_ErreursRTC != us_ERREUR_RTC_AUCUNE)										l_uc_Retour = 1;
	//xxx a completer
			
	return l_uc_Retour;
}

void vd_StartTacheEcran(void)
{
	_mqx_uint l_u_result;

	l_u_result = _task_create(0, ECRAN_TASK, 0);
	if(l_u_result == MQX_NULL_TASK_ID) 
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_ECRAN;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Erreur start task ecran\n");
	} 
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Task ecran OK\n");
	}
}
void vd_StartTacheBA(void)
{
	_mqx_uint l_u_result;

	l_u_result = _task_create(0, BA_TASK, 0);
	if(l_u_result == MQX_NULL_TASK_ID) 
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_BA;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Erreur start task BA\n");
		st_EchangeStatus.uc_Defboitier1 = 1;
		st_EchangeStatus.uc_Defboitier2 = 1;
		st_EchangeStatus.uc_Defboitier3 = 1;
	} 
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Task BA OK\n");
	}
}
void vd_StartTacheTeleinfo(void)
{
	_mqx_uint l_u_result;
	
	l_u_result = _task_create(0, TELE_TASK, 0);
	if(l_u_result == MQX_NULL_TASK_ID) 
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_TELEINFO;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Erreur start task Teleinfo\n");
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Task Teleinfo OK\n");
	}
}
void vd_StartTacheEthernet(void)
{
	_mqx_uint l_u_result;
	
	l_u_result = _task_create(0, ETH_TASK, 0);
	if(l_u_result == MQX_NULL_TASK_ID) 
	{
		DETECTION_ERREUR_TACHE_PRINCIPALE_START_TACHE_ETHERNET;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Erreur start task Ethernet\n");
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Task Ethernet OK\n");
	}
}
void vd_MAJEtatBPDansTableEchange(void)
{
	unsigned char l_uc_Temp;

	// Alerte SMS
	l_uc_Temp = 0;
	// bit 0 : 1= déclenchement alarme
	// bit 1 : 1= déclenchement alerte suite à détection fuite d'eau lave linge
	// bit 2 : 1= déclenchement alerte suite à détection fuite d'eau lave vaisselle
	if(st_EchangeStatus.uc_AlarmeDeclenchee != 0)	l_uc_Temp |= 0x01;
	if(st_EchangeStatus.uc_FuiteLL != 0)			l_uc_Temp |= 0x02;
	if(st_EchangeStatus.uc_FuiteLV != 0)			l_uc_Temp |= 0x04;
	Tb_Echange[Alerte] = l_uc_Temp;

	// Status
	l_uc_Temp = 0;
	// bit 0 : 1= heures creuses en cours
	// bit 1 : 1= délestage en cours
	// bit 2 : 1= mode secouru
	if(st_EchangeStatus.uc_HeuresCreuses != 0)							l_uc_Temp |= 0x01;
	if(st_EchangeStatus.uc_SurConsommation != 0)						l_uc_Temp |= 0x02;
	if(st_EchangeStatus.uc_Secouru != 0)								l_uc_Temp |= 0x04;
	Tb_Echange[Status] = l_uc_Temp;

	//Information -> à destination de l'écran	
		// bit 0 : 1= défaut com compteur ERDF
		// bit 1 : 1= défaut com IHM
		// bit 2 : 1= défaut com BA PDV 
		// bit 3 : 1= défaut com BA CHB
		// bit 4 : 1= défaut com BA PDE
	l_uc_Temp = 0;
	if(st_EchangeStatus.uc_DefTeleinfo != 0)		l_uc_Temp |= 0x01;
	//if(st_EchangeStatus.b_W_DefIHM != 0)			l_uc_Temp |= 2;	xxx défaut IHM non géré
	if(st_EchangeStatus.uc_Defboitier1 != 0)		l_uc_Temp |= 0x04;
	if(st_EchangeStatus.uc_Defboitier2 != 0)		l_uc_Temp |= 0x08;
	if(st_EchangeStatus.uc_Defboitier3 != 0)		l_uc_Temp |= 0x10;
	Tb_Echange[Information] = l_uc_Temp;
	
	Tb_Echange[EtatEthernet] = st_EchangeStatus.uc_EtatEthernet;
	
	//EtatBP1
		// bit 0 : alarme activée
		// bit 1 : alarme déclenchée
	l_uc_Temp = 0;
	if(st_EchangeStatus.uc_AlarmeActivee != 0)		l_uc_Temp |= 0x01;
	if(st_EchangeStatus.uc_AlarmeDeclenchee != 0)	l_uc_Temp |= 0x02;
	Tb_Echange[EtatBP1] = l_uc_Temp;
	
	//Test_ETOR_1,// Read only
		   // Etat au fil de l'eau des ETOR de la carte BP.
		   // Bit 0 = détecteur d'ouverture
		   // Bit 1 = détecteur de présence 1 signal
		   // Bit 2 = détecteur de présence 1 fraude
		   // Bit 3 = détecteur de présence 2 signal
		   // Bit 4 = détecteur de présence 2 fraude
		   // Bit 5 = sirène d'intérieur fraude
		   // Bit 6 = sirène d'extérieur fraude
	l_uc_Temp = 0;
	if(lwgpio_get_value(&IO_DIN_Detection_Ouverture) == LWGPIO_VALUE_HIGH)			l_uc_Temp |= 0x01;
	if(lwgpio_get_value(&IO_DIN_Detection_presence1) == LWGPIO_VALUE_HIGH)			l_uc_Temp |= 0x02;
	if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence1) == LWGPIO_VALUE_HIGH)	l_uc_Temp |= 0x04;
	if(lwgpio_get_value(&IO_DIN_Detection_presence2) == LWGPIO_VALUE_HIGH)			l_uc_Temp |= 0x08;
	if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence2) == LWGPIO_VALUE_HIGH)	l_uc_Temp |= 0x10;
	if(lwgpio_get_value(&IO_DIN_OuvertureSireneInterieure) == LWGPIO_VALUE_HIGH)	l_uc_Temp |= 0x20;
	if(lwgpio_get_value(&IO_DIN_OuvertureSireneExterieure) == LWGPIO_VALUE_HIGH)	l_uc_Temp |= 0x40;
	Tb_Echange[Test_ETOR_1] = l_uc_Temp;
	
	//Test_ETOR_2,// Read only
		   // Etat au fil de l'eau des ETOR de la carte BP.
		   // Bit 0 = détecteur fuite lave linge
		   // Bit 1 = détecteur fuite lave vaisselle
		   // Bit 2 = détecteur de pluie
		   // Bit 3 = détecteur de vent
		   // Bit 4 = bouton magique
		   // Bit 5 = détecteur d'ouverture tableau domotique
	l_uc_Temp = 0;
	if(st_EchangeStatus.uc_FuiteLL != 0)												l_uc_Temp |= 0x01;
	if(st_EchangeStatus.uc_FuiteLV != 0)												l_uc_Temp |= 0x02;
	if(lwgpio_get_value(&IO_DIN_Detection_Pluie) == LWGPIO_VALUE_HIGH)					l_uc_Temp |= 0x04;
	if(uc_Etat_DIN_VITESSE_VENT != 0)													l_uc_Temp |= 0x08;
	if(lwgpio_get_value(&IO_DIN_BOUTON_MAGIQUE) == LWGPIO_VALUE_HIGH)					l_uc_Temp |= 0x10;
	if(lwgpio_get_value(&IO_DIN_OuvertureTableauDominique) == LWGPIO_VALUE_HIGH)		l_uc_Temp |= 0x20;
	Tb_Echange[Test_ETOR_2] = l_uc_Temp;
}

// Initialise le timer périodique
// fonction appellée 1 fois à l'init
// ensuite s'auto-entretien dans la fonction timer 
// Ces timers sont utilisés par tout l'applicatif pour cadencer les traitements
void vd_Timer50ms_Init(void)
{
	MQX_TICK_STRUCT l_ticks;
	MQX_TICK_STRUCT l_dticks;
	

	_time_init_ticks(&l_dticks, 0);
	_time_add_msec_to_ticks(&l_dticks, 50);		// 50 ms
	
	_time_get_elapsed_ticks(&l_ticks);
	_timer_start_periodic_at_ticks(vd_Timer50ms_Traitement, 0, TIMER_ELAPSED_TIME_MODE, &l_ticks, &l_dticks);	//xxx tester retour
}
void vd_Timer1sec_Init(void)
{
	MQX_TICK_STRUCT l_ticks;
	MQX_TICK_STRUCT l_dticks;
	

	_time_init_ticks(&l_dticks, 0);
	_time_add_msec_to_ticks(&l_dticks, 1000);		// 1 sec
	
	_time_get_elapsed_ticks(&l_ticks);
	_timer_start_periodic_at_ticks(vd_Timer1sec_Traitement, 0, TIMER_ELAPSED_TIME_MODE, &l_ticks, &l_dticks);	//xxx tester retour
}

// fonction appellée lorque le timer est écoulé : toutes les 50 ms
void vd_Timer50ms_Traitement(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr)
{
	ul_EspionTachePrincipaleNbDeclenchementTimer50ms++;

	uc_CadencementTraitement_50ms++;

	// Fait flasher la led d'état périodiquement
	uc_CompteurLED_50ms++;
	
	if(uc_CompteurLED_50ms < uc_LED_CADENCEMENT_CYCLE_50ms)
	{
		lwgpio_set_value(&IO_DOUT_LEDEtatBP, LWGPIO_VALUE_LOW);
	}
	else
	{
		lwgpio_set_value(&IO_DOUT_LEDEtatBP, LWGPIO_VALUE_HIGH);
		if(uc_CompteurLED_50ms >= (uc_LED_CADENCEMENT_CYCLE_50ms+uc_LED_DUREE_ALLUMEE_50_ms))
		{
			uc_CompteurLED_50ms = uc_LED_DUREE_ALLUMEE_50_ms;
		}
	}
	
	// Compteur filtrage entrées TOR alarme
	if(uc_CompteurDetect_Ouv_50ms < uc_DUREE_ENTREE_ALARME_A_1_50ms)			uc_CompteurDetect_Ouv_50ms++;
	if(uc_CompteurDetect_Pres1_50ms < uc_DUREE_ENTREE_ALARME_A_1_50ms)			uc_CompteurDetect_Pres1_50ms++;
	if(uc_CompteurDetect_Pres2_50ms < uc_DUREE_ENTREE_ALARME_A_1_50ms)			uc_CompteurDetect_Pres2_50ms++;
	if(uc_CompteurOuvertureSireneInterieure < uc_DUREE_ENTREE_ALARME_A_1_50ms)	uc_CompteurOuvertureSireneInterieure++;
	if(uc_CompteurOuvertureSireneExterieure < uc_DUREE_ENTREE_ALARME_A_1_50ms)	uc_CompteurOuvertureSireneExterieure++;
	if(uc_CompteurOuverturePanneauDomotique < uc_DUREE_ENTREE_ALARME_A_1_50ms)	uc_CompteurOuverturePanneauDomotique++;
}

// fonction appellée lorque le timer est écoulé : toutes les secondes
void vd_Timer1sec_Traitement(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr)
{
	ul_EspionTachePrincipaleNbDeclenchementTimer1sec++;
	
	if(uc_AlarmeCompteurSecondes < 255)		uc_AlarmeCompteurSecondes++;	// xxx Verif si ++ fait en une instruction
}
