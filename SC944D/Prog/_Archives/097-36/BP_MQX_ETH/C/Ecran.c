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
#include "chauffage.h"

#define extern
#include  "Ecran.h"
#undef extern


// définitions et variables spécifiques à la tache écran

#define	us_TAILLE_BUFFER_TX		260		// xxx a verifier trame max 255 octets + 2 		à mettre à jour si les trames grandissent !
#define	us_TAILLE_BUFFER_RX		342		// xxx a verifier trame max 336 octets + 5 		à mettre à jour si les trames grandissent !
#define	us_TAILLE_BUFFER_STATUS	50		// xxx a verifier trame de status 25 octets 		à mettre à jour si les trames grandissent !

unsigned char uc_EcranBufferRx[us_TAILLE_BUFFER_RX];
unsigned short us_EcranNbOctetsRecus;

unsigned char uc_Buffer_Tx[us_TAILLE_BUFFER_TX];
unsigned char uc_Buffer_Status[us_TAILLE_BUFFER_STATUS];

LWSEM_STRUCT st_LW_lock_screen;				// Protection acces RS


// calcule la cheksum d'un buffer
unsigned char uc_fct_Calcul_CheckSum(unsigned char *p_uc_buffer, unsigned short us_Nb_Octets)
{
	unsigned short l_us_i;
	unsigned char l_uc_CheckSum = 0;

	for(l_us_i = 0;l_us_i < us_Nb_Octets;l_us_i++)
	{
		l_uc_CheckSum ^= p_uc_buffer[l_us_i];
	}
	return(l_uc_CheckSum);
}

