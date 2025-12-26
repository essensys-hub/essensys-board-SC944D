//*************************************************************
// *  TeleInformation : 
// *  gestion des informations envoyées par un compteur ERDF
// *  gestion du témoin lumineux
// *
// *	gestion des compteurs BLEU, JAUNE, EMERAUDE
// *	selon doc "Sorties de télé-information client des appareils de comptage électroniques utilisés par ERDF"
// *	fichier "erdf_noi_cpt_02e.pdf" V4 du 01/07/2010   
// *************************************************************/

#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx

#include <string.h>
#include <stdlib.h>

#include "EspionRS.h"
#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "global.h"
#include "chauffage.h"

#define extern
#include "TeleInfo.h"
#undef extern


#define	us_TAILLE_BUFFER_RX_TELEINFO		500

unsigned char uc_TeleinfoBufferRx[us_TAILLE_BUFFER_RX_TELEINFO];
unsigned short us_TeleinfoNbOctetsRecus;

unsigned char uc_FlagADPSTrouve;
unsigned char uc_EtatADPS;	// 0 : non reçu - 1 : reçu dans le dernier cycle

//unsigned short us_SimulTeleinfoCompteur = 0;
//const unsigned char uc_SimulTeleinfo[] =
//	{0x2,
//	 0xA,'A','D','C','O',0x20,'0','3','1','2','2','8','2','3','2','0','2','4',0x20,'4',0xD,
//	 0xA,'O','P','T','A','R','I','F',0x20,'H','C','.','.',0x20,'<',0xD,
//	 0xA,'I','S','O','U','S','C',0x20,'6','0',0x20,'<',0xD,
//	 0xA,'H','C','H','C',0x20,'0','0','3','3','0','9','9','6','6',0x20,'*',0xD,
//	 0xA,'H','C','H','P',0x20,'0','0','7','7','7','2','1','1','0',0x20,',',0xD,
//	 0xA,'P','T','E','C',0x20,'H','P','.','.',0x20,0x20,0xD,
//	 0xA,'I','I','N','S','T',0x20,'0','1','3',0x20,'[',0xD,
//	 0xA,'I','M','A','X',0x20,'0','4','5',0x20,'H',0xD,
//	 0xA,'P','A','P','P',0x20,'0','2','9','8','0',0x20,'4',0xD,
//	 0xA,'H','H','P','H','C',0x20,'D',0x20,'/',0xD,
//	 0xA,'M','O','T','D','E','T','A','T',0x20,'0','0','0','0','0','0',0x20,'B',0xD,
//	 0x3};


