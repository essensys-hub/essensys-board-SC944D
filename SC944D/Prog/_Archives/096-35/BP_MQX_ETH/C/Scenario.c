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

#define extern
#include "Scenario.h"
#undef extern


// inversion bit à bit d'un octet
unsigned char uc_Inv_Octet(unsigned char uc_depart)
{
	unsigned char l_uc_result, l_uc_masque_org, l_uc_masque_dest;
	unsigned short l_us_i;
	
	l_uc_result = 0;
	l_uc_masque_org = 0x80;	l_uc_masque_dest = 0x01;
	for(l_us_i=0 ; l_us_i<8 ; l_us_i++, l_uc_masque_org = l_uc_masque_org>>1, l_uc_masque_dest =l_uc_masque_dest<<1)
	{
		if((uc_depart & l_uc_masque_org) != 0)
		{
			l_uc_result |= l_uc_masque_dest;
		}
	}
	return(l_uc_result);
}

// Convertie l'ordre de pilotage du scenario en ordre de pilotage chauffage
// Mémorise dans p_uc_DernierEtatChauffageAvantScenario le mode de chauffage AVANT changement par le scénario
// Traitement spécial pour les zones sdb :
//		Zone sdb     : changement consigne meme si mode forcé à OFF
//		Autres zones : changement consigne que si mode différent de forcé à OFF 
void vd_Scenario_Chauffage(unsigned char uc_ChauffageScenario, unsigned char *p_uc_Chauffage, unsigned char *p_uc_DernierEtatChauffageAvantScenario, unsigned char uc_ZoneSdb)
{
	unsigned char l_uc_EtatChauffageAvantScenario;
	
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
	
	l_uc_EtatChauffageAvantScenario = *p_uc_Chauffage;
	if(*p_uc_Chauffage != 0x10 || uc_ZoneSdb != 0)	// action executée uniquement si le chauffage n'est pas à l'arrêt (forcé) OU zone SDB
	{
		switch(uc_ChauffageScenario)
		{
			case 0x40:
				if((*p_uc_DernierEtatChauffageAvantScenario & 0x30) == 0x20)	// Avant scénario, on était en mode anticipé -> revenir en mode AUTO
				{
					*p_uc_Chauffage = 0;
				}
				else
				{
					*p_uc_Chauffage = *p_uc_DernierEtatChauffageAvantScenario;	// Sinon revenir dans le mode précédent l'exécution du scénario
				}
			break;

			case 0x80:	// Ne rien faire
			break;

			default:	// Appliquer la consigne du scénario
				*p_uc_Chauffage = uc_ChauffageScenario;
			break;
		}
	}
	*p_uc_DernierEtatChauffageAvantScenario = l_uc_EtatChauffageAvantScenario;
}