// Prépare et envoie la trame de réponse de status
void vd_EnvoyerTrameStatus(MQX_FILE_PTR fd)     
{
	unsigned short l_us_Nb_Octets = 0;
	unsigned short l_us_temp;
	_mqx_uint l_uint_Retour;
	

	uc_Buffer_Status[l_us_Nb_Octets] = 0xF0;						// qualifier
	l_us_Nb_Octets++;
	
	// Acces st_DateHeurePourEcran -> protection par mutex
	ul_CompteurMutexEnvoyerTrameStatus++;
	l_uint_Retour = _mutex_lock(&st_Mutex_DateHeurePourEcran);
	if(l_uint_Retour != MQX_EOK)
	{
		ul_CompteurMutexEnvoyerTrameStatusErreur++;
		printf("x0 %d\n", l_uint_Retour);
		printf("X0XXXXXXXXXXXXXXXX PB MUTEX XXXXXXXXXXXXXXXXXXXXXXXX\n", l_uint_Retour);
		DETECTION_ERREUR_TACHE_ECRAN_MUTEX;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"Error mutex vd_EnvoyerTrameStatus!\n");
	}
	// On fait quand même le traitement...
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.DAY / 10) + 0x30;		// jour
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.DAY % 10) + 0x30;		// jour
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MONTH / 10) + 0x30;	// mois
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MONTH % 10) + 0x30;	// mois
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.YEAR / 1000) + 0x30;	// annee
	l_us_Nb_Octets++;
	l_us_temp = st_DateHeurePourEcran.YEAR % 1000;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (l_us_temp / 100) + 0x30;	// annee
	l_us_Nb_Octets++;
	l_us_temp = l_us_temp % 100;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (l_us_temp / 10) + 0x30;		// annee
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (l_us_temp % 10) + 0x30;		// annee
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.HOUR / 10)+ 0x30;		// heure
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.HOUR % 10)+ 0x30;		// heure
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MINUTE / 10)+ 0x30;	// minutes
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MINUTE % 10)+ 0x30;	// minutes
	l_us_Nb_Octets++;
	
	/*uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MINUTE / 10)+ 0x30;
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.MINUTE % 10)+ 0x30;
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.SECOND / 10)+ 0x30;
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) (st_DateHeurePourEcran.SECOND % 10)+ 0x30;
	l_us_Nb_Octets++;*/
	
	_mutex_unlock(&st_Mutex_DateHeurePourEcran);	// Libération...

	vd_CalculerEtatConsigne(st_chauf_ZJ, &uc_Buffer_Status[l_us_Nb_Octets]);
	l_us_Nb_Octets+=3;
	vd_CalculerEtatConsigne(st_chauf_ZN, &uc_Buffer_Status[l_us_Nb_Octets]);
	l_us_Nb_Octets+=3;

	vd_CalculerEtatConsigne(st_chauf_Zsdb1, &uc_Buffer_Status[l_us_Nb_Octets]);
	l_us_Nb_Octets+=3;
	vd_CalculerEtatConsigne(st_chauf_Zsdb2, &uc_Buffer_Status[l_us_Nb_Octets]);
	l_us_Nb_Octets+=3;

	// alarme
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) Tb_Echange[Alarme_Mode];
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) Tb_Echange[Alarme_SuiviAlarme];
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) Tb_Echange[Alarme_CompteARebours];
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) Tb_Echange[Alarme_Detection];
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = (unsigned char) Tb_Echange[Alarme_Fraude];
	l_us_Nb_Octets++;

	// alerte
	if(Tb_Echange[Securite_FuiteLinge] != 0 || Tb_Echange[Securite_FuiteVaisselle] != 0)	uc_Buffer_Status[l_us_Nb_Octets] = 1;	
	else																					uc_Buffer_Status[l_us_Nb_Octets] = 0;	
	l_us_Nb_Octets++;
	uc_Buffer_Status[l_us_Nb_Octets] = 0;
	if(st_EchangeStatus.uc_FuiteLL != 0)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x01;		
	if(st_EchangeStatus.uc_FuiteLV != 0)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x02;		
	l_us_Nb_Octets++;

	// Divers :
	//	bit 0		Fuite lave-linge (NON=0/OUI=1)									
	//	bit 1		Fuite lave-vaisselle (NON=0/OUI=1)									
	//	bit 2		Détection pluie (NON=0/OUI=1)									
	//	bit 3		Délestage (NON=0/OUI=1)									
	//	bit 4		Secouru (NON=0/OUI=1)									
	//	bit 5		Arrosage (OFF=0/ON=1)									
	//	bit 6		Machines à laver (OFF=0/ON=1)									
	//	bit 7		Prises de sécurité (OFF=0/ON=1)									
	uc_Buffer_Status[l_us_Nb_Octets] = 0;
	if(st_EchangeStatus.uc_FuiteLL != 0)									uc_Buffer_Status[l_us_Nb_Octets] |= 0x01;
	if(st_EchangeStatus.uc_FuiteLV != 0)									uc_Buffer_Status[l_us_Nb_Octets] |= 0x02;
	if(lwgpio_get_value(&IO_DIN_Detection_Pluie) != LWGPIO_VALUE_LOW)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x04;
	if(st_EchangeStatus.uc_SurConsommation != 0)							uc_Buffer_Status[l_us_Nb_Octets] |= 0x08;
	if(st_EchangeStatus.uc_Secouru != 0)									uc_Buffer_Status[l_us_Nb_Octets] |= 0x10;
	if(lwgpio_get_value(&IO_DOUT_VanneArrosage) == LWGPIO_VALUE_HIGH)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x20;
	if(lwgpio_get_value(&IO_DOUT_MachineALaver) == LWGPIO_VALUE_HIGH)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x40;
	if(lwgpio_get_value(&IO_DOUT_PriseSecurite) == LWGPIO_VALUE_HIGH)		uc_Buffer_Status[l_us_Nb_Octets] |= 0x80;
	l_us_Nb_Octets++;
	
	uc_Buffer_Status[l_us_Nb_Octets] = uc_fct_Calcul_CheckSum(uc_Buffer_Status+1, l_us_Nb_Octets-1);
	l_us_Nb_Octets++;

	if (l_us_Nb_Octets > us_TAILLE_BUFFER_STATUS)
	{
		DETECTION_ERREUR_TACHE_ECRAN_PB_TAILLE_TRAME_STATUS;
	}

	// Envoi de la trame...
	vd_Screen_write(fd, uc_Buffer_Status, l_us_Nb_Octets);
	ul_EspionTacheEcranNbTramesStatus++;
}

