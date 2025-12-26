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
#include "filpilote.h"

#define extern
#include "chauffage.h"
#undef extern


// Gestion du chauffage
// Calcule pour chaque zone la consigne de pilotage (fil pilote) en fonction du mode en cours
// cette fonction est appellée chaque minute
void vd_Chauffage(void)
{
	// Zone jour
	if(_mutex_lock(&st_Mutex_ZJ) != MQX_OK)
	{
		DETECTION_ERREUR_CHAUFFAGE_MUTEX_ZJ;
	}
	else
	{
		vd_Chauffage_ParZone(
						&st_chauf_ZJ, 						// mode en cours
						&Tb_Echange[Chauf_zj_Auto], 		// planning horaire (mode auto)
						&Tb_Echange[Chauf_zj_Mode], 		// table d'echange mode demande
						&uc_ConsigneFilPilotePourIT_ZJ);	// Consigne pour générer fil pilote (sous IT)
		_mutex_unlock(&st_Mutex_ZJ);
	}

	// Zone nuit
	if(_mutex_lock(&st_Mutex_ZN) != MQX_OK)
	{
		DETECTION_ERREUR_CHAUFFAGE_MUTEX_ZN;
	}
	else
	{
		vd_Chauffage_ParZone(
						&st_chauf_ZN, 						// mode en cours
						&Tb_Echange[Chauf_zn_Auto], 		// planning horaire (mode auto)
						&Tb_Echange[Chauf_zn_Mode], 		// table d'echange mode demande
						&uc_ConsigneFilPilotePourIT_ZN);	// Consigne pour générer fil pilote (sous IT)
		_mutex_unlock(&st_Mutex_ZN);
	}	

	// zone salle de bain 1
	if(_mutex_lock(&st_Mutex_Zsdb1) != MQX_OK)
	{
		DETECTION_ERREUR_CHAUFFAGE_MUTEX_SDB1;
	}
	else
	{
		vd_Chauffage_ParZone(
						&st_chauf_Zsdb1, 					// mode en cours
						&Tb_Echange[Chauf_zsb1_Auto], 		// planning horaire (mode auto)
						&Tb_Echange[Chauf_zsb1_Mode], 		// table d'echange mode demande
						&uc_ConsigneFilPilotePourIT_SDB1);	// Consigne pour générer fil pilote (sous IT)
		_mutex_unlock(&st_Mutex_Zsdb1);
	}
	
	// zone salle de bain 2
	if(_mutex_lock(&st_Mutex_Zsdb2) != MQX_OK)
	{
		DETECTION_ERREUR_CHAUFFAGE_MUTEX_SDB2;
	}
	else
	{
		vd_Chauffage_ParZone(
						&st_chauf_Zsdb2, 					// mode en cours
						&Tb_Echange[Chauf_zsb2_Auto], 		// planning horaire (mode auto)
						&Tb_Echange[Chauf_zsb2_Mode], 		// table d'echange mode demande
						&uc_ConsigneFilPilotePourIT_SDB2);	// Consigne pour générer fil pilote (sous IT)
		_mutex_unlock(&st_Mutex_Zsdb2);
	}
	
	//xxx a deplacer et a mettre apres espion changement table d'echange
	//sinon on ne voit pas le changement du a l'ecran qui est tout de suite ecrase par le chauffage avant execution de l'espion ecran
	vd_Chauffage_Espion();
}

