#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include <string.h>
#include <rtcs.h>
#include <ipcfg.h>

#include "application.h"

#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "global.h"
#include "ecran.h"
#include "GestionSocket.h"
#include "www.h"
#include "hard.h"
#include "tableechangeflash.h"

#define extern
#include "EspionRS.h"
#undef extern

//extern unsigned short us_SimulTeleinfoCompteur;//xxx

// Variables locales au module
LWSEM_STRUCT lock_EspionRS;	// Verrouillage accès multithread à la liaison Espion RS xxx

// Contient pour chaque provenance d'informations de debug l'autorisation ou non de la publication
unsigned char uc_EspionFiltre[uc_ESPION_NB];

// Blocage de tout affichage evenementiel - DEFINI VIA MENU
unsigned char uc_EspionSortieActivee;

// Gestion des menus
unsigned char uc_EspionMenuEnCours;


#ifdef DEBUG
void vd_EspionRS_Init(void)
{
	unsigned char l_uc_Compteur;
	unsigned short l_us_Compteur;
	
	uc_EspionSortieActivee = uc_ESPION_OFF;
	for(l_uc_Compteur = 0; l_uc_Compteur < uc_ESPION_NB; l_uc_Compteur++)
	{
		uc_EspionFiltre[l_uc_Compteur] = uc_ESPION_ON;
	}
	uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ACTIVITE] = uc_ESPION_OFF;
	//uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_TXRX] = uc_ESPION_OFF;
	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_DOWNLOAD] = uc_ESPION_OFF;
	uc_EspionFiltre[uc_ESPION_TELEINFO_RX] = uc_ESPION_OFF;
	uc_EspionFiltre[uc_ENTREE_VENT] = uc_ESPION_OFF;
	
	//uc_EspionFiltre[uc_ESPION_RTC_ACTIVITE] = uc_ESPION_ON;
			
	//uc_EspionFiltre[uc_ESPION_ES] = uc_ESPION_ON;
	
	//uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ACTIVITE] = uc_ESPION_ON;
	
	// Espions pour delestage
	//uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ACTIVITE] = uc_ESPION_ON;
	//uc_EspionFiltre[uc_ESPION_TABLE_ECHANGE] = uc_ESPION_ON;
	//uc_EspionFiltre[uc_ESPION_ETAT_BP] = uc_ESPION_ON;
	//uc_EspionFiltre[uc_ESPION_TELEINFO] = uc_ESPION_ON;
	
	
	uc_EspionMenuEnCours = uc_MENU_AUCUN;

	us_ErreursTacheEcran = us_ERREUR_TACHE_ECRAN_AUCUNE;
	us_ErreursTacheEcranPrecedent = us_ERREUR_TACHE_ECRAN_AUCUNE;
	
	ul_EspionTacheEcranNbOctetsRecus = 0;
	ul_EspionTacheEcranNbTramesCorrectesRecues = 0;
	ul_EspionTacheEcranNbTramesIncorrectesRecues = 0;
	ul_EspionTacheEcranNbTramesStatus = 0;
	ul_EspionTacheEcranNbTramesLectureDiscrete = 0;
	ul_EspionTacheEcranNbTramesEcritureDiscrete = 0;
	ul_EspionTacheEcranNbTramesLectureBloc = 0;
	ul_EspionTacheEcranNbTramesEcritureBloc = 0;
	ul_EspionTacheEcranNbRAZBufferReceptionSurTimeOUT = 0;
	ul_EspionTacheEcranNbDeclenchementTimeOUT = 0;
	uc_EspionTacheEcranEtat = 0;
	ul_EspionTacheEcranCompteurActivite = 0;
	ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur1 = 0;
	ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur3 = 0;
	ul_EspionTacheEcranEnteteTrameIncorrecte = 0;

	for(l_uc_Compteur = 0; l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
	{
		us_ErreursDialogueBA[l_uc_Compteur] = us_ERREUR_DIALOGUE_BA_AUCUNE;
		us_ErreursDialogueBAPrecedent[l_uc_Compteur] = us_ERREUR_DIALOGUE_BA_AUCUNE;
		ul_EspionTacheBACompteurEmissionI2C[l_uc_Compteur] = 0;
		ul_EspionTacheBACompteurErreurI2C[l_uc_Compteur] = 0;
	}
	us_ErreursTacheBA = us_ERREUR_TACHE_BA_AUCUNE;
	us_ErreursTacheBAPrecedent = us_ERREUR_TACHE_BA_AUCUNE;
	uc_EspionTacheBAEtat = 0;
	uc_Espionsl_fct_write_polled = 0;
	ul_EspionTacheBACompteurActivite = 0;
	ul_EspionTacheBANbBlocageI2C = 0;
	uc_NbEssaisReinitMax = 0;
	uc_NbLoupesReinit = 0;
		
	for(l_uc_Compteur = 0; l_uc_Compteur < uc_SCENARIO_NB; l_uc_Compteur++)
	{
		ul_ScenariosNbExecution[l_uc_Compteur] = 0;
	}
	ul_ScenariosNumeroIncorrect = 0;
	ul_ScenariosAdressesValeursIncorrectes = 0;
	ul_ScenariosAdressesEnDehorsPlage = 0;
	ul_ScenariosInitAdressesEnDehorsPlage = 0;
	
	us_ErreursAlarme = us_ERREUR_ALARME_AUCUNE;
	us_ErreursAlarmePrecedent = us_ERREUR_ALARME_AUCUNE;
	
	us_ErreursTachePrincipale = us_ERREUR_TACHE_PRINCIPALE_AUCUNE;
	us_ErreursTachePrincipalePrecedent = us_ERREUR_TACHE_PRINCIPALE_AUCUNE;
	uc_EspionTachePrincipaleEtat = 0;
	ul_EspionTachePrincipaleCompteurActivite = 0;
	ul_EspionTachePrincipaleNbMessagesBAEnvoyes = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs1 = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs2 = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs3 = 0;
	ul_EspionTachePrincipaleNbMessagesBARecus = 0;
	//ul_EspionTachePrincipaleAcquisitionAINEnCours = 0;
	//ul_EspionTachePrincipaleAcquisitionAINOK = 0;
	ul_EspionTachePrincipaleNbCoupuresCourant = 0;
	ul_EspionTachePrincipaleNbDeclenchementTimer50ms = 0;
	ul_EspionTachePrincipaleNbDeclenchementTimer1sec = 0;
	ul_EspionTableEchangeNbAccesEcritureRefuse = 0;
	ul_EspionTableEchangeNbAccesLectureRefuse = 0;

	us_ErreursRTC = us_ERREUR_RTC_AUCUNE;
	us_ErreursRTCPrecedent = us_ERREUR_RTC_AUCUNE;
	
	ul_EspionRTCNbMAJRTC = 0;
	ul_EspionRTCNbPassageHeureEte = 0;
	ul_EspionRTCNbPassageHeureHiver = 0;
	
	ul_ErreursHard1 = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard1Precedent = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard2 = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard2Precedent = ul_ERREUR_HARD_AUCUNE;

	us_ErreursChauffage = us_ERREUR_CHAUFFAGE_AUCUNE;
	us_ErreursChauffagePrecedent = us_ERREUR_CHAUFFAGE_AUCUNE;

	for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees;l_us_Compteur++)
	{
		Tb_EchangePrecedent[l_us_Compteur] = 0;
	}

	uc_AlarmeDetection_Precedent = 0xFF;
	uc_AlarmeFraude_Precedent = 0xFF;
	uc_AlarmeCompteurSecondes_Precedent = 0xFF;
	uc_Alarme_NouveauCode_1_LSB_Precedent = 0xFF;
	uc_Alarme_NouveauCode_1_MSB_Precedent = 0xFF;
	uc_Alarme_NouveauCode_2_LSB_Precedent = 0xFF;
	uc_Alarme_NouveauCode_2_MSB_Precedent = 0xFF;
	Tb_Echange_Alarme_Commande_Precedent = 0xFF;
	Tb_Echange_Alarme_Mode_Precedent = 0xFF;
	Tb_Echange_Alarme_SuiviChangementCode_Precedent = 0xFF;
	Tb_Echange_Alarme_Autorisation_Precedent = 0xFF;
	Tb_Echange_Alarme_SuiviAlarme_Precedent = 0xFF;
	uc_AlarmeActivee_Precedent = 0xFF;
	uc_AlarmeDeclenchee_Precedent = 0xFF;
	uc_AlarmeProcedureSortieEnCours_Precedent = 0xFF;
	uc_AlarmeProcedureRentreeEnCours_Precedent = 0xFF;
	
	st_chauf_ZJ_Precedent.uc_Mode = 0xFF;
	st_chauf_ZJ_Precedent.uc_ConsigneFilPiloteEnCours = 0xFF;
	st_chauf_ZJ_Precedent.uc_DelestageActif = 0;
	st_chauf_ZJ_Precedent.uc_ConsigneFilPiloteNonAnticipe = 0xFF;
	st_chauf_ZJ_Precedent.uc_HeurePassageAnticipe = 0;
	st_chauf_ZN_Precedent.uc_Mode = 0xFF;
	st_chauf_ZN_Precedent.uc_ConsigneFilPiloteEnCours = 0xFF;
	st_chauf_ZN_Precedent.uc_DelestageActif = 0;
	st_chauf_ZN_Precedent.uc_ConsigneFilPiloteNonAnticipe = 0xFF;
	st_chauf_ZN_Precedent.uc_HeurePassageAnticipe = 0;
	st_chauf_Zsdb1_Precedent.uc_Mode = 0xFF;
	st_chauf_Zsdb1_Precedent.uc_ConsigneFilPiloteEnCours = 0xFF;
	st_chauf_Zsdb1_Precedent.uc_DelestageActif = 0;
	st_chauf_Zsdb1_Precedent.uc_ConsigneFilPiloteNonAnticipe = 0xFF;
	st_chauf_Zsdb1_Precedent.uc_HeurePassageAnticipe = 0;
	st_chauf_Zsdb2_Precedent.uc_Mode = 0xFF;
	st_chauf_Zsdb2_Precedent.uc_ConsigneFilPiloteEnCours = 0xFF;
	st_chauf_Zsdb2_Precedent.uc_DelestageActif = 0;
	st_chauf_Zsdb2_Precedent.uc_ConsigneFilPiloteNonAnticipe = 0xFF;
	st_chauf_Zsdb2_Precedent.uc_HeurePassageAnticipe = 0;

	uc_DIN_OuvertureSireneInterieurePrecedent = 0xFF;
	uc_DIN_OuvertureSireneExterieurePrecedent = 0xFF;
	uc_DIN_OuvertureTableauDominiquePrecedent = 0xFF;
	uc_Cde_Sirene_ExterieurePrecedent = 0xFF;
	uc_Cde_15VSP_AlimBAPrecedent = 0xFF;
	uc_Cde_VanneArrosagePrecedent = 0xFF;
	uc_Cde_PriseSecuritePrecedent = 0xFF;
	uc_Cde_MachineALaverPrecedent = 0xFF;
	uc_Cde_CumulusPrecedent = 0xFF;
	uc_DIN_Detection_OuverturePrecedent = 0xFF;
	uc_DIN_Detection_presence1Precedent = 0xFF;
	uc_DIN_Detection_presence2Precedent = 0xFF;
	uc_DIN_Detection_PluiePrecedent = 0xFF;
	uc_DIN_Etat_AlimPrincipalePrecedent = 0xFF;

	uc_DOUT_EcranDirectionPrecedent = 0xFF;
	uc_DIN_OuvertureDecteurPresence1Precedent = 0xFF;
	uc_DIN_OuvertureDecteurPresence2Precedent = 0xFF;
	uc_DIN_VitesseVentPrecedent = 0xFF;
	uc_DIN_BoutonMagiquePrecedent = 0xFF;
	uc_DIN_ErreurEcranHardPrecedent = 0xFF;

	uc_EspionTacheEthernetEtat = 0;
	uc_EspionTacheEthernetInitRTCS = 0;
	uc_EspionTacheEthernetDialogueAvecServeur = 0;
	ul_EspionTacheEthernetCompteurActivite = 0;
	uc_EspionTacheEthernetOpenSocket = 0;
	ul_EspionTacheEthernetCableOK = 0;
	ul_EspionTacheEthernetCablePB = 0;
	ul_EspionTacheEthernetDCHPOK = 0;
	ul_EspionTacheEthernetDCHPPB = 0;
	ul_EspionTacheEthernetIPFixeOK = 0;
	ul_EspionTacheEthernetIPFixePB = 0;
	ul_EspionTacheEthernetDNSOK = 0;
	ul_EspionTacheEthernetDNSPB = 0;
	ul_EspionTacheEthernetDialogueServeurOK = 0;
	ul_EspionTacheEthernetDialogueServeurPBRTCS = 0;
	ul_EspionTacheEthernetDialogueServeurPBData = 0;
	ul_EspionTacheEthernetFonction1OK = 0;
	ul_EspionTacheEthernetFonction1PB = 0;
	ul_EspionTacheEthernetFonction2OK = 0;
	ul_EspionTacheEthernetFonction2PB = 0;
	ul_EspionTacheEthernetFonctions3et4OK = 0;
	ul_EspionTacheEthernetFonctions3et4PB = 0;
	uc_EspionTacheEthernetGetInformationServer = 0;
	uc_EspionTacheEthernetPostInformationServer = 0;
	uc_EspionTacheEthernetActionManagment = 0;
	uc_EspionTacheEthernetTraiterActions = 0;
	uc_EspionTacheEthernetTraitementAlarme = 0;
	uc_EspionTacheEthernetTraitementAction = 0;
	ul_EspionTacheEthernetFonctionDownload = 0;
	uc_EspionTacheEthernetDownload = 0;
	
	uc_EspionEepromSoftErreurLectureEtat = 0;
	uc_EspionEepromSoftErreurAttenteBusy = 0;
	uc_EspionEepromSoftErreurEnableEcriture = 0;
	uc_EspionEepromSoftErreurEcriture = 0;
	uc_EspionEepromSoftErreurLecture = 0;
	uc_EspionEepromSoftErreurEnableEcritureStatus = 0;
	uc_EspionEepromSoftErreurStatusEnableEcriture = 0;
	uc_EspionEepromSoftErreurEffacement = 0;

	ul_EspionCompteurITAlternanceSecteur = 0;
	ul_EspionCompteurITAlternanceSecteurMain = 0;
	ul_EspionCompteurITAlternanceSecteurMainPrecedent = 0;
	ul_EspionCompteurITTimerAlternance = 0;
	ul_EspionCompteurITTimerAlternanceMain = 0;
	ul_EspionCompteurITTimerAlternanceMainPrecedent = 0;
	
	us_ErreursTacheTeleinfo = 0;
	us_ErreursTacheTeleinfoPrecedent = 0;
	uc_EspionTacheTeleinfoEtat = 0;
	ul_EspionTacheTeleinfoCompteurActivite = 0;
	ul_EspionTacheTeleinfoNbDeclenchementTimeOUT = 0;
	ul_EspionTacheTeleinfoCompteurRx = 0;
	ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoDebut = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoFin = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData = 0;
}

unsigned char vd_EspionMainActive(void)
{
	if(uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ACTIVITE] == uc_ESPION_ON && uc_EspionSortieActivee == uc_ESPION_ON && uc_EspionMenuEnCours == uc_MENU_AUCUN)
	{
		return uc_ESPION_ON;
	}
	else
	{
		return uc_ESPION_OFF;
	}
}

// Printf réencapsulé : permet de connaitre l'origine du printf et de ne pas afficher si demandé
void vd_EspionRS_Printf(unsigned char uc_Source, const char _PTR_ String, ...)
{
	va_list vl;
    TIME_STRUCT l_time_mqx;
    DATE_STRUCT l_st_DateHeure;

    // xxx Rajouter MUTEX
	if(uc_EspionFiltre[uc_Source] == uc_ESPION_ON && uc_EspionSortieActivee == uc_ESPION_ON && uc_EspionMenuEnCours == uc_MENU_AUCUN)
	{
		if(String[0] == 0)
		{
			printf("\n");
		}
		else
		{
			_time_get(&l_time_mqx);						// Récupère la date / heure MQX
			_time_to_date(&l_time_mqx,&l_st_DateHeure);	// Convertie la date / heure en format utilisable -> st_DateHeure
			printf("%02i/", l_st_DateHeure.DAY);
			printf("%02i/", l_st_DateHeure.MONTH);
			printf("%04i ", l_st_DateHeure.YEAR);
			printf("%02i:", l_st_DateHeure.HOUR);
			printf("%02i:", l_st_DateHeure.MINUTE);
			printf("%02i.", l_st_DateHeure.SECOND);
			printf("%03i\t", l_st_DateHeure.MILLISEC);
			vd_EspionRS_PrintfLibelleTypeTexte(uc_Source);
	
			va_start(vl,String);
			vprintf(String,vl);
			va_end(vl);
		}
	}
}
void vd_EspionRS_PrintfSansHorodatage(unsigned char uc_Source, const char _PTR_ String, ...)	//xxx verif all cas vd_EspionRS_PrintfLibelleTypeTexte pas util ?
{
	va_list vl;

    // xxx Rajouter MUTEX
	if(uc_EspionFiltre[uc_Source] == uc_ESPION_ON && uc_EspionSortieActivee == uc_ESPION_ON && uc_EspionMenuEnCours == uc_MENU_AUCUN)
	{
		//vd_EspionRS_PrintfLibelleTypeTexte(uc_Source);
	    va_start(vl,String);
		vprintf(String,vl);
		va_end(vl);
	}
}