// Writes the provided data buffer at screen
void vd_Screen_write
(
	MQX_FILE_PTR fd,		// [IN] The file pointer for the I2C channel 
	uchar_ptr  buffer,		// [IN] The array of characters are to be written
	uint_16    length		// [IN] Number of bytes in that buffer     
)
{
	int_32 l_ul_result;
	
	// Protect transaction in multitask environment 
	_lwsem_wait(&st_LW_lock_screen);
	
	// write data
	// pour trames longues avec taille > pile du driver : les appels suivants passent le reste de la trame
	l_ul_result =0;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TX (hexa) : ");
	if(uc_DownloadEnCours == 0 && uc_BloquerEmissionVersEcran == 0)	// Pas de dialogue écran pendant telechargement
	{
		do 
		{
			//xxxl_ul_result += write( fd, buffer + l_ul_result , length - l_ul_result);
			if(write( fd, buffer + l_ul_result , 1) != 0) // Envoie caractère par caractère (idem ligne précédente mais plus simple à comprendre pour moi :-) CPA
			// Retourne 1 si octet envoyé ou 0 s'il n'a pas pu etre envoyé (buffer émission plein) -> reemission au coup d'apres...
			{
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE,"%02X ",buffer[l_ul_result]);
				l_ul_result++;
			}
		}while (l_ul_result < length);
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE," FIN TX\n");
	}
	else
	{
		if(uc_BloquerEmissionVersEcran == 0)		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TX BLOQUE PAR DOWNLOAD\n");
		else										vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TX BLOQUE PAR ESPION RS\n");
	}
	
	// empty queue
    l_ul_result = fflush(fd);
	if(l_ul_result != MQX_OK)
	{
		DETECTION_ERREUR_TACHE_ECRAN_TX_RS;
	}
	// wait for transfer complete flag
	ioctl( fd, IO_IOCTL_SERIAL_WAIT_FOR_TC, NULL);	// xxx tester retour
	// half duplex, two wire
      
	us_EcranNbOctetsRecus = 0;		// Init pour réception
	
	// Release the transaction lock
	_lwsem_post (&st_LW_lock_screen);
}

// Lit les octets reçus sur la RS de l'écran
void vd_RS_Screen_read(MQX_FILE_PTR fd)
{
	unsigned char l_uc_Caractere;
	unsigned char l_uc_Reception;
	
	
	// Lecture du buffer de reception MQX (fonction non bloquante)
	l_uc_Reception = 0;
	while(read(fd, &l_uc_Caractere, 1) != 0 && us_EcranNbOctetsRecus < us_TAILLE_BUFFER_RX)
	{
		if(l_uc_Reception == 0)	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"RX (Hexa) : ");
		l_uc_Reception = 1;
		uc_EcranBufferRx[us_EcranNbOctetsRecus] = l_uc_Caractere;
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE,"%02X ", l_uc_Caractere);
		us_EcranNbOctetsRecus++;
		ul_EspionTacheEcranNbOctetsRecus++;
	}
	if(us_EcranNbOctetsRecus >= us_TAILLE_BUFFER_RX)
	{
		DETECTION_ERREUR_TACHE_ECRAN_SATURATION_RX_ECRAN;
		us_EcranNbOctetsRecus = 0;
	}
	
	// Gestion time out
	if(l_uc_Reception != 0)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ECRAN_ACTIVITE," FIN RX\n");
		vd_RestartTimerEcranTimeOUT();
		uc_EcranTimeOUT = 0;
	}
	if(uc_EcranTimeOUT != 0)
	{
		if(us_EcranNbOctetsRecus != 0)	ul_EspionTacheEcranNbRAZBufferReceptionSurTimeOUT++;
		us_EcranNbOctetsRecus = 0;	// Time out -> RAZ buffer réception
		st_EchangeStatus.uc_EcranTimeOUT = 1;
	}
	else
	{
		st_EchangeStatus.uc_EcranTimeOUT = 0;
	}
}

