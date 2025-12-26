#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx

#include "EspionRS.h"
#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "global.h"
#include "hard.h"
#include "scenario.h"

#define extern
#include "reveil.h"
#undef extern


// les réveils provoquent l'ouverture automatique des volants roulants (1 réveil par chambre)
// les réveils sont paramétrés par l'opérateur (écran, internet) : heure et validation
// ils sont ensuite armés par 1 scénario
// ils ne s'exécutent que s'ils sont armés et validés
// ils sont automatiquement désarmés lorsqu'ils arrivent à échéance ou s'ils ne sont pas validés
// => ils ne s'exécutent donc qu'une fois

// cette fonction est appellée chaque minute
void vd_Reveil(void)
{
	if(st_Reveil_Arme.uc_ChGr != 0)	// grande chambre
	{	// réveil armé
		if(Tb_Echange[Reveil_ChambreGr_ON] != 0)   
		{	// réveil validé => surveille l'heure
			if(Tb_Echange[Reveil_ChambreGr_H] == st_DateHeure.HOUR && Tb_Echange[Reveil_ChambreGr_Mn] == st_DateHeure.MINUTE)	// xxx risque si traitement pas effectué chaque seconde
			{	// heure de réveil atteinte -> commande ouverture des volets 
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** REVEIL GRANDE CHAMBRE ***\n");
				us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_CHAMBRES] &= ~CHAMBRE_GR_VOLETS;
				us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] |= CHAMBRE_GR_VOLETS;
				st_Reveil_Arme.uc_ChGr = 0;	// désarme le réveil
			}   
		}
		else
		{	
			st_Reveil_Arme.uc_ChGr = 0;	// réveil dévalidé -> désarme le réveil
		}
	}
	if(st_Reveil_Arme.uc_Ch1 != 0)	// chambre 1
	{	// réveil armé
		if(Tb_Echange[Reveil_Chambre1_ON] != 0)	
		{	// réveil validé => surveille l'heure
			if(Tb_Echange[Reveil_Chambre1_H] == st_DateHeure.HOUR && Tb_Echange[Reveil_Chambre1_Mn] == st_DateHeure.MINUTE)
			{	// heure de réveil atteinte -> commande ouverture des volets
			   vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** REVEIL CHAMBRE 1 ***\n");
			   us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_CHAMBRES] &= ~CHAMBRE1_VOLETS;
			   us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] |= CHAMBRE1_VOLETS;
			   st_Reveil_Arme.uc_Ch1 = 0;	// désarme le réveil
			}
		}
		else
		{	
			st_Reveil_Arme.uc_Ch1 = 0;	// réveil dévalidé -> désarme le réveil
		}
	}
	if(st_Reveil_Arme.uc_Ch2 != 0)	// chambre 2
	{	// réveil armé
		if(Tb_Echange[Reveil_Chambre2_ON] != 0)
		{	// réveil validé => surveille l'heure
			if(Tb_Echange[Reveil_Chambre2_H] == st_DateHeure.HOUR && Tb_Echange[Reveil_Chambre2_Mn] == st_DateHeure.MINUTE)
			{	// heure de réveil atteinte -> commande ouverture des volets
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** REVEIL CHAMBRE 2 ***\n");
				us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_CHAMBRES] &= ~CHAMBRE2_VOLETS;
				us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] |= CHAMBRE2_VOLETS;
				st_Reveil_Arme.uc_Ch2 = 0;	// désarme le réveil
			}
		}
		else
		{	
			st_Reveil_Arme.uc_Ch2 = 0;	// réveil dévalidé -> désarme le réveil
		}
	}
	if(st_Reveil_Arme.uc_Ch3 != 0)	// chambre 3
	{	// réveil armé
		if(Tb_Echange[Reveil_Chambre3_ON] != 0)
		{	// réveil validé => surveille l'heure
			if(Tb_Echange[Reveil_Chambre3_H] == st_DateHeure.HOUR && Tb_Echange[Reveil_Chambre3_Mn] == st_DateHeure.MINUTE)
			{	// heure de réveil atteinte -> commande ouverture des volets
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** REVEIL CHAMBRE 3 ***\n");
				us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_CHAMBRES] &= ~CHAMBRE3_VOLETS;
				us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] |= CHAMBRE3_VOLETS;
				st_Reveil_Arme.uc_Ch3 = 0;	// désarme le réveil
			}
		}
		else
		{	
			st_Reveil_Arme.uc_Ch3 = 0;	// réveil dévalidé -> désarme le réveil
		}
	}
	if(st_Reveil_Arme.uc_Bur != 0)	// bureau
	{	// réveil armé
		if (Tb_Echange[Reveil_Bureau_ON] != 0)
		{	
			// réveil validé => surveille l'heure
			if(Tb_Echange[Reveil_Bureau_H] == st_DateHeure.HOUR && Tb_Echange[Reveil_Bureau_Mn] == st_DateHeure.MINUTE)
			{	// heure de réveil atteinte -> commande ouverture des volets
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** REVEIL BUREAU ***\n");
				us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_PIECES_VIE] &= ~BUREAU_VOLETS;
				us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_PIECES_VIE] |= BUREAU_VOLETS;
				st_Reveil_Arme.uc_Bur = 0;	// désarme le réveil
			}
		}
		else
		{	
			st_Reveil_Arme.uc_Bur = 0;	// réveil dévalidé -> désarme le réveil
		}
	}
}