void vd_EspionRS_PrintfLibelleTypeTexte(unsigned char Source)
{
	switch(Source)
	{
		case uc_ESPION_TACHE_ECRAN_ERREUR:
			TypeTexte("XCRAN ERREUR");
		break;
		case uc_ESPION_TACHE_ECRAN_ACTIVITE:
			TypeTexte("ECRAN");
		break;
		case uc_ESPION_DIALOGUE_BA_ERREUR:
			TypeTexte("BA I2C ERREUR");
		break;
		case uc_ESPION_TACHE_BA_ERREUR:
			TypeTexte("BA ERREUR");
		break;
		case uc_ESPION_TACHE_BA_ACTIVITE:
			TypeTexte("BA");
		break;
		case uc_ESPION_SCENARIOS:
			TypeTexte("SCENARIO");
		break;
		case uc_ESPION_ALARME_ERREURS:
			TypeTexte("ALARME ERREUR");
		break;
		case uc_ESPION_ALARME_ACTIVITE:
			TypeTexte("ALARME");
		break;
		case uc_ESPION_TACHE_PRINCIPALE_ERREUR:
			TypeTexte("MAIN ERREUR");
		break;
		case uc_ESPION_TACHE_PRINCIPALE_ACTIVITE:
			TypeTexte("MAIN");
		break;
		case uc_ESPION_RTC_ERREUR:
			TypeTexte("RTC ERREUR");
		break;
		case uc_ESPION_RTC_ACTIVITE:
			TypeTexte("RTC");
		break;
		case uc_ESPION_HARD_ERREURS:
			TypeTexte("HARD ERREUR");
		break;
		case uc_ESPION_CHAUFFAGE_ERREURS:
			TypeTexte("CHAUFFAGE ERREUR");
		break;
		case uc_ESPION_CHAUFFAGE_ACTIVITE:
			TypeTexte("CHAUFFAGE");
		break;
		case uc_ESPION_TABLE_ECHANGE:
			TypeTexte("TABLE ECHANGE");
		break;
		case uc_ESPION_ES:
			TypeTexte("ES");
		break;
		case uc_ESPION_ETAT_BP:
			TypeTexte("ETAT BP");
		break;
		case uc_ESPION_TACHE_ETHERNET_ACTIVITE:
			TypeTexte("ETHERNET");
		break;
		case uc_ESPION_TACHE_ETHERNET_TXRX:
			TypeTexte("ETHERNET TXRX");
		break;
		case uc_ESPION_TACHE_ETHERNET_DOWNLOAD:
			TypeTexte("ETHERNET DOWNLOAD");
		break;
		case uc_ESPION_FIL_PILOTE:
			TypeTexte("FIL PILOTE");
		break;
		case uc_ESPION_TELEINFO:
			TypeTexte("TELEINFO");
		break;
		case uc_ESPION_TELEINFO_RX:
			TypeTexte("TELEINFO RX");
		break;
		case uc_ENTREE_VENT:
			TypeTexte("ENTREE VENT");
		break;
		default:
			TypeTexte("???");
		break;
	}
	printf("\t");
}

void TypeTexte(char *pc_Texte)
{
	unsigned char l_uc_Compteur;
	
	printf(pc_Texte);
	l_uc_Compteur = strlen(pc_Texte);
	while(l_uc_Compteur < 30)
	{
		printf(" ");
		l_uc_Compteur++;
	}
}

char * puc_EspionFiltreEtat(unsigned char uc_Numero)
{
	if(uc_EspionFiltre[uc_Numero] == uc_ESPION_ON)	return "ON";
	else											return "OFF";
}

