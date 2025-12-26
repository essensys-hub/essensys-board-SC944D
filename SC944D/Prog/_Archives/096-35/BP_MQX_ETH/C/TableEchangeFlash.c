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
#include "tableechangedroits.h"
#include "global.h"
#include "EspionRS.h"
#include "crc.h"
#include "main.h"

#define extern
#include "tableechangeflash.h"
#undef extern


#define FLASH_NAME "flashx:bank0"	// Toute la zone FLASH

#define ul_FLASH_TAILLE				0x80000

#define ul_FLASH_CONFIG_ADRESSE		0x7E000
#define ul_FLASH_TAILLE_SECTEUR		0x01000
#define ul_FLASH_CONFIG_LENGTH		ul_FLASH_TAILLE_SECTEUR	// 1 secteur -> 4096 octets

// Adresses relatives
#define ul_FLASH_REL_SOFT_CRC		0x00000
#define ul_FLASH_REL_SOFT_CRC_SIZE	0x02


//xxx reprendre printf
//xxx verif taille table echange < secteur


unsigned short us_CalculerCRCZoneConfig(void)
{ 
	unsigned short l_us_CRC;
	unsigned char *l_puc_address;

	l_us_CRC = 0xFFFF;
	
	for(l_puc_address=(unsigned char *)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_REL_SOFT_CRC_SIZE);(unsigned long)l_puc_address < (unsigned long)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_CONFIG_LENGTH) ; (unsigned long)l_puc_address++)	
	{
		l_us_CRC = us_CalculerCRCSurUnOctet(l_us_CRC,(unsigned char)*l_puc_address);		//poid faible
	}
	return l_us_CRC;
}

// Fonction appelée au démarrage du logiciel
// Vérifie que CRC zone config OK
// Si OK -> lecture des valeurs de la flash -> table echange QUE POUR LES VALEURS EN ACCES VALEUR_SAUVEGARDEE
// Si PB -> utilisation des valeurs par défaut
void vd_TableEchangeLireEnFlash(void)
{
	unsigned short l_us_CRCCalcule;
	unsigned short l_us_CRCFlash;
	unsigned short *l_p_us_Pointeur;
	unsigned char *l_p_uc_Pointeur;
	unsigned short l_us_Compteur;
	
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_TableEchangeLireEnFlash DEBUT\n");
	
	l_us_CRCCalcule = us_CalculerCRCZoneConfig();
	
	l_p_us_Pointeur = (unsigned short *)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_REL_SOFT_CRC);
	l_us_CRCFlash = *l_p_us_Pointeur;	

	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"l_us_CRCCalcule : %x\n", l_us_CRCCalcule);
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"l_us_CRCFlash   : %x\n", l_us_CRCFlash);
	

	// Initialise toute la table d'échange avec les valeurs par défaut
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Table échange initialisée avec valeurs par défaut !!!\n");
	vd_Init_Echange();
	
	if(l_us_CRCCalcule == l_us_CRCFlash)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Table échange initialisée avec valeurs lues en FLASH !!!\n");
		
		l_p_uc_Pointeur = (unsigned char *)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_REL_SOFT_CRC_SIZE);
		for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees;l_us_Compteur++)
		{
			// Utilisation des valeurs lues en flash UNIQUEMENT pour les valeurs avec ACCES VALEUR_SAUVEGARDEE
			if((Tb_Echange_Droits[l_us_Compteur] & VALEUR_SAUVEGARDEE) != 0)
			{
				Tb_Echange[l_us_Compteur] = l_p_uc_Pointeur[l_us_Compteur];
			}
		}
	}
	
	for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees;l_us_Compteur++)
	{
		Tb_EchangePrecedent[l_us_Compteur] = Tb_Echange[l_us_Compteur]; // ne pas loguer les premiers changements de la table d'echange suite à l'init de cette table
	}

	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_TableEchangeLireEnFlash FIN\n");
	_time_delay(5000);
}