// Gestion du chauffage en fonction du mode
// Fonction commune à toutes les zones
// Retourne le mode et la consigne de chauffage active (fil pilote) dans pst_chauf
void vd_Chauffage_ParZone(
			pstruct_chauffage pst_chauf, 				// Mode en cours
			unsigned char *p_uc_planning, 				// Planning mode auto
			unsigned char *uc_ModeDemande,				// Table d'echange mode demande
			unsigned char *uc_ConsignePourPilotage)		// Variable utilisée pour générer le fil pilote (sous IT)
{
	unsigned char l_uc_ModeDemande;
	unsigned char l_uc_ConsigneDemande;
	unsigned char l_uc_Indice_Plage;
	unsigned char l_uc_Compteur;
	unsigned char l_uc_Consigne;
	unsigned char l_uc_Consigne2;
	unsigned char l_uc_HeureEnCours;
	

	
	l_uc_ModeDemande = (*uc_ModeDemande & 0x30) >> 4;
	l_uc_ConsigneDemande = *uc_ModeDemande & 0x0F;
	l_uc_HeureEnCours = st_DateHeure.HOUR;
	
	switch(l_uc_ModeDemande)
	{
		case uc_AUTO:
			// Convertie l'heure en indice sur les plages de chauffage
			// Plages de chauffage : 7 jours de 24 heures avec 1 plage par heure
			l_uc_Indice_Plage = (unsigned char)(uc_JourSemaine * 24 + l_uc_HeureEnCours);
			pst_chauf->uc_Mode = uc_AUTO;
			pst_chauf->uc_ConsigneFilPiloteEnCours = uc_Plage(p_uc_planning, l_uc_Indice_Plage);
		break;
		
		case uc_FORCE:
			// Mode forcé
			pst_chauf->uc_Mode = uc_FORCE;
			pst_chauf->uc_ConsigneFilPiloteEnCours = l_uc_ConsigneDemande;
		break;

		case uc_ANTICIPE:
			// Mode anticipé : anticipe le prochain mode de marche de la journée 
			
			if(pst_chauf->uc_Mode != uc_ANTICIPE)	// Passage en anticipé -> recherche prochaine plage
			{
				// Convertie l'heure en indice sur les plages de chauffage
				// Plages de chauffage : 7 jours de 24 heures avec 1 plage par heure
				l_uc_Indice_Plage = (unsigned char)(uc_JourSemaine * 24 + l_uc_HeureEnCours);
				l_uc_Consigne = uc_Plage(p_uc_planning, l_uc_Indice_Plage);
				pst_chauf->uc_ConsigneFilPiloteNonAnticipe = l_uc_Consigne;
				pst_chauf->uc_HeurePassageAnticipe = l_uc_HeureEnCours;
				
				// Recherche sur les prochaines 24 heures si une consigne est différente de celle en cours
				for(l_uc_Compteur = 0;l_uc_Compteur <= 24;l_uc_Compteur++)
				{
					if(l_uc_Indice_Plage >= 168)	l_uc_Indice_Plage = 0;	// Rebouclage à Lundi 0h
					l_uc_Consigne2 = uc_Plage(p_uc_planning, l_uc_Indice_Plage);
					
					if(l_uc_Consigne != l_uc_Consigne2)
					{
						l_uc_Compteur = 24;	// Fin de boucle
					}
					l_uc_Indice_Plage++;
				}

				// On se met en anticipé dans tous les cas
				// Si consigne identique a actuel (on n'en a pas trouvé d'autre) -> on se remet en auto a la prochaine plage planning
				pst_chauf->uc_Mode = uc_ANTICIPE;
				pst_chauf->uc_ConsigneFilPiloteEnCours = l_uc_Consigne2;
			}
			else
			{
				// Mode anticipé : anticipe le prochain mode de marche de la journée 
				// Convertie l'heure en indice sur les plages de chauffage
				// Plages de chauffage : 7 jours de 24 heures avec 1 plage par heure
				l_uc_Indice_Plage = (unsigned char)(uc_JourSemaine * 24 + l_uc_HeureEnCours);
				l_uc_Consigne = uc_Plage(p_uc_planning, l_uc_Indice_Plage);

				if(pst_chauf->uc_ConsigneFilPiloteEnCours == pst_chauf->uc_ConsigneFilPiloteNonAnticipe)
				{	// Cas ou aucun mode different n'a ete trouve -> on reste en mode anticipe jusqu'au changement heure
					if(l_uc_HeureEnCours != pst_chauf->uc_HeurePassageAnticipe)
					{
						pst_chauf->uc_ConsigneFilPiloteEnCours = l_uc_Consigne;
						pst_chauf->uc_Mode = uc_AUTO;
					}
				}
				else
				{	// Cas ou mode différent trouve -> Surveille l'arrivée sur la plage anticipée pour faire retomber ce mode
					if (pst_chauf->uc_ConsigneFilPiloteNonAnticipe != l_uc_Consigne)	// Permet de sortir de la fonction à la première valeur différente
																						// Même s'il y a eu modification des plages alors que le mode anticipé était en cours (ce qui explique le uc_ConsigneFilPiloteNonAnticipe)
					{
						pst_chauf->uc_ConsigneFilPiloteEnCours = l_uc_Consigne;
						pst_chauf->uc_Mode = uc_AUTO;
					}
				}
			}
		break;
			
		default:
			// ne devrait pas arrivé => valeur par défaut
			pst_chauf->uc_Mode = uc_AUTO;
			pst_chauf->uc_ConsigneFilPiloteEnCours = uc_CHAUFFAGE_CONFORT;
		break;

	}
	*uc_ModeDemande = (unsigned char) ((pst_chauf->uc_Mode << 4) + pst_chauf->uc_ConsigneFilPiloteEnCours);
	
	// Valeur calculée -> copie dans variable utilisée pour génération fil pilote (sous IT)
	l_uc_Consigne2 = pst_chauf->uc_ConsigneFilPiloteEnCours;

	// Applicatif délestage si actif
	l_uc_Consigne = pst_chauf->uc_DelestageActif;	// Recopie en local car peut changer en cours de traitement (autre tache)
	if(l_uc_Consigne != 0xFF && Tb_Echange[Delestage] != 0)	// N'appliquer la consigne de délestage que si DELESTAGE activé !
	{
		if(l_uc_Consigne2 != uc_CHAUFFAGE_OFF)	// N'appliquer la consigne que si différent de OFF
		{
			l_uc_Consigne2 = l_uc_Consigne;
		}
	}
	*uc_ConsignePourPilotage = l_uc_Consigne2;	// Passage par l_uc_Consigne2 car risque utilisation valeur sous IT pendant calcul...
}