void vd_VerifierDateRetourVacances(void)
{
	TIME_STRUCT l_time_mqx;
	DATE_STRUCT l_st_DateHeure;
	boolean l_b_Retour;
	uint_16 l__uint16_MoisSaisi;
	
	if(uc_ControlerDateRetourVacances != 0)
	{
		uc_ControlerDateRetourVacances = 0;
		
		// Changement date retour -> verification saisie - correction date si pb
		l_st_DateHeure.MILLISEC = 0;
		l_st_DateHeure.SECOND = 0;
		l_st_DateHeure.MINUTE = 0;
		l_st_DateHeure.HOUR = 0;
		l_st_DateHeure.DAY = Tb_Echange[VacanceFin_J];
		l_st_DateHeure.MONTH = Tb_Echange[VacanceFin_M];
		l_st_DateHeure.YEAR = (uint_16)(Tb_Echange[VacanceFin_A] + 2000);
		
		l__uint16_MoisSaisi = l_st_DateHeure.MONTH;
		
		l_b_Retour = _time_from_date(&l_st_DateHeure, &l_time_mqx);
	    if(l_b_Retour != TRUE)
	    {
	    	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE, "ERREUR VERIFICATION DATE RETOUR VACANCES %d\n", l_b_Retour);
	    }
	    else
	    {
	    	if(l__uint16_MoisSaisi != l_st_DateHeure.MONTH)
	    	{
	    		// Date corrigée par MQX -> forcer à 1 le jour et prendre le nouveau mois
	    		Tb_Echange[VacanceFin_M] = (unsigned char)l_st_DateHeure.MONTH;
	    		Tb_Echange[VacanceFin_J] = 1;
	    		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE, "CORRECTION AUTO DATE RETOUR VACANCES\n");
	    	}
	    }
	}
}

