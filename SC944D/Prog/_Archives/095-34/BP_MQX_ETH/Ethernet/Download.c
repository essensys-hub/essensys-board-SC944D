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
#include "TableEchangeAcces.h"
#include "global.h"
#include "ecran.h"
#include "GestionSocket.h"
#include "www.h"
#include "hard.h"
#include "espionrs.h"
#include "crc.h"
#include "eepromsoft.h"
#include "eepromspi.h"

#define extern
#include "download.h"
#undef extern


#if !BSPCFG_ENABLE_FLASHX
	#error This application requires BSPCFG_ENABLE_FLASHX defined non-zero in user_config.h. Please recompile BSP with this option.
#endif

// !!! Utilisation des fonctions suivantes !!!
// sc_OpenFlash() : ouvre le fichier "FLASH", et verifie sa taille - A FAIRE EN PREMIER
// sc_EffacerZoneNouveauProgramme() : efface la flash (zone nouveau programme) et vérifie qu'il soit à blanck
// sc_FlashMemoriserBinaireRecu() : decortique les infos S19 recues, et pour chaque ligne complete fait appel à uc_MemoriseLigneS19EnFlash()
//		uc_MemoriseLigneS19EnFlash() : mémorise la ligne S19 et relie
// vd_CheckZoneNouveauProgramme() : verifie le CRC de la zone nouveau programme, et affiche en printf les infos version et CRC de cette zone
// sc_CloseFlash() : ferme le fichier "FLASH" - A faire si l'ouverture avait fonctionner


//MQX_FILE_PTR flash_file = NULL;

// Modification 10/04/204 : ajout gestion EEPROM SOFT (par SPI)
// Les fonctions sont laissées telles quelles. Seul le contenu de ces fonctions est modifié pour faire appel aux fonction SPI d'accès à l'EEPROM Soft


// Ouvre le fichier FLASH et verifie sa taille
signed char sc_OpenFlash(void)
{
	signed char l_sc_Retour;
	unsigned char l_uc_Retour;
	unsigned char l_uc_Status;
	unsigned char l_uc_Data;
	
	
	l_sc_Retour = -1;
//	flash_file = fopen(FLASH_NAME, NULL);
//	if(flash_file == NULL)
//	{
//		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ERREUR OUVERTURE FLASH !\n");
//		l_sc_Retour = -2;
//	}
//	else
//	{
//		// Verification taille FLASH
//		fseek(flash_file, 0, IO_SEEK_END);	// xxx tester retour + verif fct ftell)
//		if(ftell(flash_file) != ul_FLASH_TAILLE)
//		{
//			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ERREUR TAILLE FLASH %x !\n", ftell(flash_file));
//			l_sc_Retour = -3;
//		}
//		else
//		{
//			// Désactive le mode cache
//			ioctl(flash_file, FLASH_IOCTL_DISABLE_SECTOR_CACHE, NULL);	//xxx tester retour
//			l_sc_Retour = 0;
//		}
//	}
	
	vd_SpiSelectEEPROMSoft();

	l_uc_Retour = uc_EEEPROMSoft_LectureStatus(&l_uc_Status);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEEPROMSoft_LectureStatus : %d -> %d\n", l_uc_Retour, l_uc_Status);

	if(l_uc_Retour == 0)
	{
		l_uc_Retour = uc_EEEPROMSoft_EnableEcritureStatus();
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEEPROMSoft_EnableEcritureStatus : %d\n", l_uc_Retour);

		if(l_uc_Retour == 0)
		{
			l_uc_Retour = uc_EEEPROMSoft_StatusEnableEcriture();
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEEPROMSoft_StatusEnableEcriture : %d\n", l_uc_Retour);

			if(l_uc_Retour == 0)
			{
				l_uc_Retour = uc_EEEPROMSoft_LectureStatus(&l_uc_Status);
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEEPROMSoft_LectureStatus : %d -> %d\n", l_uc_Retour, l_uc_Status);
				
				if(l_uc_Retour == 0)
				{
					l_sc_Retour = 0;
					
					// lecture juste pour checker état flash
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEPROMSoft_Lecture : ");
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d ", uc_EEPROMSoft_Lecture(0, 1, &l_uc_Data));
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%02X\n", l_uc_Data);
				}
			}
		}
	}
	
	return l_sc_Retour;
}