// Gere la console de debug : traitement en fonction des touches recues
void vd_EspionRS(void)
{
	unsigned char l_uc_RefreshAffichage;
	unsigned char l_uc_Caractere;

	
	l_uc_RefreshAffichage = 0;
	if(status() == 0)	// Pas de caractère recu
	{
		if(uc_EspionActive != 0)
		{
			if(uc_CompteurDesactivationEspionAuto_sec >= uc_TEMPO_DESACTIVATION_AUTO_sec)
			{
				uc_EspionActive = 0;
				printf("XXXXX ESPION DESACTIVE !!!\n\n\n\n");
			}
		}
	}
	else	// Caractère reçu !
	{
		l_uc_Caractere = getchar();

		uc_CompteurDesactivationEspionAuto_sec = 0;
		
		// Check mdp si espion pas activé, sinon exécution commandes reçues
		if(uc_EspionActive == 0)
		{
			if(uc_CompteurCheckMdpEsion < sizeof(uc_ESPION_MOT_PASSE_ACTIVATION))
			{
				if(l_uc_Caractere == uc_ESPION_MOT_PASSE_ACTIVATION[uc_CompteurCheckMdpEsion])
				{
					uc_CompteurCheckMdpEsion++;
					if(uc_CompteurCheckMdpEsion >= sizeof(uc_ESPION_MOT_PASSE_ACTIVATION)-1)
					{
						uc_CompteurCheckMdpEsion = 0;
						uc_EspionActive = 1;
						printf("XXXXX ESPION ACTIVE !!!\n\n\n\n");
					}
				}
				else
				{
					uc_CompteurCheckMdpEsion = 0;
				}
			}
			else
			{
				uc_CompteurCheckMdpEsion = 0;
			}
		}
		else
		{
			l_uc_RefreshAffichage = 1;
			
			switch(l_uc_Caractere)
			{
				case 'm':
				case 'M':	// Rentre (ou retour) dans menu principal
					uc_EspionMenuEnCours = uc_MENU_PRINCIPAL;
				break;
				
				case 'q':
				case 'Q':	// Sortie du menu debug
					uc_EspionMenuEnCours = uc_MENU_AUCUN;
				break;

				default:
					switch(uc_EspionMenuEnCours)
					{
						case uc_MENU_PRINCIPAL:
							vd_GestionMenuPrincipal(l_uc_Caractere);
						break;
						case uc_MENU_MESSAGES_EVENEMENTIELS:
							vd_GestionMenuMessagesEvenementiels(l_uc_Caractere);
						break;
						case uc_MENU_AFFICHER_ESPIONS:
							vd_GestionMenuAfficherEspions(l_uc_Caractere);
						break;
						case uc_MENU_RAZ_ESPIONS:
							vd_GestionMenuRAZEspions(l_uc_Caractere);
						break;
						case uc_MENU_ACTIONS:
							vd_GestionMenuActions(l_uc_Caractere);
						break;
					}
				break;
			}
		}
	}
	
	if(l_uc_RefreshAffichage != 0)
	{
		printf("\n\n");
		switch(uc_EspionMenuEnCours)
		{
			case uc_MENU_PRINCIPAL:
					printf("1. MSGS EVENT ON/OFF\n");
					printf("2. AFFICHER ESPIONS\n");
					printf("3. RAZ ESPIONS\n");
					printf("4. AFFICHER TOUS LES ESPIONS\n");
					printf("5. RAZ TOUS LES ESPIONS\n");
					printf("6. ACTIONS\n");
					printf("> ");
			break;
			case uc_MENU_MESSAGES_EVENEMENTIELS:
				if(uc_EspionSortieActivee == uc_ESPION_ON)		printf("X. ETAT ESPION : ON\n");
				else											printf("X. ETAT ESPION : OFF\n");

				printf("1. ECRAN ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_ECRAN_ERREUR));
				printf("2. ECRAN : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_ECRAN_ACTIVITE));
				printf("3. BA I2C ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_DIALOGUE_BA_ERREUR));
				printf("4. BA ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_BA_ERREUR));
				printf("5. BA : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_BA_ACTIVITE));
				printf("6. SCENARIOS : %s\n",puc_EspionFiltreEtat(uc_ESPION_SCENARIOS));
				printf("7. ALARME ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_ALARME_ERREURS));
				printf("8. ALARME : %s\n",puc_EspionFiltreEtat(uc_ESPION_ALARME_ACTIVITE));
				printf("9. MAIN ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_PRINCIPALE_ERREUR));
				printf("A. MAIN : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE));
				printf("B. RTC ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_RTC_ERREUR));
				printf("C. RTC : %s\n",puc_EspionFiltreEtat(uc_ESPION_RTC_ACTIVITE));
				printf("D. HARD ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_HARD_ERREURS));
				printf("E. CHAUFFAGE ERREURS : %s\n",puc_EspionFiltreEtat(uc_ESPION_CHAUFFAGE_ERREURS));
				printf("F. CHAUFFAGE : %s\n",puc_EspionFiltreEtat(uc_ESPION_CHAUFFAGE_ACTIVITE));
				printf("G. TABLE ECHANGE : %s\n",puc_EspionFiltreEtat(uc_ESPION_TABLE_ECHANGE));
				printf("H. E/S : %s\n",puc_EspionFiltreEtat(uc_ESPION_ES));
				printf("I. ETAT BP : %s\n",puc_EspionFiltreEtat(uc_ESPION_ETAT_BP));
				printf("J. ETHERNET ACTIVITE : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_ETHERNET_ACTIVITE));
				printf("K. ETHERNET TX RX : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_ETHERNET_TXRX));
				printf("L. ETHERNET DOWNLOAD : %s\n",puc_EspionFiltreEtat(uc_ESPION_TACHE_ETHERNET_DOWNLOAD));
				printf("N. FIL PILOTE : %s\n",puc_EspionFiltreEtat(uc_ESPION_FIL_PILOTE));
				printf("O. TELEINFO : %s\n",puc_EspionFiltreEtat(uc_ESPION_TELEINFO));
				printf("P. TELEINFO RX : %s\n",puc_EspionFiltreEtat(uc_ESPION_TELEINFO_RX));
				printf("R. ENTREE VENT : %s\n",puc_EspionFiltreEtat(uc_ENTREE_VENT));
				printf("> ");
			break;
			case uc_MENU_AFFICHER_ESPIONS:
				printf("0. AFFICHER STATS I2C\n");
				printf("1. AFFICHER ECRAN ERREURS\n");	// us_ErreursTacheEcran
				printf("2. AFFICHER ECRAN \n");	// uc_EspionTacheEcranEtat...
				printf("3. AFFICHER BA ERREURS I2C\n");	// us_ErreursDialogueBA
				printf("4. AFFICHER BA ERREURS\n");		// us_ErreursTacheBA
				printf("5. AFFICHER BA\n");		// uc_EspionTacheBAEtat...
				printf("6. AFFICHER SCENARIOS\n");				// ul_ScenariosNbExecution...
				printf("7. AFFICHER ALARME ERREURS\n");
				printf("8. AFFICHER ALARME\n");
				printf("9. AFFICHER MAIN ERREURS\n");
				printf("A. AFFICHER MAIN\n");
				printf("B. AFFICHER RTC ERREURS\n");
				printf("C. AFFICHER RTC\n");
				printf("D. AFFICHER HARD ERREURS\n");
				printf("E. AFFICHER CHAUFFAGE ERREURS\n");
				printf("F. AFFICHER CHAUFFAGE\n");
				printf("G. AFFICHER TABLE ECHANGE\n");
				printf("H. AFFICHER E/S\n");
				printf("I. AFFICHER ETAT BP\n");
				printf("J. AFFICHER ETHERNET\n");
				printf("K. AFFICHER TELEINFO\n");
				printf("> ");
			break;
			case uc_MENU_RAZ_ESPIONS:
				printf("0. RAZ STATS I2C\n");
				printf("1. AFFICHER ECRAN ERREURS\n");	// us_ErreursTacheEcran
				printf("2. AFFICHER ECRAN\n");	// uc_EspionTacheEcranEtat...
				printf("3. RAZ BA ERREURS I2C\n");			// us_ErreursDialogueBA
				printf("4. RAZ BA ERREURS\n");			// us_ErreursTacheBA
				printf("5. RAZ BA\n");			// uc_EspionTacheBAEtat...
				printf("6. RAZ SCENARIOS\n");					// ul_ScenariosNbExecution...
				printf("7. RAZ ALARME ERREURS\n");
				printf("8. RAZ ALARME\n");
				printf("9. RAZ MAIN ERREURS\n");
				printf("A. RAZ MAIN\n");
				printf("B. RAZ RTC ERREURS\n");
				printf("C. RAZ RTC\n");
				printf("D. RAZ HARD ERREURS\n");
				printf("E. RAZ CHAUFFAGE ERREURS\n");
				printf("F. RAZ CHAUFFAGE\n");
				printf("G. RAZ ETHERNET\n");
				printf("H. RAZ TELEINFO\n");
				printf("> ");
			break;
			case uc_MENU_ACTIONS:
				printf("0. EFFACER ZONE CONFIG\n");
				printf("1. MEMORISER TABLE ECHANGE EN FLASH\n");
				printf("2. DESACTIVER EMISSION BP VERS ECRAN\n");
				printf("3. ACTIVER EMISSION BP VERS ECRAN\n");
			break;
		}
	}
	
	if(uc_EspionMenuEnCours == uc_MENU_AUCUN)
	{
		vd_EspionRS_Afficher_ErreursTacheEcran(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursDialogueBA(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursTacheBA(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursAlarme(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursTachePrincipale(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursRTC(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursHard(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ErreursChauffage(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_TableEchange(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_ES(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_EtatBP(uc_AFFICHAGE_FORCE_NON);
		vd_EspionRS_Afficher_EthernetActivite(uc_AFFICHAGE_FORCE_NON);
	}
}

void vd_GestionMenuPrincipal(unsigned char uc_Caractere)
{
	switch(uc_Caractere)
	{
		case '1':
			uc_EspionMenuEnCours = uc_MENU_MESSAGES_EVENEMENTIELS;
		break;
		case '2':
			uc_EspionMenuEnCours = uc_MENU_AFFICHER_ESPIONS;
		break;
		case '3':
			uc_EspionMenuEnCours = uc_MENU_RAZ_ESPIONS;
		break;
		case '4':
			// Afficher tous les espions
			vd_EspionRS_Afficher_ActiviteI2C();
			vd_EspionRS_Afficher_ErreursTacheEcran(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteTacheEcran();
			vd_EspionRS_Afficher_ErreursDialogueBA(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ErreursTacheBA(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteTacheBA();
			vd_EspionRS_Afficher_Scenarios();
			vd_EspionRS_Afficher_ErreursAlarme(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteAlarme();
			vd_EspionRS_Afficher_ErreursTachePrincipale(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteTachePrincipale();
			vd_EspionRS_Afficher_ErreursRTC(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteRTC();
			vd_EspionRS_Afficher_ErreursHard(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ErreursChauffage(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ActiviteChauffage();
			vd_EspionRS_Afficher_TableEchange(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_ES(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_EtatBP(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_EthernetActivite(uc_AFFICHAGE_FORCE_OUI);
			vd_EspionRS_Afficher_Teleinfo(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '5':
			// RAZ tous les espions
			vd_EspionRS_RAZ_ActiviteI2C();
			vd_EspionRS_RAZ_ErreursTacheEcran();
			vd_EspionRS_RAZ_ActiviteTacheEcran();
			vd_EspionRS_RAZ_ErreursDialogueBA();
			vd_EspionRS_RAZ_ErreursTacheBA();
			vd_EspionRS_RAZ_ActiviteTacheBA();
			vd_EspionRS_RAZ_Scenarios();
			vd_EspionRS_RAZ_ErreursAlarme();
			vd_EspionRS_RAZ_ActiviteAlarme();
			vd_EspionRS_RAZ_ErreursTachePrincipale();
			vd_EspionRS_RAZ_ActiviteTachePrincipale();
			vd_EspionRS_RAZ_ErreursRTC();
			vd_EspionRS_RAZ_ActiviteRTC();
			vd_EspionRS_RAZ_ErreursHard();
			vd_EspionRS_RAZ_ErreursChauffage();
			vd_EspionRS_RAZ_EthernetActivite();
			vd_EspionRS_RAZ_Teleinfo();
			
			// RAZ AIN MIN / MAX
			st_AIN4_VBat.us_AINMin = 0xFFFF;
			st_AIN4_VBat.us_AINMax = 0;
			st_AIN5_FuiteLV.us_AINMin = 0xFFFF;
			st_AIN5_FuiteLV.us_AINMax = 0;
			st_AIN6_FuiteLL.us_AINMin = 0xFFFF;
			st_AIN6_FuiteLL.us_AINMax = 0;

		break;
		case '6':
			uc_EspionMenuEnCours = uc_MENU_ACTIONS;
		break;
	}
}

void vd_GestionMenuMessagesEvenementiels(unsigned char uc_Caractere)
{
	switch(uc_Caractere)
	{
		case 'x':
		case 'X':
			if(uc_EspionSortieActivee == uc_ESPION_ON)							uc_EspionSortieActivee = uc_ESPION_OFF;
			else																uc_EspionSortieActivee = uc_ESPION_ON;
		break;
		
		case '1':
			if(uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ERREUR] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ERREUR] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ERREUR] = uc_ESPION_ON;
		break;
		case '2':
			if(uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ACTIVITE] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ACTIVITE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_TACHE_ECRAN_ACTIVITE] = uc_ESPION_ON;
		break;
		case '3':
			if(uc_EspionFiltre[uc_ESPION_DIALOGUE_BA_ERREUR] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_DIALOGUE_BA_ERREUR] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_DIALOGUE_BA_ERREUR] = uc_ESPION_ON;
		break;
		case '4':
			if(uc_EspionFiltre[uc_ESPION_TACHE_BA_ERREUR] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_TACHE_BA_ERREUR] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_TACHE_BA_ERREUR] = uc_ESPION_ON;
		break;
		case '5':
			if(uc_EspionFiltre[uc_ESPION_TACHE_BA_ACTIVITE] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_BA_ACTIVITE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_TACHE_BA_ACTIVITE] = uc_ESPION_ON;
		break;
		case '6':
			if(uc_EspionFiltre[uc_ESPION_SCENARIOS] == uc_ESPION_ON)			uc_EspionFiltre[uc_ESPION_SCENARIOS] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_SCENARIOS] = uc_ESPION_ON;
		break;
		case '7':
			if(uc_EspionFiltre[uc_ESPION_ALARME_ERREURS] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_ALARME_ERREURS] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_ALARME_ERREURS] = uc_ESPION_ON;
		break;
		case '8':
			if(uc_EspionFiltre[uc_ESPION_ALARME_ACTIVITE] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_ALARME_ACTIVITE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_ALARME_ACTIVITE] = uc_ESPION_ON;
		break;
		case '9':
			if(uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ERREUR] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ERREUR] = uc_ESPION_OFF;
			else																		uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ERREUR] = uc_ESPION_ON;
		break;
		case 'a':
		case 'A':
			if(uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ACTIVITE] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ACTIVITE] = uc_ESPION_OFF;
			else																		uc_EspionFiltre[uc_ESPION_TACHE_PRINCIPALE_ACTIVITE] = uc_ESPION_ON;
		break;
		case 'b':
		case 'B':
			if(uc_EspionFiltre[uc_ESPION_RTC_ERREUR] == uc_ESPION_ON)			uc_EspionFiltre[uc_ESPION_RTC_ERREUR] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_RTC_ERREUR] = uc_ESPION_ON;
		break;
		case 'c':
		case 'C':
			if(uc_EspionFiltre[uc_ESPION_RTC_ACTIVITE] == uc_ESPION_ON)			uc_EspionFiltre[uc_ESPION_RTC_ACTIVITE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_RTC_ACTIVITE] = uc_ESPION_ON;
		break;
		case 'd':
		case 'D':
			if(uc_EspionFiltre[uc_ESPION_HARD_ERREURS] == uc_ESPION_ON)			uc_EspionFiltre[uc_ESPION_HARD_ERREURS] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_HARD_ERREURS] = uc_ESPION_ON;
		break;
		case 'e':
		case 'E':
			if(uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ERREURS] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ERREURS] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ERREURS] = uc_ESPION_ON;
		break;
		case 'f':
		case 'F':
			if(uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ACTIVITE] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ACTIVITE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_CHAUFFAGE_ACTIVITE] = uc_ESPION_ON;
		break;
		case 'g':
		case 'G':
			if(uc_EspionFiltre[uc_ESPION_TABLE_ECHANGE] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_TABLE_ECHANGE] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_TABLE_ECHANGE] = uc_ESPION_ON;
		break;
		case 'h':
		case 'H':
			if(uc_EspionFiltre[uc_ESPION_ES] == uc_ESPION_ON)					uc_EspionFiltre[uc_ESPION_ES] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_ES] = uc_ESPION_ON;
		break;
		case 'i':
		case 'I':
			if(uc_EspionFiltre[uc_ESPION_ETAT_BP] == uc_ESPION_ON)				uc_EspionFiltre[uc_ESPION_ETAT_BP] = uc_ESPION_OFF;
			else																uc_EspionFiltre[uc_ESPION_ETAT_BP] = uc_ESPION_ON;
		break;
		case 'j':
		case 'J':
			if(uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_ACTIVITE] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_ACTIVITE] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_ACTIVITE] = uc_ESPION_ON;
		break;
		case 'k':
		case 'K':
			if(uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_TXRX] == uc_ESPION_ON)		uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_TXRX] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_TXRX] = uc_ESPION_ON;
		break;
		case 'l':
		case 'L':
			if(uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_DOWNLOAD] == uc_ESPION_ON)	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_DOWNLOAD] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_TACHE_ETHERNET_DOWNLOAD] = uc_ESPION_ON;
		break;
		case 'n':
		case 'N':
			if(uc_EspionFiltre[uc_ESPION_FIL_PILOTE] == uc_ESPION_ON)				uc_EspionFiltre[uc_ESPION_FIL_PILOTE] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_FIL_PILOTE] = uc_ESPION_ON;
		break;
		case 'o':
		case 'O':
			if(uc_EspionFiltre[uc_ESPION_TELEINFO] == uc_ESPION_ON)					uc_EspionFiltre[uc_ESPION_TELEINFO] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_TELEINFO] = uc_ESPION_ON;
		break;
		case 'p':
		case 'P':
			if(uc_EspionFiltre[uc_ESPION_TELEINFO_RX] == uc_ESPION_ON)				uc_EspionFiltre[uc_ESPION_TELEINFO_RX] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ESPION_TELEINFO_RX] = uc_ESPION_ON;
		break;
		case 'r':
		case 'R':
			if(uc_EspionFiltre[uc_ENTREE_VENT] == uc_ESPION_ON)						uc_EspionFiltre[uc_ENTREE_VENT] = uc_ESPION_OFF;
			else																	uc_EspionFiltre[uc_ENTREE_VENT] = uc_ESPION_ON;
		break;
	}
}

void vd_GestionMenuAfficherEspions(unsigned char uc_Caractere)
{
	switch(uc_Caractere)
	{
		case '0':
			vd_EspionRS_Afficher_ActiviteI2C();
		break;
		case '1':
			vd_EspionRS_Afficher_ErreursTacheEcran(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '2':
			vd_EspionRS_Afficher_ActiviteTacheEcran();
		break;
		case '3':
			vd_EspionRS_Afficher_ErreursDialogueBA(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '4':
			vd_EspionRS_Afficher_ErreursTacheBA(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '5':
			vd_EspionRS_Afficher_ActiviteTacheBA();
		break;
		case '6':
			vd_EspionRS_Afficher_Scenarios();
		break;
		case '7':
			vd_EspionRS_Afficher_ErreursAlarme(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '8':
			vd_EspionRS_Afficher_ActiviteAlarme();
		break;
		case '9':
			vd_EspionRS_Afficher_ErreursTachePrincipale(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'a':
		case 'A':
			vd_EspionRS_Afficher_ActiviteTachePrincipale();
		break;
		case 'b':
		case 'B':
			vd_EspionRS_Afficher_ErreursRTC(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'c':
		case 'C':
			vd_EspionRS_Afficher_ActiviteRTC();
		break;
		case 'd':
		case 'D':
			vd_EspionRS_Afficher_ErreursHard(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'e':
		case 'E':
			vd_EspionRS_Afficher_ErreursChauffage(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'f':
		case 'F':
			vd_EspionRS_Afficher_ActiviteChauffage();
		break;
		case 'g':
		case 'G':
			vd_EspionRS_Afficher_TableEchange(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'h':
		case 'H':
			vd_EspionRS_Afficher_ES(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'i':
		case 'I':
			vd_EspionRS_Afficher_EtatBP(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'j':
		case 'J':
			vd_EspionRS_Afficher_EthernetActivite(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'k':
		case 'K':
			vd_EspionRS_Afficher_Teleinfo(uc_AFFICHAGE_FORCE_OUI);
		break;
	}
}

void vd_GestionMenuRAZEspions(unsigned char uc_Caractere)
{
	switch(uc_Caractere)
	{
		case '0':
			vd_EspionRS_RAZ_ActiviteI2C();
			vd_EspionRS_Afficher_ActiviteI2C();
		break;
		case '1':
			vd_EspionRS_RAZ_ErreursTacheEcran();
			vd_EspionRS_Afficher_ErreursTacheEcran(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '2':
			vd_EspionRS_RAZ_ActiviteTacheEcran();
			vd_EspionRS_Afficher_ActiviteTacheEcran();
		break;
		case '3':
			vd_EspionRS_RAZ_ErreursDialogueBA();
			vd_EspionRS_Afficher_ErreursDialogueBA(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '4':
			vd_EspionRS_RAZ_ErreursTacheBA();
			vd_EspionRS_Afficher_ErreursTacheBA(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '5':
			vd_EspionRS_RAZ_ActiviteTacheBA();
			vd_EspionRS_Afficher_ActiviteTacheBA();
		break;
		case '6':
			vd_EspionRS_RAZ_Scenarios();
			vd_EspionRS_Afficher_Scenarios();
		break;
		case '7':
			vd_EspionRS_RAZ_ErreursAlarme();
			vd_EspionRS_Afficher_ErreursAlarme(uc_AFFICHAGE_FORCE_OUI);
		break;
		case '8':
			vd_EspionRS_RAZ_ActiviteAlarme();
			vd_EspionRS_Afficher_ActiviteAlarme();
		break;
		case '9':
			vd_EspionRS_RAZ_ErreursTachePrincipale();
			vd_EspionRS_Afficher_ErreursTachePrincipale(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'a':
		case 'A':
			vd_EspionRS_RAZ_ActiviteTachePrincipale();
			vd_EspionRS_Afficher_ActiviteTachePrincipale();
		break;
		case 'b':
		case 'B':
			vd_EspionRS_RAZ_ErreursRTC();
			vd_EspionRS_Afficher_ErreursRTC(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'c':
		case 'C':
			vd_EspionRS_RAZ_ActiviteRTC();
			vd_EspionRS_Afficher_ActiviteRTC();
		break;
		case 'd':
		case 'D':
			vd_EspionRS_RAZ_ErreursHard();
			vd_EspionRS_Afficher_ErreursHard(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'e':
		case 'E':
			vd_EspionRS_RAZ_ErreursChauffage();
			vd_EspionRS_Afficher_ErreursChauffage(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'f':
		case 'F':
			vd_EspionRS_RAZ_ActiviteChauffage();
			vd_EspionRS_Afficher_ActiviteChauffage();
		break;
		case 'g':
		case 'G':
			vd_EspionRS_RAZ_EthernetActivite();
			vd_EspionRS_Afficher_EthernetActivite(uc_AFFICHAGE_FORCE_OUI);
		break;
		case 'h':
		case 'H':
			vd_EspionRS_RAZ_Teleinfo();
			vd_EspionRS_Afficher_Teleinfo(uc_AFFICHAGE_FORCE_OUI);
		break;
	}
}

void vd_GestionMenuActions(unsigned char uc_Caractere)
{
	switch(uc_Caractere)
	{
		case '0':
			vd_EffacerZoneConfig();
			printf("Zone config effacé !\n\n");
		break;
		case '1':
			vd_TableEchangeSaveEnFLash();
			printf("Mémorisation table échange en FLASH OK !\n\n");
		break;
		case '2':
			uc_BloquerEmissionVersEcran = 1;
			printf("Blocage émission BP vers ECRAN\n\n");
		break;
		case '3':
			uc_BloquerEmissionVersEcran = 0;
			printf("Activation émission BP Vers ECRAN\n\n");
		break;
	}
}

void vd_EspionRS_Afficher_ErreursTacheEcran(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursTacheEcran;
		if(l_us_ValeurEnCours != us_ErreursTacheEcranPrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"ErreursTacheEcran : 0x%X -> 0x%X\n\n", us_ErreursTacheEcranPrecedent, l_us_ValeurEnCours);
			us_ErreursTacheEcranPrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("us_ErreursTacheEcran (Hexa) : %X\n\n", us_ErreursTacheEcran);
	}
}
void vd_EspionRS_RAZ_ErreursTacheEcran(void)
{
	us_ErreursTacheEcran = us_ERREUR_TACHE_ECRAN_AUCUNE;
	us_ErreursTacheEcranPrecedent = us_ERREUR_TACHE_ECRAN_AUCUNE;
}

void vd_EspionRS_Afficher_Teleinfo(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursTacheTeleinfo;
		if(l_us_ValeurEnCours != us_ErreursTacheTeleinfoPrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_TELEINFO,"ErreursTacheTeleinfo : 0x%X -> 0x%X\n\n", us_ErreursTacheTeleinfoPrecedent, l_us_ValeurEnCours);
			us_ErreursTacheTeleinfoPrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("us_ErreursTacheTeleinfo (Hexa) : %X\n", us_ErreursTacheTeleinfo);
		
		printf("uc_EspionTacheTeleinfoEtat (Hexa) : %X\n", uc_EspionTacheTeleinfoEtat);
		printf("ul_EspionTacheTeleinfoCompteurActivite (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurActivite);
		printf("ul_EspionTacheTeleinfoNbDeclenchementTimeOUT (Hexa) : %X\n", ul_EspionTacheTeleinfoNbDeclenchementTimeOUT);
		printf("ul_EspionTacheTeleinfoCompteurRx (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurRx);
		printf("ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT (Hexa) : %X\n", ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT);
		printf("ul_EspionTacheTeleinfoCompteurGroupeInfoDebut (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurGroupeInfoDebut);
		printf("ul_EspionTacheTeleinfoCompteurGroupeInfoFin (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurGroupeInfoFin);
		printf("ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum);
		printf("ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC (Hexa) : %X\n", ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC);
		printf("ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData (Hexa) : %X\n\n", ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData);
	}
}

void vd_EspionRS_RAZ_Teleinfo(void)
{
	us_ErreursTacheTeleinfo = us_ERREUR_TACHE_TELEINFO_AUCUNE;
	us_ErreursTacheTeleinfoPrecedent = us_ERREUR_TACHE_TELEINFO_AUCUNE;
	
	uc_EspionTacheTeleinfoEtat = 0;
	ul_EspionTacheTeleinfoCompteurActivite = 0;
	ul_EspionTacheTeleinfoNbDeclenchementTimeOUT = 0;
	ul_EspionTacheTeleinfoCompteurRx = 0;
	ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoDebut = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoFin = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC = 0;
	ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData = 0;
}

void vd_EspionRS_Afficher_ActiviteTacheEcran(void) //xxx
{
	printf("Espion Ecran :\n");
	printf("NbOctetsRecus : %d\n", ul_EspionTacheEcranNbOctetsRecus);
	printf("NbTramesOKRecues : %d\n", ul_EspionTacheEcranNbTramesCorrectesRecues);
	printf("NbTramesPBRecues : %d\n", ul_EspionTacheEcranNbTramesIncorrectesRecues);
	printf("NbTramesStatus : %d\n", ul_EspionTacheEcranNbTramesStatus);
	printf("NbTramesLectureDiscrete : %d\n", ul_EspionTacheEcranNbTramesLectureDiscrete);
	printf("NbTramesEcritureDiscrete : %d\n", ul_EspionTacheEcranNbTramesEcritureDiscrete);
	printf("NbTramesLectureBloc : %d\n", ul_EspionTacheEcranNbTramesLectureBloc);
	printf("NbTramesEcritureBloc : %d\n", ul_EspionTacheEcranNbTramesEcritureBloc);
	printf("NbRAZBufferRXSurTimeOUT : %d\n", ul_EspionTacheEcranNbRAZBufferReceptionSurTimeOUT);
	printf("NbTimeOUT : %d\n", ul_EspionTacheEcranNbDeclenchementTimeOUT);
	printf("Etat : %d\n", uc_EspionTacheEcranEtat);
	printf("CptActivite : %d\n", ul_EspionTacheEcranCompteurActivite);
	printf("NbTrameSyncRecueAvecValeur1 : %d\n", ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur1);
	printf("NbTrameSyncRecueAvecValeur3 : %d\n", ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur3);
	printf("EspionTacheEcranEnteteTrameIncorrecte : %d\n", ul_EspionTacheEcranEnteteTrameIncorrecte);
	printf("\n");
}

void vd_EspionRS_RAZ_ActiviteTacheEcran(void)
{
	ul_EspionTacheEcranNbOctetsRecus = 0;
	ul_EspionTacheEcranNbTramesCorrectesRecues = 0;
	ul_EspionTacheEcranNbTramesIncorrectesRecues = 0;
	ul_EspionTacheEcranNbTramesStatus = 0;
	ul_EspionTacheEcranNbTramesLectureDiscrete = 0;
	ul_EspionTacheEcranNbTramesEcritureDiscrete = 0;
	ul_EspionTacheEcranNbTramesLectureBloc = 0;
	ul_EspionTacheEcranNbTramesEcritureBloc = 0;
	ul_EspionTacheEcranNbRAZBufferReceptionSurTimeOUT = 0;
	ul_EspionTacheEcranNbDeclenchementTimeOUT = 0;
	uc_EspionTacheEcranEtat = 0;
	ul_EspionTacheEcranCompteurActivite = 0;
	ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur1 = 0;
	ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur3 = 0;
	ul_EspionTacheEcranEnteteTrameIncorrecte = 0;
}

void vd_EspionRS_Afficher_ErreursDialogueBA(unsigned char uc_AffichageForce)
{
	unsigned char l_uc_Compteur;
	unsigned char l_uc_FlagSautLigne;
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	l_uc_FlagSautLigne = 0;
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
		{
			l_us_ValeurEnCours = us_ErreursDialogueBA[l_uc_Compteur];
			if(us_ErreursDialogueBAPrecedent[l_uc_Compteur] != l_us_ValeurEnCours)
			{
				l_uc_FlagSautLigne = 1;
				vd_EspionRS_Printf(uc_ESPION_DIALOGUE_BA_ERREUR,"ErreursDialogueBA[%d] : 0x%X -> 0x%X\n", l_uc_Compteur, us_ErreursDialogueBAPrecedent[l_uc_Compteur], l_us_ValeurEnCours);
				us_ErreursDialogueBAPrecedent[l_uc_Compteur] = l_us_ValeurEnCours;
			}
		}
		if(l_uc_FlagSautLigne != 0)		vd_EspionRS_Printf(uc_ESPION_DIALOGUE_BA_ERREUR,"\n");
	}
	else
	{
		for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
		{
			printf("ErreursDialogueBA[%d] : 0x%X\n", l_uc_Compteur, us_ErreursDialogueBA[l_uc_Compteur]);
		}
		printf("\n");
	}
}
void vd_EspionRS_RAZ_ErreursDialogueBA(void)
{
	unsigned char l_uc_Compteur;
	
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
	{
		us_ErreursDialogueBA[l_uc_Compteur] = us_ERREUR_DIALOGUE_BA_AUCUNE;
		us_ErreursDialogueBAPrecedent[l_uc_Compteur] = us_ERREUR_DIALOGUE_BA_AUCUNE;
	}
}

void vd_EspionRS_Afficher_ErreursTacheBA(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursTacheBA;
		if(l_us_ValeurEnCours != us_ErreursTacheBAPrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"ErreursTacheBA : 0x%X -> 0x%X\n\n", us_ErreursTacheBAPrecedent, l_us_ValeurEnCours);
			us_ErreursTacheBAPrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursTacheBA : 0x%X\n\n", us_ErreursTacheBA);
	}
}
void vd_EspionRS_RAZ_ErreursTacheBA(void)
{
	us_ErreursTacheBA = us_ERREUR_TACHE_BA_AUCUNE;
	us_ErreursTacheBAPrecedent = us_ERREUR_TACHE_BA_AUCUNE;
}

void vd_EspionRS_Afficher_ActiviteTacheBA(void)
{
	unsigned char l_uc_Compteur;
	
	printf("BAEtat : %d\n", uc_EspionTacheBAEtat);
	printf("write_polled : %d\n", uc_Espionsl_fct_write_polled);
	printf("BACompteurActivite : %d\n", ul_EspionTacheBACompteurActivite);
	printf("ul_EspionTacheBANbBlocageI2C : %d\n", ul_EspionTacheBANbBlocageI2C);
	printf("uc_NbEssaisReinitMax : %d\n", uc_NbEssaisReinitMax);
	printf("uc_NbLoupesReinit : %d\n", uc_NbLoupesReinit);
			
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
	{
		printf("CptEmissionI2C[%d] : %d\n", l_uc_Compteur, ul_EspionTacheBACompteurEmissionI2C[l_uc_Compteur]);
		printf("CptErreurI2C[%d] : %d\n", l_uc_Compteur, ul_EspionTacheBACompteurErreurI2C[l_uc_Compteur]);
	}
	printf("\n");
	
	printf("ul_I2C_While_1 : %d\n", ul_I2C_While_1);
	printf("ul_I2C_While_1Max : %d\n", ul_I2C_While_1Max);
	printf("ul_I2C_While_1TimeOut : %d\n", ul_I2C_While_1TimeOut);
	printf("ul_I2C_While_2 : %d\n", ul_I2C_While_2);
	printf("ul_I2C_While_2Max : %d\n", ul_I2C_While_2Max);
	printf("ul_I2C_While_2TimeOut : %d\n", ul_I2C_While_2TimeOut);
	printf("ul_I2C_While_3 : %d\n", ul_I2C_While_3);
	printf("ul_I2C_While_3Max : %d\n", ul_I2C_While_3Max);
	printf("ul_I2C_While_3TimeOut : %d\n", ul_I2C_While_3TimeOut);
	printf("ul_I2C_While_4 : %d\n", ul_I2C_While_4);
	printf("ul_I2C_While_4Max : %d\n", ul_I2C_While_4Max);
	printf("ul_I2C_While_4TimeOut : %d\n", ul_I2C_While_4TimeOut);
	printf("ul_I2C_While_5 : %d\n", ul_I2C_While_5);
	printf("ul_I2C_While_5Max : %d\n", ul_I2C_While_5Max);
	printf("ul_I2C_While_5TimeOut : %d\n", ul_I2C_While_5TimeOut);
}
void vd_EspionRS_RAZ_ActiviteTacheBA(void)
{
	unsigned char l_uc_Compteur;
	
	uc_EspionTacheBAEtat = 0;
	uc_Espionsl_fct_write_polled = 0;
	ul_EspionTacheBACompteurActivite = 0;
	ul_EspionTacheBANbBlocageI2C = 0;
	uc_NbEssaisReinitMax = 0;
	uc_NbLoupesReinit = 0;
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_BOITIER_AUXILIAIRE; l_uc_Compteur++)
	{
		ul_EspionTacheBACompteurEmissionI2C[l_uc_Compteur] = 0;
		ul_EspionTacheBACompteurErreurI2C[l_uc_Compteur] = 0;
	}
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
}

void vd_EspionRS_Afficher_ActiviteI2C(void)
{
	I2C_STATISTICS_STRUCT l_st_Stats;
	signed long l_sl_param;
	
	if(fd_BA_I2C == NULL)
	{
		printf("I2C NULL !\n\n");
	}
	else
	{
		printf("I2C stat :\n");
		if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_GET_STATISTICS, (pointer)&l_st_Stats) == I2C_OK)
		{
			printf ("  IT:  %d\n", l_st_Stats.INTERRUPTS);
			printf ("  Rx:  %d\n", l_st_Stats.RX_PACKETS);
			printf ("  Tx:  %d\n", l_st_Stats.TX_PACKETS);
			printf ("  Tx lost arb: %d\n", l_st_Stats.TX_LOST_ARBITRATIONS);
			printf ("  Tx as slave: %d\n", l_st_Stats.TX_ADDRESSED_AS_SLAVE);
			printf ("  Tx naks:     %d\n", l_st_Stats.TX_NAKS);
		}
		else
		{
			printf("ERROR\n");
		}
		
		printf ("I2C state\n");
		if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_GET_STATE, &l_sl_param) == I2C_OK)
		{
			printf("0x%02x\n", l_sl_param);
		}
		else
		{
			printf("ERROR\n");
		}
		printf("\n");
	}
}
void vd_EspionRS_RAZ_ActiviteI2C(void)
{
	if(fd_BA_I2C == NULL)
	{
		printf("I2C NULL !\n\n");
	}
	else
	{
		if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_CLEAR_STATISTICS, NULL) == I2C_OK)
		{
			printf("I2C RAZ STATS OK\n\n");
		}
		else
		{
			printf("I2C RAZ STATS ERROR\n\n");
		}
	}
}

void vd_EspionRS_Afficher_Scenarios(void)
{
	unsigned char l_uc_Compteur;
	
	printf("Scenarios :\n");
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_SCENARIO_NB; l_uc_Compteur++)
	{
		printf("ScenariosNbExecution[%d] : %d\n", l_uc_Compteur, ul_ScenariosNbExecution[l_uc_Compteur]);
	}
	printf("N° Incorrect : %d\n", ul_ScenariosNumeroIncorrect);
	printf("@ ValeursIncorrectes : %d\n", ul_ScenariosAdressesValeursIncorrectes);
	printf("@ EnDehorsPlage : %d\n", ul_ScenariosAdressesEnDehorsPlage);
	printf("Init @ EnDehorsPlage : %d\n", ul_ScenariosInitAdressesEnDehorsPlage);
	
	printf("\n");
}
void vd_EspionRS_RAZ_Scenarios(void)
{
	unsigned char l_uc_Compteur;
	
	for(l_uc_Compteur = 0; l_uc_Compteur < uc_SCENARIO_NB; l_uc_Compteur++)
	{
		ul_ScenariosNbExecution[l_uc_Compteur] = 0;
	}
	ul_ScenariosNumeroIncorrect = 0;
	ul_ScenariosAdressesValeursIncorrectes = 0;
	ul_ScenariosAdressesEnDehorsPlage = 0;
	ul_ScenariosInitAdressesEnDehorsPlage = 0;
}

void vd_EspionRS_Afficher_ErreursAlarme(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursAlarme;
		if(l_us_ValeurEnCours != us_ErreursAlarmePrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_ALARME_ERREURS,"ErreursAlarme : 0x%X -> 0x%X\n\n", us_ErreursAlarmePrecedent, l_us_ValeurEnCours);
			us_ErreursAlarmePrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursAlarme : 0x%X\n\n", us_ErreursAlarme);
	}
}
void vd_EspionRS_RAZ_ErreursAlarme(void)
{
	us_ErreursAlarme = us_ERREUR_ALARME_AUCUNE;
	us_ErreursAlarmePrecedent = us_ERREUR_ALARME_AUCUNE;
}

void vd_EspionRS_Afficher_ActiviteAlarme(void)
{
	printf("---\n");
	
	printf("\n");
}
void vd_EspionRS_RAZ_ActiviteAlarme(void)
{
}

void vd_EspionRS_Afficher_ErreursTachePrincipale(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursTachePrincipale;
		if(l_us_ValeurEnCours != us_ErreursTachePrincipalePrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"ErreursMain : 0x%X -> 0x%X\n\n", us_ErreursTachePrincipalePrecedent, l_us_ValeurEnCours);
			us_ErreursTachePrincipalePrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursMain : 0x%X\n\n", us_ErreursTachePrincipale);
	}
}
void vd_EspionRS_RAZ_ErreursTachePrincipale(void)
{
	us_ErreursTachePrincipale = us_ERREUR_TACHE_PRINCIPALE_AUCUNE;
	us_ErreursTachePrincipalePrecedent = us_ERREUR_TACHE_PRINCIPALE_AUCUNE;
}

void vd_EspionRS_Afficher_ActiviteTachePrincipale(void)
{
	printf("Main :\n");
	printf("Etat : %d\n", uc_EspionTachePrincipaleEtat);
	printf("CptActivite : %d\n", ul_EspionTachePrincipaleCompteurActivite);
	printf("NbMessagesBAEnvoyes : %d\n", ul_EspionTachePrincipaleNbMessagesBAEnvoyes);
	printf("NbMessagesBAErreurs1 : %d\n", ul_EspionTachePrincipaleNbMessagesBAErreurs1);
	printf("NbMessagesBAErreurs2 : %d\n", ul_EspionTachePrincipaleNbMessagesBAErreurs2);
	printf("NbMessagesBAErreurs3 : %d\n", ul_EspionTachePrincipaleNbMessagesBAErreurs3);
	printf("NbMessagesBARecus : %d\n", ul_EspionTachePrincipaleNbMessagesBARecus);
	//printf("AcqAINEnCours : %d\n", ul_EspionTachePrincipaleAcquisitionAINEnCours);
	//printf("AcqAINOK : %d\n", ul_EspionTachePrincipaleAcquisitionAINOK);
	printf("CoupuresCourant : %d\n", ul_EspionTachePrincipaleNbCoupuresCourant);
	printf("BatteriePresente : %d\n", uc_BatteriePresente);
	printf("NbDeclTimer50ms : %d\n", ul_EspionTachePrincipaleNbDeclenchementTimer50ms);
	printf("NbDeclTimer1sec : %d\n", ul_EspionTachePrincipaleNbDeclenchementTimer1sec);
	printf("TableEchangeAcces NbEcritureRefuse : %d\n", ul_EspionTableEchangeNbAccesEcritureRefuse);
	printf("TableEchangeAcces NbLectureRefuse : %d\n", ul_EspionTableEchangeNbAccesLectureRefuse);
		
	printf("\n");
}
void vd_EspionRS_RAZ_ActiviteTachePrincipale(void)
{
	uc_EspionTachePrincipaleEtat = 0;
	ul_EspionTachePrincipaleCompteurActivite = 0;
	ul_EspionTachePrincipaleNbMessagesBAEnvoyes = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs1 = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs2 = 0;
	ul_EspionTachePrincipaleNbMessagesBAErreurs3 = 0;
	ul_EspionTachePrincipaleNbMessagesBARecus = 0;
	//ul_EspionTachePrincipaleAcquisitionAINEnCours = 0;
	//ul_EspionTachePrincipaleAcquisitionAINOK = 0;
	ul_EspionTachePrincipaleNbCoupuresCourant = 0;
	ul_EspionTachePrincipaleNbDeclenchementTimer50ms = 0;
	ul_EspionTachePrincipaleNbDeclenchementTimer1sec = 0;
	ul_EspionTableEchangeNbAccesEcritureRefuse = 0;
	ul_EspionTableEchangeNbAccesLectureRefuse = 0;
}

void vd_EspionRS_Afficher_ErreursRTC(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursRTC;
		if(l_us_ValeurEnCours != us_ErreursRTCPrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"ErreursRTC : 0x%X -> 0x%X\n\n", us_ErreursTachePrincipalePrecedent, l_us_ValeurEnCours);
			us_ErreursRTCPrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursRTC : 0x%X\n\n", us_ErreursRTC);
	}
}
void vd_EspionRS_RAZ_ErreursRTC(void)
{
	us_ErreursRTC = us_ERREUR_RTC_AUCUNE;
	us_ErreursRTCPrecedent = us_ERREUR_RTC_AUCUNE;
}

void vd_EspionRS_Afficher_ActiviteRTC(void)
{
	printf("RTCNbMAJRTC : %d\n", ul_EspionRTCNbMAJRTC);
	printf("RTCNbPassageHeureEte : %d\n", ul_EspionRTCNbPassageHeureEte);
	printf("RTCNbPassageHeureHiver : %d\n", ul_EspionRTCNbPassageHeureHiver);
	
	printf("\n");
}

void vd_EspionRS_RAZ_ActiviteRTC(void)
{
	ul_EspionRTCNbMAJRTC = 0;
	ul_EspionRTCNbPassageHeureEte = 0;
	ul_EspionRTCNbPassageHeureHiver = 0;
}

void vd_EspionRS_Afficher_ErreursHard(unsigned char uc_AffichageForce)
{
	unsigned long l_ul_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_ul_ValeurEnCours = ul_ErreursHard1;
		if(l_ul_ValeurEnCours != ul_ErreursHard1Precedent)
		{
			vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"ErreursHard1 : 0x%X -> 0x%X\n\n", ul_ErreursHard1Precedent, l_ul_ValeurEnCours);
			ul_ErreursHard1Precedent = l_ul_ValeurEnCours;
		}
		l_ul_ValeurEnCours = ul_ErreursHard2;
		if(l_ul_ValeurEnCours != ul_ErreursHard2Precedent)
		{
			vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"ErreursHard2 : 0x%X -> 0x%X\n\n", ul_ErreursHard2Precedent, l_ul_ValeurEnCours);
			ul_ErreursHard2Precedent = l_ul_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursHard1 : 0x%X\n", ul_ErreursHard1);
		printf("ErreursHard2 : 0x%X\n\n", ul_ErreursHard2);
	}
}
void vd_EspionRS_RAZ_ErreursHard(void)
{
	ul_ErreursHard1 = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard1Precedent = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard2 = ul_ERREUR_HARD_AUCUNE;
	ul_ErreursHard2Precedent = ul_ERREUR_HARD_AUCUNE;
}

void vd_EspionRS_Afficher_ErreursChauffage(unsigned char uc_AffichageForce)
{
	unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_us_ValeurEnCours = us_ErreursChauffage;
		if(l_us_ValeurEnCours != us_ErreursChauffagePrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"ErreursChauffage : 0x%X -> 0x%X\n\n", us_ErreursChauffagePrecedent, l_us_ValeurEnCours);
			us_ErreursChauffagePrecedent = l_us_ValeurEnCours;
		}
	}
	else
	{
		printf("ErreursChauffage : 0x%X\n\n", us_ErreursChauffage);
	}
}
void vd_EspionRS_RAZ_ErreursChauffage(void)
{
	us_ErreursChauffage = us_ERREUR_CHAUFFAGE_AUCUNE;
	us_ErreursChauffagePrecedent = us_ERREUR_CHAUFFAGE_AUCUNE;
}

void vd_EspionRS_Afficher_ActiviteChauffage(void)
{
	printf("ul_EspionCompteurITAlternanceSecteur : %d\n", ul_EspionCompteurITAlternanceSecteur);
	
	printf("\n");
}

void vd_EspionRS_RAZ_ActiviteChauffage(void)
{
	ul_EspionCompteurITAlternanceSecteur = 0;
}

void vd_EspionRS_Afficher_TableEchange(unsigned char uc_AffichageForce)
{
	unsigned char l_uc_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	unsigned short l_us_Compteur;
	unsigned char l_uc_FlagValeurModifee;
	
	l_uc_FlagValeurModifee = 0;
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees;l_us_Compteur++)
		{
			l_uc_ValeurEnCours = Tb_Echange[l_us_Compteur];
			if(l_uc_ValeurEnCours != Tb_EchangePrecedent[l_us_Compteur])
			{
				l_uc_FlagValeurModifee = 1;
				vd_EspionRS_Printf(uc_ESPION_TABLE_ECHANGE,"Tb_Echange[%03D %s] : 0x%X -> 0x%X\n", l_us_Compteur, puc_AfficherLibelleTableEchange(l_us_Compteur), Tb_EchangePrecedent[l_us_Compteur], l_uc_ValeurEnCours);
				Tb_EchangePrecedent[l_us_Compteur] = l_uc_ValeurEnCours;
			}
		}
		if(l_uc_FlagValeurModifee != 0)	vd_EspionRS_Printf(uc_ESPION_TABLE_ECHANGE,"");
	}
	else
	{
		vd_PrintTableEchange();
	}
}

void vd_EspionRS_Afficher_EtatBP(unsigned char uc_AffichageForce)
{
	unsigned char l_uc_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	

	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_uc_ValeurEnCours = st_EchangeStatus.uc_AlarmeActivee;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_AlarmeActivee)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status AlarmeActivee : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_AlarmeActivee, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_AlarmeActivee = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_AlarmeDeclenchee;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_AlarmeDeclenchee)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status AlarmeDeclenchee : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_AlarmeDeclenchee, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_AlarmeDeclenchee = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_AlarmeProcedureSortieEnCours;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_AlarmeProcedureSortieEnCours)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status AlarmeProcedureSortieEnCours : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_AlarmeProcedureSortieEnCours, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_AlarmeProcedureSortieEnCours = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_AlarmeProcedureRentreeEnCours)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status AlarmeProcedureRentreeEnCours : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_AlarmeProcedureRentreeEnCours, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_AlarmeProcedureRentreeEnCours = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_EcranTimeOUT;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_EcranTimeOUT)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status EcranTimeOUT : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_EcranTimeOUT, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_EcranTimeOUT = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_DinEcranOuvert;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_DinEcranOuvert)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status DinEcranOuvert : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_DinEcranOuvert, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_DinEcranOuvert = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_BloquerVolets;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_BloquerVolets)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status BloqueVolets : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_BloquerVolets, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_BloquerVolets = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_ForcerAllumage;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_ForcerAllumage)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status ForcerAllumage : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_ForcerAllumage, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_ForcerAllumage = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_SurConsommation;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_SurConsommation)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status SurConsommation : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_SurConsommation, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_SurConsommation = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_HeuresCreuses;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_HeuresCreuses)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status HeuresCreuses : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_HeuresCreuses, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_HeuresCreuses = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_Secouru;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_Secouru)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status Secouru : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_Secouru, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_Secouru = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_FuiteLL;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_FuiteLL)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status FuiteLL : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_FuiteLL, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_FuiteLL = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_FuiteLV;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_FuiteLV)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status FuiteLV : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_FuiteLV, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_FuiteLV = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_DefTeleinfo;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_DefTeleinfo)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status uc_DefTeleinfo : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_DefTeleinfo, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_DefTeleinfo = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_Defboitier1;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_Defboitier1)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status Defboitier1 : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_Defboitier1, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_Defboitier1 = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_Defboitier2;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_Defboitier2)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status Defboitier2 : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_Defboitier2, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_Defboitier2 = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_Defboitier3;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_Defboitier3)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status Defboitier3 : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_Defboitier3, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_Defboitier3 = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_SécuOFF;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_SécuOFF)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status SécuOFF : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_SécuOFF, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_SécuOFF = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_MachinesOFF;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_MachinesOFF)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status MachinesOFF : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_MachinesOFF, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_MachinesOFF = l_uc_ValeurEnCours;
		}

		l_uc_ValeurEnCours = st_EchangeStatus.uc_EtatEthernet;
		if(l_uc_ValeurEnCours != st_EchangeStatusPrecedent.uc_EtatEthernet)
		{
			vd_EspionRS_Printf(uc_ESPION_ETAT_BP,"Status uc_EtatEthernet : 0x%X -> 0x%X\n", st_EchangeStatusPrecedent.uc_EtatEthernet, l_uc_ValeurEnCours);
			st_EchangeStatusPrecedent.uc_EtatEthernet = l_uc_ValeurEnCours;
		}
	}
	else
	{
		printf("Status :\n");
		printf("AlarmeActivee : 0x%X\n", st_EchangeStatus.uc_AlarmeActivee);
		printf("AlarmeDeclenchee : 0x%X\n", st_EchangeStatus.uc_AlarmeDeclenchee);
		printf("AlarmeProcedureSortieEnCours : 0x%X\n", st_EchangeStatus.uc_AlarmeProcedureSortieEnCours);
		printf("AlarmeProcedureRentreeEnCours : 0x%X\n", st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours);
		printf("EcranTimeOUT : 0x%X\n", st_EchangeStatus.uc_EcranTimeOUT);
		printf("DinEcranOuvert : 0x%X\n", st_EchangeStatus.uc_DinEcranOuvert);
		printf("BloqueVolets : 0x%X\n", st_EchangeStatus.uc_BloquerVolets);
		printf("ForcerAllumage : 0x%X\n", st_EchangeStatus.uc_ForcerAllumage);
		printf("SurConsommation : 0x%X\n", st_EchangeStatus.uc_SurConsommation);
		printf("HeuresCreuses : 0x%X\n", st_EchangeStatus.uc_HeuresCreuses);
		printf("Secouru : 0x%X\n", st_EchangeStatus.uc_Secouru);
		printf("FuiteLL : 0x%X\n", st_EchangeStatus.uc_FuiteLL);
		printf("FuiteLV : 0x%X\n", st_EchangeStatus.uc_FuiteLV);
		printf("DefTeleinfo : 0x%X\n", st_EchangeStatus.uc_DefTeleinfo);
		printf("Defboitier1 : 0x%X\n", st_EchangeStatus.uc_Defboitier1);
		printf("Defboitier2 : 0x%X\n", st_EchangeStatus.uc_Defboitier2);
		printf("Defboitier3 : 0x%X\n", st_EchangeStatus.uc_Defboitier3);
		printf("SécuOFF : 0x%X\n", st_EchangeStatus.uc_SécuOFF);
		printf("MachinesOFF : 0x%X\n", st_EchangeStatus.uc_MachinesOFF);
		printf("uc_EtatEthernet : 0x%X\n", st_EchangeStatus.uc_EtatEthernet);
	}
}