// Initialisation d'un scénario
//
// paramètres d'entrées :
// us_AdresseScenario		Adresse de début de définition du scénario dans la table d'échange
//
// Init 					Valeurs à mettre dans le scénario spécifié
//							-> utiliser l'enum SCENARIOS
//	init avec les valeurs par défaut du document 
//	_Essensys - Spécification générale v3.docx	du 03/12/2012 xxx
void vd_Scenario_Init(unsigned short us_AdresseScenario, unsigned char uc_ValeurInit)
{
	unsigned short l_us_Compteur;
	
	
	// Controle adresse
	if(us_AdresseScenario < Scenario1 || (us_AdresseScenario + Scenario_NB_VALEURS) > Nb_Tbb_Donnees)	//xxx Scenario_NB_VALEURS - 1 sans le -1 ca devrait passer en erreur...
	{
		vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"SCENARIO INIT : ADRESSES EN DEHORS PLAGE (%d)\n",us_AdresseScenario);
		ul_ScenariosInitAdressesEnDehorsPlage++;
		return;
	}
	
	switch(uc_ValeurInit)
	{
		case uc_SCENARIO_RAZ:
			for(l_us_Compteur = 0;l_us_Compteur++;l_us_Compteur < Scenario_NB_VALEURS)
			{
				Tb_Echange[us_AdresseScenario + l_us_Compteur] = 0;
			}
		break;
		case uc_SCENARIO_SERVEUR:
			for(l_us_Compteur = 0;l_us_Compteur++;l_us_Compteur < Scenario_NB_VALEURS)
			{
				Tb_Echange[us_AdresseScenario + l_us_Compteur] = 0;
			}
		break;
		case uc_SCENARIO_JE_SORS:
			vd_Scenario_Init_Valeurs_JE_SORS(us_AdresseScenario);
		break;
		case uc_SCENARIO_JE_PARS_EN_VACANCES:
			vd_Scenario_Init_Valeurs_JE_PARS_EN_VACANCES(us_AdresseScenario);
		break;
		case uc_SCENARIO_JE_RENTRE:
			vd_Scenario_Init_Valeurs_JE_RENTRE(us_AdresseScenario);
		break;
		case uc_SCENARIO_JE_ME_COUCHE:
			vd_Scenario_Init_Valeurs_JE_ME_COUCHE(us_AdresseScenario);
		break;
		case uc_SCENARIO_JE_ME_LEVE:
			vd_Scenario_Init_Valeurs_JE_ME_LEVE(us_AdresseScenario);
		break;
		case uc_SCENARIO_PERSO:
			vd_Scenario_Init_Valeurs_Perso1(us_AdresseScenario);
		break;
		case uc_SCENARIO_LIBRE:
			vd_Scenario_Init_Valeurs_Perso2(us_AdresseScenario);
		break;
	}
}
void vd_Scenario_Init_Valeurs_JE_SORS(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0x08;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0x3F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0x1F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0x07;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}
void vd_Scenario_Init_Valeurs_JE_PARS_EN_VACANCES(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 255;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0x08;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0x3F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0x1F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0x07;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x15;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x15;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x10;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x10;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x02;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}
void vd_Scenario_Init_Valeurs_JE_RENTRE(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0x3F;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0x1F;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0x07;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 2;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x40;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x40;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x40;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x40;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x40;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}
void vd_Scenario_Init_Valeurs_JE_ME_COUCHE(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] =1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0x08;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0x3F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0x1F;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0x07;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}
void vd_Scenario_Init_Valeurs_JE_ME_LEVE(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] =0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0x3F;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0x1F;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0x07;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 2;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}

void vd_Scenario_Init_Valeurs_Perso1(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}

void vd_Scenario_Init_Valeurs_Perso2(unsigned short us_AdresseScenario)
{
	Tb_Echange[us_AdresseScenario + Scenario_Confirme_Scenario] = 1;
	Tb_Echange[us_AdresseScenario + Scenario_Alarme_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Code] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuv] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect1SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_Detect2SurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_DetectOuvSurVoieAcces] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneInt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_SireneExt] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_BloqueVolets] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_AlarmeConfig + AlarmeConfig_ForcerEclairage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Securite] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Machines] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Cumulus] = 0x80;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_Reglage] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] = 0;
	Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;
}