// Efface tous les secteurs de la zone NOUVEAU PROGRAMME et vérifie que tout est à 0xFF après
signed char sc_EffacerZoneNouveauProgramme(void)
{
	signed char l_sc_Retour;
	unsigned char l_uc_Retour;
	unsigned char l_uc_Data[12];
	unsigned char l_uc_Compteur;

	unsigned long l_ul_AdresseFlash;
	unsigned char *l_uc_PointeurRelecture;

	l_sc_Retour = 0;

//	// Efface tous les blocs
//	for(l_ul_AdresseFlash = ul_FLASH_NEW_SOFT_START; l_ul_AdresseFlash < (ul_FLASH_NEW_SOFT_START+ul_FLASH_SOFT_LENGTH); l_ul_AdresseFlash+=ul_FLASH_TAILLE_SECTEUR)
//	{
//		fseek(flash_file, l_ul_AdresseFlash, IO_SEEK_SET);	// xxx tester retour
//		ioctl(flash_file, FLASH_IOCTL_ERASE_SECTOR, NULL);	// xxx tester retour
//	}
//	
//	// Verification de toute la zone NOUVEAU PROGRAMME
//	l_uc_PointeurRelecture = (unsigned char *)ul_FLASH_NEW_SOFT_START;
//	for(l_ul_AdresseFlash = 0; l_ul_AdresseFlash < ul_FLASH_SOFT_LENGTH && l_sc_Retour == 0; l_ul_AdresseFlash++)
//	{
//		if(l_uc_PointeurRelecture[l_ul_AdresseFlash] != 0xFF)
//		{
//			l_sc_Retour = -1;
//			printf("FLASH NON BLANCK @ %x : %d", ul_FLASH_NEW_SOFT_START+l_ul_AdresseFlash, l_uc_PointeurRelecture[l_ul_AdresseFlash]);
//		}
//	}
	
	// Efface toute l'EEPROM Soft
	l_uc_Retour = uc_EEEPROMSoft_Effacer();
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEEPROMSoft_Effacer : %d\n", l_uc_Retour);

	// lecture juste pour checker état flash
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_EEPROMSoft_Lecture : ");
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d - ", uc_EEPROMSoft_Lecture(0, 5, l_uc_Data));
	for(l_uc_Compteur = 0;l_uc_Compteur < 8;l_uc_Compteur++)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%02X.", l_uc_Data[4+l_uc_Compteur]);
	}
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n");

	// Verifie que tout est à 0xFF
	// xxx a faire
	
	return l_sc_Retour;
}


// Traite toute la chaine qui peut contenir plusieurs lignes S19
// !!! La dernière ligne n'est pas forcement complete -> conserver en attendant la suite...
// Caracteres de separation entre deux lignes S9 : "\r\n" -> CR.LF -> 13.10
signed char sc_FlashMemoriserBinaireRecu(char *pc_BinaireTelecharge)
{
	signed char l_sc_Retour;
	unsigned char l_uc_Retour;
	unsigned short l_us_CompteurNewData;
	unsigned char l_uc_CompteurAntiSlash;
	unsigned char l_uc_CompteurR;
	unsigned char l_uc_CompteurN;
	unsigned char l_uc_Octet;
	
	
	l_sc_Retour = 0;


	// xxx verifier que le  buffer contient bien \0
		
	// Traitement des nouvelles lignes recues :
	l_us_CompteurNewData = 0;
	l_uc_CompteurAntiSlash = 0;
	l_uc_CompteurR = 0;
	l_uc_CompteurN = 0;
	while(pc_BinaireTelecharge[l_us_CompteurNewData] != 0 && l_sc_Retour == 0)
	{
		l_uc_Octet = (unsigned char)pc_BinaireTelecharge[l_us_CompteurNewData];
		uc_LigneS19[us_CompteurLigneS19] = l_uc_Octet;
		us_CompteurLigneS19++;				// xxx verif max tableau uc_LigneS19_TAILLE
		l_us_CompteurNewData++;	// xxx verif max tableau

		if(l_uc_Octet == '\\')		l_uc_CompteurAntiSlash++;
		else if(l_uc_Octet == 'r')	l_uc_CompteurR++;
		else if(l_uc_Octet == 'n')
		{
			l_uc_CompteurN++;
			if(l_uc_CompteurAntiSlash == 2 && l_uc_CompteurR == 1 && l_uc_CompteurN == 1)
			{
				uc_LigneS19[us_CompteurLigneS19-4] = 0;
				//printf("%s\n", uc_LigneS19);
				l_uc_Retour = uc_MemoriseLigneS19EnFlash(uc_LigneS19, us_CompteurLigneS19-4);
				if(l_uc_Retour != 0)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur uc_MemoriseLigneS19EnFlash %d\n", l_uc_Retour);
					l_sc_Retour = -1;
				}
				us_CompteurLigneS19 = 0;
			}
			else
			{
				// xxx erreur coherence / tester tous les cas possibles ! notamment si un S suit le n
			}
		}
		else	// Autre caractere
		{
			l_uc_CompteurAntiSlash = 0;
			l_uc_CompteurR = 0;
			l_uc_CompteurN = 0;
		}
	}
	
	return l_sc_Retour;
}

