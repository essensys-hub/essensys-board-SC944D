#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>

#include "application.h"
#include "EspionRS.h"
#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "global.h"
#include "hard.h"
#include "eepromadressemac.h"

#define extern
#include  "alarme.h"
#undef extern


// A appeler au start du Main -> Init variables utilisées par l'alarme
void vd_AlarmeInit(void)
{
	unsigned short l_us_Compteur;
	
	
	// Config alarme : initialisée par défaut avec config "alarme autonome"
	for(l_us_Compteur = 0;l_us_Compteur < AlarmeConfig_NB_VALEURS;l_us_Compteur++)
	{
		uc_AlarmeConfigEnCours[l_us_Compteur] = Tb_Echange[AlarmeConfig + l_us_Compteur];
	}
	uc_AlarmeModePrecedent = uc_ALARME_MODE_PAS_UTILISEE;
	uc_AlarmeDetection = 0;
	uc_AlarmeFraude = 0;

	uc_AlarmeCompteurSecondes = 0;
	uc_Alarme_NouveauCode_1_LSB = 0;
	uc_Alarme_NouveauCode_1_MSB = 0;
	uc_Alarme_NouveauCode_2_LSB = 0;
	uc_Alarme_NouveauCode_2_MSB = 0;

	uc_CompteurDetect_Ouv_50ms = 0;
	uc_CompteurDetect_Pres1_50ms = 0;
	uc_CompteurDetect_Pres2_50ms = 0;
	uc_CompteurOuvertureSireneInterieure = 0;
	uc_CompteurOuvertureSireneExterieure = 0;
	uc_CompteurOuverturePanneauDomotique = 0;
}