void vd_EspionRS_Afficher_ES(unsigned char uc_AffichageForce)
{
	unsigned char l_uc_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	unsigned char l_uc_FlagValeurModifiee;
	
	l_uc_FlagValeurModifiee = 0;
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureSireneInterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_OuvertureSireneInterieurePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN OuvSireneInt : %X\n", l_uc_ValeurEnCours);
			uc_DIN_OuvertureSireneInterieurePrecedent = l_uc_ValeurEnCours;
		}		
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureSireneExterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_OuvertureSireneExterieurePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN OuvSireneExt : %X\n", l_uc_ValeurEnCours);
			uc_DIN_OuvertureSireneExterieurePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureTableauDominique) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_OuvertureTableauDominiquePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN OuvTabDom : %X\n", l_uc_ValeurEnCours);
			uc_DIN_OuvertureTableauDominiquePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_Sirene_Exterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_Sirene_ExterieurePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT SireneExt : %X\n", l_uc_ValeurEnCours);
			uc_Cde_Sirene_ExterieurePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_15VSP_AlimBA) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_15VSP_AlimBAPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT 15VSP : %X\n", l_uc_ValeurEnCours);
			uc_Cde_15VSP_AlimBAPrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_VanneArrosage) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_VanneArrosagePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT EVArrosage : %X\n", l_uc_ValeurEnCours);
			uc_Cde_VanneArrosagePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_PriseSecurite) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_PriseSecuritePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT PriseSecurite : %X\n", l_uc_ValeurEnCours);
			uc_Cde_PriseSecuritePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_MachineALaver) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_MachineALaverPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT MachineALaver : %X\n", l_uc_ValeurEnCours);
			uc_Cde_MachineALaverPrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_Cumulus) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_Cde_CumulusPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT Cumulus : %X\n", l_uc_ValeurEnCours);
			uc_Cde_CumulusPrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_Ouverture) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_Detection_OuverturePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN DetectOuv : %X\n", l_uc_ValeurEnCours);
			uc_DIN_Detection_OuverturePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_presence1) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_Detection_presence1Precedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN DetectPres1 : %X\n", l_uc_ValeurEnCours);
			uc_DIN_Detection_presence1Precedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_presence2) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_Detection_presence2Precedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN DetectPres2 : %X\n", l_uc_ValeurEnCours);
			uc_DIN_Detection_presence2Precedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_Pluie) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_Detection_PluiePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN DetectPluie : %X\n", l_uc_ValeurEnCours);
			uc_DIN_Detection_PluiePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Etat_AlimPrincipale) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_Etat_AlimPrincipalePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN AlimPrinc : %X\n", l_uc_ValeurEnCours);
			uc_DIN_Etat_AlimPrincipalePrecedent = l_uc_ValeurEnCours;
		}
		
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_EcranDirection) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DOUT_EcranDirectionPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DOUT EcranDir : %X\n", l_uc_ValeurEnCours);
			uc_DOUT_EcranDirectionPrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence1) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_OuvertureDecteurPresence1Precedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN OuvDetPres1 : %X\n", l_uc_ValeurEnCours);
			uc_DIN_OuvertureDecteurPresence1Precedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence2) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_OuvertureDecteurPresence2Precedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN OuvDetPres2 : %X\n", l_uc_ValeurEnCours);
			uc_DIN_OuvertureDecteurPresence2Precedent = l_uc_ValeurEnCours;
		}
		/*l_uc_ValeurEnCours = 0;	//xxxENTREE ANEMMOMETRE -> GENERE BEAUCOUP DE LOG !!!
		if(lwgpio_get_value(&IO_DIN_VITESSE_VENT) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_VitesseVentPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN VITESSE VENT : %X\n", l_uc_ValeurEnCours);
			uc_DIN_VitesseVentPrecedent = l_uc_ValeurEnCours;
		}*/
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_BOUTON_MAGIQUE) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_BoutonMagiquePrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN BOUTON MAGIQUE : %X\n", l_uc_ValeurEnCours);
			uc_DIN_BoutonMagiquePrecedent = l_uc_ValeurEnCours;
		}
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_ErreurEcranHard) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		if(l_uc_ValeurEnCours != uc_DIN_ErreurEcranHardPrecedent)
		{
			l_uc_FlagValeurModifiee = 1;
			vd_EspionRS_Printf(uc_ESPION_ES,"DIN ErreurEcranHard : %X\n", l_uc_ValeurEnCours);
			uc_DIN_ErreurEcranHardPrecedent = l_uc_ValeurEnCours;
		}
		if(l_uc_FlagValeurModifiee != 0)	vd_EspionRS_Printf(uc_ESPION_ES,"");
		
		if(uc_CompteurEspionAIN_sec >= uc_TEMPO_ESPION_AIN_sec)
		{
			uc_CompteurEspionAIN_sec = 0;
			vd_EspionRS_Printf(uc_ESPION_ES,"AIN4_VBat     : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN4_VBat.us_AINBrut, st_AIN4_VBat.us_AINMin, st_AIN4_VBat.us_AINMax);
			vd_EspionRS_Printf(uc_ESPION_ES,"AIN5_FuiteLV  : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN5_FuiteLV.us_AINBrut, st_AIN5_FuiteLV.us_AINMin, st_AIN5_FuiteLV.us_AINMax);
			vd_EspionRS_Printf(uc_ESPION_ES,"AIN6_FuiteLL  : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN6_FuiteLL.us_AINBrut, st_AIN6_FuiteLL.us_AINMin, st_AIN6_FuiteLL.us_AINMax);
			vd_EspionRS_Printf(uc_ESPION_ES,"");
		}
	}
	else
	{
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureSireneInterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN OuvSireneInt : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureSireneExterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN OuvSireneExt : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureTableauDominique) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN OuvTabDom : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_Sirene_Exterieure) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT SireneExt : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_15VSP_AlimBA) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT 15VSP : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_VanneArrosage) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT EVArrosage : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_PriseSecurite) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT PriseSecurite : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_MachineALaver) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT MachineALaver : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_Cumulus) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT Cumulus : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_Ouverture) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN DetectOuv : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_presence1) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN DetectPres1 : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_presence2) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN DetectPres2 : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Detection_Pluie) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN DetectPluie : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_Etat_AlimPrincipale) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN AlimPrinc : %X\n", l_uc_ValeurEnCours);
		
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DOUT_EcranDirection) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DOUT EcranDir : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence1) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN OuvDetPres1 : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_OuvertureDecteurPresence2) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN OuvDetPres2 : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_VITESSE_VENT) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN Vitesse vent : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_BOUTON_MAGIQUE) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN Bouton magique : %X\n", l_uc_ValeurEnCours);
		l_uc_ValeurEnCours = 0;
		if(lwgpio_get_value(&IO_DIN_ErreurEcranHard) == LWGPIO_VALUE_HIGH)	l_uc_ValeurEnCours = 1;
		printf("DIN ErreurEcranHard : %X\n", l_uc_ValeurEnCours);
		printf("\n");
		
		printf("AIN4_VBat     : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN4_VBat.us_AINBrut, st_AIN4_VBat.us_AINMin, st_AIN4_VBat.us_AINMax);
		printf("AIN5_FuiteLV  : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN5_FuiteLV.us_AINBrut, st_AIN5_FuiteLV.us_AINMin, st_AIN5_FuiteLV.us_AINMax);
		printf("AIN6_FuiteLL  : BRUT -> %4d MIN -> %4d MAX -> %4d\n", st_AIN6_FuiteLL.us_AINBrut, st_AIN6_FuiteLL.us_AINMin, st_AIN6_FuiteLL.us_AINMax);
		printf("\n");
	}
	
	// Non traité par l'espion :
	// IO_Cde_LEDEtatBP, IO_Cde_TeleInfo_Led
	// IO_Cde_FilPilote_ZJ, IO_Cde_FilPilote_ZN, IO_Cde_FilPilote_SDB1, IO_Cde_FilPilote_SDB2, IO_DIN_Secteur_Synchro
}