// Verifie le CRC de la zone NOUVEAU PROGRAMME
// Affiche dans tous les cas en PRINTF les infos de la zone NOUVEAU PROGRAMME :
//		CRC calculé, CRC Flash, Version Flash
// Demande le reset si la zone NOUVEAU PROGRAMME contient un programme valide
signed char sc_CheckZoneNouveauProgramme(void)
{
	unsigned short l_us_CRCNewSoftCalcule;
	unsigned short l_us_CRCNewSoftLuEnFlash;
	unsigned short l_us_VersionNewSoft;
	
	l_us_CRCNewSoftCalcule = 0;
	l_us_CRCNewSoftLuEnFlash = 0;
	l_us_VersionNewSoft = 0;
	
	vd_CalculInfosZoneNew(&l_us_CRCNewSoftCalcule, &l_us_CRCNewSoftLuEnFlash, &l_us_VersionNewSoft);
	
	uc_ResetDemande = 0;
	if(l_us_CRCNewSoftCalcule == l_us_CRCNewSoftLuEnFlash)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DEMANDE RESET...\n");
		uc_ResetDemande = 1;
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CRC NOUVEAU PROGRAMME INCORRECT !\n");
	}
}

// Ferme le fichier FLASH si different de NULL
signed char sc_CloseFlash(void)
{
//	if(flash_file != NULL)
//	{
//		fclose(flash_file);
//		flash_file = NULL;
//	}
	vd_SpiSelectEEPROMAdresseMac();
	
	return 0;
}

unsigned char uc_ConvertirAsciiToValue(unsigned char uc_Ascii)
{
	unsigned char l_uc_Retour;
	
	l_uc_Retour = 0;
	if(uc_Ascii >= '0' && uc_Ascii <= '9')
	{
		l_uc_Retour = (unsigned char)(uc_Ascii - '0');
	}
	else if(uc_Ascii >= 'A' && uc_Ascii <= 'F')
	{
		l_uc_Retour = (unsigned char)(uc_Ascii - 'A' + 10);
	}
	return l_uc_Retour;
}

unsigned char uc_ConvertirAsciiToByte(unsigned char uc_Ascii1, unsigned char uc_Ascii2)
{
	unsigned char l_uc_Valeur;
	
	l_uc_Valeur = (unsigned char)((uc_ConvertirAsciiToValue(uc_Ascii1) << 4) + uc_ConvertirAsciiToValue(uc_Ascii2));
	return l_uc_Valeur;
}

