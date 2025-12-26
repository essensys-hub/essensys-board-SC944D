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

#define extern
#include "arrosage.h"
#undef extern


// Retourne l'état à mettre sur la sortie EV arrosage (0 ou 1)
// Peut etre appelé "en permanence", cadencement interne basé sur les secondes de la RTC
unsigned char uc_Arrosage(void)
{
	static unsigned short s_us_CompteurArrosageSec = 0;
	static unsigned char s_uc_old_seconde = 0;
	unsigned char l_uc_PiloterArrosage;
	unsigned short l_us_indice;
	unsigned char l_uc_Masque;

	
	l_uc_PiloterArrosage = 0;
	switch(Tb_Echange[Arrose_Mode])
	{
		case 0:		// OFF
			s_us_CompteurArrosageSec = 0;
		break;
		case 255:	// mode automatique
			s_us_CompteurArrosageSec = 0;
			
			// xxx niveau du pluviomètre à confirmer
			if(Tb_Echange[Arrose_Detect] == 0 ||									// détecteur inactif OU
			   lwgpio_get_value(&IO_DIN_Detection_Pluie) == LWGPIO_VALUE_LOW)		// pluviomètre indique absence de pluie
			{
	           	//Arrose_Auto,			// 1 ordre pour 30 minutes sur 7 jours : 2 modes possibles (1 bit), soit 42 octets
				l_us_indice = (uc_JourSemaine * 48) + (st_DateHeure.HOUR * 2);	// Donne le numéro de bit à utiliser dans le tableau
				if(st_DateHeure.MINUTE >= 30)	l_us_indice++;	// 2ème plage de 30mn
				l_uc_Masque = uc_CreeMasque(l_us_indice & 0x07);	// 1 bit par plage, soit 8 plages dans 1 octet 
				l_us_indice = l_us_indice >> 3;	// 1 bit par plage, soit 8 plages dans 1 octet => divise par 8
				if(l_us_indice >= uc_PLANNING_ARROSAGE_TAILLE)
				{
					DETECTION_ERREUR_TACHE_PRINCIPALE_ARROSAGE_CALCUL;	// sécurité : indice hors plage
				}
				else
				{
					if((Tb_Echange[Arrose_Auto+l_us_indice] & l_uc_Masque) != 0)			l_uc_PiloterArrosage = 1;
				}
			}
		break;
		default:	// 1 à 254 mode forcé : mode = temps d'arrosage en mn
			// le décomptage se fait sans utiliser les timers : on n'a besoin ni d'être rapide, ni d'être précis
			// et ça permet de prendre en compte les changements de temps à la volée
			l_uc_PiloterArrosage = 1;
			if(s_uc_old_seconde != st_DateHeure.SECOND)
			{
				s_uc_old_seconde = st_DateHeure.SECOND;
				s_us_CompteurArrosageSec++;
			}
			if(s_us_CompteurArrosageSec >= ((unsigned short)Tb_Echange[Arrose_Mode] * (unsigned short)60))
			{	// durée d'arrosage atteinte
				s_us_CompteurArrosageSec = 0;
				Tb_Echange[Arrose_Mode] = 0;	// OFF
			}
	}
	return(l_uc_PiloterArrosage);
}

unsigned char uc_CreeMasque(unsigned short us_Valeur)
{
	switch(us_Valeur) 
	{
		case 0:
			return (0x01);
		case 1:
			return (0x02);
		case 2:
			return (0x04);
		case 3:
			return (0x08);
		case 4:
			return (0x10);
		case 5:
			return (0x20);
		case 6:
			return (0x40);
		case 7:
			return (0x80);
		default :		// erreur
			return (0x00);
	}
}