// recherche à l'intérieur d'une plage planning
// programmation horaire sur 1 semaine
// 1 ordre pour 1 heure sur 7 jours : 6 modes possibles (4 bits) 
// soit 168 plages
// soit 84 octets 
// cette fonction est la seule à gérer la manière dont sont stockées les plages horaires
unsigned char uc_Plage(unsigned char *p_uc_planning, unsigned char uc_ind)
{
	unsigned char uc_i, uc_valeur;


	uc_i = uc_ind >> 1;
	if(uc_i >= uc_PLANNING_CHAUFFAGE_TAILLE)
	{
		return (0);			// sécurité : indice hors plage : sortie avec valeur par défaut
	}
	uc_valeur = p_uc_planning[uc_i];
	if((uc_ind & 0x01) == 1)
	{
		uc_valeur = uc_valeur >> 4;
	}
	uc_valeur &= 0x07;			// conserve 3 bits de poids faibles
	
	return (uc_valeur);
}

// Délestage - Plusieurs niveau de délestage
// Augmentation du niveau de délestage si dépassement reçu de la teleinfo
// Diminission du niveau de délestage si dépassement pas reçu de la teleinfo
// Si zone déjà consigne délestage -> passer au niveau suivant / précédent et ainsi de suite
// uc_DelestageNiveauEnCours :
// 0 : AUCUN
// 1 : ZJ en HG
// 2 : ZN en HG
// 3 : SDB1 en OFF
// 4 : SDB2 en OFF
// 5 : CUMULUS OFF

// RAZ délestage en cas de défaut - Si changement Delestage, sera pris automatiquement au prochain cycle de teleinfo
void vd_DelestageRAZ(void)
{
	uc_DelestageNiveauEnCours = 0;
	vd_DelestageUpdateConsigneZones();
}