// Tache teleinfo
void TeleInfo_task(uint_32 dummy)
{
	MQX_FILE_PTR l_fd;		// RS LINKY
	boolean l_b_disable_rx = FALSE;
	signed long l_l_param, l_l_result;
	
	
	vd_EspionRS_Printf(uc_ESPION_TELEINFO,"INIT\n");
	uc_EspionTacheTeleinfoEtat = 1;
	
	// Ouverture RS
	l_fd = fopen(TELEINFO_DEVICE_INTERRUPT, ( char const * )(IO_SERIAL_NON_BLOCKING));
	uc_EspionTacheTeleinfoEtat = 2;
	if(l_fd == NULL) 
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error opening uart driver!\n");
		DETECTION_ERREUR_TACHE_TELEINFO_RS_OPEN;
		_task_block();
	}

	uc_EspionTacheTeleinfoEtat = 3;
	l_l_param = TELEINFO_BAUD_RATE;
	if(ioctl(l_fd, IO_IOCTL_SERIAL_SET_BAUD, &l_l_param) != MQX_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error baud rate modification!\n");
		DETECTION_ERREUR_TACHE_TELEINFO_RS_VITESSE;
		_task_block();
	}

	uc_EspionTacheTeleinfoEtat = 4;
	l_l_param = IO_SERIAL_PARITY_EVEN;
	if(ioctl(l_fd, IO_IOCTL_SERIAL_SET_PARITY, &l_l_param) != MQX_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error parity configuration!\n");
		DETECTION_ERREUR_TACHE_TELEINFO_RS_PARITE;
		_task_block();
	}

	uc_EspionTacheTeleinfoEtat = 5;
	l_l_param = 7;
	if(ioctl(l_fd, IO_IOCTL_SERIAL_SET_DATA_BITS, &l_l_param) != MQX_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error bits number!\n");
		DETECTION_ERREUR_TACHE_TELEINFO_RS_BITS;
		_task_block();
	}

	uc_EspionTacheTeleinfoEtat = 6;
	l_b_disable_rx = FALSE;
	/*if(ioctl(l_fd, IO_IOCTL_SERIAL_DISABLE_RX, &l_b_disable_rx) != MQX_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error RX enable!\n");
		DETECTION_ERREUR_TACHE_TELEINFO_RX_ENABLE;
		_task_block();
	}*///xxx tester retour
	ioctl(l_fd, IO_IOCTL_SERIAL_DISABLE_RX, &l_b_disable_rx);

	us_TeleinfoNbOctetsRecus = 0;	// init pour réception
	uc_FlagADPSTrouve = 0;
	uc_EtatADPS = 0;

	uc_EspionTacheTeleinfoEtat = 7;
	vd_StartTimerTeleinfo();
	us_TeleinfoTimeOUT = us_TELEINFO_TIME_OUT;	// Forcer init buffer tant que un caractere n'est pas recu

	uc_EspionTacheTeleinfoEtat = 8;
	// tache de fond permanente
	vd_EspionRS_Printf(uc_ESPION_TELEINFO,"START\n");
	
	
	for (;;)
	{
		ul_EspionTacheTeleinfoCompteurActivite++;
		
		uc_EspionTacheTeleinfoEtat = 50;
		vd_RSTeleinfoRead(l_fd);
		uc_EspionTacheTeleinfoEtat = 51;
		vd_AnalyserOctetsTeleinfoRecus();
		uc_EspionTacheTeleinfoEtat = 52;
		vd_GestionDataTeleinfo();
		uc_EspionTacheTeleinfoEtat = 53;
		_sched_yield();		// passe la main à la tache suivante
		uc_EspionTacheTeleinfoEtat = 54;
	}
	
	vd_EspionRS_Printf(uc_ESPION_TELEINFO,"STOP\n");
	uc_EspionTacheTeleinfoEtat = 100;
	
	/* Close the driver */
	l_l_result = fclose(l_fd);
	if(l_l_result)
	{
		vd_EspionRS_Printf(uc_ESPION_TELEINFO,"Error close rs : %d\n", l_l_result);
	}

	vd_EspionRS_Printf(uc_ESPION_TELEINFO,"END\n");
	uc_EspionTacheTeleinfoEtat = 101;
	_task_block();
}

#define uc_ETIQUETTE_DEBUT_TRAME		0x02
#define uc_ETIQUETTE_FIN_TRAME			0x03
#define uc_ETIQUETTE_PAUSE_EMISSION		0x04

// Lit les octets reçus sur la RS de la téléinfo
void vd_RSTeleinfoRead(MQX_FILE_PTR fd)
{
	unsigned char l_uc_Caractere;
	unsigned char l_uc_Reception;
	
	
	// Lecture du buffer de reception MQX (fonction non bloquante)
	l_uc_Reception = 0;
	while(read(fd, &l_uc_Caractere, 1) != 0 && us_TeleinfoNbOctetsRecus < us_TAILLE_BUFFER_RX_TELEINFO)
	//if(us_SimulTeleinfoCompteur < sizeof(uc_SimulTeleinfo) && us_TeleinfoNbOctetsRecus < us_TAILLE_BUFFER_RX_TELEINFO)
	{
		//l_uc_Caractere = uc_SimulTeleinfo[us_SimulTeleinfoCompteur];
		//us_SimulTeleinfoCompteur++;
		//_time_delay(5);	//xxx pour debug
		l_uc_Reception = 1;
		ul_EspionTacheTeleinfoCompteurRx++;
		if(l_uc_Caractere <= 32)	vd_EspionRS_Printf(uc_ESPION_TELEINFO_RX,"0x%02X\n", l_uc_Caractere);
		else						vd_EspionRS_Printf(uc_ESPION_TELEINFO_RX,"%c\n", l_uc_Caractere);

		// Suppression caractères début trame / fin trame / pause émission
		if(l_uc_Caractere != uc_ETIQUETTE_DEBUT_TRAME &&
		   l_uc_Caractere != uc_ETIQUETTE_FIN_TRAME &&
		   l_uc_Caractere != uc_ETIQUETTE_PAUSE_EMISSION)
		{
			uc_TeleinfoBufferRx[us_TeleinfoNbOctetsRecus] = l_uc_Caractere;
			us_TeleinfoNbOctetsRecus++;
		}
		if(l_uc_Caractere == uc_ETIQUETTE_PAUSE_EMISSION)
		{
			// Pause émission envoyée par compteur -> supprimer ce qui a été déjà reçu car reprise émission par début trame
			us_TeleinfoNbOctetsRecus = 0;
		}
	}
	if(us_TeleinfoNbOctetsRecus >= us_TAILLE_BUFFER_RX_TELEINFO)
	{
		DETECTION_ERREUR_TACHE_TELEINFO_SATURATION_RX_ECRAN;
		us_TeleinfoNbOctetsRecus = 0;
	}
	
	// Gestion time out
	if(l_uc_Reception != 0)
	{
		us_TeleinfoTimeOUT = 0;
	}
	if(us_TeleinfoTimeOUT >= us_TELEINFO_TIME_OUT)
	{
		if(us_TeleinfoNbOctetsRecus != 0)	ul_EspionTacheTeleinfoNbRAZBufferReceptionSurTimeOUT++;
		us_TeleinfoNbOctetsRecus = 0;	// Time out -> RAZ buffer réception
	}
}