void vd_PrintValeurTableEchange(unsigned short us_Indice)
{
	if(us_Indice == Alarme_CodeUser1LSB ||
	   us_Indice == Alarme_CodeUser1MSB ||
	   us_Indice == Alarme_CodeUser2LSB ||
	   us_Indice == Alarme_CodeUser2MSB)
	{
		printf("Tb_Echange[%03D : %s] : XXX\n",us_Indice,puc_AfficherLibelleTableEchange(us_Indice));
	}
	else
	{
		printf("Tb_Echange[%03D : %s] : %d.0x%02X\n",us_Indice,puc_AfficherLibelleTableEchange(us_Indice),Tb_Echange[us_Indice],Tb_Echange[us_Indice]);
	}
}

void vd_PrintValeurTableEchangePlanningChauffage(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : %s] : PLANNING CHAUFFAGE\n", us_Indice, pc_Libelle);
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_PLANNING_CHAUFFAGE_TAILLE;l_uc_Compteur++)
	{
		if((l_uc_Compteur % 8) == 0)	printf("\t");
		printf("%d.0x%02X ",Tb_Echange[us_Indice+l_uc_Compteur],Tb_Echange[us_Indice+l_uc_Compteur]);
		if((l_uc_Compteur % 8) == 7)	printf("\n");
	}
	printf("\n");
}

void vd_PrintValeurTableEchangeConfigAlarme(const char *pc_Libelle, unsigned short us_Indice, unsigned char uc_ConfigScenario)
{
	unsigned short l_us_Compteur;
		
	if(uc_ConfigScenario != 0)	printf("\t");	printf("Tb_Echange[%03D : %s] : CONFIG ALARME\n", us_Indice, pc_Libelle);
	
	for(l_us_Compteur = 0;l_us_Compteur < AlarmeConfig_NB_VALEURS;l_us_Compteur++)
	{
		if(uc_ConfigScenario != 0)	printf("\t");
		printf("\t%d : %s",us_Indice+l_us_Compteur, puc_AfficherLibelleConfigAlarme(l_us_Compteur));
		printf("\t%d.0x%02X\n",Tb_Echange[us_Indice+l_us_Compteur],Tb_Echange[us_Indice+l_us_Compteur]);
	}
}

void vd_PrintValeurTableEchangePlanningArrosage(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : %s] : PLANNING ARROSAGE\n", us_Indice, pc_Libelle);
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_PLANNING_ARROSAGE_TAILLE;l_uc_Compteur++)
	{
		if((l_uc_Compteur % 8) == 0)	printf("\t");
		printf("%d.0x%02X ",Tb_Echange[us_Indice+l_uc_Compteur],Tb_Echange[us_Indice+l_uc_Compteur]);
		if((l_uc_Compteur % 8) == 7)	printf("\n");
	}
	printf("\n");
}

void vd_PrintValeurTableEchangeVariateurs(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : %s] : CONFIG VARIATEURS\n", us_Indice, pc_Libelle);
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_VARIATEURS_POSSIBLES_PAR_BA;l_uc_Compteur++)
	{
		printf("\t%d.0x%02X",Tb_Echange[us_Indice+l_uc_Compteur],Tb_Echange[us_Indice+l_uc_Compteur]);
	}
	printf("\n");
}

void vd_PrintValeurTableEchangeLampes(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : %s] : CONFIG LAMPES\n", us_Indice, pc_Libelle);
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_LAMPES_POSSIBLES_PAR_BA;l_uc_Compteur++)
	{
		printf("\t%d.0x%02X",Tb_Echange[us_Indice+l_uc_Compteur],Tb_Echange[us_Indice+l_uc_Compteur]);
	}
	printf("\n");
}

void vd_PrintValeurTableEchangeVolets(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : %s] : CONFIG VOLETS\n", us_Indice, pc_Libelle);
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NB_VOLETS_POSSIBLES_PAR_BA;l_uc_Compteur++)
	{
		printf("\t%d.0x%02X",Tb_Echange[us_Indice+l_uc_Compteur],Tb_Echange[us_Indice+l_uc_Compteur]);
	}
	printf("\n");
}

void vd_PrintValeurTableEchangeCleAccesDistance(void)
{
	unsigned char l_uc_Compteur;
	
	printf("Tb_Echange[%03D : CLE ACCES DISTANCE] : ", Cle_Acces_Distance);
	for(l_uc_Compteur = 0;l_uc_Compteur < Cle_Acces_Distance_TAILLE;l_uc_Compteur++)
	{
		//printf("%d%d.",(Tb_Echange[Cle_Acces_Distance+l_uc_Compteur] & 0x0F),((Tb_Echange[Cle_Acces_Distance+l_uc_Compteur] >> 4) & 0x0F));
		printf("XX.");
	}
	printf("\n");
}

