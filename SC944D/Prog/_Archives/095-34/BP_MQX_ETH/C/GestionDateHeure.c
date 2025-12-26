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

#define extern
#include  "gestiondateheure.h"
#undef extern


void vd_Init_Horloge(void)
{
    RTC_TIME_STRUCT l_time_rtc;
    TIME_STRUCT l_time_mqx;
    _mqx_int l_sl_Retour;
    boolean l_b_Retour;

    _rtc_get_time(&l_time_rtc);	// Get the date stored in the RTC registers
    l_sl_Retour = _rtc_sync_with_mqx(TRUE);	// Set the MQX time to the time defined by the RTC registers
    if(l_sl_Retour != MQX_OK)
    {
    	DETECTION_ERREUR_RTC_INIT_RTC_MQX;
		vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur init RTC %d\n",l_sl_Retour);
    }
    
	_time_get(&l_time_mqx);  				// Get the current time
	l_b_Retour = _time_to_date(&l_time_mqx,&st_DateHeure);	// Get the current time in MQX date format
	if(l_b_Retour != TRUE)
	{
		DETECTION_ERREUR_RTC_INIT_CONVERSION_DATE;
		vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion date %d\n",l_b_Retour);
	}
}

// Met a jour st_DateHeure et uc_JourSemaine utilisés par l'applicatif
// Prend en compte une nouvelle date / heure envoyée par l'écran -> MAJ RTC BP
void vd_rtc(void)
{
    TIME_STRUCT l_time_mqx;
    _mqx_int l_sl_Retour;
    boolean l_b_Retour;
    _mqx_uint l_uint_Retour;
    uint_16 l_uint16_MoisSaisi;

    
	st_DateHeure.MILLISEC = 0;
	st_DateHeure.SECOND =0;

	_time_get(&l_time_mqx);						// Récupère la date / heure MQX
    l_b_Retour = _time_to_date(&l_time_mqx,&st_DateHeure);	// Convertie la date / heure en format utilisable -> st_DateHeure
    if(l_b_Retour != TRUE)
    {
    	DETECTION_ERREUR_RTC_CONVERSION_DATE;
    	vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion date %d\n",l_b_Retour);
    }
    
	// Mise à jour date / heure par écran ?
	if(_lwsem_poll(&lwsem_wr_date)== TRUE)	// nonblocking - TRUE when it is available
	{	
		// prise en compte d'un changement opérateur
		// seule les valeurs reçues sont modifiées : permet le changement d'une seule valeur

		ul_EspionRTCNbMAJRTC++;
		vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"DEMANDE CHANGEMENT DATE / HEURE\n");
		vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tANCIENNE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);
		
		// Secondes et millisecondes -> remis à 0 si heure changée
		if(st_DateHeureEnvoyeeParEcran.MINUTE < 60 || st_DateHeureEnvoyeeParEcran.HOUR < 24)
		{
			st_DateHeure.MILLISEC = 0;
			st_DateHeure.SECOND = 0;
		}
		if(st_DateHeureEnvoyeeParEcran.MINUTE < 60)												st_DateHeure.MINUTE = st_DateHeureEnvoyeeParEcran.MINUTE;
		if(st_DateHeureEnvoyeeParEcran.HOUR < 24)												st_DateHeure.HOUR = st_DateHeureEnvoyeeParEcran.HOUR;
		if(st_DateHeureEnvoyeeParEcran.DAY <= 31 && st_DateHeureEnvoyeeParEcran.DAY > 0)		st_DateHeure.DAY = st_DateHeureEnvoyeeParEcran.DAY;
		if(st_DateHeureEnvoyeeParEcran.MONTH <= 12 && st_DateHeureEnvoyeeParEcran.MONTH > 0)	st_DateHeure.MONTH = st_DateHeureEnvoyeeParEcran.MONTH;
		if(st_DateHeureEnvoyeeParEcran.YEAR < 255)												st_DateHeure.YEAR = (uint_16)(st_DateHeureEnvoyeeParEcran.YEAR + 2000);	// années 2000 à 2254 uniquement
		
		l_uint16_MoisSaisi = st_DateHeure.MONTH;
		
		st_DateHeureEnvoyeeParEcran.MINUTE = 0xFF;		// RAZ nouvelles valeurs
		st_DateHeureEnvoyeeParEcran.HOUR = 0xFF;		// xxx risque ecrasement nouveau changement - a quoi sert la semaphore ???
		st_DateHeureEnvoyeeParEcran.DAY = 0xFF;
		st_DateHeureEnvoyeeParEcran.MONTH = 0xFF;
		st_DateHeureEnvoyeeParEcran.YEAR = 0xFF;

		// Prise en compte nouveau temps -> MQX - !!!! _time_from_date() met à jour également st_DateHeure avec la date reconvertie après vérification
		l_b_Retour = _time_from_date(&st_DateHeure, &l_time_mqx);
	    if(l_b_Retour != TRUE)
	    {
	    	DETECTION_ERREUR_RTC_CONVERSION_DATE2;
	    	vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion date2 %d\n",l_b_Retour);
	    }
	    
	    if(l_uint16_MoisSaisi != st_DateHeure.MONTH)
	    {
	    	// Date corrigée par MQX -> forcer à 1 le jour
	    	st_DateHeure.DAY = 1;
	    	vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"MOIS DIFFERENT SAISIE -> FORCAGE JOUR A 1\n");
	    	
			// Prise en compte nouveau temps -> MQX
			l_b_Retour = _time_from_date(&st_DateHeure, &l_time_mqx);
		    if(l_b_Retour != TRUE)
		    {
		    	DETECTION_ERREUR_RTC_CONVERSION_DATE2;
		    	vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion VERIF date3 %d\n",l_b_Retour);
		    }
	    }

		_time_set(&l_time_mqx);
		l_sl_Retour = _rtc_sync_with_mqx(FALSE);		// Write the MQX BSP time to the RTC registers xxx test retour
		if(l_sl_Retour != MQX_OK)
		{
	    	DETECTION_ERREUR_RTC_RTC_MQX;
			vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur init RTC MQX %d\n",l_sl_Retour);
		}
		uc_MinutePrecedente = 0xFF;		// Force le traitement à chaque changement de date / heure
		
		uc_ModeChauffageModifieParEcranOuScenario = 1;	//xxx bien placé ?

		vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tNOUVELLE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);
	}
	
	uc_JourSemaine = uc_CalculerJourSemaine();
	vd_EteHiver();

	// Mise à jour de la table d'échange
	//xxx mutex maj
	Tb_Echange[Minutes] = (unsigned char)st_DateHeure.MINUTE;
	Tb_Echange[Heure] = (unsigned char)st_DateHeure.HOUR;
	Tb_Echange[Jour] = (unsigned char)st_DateHeure.DAY;
	Tb_Echange[Mois] = (unsigned char)st_DateHeure.MONTH;
	if(st_DateHeure.YEAR >= 2000)		// années 2000 à 2099 uniquement
	{
		Tb_Echange[Annee] = (unsigned char)(st_DateHeure.YEAR -2000);
	}
	else
	{
		Tb_Echange[Annee] = 0;
	}
	
	// MAJ st_DateHeurePourEcran -> protection par mutex
	ul_CompteurMutexRTC++;
	l_uint_Retour = _mutex_lock(&st_Mutex_DateHeurePourEcran);
	if(l_uint_Retour != MQX_EOK)
	{
		printf("x3 %d\n", l_uint_Retour);
		printf("X3XXXXXXXXXXXXXXXX PB MUTEX XXXXXXXXXXXXXXXXXXXXXXXX\n");
		DETECTION_ERREUR_RTC_MUTEX;
		ul_CompteurMutexRTCErreur;
		vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur mutex!\n");
	}
	// On fait quand même le traitement...
	st_DateHeurePourEcran.YEAR = st_DateHeure.YEAR;
	st_DateHeurePourEcran.MONTH = st_DateHeure.MONTH;
	st_DateHeurePourEcran.DAY = st_DateHeure.DAY;
	st_DateHeurePourEcran.HOUR = st_DateHeure.HOUR;
	st_DateHeurePourEcran.MINUTE = st_DateHeure.MINUTE;
	st_DateHeurePourEcran.SECOND = st_DateHeure.SECOND;
	st_DateHeurePourEcran.MILLISEC = st_DateHeure.MILLISEC;
	_mutex_unlock(&st_Mutex_DateHeurePourEcran);	// Libération...	
}

