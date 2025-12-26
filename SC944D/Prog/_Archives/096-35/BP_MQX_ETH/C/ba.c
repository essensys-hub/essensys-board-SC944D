#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx

#include <i2c.h>

#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "EspionRS.h"
#include "ba_i2c.h"
#include "global.h"
#include "crc.h"

#define extern
#include "ba.h"
#undef extern


// Variables locales utilisées par le module

// Contient les informations sur les BA de la table d'échange
unsigned char uc_Tb_Borniers_Data[Scenario - Variateurs_PDV_Conf];		// la taille doit correspondre à la zone mémoire de la table d'échange !

// Gestion des émissions / répétitions
typedef struct {
	unsigned short	b_Forcage		:1;		// 1= message à émettre
	unsigned short	b_Conf			:1;		// 1= message à émettre
	unsigned short	b_Extinction	:1;		// 1= message à émettre
	unsigned short	b_TpsAction		:1;		// 1= message à émettre
	unsigned short	b_Action		:1;		// 1= message à émettre
	unsigned short	b_nu3			:1;		// non utilisé
	unsigned short	b_nu2			:1;		// non utilisé
	unsigned short	b_nu1			:1;		// non utilisé
	unsigned short	o_Repete		:8;		// compteur de répétitions ou d'erreurs
}tst_Com_BA;
tst_Com_BA Tb_Com_BA[uc_NB_BOITIER_AUXILIAIRE];

// Contient les actions à envoyer - ORDRE champs -> ORDRE I2C - Ne pas modifier !!!
typedef union {
	struct {
		unsigned char	nu1				:1;		// non utilisé
		unsigned char	nu2				:1;		// non utilisé
		unsigned char	nu3				:1;		// non utilisé
		unsigned char	nu4				:1;		// non utilisé
		unsigned char	nu5				:1;		// non utilisé
		unsigned char	ForceAllumage	:1;		// 1= allumage forcé
		unsigned char	BloqueVolets	:1;		// 1= blocage des volets
		unsigned char	Secouru			:1;		// 1= mode secouru
	}b;
	unsigned char	Octet;
}Actions_BA;
Actions_BA Tb_Actions_Cde_BA[uc_NB_BOITIER_AUXILIAIRE];			// actions en cours pour chaque bornier
Actions_BA Tb_Actions_Ack_BA[uc_NB_BOITIER_AUXILIAIRE];			// actions acquittées pour chaque bornier