const char *puc_LibelleScenario(unsigned short us_Indice)
{
	if(us_Indice < Scenario2)		return "RESERVE ACCES DISTANCE";
	else if(us_Indice < Scenario3)	return "JE SORS";
	else if(us_Indice < Scenario4)	return "JE PARS EN VOYAGE";
	else if(us_Indice < Scenario5)	return "JE RENTRE";
	else if(us_Indice < Scenario6)	return "JE VAIS ME COUCHER";
	else if(us_Indice < Scenario7)	return "JE ME LEVE";
	else if(us_Indice < Scenario8)	return "PERSONNALISE 1";
	else 							return "PERSONNALISE 2";
}

void vd_PrintValeurTableEchangeScenario(const char *pc_Libelle, unsigned short us_Indice)
{
	printf("Tb_Echange[%03D : %s] : SCENARIO %s\n", us_Indice, pc_Libelle, puc_LibelleScenario(us_Indice));
	
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Confirme_Scenario,puc_AfficherLibelleConfigScenario(Scenario_Confirme_Scenario),Tb_Echange[us_Indice+Scenario_Confirme_Scenario],Tb_Echange[us_Indice+Scenario_Confirme_Scenario]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Alarme_ON,puc_AfficherLibelleConfigScenario(Scenario_Alarme_ON),Tb_Echange[us_Indice+Scenario_Alarme_ON],Tb_Echange[us_Indice+Scenario_Alarme_ON]);
	vd_PrintValeurTableEchangeConfigAlarme("Scenario_AlarmeConfig", us_Indice+Scenario_AlarmeConfig,1);

	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_PDV_LSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_PDV_LSB),Tb_Echange[us_Indice+Scenario_Eteindre_PDV_LSB],Tb_Echange[us_Indice+Scenario_Eteindre_PDV_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_PDV_MSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_PDV_MSB),Tb_Echange[us_Indice+Scenario_Eteindre_PDV_MSB],Tb_Echange[us_Indice+Scenario_Eteindre_PDV_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_CHB_LSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_CHB_LSB),Tb_Echange[us_Indice+Scenario_Eteindre_CHB_LSB],Tb_Echange[us_Indice+Scenario_Eteindre_CHB_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_CHB_MSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_CHB_MSB),Tb_Echange[us_Indice+Scenario_Eteindre_CHB_MSB],Tb_Echange[us_Indice+Scenario_Eteindre_CHB_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_PDE_LSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_PDE_LSB),Tb_Echange[us_Indice+Scenario_Eteindre_PDE_LSB],Tb_Echange[us_Indice+Scenario_Eteindre_PDE_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Eteindre_PDE_MSB,puc_AfficherLibelleConfigScenario(Scenario_Eteindre_PDE_MSB),Tb_Echange[us_Indice+Scenario_Eteindre_PDE_MSB],Tb_Echange[us_Indice+Scenario_Eteindre_PDE_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_PDV_LSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_PDV_LSB),Tb_Echange[us_Indice+Scenario_Allumer_PDV_LSB],Tb_Echange[us_Indice+Scenario_Allumer_PDV_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_PDV_MSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_PDV_MSB),Tb_Echange[us_Indice+Scenario_Allumer_PDV_MSB],Tb_Echange[us_Indice+Scenario_Allumer_PDV_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_CHB_LSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_CHB_LSB),Tb_Echange[us_Indice+Scenario_Allumer_CHB_LSB],Tb_Echange[us_Indice+Scenario_Allumer_CHB_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_CHB_MSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_CHB_MSB),Tb_Echange[us_Indice+Scenario_Allumer_CHB_MSB],Tb_Echange[us_Indice+Scenario_Allumer_CHB_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_PDE_LSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_PDE_LSB),Tb_Echange[us_Indice+Scenario_Allumer_PDE_LSB],Tb_Echange[us_Indice+Scenario_Allumer_PDE_LSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Allumer_PDE_MSB,puc_AfficherLibelleConfigScenario(Scenario_Allumer_PDE_MSB),Tb_Echange[us_Indice+Scenario_Allumer_PDE_MSB],Tb_Echange[us_Indice+Scenario_Allumer_PDE_MSB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_OuvrirVolets_PDV,puc_AfficherLibelleConfigScenario(Scenario_OuvrirVolets_PDV),Tb_Echange[us_Indice+Scenario_OuvrirVolets_PDV],Tb_Echange[us_Indice+Scenario_OuvrirVolets_PDV]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_OuvrirVolets_CHB,puc_AfficherLibelleConfigScenario(Scenario_OuvrirVolets_CHB),Tb_Echange[us_Indice+Scenario_OuvrirVolets_CHB],Tb_Echange[us_Indice+Scenario_OuvrirVolets_CHB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_OuvrirVolets_PDE,puc_AfficherLibelleConfigScenario(Scenario_OuvrirVolets_PDE),Tb_Echange[us_Indice+Scenario_OuvrirVolets_PDE],Tb_Echange[us_Indice+Scenario_OuvrirVolets_PDE]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_FermerVolets_PDV,puc_AfficherLibelleConfigScenario(Scenario_FermerVolets_PDV),Tb_Echange[us_Indice+Scenario_FermerVolets_PDV],Tb_Echange[us_Indice+Scenario_FermerVolets_PDV]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_FermerVolets_CHB,puc_AfficherLibelleConfigScenario(Scenario_FermerVolets_CHB),Tb_Echange[us_Indice+Scenario_FermerVolets_CHB],Tb_Echange[us_Indice+Scenario_FermerVolets_CHB]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_FermerVolets_PDE,puc_AfficherLibelleConfigScenario(Scenario_FermerVolets_PDE),Tb_Echange[us_Indice+Scenario_FermerVolets_PDE],Tb_Echange[us_Indice+Scenario_FermerVolets_PDE]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Securite,puc_AfficherLibelleConfigScenario(Scenario_Securite),Tb_Echange[us_Indice+Scenario_Securite],Tb_Echange[us_Indice+Scenario_Securite]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Machines,puc_AfficherLibelleConfigScenario(Scenario_Machines),Tb_Echange[us_Indice+Scenario_Machines],Tb_Echange[us_Indice+Scenario_Machines]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Chauf_zj,puc_AfficherLibelleConfigScenario(Scenario_Chauf_zj),Tb_Echange[us_Indice+Scenario_Chauf_zj],Tb_Echange[us_Indice+Scenario_Chauf_zj]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Chauf_zn,puc_AfficherLibelleConfigScenario(Scenario_Chauf_zn),Tb_Echange[us_Indice+Scenario_Chauf_zn],Tb_Echange[us_Indice+Scenario_Chauf_zn]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Chauf_zsb1,puc_AfficherLibelleConfigScenario(Scenario_Chauf_zsb1),Tb_Echange[us_Indice+Scenario_Chauf_zsb1],Tb_Echange[us_Indice+Scenario_Chauf_zsb1]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Chauf_zsb2,puc_AfficherLibelleConfigScenario(Scenario_Chauf_zsb2),Tb_Echange[us_Indice+Scenario_Chauf_zsb2],Tb_Echange[us_Indice+Scenario_Chauf_zsb2]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Cumulus,puc_AfficherLibelleConfigScenario(Scenario_Cumulus),Tb_Echange[us_Indice+Scenario_Cumulus],Tb_Echange[us_Indice+Scenario_Cumulus]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Reveil_Reglage,puc_AfficherLibelleConfigScenario(Scenario_Reveil_Reglage),Tb_Echange[us_Indice+Scenario_Reveil_Reglage],Tb_Echange[us_Indice+Scenario_Reveil_Reglage]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Reveil_ON,puc_AfficherLibelleConfigScenario(Scenario_Reveil_ON),Tb_Echange[us_Indice+Scenario_Reveil_ON],Tb_Echange[us_Indice+Scenario_Reveil_ON]);
	printf("\t[%03D]%s : %d.0x%02X\n", us_Indice+Scenario_Efface,puc_AfficherLibelleConfigScenario(Scenario_Efface),Tb_Echange[us_Indice+Scenario_Efface],Tb_Echange[us_Indice+Scenario_Efface]);
}

void vd_PrintTableEchange(void)
{
	vd_PrintValeurTableEchange(Version_SoftBP_Embedded);
	vd_PrintValeurTableEchange(Version_SoftBP_Web);
	vd_PrintValeurTableEchange(Version_SoftIHM_Majeur);
	vd_PrintValeurTableEchange(Version_SoftIHM_Mineur);
	vd_PrintValeurTableEchange(Version_TableEchange);
	vd_PrintValeurTableEchange(Minutes);
	vd_PrintValeurTableEchange(Heure);
	vd_PrintValeurTableEchange(Jour);
	vd_PrintValeurTableEchange(Mois);
	vd_PrintValeurTableEchange(Annee);
	vd_PrintValeurTableEchange(Status);
	vd_PrintValeurTableEchange(Alerte);
	vd_PrintValeurTableEchange(Information);
	vd_PrintValeurTableEchangePlanningChauffage("Chauf_zj_Auto", Chauf_zj_Auto);
	vd_PrintValeurTableEchangePlanningChauffage("Chauf_zn_Auto", Chauf_zn_Auto);
	vd_PrintValeurTableEchangePlanningChauffage("Chauf_zsb1_Auto", Chauf_zsb1_Auto);
	vd_PrintValeurTableEchangePlanningChauffage("Chauf_zsb2_Auto", Chauf_zsb2_Auto);
	vd_PrintValeurTableEchange(Chauf_zj_Mode);
	vd_PrintValeurTableEchange(Chauf_zn_Mode);
	vd_PrintValeurTableEchange(Chauf_zsb1_Mode);
	vd_PrintValeurTableEchange(Chauf_zsb2_Mode);
	vd_PrintValeurTableEchange(Cumulus_Mode);
	vd_PrintValeurTableEchange(VacanceFin_H);
	vd_PrintValeurTableEchange(VacanceFin_Mn);
	vd_PrintValeurTableEchange(VacanceFin_J);
	vd_PrintValeurTableEchange(VacanceFin_M);
	vd_PrintValeurTableEchange(VacanceFin_A);
	vd_PrintValeurTableEchange(VacanceFin_zj_Force);
	vd_PrintValeurTableEchange(VacanceFin_zn_Force);
	vd_PrintValeurTableEchange(VacanceFin_zsb1_Force);
	vd_PrintValeurTableEchange(VacanceFin_zsb2_Force);
	vd_PrintValeurTableEchange(Arrose_Mode);
	vd_PrintValeurTableEchangePlanningArrosage("Arrose_Auto", Arrose_Auto);
	vd_PrintValeurTableEchange(Arrose_Detect);
	vd_PrintValeurTableEchange(Alarme_AccesADistance);
	vd_PrintValeurTableEchange(Alarme_Mode);
	vd_PrintValeurTableEchange(Alarme_Commande);
	vd_PrintValeurTableEchange(Alarme_CodeSaisiLSB);
	vd_PrintValeurTableEchange(Alarme_CodeSaisiMSB);
	vd_PrintValeurTableEchange(Alarme_Autorisation);
	vd_PrintValeurTableEchange(Alarme_SuiviAlarme);
	vd_PrintValeurTableEchange(Alarme_Detection);
	vd_PrintValeurTableEchange(Alarme_Fraude);
	vd_PrintValeurTableEchange(Alarme_SuiviChangementCode);
	vd_PrintValeurTableEchange(Alarme_CodeUser1LSB);
	vd_PrintValeurTableEchange(Alarme_CodeUser1MSB);
	vd_PrintValeurTableEchange(Alarme_CodeUser2LSB);
	vd_PrintValeurTableEchange(Alarme_CodeUser2MSB);
	vd_PrintValeurTableEchange(Alarme_CompteARebours);
	vd_PrintValeurTableEchange(Alarme_Reserve);
	vd_PrintValeurTableEchange(Alarme_TestRAZPresence);
	vd_PrintValeurTableEchange(Alarme_TestSirenes);
	vd_PrintValeurTableEchangeConfigAlarme("AlarmeConfig", AlarmeConfig,0);
	vd_PrintValeurTableEchange(Alerte_Intensite);
	vd_PrintValeurTableEchange(Alerte_Duree);
	vd_PrintValeurTableEchange(Alerte_TestSirene);
	vd_PrintValeurTableEchange(Alerte_Acquit);
	vd_PrintValeurTableEchange(Securite_PriseCoupe);
	vd_PrintValeurTableEchange(Securite_FuiteLinge);
	vd_PrintValeurTableEchange(Securite_FuiteVaisselle);
	vd_PrintValeurTableEchange(Securite_FuiteAlerte);
	vd_PrintValeurTableEchange(Reveil_ChambreGr_H);
	vd_PrintValeurTableEchange(Reveil_ChambreGr_Mn);
	vd_PrintValeurTableEchange(Reveil_ChambreGr_ON);
	vd_PrintValeurTableEchange(Reveil_Chambre1_H);
	vd_PrintValeurTableEchange(Reveil_Chambre1_Mn);
	vd_PrintValeurTableEchange(Reveil_Chambre1_ON);
	vd_PrintValeurTableEchange(Reveil_Chambre2_H);
	vd_PrintValeurTableEchange(Reveil_Chambre2_Mn);
	vd_PrintValeurTableEchange(Reveil_Chambre2_ON);
	vd_PrintValeurTableEchange(Reveil_Chambre3_H);
	vd_PrintValeurTableEchange(Reveil_Chambre3_Mn);
	vd_PrintValeurTableEchange(Reveil_Chambre3_ON);
	vd_PrintValeurTableEchange(Reveil_Bureau_H);
	vd_PrintValeurTableEchange(Reveil_Bureau_Mn);
	vd_PrintValeurTableEchange(Reveil_Bureau_ON);
	vd_PrintValeurTableEchange(Delestage);
	vd_PrintValeurTableEchange(TeleInf_OPTARIF);
	vd_PrintValeurTableEchange(TeleInf_PTEC);
	vd_PrintValeurTableEchange(TeleInf_ADPS);
	vd_PrintValeurTableEchange(TeleInf_PAPP_LSB);
	vd_PrintValeurTableEchange(TeleInf_PAPP_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Global_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Global_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Chauffage_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Chauffage_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Refroid_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Refroid_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_EauChaude_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_EauChaude_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Prises_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Prises_MSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Autres_LSB);
	vd_PrintValeurTableEchange(TeleInf_HPB_Autres_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Global_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Global_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Chauffage_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Chauffage_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Refroid_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Refroid_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_EauChaude_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_EauChaude_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Prises_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Prises_MSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Autres_LSB);
	vd_PrintValeurTableEchange(TeleInf_HC_Autres_MSB);
	vd_PrintValeurTableEchange(TeleInf_Repartition_Chauffage);
	vd_PrintValeurTableEchange(TeleInf_Repartition_Refroid);
	vd_PrintValeurTableEchange(TeleInf_Repartition_EauChaude);
	vd_PrintValeurTableEchange(TeleInf_Repartition_Prises);
	vd_PrintValeurTableEchange(TeleInf_Repartition_Autres);
	vd_PrintValeurTableEchangeVariateurs("Var_PDV_Conf", Variateurs_PDV_Conf);
	vd_PrintValeurTableEchangeVariateurs("Var_CHB_Conf", Variateurs_CHB_Conf);
	vd_PrintValeurTableEchangeVariateurs("Var_PDE_Conf", Variateurs_PDE_Conf);
	vd_PrintValeurTableEchangeLampes("Lampes_PDV_Tps", Lampes_PDV_Temps);
	vd_PrintValeurTableEchangeLampes("Lampes_CHB_Tps", Lampes_CHB_Temps);
	vd_PrintValeurTableEchangeLampes("Lampes_PDE_Tps", Lampes_PDE_Temps);
	vd_PrintValeurTableEchangeVolets("Volets_PDV_Tps", Volets_PDV_Temps);
	vd_PrintValeurTableEchangeVolets("Volets_CHB_Tps", Volets_CHB_Temps);
	vd_PrintValeurTableEchangeVolets("Volets_PDE_Tps", Volets_PDE_Temps);
	vd_PrintValeurTableEchange(Scenario);
	vd_PrintValeurTableEchange(Scenario_DernierLance);
	vd_PrintValeurTableEchangeScenario("Scenario1", Scenario1);
	vd_PrintValeurTableEchangeScenario("Scenario2", Scenario2);
	vd_PrintValeurTableEchangeScenario("Scenario3", Scenario3);
	vd_PrintValeurTableEchangeScenario("Scenario4", Scenario4);
	vd_PrintValeurTableEchangeScenario("Scenario5", Scenario5);
	vd_PrintValeurTableEchangeScenario("Scenario6", Scenario6);
	vd_PrintValeurTableEchangeScenario("Scenario7", Scenario7);
	vd_PrintValeurTableEchangeScenario("Scenario8", Scenario8);
	vd_PrintValeurTableEchange(EtatBP1);
	vd_PrintValeurTableEchange(EtatBP2);
	vd_PrintValeurTableEchangeCleAccesDistance();
	vd_PrintValeurTableEchange(Store_VR);
	vd_PrintValeurTableEchange(Store_Vitesse_Vent_Repliage);
	vd_PrintValeurTableEchange(Store_Vitesse_Vent_Instantane);
	vd_PrintValeurTableEchange(Constructeur_CodeLSB);
	vd_PrintValeurTableEchange(Constructeur_CodeMSB);
	vd_PrintValeurTableEchange(Test_ETOR_1);
	vd_PrintValeurTableEchange(Test_ETOR_2);
	vd_PrintValeurTableEchange(EtatEthernet);
	vd_PrintValeurTableEchange(Mode_Test);
	vd_PrintValeurTableEchange(AdresseMAC_1);
	vd_PrintValeurTableEchange(AdresseMAC_2);
	vd_PrintValeurTableEchange(AdresseMAC_3);
	vd_PrintValeurTableEchange(AdresseMAC_4);
	vd_PrintValeurTableEchange(AdresseMAC_5);
	vd_PrintValeurTableEchange(AdresseMAC_6);
}

unsigned char *puc_AfficherLibelleTableEchange(unsigned short us_Indice)
{
	// Planning chauffage :
	if(us_Indice >= Chauf_zj_Auto && us_Indice < Chauf_zn_Auto)		return puc_AfficherLibelleAvecEspaces2("Chauf_zj_Auto",us_Indice-Chauf_zj_Auto);	else
	if(us_Indice >= Chauf_zn_Auto && us_Indice < Chauf_zsb1_Auto)	return puc_AfficherLibelleAvecEspaces2("Chauf_zn_Auto",us_Indice-Chauf_zn_Auto);	else
	if(us_Indice >= Chauf_zsb1_Auto && us_Indice < Chauf_zsb2_Auto)	return puc_AfficherLibelleAvecEspaces2("Chauf_zsb1_Auto",us_Indice-Chauf_zsb1_Auto);	else
	if(us_Indice >= Chauf_zsb2_Auto && us_Indice < Chauf_zj_Mode)	return puc_AfficherLibelleAvecEspaces2("Chauf_zsb2_Auto",us_Indice-Chauf_zsb2_Auto);	else
		
	if(us_Indice >= Arrose_Auto && us_Indice < Arrose_Detect)	return puc_AfficherLibelleAvecEspaces2("Arrose_Auto",us_Indice-Arrose_Auto);	else
		
	if(us_Indice >= AlarmeConfig && us_Indice < Alerte_Intensite)	return puc_AfficherLibelleConfigAlarme(us_Indice-AlarmeConfig);	else
		
	if(us_Indice >= Variateurs_PDV_Conf && us_Indice < Variateurs_CHB_Conf)	return puc_AfficherLibelleAvecEspaces2("Var_PDV_Conf",us_Indice-Variateurs_PDV_Conf);	else
	if(us_Indice >= Variateurs_CHB_Conf && us_Indice < Variateurs_PDE_Conf)	return puc_AfficherLibelleAvecEspaces2("Var_CHB_Conf",us_Indice-Variateurs_CHB_Conf);	else
	if(us_Indice >= Variateurs_PDE_Conf && us_Indice < Lampes_PDV_Temps)	return puc_AfficherLibelleAvecEspaces2("Var_PDE_Conf",us_Indice-Variateurs_PDE_Conf);	else

	if(us_Indice >= Lampes_PDV_Temps && us_Indice < Lampes_CHB_Temps)	return puc_AfficherLibelleAvecEspaces2("Lampes_PDV_Tps",us_Indice-Lampes_PDV_Temps);	else
	if(us_Indice >= Lampes_CHB_Temps && us_Indice < Lampes_PDE_Temps)	return puc_AfficherLibelleAvecEspaces2("Lampes_CHB_Tps",us_Indice-Lampes_CHB_Temps);	else
	if(us_Indice >= Lampes_PDE_Temps && us_Indice < Volets_PDV_Temps)	return puc_AfficherLibelleAvecEspaces2("Lampes_PDE_Tps",us_Indice-Lampes_PDE_Temps);	else

	if(us_Indice >= Volets_PDV_Temps && us_Indice < Volets_CHB_Temps)	return puc_AfficherLibelleAvecEspaces2("Volets_PDV_Tps",us_Indice-Volets_PDV_Temps);	else
	if(us_Indice >= Volets_CHB_Temps && us_Indice < Volets_PDE_Temps)	return puc_AfficherLibelleAvecEspaces2("Volets_CHB_Tps",us_Indice-Volets_CHB_Temps);	else
	if(us_Indice >= Volets_PDE_Temps && us_Indice < Scenario)			return puc_AfficherLibelleAvecEspaces2("Volets_PDE_Tps",us_Indice-Volets_PDE_Temps);	else
		
	if(us_Indice >= Scenario1 && us_Indice < Scenario2)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario1);	else		
	if(us_Indice >= Scenario2 && us_Indice < Scenario3)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario2);	else
	if(us_Indice >= Scenario3 && us_Indice < Scenario4)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario3);	else
	if(us_Indice >= Scenario4 && us_Indice < Scenario5)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario4);	else
	if(us_Indice >= Scenario5 && us_Indice < Scenario6)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario5);	else
	if(us_Indice >= Scenario6 && us_Indice < Scenario7)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario6);	else
	if(us_Indice >= Scenario7 && us_Indice < Scenario8)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario7);	else
	if(us_Indice >= Scenario8 && us_Indice < EtatBP1)			return puc_AfficherLibelleConfigScenario(us_Indice-Scenario8);	else
		
	if(us_Indice >= Cle_Acces_Distance && us_Indice < Store_VR)			return puc_AfficherLibelleAvecEspaces2("Cle_Acces_Distance",us_Indice-Cle_Acces_Distance);	else

	{		
		switch(us_Indice)
		{
			case Version_SoftBP_Embedded:			return puc_AfficherLibelleAvecEspaces("Version_SoftBP_Embedded");			break;
			case Version_SoftBP_Web:				return puc_AfficherLibelleAvecEspaces("Version_SoftBP_Web");				break;
			case Version_SoftIHM_Majeur:			return puc_AfficherLibelleAvecEspaces("V IHM Majeur");						break;
			case Version_SoftIHM_Mineur:			return puc_AfficherLibelleAvecEspaces("V IHM Mineur");						break;
			case Version_TableEchange:				return puc_AfficherLibelleAvecEspaces("V_TableEchange");					break;
			case Minutes:							return puc_AfficherLibelleAvecEspaces("Min");								break;
			case Heure:								return puc_AfficherLibelleAvecEspaces("H");									break;
			case Jour:								return puc_AfficherLibelleAvecEspaces("J");									break;
			case Mois:								return puc_AfficherLibelleAvecEspaces("M");									break;
			case Annee:								return puc_AfficherLibelleAvecEspaces("A");									break;
			case Status:							return puc_AfficherLibelleAvecEspaces("Status");							break;
			case Alerte:							return puc_AfficherLibelleAvecEspaces("Alerte");							break;
			case Information:						return puc_AfficherLibelleAvecEspaces("Information");						break;
			case Chauf_zj_Mode:						return puc_AfficherLibelleAvecEspaces("Chauf_zj_Mode");					break;
			case Chauf_zn_Mode:						return puc_AfficherLibelleAvecEspaces("Chauf_zn_Mode");					break;
			case Chauf_zsb1_Mode:					return puc_AfficherLibelleAvecEspaces("Chauf_zsb1_Mode");					break;
			case Chauf_zsb2_Mode:					return puc_AfficherLibelleAvecEspaces("Chauf_zsb2_Mode");					break;
			case Cumulus_Mode:						return puc_AfficherLibelleAvecEspaces("Cumulus_Mode");						break;
			case VacanceFin_H:						return puc_AfficherLibelleAvecEspaces("VacanceFin_H");						break;
			case VacanceFin_Mn:						return puc_AfficherLibelleAvecEspaces("VacanceFin_Mn");						break;
			case VacanceFin_J:						return puc_AfficherLibelleAvecEspaces("VacanceFin_J");						break;
			case VacanceFin_M:						return puc_AfficherLibelleAvecEspaces("VacanceFin_M");						break;
			case VacanceFin_A:						return puc_AfficherLibelleAvecEspaces("VacanceFin_A");						break;
			case VacanceFin_zj_Force:				return puc_AfficherLibelleAvecEspaces("VacanceFin_zj_Force");				break;
			case VacanceFin_zn_Force:				return puc_AfficherLibelleAvecEspaces("VacanceFin_zn_Force");				break;
			case VacanceFin_zsb1_Force:				return puc_AfficherLibelleAvecEspaces("VacanceFin_zsb1_Force");				break;
			case VacanceFin_zsb2_Force:				return puc_AfficherLibelleAvecEspaces("VacanceFin_zsb2_Force");				break;
			case Arrose_Mode:						return puc_AfficherLibelleAvecEspaces("Arrose_Mode");						break;
			case Arrose_Detect:						return puc_AfficherLibelleAvecEspaces("Arrose_Detect");						break;
			case Alarme_AccesADistance:				return puc_AfficherLibelleAvecEspaces("Alarme_AccesADistance");				break;
			case Alarme_Mode:						return puc_AfficherLibelleAvecEspaces("Alarme_Mode");						break;
			case Alarme_Commande:					return puc_AfficherLibelleAvecEspaces("Alarme_Commande");					break;
			case Alarme_CodeSaisiLSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeSaisiLSB");				break;
			case Alarme_CodeSaisiMSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeSaisiMSB");				break;
			case Alarme_Autorisation:				return puc_AfficherLibelleAvecEspaces("Alarme_Autorisation");				break;
			case Alarme_SuiviAlarme:				return puc_AfficherLibelleAvecEspaces("Alarme_SuiviAlarme");				break;
			case Alarme_Detection:					return puc_AfficherLibelleAvecEspaces("Alarme_Detection");					break;
			case Alarme_Fraude:						return puc_AfficherLibelleAvecEspaces("Alarme_Fraude");						break;
			case Alarme_SuiviChangementCode:		return puc_AfficherLibelleAvecEspaces("Alarme_SuiviChangementCode");		break;
			case Alarme_CodeUser1LSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeUser1LSB");				break;
			case Alarme_CodeUser1MSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeUser1MSB");				break;
			case Alarme_CodeUser2LSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeUser2LSB");				break;
			case Alarme_CodeUser2MSB:				return puc_AfficherLibelleAvecEspaces("Alarme_CodeUser2MSB");				break;
			case Alarme_CompteARebours:				return puc_AfficherLibelleAvecEspaces("Alarme_CompteARebours");				break;
			case Alarme_Reserve:					return puc_AfficherLibelleAvecEspaces("Alarme_Reserve");					break;
			case Alarme_TestRAZPresence:			return puc_AfficherLibelleAvecEspaces("Alarme_TestRAZPresence");			break;
			case Alarme_TestSirenes:				return puc_AfficherLibelleAvecEspaces("Alarme_TestSirenes");				break;
			case Alerte_Intensite:					return puc_AfficherLibelleAvecEspaces("Alerte_Intensite");					break;
			case Alerte_Duree:						return puc_AfficherLibelleAvecEspaces("Alerte_Duree");						break;
			case Alerte_TestSirene:					return puc_AfficherLibelleAvecEspaces("Alerte_TestSirene");					break;
			case Alerte_Acquit:						return puc_AfficherLibelleAvecEspaces("Alerte_Acquit");						break;
			case Securite_PriseCoupe:				return puc_AfficherLibelleAvecEspaces("Securite_PriseCoupe");				break;
			case Securite_FuiteLinge:				return puc_AfficherLibelleAvecEspaces("Securite_FuiteLinge");				break;
			case Securite_FuiteVaisselle:			return puc_AfficherLibelleAvecEspaces("Securite_FuiteVaisselle");			break;
			case Securite_FuiteAlerte:				return puc_AfficherLibelleAvecEspaces("Securite_FuiteAlerte");				break;
			case Reveil_ChambreGr_H:				return puc_AfficherLibelleAvecEspaces("Reveil_ChambreGr_H");				break;
			case Reveil_ChambreGr_Mn:				return puc_AfficherLibelleAvecEspaces("Reveil_ChambreGr_Mn");				break;
			case Reveil_ChambreGr_ON:				return puc_AfficherLibelleAvecEspaces("Reveil_ChambreGr_ON");				break;
			case Reveil_Chambre1_H:					return puc_AfficherLibelleAvecEspaces("Reveil_Chambre1_H");					break;
			case Reveil_Chambre1_Mn:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre1_Mn");				break;
			case Reveil_Chambre1_ON:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre1_ON");				break;
			case Reveil_Chambre2_H:					return puc_AfficherLibelleAvecEspaces("Reveil_Chambre2_H");					break;
			case Reveil_Chambre2_Mn:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre2_Mn");				break;
			case Reveil_Chambre2_ON:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre2_ON");				break;
			case Reveil_Chambre3_H:					return puc_AfficherLibelleAvecEspaces("Reveil_Chambre3_H");					break;
			case Reveil_Chambre3_Mn:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre3_Mn");				break;
			case Reveil_Chambre3_ON:				return puc_AfficherLibelleAvecEspaces("Reveil_Chambre3_ON");				break;
			case Reveil_Bureau_H:					return puc_AfficherLibelleAvecEspaces("Reveil_Bureau_H");					break;
			case Reveil_Bureau_Mn:					return puc_AfficherLibelleAvecEspaces("Reveil_Bureau_Mn");					break;
			case Reveil_Bureau_ON:					return puc_AfficherLibelleAvecEspaces("Reveil_Bureau_ON");					break;
			case Delestage:							return puc_AfficherLibelleAvecEspaces("Delestage");							break;
			case TeleInf_OPTARIF:					return puc_AfficherLibelleAvecEspaces("TeleInf_OPTARIF");					break;
			case TeleInf_PTEC:						return puc_AfficherLibelleAvecEspaces("TeleInf_PTEC");						break;
			case TeleInf_ADPS:						return puc_AfficherLibelleAvecEspaces("TeleInf_ADPS");						break;
			case TeleInf_PAPP_LSB:					return puc_AfficherLibelleAvecEspaces("TeleInf_PAPP_LSB");					break;
			case TeleInf_PAPP_MSB:					return puc_AfficherLibelleAvecEspaces("TeleInf_PAPP_MSB");					break;
			case TeleInf_HPB_Global_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Global_LSB");			break;
			case TeleInf_HPB_Global_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Global_MSB");			break;
			case TeleInf_HPB_Chauffage_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Chauffage_LSB");			break;
			case TeleInf_HPB_Chauffage_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Chauffage_MSB");			break;
			case TeleInf_HPB_Refroid_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Refroid_LSB");			break;
			case TeleInf_HPB_Refroid_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Refroid_MSB");			break;
			case TeleInf_HPB_EauChaude_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_EauChaude_LSB");			break;
			case TeleInf_HPB_EauChaude_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_EauChaude_MSB");			break;
			case TeleInf_HPB_Prises_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Prises_LSB");			break;
			case TeleInf_HPB_Prises_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Prises_MSB");			break;
			case TeleInf_HPB_Autres_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Autres_LSB");			break;
			case TeleInf_HPB_Autres_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HPB_Autres_MSB");			break;
			case TeleInf_HC_Global_LSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Global_LSB");				break;
			case TeleInf_HC_Global_MSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Global_MSB");				break;
			case TeleInf_HC_Chauffage_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Chauffage_LSB");			break;
			case TeleInf_HC_Chauffage_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Chauffage_MSB");			break;
			case TeleInf_HC_Refroid_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Refroid_LSB");			break;
			case TeleInf_HC_Refroid_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Refroid_MSB");			break;
			case TeleInf_HC_EauChaude_LSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_EauChaude_LSB");			break;
			case TeleInf_HC_EauChaude_MSB:			return puc_AfficherLibelleAvecEspaces("TeleInf_HC_EauChaude_MSB");			break;
			case TeleInf_HC_Prises_LSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Prises_LSB");				break;
			case TeleInf_HC_Prises_MSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Prises_MSB");				break;
			case TeleInf_HC_Autres_LSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Autres_LSB");				break;
			case TeleInf_HC_Autres_MSB:				return puc_AfficherLibelleAvecEspaces("TeleInf_HC_Autres_MSB");				break;
			case TeleInf_Repartition_Chauffage:		return puc_AfficherLibelleAvecEspaces("TeleInf_Repartition_Chauffage");		break;
			case TeleInf_Repartition_Refroid:		return puc_AfficherLibelleAvecEspaces("TeleInf_Repartition_Refroid");		break;
			case TeleInf_Repartition_EauChaude:		return puc_AfficherLibelleAvecEspaces("TeleInf_Repartition_EauChaude");		break;
			case TeleInf_Repartition_Prises:		return puc_AfficherLibelleAvecEspaces("TeleInf_Repartition_Prises");		break;
			case TeleInf_Repartition_Autres:		return puc_AfficherLibelleAvecEspaces("TeleInf_Repartition_Autres");		break;
			case Scenario:							return puc_AfficherLibelleAvecEspaces("Scenario");							break;
			case Scenario_DernierLance:				return puc_AfficherLibelleAvecEspaces("Scenario_DernierLance");				break;
			case EtatBP1:							return puc_AfficherLibelleAvecEspaces("EtatBP1");							break;
			case EtatBP2:							return puc_AfficherLibelleAvecEspaces("EtatBP2");							break;
			case Store_VR:							return puc_AfficherLibelleAvecEspaces("Store_VR");							break;
			case Store_Vitesse_Vent_Repliage:		return puc_AfficherLibelleAvecEspaces("Store_Vitesse_Vent_Repliage");		break;
			case Store_Vitesse_Vent_Instantane:		return puc_AfficherLibelleAvecEspaces("Store_Vitesse_Vent_Instantane");		break;
			case Constructeur_CodeLSB:				return puc_AfficherLibelleAvecEspaces("Constructeur_CodeLSB");				break;
			case Constructeur_CodeMSB:				return puc_AfficherLibelleAvecEspaces("Constructeur_CodeMSB");				break;
			case Test_ETOR_1:						return puc_AfficherLibelleAvecEspaces("Test_ETOR_1");						break;
			case Test_ETOR_2:						return puc_AfficherLibelleAvecEspaces("Test_ETOR_2");						break;
			case EtatEthernet:						return puc_AfficherLibelleAvecEspaces("EtatEthernet");						break;
			case Mode_Test:							return puc_AfficherLibelleAvecEspaces("Mode_Test");							break;
			case AdresseMAC_1:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_1");						break;
			case AdresseMAC_2:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_2");						break;
			case AdresseMAC_3:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_3");						break;
			case AdresseMAC_4:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_4");						break;
			case AdresseMAC_5:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_5");						break;
			case AdresseMAC_6:						return puc_AfficherLibelleAvecEspaces("AdresseMAC_6");						break;
			
			default:
				return puc_AfficherLibelleAvecEspaces("???");
			break;
		}
	}
}

