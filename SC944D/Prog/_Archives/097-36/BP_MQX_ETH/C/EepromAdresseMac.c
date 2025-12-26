#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include <string.h>
#include <rtcs.h>
#include <ipcfg.h>
#include "application.h"	//xxx

#include <spi.h>

#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "EspionRS.h"
#include "ba_i2c.h"
#include "global.h"
#include "crc.h"
#include "eepromspi.h"
#include "GestionSocket.h"
#include "cryptage.h"
#include "Cryptagerijndael_mode.h"
#include "download.h"
#include "hard.h"
#include "www.h"
#include "json.h"

#define extern
#include "EepromAdresseMac.h"
#undef extern

#define uc_ADRESSE_EEPROM_ADRESSE_MAC	0xFA
#define uc_ADRESSE_EEPROM_CLE_SERVEUR	0x00						// TAILLE : Cle_Acces_Distance_TAILLE - 16 CARACTERES
#define uc_ADRESSE_EEPROM_CODE_ALARME	Cle_Acces_Distance_TAILLE	// TAILLE : 2

//xxx rajotuer crc sur contenu eeprom !!!

// Port SPI deja ouvert et configuré par vd_SpiOpen()
// CS EEPROM Soft sélectionné par vd_SpiSelectEEPROMAdresseMac();

// Lit l'adresse MAC -> uc_AdresseMac[]
// Si PB : adresse MAC <- 0
void vd_ReadAdresseMac(void)
{
	unsigned char l_uc_Buffer[10];
	unsigned char l_uc_Compteur;
	signed long l_sl_Retour;

	

	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_AdresseMac);l_uc_Compteur++)
	{
		uc_AdresseMac[l_uc_Compteur] = 0;
		Tb_Echange[AdresseMAC_1+l_uc_Compteur] = 0xFF;
	}

	if(uc_PortSPIouvert != 0)
	{
		// Config OK -> lecture @ MAC
		for(l_uc_Compteur = 0; l_uc_Compteur < 10; l_uc_Compteur++)
		{
			l_uc_Buffer[l_uc_Compteur] = 0;
		}
		
		l_uc_Buffer[0] = 0x03;
		l_uc_Buffer[1] = uc_ADRESSE_EEPROM_ADRESSE_MAC;
		
		l_sl_Retour = read(fd_SPI_EEPROM, l_uc_Buffer, 8);
		fflush(fd_SPI_EEPROM);

		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM @ MAC\tREAD %d\n",l_sl_Retour);	
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM @ MAC\tBUFFER : ");
		for(l_uc_Compteur = 0; l_uc_Compteur < 10; l_uc_Compteur++)
		{
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"%02X.", l_uc_Buffer[l_uc_Compteur]);
		}
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"\n");
		
		if(l_sl_Retour == 8)
		{
			// Lecture OK -> prise en compte valeur pour applicatif
			for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_AdresseMac);l_uc_Compteur++)
			{
				uc_AdresseMac[l_uc_Compteur] = l_uc_Buffer[l_uc_Compteur+2];
				Tb_Echange[AdresseMAC_1+l_uc_Compteur] = l_uc_Buffer[l_uc_Compteur+2];
			}
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM @ MAC\tLECTURE @ MAC EEPROM OK\n");
		}
	}
}

void vd_ReadCleServeur(void)
{
	unsigned char l_uc_Buffer[sizeof(uc_Cle_Acces_Distance)+2];
	unsigned char l_uc_Compteur;
	signed long l_sl_Retour;


	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
	{
		uc_Cle_Acces_Distance[l_uc_Compteur] = 0;
	}

	if(uc_PortSPIouvert != 0)
	{
		// Config OK -> lecture data
		for(l_uc_Compteur = 0; l_uc_Compteur < sizeof(l_uc_Buffer); l_uc_Compteur++)
		{
			l_uc_Buffer[l_uc_Compteur] = 0;
		}
		
		l_uc_Buffer[0] = 0x03;
		l_uc_Buffer[1] = uc_ADRESSE_EEPROM_CLE_SERVEUR;
		
		l_sl_Retour = read(fd_SPI_EEPROM, l_uc_Buffer, sizeof(l_uc_Buffer));
		fflush(fd_SPI_EEPROM);

		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CLE SERVEUR\tREAD %d\n",l_sl_Retour);	
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CLE SERVEUR\tBUFFER : ");
		for(l_uc_Compteur = 0; l_uc_Compteur < sizeof(l_uc_Buffer); l_uc_Compteur++)
		{
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"%02X.", l_uc_Buffer[l_uc_Compteur]);
		}
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"\n");
		
		if(l_sl_Retour == sizeof(l_uc_Buffer))
		{
			// Lecture OK -> prise en compte valeur pour applicatif
			for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
			{
				uc_Cle_Acces_Distance[l_uc_Compteur] = l_uc_Buffer[l_uc_Compteur+2];
			}
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CLE SERVEUR\tLECTURE EEPROM OK\n");
		}
	}
}