// Analyse le contenu du buffer passe en paramètre, et stocke les informations en Flash dans zone nouveau programme + relecture octet par octet
// Retourne 0 si OK, > 0 si erreur
// Exemples de lignes S19
// 		S0030000FC
// 		S31D00000400000000000000000000000000000000000000000000000000DE
//		STLLDDDDCC	T : type de ligne, LL : nb de caractères / 2 à partir de LL et jusqu'à la fin de la ligne, DDDD : data (taille fonction du type), CC : checksum
// Analyse du contenu :
//		- Ligne commence par un S
//		- Longueur LL = longueur passée en paramètre
// 		- Checksum CC
//		- Ignorent les lignes type 0 et 7
//		- Traite les lignes type 3 (adresse sur 4 octets)
//		- Verifie adresse comprise dans zone programme
unsigned char uc_MemoriseLigneS19EnFlash(unsigned char *p_uc_S19, unsigned char uc_S19Size)
{
	unsigned char l_uc_CodeRetour;
	unsigned char l_uc_CheckSumCalcule;
	unsigned char l_uc_CheckSumRecu;
	unsigned char l_uc_Longueur;
	unsigned char l_uc_Compteur;
	unsigned long l_ul_Adresse;
//	unsigned long l_ul_Valeur;
//	unsigned long l_ul_Flashx;
//	unsigned long *l_p_ul_Flash;
//	unsigned char *l_p_uc_Flash;
	unsigned char l_uc_NbOctetsAEcrire;
//	unsigned char l_uc_DecalageStart;
//	unsigned char l_uc_CompteurDataRecu;
	unsigned char l_uc_OctetAEcrire;
	//unsigned char *l_puc_PointeurRelecture;
	long l_l_CodeRetour;


	l_uc_CodeRetour = 0;
	// Contrôle entete ligne
	if(p_uc_S19[0] == 'S')
	{
		// Contrôle longueur ligne
		l_uc_Longueur = (unsigned char)(uc_ConvertirAsciiToByte(p_uc_S19[2],p_uc_S19[3])*2+4);
		if(l_uc_Longueur == uc_S19Size)
		{
			// Contrôle checksum ligne
			l_uc_CheckSumCalcule = 0;
			for(l_uc_Compteur=2;l_uc_Compteur<(uc_S19Size-2);l_uc_Compteur+=2)
			{
				l_uc_CheckSumCalcule = (unsigned char)(l_uc_CheckSumCalcule + uc_ConvertirAsciiToByte(p_uc_S19[l_uc_Compteur],p_uc_S19[l_uc_Compteur+1]));
			}
			l_uc_CheckSumCalcule = (unsigned char)(~l_uc_CheckSumCalcule);
			
			l_uc_CheckSumRecu = uc_ConvertirAsciiToByte(p_uc_S19[uc_S19Size-2],p_uc_S19[uc_S19Size-1]);
			
			if(l_uc_CheckSumRecu == l_uc_CheckSumCalcule)
			{
				if(p_uc_S19[1] == '0')
				{
					// Ne rien faire
				}
				else if(p_uc_S19[1] == '7')
				{
					// Ne rien faire
				}
				else if(p_uc_S19[1] == '3')
				{
					l_ul_Adresse = 0;
					l_ul_Adresse = uc_ConvertirAsciiToByte(p_uc_S19[4],p_uc_S19[5]);
					l_ul_Adresse = l_ul_Adresse << 8;
					l_ul_Adresse = l_ul_Adresse + uc_ConvertirAsciiToByte(p_uc_S19[6],p_uc_S19[7]);
					l_ul_Adresse = l_ul_Adresse << 8;
					l_ul_Adresse = l_ul_Adresse + uc_ConvertirAsciiToByte(p_uc_S19[8],p_uc_S19[9]);
					l_ul_Adresse = l_ul_Adresse << 8;
					l_ul_Adresse = l_ul_Adresse + uc_ConvertirAsciiToByte(p_uc_S19[10],p_uc_S19[11]);
					//printf("l_ul_Adresse : %x\n", l_ul_Adresse);
										
					if(l_ul_Adresse >= 0 && l_ul_Adresse < ul_FLASH_APP_SOFT_END)	// On peut recevoir un binaire intégrant le bootloader
					{
						if(l_ul_Adresse >= ul_FLASH_APP_SOFT_START && l_ul_Adresse < ul_FLASH_APP_SOFT_END)	// On ne prend que l'applicatif
						{
							l_ul_Adresse = l_ul_Adresse - ul_FLASH_APP_SOFT_START;
							
							//l_puc_PointeurRelecture = (unsigned char *)(l_ul_Adresse + ul_FLASH_NEW_SOFT_START);
		
							l_uc_NbOctetsAEcrire = (uc_S19Size - 14) / 2;
							//printf("l_uc_NbOctetsAEcrire %d\n", l_uc_NbOctetsAEcrire);
	
							//fseek(flash_file, (long)(l_ul_Adresse + ul_FLASH_NEW_SOFT_START), IO_SEEK_SET);	//xxx check retour

							for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbOctetsAEcrire && l_uc_CodeRetour == 0;l_uc_Compteur++)
							{
								l_uc_OctetAEcrire = uc_ConvertirAsciiToByte(p_uc_S19[12 + l_uc_Compteur*2],p_uc_S19[13 + l_uc_Compteur*2]);

								//l_l_CodeRetour = write(flash_file, &l_uc_OctetAEcrire, 1);
								
								l_uc_CodeRetour = uc_EEPROMSoft_Ecriture(l_ul_Adresse, l_uc_OctetAEcrire);								
								if(l_uc_CodeRetour != 0)
								{
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur save FLASH @ : %x - Value : %d - Retour : %d\n", l_ul_Adresse, l_uc_OctetAEcrire, l_uc_CodeRetour);
									l_uc_CodeRetour = 6;
								}
								else
								{
									// Verification octet ecrit correctement
//									if(l_puc_PointeurRelecture[l_uc_Compteur] != l_uc_OctetAEcrire)
//									{
//										printf("Erreur relecture FLASH @ : %x - Value écrite : %d - Valeur Flash : %d\n", l_ul_Adresse+l_uc_Compteur, l_uc_OctetAEcrire, l_puc_PointeurRelecture[l_uc_Compteur]);
//										l_uc_CodeRetour = 7;
//									}
									//xxx a faire
								}
								l_ul_Adresse++;
							}
						}
					}
					else
					{
						l_uc_CodeRetour = 5;
					}
				}
				else
				{
					l_uc_CodeRetour = 4;
				}
			}
			else
			{
				l_uc_CodeRetour = 3;
			}
		}
		else
		{
			l_uc_CodeRetour = 2;
		}
	}
	else
	{
		l_uc_CodeRetour = 1;
	}
	return l_uc_CodeRetour;
}