void vd_DecalerBufferReception(void)
{
	unsigned short l_us_Compteur;
	
	if(us_EcranNbOctetsRecus > 0)
	{
		us_EcranNbOctetsRecus--;
		for(l_us_Compteur = 0; l_us_Compteur < (us_TAILLE_BUFFER_RX-1); l_us_Compteur++)
		{
			uc_EcranBufferRx[l_us_Compteur] = uc_EcranBufferRx[l_us_Compteur+1];
		}
	}
}

// Analyse des octets recus
void vd_AnalyserOctetsRecus(MQX_FILE_PTR fd)
{
	unsigned short l_us_TailleTrameAttendue;
	
	
	l_us_TailleTrameAttendue = 0;
	if(us_EcranNbOctetsRecus > 1)	// On traite le 1er octet (entête trame) et selon les cas aussi le 2eme octet (nb octets trames)
	{								// Pas genant d'attendre 2 octets car toutes les trames font au minimum plus de 2 octets
		// qualifier et octet suivant reçus : calcul la longueur de la trame attendue
		switch(uc_EcranBufferRx[0])
		{
			case 0x0F:		// trame synchro (au demarrage de l'écran)
				l_us_TailleTrameAttendue = (unsigned short)5;
			break;
			case 0xF0 :		// demande de status
				l_us_TailleTrameAttendue = (unsigned short)3;
			break;
			case 0x55 :		// data discrete read
				l_us_TailleTrameAttendue = (unsigned short)uc_EcranBufferRx[1] * 2 + 3;		// 2n+3
			break;
			case 0x5A :		// data discrete write
				l_us_TailleTrameAttendue = (unsigned short)uc_EcranBufferRx[1] * 3 + 3; 	// 3n+3
			break;
			case 0xA5 :		// data block read
				l_us_TailleTrameAttendue = 5;
			break;
			case 0xAA :		// data block write
				l_us_TailleTrameAttendue = (unsigned short)uc_EcranBufferRx[1] + 5;
			break;
			default :		// debut trame incorrecte -> élimine le déja reçu (attend début de trame)
				//us_EcranNbOctetsRecus = 0;
				ul_EspionTacheEcranEnteteTrameIncorrecte++;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"PB DEBUT TRAME : %d -> IGNORE !\n", uc_EcranBufferRx[0]);
				vd_DecalerBufferReception();	// Entête incorrecte -> décalage de 1 octet le buffer de réception -> la suite peut contenir une trame valide
			break;
		}
	}
	
	// Trame complete recue : analyse contenue et traitement
	if(us_EcranNbOctetsRecus >= l_us_TailleTrameAttendue && l_us_TailleTrameAttendue > 0)
	{
		if(uc_fct_Calcul_CheckSum(uc_EcranBufferRx, l_us_TailleTrameAttendue-1) != uc_EcranBufferRx[l_us_TailleTrameAttendue-1])
		{
			ul_EspionTacheEcranNbTramesIncorrectesRecues++;
			vd_DecalerBufferReception();	// CRC incorrect -> décalage de 1 octet le buffer de réception -> la suite peut contenir une trame valide
		}
		else
		{
			ul_EspionTacheEcranNbTramesCorrectesRecues++;
			
			switch (uc_EcranBufferRx[0])	// gestion trame selon code reçu
			{
				case 0x0F:		// trame synchro (au demarrage de l'écran)
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME SYNCHRO %d\n", uc_EcranBufferRx[3]);
					vd_EnvoyerTrameSynchro(fd);
				break;
				case 0xF0 :		// demande de status
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME DEMANDE STATUS RECUE\n");
					if(uc_EcranBufferRx[1] == 0x55)
					{
						st_EchangeStatus.uc_DinEcranOuvert = 0;
					}
					else
					{
						st_EchangeStatus.uc_DinEcranOuvert = 1;
					}
					vd_EnvoyerTrameStatus(fd);
				break;
				case 0x55 :		// data discrete read
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME LECTURE DONNEES DISCRETES\n");
					vd_EnvoyerTrameLectureDonneesDiscretes(fd);
				break;
				case 0x5A :		// data discrete write
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME ECRITURE DONNEES DISCRETES\n");
					vd_EnvoyerTrameEcritureDonneesDiscretes(fd);
				break;
				case 0xA5 :		// data block read
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME LECTURE DATA BLOCK\n");
					vd_EnvoyerTrameLectureDonneesBloc(fd);
				break;
				case 0xAA :		// data block write
					vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"TRAME ECRITURE DATA BLOCK\n");
					vd_EnvoyerTrameEcritureDonneesBloc(fd);
				break;
				default:
					DETECTION_ERREUR_TACHE_ECRAN_DEMANDE_ECRAN_INCORRECTE;
				break;
			}			
			us_EcranNbOctetsRecus = 0;	// Dans tous les cas, RAZ du buffer de réception
		}
	}
}