void Boitiers_task(uint_32 dummy)
{
	signed long l_sl_param;
	_queue_id l_BA_qid;
	unsigned short l_us_i, l_us_j, l_us_k, l_us_l;
	unsigned char l_uc_buffer[uc_I2C_BUFFER_SIZE];
	signed long l_sl_result;
	unsigned char l_uc_ReponseRecue;	// xxx a tester apres chaque requete -> regarder ce que ba peut renvoyer la dedans
	static COMMANDE_MESSAGE_PTR	s_msg_ptr = NULL;
	unsigned char l_uc_Compteur;
	unsigned short l_us_CRC;
	

	uc_EspionTacheBAEtat = 1;
	vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"INIT\n");
	
	// I2C transaction lock 
	_lwsem_create(&lock_I2C, 1);	// xxx tester retour

	// Open the I2C driver          
	fd_BA_I2C = fopen(I2C_DEVICE_POLLED, NULL);
	if(fd_BA_I2C == NULL) 
	{
		DETECTION_ERREUR_TACHE_BA_OUVERTURE_I2C;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur driver I2C!\n");
		_task_block ();
	}

	uc_EspionTacheBAEtat = 2;
	l_sl_param = 50000;
	if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_SET_BAUD, &l_sl_param) != I2C_OK)
	{
		DETECTION_ERREUR_TACHE_BA_CONFIG_VITESSE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur config vitesse!\n");
		_task_block ();
	}

	// BA: Set master mode
	uc_EspionTacheBAEtat = 3;
	if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_SET_MASTER_MODE, NULL) != I2C_OK)
	{
		DETECTION_ERREUR_TACHE_BA_CONFIG_MAITRE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur config master!\n");
		_task_block ();
	}

	uc_EspionTacheBAEtat = 4;
	l_sl_param = I2C_BUS_ADDRESS_BP;	// adresse du boitier principal
	if(ioctl(fd_BA_I2C, IO_IOCTL_I2C_SET_STATION_ADDRESS, &l_sl_param) != I2C_OK)
	{
		DETECTION_ERREUR_TACHE_BA_CONFIG_ADRESSE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur config adresse!\n");
		_task_block ();
	}

	// Open a message queue
	uc_EspionTacheBAEtat = 5;
	l_BA_qid = _msgq_open(BA_QUEUE_ID, 0);
	if(l_BA_qid == MSGQ_NULL_QUEUE_ID)
	{
		DETECTION_ERREUR_TACHE_BA_CREATION_MESSAGE_QUEUE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur open queue!\n");
		_task_block ();
	}	

	// Init des données à partir de la table d'échange
	// Recopie les valeurs de la table d'échange dans le tableau utilisé par la gestion du dialogue avec les BA
	uc_EspionTacheBAEtat = 6;
	for(l_us_i=0, l_us_j=Variateurs_PDV_Conf; l_us_j < Scenario ; l_us_i++, l_us_j++)
	{
		uc_Tb_Borniers_Data[l_us_i] = Tb_Echange[l_us_j];
	}
	// Init des variables
	for(l_us_i=0; l_us_i < uc_NB_BOITIER_AUXILIAIRE ; l_us_i++)
	{
		Tb_Com_BA[l_us_i].b_Forcage = 0;
		Tb_Com_BA[l_us_i].b_Conf = 0;
		Tb_Com_BA[l_us_i].b_Extinction = 0;
		Tb_Com_BA[l_us_i].b_TpsAction = 0;
		Tb_Com_BA[l_us_i].b_Action = 0;
		Tb_Com_BA[l_us_i].o_Repete = 0;
		
		Tb_Actions_Cde_BA[l_us_i].Octet = 0;
		Tb_Actions_Ack_BA[l_us_i].Octet = 0;
	}

	// tache de fond permanente
	// Assure l'envoi / réémission des valeurs aux BA en cas de changement de valeur
	uc_EspionTacheBAEtat = 7;
	vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"START\n");
	
	for (;;)
	{
		ul_EspionTacheBACompteurActivite++;
		//_time_delay(100);//zzz
		
		// Emission des actions
		uc_EspionTacheBAEtat = 8;
		for (l_us_i=0; l_us_i<uc_NB_BOITIER_AUXILIAIRE; l_us_i++)	// boucle sur tous les boitiers
		{
			Tb_Actions_Cde_BA[l_us_i].b.Secouru = 0;
			if(st_EchangeStatus.uc_Secouru != 0 || uc_FlagResetBP != 0)		// mode secouru => informer les boitiers pour qu'ils fassent leur sauvegarde
			{																// ou Reset BP programmé => sauvegarde état BA
				Tb_Actions_Cde_BA[l_us_i].b.Secouru = 1;
			}
			Tb_Actions_Cde_BA[l_us_i].b.BloqueVolets = st_EchangeStatus.uc_BloquerVolets;		// blocage des volets
			Tb_Actions_Cde_BA[l_us_i].b.ForceAllumage = st_EchangeStatus.uc_ForcerAllumage;		// allumage forcé
			if(us_CompteurPilotageSireneInterieure_sec >= us_DUREE_PILOTAGE_SIRENE_INTERIEURE_sec)	// Suivre le pilotage de la sirène intérieure
			{
				Tb_Actions_Cde_BA[l_us_i].b.ForceAllumage = 0;
			}

			if (Tb_Actions_Cde_BA[l_us_i].Octet != Tb_Actions_Ack_BA[l_us_i].Octet)	// Envoi si valeur différente que celle précédemment envoyée
			{
				// au moins 1 action à faire
				Tb_Com_BA[l_us_i].b_Action = 1;
				l_uc_buffer[0] = uc_TRAME_BA_ACTIONS;							// Code de la trame
				l_uc_buffer[1] = Tb_Actions_Cde_BA[l_us_i].Octet;				// Data
				//				l_uc_buffer[2] = us_CalculerCRCSurTrame(l_uc_buffer, 1);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, 2);	// Checksum
				//				printf("xxx %d\n", l_uc_buffer[2]);
				l_us_CRC = us_CalculerCRCSurTrame(l_uc_buffer, 2);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, 2);	// Checksum
				l_uc_buffer[2] = (l_us_CRC & 0xFF);
				l_uc_buffer[3] = ((l_us_CRC>>8) & 0xFF);
				//				printf("xxx %d\n", l_uc_buffer[2]);
								
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA : envoi action au boitier %i (%i) -> %d.%d.%d.%d\n", l_us_i, Tb_Com_BA[l_us_i].o_Repete, l_uc_buffer[0], l_uc_buffer[1], l_uc_buffer[2], l_uc_buffer[3]);//zzz rajouter trame
				ul_EspionTacheBACompteurEmissionI2C[l_us_i]++;

				l_sl_result = sl_fct_write_polled(fd_BA_I2C, (uchar) (l_us_i), l_uc_buffer, 4, &l_uc_ReponseRecue);
				if(l_sl_result == I2C_OK)
				{
					// trame transmise et acquittée : pas de répétition nécessaire
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission OK\n");
					Tb_Actions_Ack_BA[l_us_i].Octet = Tb_Actions_Cde_BA[l_us_i].Octet;	
					Tb_Com_BA[l_us_i].b_Action = 0;
					Tb_Com_BA[l_us_i].o_Repete = 0;		// raz compteur de repetitions
				}
				else
				{
					// erreurs => répétition
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission PB %d\n", l_sl_result);
					ul_EspionTacheBACompteurErreurI2C[l_us_i]++;
					Tb_Com_BA[l_us_i].o_Repete++;		// compteur de repetitions
					if(Tb_Com_BA[l_us_i].o_Repete > uc_I2C_NB_REPETE)
					{
						// lorsque le nombre max de répétition est atteint, acquitte la demande pour arrêter les répétitions
						// trame en erreur : arrêt répétition
						Tb_Actions_Ack_BA[l_us_i].Octet = Tb_Actions_Cde_BA[l_us_i].Octet;
						Tb_Com_BA[l_us_i].b_Action = 0;
						Tb_Com_BA[l_us_i].o_Repete = uc_I2C_NB_REPETE+1;		// pour éviter les débordements
					}
				}
			}
		}
		
		// Gestion forcage
		uc_EspionTacheBAEtat = 9;
		if(s_msg_ptr == NULL)
		{
			// pas de message en attente de traitement (prise en compte des répétitions)
			// regarde si un message dans la pile
			s_msg_ptr = _msgq_poll(l_BA_qid);
			if(s_msg_ptr != NULL)
			{
				ul_EspionTachePrincipaleNbMessagesBARecus++;
			}
		}
		uc_EspionTacheBAEtat = 10;
		if(s_msg_ptr != NULL)
		{
			uc_EspionTacheBAEtat = 201;
			// message reçu
			l_us_i = s_msg_ptr->DATA[0];
			Tb_Com_BA[l_us_i].b_Forcage = 1;
			uc_EspionTacheBAEtat = 202;
			// au moins 1 forçage à faire
			l_uc_buffer[0] = uc_TRAME_BA_FORCAGE_SORTIES;		// Code de la trame
			for(l_us_j=1; l_us_j < 9; l_us_j++)
			{
				l_uc_buffer[l_us_j] = s_msg_ptr->DATA[l_us_j];
			}
			uc_EspionTacheBAEtat = 203;
//			l_uc_buffer[9] = us_CalculerCRCSurTrame(l_uc_buffer, 7);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, 9);
//			printf("xxx %d\n", l_uc_buffer[9]);
//			l_uc_buffer[9] = us_CalculerCRCSurTrame(l_uc_buffer, 8);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, 9);
//			printf("xxx %d\n", l_uc_buffer[9]);
			l_us_CRC = us_CalculerCRCSurTrame(l_uc_buffer, 9);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, 9);
			l_uc_buffer[9] = (l_us_CRC & 0xFF);
			l_uc_buffer[10] = ((l_us_CRC>>8) & 0xFF);