unsigned short us_CalculerCRCZoneApp(void)
{ 
	unsigned short l_us_CRC;
	unsigned char *l_puc_address;

	l_us_CRC = 0xFFFF;
	
	for(l_puc_address=(unsigned char *)(ul_FLASH_APP_SOFT_START+ul_FLASH_REL_SOFT_CRC_SIZE);(unsigned long)l_puc_address < (unsigned long)(ul_FLASH_APP_SOFT_START+ul_FLASH_SOFT_LENGTH) ; (unsigned long)l_puc_address++)	
	{
		l_us_CRC = us_CalculerCRCSurUnOctet(l_us_CRC,(unsigned char)*l_puc_address);		//poid faible
	}
	return l_us_CRC;
}

void vd_CalculInfosZoneSoft(unsigned short *p_us_CRCCalcule,
						    unsigned short *p_us_CRCLuEnFlash,
						    unsigned short *p_us_Version)
{
	unsigned short *l_p_us_Pointeur;

	*p_us_CRCCalcule = us_CalculerCRCZoneApp();

	l_p_us_Pointeur = (unsigned short *)(ul_FLASH_APP_SOFT_START+ul_FLASH_REL_SOFT_CRC);
	*p_us_CRCLuEnFlash = *l_p_us_Pointeur;	

	l_p_us_Pointeur = (unsigned short *)(ul_FLASH_APP_SOFT_START+ul_FLASH_REL_SOFT_VERSION);
	*p_us_Version = *l_p_us_Pointeur;

	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CRC CALCULE     : %04X\r\n", *p_us_CRCCalcule);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CRC LU EN FLASH : %04X\r\n", *p_us_CRCLuEnFlash);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"V. : %04X\r\n", *p_us_Version);
}

unsigned short us_CalculerCRCZoneNew(void)
{ 
	unsigned short l_us_CRC;
	unsigned long l_ul_Compteur;
	unsigned long l_ul_Compteur2;
	unsigned char l_uc_Data[8];

	l_us_CRC = 0xFFFF;
	
	for(l_ul_Compteur = (unsigned long)ul_FLASH_REL_SOFT_CRC_SIZE ; l_ul_Compteur < (unsigned long)ul_FLASH_SOFT_LENGTH ; l_ul_Compteur+=8)	
	{
		uc_EEPROMSoft_Lecture(l_ul_Compteur, 8, l_uc_Data);	// xxx check retour ??? pas forcement besoin -> si erreur lecture on aura forcement un pb de CRC
		for(l_ul_Compteur2 = 0;l_ul_Compteur2 < 8;l_ul_Compteur2++)
		{
			if((l_ul_Compteur+l_ul_Compteur2) < (unsigned long)ul_FLASH_SOFT_LENGTH)	l_us_CRC = us_CalculerCRCSurUnOctet(l_us_CRC,l_uc_Data[l_ul_Compteur2]);
		}
	}
	return l_us_CRC;
}

void vd_CalculInfosZoneNew(unsigned short *p_us_CRCCalcule,
						   unsigned short *p_us_CRCLuEnFlash,
						   unsigned short *p_us_Version)
{
	unsigned char l_uc_Data1;
	unsigned char l_uc_Data2;

	*p_us_CRCCalcule = us_CalculerCRCZoneNew();

	uc_EEPROMSoft_Lecture(ul_FLASH_REL_SOFT_CRC, 1, &l_uc_Data1);
	uc_EEPROMSoft_Lecture(ul_FLASH_REL_SOFT_CRC+1, 1, &l_uc_Data2);
	*p_us_CRCLuEnFlash = (unsigned short)l_uc_Data2 + (unsigned short)(l_uc_Data1 << 8);

	uc_EEPROMSoft_Lecture(ul_FLASH_REL_SOFT_VERSION, 1, &l_uc_Data1);
	uc_EEPROMSoft_Lecture(ul_FLASH_REL_SOFT_VERSION+1, 1, &l_uc_Data2);
	*p_us_Version = (unsigned short)l_uc_Data2 + (unsigned short)(l_uc_Data1 << 8);

	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CRC CALCULE     : %04X\r\n", *p_us_CRCCalcule);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CRC LU EN FLASH : %04X\r\n", *p_us_CRCLuEnFlash);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"V. : %04X\r\n", *p_us_Version);
}