// met à jour l'EEPROM si la clé serveur a été modifiée
void vd_UpdateCleServeurDansEEPROM(void)
{
	unsigned char l_uc_Compteur;
	unsigned char l_uc_Valeur;
	unsigned char l_uc_CleServeurModifiee;
	
	l_uc_CleServeurModifiee = 0;
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
	{
		l_uc_Valeur = Tb_Echange[Cle_Acces_Distance + l_uc_Compteur];
		if(l_uc_Valeur != uc_Cle_Acces_Distance[l_uc_Compteur])
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"CLE SERVEUR MODIFIEE (@ %d) : %02X -> %02X\n", l_uc_Compteur, uc_Cle_Acces_Distance[l_uc_Compteur], l_uc_Valeur);
			uc_Cle_Acces_Distance[l_uc_Compteur] = l_uc_Valeur;
			vd_MemoriserValeurDansEEPROMAdresseMac(uc_ADRESSE_EEPROM_CLE_SERVEUR+l_uc_Compteur, l_uc_Valeur);
			l_uc_CleServeurModifiee = 1;
		}
	}
	
	if(l_uc_CleServeurModifiee != 0)
	{
		cryptage();	// Il faut recalculer la clé à envoyer au serveur
	}
}

void vd_ReadCodeAlarme(void)
{
	unsigned char l_uc_Buffer[2+2];
	unsigned char l_uc_Compteur;
	signed long l_sl_Retour;


	Tb_Echange[Alarme_CodeUser1LSB] = 0;
	Tb_Echange[Alarme_CodeUser1MSB] = 0;
	if(uc_PortSPIouvert != 0)
	{
		// Config OK -> lecture data
		for(l_uc_Compteur = 0; l_uc_Compteur < sizeof(l_uc_Buffer); l_uc_Compteur++)
		{
			l_uc_Buffer[l_uc_Compteur] = 0;
		}
		
		l_uc_Buffer[0] = 0x03;
		l_uc_Buffer[1] = uc_ADRESSE_EEPROM_CODE_ALARME;
		
		l_sl_Retour = read(fd_SPI_EEPROM, l_uc_Buffer, sizeof(l_uc_Buffer));
		fflush(fd_SPI_EEPROM);

		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CODE ALARME\tREAD %d\n",l_sl_Retour);	
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CODE ALARME\tBUFFER : ");
		for(l_uc_Compteur = 0; l_uc_Compteur < sizeof(l_uc_Buffer); l_uc_Compteur++)
		{
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"%02X.", l_uc_Buffer[l_uc_Compteur]);
		}
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"\n");
		
		if(l_sl_Retour == sizeof(l_uc_Buffer))
		{
			// Lecture OK -> prise en compte valeur pour applicatif
			if(l_uc_Buffer[2] != 0xFF && l_uc_Buffer[3] != 0xFF)
			{
				Tb_Echange[Alarme_CodeUser1LSB] = l_uc_Buffer[2];
				Tb_Echange[Alarme_CodeUser1MSB] = l_uc_Buffer[3];
			}
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI EEPROM CLE SERVEUR\tLECTURE EEPROM OK\n");
		}
	}
}

// met à jour l'EEPROM si le code alarme a été modifié
void vd_UpdateCodeAlarmeDansEEPROM(void)
{
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"MEMORISATION CODE ALARME %d %d\n", Tb_Echange[Alarme_CodeUser1LSB], Tb_Echange[Alarme_CodeUser1MSB]);
	vd_MemoriserValeurDansEEPROMAdresseMac(uc_ADRESSE_EEPROM_CODE_ALARME, Tb_Echange[Alarme_CodeUser1LSB]);
	vd_MemoriserValeurDansEEPROMAdresseMac(uc_ADRESSE_EEPROM_CODE_ALARME+1, Tb_Echange[Alarme_CodeUser1MSB]);
}

//xxx rajouter retour erreur, log erreur et relecture après écriture
void vd_MemoriserValeurDansEEPROMAdresseMac(unsigned char uc_Adresse, unsigned char uc_ValeurASauvegarder)
{
	unsigned char l_uc_Buffer[3];
	signed long l_sl_Retour;
	
	// CS EEPROM ADRESSE MAC
	vd_SpiSelectEEPROMAdresseMac();	// Au cas où...
	
	// Autoriser accès en écriture
	l_uc_Buffer[0] = 0x06;
	l_sl_Retour = write(fd_SPI_EEPROM, l_uc_Buffer, 1);
	fflush(fd_SPI_EEPROM);
	//xxx tester l_sl_Retour

	// Ecriture valeur
	l_uc_Buffer[0] = 0x02;
	l_uc_Buffer[1] = uc_Adresse;
	l_uc_Buffer[2] = uc_ValeurASauvegarder;
	l_sl_Retour = write(fd_SPI_EEPROM, l_uc_Buffer, 3);
	fflush(fd_SPI_EEPROM);
	//xxx tester l_sl_Retour
	
	//xxx relire registre status pour attendre fin opération
	_time_delay(5);	
}