void vd_EnvoyerTrameSynchro(MQX_FILE_PTR fd)
{
	uc_Buffer_Tx[0] = 0x0F;
	uc_Buffer_Tx[1] = Tb_Echange[Version_SoftBP_Embedded];
	uc_Buffer_Tx[2] = Tb_Echange[Version_SoftBP_Web];
	
	// Recuperation versions ecran
	Tb_Echange[Version_SoftIHM_Majeur] = uc_EcranBufferRx[1];
	Tb_Echange[Version_SoftIHM_Mineur] = uc_EcranBufferRx[2];

	if(uc_EcranBufferRx[3] == 0x01)
	{
		uc_Buffer_Tx[3] = 0x02;
		ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur1++;
		
		// Gestion inversion
		if(uc_TrameSynchroEtat == 1)
		{
			// On a déjà reçu la trame de demande de synchro : l'écran n'a pas recu notre reponse -> inversion sens écran
			lwgpio_toggle_value(&IO_DOUT_EcranDirection);
		}
		uc_TrameSynchroEtat = 1;
	}
	else
	{
		uc_Buffer_Tx[3] = 0x04;
		ul_EspionTacheEcranNbTrameSynchroRecueAvecValeur3++;
		uc_TrameSynchroEtat = 0;
	}
	uc_Buffer_Tx[4] = uc_fct_Calcul_CheckSum(uc_Buffer_Tx, 4);

	vd_Screen_write(fd, uc_Buffer_Tx, 5);
}