// Supprime du tableau l'octet le plus ancien
void vd_DecalerTableauReception(unsigned short us_NbOctets)
{
	unsigned short l_us_Compteur;
	
	for(l_us_Compteur = us_NbOctets;l_us_Compteur < us_TAILLE_BUFFER_RX_TELEINFO; l_us_Compteur++)
	{
		uc_TeleinfoBufferRx[l_us_Compteur-us_NbOctets] = uc_TeleinfoBufferRx[l_us_Compteur];
	}
	if(us_TeleinfoNbOctetsRecus > us_NbOctets)	us_TeleinfoNbOctetsRecus-=us_NbOctets;
	else										us_TeleinfoNbOctetsRecus = 0;
}

// Calcule la cheksum d'un buffer
// La "checksum" est calculée sur l'ensemble des caractères allant du début du champ étiquette à la fin du champ donnée, caractère SP inclus. 
// On fait tout d'abord la somme des codes ASCII de tous ces caractères. 
// Pour éviter d'introduire des fonctions ASCII (00 à 1F en hexadécimal), on ne conserve que les six bits de poids faible du résultat obtenu. 
// Enfin, on ajoute 20 en hexadécimal. Le résultat sera donc toujours un caractère ASCII imprimable allant de 20 à 5F en hexadécimal.
unsigned char uc_fct_Calcul_CheckSum_Tele(unsigned char *puc_buffer, uint_16 us_Nb_Octets)
{
	uint_16	l_us_i;
	unsigned char l_uc_CheckSum;

	
	l_uc_CheckSum = 0;
	for(l_us_i=0; l_us_i<us_Nb_Octets; l_us_i++)
	{
		l_uc_CheckSum += puc_buffer[l_us_i];
	}
	l_uc_CheckSum &= 0x3F;
	l_uc_CheckSum += 0x20;
	return(l_uc_CheckSum);
}

#define uc_ETIQUETTE_GROUPE_INFORMATION_DEBUT			0x0A
#define uc_ETIQUETTE_GROUPE_INFORMATION_FIN				0x0D
#define uc_ETIQUETTE_GROUPE_INFORMATION_SEPARATEUR		0x20