// Exécution d'un scénario en utilisant la config stockée à l'adresse spécifiée
// SCENARIO_SERVEUR : ne peut QUE agir sur les volets !!!
// Scenario_Efface					-> Traité dans le Main, indépendamment de l'exécution du scénario
void vd_Scenario_Exec(unsigned short us_AdresseScenario, unsigned char uc_NumeroScenario)
{
	unsigned char l_uc_EtatCumulusAvantScenario;
	unsigned char l_uc_Ouvrir;
	unsigned char l_uc_Fermer;
	
	
	// Controle adresse
	if(us_AdresseScenario < Scenario1 || (us_AdresseScenario + Scenario_NB_VALEURS) > Nb_Tbb_Donnees)	//xxx Scenario_NB_VALEURS - 1 sans le -1 ca devrait passer en erreur...
	{
		vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"SCENARIO : ADRESSES EN DEHORS PLAGE (%d)\n",us_AdresseScenario);
		ul_ScenariosAdressesEnDehorsPlage++;
		return;
	}
	
	// Scenario_Confirme_Scenario		 -> Géré par l'écran
	// Scenario_Alarme_ON				 -> Géré par l'écran
	// Scenario_AlarmeConfig			 -> Géré par l'écran / config prise en compte lors de l'activation de l'alarme (c'est l'écran qui dit quelle config utiliser !)
	// Scenario_Reveil_Reglage			 -> Géré par l'écran
	// Scenario_Efface					 -> Géré en dehors de cette fonction

	if(uc_NumeroScenario != uc_SCENARIO_SERVEUR)
	{
		// Réveil - A FAIRE AVANT FORCAGE VOLETS
		// Scenario_Reveil_Reglage			-> Geré par l'écran
		if(Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] == 1)
		{	// Arme le réveil pour toutes les chambres et bureaux
			vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"Reveil activé\n");
			st_Reveil_Arme.uc_ChGr = 1;
			st_Reveil_Arme.uc_Ch1 = 1;
			st_Reveil_Arme.uc_Ch2 = 1;
			st_Reveil_Arme.uc_Ch3 = 1;
			st_Reveil_Arme.uc_Bur = 1;
		}
		else if(Tb_Echange[us_AdresseScenario + Scenario_Reveil_ON] == 2)
		{	// Désactive le réveil pour toutes les chambres et bureaux
			vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"Reveil désactivé\n");
			st_Reveil_Arme.uc_ChGr = 0;
			st_Reveil_Arme.uc_Ch1 = 0;
			st_Reveil_Arme.uc_Ch2 = 0;
			st_Reveil_Arme.uc_Ch3 = 0;
			st_Reveil_Arme.uc_Bur = 0;
		}

		// Eclairage
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_PIECES_VIE] = Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_PIECES_VIE] |= (Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+uc_BOITIER_PIECES_VIE] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDV_MSB]);
		
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_CHAMBRES] = Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_CHAMBRES] |= (Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+uc_BOITIER_CHAMBRES] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Eteindre_CHB_MSB]);
		
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_PIECES_EAU] = Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+uc_BOITIER_PIECES_EAU] |= (Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+uc_BOITIER_PIECES_EAU] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Eteindre_PDE_MSB]);
		
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_PIECES_VIE] = Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_PIECES_VIE] |= (Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+uc_BOITIER_PIECES_VIE] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDV_MSB]);
		
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_CHAMBRES] = Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_CHAMBRES] |= (Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+uc_BOITIER_CHAMBRES] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Allumer_CHB_MSB]);
		
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_PIECES_EAU] = Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_LSB];
		us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+uc_BOITIER_PIECES_EAU] |= (Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB] << 8);
		
		us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+uc_BOITIER_PIECES_EAU] = uc_Inv_Octet(Tb_Echange[us_AdresseScenario + Scenario_Allumer_PDE_MSB]);
	}
	
	// volets & store
	// !!! Ouvrir les volets sauf ceux avec fonction réveil active !!!

	// Gestion spéciale sur le store : peut être utilisé comme volet - dans ce cas -> inverser la commande du store
		// Store_VR
		// 0x00 = Store utilisé comme store.
		// 0x01 = Store utilisé comme 15ème VR.
		// ETAT INITIAL = 0x00.
	
		//Scenario_OuvrirVolets_PDE,	// 0  = ne rien faire / combinaison des valeurs suivantes pour ouvrir les volets voulus
							// 8  = remonter le store de la terrasse
		//Scenario_FermerVolets_PDE,	// 0  = ne rien faire / combinaison des valeurs suivantes pour fermer les volets voulus
							// 8  = sortir le store de la terrasse

	l_uc_Ouvrir = Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDE];
	l_uc_Fermer = Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDE];

	if(uc_NumeroScenario == uc_SCENARIO_SERVEUR)	// Inversion à ne faire que pour scénario SERVEUR (autres scénarios : inversion gérée par l'écran)
	{
		if(Tb_Echange[Store_VR] != 0)	// Store utilisé en volet roulant
		{
			l_uc_Ouvrir &= ~(STORE);	// Bloque toute remontée du volet

			if(Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV] != 0)	// Si descente volets PDV
			{
				l_uc_Fermer |= STORE;	// Descendre aussi ce volet roulant
			}
		}
	}
	us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_PIECES_EAU] = l_uc_Ouvrir;	
	us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_PIECES_EAU] = l_uc_Fermer;

	
	us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_PIECES_VIE] = Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_PDV];	
	if(st_Reveil_Arme.uc_Bur != 0)
	{
		us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_PIECES_VIE] &= ~BUREAU_VOLETS;
	}

	us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] = Tb_Echange[us_AdresseScenario + Scenario_OuvrirVolets_CHB];
	if(st_Reveil_Arme.uc_ChGr != 0)
	{
		us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] &= ~CHAMBRE_GR_VOLETS;
	}
	if(st_Reveil_Arme.uc_Ch1 != 0)
	{
		us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] &= ~CHAMBRE1_VOLETS;
	}
	if(st_Reveil_Arme.uc_Ch2 != 0)
	{
		us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] &= ~CHAMBRE2_VOLETS;
	}
	if(st_Reveil_Arme.uc_Ch3 != 0)
	{
		us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_CHAMBRES] &= ~CHAMBRE3_VOLETS;
	}

	us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_PIECES_VIE] = Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_PDV];
	us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+uc_BOITIER_CHAMBRES] = Tb_Echange[us_AdresseScenario + Scenario_FermerVolets_CHB];
	
	if(uc_NumeroScenario != uc_SCENARIO_SERVEUR)
	{
		// Sécurité
		if(Tb_Echange[us_AdresseScenario + Scenario_Securite] == 1)
		{
			//xxx util ? if(Tb_Echange[Securite_PriseCoupe] == 0)
			st_EchangeStatus.uc_SécuOFF = 1;
		}
		else if(Tb_Echange[us_AdresseScenario + Scenario_Securite] == 2)
		{
			st_EchangeStatus.uc_SécuOFF = 0;	// Remettre prises sécurité
		}
	
		if(Tb_Echange[us_AdresseScenario + Scenario_Machines] == 1)
		{
			st_EchangeStatus.uc_MachinesOFF = 1;
		}
		else if(Tb_Echange[us_AdresseScenario + Scenario_Machines] == 2)
		{
			st_EchangeStatus.uc_MachinesOFF = 0;
		}

		// Chauffage -> prendre le mode du scénario lance
		// !!! Pour scénario JE RENTRE -> ne prendre le mode du scénario QUE si pas d'action serveur depuis dernier scenario lancé !!!
		//xxx proteger par mutex
		if(uc_NumeroScenario != uc_SCENARIO_JE_RENTRE || (uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zj == 0))
		{
			if(_mutex_lock(&st_Mutex_ZJ) != MQX_OK)
			{
				DETECTION_ERREUR_SCENARIO_MUTEX_ZJ;
			}
			else
			{
				vd_Scenario_Chauffage(Tb_Echange[us_AdresseScenario + Scenario_Chauf_zj], &Tb_Echange[Chauf_zj_Mode], &uc_Chauf_zj_Mode_EtatLorsDernierScenario, 0);
				_mutex_unlock(&st_Mutex_ZJ);
			}
		}
		else if(uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zj != 0)	// Memorisation etat en cours dans ce cas
		{
			uc_Chauf_zj_Mode_EtatLorsDernierScenario = Tb_Echange[Chauf_zj_Mode];
		}
		
		if(uc_NumeroScenario != uc_SCENARIO_JE_RENTRE || (uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zn == 0))
		{
			if(_mutex_lock(&st_Mutex_ZN) != MQX_OK)
			{
				DETECTION_ERREUR_SCENARIO_MUTEX_ZN;
			}
			else
			{
				vd_Scenario_Chauffage(Tb_Echange[us_AdresseScenario + Scenario_Chauf_zn], &Tb_Echange[Chauf_zn_Mode], &uc_Chauf_zn_Mode_EtatLorsDernierScenario, 0);
				_mutex_unlock(&st_Mutex_ZN);
			}
		}
		else if(uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zn != 0)	// Memorisation etat en cours dans ce cas
		{
			uc_Chauf_zn_Mode_EtatLorsDernierScenario = Tb_Echange[Chauf_zn_Mode];
		}
		
		if(uc_NumeroScenario != uc_SCENARIO_JE_RENTRE || (uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zsb1 == 0))
		{
			if(_mutex_lock(&st_Mutex_Zsdb1) != MQX_OK)
			{
				DETECTION_ERREUR_SCENARIO_MUTEX_SDB1;
			}
			else
			{
				vd_Scenario_Chauffage(Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb1], &Tb_Echange[Chauf_zsb1_Mode], &uc_Chauf_zsb1_Mode_EtatLorsDernierScenario, 1);
				_mutex_unlock(&st_Mutex_Zsdb1);
			}
		}
		else if(uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zsb1 != 0)	// Memorisation etat en cours dans ce cas
		{
			uc_Chauf_zsb1_Mode_EtatLorsDernierScenario = Tb_Echange[Chauf_zsb1_Mode];
		}
		
		if(uc_NumeroScenario != uc_SCENARIO_JE_RENTRE || (uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zsb2 == 0))
		{
			if(_mutex_lock(&st_Mutex_Zsdb2) != MQX_OK)
			{
				DETECTION_ERREUR_SCENARIO_MUTEX_SDB2;
			}
			else
			{
				vd_Scenario_Chauffage(Tb_Echange[us_AdresseScenario + Scenario_Chauf_zsb2], &Tb_Echange[Chauf_zsb2_Mode], &uc_Chauf_zsb2_Mode_EtatLorsDernierScenario, 1);
				_mutex_unlock(&st_Mutex_Zsdb2);
			}
		}
		else if(uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_ChauffageModifieDepuisServeur_zsb2 != 0)	// Memorisation etat en cours dans ce cas
		{
			uc_Chauf_zsb2_Mode_EtatLorsDernierScenario = Tb_Echange[Chauf_zsb2_Mode];
		}
		
		uc_ModeChauffageModifieParEcranOuScenario = 1;

		// RAZ a chaque execution d'un scenario 
		uc_ChauffageModifieDepuisServeur_zj = 0;
		uc_ChauffageModifieDepuisServeur_zn = 0;
		uc_ChauffageModifieDepuisServeur_zsb1 = 0;
		uc_ChauffageModifieDepuisServeur_zsb2 = 0;

	
		// cumulus
		
		// Cumulus_Modex :
		// 0 = Autonome (ON) / 1 = gestion heures creuses / 2 = OFF
		
		// Scenario_Cumulus :
		// 0x00 = Forçage mode autonome (= HP permanentes) <- idem "Cumulus_Mode"
		// 0x01 = Forçage gestion HP/HC <- idem "Cumulus_Mode"
		// 0x02 = Forçage OFF (= HC permanent) <- idem "Cumulus_Mode"
		// 0x40 := Reprendre le dernier fonctionnement mémorisé
		// 0x80 := Continuer le fonctionnement actuel
		l_uc_EtatCumulusAvantScenario = Tb_Echange[Cumulus_Mode];
		uc_Cumulus_ForcerAOnAuRetourVacancesProgramme = 0;	// RAZ a chaque exécution de scénario - Scénario serveur : on reprend la main sur l'automatisme donc on supprime ce flag
		if(uc_NumeroScenario != uc_SCENARIO_JE_RENTRE || (uc_NumeroScenario == uc_SCENARIO_JE_RENTRE && uc_CumulusModifieDepuisServeur == 0))
		{
			switch(Tb_Echange[us_AdresseScenario + Scenario_Cumulus])
			{
				case 0x40:
					Tb_Echange[Cumulus_Mode] = uc_Cumulus_Mode_EtatLorsDernierScenario;
				break;
				
				case 0x80:	// Ne rien faire
				break;
				
				default:
					if(Tb_Echange[Cumulus_Mode] != uc_CUMULUS_OFF && Tb_Echange[us_AdresseScenario + Scenario_Cumulus] == uc_CUMULUS_OFF)
					{
						// Le scénario va couper le cumulus alors que ce dernier est utilisé
						// -> Armer le flag suivant pour remettre en route le cumulus au retour de vacances programmé
						// A faire UNIQUEMENT si cumulus différent de OFF !!!!
						uc_Cumulus_ForcerAOnAuRetourVacancesProgramme = 1;
					}
					Tb_Echange[Cumulus_Mode] = Tb_Echange[us_AdresseScenario + Scenario_Cumulus];
				break;
			}
		}
		uc_Cumulus_Mode_EtatLorsDernierScenario = l_uc_EtatCumulusAvantScenario;
		
		uc_CumulusModifieDepuisServeur = 0;
	}
}