// cette fonction est appellée chaque minute
void vd_RetourVacances(void)
{
	unsigned char l_uc_Valeur;
	
	if(uc_DernierScenarioLance == uc_SCENARIO_JE_PARS_EN_VACANCES)
	{
		// xxx risque si traitement pas effectué chaque seconde //xxx a proteger par mutex
		if((Tb_Echange[VacanceFin_A]+2000) == st_DateHeure.YEAR &&
		   Tb_Echange[VacanceFin_M] == st_DateHeure.MONTH &&
		   Tb_Echange[VacanceFin_J] == st_DateHeure.DAY &&
		   Tb_Echange[VacanceFin_H] == st_DateHeure.HOUR &&
		   Tb_Echange[VacanceFin_Mn] == st_DateHeure.MINUTE)
		{
			uc_DernierScenarioLance = uc_SCENARIO_RAZ;	// Pour ne faire le traitement qu'UNE seule fois
			
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** RETOUR VACANCES ***\n");
			
			// Forcer les modes du chauffage pour toutes les zones
			// Si mode mis par uc_SCENARIO_JE_PARS_EN_VACANCES est 0x80 (0x80 := Continuer le fonctionnement actuel) -> NE RIEN FAIRE A LA DATE DE RETOUR !!!
			// A NE FAIRE QUE SI PAS DE MODIFICATION DE MODE VIA LE SERVEUR - SINON L'UTILISATEUR A REPRIS LA MAIN SUR L'AUTOMATISME -> ON RESTE COMME CA
			//xxx proteger par mutex
			
			if(uc_ChauffageModifieDepuisServeur_zj == 0 && Tb_Echange[Scenario3+Scenario_Chauf_zj] != 0x80)
			{
				if(_mutex_lock(&st_Mutex_ZJ) != MQX_OK)
				{
					DETECTION_ERREUR_SCENARIO_MUTEX_ZJ;
				}
				else
				{
					l_uc_Valeur = Tb_Echange[Chauf_zj_Mode];	// :=> 0x40 := Reprendre le dernier fonctionnement mémorisé - NON GERE DANS VACANCE FIN !!! -> IDEM 0x80
					vd_Scenario_Chauffage(Tb_Echange[VacanceFin_zj_Force], &Tb_Echange[Chauf_zj_Mode], &l_uc_Valeur, 0);
					_mutex_unlock(&st_Mutex_ZJ);
				}
			}
			
			if(uc_ChauffageModifieDepuisServeur_zn == 0 && Tb_Echange[Scenario3+Scenario_Chauf_zn] != 0x80)
			{
				if(_mutex_lock(&st_Mutex_ZN) != MQX_OK)
				{
					DETECTION_ERREUR_SCENARIO_MUTEX_ZN;
				}
				else
				{
					l_uc_Valeur = Tb_Echange[Chauf_zn_Mode];	// :=> 0x40 := Reprendre le dernier fonctionnement mémorisé - NON GERE DANS VACANCE FIN !!! -> IDEM 0x80
					vd_Scenario_Chauffage(Tb_Echange[VacanceFin_zn_Force], &Tb_Echange[Chauf_zn_Mode], &l_uc_Valeur, 0);
					_mutex_unlock(&st_Mutex_ZN);
				}
			}
			
			// Zones SDB : fonctionnement spécial !
			// Si mode mis par uc_SCENARIO_JE_PARS_EN_VACANCES est "arrêter le chauffage" (0x10) -> on remet comme avant le scénario - sinon on ne fait rien
			if(uc_ChauffageModifieDepuisServeur_zsb1 == 0 && Tb_Echange[Scenario3+Scenario_Chauf_zsb1] == 0x10)
			{
				if(_mutex_lock(&st_Mutex_Zsdb1) != MQX_OK)
				{
					DETECTION_ERREUR_SCENARIO_MUTEX_SDB1;
				}
				else
				{
					l_uc_Valeur = uc_Chauf_zsb1_Mode_EtatLorsDernierScenario;	// :=> 0x40 := Reprendre le dernier fonctionnement mémorisé - NON GERE DANS VACANCE FIN !!! -> IDEM 0x80
					vd_Scenario_Chauffage(uc_Chauf_zsb1_Mode_EtatLorsDernierScenario, &Tb_Echange[Chauf_zsb1_Mode], &l_uc_Valeur, 1);
					_mutex_unlock(&st_Mutex_Zsdb1);
				}
			}
			
			if(uc_ChauffageModifieDepuisServeur_zsb2 == 0 && Tb_Echange[Scenario3+Scenario_Chauf_zsb2] == 0x10)
			{
				if(_mutex_lock(&st_Mutex_Zsdb2) != MQX_OK)
				{
					DETECTION_ERREUR_SCENARIO_MUTEX_SDB2;
				}
				else
				{
					l_uc_Valeur = uc_Chauf_zsb2_Mode_EtatLorsDernierScenario;	// :=> 0x40 := Reprendre le dernier fonctionnement mémorisé - NON GERE DANS VACANCE FIN !!! -> IDEM 0x80
					vd_Scenario_Chauffage(uc_Chauf_zsb2_Mode_EtatLorsDernierScenario, &Tb_Echange[Chauf_zsb2_Mode], &l_uc_Valeur, 1);
					_mutex_unlock(&st_Mutex_Zsdb2);
				}
			}
			uc_ModeChauffageModifieParEcranOuScenario = 1;
			
			if(uc_CumulusModifieDepuisServeur == 0 && uc_Cumulus_ForcerAOnAuRetourVacancesProgramme != 0)
			{
				Tb_Echange[Cumulus_Mode] = uc_CUMULUS_ON;
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"*** RETOUR VACANCES -> CUMULUS FORCE A ON ***\n");
			}
			uc_Cumulus_ForcerAOnAuRetourVacancesProgramme = 0;
		}
	}
}