//			printf("xxx %d\n", l_uc_buffer[9]);

			uc_EspionTacheBAEtat = 204;

			vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA : Forçage sorties -> BA %i (%i) -> %d", l_us_i, Tb_Com_BA[l_us_i].o_Repete, l_uc_buffer[0]);
			for(l_uc_Compteur = 1 ; l_uc_Compteur < 11; l_uc_Compteur++)
			{
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,".%d", l_uc_buffer[l_uc_Compteur]);
			}
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,"\n");
			ul_EspionTacheBACompteurEmissionI2C[l_us_i]++;

			uc_EspionTacheBAEtat = 205;
			l_sl_result = sl_fct_write_polled(fd_BA_I2C, (uchar) (l_us_i), l_uc_buffer, (l_us_j+2), &l_uc_ReponseRecue);
			uc_EspionTacheBAEtat = 206;
			if(l_sl_result == I2C_OK)
			{
				uc_EspionTacheBAEtat = 207;
				// trame transmise et acquittée : pas de répétition nécessaire
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission OK\n");
				Tb_Com_BA[l_us_i].b_Forcage = 0;		// trame transmise et acquittée : pas de répétition nécessaire
				Tb_Com_BA[l_us_i].o_Repete = 0;		// raz compteur de repetitions
				uc_EspionTacheBAEtat = 208;
				// libère message sur réception réponse OK
				_msg_free(s_msg_ptr);
				uc_EspionTacheBAEtat = 209;
				s_msg_ptr = NULL;
			}
			else
			{
				uc_EspionTacheBAEtat = 210;
				// erreurs => répétition
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission PB %d\n", l_sl_result);
				ul_EspionTacheBACompteurErreurI2C[l_us_i]++;
				uc_EspionTacheBAEtat = 211;
				Tb_Com_BA[l_us_i].o_Repete++;		// compteur de repetitions
				if(Tb_Com_BA[l_us_i].o_Repete > uc_I2C_NB_REPETE)
				{
					uc_EspionTacheBAEtat = 212;
					// lorsque le nombre max de répétition est atteint, acquitte la demande pour arrêter les répétitions
					// trame en erreur : arrêt répétition
					Tb_Com_BA[l_us_i].b_Forcage = 0;
					//libère message 
					uc_EspionTacheBAEtat = 213;
					_msg_free(s_msg_ptr);
					uc_EspionTacheBAEtat = 214;
					s_msg_ptr = NULL;
					Tb_Com_BA[l_us_i].o_Repete = uc_I2C_NB_REPETE+1;		// pour éviter les débordements
					uc_EspionTacheBAEtat = 215;
				}
				uc_EspionTacheBAEtat = 216;
			}
		}
		
		// Surveille si configuration des sorties à envoyer
		uc_EspionTacheBAEtat = 11;
		for(l_us_i=0; l_us_i<uc_NB_BOITIER_AUXILIAIRE; l_us_i++)			// boucle sur tous les boitiers
		//l_us_i = uc_BOITIER_PIECES_EAU;	//zzz
		{
			l_us_j = 1;		// pointeur dans la trame
			l_us_k = Variateurs_PDV_Conf + (l_us_i <<3);	// 8 max par boitier
		
			for (l_us_l=0; l_us_l<8; l_us_l++, l_us_k++)		// 8 transmis
			{
				// test et prépare la trame en même temps
				if(Tb_Echange[l_us_k] != uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf])
				{
					Tb_Com_BA[l_us_i].b_Conf = 1;
					uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf] = Tb_Echange[l_us_k];
				}
				l_uc_buffer[l_us_j++] = uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf];
			}
		
			if(Tb_Com_BA[l_us_i].b_Conf == 1)
			{
				// au moins 1 configuration des sorties à envoyer
				l_uc_buffer[0] = uc_TRAME_BA_CONF_SORTIES;		//code de la trame
				l_us_CRC = us_CalculerCRCSurTrame(l_uc_buffer, l_us_j);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, l_us_j);
				l_uc_buffer[l_us_j] = (l_us_CRC & 0xFF);
				l_uc_buffer[l_us_j+1] = ((l_us_CRC>>8) & 0xFF);
		
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA : envoi configuration des sorties au boitier %i (%i) -> %d", l_us_i, Tb_Com_BA[l_us_i].o_Repete, l_uc_buffer[0]);
				for(l_uc_Compteur = 1 ; l_uc_Compteur < 10; l_uc_Compteur++)
				{
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,".%d", l_uc_buffer[l_uc_Compteur]);
				}
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,"\n");
				ul_EspionTacheBACompteurEmissionI2C[l_us_i]++;
				
				l_sl_result = sl_fct_write_polled(fd_BA_I2C, (uchar) (l_us_i), l_uc_buffer, (l_us_j+2), &l_uc_ReponseRecue);
				if(l_sl_result == I2C_OK)
				{
					// trame transmise et acquittée : pas de répétition nécessaire
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission OK\n");
					Tb_Com_BA[l_us_i].b_Conf = 0;		// trame transmise et acquittée : pas de répétition nécessaire
					Tb_Com_BA[l_us_i].o_Repete = 0;		// raz compteur de repetitions
				}
				else
				{
					// erreurs => répétition
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission PB %d\n", l_sl_result);
					ul_EspionTacheBACompteurErreurI2C[l_us_i]++;
					Tb_Com_BA[l_us_i].o_Repete++;		// compteur de repetitions
					if(Tb_Com_BA[l_us_i].o_Repete > uc_I2C_NB_REPETE)
					{
						// lorsque le nombre max de répétition est atteint, acquitte la demande pour arrêter les répétitions
						Tb_Com_BA[l_us_i].b_Conf = 0;
						Tb_Com_BA[l_us_i].o_Repete = uc_I2C_NB_REPETE+1;		// pour éviter les débordements
					}
				}
			}
		}
		
		// boitier après boitier
		// surveille si réglage des temps d'extinction à envoyer
		uc_EspionTacheBAEtat = 12;
		for (l_us_i=0; l_us_i<uc_NB_BOITIER_AUXILIAIRE; l_us_i++)			// boucle sur tous les boitiers
		//l_us_i = uc_BOITIER_PIECES_EAU;	//zzz
		{
			l_us_j = 1;		// pointeur dans la trame
			l_us_k = Lampes_PDV_Temps + (l_us_i <<4);	// 16 max par boitier
		
			for (l_us_l = 0; l_us_l<16; l_us_l++, l_us_k++)			// 16 max par boitier 
			{
				// test et prépare la trame en même temps
				if(Tb_Echange[l_us_k] != uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf])
				{
					Tb_Com_BA[l_us_i].b_Extinction = 1;
					uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf] = Tb_Echange[l_us_k];
				}
				l_uc_buffer[l_us_j++] = uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf];
			}
		
			//Tb_Com_BA[l_us_i].b_Extinction = 1;//zzz
			if (Tb_Com_BA[l_us_i].b_Extinction == 1)
			{
				// au moins 1 réglage des temps d'extinction à envoyer
				l_uc_buffer[0] = uc_TRAME_BA_TPS_EXTINCTION;		//code de la trame
				l_us_CRC = us_CalculerCRCSurTrame(l_uc_buffer, l_us_j);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, l_us_j);
				l_uc_buffer[l_us_j] = (l_us_CRC & 0xFF);
				l_uc_buffer[l_us_j+1] = ((l_us_CRC>>8) & 0xFF); 
		
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA : envoi reglage des temps d extinction au boitier %i (%i) -> %d", l_us_i, Tb_Com_BA[l_us_i].o_Repete, l_uc_buffer[0]);
				for(l_uc_Compteur = 1 ; l_uc_Compteur < 18; l_uc_Compteur++)
				{
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,".%d", l_uc_buffer[l_uc_Compteur]);
				}
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,"\n");
				ul_EspionTacheBACompteurEmissionI2C[l_us_i]++;
				
				l_sl_result = sl_fct_write_polled(fd_BA_I2C, (uchar) (l_us_i), l_uc_buffer, (l_us_j+2), &l_uc_ReponseRecue);
				if(l_sl_result == I2C_OK)
				{
					// trame transmise et acquittée : pas de répétition nécessaire
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission OK\n");
					Tb_Com_BA[l_us_i].b_Extinction = 0;	// trame transmise et acquittée : pas de répétition nécessaire
					Tb_Com_BA[l_us_i].o_Repete = 0;		// raz compteur de repetitions
				}
				else
				{
					// erreurs => répétition
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission PB %d\n", l_sl_result);
					ul_EspionTacheBACompteurErreurI2C[l_us_i]++;
					Tb_Com_BA[l_us_i].o_Repete++;		// compteur de repetitions
					if (Tb_Com_BA[l_us_i].o_Repete > uc_I2C_NB_REPETE)
					{
						// lorsque le nombre max de répétition est atteint, acquitte la demande pour arrêter les répétitions
						Tb_Com_BA[l_us_i].b_Extinction = 0;	// trame en erreur : arrêt répétition 
						Tb_Com_BA[l_us_i].o_Repete = uc_I2C_NB_REPETE+1;		// pour éviter les débordements
					}
				}
			}
		}
		
		// boitier après boitier
		// surveille si réglage des temps d'action à envoyer
		uc_EspionTacheBAEtat = 13;
		for (l_us_i=0; l_us_i<uc_NB_BOITIER_AUXILIAIRE; l_us_i++)			// boucle sur tous les boitiers
		//l_us_i = uc_BOITIER_PIECES_EAU;	//zzz
		{
			l_us_j = 1;		// pointeur dans la trame
			l_us_k = Volets_PDV_Temps + (l_us_i <<3);	// 8 max par boitier
		
			for (l_us_l = 0; l_us_l<8; l_us_l++, l_us_k++)		// 8 transmis
			{
				// test et prépare la trame en même temps
				if (Tb_Echange[l_us_k] != uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf])
				{
					Tb_Com_BA[l_us_i].b_TpsAction = 1;
					uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf] = Tb_Echange[l_us_k];
				}
				l_uc_buffer[l_us_j++] = uc_Tb_Borniers_Data[l_us_k - Variateurs_PDV_Conf];
			}
		
			//Tb_Com_BA[l_us_i].b_TpsAction = 1;	//zzz
			if (Tb_Com_BA[l_us_i].b_TpsAction == 1)
			{
				// au moins 1 réglage des temps d'extinction à envoyer
				l_uc_buffer[0] = uc_TRAME_BA_TPS_ACTION;		//code de la trame
				l_us_CRC = us_CalculerCRCSurTrame(l_uc_buffer, l_us_j);	//uc_fct_Calcul_CheckSum_I2C(l_uc_buffer, l_us_j);
				l_uc_buffer[l_us_j] = (l_us_CRC & 0xFF);
				l_uc_buffer[l_us_j+1] = ((l_us_CRC>>8) & 0xFF); 
		
				vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA : envoi reglage reglage des temps d action au boitier %i (%i) -> %d", l_us_i, Tb_Com_BA[l_us_i].o_Repete, l_uc_buffer[0]);
				for(l_uc_Compteur = 1 ; l_uc_Compteur < 10; l_uc_Compteur++)
				{
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,".%d", l_uc_buffer[l_uc_Compteur]);
				}
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_BA_ACTIVITE,"\n");
				ul_EspionTacheBACompteurEmissionI2C[l_us_i]++;
				
				l_sl_result = sl_fct_write_polled(fd_BA_I2C, (uchar) (l_us_i), l_uc_buffer, (l_us_j+2), &l_uc_ReponseRecue);
				if(l_sl_result == I2C_OK)
				{
					// trame transmise et acquittée : pas de répétition nécessaire
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission OK\n");
					Tb_Com_BA[l_us_i].b_TpsAction = 0;	// trame transmise et acquittée : pas de répétition nécessaire
					Tb_Com_BA[l_us_i].o_Repete = 0;		// raz compteur de repetitions
				}
				else
				{
					// erreurs => répétition
					vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"BA emission PB %d\n", l_sl_result);
					ul_EspionTacheBACompteurErreurI2C[l_us_i]++;
					Tb_Com_BA[l_us_i].o_Repete++;		// compteur de repetitions
					if (Tb_Com_BA[l_us_i].o_Repete > uc_I2C_NB_REPETE)
					{
						// lorsque le nombre max de répétition est atteint, acquitte la demande pour arrêter les répétitions
						Tb_Com_BA[l_us_i].b_TpsAction = 0;	// trame en erreur : arrêt répétition 
						Tb_Com_BA[l_us_i].o_Repete = uc_I2C_NB_REPETE+1;		// pour éviter les débordements
					}
				}
			}
		}

		// surveillance de la communication
		uc_EspionTacheBAEtat = 14;
		if (Tb_Com_BA[0].o_Repete > uc_I2C_NB_REPETE_AV_ERREUR)
		{
			// considéré en erreur au bout de x trames émises sans réception correct du OK
			st_EchangeStatus.uc_Defboitier1 = 1;
		}
		else
		{
			st_EchangeStatus.uc_Defboitier1 = 0;
		}
		if (Tb_Com_BA[1].o_Repete > uc_I2C_NB_REPETE_AV_ERREUR)
		{
			// considéré en erreur au bout de x trames émises sans réception correct du OK
			st_EchangeStatus.uc_Defboitier2 = 1;
		}
		else
		{
			st_EchangeStatus.uc_Defboitier2 = 0;
		}
		if (Tb_Com_BA[2].o_Repete > uc_I2C_NB_REPETE_AV_ERREUR)
		{
			// considéré en erreur au bout de x trames émises sans réception correct du OK
			st_EchangeStatus.uc_Defboitier3 = 1;
		}
		else
		{
			st_EchangeStatus.uc_Defboitier3 = 0;
		}

		uc_EspionTacheBAEtat = 15;
		_sched_yield();		// passe la main à la tache suivante
		uc_EspionTacheBAEtat = 16;
	}	   // fin boucle tache de fond permanente

	vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"STOP\n");
	
	// close the message queue
	uc_EspionTacheBAEtat = 17;
	if(_msgq_close(l_BA_qid) == FALSE)
	{
		DETECTION_ERREUR_TACHE_BA_CLOSE_MESSAGE_QUEUE;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Close queue\n");
	}
	
	// Close the driver 
	uc_EspionTacheBAEtat = 18;
	l_sl_result = (int_32)fclose(fd_BA_I2C);
	if(l_sl_result) 
	{
		DETECTION_ERREUR_TACHE_BA_CLOSE_I2C;
		vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ERREUR,"Erreur close I2C %d\n", l_sl_result);
	}

	// Free transation lock
	uc_EspionTacheBAEtat = 19;
	_lwsem_destroy (&lock_I2C);

	vd_EspionRS_Printf(uc_ESPION_TACHE_BA_ACTIVITE,"END\n");
	uc_EspionTacheBAEtat = 20;
	_task_block();
	uc_EspionTacheBAEtat = 21;
} 