// Exécution d'un scénario par son numéro
void vd_Scenario_Cde(unsigned char uc_Numero)
{
	unsigned short l_us_AdresseScenario;

	vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"Exécution N° %d - %s\n",uc_Numero,puc_NomScenario(uc_Numero));
	
	if(uc_Numero > uc_SCENARIO_RAZ && uc_Numero < uc_SCENARIO_NB)
	{
		ul_ScenariosNbExecution[uc_Numero]++;
		l_us_AdresseScenario = (unsigned short)(uc_Numero * Scenario_NB_VALEURS + Scenario1);	// Adresse de la dernière valeur du scénario souhaité
		if(l_us_AdresseScenario <= Nb_Tbb_Donnees)
		{
			// compris dans la table, donc OK
			l_us_AdresseScenario -= Scenario_NB_VALEURS;								// ramène en début de scénario
			vd_Scenario_Exec(l_us_AdresseScenario, uc_Numero);
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"@ valeurs incorrectes (%d)\n",l_us_AdresseScenario);
			ul_ScenariosAdressesValeursIncorrectes++;
		}
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"N° incorrect (%d)\n",uc_Numero);
		ul_ScenariosNumeroIncorrect++;
	}
	vd_EspionRS_Printf(uc_ESPION_SCENARIOS,"\n");
}