void vd_EnvoyerTrameLectureDonneesDiscretes(MQX_FILE_PTR fd)     
{
	unsigned char l_uc_NbOctetsDemandes;
	unsigned char l_uc_Compteur;
	unsigned short l_us_Indice;

	
	l_uc_NbOctetsDemandes = uc_EcranBufferRx[1];			// Nombre de data demandées
	if((l_uc_NbOctetsDemandes+2) > us_TAILLE_BUFFER_TX)
	{	// sécurité : buffer trop petit
		DETECTION_ERREUR_TACHE_ECRAN_SATURATION_TX_ECRAN;
		
		// limite le nombre de données émises ou refuser toute la trame avec un code d'erreur
		l_uc_NbOctetsDemandes = (unsigned char)(us_TAILLE_BUFFER_TX-2);
	}

	// traitement lecture
	for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbOctetsDemandes;l_uc_Compteur++)
	{
		l_us_Indice = ((unsigned short) uc_EcranBufferRx[(l_uc_Compteur*2)+3] << 8) + (unsigned short) uc_EcranBufferRx[(l_uc_Compteur*2)+2];
		if (l_us_Indice < Nb_Tbb_Donnees)
		{
			uc_Buffer_Tx[1+l_uc_Compteur] = uc_TableEchange_Lit_Data(l_us_Indice);
			//xxx rajouter code erreur si data non accessible dans uc_TableEchange_Lit_Data
		}
		else
		{	// sécurité : donnée inexistante
			uc_Buffer_Tx[1+l_uc_Compteur] = 0;
			DETECTION_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT;
		}
	}
	
	// trame de réponse
	uc_Buffer_Tx[0] = 0x55;
	uc_Buffer_Tx[l_uc_NbOctetsDemandes+1] = uc_fct_Calcul_CheckSum(uc_Buffer_Tx, l_uc_NbOctetsDemandes+1);
	
	vd_Screen_write(fd, uc_Buffer_Tx, l_uc_NbOctetsDemandes+2);
	ul_EspionTacheEcranNbTramesLectureDiscrete++;
}

void vd_EnvoyerTrameEcritureDonneesDiscretes(MQX_FILE_PTR fd)     
{
	unsigned char l_uc_NbOctetsAEcrire;
	unsigned char l_uc_Compteur;
	unsigned short l_us_Indice;

	
	l_uc_NbOctetsAEcrire = uc_EcranBufferRx[1];			// Nombre de data demandées
	//xxx verifier taille trame correcte par rapport a l_uc_NbOctetsAEcrire

	// traitement écriture
	for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbOctetsAEcrire;l_uc_Compteur++)
	{
		l_us_Indice = ((unsigned short) uc_EcranBufferRx[(l_uc_Compteur*3)+3] << 8) + (unsigned short) uc_EcranBufferRx[(l_uc_Compteur*3)+2];
		if (l_us_Indice < Nb_Tbb_Donnees)
		{
			uc_TableEchange_Ecrit_Data(l_us_Indice, uc_EcranBufferRx[(l_uc_Compteur*3)+4], 0);
			//xxx tester retour uc_TableEchange_Ecrit_Data
		}
		else
		{	// sécurité : donnée inexistante
			DETECTION_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT;
		}
	}
			
	// trame de réponse
	uc_Buffer_Tx[0] = 0x5A;
	uc_Buffer_Tx[1] = 0xA5;
	uc_Buffer_Tx[2] = uc_fct_Calcul_CheckSum(uc_Buffer_Tx, 2);
	
	vd_Screen_write(fd, uc_Buffer_Tx, 3);
	ul_EspionTacheEcranNbTramesEcritureDiscrete++;
}

void vd_EnvoyerTrameLectureDonneesBloc(MQX_FILE_PTR fd)     
{
	unsigned char l_uc_NbOctetsDemandes;
	unsigned char l_uc_Compteur;
	unsigned short l_us_Indice;

	
	l_uc_NbOctetsDemandes = uc_EcranBufferRx[1];			// Nombre de data demandées
	if((l_uc_NbOctetsDemandes+2) > us_TAILLE_BUFFER_TX)
	{	// sécurité : buffer trop petit
		DETECTION_ERREUR_TACHE_ECRAN_SATURATION_TX_ECRAN;
		
		// limite le nombre de données émises ou refuser toute la trame avec un code d'erreur
		l_uc_NbOctetsDemandes = (unsigned char)(us_TAILLE_BUFFER_TX-2);
	}

	// traitement lecture
	l_us_Indice = ((unsigned short) uc_EcranBufferRx[3] << 8) + (unsigned short) uc_EcranBufferRx[2];
	for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbOctetsDemandes;l_uc_Compteur++)
	{
		if (l_us_Indice < Nb_Tbb_Donnees)
		{
			uc_Buffer_Tx[1+l_uc_Compteur] = uc_TableEchange_Lit_Data(l_us_Indice);
			//xxx rajouter code erreur si data non accessible dans uc_TableEchange_Lit_Data
		}
		else
		{	// sécurité : donnée inexistante
			uc_Buffer_Tx[1+l_uc_Compteur] = 0;
			DETECTION_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT;
		}
		l_us_Indice++;
	}
	
	// trame de réponse
	uc_Buffer_Tx[0] = 0xA5;
	uc_Buffer_Tx[l_uc_NbOctetsDemandes+1] = uc_fct_Calcul_CheckSum(uc_Buffer_Tx, l_uc_NbOctetsDemandes+1);
	
	vd_Screen_write(fd, uc_Buffer_Tx, l_uc_NbOctetsDemandes+2);
	ul_EspionTacheEcranNbTramesLectureBloc++;
}

