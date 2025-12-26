#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx

#include <spi.h>

#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "EspionRS.h"
#include "ba_i2c.h"
#include "global.h"
#include "crc.h"
#include "eepromspi.h"

#define extern
#include "EepromSoft.h"
#undef extern


// EEPROM Soft : c'est une FLASH mais pour avoir les fichiers au meme endroit dans le projet, je l'appelle EEPROM SOFT...

// Port SPI deja ouvert et configuré par vd_SpiOpen()
// CS EEPROM Soft sélectionné par vd_SpiSelectEEPROMSoft(); - A APPELER AVANT TOUT ACCES A L'EEPROM SOFT

// Envoie la commande pour effacer tout le composant
// Avant l'effacement, appelle la fonction uc_EEEPROMSoft_EnableEcriture()
// Et après chaque effacement réussi, attend que busy retombe via la fonction uc_EEPROMSoft_AttenteBusyA0()
// Retourne 0 si OK
unsigned char uc_EEEPROMSoft_Effacer(void)
{
	unsigned char l_uc_Buffer;
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;
	unsigned char l_uc_Status;
	unsigned char l_uc_Compteur;
	
	l_uc_Retour = 1;
	if(uc_PortSPIouvert != 0)
	{
		if(uc_EEEPROMSoft_EnableEcriture() == 0)
		{
			l_uc_Buffer = 0x60;
			l_sl_Retour = write(fd_SPI_EEPROM, &l_uc_Buffer, 1);
			fflush(fd_SPI_EEPROM);
			
			if(l_sl_Retour == 1)
			{
				if(uc_EEPROMSoft_AttenteBusyA0() == 0)	l_uc_Retour = 0;
			}
			else					uc_EspionEepromSoftErreurEffacement++;
		}
	}
	return l_uc_Retour;
}

// Retourne 0 si écriture status OK
unsigned char uc_EEEPROMSoft_StatusEnableEcriture(void)
{
	unsigned char l_uc_Buffer[2];
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;

	l_uc_Retour = 1;	
	if(uc_PortSPIouvert != 0)
	{
		l_uc_Buffer[0] = 0x01;
		l_uc_Buffer[1] = 0x00;

		l_sl_Retour = write(fd_SPI_EEPROM, l_uc_Buffer, 2);
		fflush(fd_SPI_EEPROM);

		if(l_sl_Retour == 2)		l_uc_Retour = 0;
		else						uc_EspionEepromSoftErreurStatusEnableEcriture++;
	}
	return l_uc_Retour;
}

// Retourne 0 si écriture commande OK
unsigned char uc_EEEPROMSoft_EnableEcritureStatus(void)
{
	unsigned char l_uc_Buffer;
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;
	
	l_uc_Retour = 1;
	if(uc_PortSPIouvert != 0)
	{
		l_uc_Buffer = 0x50;

		l_sl_Retour = write(fd_SPI_EEPROM, &l_uc_Buffer, 1);
		fflush(fd_SPI_EEPROM);

		if(l_sl_Retour == 1)	l_uc_Retour = 0;
		else					uc_EspionEepromSoftErreurEnableEcritureStatus++;
	}
	return l_uc_Retour;
}

// Lit à l'adresse spécifiée le nombre d'octets spécifié - MAX 16 octets !!!
// Le tableau passé en paramètre doit être suffisant grand pour recevoir les octets lus
// Retourne 0 si lecture OK
unsigned char uc_EEPROMSoft_Lecture(unsigned long ul_Adresse, unsigned char uc_NbOctetsALire, unsigned char uc_ValeursLues[])
{
	unsigned char l_uc_Compteur;
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;
	unsigned char l_uc_Tab[4+16];
	
	l_uc_Retour = 1;
	if(uc_PortSPIouvert != 0)
	{
		for(l_uc_Compteur = 0; l_uc_Compteur < sizeof(l_uc_Tab); l_uc_Compteur++)
		{
			l_uc_Tab[l_uc_Compteur] = 0;
		}
		l_uc_Tab[0] = 0x03;
		l_uc_Tab[1] = ((ul_Adresse >> 16) & 0xFF);
		l_uc_Tab[2] = ((ul_Adresse >> 8) & 0xFF);
		l_uc_Tab[3] = (ul_Adresse & 0xFF);
		
		l_sl_Retour = read(fd_SPI_EEPROM, l_uc_Tab, 4+uc_NbOctetsALire);
		fflush(fd_SPI_EEPROM);

		if(l_sl_Retour == (4+uc_NbOctetsALire))
		{
			for(l_uc_Compteur = 0; l_uc_Compteur < uc_NbOctetsALire; l_uc_Compteur++)
			{
				uc_ValeursLues[l_uc_Compteur] = l_uc_Tab[4 + l_uc_Compteur];
			}
			l_uc_Retour = 0;
		}
		else					uc_EspionEepromSoftErreurLecture++;
	}
	return l_uc_Retour;
}