// Traitement de l'alarme
// Assure tout le traitement de l'alarme :
//		Mise en route / arrêt
//		Controle / modif code alarme
//		Gestion alarme : détection / pilotage sirène / mode test / ...
void vd_Alarme(void)
{
	unsigned short l_us_Compteur;
	unsigned short l_us_Indice;
	unsigned char l_uc_DemarrerAlarme;
	unsigned char l_uc_EtatCodeSaisi;
	unsigned char l_uc_EtatDetecteurs;
	unsigned char l_uc_NouveauCodeSaisiLSB;
	unsigned char l_uc_NouveauCodeSaisiMSB;
	
	
	l_uc_DemarrerAlarme = 0;
	
	// RAZ temporisation sortie / rentrée INUTILE -> fait plus bas...
	/*if(st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours == 0 &&
	   st_EchangeStatus.uc_AlarmeProcedureSortieEnCours == 0)
	{
		uc_AlarmeCompteurSecondes = 0;	// Verif si = 0 fait en une instruction
	}*/

	// A faire en premier et tout le temps
	// Refresh etat detecteurs et fraudes -> declenche alarme si besoin + MAJ table echange (valeurs utilisées plus bas...)
	vd_ControleDetecteursEtFraudes();

	// A faire tout le temps -> controle saisi si valeur LSB / MSB != de 0xFF (nouvelle saisie)
	// Et remet a 0xFF apres controle
	// Ok pour multithread : on attend les deux avant de remettre à 0
	l_uc_NouveauCodeSaisiLSB = 0xFF;
	l_uc_NouveauCodeSaisiMSB = 0xFF;
	l_uc_EtatCodeSaisi = uc_ControleCodeSaisie(&l_uc_NouveauCodeSaisiLSB, &l_uc_NouveauCodeSaisiMSB);
	
	
	// Demande activation alarme depuis serveur
	if(uc_DemandeServeurActiverAlarme != 0)
	{
		uc_DemandeServeurActiverAlarme = 0;
		
		// Demande acceptée si Alarme_AccesADistance == 1 (écran en veille) ET alarme pas encore activée
		if(Tb_Echange[Alarme_AccesADistance] != 0 && st_EchangeStatus.uc_AlarmeActivee == 0)
		{
			// Il faut activer la bonne configuration alarme en regardant le dernier scénario lancé
			// Selon les cas, config du dernier scénario lancé, soit alarme autonome
			switch(Tb_Echange[Scenario_DernierLance])
			{
				case uc_SCENARIO_JE_SORS:
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_ALARME_SCENARIO_JE_SORS;
				break;
				case uc_SCENARIO_JE_PARS_EN_VACANCES:
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_ALARME_SCENARIO_JE_PARS_EN_VACANCES;
				break;
				case uc_SCENARIO_JE_ME_COUCHE:
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_ALARME_SCENARIO_JE_VAIS_ME_COUCHER;
				break;
				//case uc_SCENARIO_PERSO:	// Alarme independante (voir email NG du 12/08/2014 13:19)
				//	Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_ALARME_SCENARIO_PERSONNALISE;
				//break;
				default:	// Tous les autres cas : uc_SCENARIO_JE_RENTRE, uc_SCENARIO_JE_ME_LEVE, uc_SCENARIO_PERSO et aucun scénario lancé (depuis démarrage système)
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_ALARME_INDEPENDANTE;
				break;
			}
			
			// Demande d'activation de l'alarme -> va suivre la même procédure que si activation depuis l'écran
			l_uc_DemarrerAlarme = 1;			
		}
	}

	
	// Mode de l'alarme : config ou démarrage - ne faire traitement que si alarme à OFF
	if(Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE && st_EchangeStatus.uc_AlarmeActivee == 0)
	{
		// Mode réglage
		if(Tb_Echange[Alarme_Mode] != uc_AlarmeModePrecedent)
		{
			Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART;
		}
		
		switch(Tb_Echange[Alarme_SuiviChangementCode])
		{
			case uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART:
				//l_uc_CodeRetour = uc_ControleCodeSaisie();
				if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_INCORRECT)
				{
					Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART;
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
					Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_INVALIDE;
				}
				else if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_CORRECT)
				{
					Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_1ER_ENVOI;
					Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_VALIDE;
				}
				l_uc_EtatCodeSaisi = uc_ControleCodeSaisie_AUCUN_CODE_SAISI;	// RAZ info apres utilisation (ne doit servir qu'une seule fois !)
			break;
			case uc_ALARME_SUIVI_CHANGEMENT_CODE_1ER_ENVOI:
				if(l_uc_NouveauCodeSaisiLSB != 0xFF && l_uc_NouveauCodeSaisiMSB != 0xFF)
				{
					uc_Alarme_NouveauCode_1_LSB = l_uc_NouveauCodeSaisiLSB;
					uc_Alarme_NouveauCode_1_MSB = l_uc_NouveauCodeSaisiMSB;
					l_uc_NouveauCodeSaisiLSB = 0xFF;
					l_uc_NouveauCodeSaisiMSB = 0xFF;
					Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_2EME_ENVOI;
					Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_VALIDE;
				}
			break;
			case uc_ALARME_SUIVI_CHANGEMENT_CODE_2EME_ENVOI:
				if(l_uc_NouveauCodeSaisiLSB != 0xFF && l_uc_NouveauCodeSaisiMSB != 0xFF)
				{
					uc_Alarme_NouveauCode_2_LSB = l_uc_NouveauCodeSaisiLSB;
					uc_Alarme_NouveauCode_2_MSB = l_uc_NouveauCodeSaisiMSB;
					l_uc_NouveauCodeSaisiLSB = 0xFF;
					l_uc_NouveauCodeSaisiMSB = 0xFF;
					
					if(uc_Alarme_NouveauCode_1_LSB != uc_Alarme_NouveauCode_2_LSB || uc_Alarme_NouveauCode_1_MSB != uc_Alarme_NouveauCode_2_MSB)
					{
						// Les deux nouveaux codes reçus sont différents -> RAZ demande
						Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART;
						Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
						Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_INVALIDE;
					}
					else
					{
						// Les deux nouveaux codes reçus sont identiques -> prise en compte et fin procédure
						vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"NEW CODE ACCEPTE !\n");
						Tb_Echange[Alarme_CodeUser1LSB] = uc_Alarme_NouveauCode_1_LSB;
						Tb_Echange[Alarme_CodeUser1MSB] = uc_Alarme_NouveauCode_1_MSB;
						
						vd_UpdateCodeAlarmeDansEEPROM();	// Mémorisation en EEPROM @ MAC //xxx ne pas faire si telechargement en cours
						
						Tb_Echange[Alarme_SuiviChangementCode] = uc_ALARME_SUIVI_CHANGEMENT_CODE_ETAT_DEPART;
						Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
						Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_VALIDE;
					}
				}
			break;
		}
	}
	else if(Tb_Echange[Alarme_Mode] != uc_ALARME_MODE_PAS_UTILISEE &&
			Tb_Echange[Alarme_Mode] != uc_ALARME_MODE_REGLAGE &&
			st_EchangeStatus.uc_AlarmeActivee == 0)
	{
		// Init config et RAZ Suivi alarme que si nouvelle demande (sinon RAZ en permanence...)
		if(Tb_Echange[Alarme_Mode] != uc_AlarmeModePrecedent)
		{
			// Configuration de l'alarme : prendre config "alarme autonome" ou "config scenario"
			l_us_Indice = AlarmeConfig;	// Par défaut
			switch(Tb_Echange[Alarme_Mode])
			{
				case uc_ALARME_MODE_ALARME_SCENARIO_JE_SORS:
					l_us_Indice = Scenario2 + Scenario_AlarmeConfig;
				break;
				case uc_ALARME_MODE_ALARME_SCENARIO_JE_VAIS_ME_COUCHER:
					l_us_Indice = Scenario5 + Scenario_AlarmeConfig;
				break;
				case uc_ALARME_MODE_ALARME_SCENARIO_JE_PARS_EN_VACANCES:
					l_us_Indice = Scenario3 + Scenario_AlarmeConfig;
				break;
				case uc_ALARME_MODE_ALARME_SCENARIO_PERSONNALISE:
					l_us_Indice = Scenario7 + Scenario_AlarmeConfig;
				break;
				default:	// uc_ALARME_MODE_ALARME_INDEPENDANTE
					l_us_Indice = AlarmeConfig;
				break;
			}
			for(l_us_Compteur = 0;l_us_Compteur < AlarmeConfig_NB_VALEURS;l_us_Compteur++)
			{
				uc_AlarmeConfigEnCours[l_us_Compteur] = Tb_Echange[l_us_Indice + l_us_Compteur];
			}

			Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_ETAPE_DEPART;
		}

		// Demande de mise en route de l'alarme
		if(Tb_Echange[Alarme_Commande] != 0)
		{
			l_uc_DemarrerAlarme = 1;
			Tb_Echange[Alarme_Commande] = 0; // demande prise en compte !
		}

		//l_uc_CodeRetour = uc_ControleCodeSaisie();
		if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_INCORRECT)
		{
			Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_INVALIDE;
			Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
		}
		else if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_CORRECT)
		{
			Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_VALIDE;
		
			// Code saisi OK
			if(Tb_Echange[Alarme_SuiviAlarme] == uc_ALARME_SUIVI_ALARME_ETAPE_DEPART)
			{
				// Demande d'activation de l'alarme
				l_uc_DemarrerAlarme = 1;
			}
		}
		l_uc_EtatCodeSaisi = uc_ControleCodeSaisie_AUCUN_CODE_SAISI;	// RAZ info apres utilisation (ne doit servir qu'une seule fois !)
		
		if(l_uc_DemarrerAlarme != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"DEMARRAGE ALARME... !\n");
			
			// verifier alarme pas deja en route...
			if(st_EchangeStatus.uc_AlarmeActivee == 0)	//Tb_Echange[Alarme_SuiviAlarme] == uc_ALARME_SUIVI_ALARME_ETAPE_DEPART)
			{
				// Rafraichissement de l'état des détecteurs et fraudes : nécessaire car la config a peut etre change -> on doit donc rafraichir ces états en tenant compte de la nouvelle config !
				vd_ControleDetecteursEtFraudes();
				
				l_uc_EtatDetecteurs = uc_AlarmeDetection;
				// Suppression des détecteurs dans la zone de sortie
				if(uc_AlarmeConfigEnCours[AlarmeConfig_DetectOuvSurVoieAcces] != 0)		l_uc_EtatDetecteurs &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
				if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect1SurVoieAcces] != 0)		l_uc_EtatDetecteurs &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
				if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect2SurVoieAcces] != 0)		l_uc_EtatDetecteurs &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;

				if(uc_AlimentationsCorrectesPourDemarrerAlarme() == 0)
				{
					vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"DEMARRAGE ALARME... IMPOSSIBLE -> PB ALIM !\n");
					vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE," !\n");
					Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_PROBLEME_ALIMENTATION;
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;	
				}
				else if(l_uc_EtatDetecteurs != 0 || uc_AlarmeFraude != 0)
				{	// Etats testés -> renseignés par vd_ControleDetecteursEtFraudes()
					vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"DEMARRAGE ALARME... IMPOSSIBLE -> DETECTEURS : %d - FRAUDE : %d !\n", l_uc_EtatDetecteurs, uc_AlarmeFraude);
					Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_PROBLEME_DETECTION;
					Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
				}
				else
				{
					vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"DEMARRAGE ALARME... LANCEE -> PROCEDURE SORTIE...\n");
					Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_PROCEDURE_SORTIE;
					st_EchangeStatus.uc_AlarmeDeclenchee = 0;
					st_EchangeStatus.uc_AlarmeActivee = 1;
					uc_AlarmeCompteurSecondes = 0;
					st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 1;
					st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
				}
			}
			else
			{
				vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"DEMARRAGE ALARME... IMPOSSIBLE -> ALARME DEJA ACTIVE!\n");
			}
		}
	}
	uc_AlarmeModePrecedent = Tb_Echange[Alarme_Mode];
	
	// Arret de l'alarme
	if(st_EchangeStatus.uc_AlarmeActivee != 0)
	{
		//l_uc_CodeRetour = uc_ControleCodeSaisie();
		if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_INCORRECT)
		{
			Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_INVALIDE;
		}
		else if(l_uc_EtatCodeSaisi == uc_ControleCodeSaisie_CODE_SAISI_CORRECT)
		{
			Tb_Echange[Alarme_Autorisation] = uc_ALARME_AUTORISATION_CODE_VALIDE;
		
			// Code saisi OK
			//if(Tb_Echange[Alarme_SuiviAlarme] >= uc_ALARME_SUIVI_ALARME_PROCEDURE_SORTIE) pas besoin de tester on arrete si alarme active (uc_AlarmeActivee)
			{
				// Demande d'arrêt de l'alarme
				st_EchangeStatus.uc_AlarmeDeclenchee = 0;
				st_EchangeStatus.uc_AlarmeActivee = 0;
				st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 0;
				st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
				Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
				Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_ETAPE_DEPART;
			}
		}
		l_uc_EtatCodeSaisi = uc_ControleCodeSaisie_AUCUN_CODE_SAISI;	// RAZ info apres utilisation (ne doit servir qu'une seule fois !)
		
		// Arrêt de l'alarme sur demande du serveur
		if(uc_DemandeServeurCouperAlarme != 0)
		{
			uc_DemandeServeurCouperAlarme = 0;
			
			// Demande acceptée si Alarme_AccesADistance == 1 (écran en veille)
			if(Tb_Echange[Alarme_AccesADistance] != 0)
			{
				// Désactivation de l'alarme
				st_EchangeStatus.uc_AlarmeDeclenchee = 0;
				st_EchangeStatus.uc_AlarmeActivee = 0;
				st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 0;
				st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
				Tb_Echange[Alarme_Mode] = uc_ALARME_MODE_PAS_UTILISEE;
				Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_ETAPE_DEPART;
			}			
		}
	}
	
	// Renseigner Alarme_CompteARebours pour affichage
	if(st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours == 0 &&
	   st_EchangeStatus.uc_AlarmeProcedureSortieEnCours == 0)
	{
		uc_AlarmeCompteurSecondes = 0;
		Tb_Echange[Alarme_CompteARebours] = 0;
	}
	else
	{
		if(uc_AlarmeCompteurSecondes >= uc_DUREE_PROCEDURE_RENTREE_SORTIE_Sec)
		{
			Tb_Echange[Alarme_CompteARebours] = 0;
		}
		else
		{
			Tb_Echange[Alarme_CompteARebours] = uc_DUREE_PROCEDURE_RENTREE_SORTIE_Sec - uc_AlarmeCompteurSecondes;
		}
	}
	