// Appelé à chaque réception de ADPS -> augmente le niveau de délestage de 1 cran
void vd_DelestageAugmenterNiveau(void)
{
	unsigned char l_uc_FlagNewConsigne;	// Permet de passer au niveau suivant si le premier ne change rien à la consigne en cours
	
	if(Tb_Echange[Delestage] == 0 || st_EchangeStatus.uc_DefTeleinfo != 0)
	{
		uc_DelestageNiveauEnCours = 0;
	}
	else
	{
		l_uc_FlagNewConsigne = 0;
		if(uc_DelestageNiveauEnCours == 0 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 1;
			if(st_chauf_ZJ.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_HORS_GEL && st_chauf_ZJ.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 1 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 2;
			if(st_chauf_ZN.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_HORS_GEL && st_chauf_ZN.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 2 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 3;
			if(st_chauf_Zsdb1.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 3 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 4;
			if(st_chauf_Zsdb2.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 4 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 5;
		}
	}
	vd_DelestageUpdateConsigneZones();
}

// Appelé à chaque réception de OPTARIF et si pas de réception de ADPS depuis dernier OPTARIF -> diminue le niveau de délestage de 1 cran
void vd_DelestageDiminuerNiveau(void)
{
	unsigned char l_uc_FlagNewConsigne;	// Permet de passer au niveau precedent si le premier ne change rien à la consigne en cours
	
	if(Tb_Echange[Delestage] == 0 || st_EchangeStatus.uc_DefTeleinfo != 0)
	{
		uc_DelestageNiveauEnCours = 0;
	}
	else
	{
		l_uc_FlagNewConsigne = 0;
		if(uc_DelestageNiveauEnCours == 5 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 4;
			l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 4 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 3;
			if(st_chauf_Zsdb2.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 3 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 2;
			if(st_chauf_Zsdb1.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_OFF)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 2 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 1;
			if(st_chauf_ZN.uc_ConsigneFilPiloteEnCours != uc_CHAUFFAGE_HORS_GEL)	l_uc_FlagNewConsigne = 1;
		}
		if(uc_DelestageNiveauEnCours == 1 && l_uc_FlagNewConsigne == 0)
		{
			uc_DelestageNiveauEnCours = 0;
		}
	}
	vd_DelestageUpdateConsigneZones();
}

// Applique le délestage en cours aux différentes zones (uc_DelestageNiveauEnCours)
// 0 : AUCUN
// 1 : ZJ en HG
// 2 : ZN en HG
// 3 : SDB1 en OFF
// 4 : SDB2 en OFF
// 5 : CUMULUS OFF
//
// Consigne Délestage -> struct_chauffage.uc_DelestageActif;		// Delestage actif pour cette zone si != 0xFF -> contient la consigne à appliquer
																	// b0-b3 : consigne : 0 = OFF / 1 = CONFORT / 2 = ECO / 3 = ECO+ / 4 = ECO++ / 5 = HORS GEL
void vd_DelestageUpdateConsigneZones(void)
{
	switch(uc_DelestageNiveauEnCours)
	{
		case 1:
			st_chauf_ZJ.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_ZN.uc_DelestageActif = 0xFF;
			st_chauf_Zsdb1.uc_DelestageActif = 0xFF;
			st_chauf_Zsdb2.uc_DelestageActif = 0xFF;
			uc_DelestageCouperCumulus = 0;
		break;
		case 2:
			st_chauf_ZJ.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_ZN.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_Zsdb1.uc_DelestageActif = 0xFF;
			st_chauf_Zsdb2.uc_DelestageActif = 0xFF;
			uc_DelestageCouperCumulus = 0;
		break;
		case 3:
			st_chauf_ZJ.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_ZN.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_Zsdb1.uc_DelestageActif = uc_CHAUFFAGE_OFF;
			st_chauf_Zsdb2.uc_DelestageActif = 0xFF;
			uc_DelestageCouperCumulus = 0;
		break;
		case 4:
			st_chauf_ZJ.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_ZN.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_Zsdb1.uc_DelestageActif = uc_CHAUFFAGE_OFF;
			st_chauf_Zsdb2.uc_DelestageActif = uc_CHAUFFAGE_OFF;
			uc_DelestageCouperCumulus = 0;
		break;
		case 5:
			st_chauf_ZJ.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_ZN.uc_DelestageActif = uc_CHAUFFAGE_HORS_GEL;
			st_chauf_Zsdb1.uc_DelestageActif = uc_CHAUFFAGE_OFF;
			st_chauf_Zsdb2.uc_DelestageActif = uc_CHAUFFAGE_OFF;
			uc_DelestageCouperCumulus = 1;
		break;
		default:	// Délestage désactivé
			st_chauf_ZJ.uc_DelestageActif = 0xFF;
			st_chauf_ZN.uc_DelestageActif = 0xFF;
			st_chauf_Zsdb1.uc_DelestageActif = 0xFF;
			st_chauf_Zsdb2.uc_DelestageActif = 0xFF;
			uc_DelestageCouperCumulus = 0;
		break;
	}
	
	// Si changement niveau délestage -> recalcul consignes chauffage + affichage dans log
	if(uc_DelestageNiveauEnCours != uc_DelestageNiveauEnCoursPrecedent)
	{
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"DELESTAGE %d -> %d\n", uc_DelestageNiveauEnCoursPrecedent, uc_DelestageNiveauEnCours);
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"\tst_chauf_ZJ -> %d\n", st_chauf_ZJ.uc_DelestageActif);
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"\tst_chauf_ZN -> %d\n", st_chauf_ZN.uc_DelestageActif);
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"\tst_chauf_Zsdb1 -> %d\n", st_chauf_Zsdb1.uc_DelestageActif);
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"\tst_chauf_Zsdb2 -> %d\n", st_chauf_Zsdb2.uc_DelestageActif);
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"\tCouper CUMULUS -> %d\n", uc_DelestageCouperCumulus);

		uc_DelestageNiveauEnCoursPrecedent = uc_DelestageNiveauEnCours;
		uc_ModeChauffageModifieParEcranOuScenario = 1;
	}
}