// Efface le secteur contenant la config
// Mémorise en flash les valeurs de toute la table d'échange
// Complète la fin du secteur par des 0
// Calcule le CRC et le mémorise en FLASH
void vd_TableEchangeSaveEnFLash(void)
{
	MQX_FILE_PTR flash_file = NULL;
	unsigned short l_us_Compteur;
	long l_l_CodeRetour;
	unsigned char l_uc_Valeur;
	unsigned short l_us_CRCCalcule;
	
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_TableEchangeSaveEnFLash DEBUT\n");
	
	// Empêche la sauvegarde des paramètres en cas de coupure d’alim secteur.
	// 0 = Mode de fonctionnement normal.
	// 1 = Mode de fonctionnement de test.
	if(Tb_Echange[Mode_Test] == 0)
	{
		// Efface le secteur contenant la config
		vd_EffacerZoneConfig();
	
		vd_BloquerITFilPIlote();	
		flash_file = fopen(FLASH_NAME, NULL);
		if(flash_file == NULL)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OUVERTURE FLASH PB !\n");
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OUVERTURE FLASH OK !\n");
			fseek(flash_file, 0, IO_SEEK_END);	// xxx tester retour + verif fct ftell)
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH %x !\n", ftell(flash_file));
			if(ftell(flash_file) != ul_FLASH_TAILLE)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH ERREUR\n");
			}
			else
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH OK\n");
				
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"MEMORISATION EN FLASH...");
				// Mémorise en flash les valeurs de toute la table d'échange
				for(l_us_Compteur = 0;l_us_Compteur < Nb_Tbb_Donnees;l_us_Compteur++)
				{
					fseek(flash_file, (long)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_REL_SOFT_CRC_SIZE + l_us_Compteur), IO_SEEK_SET);	//xxx check retour
					l_l_CodeRetour = write(flash_file, &Tb_Echange[l_us_Compteur], 1);
					if(l_l_CodeRetour != 1)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Erreur save FLASH @ : %x - Value : %d - Retour : %d\n", l_us_Compteur, Tb_Echange[l_us_Compteur], l_l_CodeRetour);
					}
					else
					{
						// xxx Verification octet ecrit correctement
					}
				}
				
				// Complète la fin du secteur par des 0
				l_uc_Valeur = 0;
				for(l_us_Compteur = Nb_Tbb_Donnees;l_us_Compteur < (ul_FLASH_CONFIG_LENGTH-ul_FLASH_REL_SOFT_CRC_SIZE);l_us_Compteur++)	//xxx verifier les max des boucles de ce fichier !!!
				{
					fseek(flash_file, (long)(ul_FLASH_CONFIG_ADRESSE+ul_FLASH_REL_SOFT_CRC_SIZE + l_us_Compteur), IO_SEEK_SET);	//xxx check retour
					l_l_CodeRetour = write(flash_file, &l_uc_Valeur, 1);
					if(l_l_CodeRetour != 1)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Erreur save FLASH @ : %x - Value : %d - Retour : %d\n", l_us_Compteur, l_uc_Valeur, l_l_CodeRetour);
					}
					else
					{
						// xxx Verification octet ecrit correctement
					}
				}
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
				
				// Calcule le CRC et le mémorise en FLASH
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"CALCUL CRC...");
				l_us_CRCCalcule = us_CalculerCRCZoneConfig();
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE," %x ", l_us_CRCCalcule);
				
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"\nMEMORISATION CRC...");
				fseek(flash_file, (long)(ul_FLASH_CONFIG_ADRESSE), IO_SEEK_SET);	//xxx check retour
				l_l_CodeRetour = write(flash_file, &l_us_CRCCalcule, 2);
				if(l_l_CodeRetour != 2)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Erreur save FLASH @ : %x - Value : %d - Retour : %d\n", ul_FLASH_CONFIG_ADRESSE, l_us_CRCCalcule, l_l_CodeRetour);
				}
				else
				{
					// xxx Verification octet ecrit correctement
				}
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
			}
		}
		
		if(flash_file != NULL)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"fclose...");
			fclose(flash_file);
			flash_file = NULL;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
		}
		
		vd_ReactiverITFilPIlote();
	}
	else
	{	// Sauvegarde désactivée !
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SAUVEGARDE DESACTIVEE PAR Mode_Test\n");
	}
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_TableEchangeSaveEnFLash FIN\n");
}

void vd_EffacerZoneConfig(void)
{
	MQX_FILE_PTR flash_file = NULL;
	
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_EffacerZoneConfig DEBUT\n");
	
	vd_BloquerITFilPIlote();
	flash_file = fopen(FLASH_NAME, NULL);
	if(flash_file == NULL)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OUVERTURE FLASH PB !\n");
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OUVERTURE FLASH OK !\n");
		fseek(flash_file, 0, IO_SEEK_END);	// xxx tester retour + verif fct ftell)
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH %x !\n", ftell(flash_file));
		if(ftell(flash_file) != ul_FLASH_TAILLE)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH ERREUR\n");
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TAILLE FLASH OK\n");
			
			// Efface le secteur contenant la config
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"EFFACEMENT...");
			fseek(flash_file, ul_FLASH_CONFIG_ADRESSE, IO_SEEK_SET);	// xxx tester retour
			ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);	// xxx tester retour
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
			//xxx verifier toute la zone a 0xFF
		}
	}
	
	if(flash_file != NULL)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"fclose...");
		fclose(flash_file);
		flash_file = NULL;
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
	}
	
	vd_ReactiverITFilPIlote();
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_EffacerZoneConfig FIN\n");
}

void vd_BloquerITFilPIlote(void)
{
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_BloquerITFilPIlote...");
	_bsp_int_init(lwgpio_int_get_vector(&IO_DIN_Secteur_Synchro), 7, 3, FALSE);	// IT Fil pilote
	_bsp_int_init(64+19, 7, 1, FALSE);	// Timer
	_time_delay(100);
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
}

void vd_ReactiverITFilPIlote(void)
{
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"vd_ReactiverITFilPIlote...");
	_bsp_int_init(lwgpio_int_get_vector(&IO_DIN_Secteur_Synchro), 7, 3, TRUE);	// IT Fil pilote
	_bsp_int_init(64+19, 7, 1, TRUE);	// Timer
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"OK\n");
}