#ifdef DEBUG
	vd_Alarme_Espion();
#endif
}

// Controle code saisi
// Multitache : prendre valeurs table echange et travailler avec copie (pour eviter tout changement durant traitement
unsigned char uc_ControleCodeSaisie(unsigned char *uc_NouveauCodeSaisiLSB, unsigned char *uc_NouveauCodeSaisiMSB)
{
	unsigned char l_uc_ValeurRetour;
	
	*uc_NouveauCodeSaisiLSB = Tb_Echange[Alarme_CodeSaisiLSB];
	*uc_NouveauCodeSaisiMSB = Tb_Echange[Alarme_CodeSaisiMSB];

	l_uc_ValeurRetour = uc_ControleCodeSaisie_AUCUN_CODE_SAISI;
	if(*uc_NouveauCodeSaisiLSB != 0xFF && *uc_NouveauCodeSaisiMSB != 0xFF)
	{
		l_uc_ValeurRetour = uc_ControleCodeSaisie_CODE_SAISI_INCORRECT;
		if(*uc_NouveauCodeSaisiLSB == Tb_Echange[Alarme_CodeUser1LSB] && *uc_NouveauCodeSaisiMSB == Tb_Echange[Alarme_CodeUser1MSB])
		{
			l_uc_ValeurRetour = uc_ControleCodeSaisie_CODE_SAISI_CORRECT;
		}
		Tb_Echange[Alarme_CodeSaisiLSB] = 0xFF;	// On remet a 0xFF la table d'échange
		Tb_Echange[Alarme_CodeSaisiMSB] = 0xFF;
	}
	
	return l_uc_ValeurRetour;
}