// Analyse des octets recus
void vd_AnalyserOctetsTeleinfoRecus(void)
{
	unsigned short l_us_Compteur;
	unsigned char l_uc_FlagTrouve;
	unsigned char l_uc_Checksum;
	unsigned short l_us_AdresseFinGroupeInformation;
	unsigned char uc_FlagBoucle;
	
	
	do
	{
		uc_FlagBoucle = 0;

		// Recherche début groupe information
		l_uc_FlagTrouve = 0;
		while(us_TeleinfoNbOctetsRecus != 0 && l_uc_FlagTrouve == 0)
		{
			if(uc_TeleinfoBufferRx[0] != uc_ETIQUETTE_GROUPE_INFORMATION_DEBUT)		vd_DecalerTableauReception(1);
			else																	l_uc_FlagTrouve = 1;
		}
		
		// Debut groupe information trouvee
		if(l_uc_FlagTrouve)
		{
			ul_EspionTacheTeleinfoCompteurGroupeInfoDebut++;
			
			// Recherche fin groupe
			l_uc_FlagTrouve = 0;
			for(l_us_Compteur = 0; l_us_Compteur < us_TeleinfoNbOctetsRecus && l_uc_FlagTrouve == 0; l_us_Compteur++)
			{
				if(uc_TeleinfoBufferRx[l_us_Compteur] == uc_ETIQUETTE_GROUPE_INFORMATION_FIN)	l_uc_FlagTrouve = 1;
			}
			
			// Fin groupe information trouvee
			if(l_uc_FlagTrouve != 0)
			{
				ul_EspionTacheTeleinfoCompteurGroupeInfoFin++;
				l_us_AdresseFinGroupeInformation = l_us_Compteur;
				
				// Verification checksum
				l_uc_Checksum = uc_fct_Calcul_CheckSum_Tele(uc_TeleinfoBufferRx+1, l_us_Compteur-4);
				if(l_uc_Checksum != uc_TeleinfoBufferRx[l_us_Compteur-2])
				{
						ul_EspionTacheTeleinfoCompteurGroupeInfoPBChecksum++;
						vd_EspionRS_Printf(uc_ESPION_TELEINFO, "PB CRC : reçu -> 0x%02X - calculé -> 0x%02X\n", uc_TeleinfoBufferRx[l_us_Compteur-2], l_uc_Checksum);
				}
				else
				{
					// Checksum OK -> contrôle présence des deux séparateurs
					if(uc_TeleinfoBufferRx[l_us_Compteur-3] != uc_ETIQUETTE_GROUPE_INFORMATION_SEPARATEUR)
					{
						ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurCRC++;
						vd_EspionRS_Printf(uc_ESPION_TELEINFO, "PB SEPARATEUR CRC NON TROUVE\n");
					}
					else
					{
						// Remplacement séparateur CRC par fin de chaine
						uc_TeleinfoBufferRx[l_us_Compteur-3] = 0;
						
						// Recherche séparateur données
						l_uc_FlagTrouve = 0;
						for(l_us_Compteur = 0; l_us_Compteur < us_TeleinfoNbOctetsRecus && l_uc_FlagTrouve == 0; l_us_Compteur++)
						{
							if(uc_TeleinfoBufferRx[l_us_Compteur] == uc_ETIQUETTE_GROUPE_INFORMATION_SEPARATEUR)	l_uc_FlagTrouve = 1;
						}
						if(l_uc_FlagTrouve == 0)
						{
							ul_EspionTacheTeleinfoCompteurGroupeInfoPBSeparateurData++;
							vd_EspionRS_Printf(uc_ESPION_TELEINFO, "PB SEPARATEUR DATA NON TROUVE\n");
						}
						else
						{
							// Remplacement séparateur DATA par fin de chaine
							uc_TeleinfoBufferRx[l_us_Compteur-1] = 0;
							
							// Les deux séparateurs trouvés -> analyse étiquette et récupération data
							vd_TeleInfExtractionEtiquetteDonnee((char *)&uc_TeleinfoBufferRx[1], (char *)&uc_TeleinfoBufferRx[l_us_Compteur]);
						}
					}
				}
				
				// Fin groupe information trouvee -> suppression informations dans tous les cas (traité ou non traité suite pb data)
				vd_DecalerTableauReception(l_us_AdresseFinGroupeInformation);
				
				// -> Traitement trame suivante
				// Dasn les autres cas, debut groupe info ou fin groupe info pas trouvé -> trame incomplète
				uc_FlagBoucle = 1;
			}
		}
	}
	while(uc_FlagBoucle != 0);
}