void vd_EnvoyerTrameEcritureDonneesBloc(MQX_FILE_PTR fd)     
{
	unsigned char l_uc_NbOctetsAEcrire;
	unsigned char l_uc_Compteur;
	unsigned short l_us_Indice;

	
	l_uc_NbOctetsAEcrire = uc_EcranBufferRx[1];			// Nombre de data demandées
	//xxx verifier taille trame correcte par rapport a l_uc_NbOctetsAEcrire

	// traitement écriture
	l_us_Indice = ((unsigned short) uc_EcranBufferRx[3] << 8) + (unsigned short) uc_EcranBufferRx[2];
	for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbOctetsAEcrire;l_uc_Compteur++)
	{
		if (l_us_Indice < Nb_Tbb_Donnees)
		{
			uc_TableEchange_Ecrit_Data(l_us_Indice, uc_EcranBufferRx[l_uc_Compteur+4], 0);
			//xxx tester retour uc_TableEchange_Ecrit_Data
		}
		else
		{	// sécurité : donnée inexistante
			DETECTION_ERREUR_TACHE_ECRAN_ACCES_DATA_ECRAN_INCORRECT;
		}
		l_us_Indice++;
	}
			
	// trame de réponse
	uc_Buffer_Tx[0] = 0xAA;
	uc_Buffer_Tx[1] = 0x55;
	uc_Buffer_Tx[2] = uc_fct_Calcul_CheckSum(uc_Buffer_Tx, 2);
	
	vd_Screen_write(fd, uc_Buffer_Tx, 3);
	ul_EspionTacheEcranNbTramesEcritureBloc++;
}