// calcul du jour de la semaine, à partir de la date	//xxx calcul a verifier
// Jour de semaine D = { [(23m)/9] + d + 4 + y + [z/4] - [z/100] + [z/400] - 2 (si m >= 3) } mod 7 
// où: 
// D = Jour de semaine   (D = 0 à 6;   0 = Dimanche, 1 = Lundi, ... , 6 = Samedi) 
// [x] signifie le nombre entier résultant de la division, le reste étant ignoré 
// m = Mois   (m = 1 à 12;   1 = Janvier, 2 = Février, 3 = Mars, ... , 12 = Décembre)     m >= 3 signifie m supérieur ou égal à 3 
// d = Jour   (d = 1 à 31) 
// y = année 
// z = y - 1   si m <3 
// z = y        si m >= 3 
// Retourne N° jour -> 0 : Lundi, 1 : Mardi, ..., 6 : Dimanche
unsigned char uc_CalculerJourSemaine(void)
{
	uint_16	l_us_jour;
	uint_16 l_us_z;

	if(st_DateHeure.MONTH < 3)
	{
		l_us_z = st_DateHeure.YEAR -1;
		l_us_jour = (23 * st_DateHeure.MONTH) / 9 + st_DateHeure.DAY + 4 + st_DateHeure.YEAR + (l_us_z/4) - (l_us_z/100) + (l_us_z/400);
	}
	else
	{
		l_us_z = st_DateHeure.YEAR;
		l_us_jour = (23 * st_DateHeure.MONTH / 9) + st_DateHeure.DAY + 4 + st_DateHeure.YEAR + (l_us_z/4) - (l_us_z/100) + (l_us_z/400) - 2;
	}
	l_us_jour %= 7;
	
	// Décalage pour ESSENSYS -> 0 : Lundi, 1 : Mardi, ..., 6 : Dimanche
	if(l_us_jour == 0)	l_us_jour = 6;
	else				l_us_jour--;
	
	return((unsigned char)l_us_jour);
}