void vd_Chauffage_Espion(void)
{
#ifdef DEBUG
	vd_Chauffage_EspionParZone(st_chauf_ZJ, &st_chauf_ZJ_Precedent, "ZJ\n");
	vd_Chauffage_EspionParZone(st_chauf_ZN, &st_chauf_ZN_Precedent, "ZN\n");
	vd_Chauffage_EspionParZone(st_chauf_Zsdb1, &st_chauf_Zsdb1_Precedent, "ZSB1\n");
	vd_Chauffage_EspionParZone(st_chauf_Zsdb2, &st_chauf_Zsdb2_Precedent, "ZSB2\n");
#endif
}

#ifdef DEBUG
void vd_Chauffage_EspionParZone(struct_chauffage st_EtatZone, pstruct_chauffage p_st_EtatZonePrecedent, const char *l_p_uc_Libelle)
{
	unsigned char l_uc_FlagChangement;

	
	l_uc_FlagChangement = 0;
	
	if(st_EtatZone.uc_Mode != p_st_EtatZonePrecedent->uc_Mode)
	{
		if(l_uc_FlagChangement == 0)	{vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,l_p_uc_Libelle);	l_uc_FlagChangement = 1;}
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"Mode : 0x%X -> 0x%X\n", p_st_EtatZonePrecedent->uc_Mode, st_EtatZone.uc_Mode);
		p_st_EtatZonePrecedent->uc_Mode = st_EtatZone.uc_Mode;
	}
	if(st_EtatZone.uc_ConsigneFilPiloteEnCours != p_st_EtatZonePrecedent->uc_ConsigneFilPiloteEnCours)
	{
		if(l_uc_FlagChangement == 0)	{vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,l_p_uc_Libelle);	l_uc_FlagChangement = 1;}
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"ConsigneFPEnCours : 0x%X -> 0x%X\n", p_st_EtatZonePrecedent->uc_ConsigneFilPiloteEnCours, st_EtatZone.uc_ConsigneFilPiloteEnCours);
		p_st_EtatZonePrecedent->uc_ConsigneFilPiloteEnCours = st_EtatZone.uc_ConsigneFilPiloteEnCours;
	}
	if(st_EtatZone.uc_DelestageActif != p_st_EtatZonePrecedent->uc_DelestageActif)
	{
		if(l_uc_FlagChangement == 0)	{vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,l_p_uc_Libelle);	l_uc_FlagChangement = 1;}
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"DelestageActif : 0x%X -> 0x%X\n", p_st_EtatZonePrecedent->uc_DelestageActif, st_EtatZone.uc_DelestageActif);
		p_st_EtatZonePrecedent->uc_DelestageActif = st_EtatZone.uc_DelestageActif;
	}
	if(st_EtatZone.uc_ConsigneFilPiloteNonAnticipe != p_st_EtatZonePrecedent->uc_ConsigneFilPiloteNonAnticipe)
	{
		if(l_uc_FlagChangement == 0)	{vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,l_p_uc_Libelle);	l_uc_FlagChangement = 1;}
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"ConsigneFPNonAnticipe : 0x%X -> 0x%X\n", p_st_EtatZonePrecedent->uc_ConsigneFilPiloteNonAnticipe, st_EtatZone.uc_ConsigneFilPiloteNonAnticipe);
		p_st_EtatZonePrecedent->uc_ConsigneFilPiloteNonAnticipe = st_EtatZone.uc_ConsigneFilPiloteNonAnticipe;
	}
	if(st_EtatZone.uc_HeurePassageAnticipe != p_st_EtatZonePrecedent->uc_HeurePassageAnticipe)
	{
		if(l_uc_FlagChangement == 0)	{vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,l_p_uc_Libelle);	l_uc_FlagChangement = 1;}
		vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"HeurePassageAnticipe : 0x%X -> 0x%X\n", p_st_EtatZonePrecedent->uc_HeurePassageAnticipe, st_EtatZone.uc_HeurePassageAnticipe);
		p_st_EtatZonePrecedent->uc_HeurePassageAnticipe = st_EtatZone.uc_HeurePassageAnticipe;
	}
	if(l_uc_FlagChangement != 0)	vd_EspionRS_Printf(uc_ESPION_CHAUFFAGE_ACTIVITE,"");
}
#endif