unsigned char *puc_AfficherLibelleConfigAlarme(unsigned short us_Indice)
{
	switch(us_Indice)
	{
		case AlarmeConfig_Code:						return puc_AfficherLibelleAvecEspaces("AlarmeConfig_Code");						break;
		case AlarmeConfig_Detect1:					return puc_AfficherLibelleAvecEspaces("AlarmeConfig_Detect1");					break;
		case AlarmeConfig_Detect2:					return puc_AfficherLibelleAvecEspaces("AlarmeConfig_Detect2");					break;
		case AlarmeConfig_DetectOuv:				return puc_AfficherLibelleAvecEspaces("AlarmeConfig_DetectOuv");				break;
		case AlarmeConfig_Detect1SurVoieAcces:		return puc_AfficherLibelleAvecEspaces("AlarmeConfig_Detect1SurVoieAcces");		break;
		case AlarmeConfig_Detect2SurVoieAcces:		return puc_AfficherLibelleAvecEspaces("AlarmeConfig_Detect2SurVoieAcces");		break;
		case AlarmeConfig_DetectOuvSurVoieAcces:	return puc_AfficherLibelleAvecEspaces("AlarmeConfig_DetectOuvSurVoieAcces");	break;
		case AlarmeConfig_SireneInt:				return puc_AfficherLibelleAvecEspaces("AlarmeConfig_SireneInt");				break;
		case AlarmeConfig_SireneExt:				return puc_AfficherLibelleAvecEspaces("AlarmeConfig_SireneExt");				break;
		case AlarmeConfig_BloqueVolets:				return puc_AfficherLibelleAvecEspaces("AlarmeConfig_BloqueVolets");				break;
		case AlarmeConfig_ForcerEclairage:			return puc_AfficherLibelleAvecEspaces("AlarmeConfig_ForcerEclairage");			break;
		default:
			return puc_AfficherLibelleAvecEspaces("???");
		break;
	}
}