// passage heure d'été / heure d'hiver
// Le passage de l'heure d'hiver à l'heure d'été s'effectue le dernier dimanche de mars, 
// où à 2H du matin on fait +1 heure (il devient donc arbitrairement 3H du matin). 
// Le passage de l'heure d'été à l'heure d'hiver s'effectue le dernier dimanche d'octobre,
// où à 3H du matin on fait -1 heure (il devient donc arbitrairement 2H du matin).
void vd_EteHiver(void)
{
    TIME_STRUCT l_time_mqx;
    _mqx_int l_sl_Retour;
    boolean l_b_Retour;
    static unsigned char s_uc_PassageHeureHiverEffectue = 0;

    
    if(uc_JourSemaine == 6)	// Dimanche
	{	// dimanche
		if(st_DateHeure.MONTH == 3 &&		// mars
		   st_DateHeure.DAY > 24 &&			// dernier dimanche du mois
		   st_DateHeure.HOUR == 2)
		{
			ul_EspionRTCNbPassageHeureEte++;
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"PASSAGE HEURE ETE\n");
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tANCIENNE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);

			st_DateHeure.HOUR ++;			// + 1 heure
		   
			l_b_Retour = _time_from_date(&st_DateHeure, &l_time_mqx);
			if(l_b_Retour != TRUE)
		    {
		    	DETECTION_ERREUR_RTC_CONVERSION_HEURE_ETE;
		    	vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion heure ete %d\n",l_b_Retour);
		    }
			_time_set(&l_time_mqx);
			l_sl_Retour = _rtc_sync_with_mqx(FALSE);		// Write the MQX BSP time to the RTC registers 
			if(l_sl_Retour != MQX_OK)
			{
		    	DETECTION_ERREUR_RTC_RTC_MQX_HEURE_ETE;
				vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur init RTC MQX heure ete %d\n",l_sl_Retour);
			}
		
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tNOUVELLE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);
		}
		
		if(st_DateHeure.MONTH == 10 && 		// octobre
		   st_DateHeure.DAY > 24 &&			// dernier dimanche du mois
		   st_DateHeure.HOUR == 3 && 
		   st_DateHeure.MINUTE == 0 && 
		   s_uc_PassageHeureHiverEffectue == 0)
		{
			s_uc_PassageHeureHiverEffectue = 1;	// Pour éviter de refaire le traitement tout le temps...
			
			ul_EspionRTCNbPassageHeureHiver++;
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"PASSAGE HEURE HIVER\n");
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tANCIENNE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);

			st_DateHeure.HOUR --;			// - 1 heure
		   
			l_b_Retour = _time_from_date(&st_DateHeure, &l_time_mqx);
		    if(l_b_Retour != TRUE)
		    {
		    	DETECTION_ERREUR_RTC_CONVERSION_HEURE_HIVER;
		    	vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur conversion heure hiver %d\n",l_b_Retour);
		    }
			_time_set(&l_time_mqx);
			l_sl_Retour = _rtc_sync_with_mqx(FALSE);		// Write the MQX BSP time to the RTC registers 
			if(l_sl_Retour != MQX_OK)
			{
		    	DETECTION_ERREUR_RTC_RTC_MQX_HEURE_HIVER;
				vd_EspionRS_Printf(uc_ESPION_RTC_ERREUR,"Erreur init RTC MQX heure hiver %d\n",l_sl_Retour);
			}
			
			vd_EspionRS_Printf(uc_ESPION_RTC_ACTIVITE,"\tNOUVELLE DATE / HEURE : %02i/%02i/%04i %02i:%02i:%02i\n\n",st_DateHeure.DAY, st_DateHeure.MONTH, st_DateHeure.YEAR, st_DateHeure.HOUR, st_DateHeure.MINUTE, st_DateHeure.SECOND);
		}
	}
    if(uc_JourSemaine != 6)	// Autre jour que Dimanche
    {
    	s_uc_PassageHeureHiverEffectue = 0;
    }
}

// xxx et si traitement pas fait toutes les secondes sur chgt ete / hiver -> va louper le changement
// xxx traitement calculjoursemaine et changement etehiver tout le temps -> a priori pas de pb