// L'applicatif va forcer l'état des sorties selon les cas
// 		-> exécution d'un scénario
// Les ordres de pilotage sont stockés dans Tb_Echangeboitier
// Dans la tache principale, si Tb_Echangeboitier contient des ordres à envoyer (!= 0), ces ordres sont envoyés à la tache BA via un message
// Pourquoi un message ?
// 		Après émission, les ordres sont remis à 0
//		Pendant émission d'autres ordres peuvent venir...
//		Le fait d'envoyer un message permet de remettre à 0 tout de suite le tableau Tb_Echangeboitier
//		Tout en envoyant son état par message, ce qui permet de ne pas perdre un éventuel ordre qui arriverait tout de suite après
// xxx mutex sur us_Tb_Echangeboitier ? a priori non car change par scenarios appelle depuis le main donc pas d'acces mutithreads...
void vd_UpdateOrdresPilotageSortiesBA(void)
{
	unsigned char l_uc_CompteurBoitier;
	unsigned char l_uc_IndiceData;
	static unsigned char s_uc_FlagErreurDetectee = 0;	// Pour éviter x messages en cas d'erreur...
	COMMANDE_MESSAGE_PTR l_msg_ptr;
	
	// synchronisation des écritures vers les BA
	for(l_uc_CompteurBoitier=0; l_uc_CompteurBoitier<uc_NB_BOITIER_AUXILIAIRE; l_uc_CompteurBoitier++)			// boucle sur tous les boitiers
	//l_uc_CompteurBoitier = uc_BOITIER_PIECES_EAU;	//zzz
	{
		if(us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+l_uc_CompteurBoitier] != 0 ||
		   us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+l_uc_CompteurBoitier] != 0 ||
		   us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+l_uc_CompteurBoitier] != 0 ||
		   us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+l_uc_CompteurBoitier] != 0 ||
		   us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+l_uc_CompteurBoitier] != 0 ||
		   us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+l_uc_CompteurBoitier] != 0)
		{
			// au moins 1 forçage à transmettre
			if(_msg_available(message_pool) != 0) 
			{
				l_msg_ptr = (COMMANDE_MESSAGE_PTR)_msg_alloc(message_pool);
				if(l_msg_ptr == NULL)
				{
					ul_EspionTachePrincipaleNbMessagesBAErreurs1++;
					DETECTION_ERREUR_TACHE_PRINCIPALE_MESSAGE_ALLOC;
					if(s_uc_FlagErreurDetectee == 0)
					{
						s_uc_FlagErreurDetectee = 1;
						vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Error creation message!\n");
					}
				}
				else
				{
					l_msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, BA_QUEUE_ID);
					l_msg_ptr->HEADER.SIZE = sizeof(COMMANDE_MESSAGE) + 1;
					// renseigne zone data
					l_msg_ptr->DATA[0] = (uchar)l_uc_CompteurBoitier;		// numero de BA
					l_uc_IndiceData = 1;		// pointeur dans la trame
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+l_uc_CompteurBoitier] & 0xFF);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+l_uc_CompteurBoitier] & 0xFF);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+l_uc_CompteurBoitier] >> 8);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+l_uc_CompteurBoitier] >> 8);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+l_uc_CompteurBoitier] & 0xFF);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+l_uc_CompteurBoitier] & 0xFF);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+l_uc_CompteurBoitier] & 0xFF);
					l_msg_ptr->DATA[l_uc_IndiceData++] = (uchar) (us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+l_uc_CompteurBoitier] & 0xFF);
					//xxx pas tout envoyé ???
			
					if(_msgq_send(l_msg_ptr) == FALSE)
					{
						ul_EspionTachePrincipaleNbMessagesBAErreurs2++;
						DETECTION_ERREUR_TACHE_PRINCIPALE_MESAGE_SEND;
						if(s_uc_FlagErreurDetectee == 0)
						{
							s_uc_FlagErreurDetectee = 1;
							vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Error envoi message!\n");
						}
						_msg_free(l_msg_ptr);
					}
					else
					{
						ul_EspionTachePrincipaleNbMessagesBAEnvoyes++;
						s_uc_FlagErreurDetectee = 0;
						// forçages transmis : raz demandes
						us_Tb_Echangeboitier[us_SSimples_BA1_CdeEteint+l_uc_CompteurBoitier] = 0;
						us_Tb_Echangeboitier[us_SSimples_BA1_CdeAllume+l_uc_CompteurBoitier] = 0;
						us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeEteint+l_uc_CompteurBoitier] = 0;
						us_Tb_Echangeboitier[us_SVariateurs_BA1_CdeAllume+l_uc_CompteurBoitier]= 0;
						us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+l_uc_CompteurBoitier] = 0;
						us_Tb_Echangeboitier[us_SVolets_BA1_CdeFerme+l_uc_CompteurBoitier] = 0;
					}
				}
			}
			else
			{
				ul_EspionTachePrincipaleNbMessagesBAErreurs3++;
				DETECTION_ERREUR_TACHE_PRINCIPALE_MESSAGE_AVAILABLE;
				if(s_uc_FlagErreurDetectee == 0)
				{
					s_uc_FlagErreurDetectee = 1;
					vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ERREUR,"Error pile de message pleine!\n");
				}
			}
		}
	}
}