unsigned char *puc_AfficherLibelleConfigScenario(unsigned short us_Indice)
{
	if(us_Indice >= Scenario_AlarmeConfig && us_Indice < Scenario_Eteindre_PDV_LSB)
	{
		return puc_AfficherLibelleConfigAlarme(us_Indice-Scenario_AlarmeConfig);
	}
	else
	{
		switch(us_Indice)
		{
			case Scenario_Confirme_Scenario:			return puc_AfficherLibelleAvecEspaces("Sc_Confirme_Scenario");			break;
			case Scenario_Alarme_ON:					return puc_AfficherLibelleAvecEspaces("Sc_Alarme_ON");					break;
			case Scenario_Eteindre_PDV_LSB:				return puc_AfficherLibelleAvecEspaces("Sc_Eteindre_PDV_LSB");				break;
			case Scenario_Eteindre_PDV_MSB:				return puc_AfficherLibelleAvecEspaces("Sc_Eteindre_PDV_MSB");				break;
			case Scenario_Eteindre_CHB_LSB:				return puc_AfficherLibelleAvecEspaces("Sc_Eteindre_CHB_LSB");				break;
			case Scenario_Eteindre_CHB_MSB:				return puc_AfficherLibelleAvecEspaces("Sc_Eteindre_CHB_MSB");				break;
			case Scenario_Eteindre_PDE_LSB:				return puc_AfficherLibelleAvecEspaces("Sce_Eteindre_PDE_LSB");				break;
			case Scenario_Eteindre_PDE_MSB:				return puc_AfficherLibelleAvecEspaces("Sce_Eteindre_PDE_MSB");				break;
			case Scenario_Allumer_PDV_LSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_PDV_LSB");				break;
			case Scenario_Allumer_PDV_MSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_PDV_MSB");				break;
			case Scenario_Allumer_CHB_LSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_CHB_LSB");				break;
			case Scenario_Allumer_CHB_MSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_CHB_MSB");				break;
			case Scenario_Allumer_PDE_LSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_PDE_LSB");				break;
			case Scenario_Allumer_PDE_MSB:				return puc_AfficherLibelleAvecEspaces("Sce_Allumer_PDE_MSB");				break;
			case Scenario_OuvrirVolets_PDV:				return puc_AfficherLibelleAvecEspaces("Sce_OuvrirVolets_PDV");				break;
			case Scenario_OuvrirVolets_CHB:				return puc_AfficherLibelleAvecEspaces("Sce_OuvrirVolets_CHB");				break;
			case Scenario_OuvrirVolets_PDE:				return puc_AfficherLibelleAvecEspaces("Sce_OuvrirVolets_PDE");				break;
			case Scenario_FermerVolets_PDV:				return puc_AfficherLibelleAvecEspaces("Sce_FermerVolets_PDV");				break;
			case Scenario_FermerVolets_CHB:				return puc_AfficherLibelleAvecEspaces("Sce_FermerVolets_CHB");				break;
			case Scenario_FermerVolets_PDE:				return puc_AfficherLibelleAvecEspaces("Sce_FermerVolets_PDE");				break;
			case Scenario_Securite:						return puc_AfficherLibelleAvecEspaces("Sce_Securite");						break;
			case Scenario_Machines:						return puc_AfficherLibelleAvecEspaces("Sce_Machines");						break;
			case Scenario_Chauf_zj:						return puc_AfficherLibelleAvecEspaces("Sce_Chauf_zj");						break;
			case Scenario_Chauf_zn:						return puc_AfficherLibelleAvecEspaces("Sce_Chauf_zn");						break;
			case Scenario_Chauf_zsb1:					return puc_AfficherLibelleAvecEspaces("Sce_Chauf_zsb1");					break;
			case Scenario_Chauf_zsb2:					return puc_AfficherLibelleAvecEspaces("Sce_Chauf_zsb2");					break;
			case Scenario_Cumulus:						return puc_AfficherLibelleAvecEspaces("Sce_Cumulus");						break;
			case Scenario_Reveil_Reglage:				return puc_AfficherLibelleAvecEspaces("Sce_Reveil_Reglage");				break;
			case Scenario_Reveil_ON:					return puc_AfficherLibelleAvecEspaces("Sce_Reveil_ON");					break;
			case Scenario_Efface:						return puc_AfficherLibelleAvecEspaces("Sce_Efface");						break;
			
			default:
				return puc_AfficherLibelleAvecEspaces("???");
			break;
		}
	}
}

#define ESPION_TABLE_ECHANGE_TAILLE_LIBELLES	40
unsigned char l_uc_ChaineLibelleTableEchange[ESPION_TABLE_ECHANGE_TAILLE_LIBELLES+1];

unsigned char *puc_AfficherLibelleAvecEspaces(const char *pc_Libelle)
{
	unsigned char l_uc_Compteur;
	unsigned char l_uc_FinChaineTrouvee;
	
	
	l_uc_FinChaineTrouvee = 0;
	for(l_uc_Compteur = 0;l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES;l_uc_Compteur++)
	{
		if(l_uc_FinChaineTrouvee == 0)
		{
			if(pc_Libelle[l_uc_Compteur] == 0)
			{
				l_uc_FinChaineTrouvee = 1;
				l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = ' ';
			}
			else
			{
				l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = (unsigned char)pc_Libelle[l_uc_Compteur];
			}
			
		}
		else
		{
			l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = ' ';
		}
	}
	l_uc_ChaineLibelleTableEchange[ESPION_TABLE_ECHANGE_TAILLE_LIBELLES] = 0;
	
	return l_uc_ChaineLibelleTableEchange;
}

unsigned char *puc_AfficherLibelleAvecEspaces2(const char *pc_Libelle, unsigned short us_Indice)
{
	unsigned char l_uc_Compteur;
	unsigned char l_uc_FinChaineTrouvee;
	
	
	l_uc_FinChaineTrouvee = 0;
	for(l_uc_Compteur = 0;l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES;l_uc_Compteur++)
	{
		if(l_uc_FinChaineTrouvee == 0)
		{
			if(pc_Libelle[l_uc_Compteur] == 0)
			{
				l_uc_FinChaineTrouvee = 1;
				l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = '(';
				l_uc_Compteur++;
				if(l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES)	l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = (us_Indice / 100) + '0';
				l_uc_Compteur++;
				us_Indice = us_Indice % 100;
				if(l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES)	l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = (us_Indice / 10) + '0';
				l_uc_Compteur++;
				us_Indice = us_Indice % 10;
				if(l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES)	l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = us_Indice + '0';
				l_uc_Compteur++;
				if(l_uc_Compteur < ESPION_TABLE_ECHANGE_TAILLE_LIBELLES)	l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = ')';
			}
			else
			{
				l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = (unsigned char)pc_Libelle[l_uc_Compteur];
			}
			
		}
		else
		{
			l_uc_ChaineLibelleTableEchange[l_uc_Compteur] = ' ';
		}
	}
	l_uc_ChaineLibelleTableEchange[ESPION_TABLE_ECHANGE_TAILLE_LIBELLES] = 0;
	
	return l_uc_ChaineLibelleTableEchange;
}

void vd_EspionRS_Afficher_EthernetActivite(unsigned char uc_AffichageForce)
{
	//unsigned short l_us_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	if(uc_AffichageForce == uc_AFFICHAGE_FORCE_NON)
	{
		/*l_us_ValeurEnCours = us_ErreursTacheEcran;
		if(l_us_ValeurEnCours != us_ErreursTacheEcranPrecedent)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"us_ErreursTacheEcran (Hexa) : %X -> %X\n\n", us_ErreursTacheEcranPrecedent, l_us_ValeurEnCours);
			us_ErreursTacheEcranPrecedent = l_us_ValeurEnCours;
		}*/
	}
	else
	{
		printf("Espion Tache Ethernet :\n");
		printf("Etat (Hexa) : %X\n", uc_EspionTacheEthernetEtat);
		printf("InitRTCS (Hexa) : %X\n", uc_EspionTacheEthernetInitRTCS);
		printf("DialogueAvecServeur (Hexa) : %X\n", uc_EspionTacheEthernetDialogueAvecServeur);
		printf("CompteurActivite (Hexa) : %X\n", ul_EspionTacheEthernetCompteurActivite);
		printf("OpenSocket (Hexa) : %X\n", uc_EspionTacheEthernetOpenSocket);
		printf("CableOK (Hexa) : %X\n", ul_EspionTacheEthernetCableOK);
		printf("CablePB (Hexa) : %X\n", ul_EspionTacheEthernetCablePB);
		printf("DCHPOK (Hexa) : %X\n", ul_EspionTacheEthernetDCHPOK);
		printf("DCHPPB (Hexa) : %X\n", ul_EspionTacheEthernetDCHPPB);
		printf("IPFixeOK (Hexa) : %X\n", ul_EspionTacheEthernetIPFixeOK);
		printf("IPFixePB (Hexa) : %X\n", ul_EspionTacheEthernetIPFixePB);
		printf("DNSOK (Hexa) : %X\n", ul_EspionTacheEthernetDNSOK);
		printf("DNSPB (Hexa) : %X\n", ul_EspionTacheEthernetDNSPB);
		printf("DialogueServeurOK (Hexa) : %X\n", ul_EspionTacheEthernetDialogueServeurOK);
		printf("DialogueServeurPBRTCS (Hexa) : %X\n", ul_EspionTacheEthernetDialogueServeurPBRTCS);
		printf("DialogueServeurPBData (Hexa) : %X\n", ul_EspionTacheEthernetDialogueServeurPBData);
		printf("Fonction1OK (Hexa) : %X\n", ul_EspionTacheEthernetFonction1OK);
		printf("Fonction1PB (Hexa) : %X\n", ul_EspionTacheEthernetFonction1PB);
		printf("Fonction2OK (Hexa) : %X\n", ul_EspionTacheEthernetFonction2OK);
		printf("Fonction2PB (Hexa) : %X\n", ul_EspionTacheEthernetFonction2PB);
		printf("Fonctions3et4OK (Hexa) : %X\n", ul_EspionTacheEthernetFonctions3et4OK);
		printf("Fonctions3et4PB (Hexa) : %X\n", ul_EspionTacheEthernetFonctions3et4PB);
		printf("GetInformationServer (Hexa) : %X\n", uc_EspionTacheEthernetGetInformationServer);
		printf("PostInformationServer (Hexa) : %X\n", uc_EspionTacheEthernetPostInformationServer);
		printf("ActionManagment (Hexa) : %X\n", uc_EspionTacheEthernetActionManagment);
		printf("TraiterActions (Hexa) : %X\n", uc_EspionTacheEthernetTraiterActions);
		printf("TraitementAlarme (Hexa) : %X\n", uc_EspionTacheEthernetTraitementAlarme);
		printf("TraitementAction (Hexa) : %X\n", uc_EspionTacheEthernetTraitementAction);
		printf("FonctionDownload (Hexa) : %X\n", ul_EspionTacheEthernetFonctionDownload);
		printf("Download (Hexa) : %X\n", uc_EspionTacheEthernetDownload);
		
		printf("\n");
		
		printf("IP Address      %d.%d.%d.%d\n",IPBYTES(IPConfig.ip));
		printf("Subnet Address  %d.%d.%d.%d\n",IPBYTES(IPConfig.mask));
		printf("Gateway Address %d.%d.%d.%d\n",IPBYTES(IPConfig.gateway));
		printf("DNS Address     %d.%d.%d.%d\n",IPBYTES(ipcfg_get_dns_ip(ENET_DEVICE,0)));
		printf("Adresse serveur trouvée -> %d.%d.%d.%d\n",IPBYTES(IPServeur));
		printf("\n");
		
		printf("uc_EspionEepromSoftErreurLectureEtat (Hexa) : %X\n", uc_EspionEepromSoftErreurLectureEtat);
		printf("uc_EspionEepromSoftErreurAttenteBusy (Hexa) : %X\n", uc_EspionEepromSoftErreurAttenteBusy);
		printf("uc_EspionEepromSoftErreurEnableEcriture (Hexa) : %X\n", uc_EspionEepromSoftErreurEnableEcriture);
		printf("uc_EspionEepromSoftErreurEcriture (Hexa) : %X\n", uc_EspionEepromSoftErreurEcriture);
		printf("uc_EspionEepromSoftErreurLecture (Hexa) : %X\n", uc_EspionEepromSoftErreurLecture);
		printf("uc_EspionEepromSoftErreurEnableEcritureStatus (Hexa) : %X\n", uc_EspionEepromSoftErreurEnableEcritureStatus);
		printf("uc_EspionEepromSoftErreurStatusEnableEcriture (Hexa) : %X\n", uc_EspionEepromSoftErreurStatusEnableEcriture);
		printf("uc_EspionEepromSoftErreurEffacement (Hexa) : %X\n", uc_EspionEepromSoftErreurEffacement);
		printf("\n");
	}
}

void vd_EspionRS_RAZ_EthernetActivite(void)
{
	uc_EspionTacheEthernetEtat = 0;
	uc_EspionTacheEthernetInitRTCS = 0;
	uc_EspionTacheEthernetDialogueAvecServeur = 0;
	ul_EspionTacheEthernetCompteurActivite = 0;
	uc_EspionTacheEthernetOpenSocket = 0;
	ul_EspionTacheEthernetCableOK = 0;
	ul_EspionTacheEthernetCablePB = 0;
	ul_EspionTacheEthernetDCHPOK = 0;
	ul_EspionTacheEthernetDCHPPB = 0;
	ul_EspionTacheEthernetIPFixeOK = 0;
	ul_EspionTacheEthernetIPFixePB = 0;
	ul_EspionTacheEthernetDNSOK = 0;
	ul_EspionTacheEthernetDNSPB = 0;
	ul_EspionTacheEthernetDialogueServeurOK = 0;
	ul_EspionTacheEthernetDialogueServeurPBRTCS = 0;
	ul_EspionTacheEthernetDialogueServeurPBData = 0;
	ul_EspionTacheEthernetFonction1OK = 0;
	ul_EspionTacheEthernetFonction1PB = 0;
	ul_EspionTacheEthernetFonction2OK = 0;
	ul_EspionTacheEthernetFonction2PB = 0;
	ul_EspionTacheEthernetFonctions3et4OK = 0;
	ul_EspionTacheEthernetFonctions3et4PB = 0;
	uc_EspionTacheEthernetGetInformationServer = 0;
	uc_EspionTacheEthernetPostInformationServer = 0;
	uc_EspionTacheEthernetActionManagment = 0;
	uc_EspionTacheEthernetTraiterActions = 0;
	uc_EspionTacheEthernetTraitementAlarme = 0;
	uc_EspionTacheEthernetTraitementAction = 0;
	ul_EspionTacheEthernetFonctionDownload = 0;
	uc_EspionTacheEthernetDownload = 0;

	uc_EspionEepromSoftErreurLectureEtat = 0;
	uc_EspionEepromSoftErreurAttenteBusy = 0;
	uc_EspionEepromSoftErreurEnableEcriture = 0;
	uc_EspionEepromSoftErreurEcriture = 0;
	uc_EspionEepromSoftErreurLecture = 0;
	uc_EspionEepromSoftErreurEnableEcritureStatus = 0;
	uc_EspionEepromSoftErreurStatusEnableEcriture = 0;
	uc_EspionEepromSoftErreurEffacement = 0;
}
#endif