void vd_Ecran_task(uint_32 dummy)
{
	boolean l_b_disable_rx = FALSE;
	signed long l_l_param, l_l_result;
	MQX_FILE_PTR l_fd;		// RS Ecran xxx
	_mqx_uint l_uint_Retour;
	

	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"INIT\n");
	uc_EspionTacheEcranEtat = 1;
	
	_lwsem_create (&st_LW_lock_screen, 1);	// screen transaction lock //xxx tester retour

	// Ouverture RS ECRAN
	uc_EspionTacheEcranEtat = 2;
	l_fd = fopen(ECRAN_DEVICE_INTERRUPT, ( char const * )(IO_SERIAL_NON_BLOCKING));
	if(l_fd == NULL) 
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"Error opening uart driver!\n");
		DETECTION_ERREUR_TACHE_ECRAN_RS_ECRAN;
		_task_block();
	}

	uc_EspionTacheEcranEtat = 3;
	l_l_param = ECRAN_BAUD_RATE;
	if(ioctl(l_fd, IO_IOCTL_SERIAL_SET_BAUD, &l_l_param) != MQX_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"Error baud rate modification!\n");
		DETECTION_ERREUR_TACHE_ECRAN_CONFIG_VITESSE_RS;
	   _task_block();
	}

	us_EcranNbOctetsRecus = 0;		// init pour réception

	l_b_disable_rx = FALSE;
	ioctl(l_fd, IO_IOCTL_SERIAL_DISABLE_RX, &l_b_disable_rx);		// en réception par défaut au démarrage

	uc_EspionTacheEcranEtat = 4;
	vd_RestartTimerEcranTimeOUT();
	uc_EcranTimeOUT = 1;	// Forcer init buffer tant que un caractere n'est pas recu

	// tache de fond permanente
	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"START\n");
	uc_EspionTacheEcranEtat = 5;
	
	
	for(;;)
	{
		ul_EspionTacheEcranCompteurActivite++;
		
		ul_CompteurMutexTacheEcran++;
		l_uint_Retour = _mutex_lock(&st_Mutex_DateHeurePourEcran);
		if(l_uint_Retour != MQX_EOK)
		{
			ul_CompteurMutexTacheEcranErreur++;
			printf("x1 %d\n", l_uint_Retour);
			printf("X1XXXXXXXXXXXXXXXX PB MUTEX XXXXXXXXXXXXXXXXXXXXXXXX\n");
			DETECTION_ERREUR_TACHE_ECRAN_MUTEX;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"Error mutex tache ecran!\n");
		}

		_mutex_unlock(&st_Mutex_DateHeurePourEcran);	// Libération...
		
		// Reception des octets provenant de l'écran et traitement des requetes
		uc_EspionTacheEcranEtat = 6;
		uc_FlagMajDate = 0;
		vd_RS_Screen_read(l_fd);
		uc_EspionTacheEcranEtat = 7;
		vd_AnalyserOctetsRecus(l_fd);	// Positionne uc_Flag_Maj_Date
		uc_EspionTacheEcranEtat = 8;
		
		if(uc_FlagMajDate != 0)
		{			// gestion spécifique sur mise à jour de la date et heure
					// pour une prise en compte globale de l'ensemble des éléments
					// en une seule fois...
			uc_FlagMajDate = 0;
			_lwsem_post(&lwsem_wr_date); 
		}
		uc_EspionTacheEcranEtat = 9;
		_sched_yield();		// passe la main à la tache suivante
		uc_EspionTacheEcranEtat = 10;
	}
	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"STOP\n");
	uc_EspionTacheEcranEtat = 11;
	
	/* Close the driver */
	l_l_result = fclose(l_fd);
	if(l_l_result)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ERREUR,"Error close rs : %d\n", l_l_result);
	}
	
	uc_EspionTacheEcranEtat = 12;
	_lwsem_destroy(&st_LW_lock_screen);

	vd_EspionRS_Printf(uc_ESPION_TACHE_ECRAN_ACTIVITE,"END\n");
	uc_EspionTacheEcranEtat = 13;
	_task_block();
}

void vd_RestartTimerEcranTimeOUT(void)
{
	static _timer_id s_TimerID = TIMER_NULL_ID;
	MQX_TICK_STRUCT l_ticks;

	_time_init_ticks(&l_ticks, 0);
	_time_add_msec_to_ticks(&l_ticks, us_ECRAN_TIME_OUT_ms);

	if(s_TimerID != TIMER_NULL_ID)
	{
		// Cancel timer en cours
		if(_timer_cancel(s_TimerID) != MQX_OK)
		{
			DETECTION_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_CANCEL;
		}
	}
	
	// Creation timer
	s_TimerID = _timer_start_periodic_every_ticks(vd_EcranTimeOUT, 0, TIMER_ELAPSED_TIME_MODE, &l_ticks);	
	if(s_TimerID == TIMER_NULL_ID)
	{
		DETECTION_ERREUR_TACHE_ECRAN_PB_TIMER_TIME_OUT_INIT;
	}
}

void vd_EcranTimeOUT(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr)
{
	ul_EspionTacheEcranNbDeclenchementTimeOUT++;
	uc_EcranTimeOUT = 1;
}

// xxx DIn ecran 0x55 pas ouvert - 0xaa ouvert