// Reprise (IR) le 22/01/2013 : selon CdC : gestion du compteur bleu monophasé multitarif évolution ICC
// Extraction des données en fonction de l'étiquette
// Détection fin etiquette et donnee par caractère 0
void vd_TeleInfExtractionEtiquetteDonnee(char_ptr p_Etiq, char_ptr p_Donnee)
{
	if(strcmp("OPTARIF", p_Etiq) == 0)
	{
		uc_TeleinfoOptionTarifaire = uc_OPT_TARIF_NON_RENSEIGNE;	
		if(strncmp("BASE", p_Donnee, 4) == 0)		uc_TeleinfoOptionTarifaire	= uc_OPT_TARIF_BASE;
		else if(strncmp("HC..", p_Donnee, 2) == 0)	uc_TeleinfoOptionTarifaire	= uc_OPT_TARIF_HC;
		else if(strncmp("EJP.", p_Donnee, 3) == 0)	uc_TeleinfoOptionTarifaire	= uc_OPT_TARIF_EJP;
		else if(strncmp("BBR", p_Donnee, 3) == 0)	uc_TeleinfoOptionTarifaire	= uc_OPT_TARIF_BBR;
		
		// Si data recue -> RAZ compteur time out
		if(uc_TeleinfoOptionTarifaire != uc_OPT_TARIF_NON_RENSEIGNE)	us_TeleinfoTimeOUTOptionTarifaire = 0;
		
		if(uc_FlagADPSTrouve != 0)
		{
			uc_FlagADPSTrouve = 0;
		}
		else
		{
			uc_EtatADPS = 0;
			vd_DelestageDiminuerNiveau();
		}
	}

	else if(strcmp("PTEC", p_Etiq) == 0)
	{
		uc_TeleinfoPeriodeTarifaire = uc_TARIF_NON_RENSEIGNE;
		if(strcmp("TH..", p_Donnee) == 0)			uc_TeleinfoPeriodeTarifaire = uc_TARIF_TH;
		else if(strcmp("HC..", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HC;
		else if(strcmp("HP..", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HP;
		else if(strcmp("HN..", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HN;
		else if(strcmp("PM..", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_PM;
		else if(strcmp("HCJB", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HCJB;
		else if(strcmp("HCJW", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HCJW;
		else if(strcmp("HCJR", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HCJR;
		else if(strcmp("HPJB", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HPJB;
		else if(strcmp("HPJW", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HPJW;
		else if(strcmp("HPJR", p_Donnee) == 0)		uc_TeleinfoPeriodeTarifaire = uc_TARIF_HPJR;
		
		// Si data recue -> RAZ compteur time out
		if(uc_TeleinfoPeriodeTarifaire != uc_TARIF_NON_RENSEIGNE)		us_TeleinfoTimeOUTPeriodeTarifaire = 0;
	}
	
	else if(strcmp("ADPS", p_Etiq) == 0)
	{
		uc_FlagADPSTrouve = 1;
		uc_EtatADPS = 1;
		vd_DelestageAugmenterNiveau();
	}

	// Puissance apparente
	else if(strcmp("PAPP", p_Etiq) == 0)
	{
		ul_TeleinfoPuissanceApp = atol(p_Donnee);
		us_TeleinfoTimeOUTPuissanceApp = 0;
	}
	
	// HCHC
	else if(strcmp("HCHC", p_Etiq) == 0)
	{
		ul_TeleinfoHCHC = atol(p_Donnee);
		us_TeleinfoTimeOUTHCHC = 0;
	}

	// HCHP
	else if(strcmp("HCHP", p_Etiq) == 0)
	{
		ul_TeleinfoHCHP = atol(p_Donnee);
		us_TeleinfoTimeOUTHCHP = 0;
	}
}

void vd_StartTimerTeleinfo(void)
{
	static _timer_id s_TimerID = TIMER_NULL_ID;
	MQX_TICK_STRUCT l_ticks;

	_time_init_ticks(&l_ticks, 0);
	_time_add_msec_to_ticks(&l_ticks, us_TIMER_TELEINFO_ms);

	// Creation timer
	s_TimerID = _timer_start_periodic_every_ticks(vd_TeleinfoTimer, 0, TIMER_ELAPSED_TIME_MODE, &l_ticks);	
	if(s_TimerID == TIMER_NULL_ID)
	{
		DETECTION_ERREUR_TACHE_TELEINFO_PB_TIMER_TIME_OUT_INIT;
	}
}

// Appelé par timer - !!! 100 ms !!! Gestion time out TELEINFO + refresh LED
void vd_TeleinfoTimer(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr)
{
	ul_EspionTacheTeleinfoNbDeclenchementTimeOUT++;
	
	if(us_TeleinfoTimeOUT < us_TELEINFO_TIME_OUT)									us_TeleinfoTimeOUT++;
	if(us_TeleinfoTimeOUTOptionTarifaire < us_TELEINFO_OPTION_TARIFAIRE_TIME_OUT)	us_TeleinfoTimeOUTOptionTarifaire++;
	if(us_TeleinfoTimeOUTPeriodeTarifaire < us_TELEINFO_PERIODE_TARIFAIRE_TIME_OUT)	us_TeleinfoTimeOUTPeriodeTarifaire++;
	if(us_TeleinfoTimeOUTPuissanceApp < us_TELEINFO_PUISSANCE_APP_TIME_OUT)			us_TeleinfoTimeOUTPuissanceApp++;
	if(us_TeleinfoTimeOUTHCHC < us_TELEINFO_HCHC_APP_TIME_OUT)						us_TeleinfoTimeOUTHCHC++;
	if(us_TeleinfoTimeOUTHCHP < us_TELEINFO_HCHP_APP_TIME_OUT)						us_TeleinfoTimeOUTHCHP++;

	// Gestion LED TELEINFORMATION
	// Fixe : TELEINFO OK
	// Clignotement : PB TELEINFO (1 HZ)
	if(st_EchangeStatus.uc_Secouru == 0 || Tb_Echange[Mode_Test] != 0)
	{
		if(us_TeleinfoCadencementLed < us_CANDENCEMENT_LED_TELEINFO)	us_TeleinfoCadencementLed++;
		else
		{
			us_TeleinfoCadencementLed = 0;
	
			if(st_EchangeStatus.uc_DefTeleinfo == 0)
			{
				lwgpio_set_value(&IO_DOUT_TeleInfo_Led, LWGPIO_VALUE_HIGH);
			}
			else
			{
				lwgpio_toggle_value(&IO_DOUT_TeleInfo_Led);
			}
		}
	}
	else
	{
		lwgpio_set_value(&IO_DOUT_TeleInfo_Led, LWGPIO_VALUE_LOW);
	}
}

// Appelé dans la boucle principale TELEINFO
// RAZ informations non rafraichies par TELEINFO
// Recopie informations recues dans table d'échange
void vd_GestionDataTeleinfo(void)
{
	unsigned long l_ul_Difference;
	unsigned long l_ul_Repartition;
	unsigned long l_ul_Cumul;
	
	
	if(us_TeleinfoTimeOUTOptionTarifaire >= us_TELEINFO_OPTION_TARIFAIRE_TIME_OUT)		uc_TeleinfoOptionTarifaire = uc_OPT_TARIF_TIME_OUT;
	Tb_Echange[TeleInf_OPTARIF] = uc_TeleinfoOptionTarifaire;	
	
	if(us_TeleinfoTimeOUTPeriodeTarifaire >= us_TELEINFO_PERIODE_TARIFAIRE_TIME_OUT)	uc_TeleinfoPeriodeTarifaire = uc_TARIF_TIME_OUT;
	Tb_Echange[TeleInf_PTEC] = uc_TeleinfoPeriodeTarifaire;
	
	switch(uc_TeleinfoPeriodeTarifaire)
	{
		// Heures creuses
		case uc_TARIF_TH:	//XXX HC ???? SE METTRE EN HC SI HC RECU OU HP PAS RECU
		case uc_TARIF_HC:
		case uc_TARIF_HN:
		case uc_TARIF_HCJB:
		case uc_TARIF_HCJW:
		case uc_TARIF_HCJR:
			st_EchangeStatus.uc_HeuresCreuses = 1;
		break;
		
		// Heures pleines -> autres cas
		default:
			st_EchangeStatus.uc_HeuresCreuses = 0;
		break;
	}

	// Surconsommation -> si uc_DelestageNiveauEnCours != 0
	if(uc_DelestageNiveauEnCours == 0)		st_EchangeStatus.uc_SurConsommation = 0;
	else									st_EchangeStatus.uc_SurConsommation = 1;
	
	// ADPS : à 1 si ADPS reçu sur dernier cycle, sinon 0
	Tb_Echange[TeleInf_ADPS] = uc_EtatADPS;
	
	if(us_TeleinfoTimeOUTPuissanceApp >= us_TELEINFO_PUISSANCE_APP_TIME_OUT)
	{
		ul_TeleinfoPuissanceApp = 0;
		Tb_Echange[TeleInf_PAPP_LSB] = uc_PUISSANCE_APPARENTE_TIME_OUT;	//xxx mutex
		Tb_Echange[TeleInf_PAPP_MSB] = uc_PUISSANCE_APPARENTE_TIME_OUT;
	}
	else
	{
		if(ul_TeleinfoPuissanceApp > 60000)	ul_TeleinfoPuissanceApp = 60000;
		Tb_Echange[TeleInf_PAPP_LSB] = (uint_8)ul_TeleinfoPuissanceApp; //xxx mutex
		Tb_Echange[TeleInf_PAPP_MSB] = (uint_8)(ul_TeleinfoPuissanceApp >> 8);
	}
	
	if(us_TeleinfoTimeOUT >= us_TELEINFO_TIME_OUT ||
	   us_TeleinfoTimeOUTOptionTarifaire >= us_TELEINFO_OPTION_TARIFAIRE_TIME_OUT ||
	   us_TeleinfoTimeOUTPeriodeTarifaire >= us_TELEINFO_PERIODE_TARIFAIRE_TIME_OUT ||
	   //us_TeleinfoTimeOUTDepassementPmax >= us_TELEINFO_DEPASSEMENT_P_MAX_TIME_OUT || Non car info envoyée que si dépassement Pmax
	   us_TeleinfoTimeOUTPuissanceApp >= us_TELEINFO_PUISSANCE_APP_TIME_OUT ||
	   us_TeleinfoTimeOUTHCHC >= us_TELEINFO_HCHC_APP_TIME_OUT ||
	   us_TeleinfoTimeOUTHCHP >= us_TELEINFO_HCHP_APP_TIME_OUT)
	{
		st_EchangeStatus.uc_DefTeleinfo = 1;
		uc_FlagADPSTrouve = 0;
		uc_EtatADPS = 0;
		vd_DelestageRAZ();
	}
	else
	{
		st_EchangeStatus.uc_DefTeleinfo = 0;
	}
	
	if(uc_UpdateHCHC_HCHP != 0)
	{
		uc_UpdateHCHC_HCHP = 0;

		if(us_TeleinfoTimeOUTHCHC >= us_TELEINFO_HCHC_APP_TIME_OUT)
		{
			ul_TeleinfoHCHC = 0;
			Tb_Echange[TeleInf_HC_Global_LSB] = uc_HCHC_HCHP_TIME_OUT;	//xxx mutex
			Tb_Echange[TeleInf_HC_Global_MSB] = uc_HCHC_HCHP_TIME_OUT;
			Tb_Echange[TeleInf_HC_Chauffage_LSB] = 0;
			Tb_Echange[TeleInf_HC_Chauffage_MSB] = 0;
			Tb_Echange[TeleInf_HC_Refroid_LSB] = 0;
			Tb_Echange[TeleInf_HC_Refroid_MSB] = 0;
			Tb_Echange[TeleInf_HC_EauChaude_LSB] = 0;
			Tb_Echange[TeleInf_HC_EauChaude_MSB] = 0;
			Tb_Echange[TeleInf_HC_Prises_LSB] = 0;
			Tb_Echange[TeleInf_HC_Prises_MSB] = 0;
			Tb_Echange[TeleInf_HC_Autres_LSB] = 0;
			Tb_Echange[TeleInf_HC_Autres_MSB] = 0;
		}
		else
		{
			l_ul_Difference = ul_TeleinfoHCHC - ul_TeleinfoHCHCPrecedent;
			if(ul_TeleinfoHCHCPrecedent == 0)	l_ul_Difference = 0;	// 1er passage 
			ul_TeleinfoHCHCPrecedent = ul_TeleinfoHCHC;
			
			l_ul_Repartition = l_ul_Difference;
			if(l_ul_Repartition > 60000)	l_ul_Repartition = 60000;
			Tb_Echange[TeleInf_HC_Global_LSB] = (uint_8)l_ul_Repartition; //xxx mutex
			Tb_Echange[TeleInf_HC_Global_MSB] = (uint_8)(l_ul_Repartition >> 8);

			l_ul_Cumul = 0;
			// Calcul des 4 premiers avec arrondi
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Chauffage, TeleInf_HC_Chauffage_LSB, TeleInf_HC_Chauffage_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Refroid, TeleInf_HC_Refroid_LSB, TeleInf_HC_Refroid_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_EauChaude, TeleInf_HC_EauChaude_LSB, TeleInf_HC_EauChaude_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Prises, TeleInf_HC_Prises_LSB, TeleInf_HC_Prises_MSB);
			
			// Calcul du dernier : reste total - 4 premiers
			l_ul_Repartition = l_ul_Difference - l_ul_Cumul;
			if(l_ul_Repartition > 60000)	l_ul_Repartition = 60000;
			Tb_Echange[TeleInf_HC_Autres_LSB] = (uint_8)l_ul_Repartition; //xxx mutex
			Tb_Echange[TeleInf_HC_Autres_MSB] = (uint_8)(l_ul_Repartition >> 8);
		}

		if(us_TeleinfoTimeOUTHCHP >= us_TELEINFO_HCHP_APP_TIME_OUT)
		{
			ul_TeleinfoHCHP = 0;
			Tb_Echange[TeleInf_HPB_Global_LSB] = uc_HCHC_HCHP_TIME_OUT;	//xxx mutex
			Tb_Echange[TeleInf_HPB_Global_MSB] = uc_HCHC_HCHP_TIME_OUT;
			Tb_Echange[TeleInf_HPB_Chauffage_LSB] = 0;
			Tb_Echange[TeleInf_HPB_Chauffage_MSB] = 0;
			Tb_Echange[TeleInf_HPB_Refroid_LSB] = 0;
			Tb_Echange[TeleInf_HPB_Refroid_MSB] = 0;
			Tb_Echange[TeleInf_HPB_EauChaude_LSB] = 0;
			Tb_Echange[TeleInf_HPB_EauChaude_MSB] = 0;
			Tb_Echange[TeleInf_HPB_Prises_LSB] = 0;
			Tb_Echange[TeleInf_HPB_Prises_MSB] = 0;
			Tb_Echange[TeleInf_HPB_Autres_LSB] = 0;
			Tb_Echange[TeleInf_HPB_Autres_MSB] = 0;
		}
		else
		{
			l_ul_Difference = ul_TeleinfoHCHP - ul_TeleinfoHCHPPrecedent;
			if(ul_TeleinfoHCHPPrecedent == 0)	l_ul_Difference = 0;	// 1er passage 
			ul_TeleinfoHCHPPrecedent = ul_TeleinfoHCHP;
			
			l_ul_Repartition = l_ul_Difference;
			if(l_ul_Repartition > 60000)	l_ul_Repartition = 60000;
			Tb_Echange[TeleInf_HPB_Global_LSB] = (uint_8)l_ul_Repartition; //xxx mutex
			Tb_Echange[TeleInf_HPB_Global_MSB] = (uint_8)(l_ul_Repartition >> 8);

			l_ul_Cumul = 0;
			// Calcul des 4 premiers avec arrondi
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Chauffage, TeleInf_HPB_Chauffage_LSB, TeleInf_HPB_Chauffage_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Refroid, TeleInf_HPB_Refroid_LSB, TeleInf_HPB_Refroid_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_EauChaude, TeleInf_HPB_EauChaude_LSB, TeleInf_HPB_EauChaude_MSB);
			vd_CalculerRepartition(l_ul_Difference, &l_ul_Cumul, TeleInf_Repartition_Prises, TeleInf_HPB_Prises_LSB, TeleInf_HPB_Prises_MSB);

			// Calcul du dernier : reste total - 4 premiers
			l_ul_Repartition = l_ul_Difference - l_ul_Cumul;
			if(l_ul_Repartition > 60000)	l_ul_Repartition = 60000;
			Tb_Echange[TeleInf_HPB_Autres_LSB] = (uint_8)l_ul_Repartition; //xxx mutex
			Tb_Echange[TeleInf_HPB_Autres_MSB] = (uint_8)(l_ul_Repartition >> 8);
		}
	}
}

unsigned long ul_ArrondirValeur(unsigned long ul_Valeur)
{
	if((ul_Valeur % 100) > 50)	return 1;
	else						return 0;
}

void vd_CalculerRepartition(unsigned long ul_Difference, unsigned long *ul_Cumul, unsigned short us_IndexCoefRepartition, unsigned short us_IndiceValeurLSB, unsigned short us_IndiceValeurMSB)
{
	unsigned long l_ul_Repartition;
	
	l_ul_Repartition = ul_Difference * Tb_Echange[us_IndexCoefRepartition];
	l_ul_Repartition = l_ul_Repartition  / 100 + ul_ArrondirValeur(l_ul_Repartition);
	if(l_ul_Repartition > 60000)	l_ul_Repartition = 60000;
	*ul_Cumul = *ul_Cumul + l_ul_Repartition;
	Tb_Echange[us_IndiceValeurLSB] = (uint_8)l_ul_Repartition; //xxx mutex
	Tb_Echange[us_IndiceValeurMSB] = (uint_8)(l_ul_Repartition >> 8);
}