// Verifie que les alimentations soient dans un état permettant l'activation de l'alarme
// 0 : pb alim(s) - 1 : alims ok
unsigned char uc_AlimentationsCorrectesPourDemarrerAlarme(void)
{
	if(lwgpio_get_value(&IO_DIN_Etat_AlimPrincipale) == LWGPIO_VALUE_HIGH && uc_BatteriePresente != 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// Rafraichie l'état des détecteurs et fraudes -> Alarme_Detection et Alarme_Fraude
// Masque les détecteurs en fonction du mode en cours (sortie / rentrée) ou s'ils sont utilisés ou non
// Déclenche l'alarme si besoin selon le mode en cours...
void vd_ControleDetecteursEtFraudes(void)
{
	unsigned char l_uc_DetectionPourAlarme;
	unsigned char l_uc_Valeur;
	
	
	// Alarme pas activée : on RAZ l'état détection précédent sauf en mode test
	// Alarme activée : on conserve l'état détection sauf en cas de procédure de sortie ou de rentrée
	if(st_EchangeStatus.uc_AlarmeActivee == 0)
	{
		uc_AlarmeFraude = 0;	// Effacer état dans tous les cas
		//uc_AlarmeDetection = 0; Effacer état complet sauf pour DETECTEUR PRESENCE 1 ET 2 SI MODE TEST

		if(Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)
		{
			uc_AlarmeDetection &= (uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1 | uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2);
			if((Tb_Echange[Alarme_TestRAZPresence] & uc_ALARME_TEST_RAZ_PRESENCE_RAZ_DETECTEUR_PRESENCE_1) != 0)
			{
				// Supprimer état détecteur présence 1
				uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
			}
			if((Tb_Echange[Alarme_TestRAZPresence] & uc_ALARME_TEST_RAZ_PRESENCE_RAZ_DETECTEUR_PRESENCE_2) != 0)
			{
				// Conserver état détecteur présence 2
				uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
			}
		}
		else
		{
			uc_AlarmeDetection = 0;
		}
	}
	Tb_Echange[Alarme_TestRAZPresence] = uc_ALARME_TEST_RAZ_PRESENCE_NE_RIEN_FAIRE;	// RAZ demande à faire dans tous les cas xxx rajotuer mutex
	
	uc_EtatDetecteurs = 0;
	uc_EtatFraudes = 0;
	
	// Prise en compte détecteurs : si à 1 pendant au moins 400 ms
	if(lwgpio_get_value(&IO_DIN_Detection_Ouverture) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurDetect_Ouv_50ms = 0;
	}
	else
	{
		if(uc_CompteurDetect_Ouv_50ms >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			uc_AlarmeDetection |= uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
			uc_EtatDetecteurs |= uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
		}
	}
	if(lwgpio_get_value(&IO_DIN_Detection_presence1) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurDetect_Pres1_50ms = 0;
	}
	else
	{
		if(uc_CompteurDetect_Pres1_50ms >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			uc_AlarmeDetection |= uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
			uc_EtatDetecteurs |= uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
		}
	}
	if(lwgpio_get_value(&IO_DIN_Detection_presence2) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurDetect_Pres2_50ms = 0;
	}
	else
	{
		if(uc_CompteurDetect_Pres2_50ms >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			uc_AlarmeDetection |= uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
			uc_EtatDetecteurs |= uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
		}
	}
	
	// Monitorage détecteurs
	// Tous les détecteurs et fraudes sont monitorés qu'ils soient autorisés ou non
	// ... sauf en "procédure de sortie" ou ceux se trouvant dans la voie d'accès
	// Déclenchement alarme : idem, ne pas prendre ceux dans la voie d'accès si procédure de sortie en cours...
	if(st_EchangeStatus.uc_AlarmeActivee != 0 && st_EchangeStatus.uc_AlarmeProcedureSortieEnCours != 0)
	{		
		if(uc_AlarmeConfigEnCours[AlarmeConfig_DetectOuvSurVoieAcces] != 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect1SurVoieAcces] != 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect2SurVoieAcces] != 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
	}

	// Prise en compte des nouvelles détections pour ecran : tous sauf ceux dans voie d'accès si procédure de sortie en cours...
	// Et si alarme active OU mode test, sinon mettre tout à 0
	if(st_EchangeStatus.uc_AlarmeActivee != 0 || Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)		Tb_Echange[Alarme_Detection] = uc_AlarmeDetection;
	else																								Tb_Echange[Alarme_Detection] = 0;
	
	// Suppression des détecteurs non utilisés - SI PAS DE MODE TEST EN COURS !!!
	if(Tb_Echange[Alarme_Mode] != uc_ALARME_MODE_REGLAGE)
	{
		if(uc_AlarmeConfigEnCours[AlarmeConfig_DetectOuv] == 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect1] == 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect2] == 0)		uc_AlarmeDetection &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
	}
	
	// Détection "procédure d'entrée" -> alarme activee et pas de procedure de sortie en cours
	if(st_EchangeStatus.uc_AlarmeActivee != 0 &&
	   st_EchangeStatus.uc_AlarmeDeclenchee == 0 &&
	   st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours == 0 &&
	   Tb_Echange[Alarme_SuiviAlarme] == uc_ALARME_SUIVI_ALARME_REGIME_CROISIERE)	// Derniere condition : pour ne pas enchainer procedure de sortie -> procedure de rentree
	{
		if(uc_AlarmeConfigEnCours[AlarmeConfig_DetectOuvSurVoieAcces] != 0 && (uc_AlarmeDetection & uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE) != 0 ||
		   uc_AlarmeConfigEnCours[AlarmeConfig_Detect1SurVoieAcces] != 0   && (uc_AlarmeDetection & uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1) != 0 ||
		   uc_AlarmeConfigEnCours[AlarmeConfig_Detect2SurVoieAcces] != 0   && (uc_AlarmeDetection & uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2) != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"PROCEDURE RENTREE DEMAREE -> Detection : %d !\n", uc_AlarmeDetection);
			Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_PROCEDURE_ENTREE;
			uc_AlarmeCompteurSecondes = 0;
			st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 1;
		}
	}	
	
	// Détection des fraudes
	// Non gérés actuellement : uc_ALARME_FRAUDE_BIT_DETECTEUR_PRESENCE_1, uc_ALARME_FRAUDE_BIT_DETECTEUR_PRESENCE_2
	if(lwgpio_get_value(&IO_DIN_OuvertureSireneInterieure) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurOuvertureSireneInterieure = 0;
	}
	else
	{
		if(uc_CompteurOuvertureSireneInterieure >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			if(uc_AlarmeConfigEnCours[AlarmeConfig_SireneInt] != 0 || Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)
			{
				uc_AlarmeFraude |= uc_ALARME_FRAUDE_BIT_SIRENE_INTERIEURE;
				uc_EtatFraudes |= uc_ALARME_FRAUDE_BIT_SIRENE_INTERIEURE;
			}
		}
	}
	if(lwgpio_get_value(&IO_DIN_OuvertureSireneExterieure) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurOuvertureSireneExterieure = 0;
	}
	else
	{
		if(uc_CompteurOuvertureSireneExterieure >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			if(uc_AlarmeConfigEnCours[AlarmeConfig_SireneExt] != 0 || Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)
			{
				uc_AlarmeFraude |= uc_ALARME_FRAUDE_BIT_SIRENE_EXTERIEURE;
				uc_EtatFraudes |= uc_ALARME_FRAUDE_BIT_SIRENE_EXTERIEURE;
			}
		}
	}
	if(lwgpio_get_value(&IO_DIN_OuvertureTableauDominique) != LWGPIO_VALUE_HIGH)
	{
		uc_CompteurOuverturePanneauDomotique = 0;
	}
	else
	{
		if(uc_CompteurOuverturePanneauDomotique >= uc_DUREE_ENTREE_ALARME_A_1_50ms)
		{
			uc_AlarmeFraude |= uc_ALARME_FRAUDE_BIT_TABLEAU_DOMOTIQUE;
			uc_EtatFraudes |= uc_ALARME_FRAUDE_BIT_TABLEAU_DOMOTIQUE;
		}
	}
	// Si time out OU (dialogue écran OK ET ecran ouvert)
	if(st_EchangeStatus.uc_EcranTimeOUT != 0 || (st_EchangeStatus.uc_EcranTimeOUT == 0 && st_EchangeStatus.uc_DinEcranOuvert != 0))
	{
		uc_AlarmeFraude |= uc_ALARME_FRAUDE_BIT_IHM;
		uc_EtatFraudes |= uc_ALARME_FRAUDE_BIT_IHM;
	}
	
	// Prise en compte des nouvelles détections pour affichage si alarme active OU mode test en cours
	if(st_EchangeStatus.uc_AlarmeActivee != 0 || Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)
	{
		l_uc_Valeur = uc_AlarmeFraude;
		if(uc_BatteriePresente == 0)	l_uc_Valeur |= uc_ALARME_FRAUDE_BATTERIE_NON_PRESENTE;
		Tb_Echange[Alarme_Fraude] = l_uc_Valeur;
	}
	else
	{
		Tb_Echange[Alarme_Fraude] = 0;
	}
	
		
	// Procédure de sortie terminée -> mode croisière
	if(Tb_Echange[Alarme_SuiviAlarme] == uc_ALARME_SUIVI_ALARME_PROCEDURE_SORTIE &&
	   st_EchangeStatus.uc_AlarmeProcedureSortieEnCours == 0)
	{
		Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_REGIME_CROISIERE;
		// Pourquoi mis ici ???
		// Pour eviter un enchainement procedure de sortie -> detection -> procedure de rentree
		// Si mis en bas, la procedure de sortie se termine et au coup d'apres la procedure de rentree prend la main
		// Avec cette mini tempo, la procédure de sortie se termine et la procédure de rentrée ne prendra la main qu'au coup suivant
		// Laissant le temps au controle commande de declencher l'alarme...
	}
	
	// Procédure de sortie en cours...
	if(st_EchangeStatus.uc_AlarmeActivee != 0 &&
	   st_EchangeStatus.uc_AlarmeProcedureSortieEnCours != 0)
	{
		if(uc_AlarmeCompteurSecondes >= uc_DUREE_PROCEDURE_RENTREE_SORTIE_Sec)
		{
			// Temps écoulé
			//Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_REGIME_CROISIERE;
			st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 0;
		}
	}
	// Procédure de rentrée en cours...
	else if(st_EchangeStatus.uc_AlarmeActivee != 0 &&
			st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours != 0)
	{
		if(uc_AlarmeCompteurSecondes >= uc_DUREE_PROCEDURE_RENTREE_SORTIE_Sec)
		{
			// Temps écoulé
			if(st_EchangeStatus.uc_AlarmeDeclenchee == 0)
			{
				vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"ALARME DECLENCHEE APRES PROCEDURE RENTREE !\n");
				Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_INTRUSION_OU_VANDALISME;
				st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
				st_EchangeStatus.uc_AlarmeDeclenchee = 1;
				if(uc_AlarmeConfigEnCours[AlarmeConfig_BloqueVolets] != 0)		st_EchangeStatus.uc_BloquerVolets = 1;
				if(uc_AlarmeConfigEnCours[AlarmeConfig_ForcerEclairage] != 0)	st_EchangeStatus.uc_ForcerAllumage = 1;
			}
		}
	}
	
	l_uc_DetectionPourAlarme = uc_AlarmeDetection;
	if(st_EchangeStatus.uc_AlarmeActivee != 0 && st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours != 0)
	{
		if(uc_AlarmeConfigEnCours[AlarmeConfig_DetectOuvSurVoieAcces] != 0)		l_uc_DetectionPourAlarme &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_OUVERTURE;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect1SurVoieAcces] != 0)		l_uc_DetectionPourAlarme &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_1;
		if(uc_AlarmeConfigEnCours[AlarmeConfig_Detect2SurVoieAcces] != 0)		l_uc_DetectionPourAlarme &= ~uc_ALARME_DETECTION_BIT_DETECTEUR_PRESENCE_2;
	}	
	// On enlève les détecteurs dans zone d'entrée en procédure de rentrée (et uniquement eux, les autres font déclencher l'alarme immédiatement)
	if(l_uc_DetectionPourAlarme != 0 || uc_AlarmeFraude != 0)
	{
		if(st_EchangeStatus.uc_AlarmeActivee != 0 && st_EchangeStatus.uc_AlarmeDeclenchee == 0)
		{
			vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"ALARME DECLENCHEE -> Detection : %d - Fraude : %d !\n", l_uc_DetectionPourAlarme, uc_AlarmeFraude);
			Tb_Echange[Alarme_SuiviAlarme] = uc_ALARME_SUIVI_ALARME_INTRUSION_OU_VANDALISME;
			st_EchangeStatus.uc_AlarmeDeclenchee = 1;
			if(st_EchangeStatus.uc_AlarmeProcedureSortieEnCours == 0)
			{	// Pendant procédure de sortie, PAS DE Blocage des volets - PAS DE forcage des allumage 
				if(uc_AlarmeConfigEnCours[AlarmeConfig_BloqueVolets] != 0)		st_EchangeStatus.uc_BloquerVolets = 1;
				if(uc_AlarmeConfigEnCours[AlarmeConfig_ForcerEclairage] != 0)	st_EchangeStatus.uc_ForcerAllumage = 1;
			}
		}
	}
	
	if(st_EchangeStatus.uc_AlarmeDeclenchee != 0)
	{
		// Réinitialisation tempo sirènes SI front montant détecté
		
		// Enlever les détecteurs et fraudes à 0 -> ce sont ceux qui sont a 0 dans l_uc_DetectionPourAlarme / uc_AlarmeFraude
		uc_EtatDetecteurs = l_uc_DetectionPourAlarme & uc_EtatDetecteurs;
		uc_EtatFraudes = uc_AlarmeFraude & uc_EtatFraudes;
		
		if(uc_DetectionFrontMontant(uc_EtatDetecteursPrecedent, uc_EtatDetecteurs) != 0 ||
		   uc_DetectionFrontMontant(uc_EtatFraudesPrecedent, uc_EtatFraudes) != 0)
		{
			uc_ReinitCompteursSirenes = 1;
		}
	}
	uc_EtatDetecteursPrecedent = uc_EtatDetecteurs;
	uc_EtatFraudesPrecedent = uc_EtatFraudes;
	
	if(st_EchangeStatus.uc_AlarmeActivee != 0 && st_EchangeStatus.uc_AlarmeDeclenchee != 0)
	{
		st_EchangeStatus.uc_AlarmeProcedureRentreeEnCours = 0;
		st_EchangeStatus.uc_AlarmeProcedureSortieEnCours = 0;
	}

	if(st_EchangeStatus.uc_AlarmeActivee == 0)
	{
		st_EchangeStatus.uc_AlarmeDeclenchee = 0;
		st_EchangeStatus.uc_BloquerVolets = 0;
		st_EchangeStatus.uc_ForcerAllumage = 0;
	}
}