// Retoure le nom du scenario passé en paramètre (pour debug)
char *puc_NomScenario(unsigned char uc_Numero)
{
	switch(uc_Numero)
	{
		case uc_SCENARIO_RAZ:
			return "uc_SCENARIO_RAZ";
		break;
		case uc_SCENARIO_SERVEUR:
			return "uc_SCENARIO_SERVEUR";
		break;
		case uc_SCENARIO_JE_SORS:
			return "uc_SCENARIO_JE_SORS";
		break;
		case uc_SCENARIO_JE_PARS_EN_VACANCES:
			return "uc_SCENARIO_JE_PARS_EN_VACANCES";
		break;
		case uc_SCENARIO_JE_RENTRE:
			return "uc_SCENARIO_JE_RENTRE";
		break;
		case uc_SCENARIO_JE_ME_COUCHE:
			return "uc_SCENARIO_JE_ME_COUCHE";
		break;
		case uc_SCENARIO_JE_ME_LEVE:
			return "uc_SCENARIO_JE_ME_LEVE";
		break;
		case uc_SCENARIO_PERSO:
			return "uc_SCENARIO_PERSO";
		break;
		default:
			return "???";
		break;
	}
}

// xxx gerer retour vacances programme

void vd_GestionScenario(void)
{
	// Faire init scenario sur demande table echange
	vd_Scenario_TraitementDemandeInit(Scenario1);
	vd_Scenario_TraitementDemandeInit(Scenario2);
	vd_Scenario_TraitementDemandeInit(Scenario3);
	vd_Scenario_TraitementDemandeInit(Scenario4);
	vd_Scenario_TraitementDemandeInit(Scenario5);
	vd_Scenario_TraitementDemandeInit(Scenario6);
	vd_Scenario_TraitementDemandeInit(Scenario7);
	vd_Scenario_TraitementDemandeInit(Scenario8);

	// Traitement des scénarios
	if (Tb_Echange[Scenario] != 0)	//xxx mutex
	{
		vd_Scenario_Cde(Tb_Echange[Scenario]);
		if(Tb_Echange[Scenario] != uc_SCENARIO_RAZ && Tb_Echange[Scenario] != uc_SCENARIO_SERVEUR)
		{	// Ne pas mémoriser l'exécution des scénarios uc_SCENARIO_RAZ et uc_SCENARIO_SERVEUR qui sont des moyens techniques pour interagir mais pas des scénarios à proprement parlé !
			Tb_Echange[Scenario_DernierLance] = Tb_Echange[Scenario];
			uc_DernierScenarioLance = Tb_Echange[Scenario];
		}
		Tb_Echange[Scenario] = 0; 		// commande prise en compte par le BP
	}
}

void vd_Scenario_TraitementDemandeInit(unsigned short us_AdresseScenario)
{
	if(Tb_Echange[us_AdresseScenario + Scenario_Efface] != 0)
	{
		//Scenario_Efface,					// 1= scénario à effacer (remise à 0 de tous les paramètres du scénario)
		//									// 2 à 6 = init par défaut de scénario prédéfini xxx constantes correspondante ???
		vd_Scenario_Init(us_AdresseScenario, Tb_Echange[us_AdresseScenario + Scenario_Efface]-1);
		Tb_Echange[us_AdresseScenario + Scenario_Efface] = 0;	// effacement effectué
	}
}