// Calcule l'etat du chauffage pour la trame d'etat (ecran)
void vd_CalculerEtatConsigne(struct_chauffage st_chauf, unsigned char *p_uc_buffer)
{
	switch(st_chauf.uc_ConsigneFilPiloteEnCours) 	//xxx mettre octet etat chauffage pour affichage
	{
		case uc_CHAUFFAGE_CONFORT:		// mode par défaut
			(*p_uc_buffer) = 0x00;						// couleur LO
			(*(p_uc_buffer+1)) = 0xF8;					// couleur HI
		break;
		case uc_CHAUFFAGE_ECO:
			(*p_uc_buffer) = 0x20;						// couleur LO
			(*(p_uc_buffer+1)) = 0xFA;					// couleur HI
		break;
		case uc_CHAUFFAGE_ECO_PLUS:
			(*p_uc_buffer) = 0xE0;						// couleur LO
			(*(p_uc_buffer+1)) = 0xFF;					// couleur HI
		break;
		case uc_CHAUFFAGE_ECO_PLUS_PLUS:
			(*p_uc_buffer) = 0xFF;						// couleur LO
			(*(p_uc_buffer+1)) = 0x07;					// couleur HI
		break;
		case uc_CHAUFFAGE_HORS_GEL:
			(*p_uc_buffer) = 0x10;						// couleur LO
			(*(p_uc_buffer+1)) = 0x80;					// couleur HI
		break;
		case uc_CHAUFFAGE_OFF:
		default :
			(*p_uc_buffer) = 0xFF;						// couleur LO
			(*(p_uc_buffer+1)) = 0xFF;					// couleur HI
		break;
	}
	
	switch(st_chauf.uc_Mode)	//xxx mettre octet etat chauffage pour affichage pour eviter inchoerence entre mode et consigne -> mettre a jour a la fin des calculs
	{
		case uc_AUTO:
			(*(p_uc_buffer+2)) = 1;	// mode
		break;
		case uc_FORCE:
			(*(p_uc_buffer+2)) = 3;	// mode
		break;
		case uc_ANTICIPE:
			(*(p_uc_buffer+2)) = 2;	// mode
		break;
		default :
			(*(p_uc_buffer+2)) = 0;	// mode
		break;
	}
}