// Ecrit un octet à l'adresse spécifiée et retourne 0 si OK
// Avant chaque écriture, appelle la fonction uc_EEEPROMSoft_EnableEcriture()
// Et après chaque écriture réussie, attend que busy retombe via la fonction uc_EEPROMSoft_AttenteBusyA0()
// Ne fait pas de relecture !!!
unsigned char uc_EEPROMSoft_Ecriture(unsigned long ul_Adresse, unsigned char uc_OctetAEcrire)
{
	unsigned char l_uc_Buffer[5];
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;
	
	l_uc_Retour = 1;
	if(uc_PortSPIouvert != 0)
	{
		if(uc_EEEPROMSoft_EnableEcriture() == 0)
		{
			l_uc_Buffer[0] = 0x02;
			l_uc_Buffer[1] = ((ul_Adresse >> 16) & 0xFF);
			l_uc_Buffer[2] = ((ul_Adresse >> 8) & 0xFF);
			l_uc_Buffer[3] = (ul_Adresse & 0xFF);
			l_uc_Buffer[4] = uc_OctetAEcrire;

			l_sl_Retour = write(fd_SPI_EEPROM, l_uc_Buffer, 5);
			fflush(fd_SPI_EEPROM);
	
			if(l_sl_Retour == 5)
			{
				if(uc_EEPROMSoft_AttenteBusyA0() == 0)	l_uc_Retour = 0;
			}
			else					uc_EspionEepromSoftErreurEcriture++;					
		}
	}
	return l_uc_Retour;
}

// A faire avant chaque écriture / effacement
// Retourne 0 si commande passée
unsigned char uc_EEEPROMSoft_EnableEcriture(void)
{
	unsigned char l_uc_Buffer;
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;
	
	l_uc_Retour = 1;
	
	if(uc_PortSPIouvert != 0)
	{
		l_uc_Buffer = 0x06;

		l_sl_Retour = write(fd_SPI_EEPROM, &l_uc_Buffer, 1);
		fflush(fd_SPI_EEPROM);

		if(l_sl_Retour == 1)	l_uc_Retour = 0;
		else					uc_EspionEepromSoftErreurEnableEcriture++;
	}
	return l_uc_Retour;
}

// Retourne 0 si lecture status OK
unsigned char uc_EEEPROMSoft_LectureStatus(unsigned char *uc_Status)
{
	unsigned char l_uc_Buffer[2];
	signed long l_sl_Retour;
	unsigned char l_uc_Retour;

	l_uc_Retour = 1;
	*uc_Status = 0;
	
	if(uc_PortSPIouvert != 0)
	{
		l_uc_Buffer[0] = 0x05;
		l_uc_Buffer[1] = 0x00;

		l_sl_Retour = read(fd_SPI_EEPROM, l_uc_Buffer, 2);
		fflush(fd_SPI_EEPROM);

		if(l_sl_Retour == 2)
		{
			l_uc_Retour = 0;
			*uc_Status = l_uc_Buffer[1];
		}
		else	uc_EspionEepromSoftErreurLectureEtat++;
	}
	return l_uc_Retour;
}

// Retourne 0 si BUSY lu et à 0, 1 dans le cas contraire (pb lecture ou busy à 1 après time out de 100 ms)
unsigned char uc_EEPROMSoft_AttenteBusyA0(void)
{
	unsigned char l_uc_Status;
	unsigned char l_uc_Compteur;

	l_uc_Status = 1;
	l_uc_Compteur = 0;
	while(l_uc_Status != 0 && l_uc_Compteur < 20)
	{
		l_uc_Compteur++;
		if(uc_EEEPROMSoft_LectureStatus(&l_uc_Status) != 0)
		{
			l_uc_Compteur = 255;	// Erreur lecture status -> Sortie de la boucle
		}
		else
		{
			l_uc_Status = l_uc_Status & 0x01;
			if(l_uc_Status != 0)	_time_delay(5);	// Mini tempo avant de redemander le status
			// Si l_uc_Status == 0 -> sortie boucle
		}
	}
	if(l_uc_Compteur >= 20 )	uc_EspionEepromSoftErreurAttenteBusy++;
	
	return l_uc_Status;
}