// Détecte si un bit passe de 0 à 1 entre la valeur précédente et la nouvelle valeur
// Retourne 1 si front montant détecté
unsigned char uc_DetectionFrontMontant(unsigned char uc_ValeurPrecedente, unsigned char uc_ValeurNouvelle)
{
	unsigned char l_uc_Retour;
	unsigned char l_uc_Compteur;
	unsigned char l_uc_Masque;
	
	l_uc_Retour = 0;
	l_uc_Masque = 1;
	for(l_uc_Compteur = 0;l_uc_Compteur < 8;l_uc_Compteur++)
	{
		if((uc_ValeurPrecedente & l_uc_Masque) == 0 && (uc_ValeurNouvelle & l_uc_Masque) != 0)		l_uc_Retour = 1;
		l_uc_Masque = l_uc_Masque * 2;
	}
	return(l_uc_Retour);
}

#ifdef DEBUG
void vd_Alarme_Espion(void)
{
	unsigned char l_uc_ValeurEnCours;	// Recupere la valeur en local avant traitement avec valeur précédente (risque modif apres if et avant affectation)
	
	l_uc_ValeurEnCours = uc_AlarmeDetection;
	if(l_uc_ValeurEnCours != uc_AlarmeDetection_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"AlarmeDetection : 0x%X -> 0x%X\n\n", uc_AlarmeDetection_Precedent, l_uc_ValeurEnCours);
		uc_AlarmeDetection_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_AlarmeFraude;
	if(l_uc_ValeurEnCours != uc_AlarmeFraude_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"AlarmeFraude : 0x%X -> 0x%X\n\n", uc_AlarmeFraude_Precedent, l_uc_ValeurEnCours);
		uc_AlarmeFraude_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_AlarmeCompteurSecondes;
	if(l_uc_ValeurEnCours != uc_AlarmeCompteurSecondes_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"AlarmeCompteurSecondes : 0x%X -> 0x%X\n\n", uc_AlarmeCompteurSecondes_Precedent, l_uc_ValeurEnCours);
		uc_AlarmeCompteurSecondes_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_Alarme_NouveauCode_1_LSB;
	if(l_uc_ValeurEnCours != uc_Alarme_NouveauCode_1_LSB_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"Alarme_NewCode_1_LSB : 0x%X -> 0x%X\n\n", uc_Alarme_NouveauCode_1_LSB_Precedent, l_uc_ValeurEnCours);
		uc_Alarme_NouveauCode_1_LSB_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_Alarme_NouveauCode_1_MSB;
	if(l_uc_ValeurEnCours != uc_Alarme_NouveauCode_1_MSB_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"Alarme_NewCode_1_MSB : 0x%X -> 0x%X\n\n", uc_Alarme_NouveauCode_1_MSB_Precedent, l_uc_ValeurEnCours);
		uc_Alarme_NouveauCode_1_MSB_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_Alarme_NouveauCode_2_LSB;
	if(l_uc_ValeurEnCours != uc_Alarme_NouveauCode_2_LSB_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"Alarme_NouveauCode_2_LSB : 0x%X -> 0x%X\n\n", uc_Alarme_NouveauCode_2_LSB_Precedent, l_uc_ValeurEnCours);
		uc_Alarme_NouveauCode_2_LSB_Precedent = l_uc_ValeurEnCours;
	}
	l_uc_ValeurEnCours = uc_Alarme_NouveauCode_2_MSB;
	if(l_uc_ValeurEnCours != uc_Alarme_NouveauCode_2_MSB_Precedent)
	{
		vd_EspionRS_Printf(uc_ESPION_ALARME_ACTIVITE,"Alarme_NouveauCode_2_MSB : 0x%X -> 0x%X\n\n", uc_Alarme_NouveauCode_2_MSB_Precedent, l_uc_ValeurEnCours);
		uc_Alarme_NouveauCode_2_MSB_Precedent = l_uc_ValeurEnCours;
	}
}
#endif

