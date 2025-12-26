#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx
#include "EspionRS.h"
#include "TableEchange.h"
#include "global.h"
#include "ecran.h"

#define extern
#include "TableEchangeAcces.h"
#include "tableechangedroits.h"
#undef extern


// !!! ACCES A LA TABLE D'ECHANGE !!!
// !!!     REGLES A RESPECTER     !!!
// BP : accès direct en lecture / écriture
// ECRAN & ETHERNET : accès via les fonctions uc_TableEchange_Ecrit_Data et uc_TableEchange_Lit_Data
// Ces fonctions intègrent les protections en lecture / écriture pour certains mots (code alarme, ...) 

// xxx acces par protection mutex
//	Chauf_zj_Mode = Chauf_zsb2_Auto + uc_PLANNING_CHAUFFAGE_TAILLE,		// chauffage zone jour - PROTECTION PAR MUTEX
//	Chauf_zn_Mode,														// chauffage zone nuit - PROTECTION PAR MUTEX
//	Chauf_zsb1_Mode,													// chauffage salle de bain 1 - PROTECTION PAR MUTEX
//	Chauf_zsb2_Mode,													// chauffage salle de bain 2 - PROTECTION PAR MUTEX


// écrit une donnée dans la table d'échange
// traitement spécifique sur certaines données
// aucun traitement sur données en lecture seule
unsigned char uc_TableEchange_Ecrit_Data(unsigned short us_Numero, unsigned char uc_Donnee, unsigned char uc_OrdreServeur)
{
	unsigned char l_uc_Retour;
	
	
	l_uc_Retour = uc_BP_PB_VALEUR;
	
	if (us_Numero < Nb_Tbb_Donnees)
	{	// sécurité : donnée existante

		if((Tb_Echange_Droits[us_Numero] & ACCES_ECRITURE) != 0)
		{	// Accès en écriture autorisé pour cette valeur

			// traitements spécifiques sur certain mnémoniques
			switch(us_Numero)
			{
				case Minutes :
					st_DateHeureEnvoyeeParEcran.MINUTE = uc_Donnee;
					uc_FlagMajDate = 1;
				break;
				case Heure :
					st_DateHeureEnvoyeeParEcran.HOUR = uc_Donnee;
					uc_FlagMajDate = 1;
				break;
				case Jour :
					st_DateHeureEnvoyeeParEcran.DAY = uc_Donnee;
					uc_FlagMajDate = 1;
				break;
				case Mois :
					st_DateHeureEnvoyeeParEcran.MONTH = uc_Donnee;
					uc_FlagMajDate = 1;
				break;
				case Annee :
					st_DateHeureEnvoyeeParEcran.YEAR = uc_Donnee;
					uc_FlagMajDate = 1;
				break;
					
				default :	// mnémoniques accessibles en écriture
					Tb_Echange[us_Numero] = uc_Donnee; 
				break;
			}
			
			//xxxif(us_Numero >= VacanceFin_H && us_Numero <= VacanceFin_A)	// Date heure retour vacances -> a controler
			if(us_Numero == VacanceFin_A)	// Position flag sur la réception de l'année -> permet de cchecker la date lorsque tous les champs sont recus xxx est ce suffisant ???
			{
				uc_ControlerDateRetourVacances = 1;
			}
			
			if(us_Numero >= Chauf_zj_Auto && us_Numero <= Chauf_zsb2_Mode)	// Modification planning chauffage ou consigne -> recalcul consigne immédiatement
			{
				// Chauf_zj_Auto, Chauf_zn_Auto, Chauf_zsb1_Auto, Chauf_zsb2_Auto
				// Chauf_zj_Mode, Chauf_zn_Mode, Chauf_zsb1_Mode, Chauf_zsb2_Mode
				uc_ModeChauffageModifieParEcranOuScenario = 1;
			}
			
			// Gestion spécial pour les ordres provenant du serveur :
			if(uc_OrdreServeur != 0)
			{
				if(us_Numero == Chauf_zj_Mode)			uc_ChauffageModifieDepuisServeur_zj = 1;
				if(us_Numero == Chauf_zn_Mode)			uc_ChauffageModifieDepuisServeur_zn = 1;
				if(us_Numero == Chauf_zsb1_Mode)		uc_ChauffageModifieDepuisServeur_zsb1 = 1;
				if(us_Numero == Chauf_zsb2_Mode)		uc_ChauffageModifieDepuisServeur_zsb2 = 1;
				
				if(us_Numero == Cumulus_Mode)			uc_CumulusModifieDepuisServeur = 1;
			}

			l_uc_Retour = uc_BP_OK;
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"ACCES ECRITURE REFUSE %d\n", us_Numero);
			ul_EspionTableEchangeNbAccesEcritureRefuse++;
			l_uc_Retour = uc_BP_PB_ACCES;
		}
	}
	return l_uc_Retour;
}

// donnée lue dans la table d'échange ou 0
// lit une donnée dans la table d'échange
// traitement spécifique sur certaines données
unsigned char uc_TableEchange_Lit_Data(unsigned short us_Numero)
{
	unsigned char l_uc_Retour;
	
	l_uc_Retour = 0;
	if (us_Numero < Nb_Tbb_Donnees)
	{	// sécurité : donnée existante
		if((Tb_Echange_Droits[us_Numero] & ACCES_LECTURE) != 0)
		{
			l_uc_Retour = Tb_Echange[us_Numero];
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"ACCES LECTURE REFUSE %d\n", us_Numero);
			ul_EspionTableEchangeNbAccesLectureRefuse++;
		}
	}
	return l_uc_Retour;
}
