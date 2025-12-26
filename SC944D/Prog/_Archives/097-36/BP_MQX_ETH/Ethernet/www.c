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
#include "global.h"
#include "EspionRS.h"
#include "GestionSocket.h"
#include "TableEchangeAcces.h"
#include "cryptage.h"
#include "Cryptagerijndael_mode.h"
#include "download.h"
#include "hard.h"
#include "tableechangeflash.h"

#define extern
#include "www.h"
#undef extern

#include "json.h"


void vd_EthernetInitVariables(void)
{
	uc_EthernetTraitement = uc_EthernetPasDemarre;
	uc_ConfigIP = uc_CONFIG_IP_DHCP;	// Mode DHCP xxx recuperer config de l'EEPROM

	// Config IP par défaut
	IPConfig.ip = IP_ADRESSE_PAR_DEFAUT;
	IPConfig.mask = IP_MASQUE_PAR_DEFAUT;
	IPConfig.gateway = IP_GATEWAY_PAR_DEFAUT;
	
	IPServeur = 0;
}

// Tache teleinfo
void Ethernet_task(uint_32 dummy)
{
	unsigned long l_ul_Erreur;
	signed char l_sc_Retour;
	unsigned char l_uc_Compteur;
	unsigned char l_uc_Etat;
	
	
	// Démarrage tache -> attente 10 secondes avant de démarrer le traitement ETHERNET
	uc_EspionTacheEthernetEtat = 1;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"START !\n");
	_time_delay(1000);
	uc_EspionTacheEthernetEtat = 2;
	
	cryptage();	// Calcule la clé à envoyer au serveur à chaque requête
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Resultat cryptage : %s\n\n", c_MatriculeCryptee);

	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"START !\n");
	
	vd_EthernetInitVariables();
	uc_EspionTacheEthernetEtat = 3;
	
	vd_RTCSInit();	// Init couche ETHERNET de MQX
	uc_EspionTacheEthernetEtat = 4;
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"INIT RTCS OK !\n");
	
	while(1)
	{
		ul_EspionTacheEthernetCompteurActivite++;
		uc_EspionTacheEthernetEtat = 100;
		
		// Cadencement traitement augmenté si user connecté
		if(uc_EthernetTraitement != uc_EthernetDNSOK)
		{
			// Si Pas de dialogue avec serveur -> RAZ informations envoyées par serveur
			st_ServerInformation.uc_IsConnected = 0;
			st_ServerInformation.us_NewVersion = 0;
			st_ServerInformation.uc_NbInfosDemandees = 0;
		}
		if(st_ServerInformation.uc_IsConnected == 0)	_time_delay(us_TRAITEMENT_ETHERNET_USER_CONNECTE_NON_MS);
		else											_time_delay(us_TRAITEMENT_ETHERNET_USER_CONNECTE_OUI_MS);
		
		uc_EspionTacheEthernetEtat = 101;
		
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TRAITEMENT !\n");
		
		// Traitement
		switch(uc_EthernetTraitement)
		{
			case uc_EthernetPasDemarre:
				// Controle présence réseau
				uc_EspionTacheEthernetEtat = 110;
				if(ipcfg_get_link_active(ENET_DEVICE) == TRUE)
				{
					ul_EspionTacheEthernetCableOK++;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CABLE OK !\n");
					uc_EthernetTraitement = uc_EthernetReseauOK;
					uc_EtatEthernetCablePB = 0;
				}
				else
				{
					ul_EspionTacheEthernetCablePB++;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"CABLE PB !\n");
					uc_EtatEthernetCablePB = 1;
					uc_EtatEthernetDHCPPB = 1;
					uc_EtatEthernetDNSPB = 1;
					uc_EtatEthernetServeurPB = 1;
				}
			break;
			case uc_EthernetReseauOK:
				// Mode DHCP : recherche serveur DHCP
				// Mode IP fixe : adresse fixée
				uc_EspionTacheEthernetEtat = 120;
				l_ul_Erreur = IPCFG_ERROR_OK;
				if(uc_ConfigIP  == uc_CONFIG_IP_FIXE)
				{
					//xxx mettre bonne adresse IP
					l_ul_Erreur = ipcfg_bind_staticip(ENET_DEVICE, &IPConfig);
					if(l_ul_Erreur != IPCFG_ERROR_OK)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"IPFIXE PB (%d) !\n", l_ul_Erreur);
						ul_EspionTacheEthernetIPFixePB++;
					}
					else
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"IPFIXE OK !\n");
						ul_EspionTacheEthernetIPFixeOK++;
					}
				}
				else	// DHCP
				{
					l_ul_Erreur = ipcfg_bind_dhcp_wait(ENET_DEVICE, FALSE, &IPConfig);
					if(l_ul_Erreur != IPCFG_ERROR_OK)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DHCP PB (%d) !\n", l_ul_Erreur);
						ul_EspionTacheEthernetDCHPPB++;
					}
					else
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DHCP OK !\n");
						ul_EspionTacheEthernetDCHPOK++;
					}
				}
				if(l_ul_Erreur == IPCFG_ERROR_OK)
				{
					uc_EthernetTraitement = uc_EthernetAdresseIPOK;
					ipcfg_get_ip(ENET_DEVICE, &IPConfig);
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"IP : %d.%d.%d.%d\n",IPBYTES(IPConfig.ip));
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Subnet :  %d.%d.%d.%d\n",IPBYTES(IPConfig.mask));
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Gateway : %d.%d.%d.%d\n",IPBYTES(IPConfig.gateway));
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DNS : %d.%d.%d.%d\n",IPBYTES(ipcfg_get_dns_ip(ENET_DEVICE,0)));
					uc_EtatEthernetDHCPPB = 0;
				}
				else
				{
					uc_EthernetTraitement = uc_EthernetPasDemarre;
					uc_EtatEthernetDHCPPB = 1;
					uc_EtatEthernetDNSPB = 1;
					uc_EtatEthernetServeurPB = 1;
				}
			break;
			case uc_EthernetAdresseIPOK:
				// Recherche @ IP DNS "mon.essensys.fr"
				uc_EspionTacheEthernetEtat = 130;
				IPServeur = 0;
				if(RTCS_resolve_ip_address(NOM_SERVEUR_ESSENSYS,&IPServeur,NULL,0) == TRUE) 
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"@ serveur trouvée -> %d.%d.%d.%d\n",IPBYTES(IPServeur));
					ul_EspionTacheEthernetDNSOK++;
					uc_EthernetTraitement = uc_EthernetDNSOK;
					uc_EtatEthernetDNSPB = 0;
				}
				else
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur DNS\n");
					ul_EspionTacheEthernetDNSPB++;
					uc_EthernetTraitement = uc_EthernetPasDemarre;
					uc_EtatEthernetDNSPB = 1;
					uc_EtatEthernetServeurPB = 1;
				}
			break;
			case uc_EthernetDNSOK:
				l_sc_Retour = sc_DialogueAvecServeur();
				uc_EtatEthernetServeurPB = 1;
				switch(l_sc_Retour)
				{
					case sc_ETHERNET_RETOUR_OK:
						uc_EtatEthernetServeurPB = 0;
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DialogueAvecServeur OK\n");
						ul_EspionTacheEthernetDialogueServeurOK++;
					break;
					case sc_ETHERNET_RETOUR_PB_DATA:
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DialogueAvecServeur PB DATA\n");
						ul_EspionTacheEthernetDialogueServeurPBData++;
						uc_EthernetTraitement = uc_EthernetPasDemarre;
					break;
					//case sc_ETHERNET_RETOUR_PB_RTCS:
					default:
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DialogueAvecServeur PB RTCS\n");
						ul_EspionTacheEthernetDialogueServeurPBRTCS++;
						uc_EthernetTraitement = uc_EthernetPasDemarre;
					break;
				}
			break;
			default:
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Etape traitement non géré (%d) !\n", uc_EthernetTraitement);
			break;
		}
		uc_EspionTacheEthernetEtat = 102;

		// Bit 0 = Etat câble (0=OK / 1=HS)
		// Bit 1 = Etat DHCP (0=OK / 1=HS)
		// Bit 2 = Etat DNS (0=OK / 1=HS)
		// Bit 3 = Etat serveur (0=OK / 1=HS)
		l_uc_Etat = 0;
		if(uc_EtatEthernetCablePB != 0)		l_uc_Etat |= 0x01;
		if(uc_EtatEthernetDHCPPB != 0)		l_uc_Etat |= 0x02;
		if(uc_EtatEthernetDNSPB != 0)		l_uc_Etat |= 0x04;
		if(uc_EtatEthernetServeurPB != 0)	l_uc_Etat |= 0x08;
		st_EchangeStatus.uc_EtatEthernet = l_uc_Etat;


		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n");
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tuc_IsConnected : %d\n", st_ServerInformation.uc_IsConnected);
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tus_NewVersion : %d\n", st_ServerInformation.us_NewVersion);
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tuc_NbInfosDemandees : %d\n", st_ServerInformation.uc_NbInfosDemandees);
		for(l_uc_Compteur = 0;l_uc_Compteur < st_ServerInformation.uc_NbInfosDemandees;l_uc_Compteur++)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tValeur(%d) : %d (%s)\n", l_uc_Compteur, st_ServerInformation.us_InfosDemandees[l_uc_Compteur], puc_AfficherLibelleTableEchange(st_ServerInformation.us_InfosDemandees[l_uc_Compteur]));
		}

		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FIN TRAITEMENT !\n");
		_sched_yield();		// passe la main à la tache suivante
		uc_EspionTacheEthernetEtat = 103;
	}
	
	uc_EspionTacheEthernetEtat = 254;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FIN !\n");
	uc_EspionTacheEthernetEtat = 255;
}

// Installation et initialisation de la couche ethernet RTCS de MQX
void vd_RTCSInit()
{
	unsigned long l_ul_Erreur;
	_enet_address l_enet_address; 
	unsigned char l_uc_Compteur;
	
	
	uc_EspionTacheEthernetInitRTCS = 1;
	
	_RTCSPCB_init = 3;	//xxx valeurs a revoir / expliquer
	_RTCS_msgpool_init = 3;
	_RTCS_socket_part_init = 3;

	l_ul_Erreur = RTCS_create();
	if(l_ul_Erreur != RTCS_OK)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur init RTCS (%d) !\n", l_ul_Erreur);
		_task_block();
	}
	
	uc_EspionTacheEthernetInitRTCS = 2;
	_IP_forward = TRUE;	//xxx ???
		
	// Calcule l'adresse MAC unique à partir de l'adresse IP
	// xxx recuperer dans eeprom i2c - ne pas calculer avec ip par defaut mais celle reellement utilise ?
	uc_EspionTacheEthernetInitRTCS = 3;

	if(uc_AdresseMac[0] != 0 ||
	   uc_AdresseMac[1] != 0 ||
	   uc_AdresseMac[2] != 0 ||
	   uc_AdresseMac[3] != 0 ||
	   uc_AdresseMac[4] != 0 ||
	   uc_AdresseMac[5] != 0)
	{
		// @ MAX EEPROM valide -> utiliser celle-ci
		for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_AdresseMac);l_uc_Compteur++)
		{
			l_enet_address[l_uc_Compteur] = uc_AdresseMac[l_uc_Compteur];
		}
	}
	else
	{
		// Sinon calculer une adresse MAC (formule MQX)
		ENET_get_mac_address(ENET_DEVICE, IP_ADRESSE_PAR_DEFAUT, l_enet_address);	//xxx regarder calcul
	}
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"@ MAC utilisée : ");
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_AdresseMac);l_uc_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%02X.", l_enet_address[l_uc_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n");

	l_ul_Erreur = ipcfg_init_device(ENET_DEVICE, l_enet_address);
	if(l_ul_Erreur != RTCS_OK) 
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur init device (%d) !\n", l_ul_Erreur);
		_task_block();
	}

	uc_EspionTacheEthernetInitRTCS = 4;
	//xxx ??? fait avant DHCP ???
	//#if RTCSCFG_ENABLE_LWDNS
//xxx	LWDNS_server_ipaddr = IP_GATEWAY_PAR_DEFAUT;   
//xxx	ipcfg_add_dns_ip(ENET_DEVICE,LWDNS_server_ipaddr);
	//#endif /* RTCSCFG_ENABLE_LWDNS 
	
	uc_EspionTacheEthernetInitRTCS = 5;
}

signed char sc_DialogueAvecServeur(void)
{
	signed char l_sc_Retour;
	
	uc_EspionTacheEthernetDialogueAvecServeur = 1;
	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DIALOGUE SERVEUR\n");

	uc_EspionTacheEthernetDialogueAvecServeur = 2;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 1\n");
	l_sc_Retour = sc_GetInformationServer();
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		ul_EspionTacheEthernetFonction1OK++;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 1 OK\n");
	}
	else
	{
		ul_EspionTacheEthernetFonction1PB++;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 1 PB %d\n", l_sc_Retour);
	}
	
	_time_delay(us_TEMPO_INTER_ACTION_ETHERNET);
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		uc_EspionTacheEthernetDialogueAvecServeur = 3;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 2\n");
		l_sc_Retour = sc_PostInformationServer();
		if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
		{
			ul_EspionTacheEthernetFonction2OK++;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 2 OK\n");
		}
		else
		{
			ul_EspionTacheEthernetFonction2PB++;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 2 PB %d\n", l_sc_Retour);
		}
	}
	
	_time_delay(us_TEMPO_INTER_ACTION_ETHERNET);
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		uc_EspionTacheEthernetDialogueAvecServeur = 4;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 3/4\n");
		l_sc_Retour = sc_ActionManagment();
		if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
		{
			ul_EspionTacheEthernetFonctions3et4OK++;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 3/4 OK\n");
		}
		else
		{
			ul_EspionTacheEthernetFonctions3et4PB++;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"FCT 3/4 PB %d\n", l_sc_Retour);
		}
	}
	
	_time_delay(us_TEMPO_INTER_ACTION_ETHERNET);
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		uc_EspionTacheEthernetDialogueAvecServeur = 5;
		if(st_ServerInformation.us_NewVersion != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DOWNLOAD\n");
			uc_DownloadEnCours = 1;
			uc_DownloadEnCoursBloquerI2CVersBA = 1;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DOWNLOAD version %d\n", st_ServerInformation.us_NewVersion);
			l_sc_Retour = sc_Download();
			if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
			{
				ul_EspionTacheEthernetFonctionDownload++;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DOWNLOAD OK\n");
				
				if(uc_ResetDemande != 0)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"MEMORISATION TABLE ECHANGE...\n");	//xxx
					vd_TableEchangeSaveEnFLash();
					
					uc_DownloadEnCoursBloquerI2CVersBA = 0;	// Réautoriser dialogue I2C BA pour envoyer ordre sauvegarde
					uc_FlagResetBP = 1;						// Sauvegarder état BA - xxx 2 secondes suffisantes , va rendre la main à la tache BA ?
					
					_time_delay(2000);
					
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"RESET...\n");	//xxx
					_time_delay(2000);
					vd_MCF52259_REBOOT();
				}
			}
			else
			{
				ul_EspionTacheEthernetFonctionDownload++;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"DOWNLOAD PB %d\n", l_sc_Retour);
			}
		}
	}
	uc_DownloadEnCours = 0;
	uc_DownloadEnCoursBloquerI2CVersBA = 0;
	
	uc_EspionTacheEthernetDialogueAvecServeur = 20;
	return l_sc_Retour;
}

void vd_InitBufferTXRX(char *c_Buffer, unsigned short us_Taille)
{
	unsigned short l_us_Compteur;
	
	for(l_us_Compteur = 0;l_us_Compteur < us_Taille;l_us_Compteur++)
	{
		c_Buffer[l_us_Compteur] = 0;
	}
}

// Interroge le serveur à l'adresse IPServeur
// Retourne 0 si OK ou < 0 si PB
// Identifiant systeme : MATRICULE_SYSTEME
// Informations reçues -> st_ServerInformation
// Utilise les buffers c_EthernetBufferTX et c_EthernetBufferRX
signed char sc_GetInformationServer(void)
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	unsigned long l_ul_sock;
	signed long l_sl_Retour;
	unsigned long l_ul_Longueur;
	unsigned char l_uc_Compteur;
	
	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_InitBufferTXRX(c_EthernetBufferTX, sizeof(c_EthernetBufferTX));
	vd_InitBufferTXRX(c_EthernetBufferRX, sizeof(c_EthernetBufferRX));
	
	// RAZ informations serveur avant de les redemander
	// Si pb communication -> on ne conservera pas un état incorrect
	st_ServerInformation.uc_IsConnected = 0;
	st_ServerInformation.us_NewVersion = 0;
	st_ServerInformation.uc_NbInfosDemandees = 0;
		
	// Ouverture socket avec serveur
	uc_EspionTacheEthernetGetInformationServer = 1;
	l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
	uc_EspionTacheEthernetGetInformationServer = 2;
	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
	}
	else
	{
		uc_EspionTacheEthernetGetInformationServer = 3;
		strcpy(c_EthernetBufferTX, "GET /api/serverinfos HTTP/1.1 \r\n");
		strcat(c_EthernetBufferTX, c_EnteteTrameAccept());
		strcat(c_EthernetBufferTX, c_EnteteTrameHost());
		strcat(c_EthernetBufferTX, c_EnteteTrameCache());
		strcat(c_EthernetBufferTX, c_EnteteTrameAcceptCharset());
		strcat(c_EthernetBufferTX, c_EnteteTrameAuthorisation());
		strcat(c_EthernetBufferTX, c_EnteteTrameMatricule());
		strcat(c_EthernetBufferTX, "\r\n");
		strcat(c_EthernetBufferTX, "\r\n");
		//strcat(c_EthernetBufferTX, "\r\n");	//xxx util ?
		
		l_ul_Longueur = strlen(c_EthernetBufferTX);
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer TX\n");
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX);
		uc_EspionTacheEthernetGetInformationServer = 4;
		l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX, l_ul_Longueur, 0);
		uc_EspionTacheEthernetGetInformationServer = 5;
		if(l_sl_Retour == RTCS_ERROR)
		{
			uc_EspionTacheEthernetGetInformationServer = 6;
			ul_DetailErreur = RTCS_geterror(l_ul_sock);
			uc_EspionTacheEthernetGetInformationServer = 7;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer erreur TX %d\n", ul_DetailErreur);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
		}
		else
		{
			uc_EspionTacheEthernetGetInformationServer = 8;
			if(l_sl_Retour != l_ul_Longueur)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer erreur TX pb lg\n");
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
			}
			else
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer TX OK\n");
				
				// Reception de la réponse du serveur
				uc_EspionTacheEthernetGetInformationServer = 9;
				l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX, sizeof(c_EthernetBufferRX)-1, 0);
				uc_EspionTacheEthernetGetInformationServer = 10;
				if(l_sl_Retour == RTCS_ERROR)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer erreur RX pb lg\n");
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
				}
				else
				{
					uc_EspionTacheEthernetGetInformationServer = 11;
					c_EthernetBufferRX[l_sl_Retour] = 0;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"GetInformationServer RX\n");
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX);
					RTCS_time_delay(10);	//xxx pourquoi ???
					
					// Traitement des données reçues
					uc_EspionTacheEthernetGetInformationServer = 12;
					l_sc_Retour = sc_JsonGetServerInformation(c_EthernetBufferRX, sizeof(c_EthernetBufferRX), (unsigned short)l_sl_Retour, &st_ServerInformation);
					uc_EspionTacheEthernetGetInformationServer = 13;
					if(l_sc_Retour == 0)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonGetServerInformation data OK\n");
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tuc_IsConnected : %d\n", st_ServerInformation.uc_IsConnected);
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tus_NewVersion : %d\n", st_ServerInformation.us_NewVersion);
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tuc_NbInfosDemandees : %d\n", st_ServerInformation.uc_NbInfosDemandees);
						for(l_uc_Compteur = 0;l_uc_Compteur < st_ServerInformation.uc_NbInfosDemandees;l_uc_Compteur++)
						{
							vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tValeur(%d) : %d (%s)\n", l_uc_Compteur, st_ServerInformation.us_InfosDemandees[l_uc_Compteur], puc_AfficherLibelleTableEchange(st_ServerInformation.us_InfosDemandees[l_uc_Compteur]));
						}
					}
					else
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonGetServerInformation erreur analyse %d\n", l_sc_Retour);
						l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
					}
					uc_EspionTacheEthernetGetInformationServer = 14;
				}
			}
		}
		uc_EspionTacheEthernetGetInformationServer = 15;
		shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket
		uc_EspionTacheEthernetGetInformationServer = 16;
	}
	uc_EspionTacheEthernetGetInformationServer = 17;
	return l_sc_Retour;
}

// Renvoie au serveur les valeurs demandées
// Retourne 0 si OK ou < 0 si PB
// Identifiant systeme : MATRICULE_SYSTEME
// Valeurs à envoyer -> st_ServerInformation
// Utilise les buffers c_EthernetBufferTX, c_EthernetBufferTX2 et c_EthernetBufferRX
signed char sc_PostInformationServer(void)
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	char l_c_Buffer[10];
	signed long l_sl_Retour;
	unsigned long l_ul_sock;
	unsigned long l_ul_Longueur;

	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_InitBufferTXRX(c_EthernetBufferTX, sizeof(c_EthernetBufferTX));
	vd_InitBufferTXRX(c_EthernetBufferTX2, sizeof(c_EthernetBufferTX2));
	vd_InitBufferTXRX(c_EthernetBufferRX, sizeof(c_EthernetBufferRX));
	
	// Ouverture socket avec serveur
	uc_EspionTacheEthernetPostInformationServer = 1;
	l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
	uc_EspionTacheEthernetPostInformationServer = 2;
	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
	}
	else
	{
		uc_EspionTacheEthernetPostInformationServer = 3;
		strcpy(c_EthernetBufferTX, "POST /api/mystatus HTTP/1.1\r\n");
		strcat(c_EthernetBufferTX, c_EnteteTrameAccept());
		strcat(c_EthernetBufferTX, c_EnteteTrameContentType());
		strcat(c_EthernetBufferTX, c_EnteteTrameHost());
		strcat(c_EthernetBufferTX, c_EnteteTrameCache());
		strcat(c_EthernetBufferTX, c_EnteteTrameAcceptCharset());
		uc_EspionTacheEthernetPostInformationServer = 4;
		l_sc_Retour = sc_JsonPostServerInformation(c_EthernetBufferTX2, sizeof(c_EthernetBufferTX2), &st_ServerInformation);
		if(l_sc_Retour != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonPostServerInformation erreur %d\n", l_sc_Retour);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
		}
		else
		{
			uc_EspionTacheEthernetPostInformationServer = 5;
			
			strcat(c_EthernetBufferTX, "Content-Length:");
			l_ul_Longueur = strlen(c_EthernetBufferTX2);
			sprintf(l_c_Buffer, " %d\r\n", l_ul_Longueur);
			strcat(c_EthernetBufferTX, l_c_Buffer);
			strcat(c_EthernetBufferTX, c_EnteteTrameAuthorisation());
			strcat(c_EthernetBufferTX, c_EnteteTrameMatricule());
			strcat(c_EthernetBufferTX, "\r\n");
			strcat(c_EthernetBufferTX, "\r\n");
			
			if((strlen(c_EthernetBufferTX) + strlen(c_EthernetBufferTX2)) < sizeof(c_EthernetBufferTX))
			{
				strcat(c_EthernetBufferTX, c_EthernetBufferTX2);

				l_ul_Longueur = strlen(c_EthernetBufferTX);
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer TX\n");
				vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX);
				uc_EspionTacheEthernetPostInformationServer = 6;
				l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX, l_ul_Longueur, 0);
				uc_EspionTacheEthernetPostInformationServer = 7;
				if(l_sl_Retour == RTCS_ERROR)
				{
					uc_EspionTacheEthernetPostInformationServer = 8;
					ul_DetailErreur = RTCS_geterror(l_ul_sock);
					uc_EspionTacheEthernetPostInformationServer = 9;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer erreur TX %d\n", ul_DetailErreur);
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
				}
				else
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer TX OK\n");
					
					// Reception de la réponse du serveur
					uc_EspionTacheEthernetPostInformationServer = 10;
					l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX, sizeof(c_EthernetBufferRX)-1, 0);
					uc_EspionTacheEthernetPostInformationServer = 11;
					if(l_sl_Retour == RTCS_ERROR)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer erreur RX pb lg\n");
						l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
					}
					else
					{
						uc_EspionTacheEthernetPostInformationServer = 12;
						c_EthernetBufferRX[l_sl_Retour] = 0;
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer RX\n");
						vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX);
						RTCS_time_delay(10);	//xxx pourquoi ???
						
						// Traitement des données reçues
						// Verification code retour serveur :
						if(strstr(c_EthernetBufferRX, "HTTP/1.1 201 Created") == NULL)
						{
							l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
							vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PostInformationServer -> code retour serveur incorrect\n");
						}
						uc_EspionTacheEthernetPostInformationServer = 13;
					}
				}
			}
			else
			{
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA; 
			}
		}
		uc_EspionTacheEthernetPostInformationServer = 14;
		shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket
		uc_EspionTacheEthernetPostInformationServer = 15;
	}
	return l_sc_Retour;
}

// Interroge le serveur s'il y a des actions à réaliser, traitent celles-ci et renvoie pour chacune l'acquittement
// Retourne 0 si OK ou < 0 si PB
signed char sc_ActionManagment(void)
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	unsigned long l_ul_sock;
	signed long l_sl_Retour;
	unsigned long l_ul_Longueur;
	unsigned char l_uc_Compteur;
	unsigned char l_uc_SocketFermee;
	
	
	l_uc_SocketFermee = 0;
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_InitBufferTXRX(c_EthernetBufferTX, sizeof(c_EthernetBufferTX));
	vd_InitBufferTXRX(c_EthernetBufferRX, sizeof(c_EthernetBufferRX));

	// Ouverture socket avec serveur
	uc_EspionTacheEthernetActionManagment = 1;
	l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
	uc_EspionTacheEthernetActionManagment = 2;
	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
	}
	else
	{
		uc_EspionTacheEthernetActionManagment = 3;
		strcpy(c_EthernetBufferTX, "GET /api/myactions HTTP/1.1 \r\n");
		strcat(c_EthernetBufferTX, c_EnteteTrameAccept());
		strcat(c_EthernetBufferTX, c_EnteteTrameHost());
		strcat(c_EthernetBufferTX, c_EnteteTrameCache());
		strcat(c_EthernetBufferTX, c_EnteteTrameAcceptCharset());
		strcat(c_EthernetBufferTX, c_EnteteTrameAuthorisation());
		strcat(c_EthernetBufferTX, c_EnteteTrameMatricule());
		strcat(c_EthernetBufferTX, "\r\n");
		strcat(c_EthernetBufferTX, "\r\n");		
		
		l_ul_Longueur = strlen(c_EthernetBufferTX);
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment TX\n");
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX);
		uc_EspionTacheEthernetActionManagment = 4;
		l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX, l_ul_Longueur, 0);
		uc_EspionTacheEthernetActionManagment = 5;
		if(l_sl_Retour == RTCS_ERROR)
		{
			uc_EspionTacheEthernetActionManagment = 6;
			ul_DetailErreur = RTCS_geterror(l_ul_sock);
			uc_EspionTacheEthernetActionManagment = 7;
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment erreur TX %d\n", ul_DetailErreur);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
		}
		else
		{
			uc_EspionTacheEthernetActionManagment = 8;
			if(l_sl_Retour != l_ul_Longueur)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment erreur TX pb lg\n");
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
			}
			else
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment TX OK\n");
				
				// Reception de la réponse du serveur
				uc_EspionTacheEthernetActionManagment = 9;
				
				l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX, sizeof(c_EthernetBufferRX)-1, 0);
				uc_EspionTacheEthernetActionManagment = 10;
				if(l_sl_Retour == RTCS_ERROR)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment erreur RX pb lg\n");
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
				}
				else
				{
					uc_EspionTacheEthernetActionManagment = 11;
					c_EthernetBufferRX[l_sl_Retour] = 0;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"ActionManagment RX\n");
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX);
					RTCS_time_delay(10);	//xxx pourquoi ???
					
					// Traitement des données reçues
					uc_EspionTacheEthernetActionManagment = 12;
					
					// Fermeture de la socket en cours -> sc_TraiterActions() va en ouvrir une pour ses requêtes
					shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);
					l_uc_SocketFermee = 1;
					
					l_sc_Retour = sc_TraiterActions();//pppl_ul_sock);
					uc_EspionTacheEthernetActionManagment = 13;
					if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraiterActions OK\n");
					}
					else
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"erreur TraiterActions %d\n", l_sc_Retour);
					}
					uc_EspionTacheEthernetActionManagment = 14;
				}
			}
		}
		uc_EspionTacheEthernetActionManagment = 15;
		if(l_uc_SocketFermee == 0)
		{
			shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket (si pas déjà fermée...)
		}
		uc_EspionTacheEthernetActionManagment = 16;
	}
	uc_EspionTacheEthernetActionManagment = 17;
	return l_sc_Retour;
}

// Analyse la reponse du serveur contenant les actions à réaliser
// Recherche pour chaque action la liste des valeurs à utiliser, met à jour la table d'échange, et envoie l'acquittement
// Retourne 0 si OK ou < 0 si PB
// Identifiant systeme : MATRICULE_SYSTEME
// Utilise les buffers c_EthernetBufferRX (contient les actions à réaliser), c_EthernetBufferTX2 et c_EthernetBufferRX2 (emission acquittement)
signed char sc_TraiterActions(void)//pppunsigned long ul_sock) xxx faire menage sur ppp
{
	signed char l_sc_Retour;
	char *l_c_Guid;
	char *l_c_Obl;
	char *l_c_Params;
	char *l_c_ParamsFin;
	unsigned char l_uc_TraitementTermine;
	unsigned char l_uc_PremierPassage;
	
	
	uc_EspionTacheEthernetTraiterActions = 1;
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_InitBufferTXRX(c_EthernetBufferTX2, sizeof(c_EthernetBufferTX2));
	vd_InitBufferTXRX(c_EthernetBufferRX2, sizeof(c_EthernetBufferRX2));


	// Verification code retour serveur :
	if(strstr(c_EthernetBufferRX, "HTTP/1.1 200 OK") == NULL)
	{
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraiterActions -> code retour serveur incorrect\n");
	}

	uc_EspionTacheEthernetTraiterActions = 10;
	l_uc_PremierPassage = 1;
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		// Traitement des actions
		l_uc_TraitementTermine = 0;
		uc_EspionTacheEthernetTraiterActions = 11;
		l_c_ParamsFin = c_EthernetBufferRX;	// Au 1er passage : traitement buffer complet, autres passage : à partir de la fin de l'action précédente
		while(l_uc_TraitementTermine == 0)
		{
			l_uc_TraitementTermine = 1;
			uc_EspionTacheEthernetTraiterActions = 12;
			l_sc_Retour = sc_JsonActions(l_c_ParamsFin, &l_c_Guid, &l_c_Params, &l_c_ParamsFin, l_uc_PremierPassage);
			l_uc_PremierPassage = 0;
			uc_EspionTacheEthernetTraiterActions = 13;
			switch(l_sc_Retour)
			{
				case sc_JSON_ACTION_PB_VALEUR:
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonActions pb valeur\n");
				break;
				case sc_JSON_ACTION_AUCUNE_ACTION:
					l_sc_Retour = sc_ETHERNET_RETOUR_OK;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonActions aucune action\n");
				break;
				case sc_JSON_ACTION_TROUVEE:
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonActions action trouvee -> %s\n", l_c_Guid);
					
					// Traitement action
					uc_EspionTacheEthernetTraiterActions = 14;
					l_sc_Retour = sc_TraitementAction(l_c_Guid, l_c_Params);//pppul_sock, 
					uc_EspionTacheEthernetTraiterActions = 15;
					if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
					{
						l_uc_TraitementTermine = 0;	// On reste dans la boucle car il y a peut etre une autre action à traiter
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction OK\n");
					}
					else
					{
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction %d\n", l_sc_Retour);
					}
					break;
				default:
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonActions %d ???\n", l_sc_Retour);
				break;
			}
		}
	}
	//_time_delay(2000);

	// Traitement alarme
	if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
	{
		uc_EspionTacheEthernetTraiterActions = 30;
		l_sc_Retour = sc_JsonAlarme(c_EthernetBufferRX, &l_c_Guid, &l_c_Obl);
		uc_EspionTacheEthernetTraiterActions = 31;
		switch(l_sc_Retour)
		{
			case sc_JSON_ALARME_PB_VALEUR:
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonAlarme pb valeur\n");
			break;
			case sc_JSON_ALARME_AUCUNE_ACTION:
				l_sc_Retour = sc_ETHERNET_RETOUR_OK;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonAlarme aucune action\n");
			break;
			case sc_JSON_ALARME_ACTION:
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonAlarme action trouvee\n");
				
				// Traitement alarme
				uc_EspionTacheEthernetTraiterActions = 32;
				l_sc_Retour = sc_TraitementAlarme(l_c_Guid, l_c_Obl);//pppul_sock, 
				uc_EspionTacheEthernetTraiterActions = 33;
				if(l_sc_Retour == sc_ETHERNET_RETOUR_OK)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme OK\n");
				}
				else
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"erreur TraitementAlarme %d\n", l_sc_Retour);
				}
				break;
			default:
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonAlarme %d ???\n", l_sc_Retour);
			break;
		}
	}
		
	uc_EspionTacheEthernetTraiterActions = 100;
	return l_sc_Retour;
}

// Analyse l'action pointée et renvoie l'acquittement pour cette action
signed char sc_TraitementAction(char *p_c_Guid, char *p_c_Params)//pppunsigned long ul_sock, 
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	signed long l_sl_Retour;
	unsigned long l_ul_Longueur;
	struct StructServerUpdateInformation l_st_InfosEnvoyees[uc_NB_MAX_INFOS_ENVOYEES_PAR_SERVEUR];
	unsigned char l_uc_NbInfosEnvoyees;
	unsigned char l_uc_Compteur;
	unsigned long l_ul_sock;
	unsigned char uc_ScenarioALancer;

	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	uc_EspionTacheEthernetTraitementAction = 1;
	vd_InitBufferTXRX(c_EthernetBufferTX2, sizeof(c_EthernetBufferTX2));
	vd_InitBufferTXRX(c_EthernetBufferRX2, sizeof(c_EthernetBufferRX2));

	uc_EspionTacheEthernetTraitementAction = 2;
	l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
	uc_EspionTacheEthernetTraitementAction = 3;
	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
	}
	else
	{
		// Prise en compte des valeurs reçues -> table d'échange,
		uc_EspionTacheEthernetTraitementAction = 10;
		l_uc_NbInfosEnvoyees = 0;
		l_sc_Retour = sc_JsonGetServerUpdateInformation(p_c_Params, l_st_InfosEnvoyees, &l_uc_NbInfosEnvoyees);
		uc_EspionTacheEthernetTraitementAction = 11;
		if(l_sc_Retour != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction erreur analyse %d\n", l_sc_Retour);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction data OK\n");
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tNbInfosDemandees : %d\n", l_uc_NbInfosEnvoyees);
			uc_ScenarioALancer = 0;
			for(l_uc_Compteur = 0;l_uc_Compteur < l_uc_NbInfosEnvoyees;l_uc_Compteur++)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tValeur(%d) : %d (%s) = %d\n", l_uc_Compteur, l_st_InfosEnvoyees[l_uc_Compteur].us_Indice, puc_AfficherLibelleTableEchange(l_st_InfosEnvoyees[l_uc_Compteur].us_Indice),l_st_InfosEnvoyees[l_uc_Compteur].uc_Donnee);
				if(l_st_InfosEnvoyees[l_uc_Compteur].us_Indice == Scenario)
				{
					// Gestion spécial si demande exécution scénario -> valeur à prendre en compte en dernier
					// Sinon risque de lancer le scénario avant d'avoir pris en compte sa configuration (vive le multitache !!!)
					uc_ScenarioALancer = l_st_InfosEnvoyees[l_uc_Compteur].uc_Donnee;
				}
				else
				{
					uc_TableEchange_Ecrit_Data(l_st_InfosEnvoyees[l_uc_Compteur].us_Indice, l_st_InfosEnvoyees[l_uc_Compteur].uc_Donnee, 1);
				}
			}
			if(uc_ScenarioALancer != 0)	//xxx verrouiller uniquement le scenario 1
			{
				uc_TableEchange_Ecrit_Data(Scenario, uc_ScenarioALancer, 1);	
			}
			l_sc_Retour = sc_ETHERNET_RETOUR_OK;
		
			// Acquittement de la demande
			uc_EspionTacheEthernetTraitementAction = 12;
			strcpy(c_EthernetBufferTX2, "POST /api/done/");
			strcat(c_EthernetBufferTX2, p_c_Guid);	// Ajout Guid pour acquittement
			strcat(c_EthernetBufferTX2, " HTTP/1.1\r\n");
			strcat(c_EthernetBufferTX2, c_EnteteTrameAccept());
			strcat(c_EthernetBufferTX2, c_EnteteTrameContentType());
			strcat(c_EthernetBufferTX2, c_EnteteTrameHost());
			strcat(c_EthernetBufferTX2, c_EnteteTrameCache());
			strcat(c_EthernetBufferTX2, c_EnteteTrameAcceptCharset());
			strcat(c_EthernetBufferTX2, "Content-Length: 0\r\n");
			strcat(c_EthernetBufferTX2, c_EnteteTrameAuthorisation());
			strcat(c_EthernetBufferTX2, c_EnteteTrameMatricule());
			strcat(c_EthernetBufferTX2, "\r\n");
			strcat(c_EthernetBufferTX2, "\r\n");
		
			l_ul_Longueur = strlen(c_EthernetBufferTX2);
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction TX\n");
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX2);
			uc_EspionTacheEthernetTraitementAction = 13;
			l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX2, l_ul_Longueur, 0);
			uc_EspionTacheEthernetTraitementAction = 14;
			
			if(l_sl_Retour == RTCS_ERROR)
			{
				uc_EspionTacheEthernetTraitementAction = 15;
				ul_DetailErreur = RTCS_geterror(l_ul_sock);
				uc_EspionTacheEthernetTraitementAction = 16;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction erreur TX %d\n", ul_DetailErreur);
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
			}
			else
			{
				l_sc_Retour = sc_ETHERNET_RETOUR_OK;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction TX OK\n");
				uc_EspionTacheEthernetTraitementAction = 17;
				l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX2, sizeof(c_EthernetBufferRX2)-1, 0);
				RTCS_time_delay(10);	//xxx pourquoi ???
				uc_EspionTacheEthernetTraitementAction = 18;
				if(l_sl_Retour == RTCS_ERROR)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction erreur RX pb lg\n");
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
				}
				else
				{
					c_EthernetBufferRX2[l_sl_Retour] = 0;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction RX\n");
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX2);
		
					// Verification code retour serveur :
					if(strstr(c_EthernetBufferRX2, "HTTP/1.1 201 Created") == NULL)
					{
						l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAction -> code retour serveur incorrect\n");
					}
				}
			}
		}
		uc_EspionTacheEthernetTraitementAction = 100;
		shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket
		uc_EspionTacheEthernetTraitementAction = 101;
	}
	
	uc_EspionTacheEthernetTraitementAction = 200;
	return l_sc_Retour;
}

// Analyse la demande alarme et renvoie l'acquittement pour cette action
signed char sc_TraitementAlarme(char *p_c_Guid, char *p_c_OrdreAlarme)//pppunsigned long ul_sock, 
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	signed long l_sl_Retour;
	unsigned long l_ul_Longueur;
	unsigned long l_ul_sock;

	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	uc_EspionTacheEthernetTraitementAlarme = 1;
	vd_InitBufferTXRX(c_EthernetBufferTX2, sizeof(c_EthernetBufferTX2));
	vd_InitBufferTXRX(c_EthernetBufferRX2, sizeof(c_EthernetBufferRX2));
	
	uc_EspionTacheEthernetTraitementAlarme = 2;
	l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
	uc_EspionTacheEthernetTraitementAlarme = 3;

	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
	}
	else
	{
		// Analyse ordre reçu
		uc_EspionTacheEthernetTraitementAlarme = 10;
		l_sc_Retour = sc_AnalyseOrdreAlarmeServeur(p_c_OrdreAlarme);
		uc_EspionTacheEthernetTraitementAlarme = 11;
		if(l_sc_Retour != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme erreur analyse %d\n", l_sc_Retour);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
		}
		else
		{
			l_sc_Retour = sc_ETHERNET_RETOUR_OK;

			// Acquittement de la demande
			uc_EspionTacheEthernetTraitementAlarme = 12;
			strcpy(c_EthernetBufferTX2, "POST /api/done/");
			strcat(c_EthernetBufferTX2, p_c_Guid);	// Ajout Guid pour acquittement
			strcat(c_EthernetBufferTX2, " HTTP/1.1\r\n");
			strcat(c_EthernetBufferTX2, c_EnteteTrameAccept());
			strcat(c_EthernetBufferTX2, c_EnteteTrameContentType());
			strcat(c_EthernetBufferTX2, c_EnteteTrameHost());
			strcat(c_EthernetBufferTX2, c_EnteteTrameCache());
			strcat(c_EthernetBufferTX2, c_EnteteTrameAcceptCharset());
			strcat(c_EthernetBufferTX2, "Content-Length: 0\r\n");
			strcat(c_EthernetBufferTX2, c_EnteteTrameAuthorisation());
			strcat(c_EthernetBufferTX2, c_EnteteTrameMatricule());
			strcat(c_EthernetBufferTX2, "\r\n");
			strcat(c_EthernetBufferTX2, "\r\n");

			l_ul_Longueur = strlen(c_EthernetBufferTX2);
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme TX\n");
			vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX2);
			uc_EspionTacheEthernetTraitementAlarme = 13;
			l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX2, l_ul_Longueur, 0);
			uc_EspionTacheEthernetTraitementAlarme = 14;
			if(l_sl_Retour == RTCS_ERROR)
			{
				uc_EspionTacheEthernetTraitementAlarme = 15;
				ul_DetailErreur = RTCS_geterror(l_ul_sock);
				uc_EspionTacheEthernetTraitementAlarme = 16;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme erreur TX %d\n", ul_DetailErreur);
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
			}
			else
			{
				l_sc_Retour = sc_ETHERNET_RETOUR_OK;
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme TX OK\n");
				uc_EspionTacheEthernetTraitementAlarme = 17;
				l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX2, sizeof(c_EthernetBufferRX2)-1, 0);
				RTCS_time_delay(10);	//xxx pourquoi ???
				uc_EspionTacheEthernetTraitementAlarme = 18;
				if(l_sl_Retour == RTCS_ERROR)
				{
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme erreur RX pb lg\n");
					l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
				}
				else
				{
					c_EthernetBufferRX2[l_sl_Retour] = 0;
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme RX\n");
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX2);
		
					// Verification code retour serveur :
					if(strstr(c_EthernetBufferRX2, "HTTP/1.1 201 Created") == NULL)
					{
						l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"TraitementAlarme -> code retour serveur incorrect\n");
					}
				}
			}
		}
		uc_EspionTacheEthernetTraitementAlarme = 100;
		shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket
		uc_EspionTacheEthernetTraitementAlarme = 101;
	}
	
	uc_EspionTacheEthernetTraitementAlarme = 200;
	return l_sc_Retour;
}

// Analyse l'ordre alarme envoyé par le serveur
// Retour 0 si OK ou < 0 si pb

// Valeur reçue complète :
// {"_de67f":{"guid":"806b4fc7-a820-4c49-9ae8-24ced8f6770f","obl":"73;178;187;105;197;154;208;248;26;52;219;233;77;251;102;182;53;27;198;207;97;29;242;49;143;6;2;114;203;240;75;156"},"actions":[]}
// Chaine ordre alarme passée en paramètre :
// 73;178;187;105;197;154;208;248;26;52;219;233;77;251;102;182;53;27;198;207;97;29;242;49;143;6;2;114;203;240;75;156;
// Décryptage :
// ALARMEON_20131210_055554

signed char sc_AnalyseOrdreAlarmeServeur(char *p_c_OrdreAlarme)
{
	signed char l_sc_Retour;
	char l_c_ChaineDecryptee[100];
	unsigned char l_uc_Compteur;
	struct stAESParse AESParse;
	RIJNDAEL_context Rij;
	unsigned char uc_CleServeur[33];
	unsigned short l_us_Compteur;
	
	
	l_sc_Retour = 0;

	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
	{
		uc_CleServeur[(l_uc_Compteur*2)] = ((uc_Cle_Acces_Distance[l_uc_Compteur]) & 0x0F) + '0';
		uc_CleServeur[(l_uc_Compteur*2)+1] = ((uc_Cle_Acces_Distance[l_uc_Compteur] >> 4) & 0x0F) + '0';
	}
	uc_CleServeur[32] = 0;

	
	strcat(p_c_OrdreAlarme,";");	// Ajoute un ";" pour prise en compte des dernieres valeurs (sans ce caractère, ne décrypte que 16 caractères)
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"AnalyseOrdreAlarmeServeur : %s\n", p_c_OrdreAlarme);
	
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(l_c_ChaineDecryptee); l_uc_Compteur++)
	{
		l_c_ChaineDecryptee[l_uc_Compteur] = 0;
	}
	
	AESParse = UnParceAES(p_c_OrdreAlarme);
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"AESParse : %d\n", AESParse.taille);
	for(l_us_Compteur=0;l_us_Compteur<255;l_us_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d.", AESParse.data[l_us_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n\n");
	
	Rij.mode=MODE_CBC;
	rijndael_setup(&Rij,32,(unsigned char *)uc_CleServeur);	//24

	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"uc_CleServeur :\n");
	for(l_us_Compteur=0;l_us_Compteur<33;l_us_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d.", uc_CleServeur[l_us_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n\n");
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Rij : %d - %d\n", Rij.nrounds, Rij.mode);
	for(l_us_Compteur=0;l_us_Compteur<60;l_us_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d.", Rij.keys[l_us_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n");
	for(l_us_Compteur=0;l_us_Compteur<60;l_us_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d.", Rij.ikeys[l_us_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n\n");

	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"c_CleDecryptageAlarme :\n");
	for(l_us_Compteur=0;l_us_Compteur<46;l_us_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%d.", c_CleDecryptageAlarme[l_us_Compteur]);
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n\n");
	
	block_decrypt(&Rij,(unsigned char *)AESParse.data,AESParse.taille,(unsigned char *)l_c_ChaineDecryptee,(unsigned char *)c_CleDecryptageAlarme);
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"AnalyseOrdreAlarmeServeur décryptage : %s\n", l_c_ChaineDecryptee);
	
	if(strstr(l_c_ChaineDecryptee, "ALARMEON") != NULL)
	{
		uc_DemandeServeurActiverAlarme = 1;
		uc_DemandeServeurCouperAlarme = 0;
	}
	else if(strstr(l_c_ChaineDecryptee, "ALARMEOFF") != NULL)
	{
		uc_DemandeServeurActiverAlarme = 0;
		uc_DemandeServeurCouperAlarme = 1;
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"PBANCDABCD\n");
		l_sc_Retour = -1;
	}

	return l_sc_Retour;
}

// Telecharge tout le nouveau soft sans rendre la main
// Retourne 0 si le nouveau soft a été entièrement téléchargé, ou < 0 si PB
signed char sc_Download(void)
{
	signed char l_sc_Retour;
	unsigned long ul_DetailErreur;
	unsigned long l_ul_sock;
	signed long l_sl_Retour;
	unsigned long l_ul_Longueur;
	unsigned char l_uc_Compteur;
	unsigned long l_ul_IndexTelechargement;
	char l_c_IndexTelechargement[10];
	char *l_pc_BinaireTelecharge;
	unsigned char l_uc_FinTraitement;
	unsigned short l_us_Compteur;

	
	l_sc_Retour = sc_ETHERNET_RETOUR_OK;
	vd_InitBufferTXRX(c_EthernetBufferTX, sizeof(c_EthernetBufferTX));
	vd_InitBufferTXRX(c_EthernetBufferRX, sizeof(c_EthernetBufferRX));

	for(l_us_Compteur = 0; l_us_Compteur ++; l_us_Compteur < uc_LigneS19_TAILLE)
	{
		uc_LigneS19[l_us_Compteur] = 0;
	}
	us_CompteurLigneS19 = 0;
	
	uc_EspionTacheEthernetDownload = 200;
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Ouverture zone FLASH...\n");
	l_sc_Retour = sc_OpenFlash();
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"sc_OpenFlash %d\n", l_sc_Retour);
	uc_EspionTacheEthernetDownload = 201;
	if(l_sc_Retour != 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur ouverture zone FLASH %d\n", l_sc_Retour);
		l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA; 
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Efface zone nouveau programme...\n");
		l_sc_Retour = sc_EffacerZoneNouveauProgramme();
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"sc_EffacerZoneNouveauProgramme %d\n", l_sc_Retour);
		uc_EspionTacheEthernetDownload = 201;
		if(l_sc_Retour != 0)
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur effacement zone nouveau programme %d\n", l_sc_Retour);
			l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA; 
		}
		else
		{
			uc_EspionTacheEthernetDownload = 1;
			l_sc_Retour = sc_SocketOpen(IPServeur, &ul_DetailErreur, &l_ul_sock);
			uc_EspionTacheEthernetDownload = 2;
			if(l_sc_Retour != 0)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur socket %d.%d\n", l_sc_Retour, ul_DetailErreur);
				l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS; 
			}
			else
			{
				l_ul_IndexTelechargement = 0;	// Debut du telechargement à l'index 0
				l_uc_FinTraitement = 0;
		
				while(l_uc_FinTraitement == 0)	// Téléchargement de tout le binaire
				{
					l_uc_FinTraitement = 1;	// On sort dans tous les cas, sauf si on décide de continuer...
					uc_EspionTacheEthernetDownload = 3;
					strcpy(c_EthernetBufferTX, "POST /api/getversioncontent/");
					sprintf(l_c_IndexTelechargement, "%d", l_ul_IndexTelechargement);
					strcat(c_EthernetBufferTX, l_c_IndexTelechargement);
					strcat(c_EthernetBufferTX, " HTTP/1.1\r\n");
					strcat(c_EthernetBufferTX, c_EnteteTrameAccept());
					strcat(c_EthernetBufferTX, c_EnteteTrameContentType());
					strcat(c_EthernetBufferTX, c_EnteteTrameHost());
					strcat(c_EthernetBufferTX, c_EnteteTrameCache());
					strcat(c_EthernetBufferTX, c_EnteteTrameAcceptCharset());
					strcat(c_EthernetBufferTX, "Content-Length: 0\r\n");
					strcat(c_EthernetBufferTX, c_EnteteTrameAuthorisation());
					strcat(c_EthernetBufferTX, c_EnteteTrameMatricule());
					strcat(c_EthernetBufferTX, "\r\n");
					strcat(c_EthernetBufferTX, "\r\n");
					uc_EspionTacheEthernetDownload = 4;
			
					l_ul_Longueur = strlen(c_EthernetBufferTX);
					vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download TX\n");
					vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX);
					uc_EspionTacheEthernetDownload = 5;
					l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX, l_ul_Longueur, 0);
					uc_EspionTacheEthernetDownload = 6;
					if(l_sl_Retour == RTCS_ERROR)
					{
						uc_EspionTacheEthernetDownload = 7;
						ul_DetailErreur = RTCS_geterror(l_ul_sock);
						uc_EspionTacheEthernetDownload = 8;
						vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur TX %d\n", ul_DetailErreur);
						l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
					}
					else
					{
						uc_EspionTacheEthernetDownload = 9;
						if(l_sl_Retour != l_ul_Longueur)
						{
							vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur TX pb lg\n");
							l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
						}
						else
						{
							vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download TX OK\n");
			
							// Reception de la réponse du serveur
							uc_EspionTacheEthernetDownload = 10;
							l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX, sizeof(c_EthernetBufferRX)-1, 0);
							RTCS_time_delay(10);	//xxx pourquoi ???
							uc_EspionTacheEthernetDownload = 11;
							if(l_sl_Retour == RTCS_ERROR)
							{
								vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur RX pb lg\n");
								l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
							}
							else
							{
								uc_EspionTacheEthernetDownload = 12;
								c_EthernetBufferRX[l_sl_Retour] = 0;
								vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download RX\n");
								vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX);
								
								// Traitement réponse serveur
								uc_EspionTacheEthernetDownload = 13;
								l_sc_Retour = sc_JsonGetDownloadInformation(c_EthernetBufferRX, (unsigned short)l_sl_Retour, &l_pc_BinaireTelecharge, &l_ul_IndexTelechargement);
								uc_EspionTacheEthernetDownload = 14;
								
								if(l_sc_Retour == 0)
								{
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download data OK\n");
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tBinaireTelecharge : %s\n", l_pc_BinaireTelecharge);
									//printf("BinaireTelecharge : %s\n", l_pc_BinaireTelecharge);
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,".");
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\tIndexTelechargement : %d\n", l_ul_IndexTelechargement);
									
									// Mémorisation en Flash
									l_sc_Retour = sc_FlashMemoriserBinaireRecu(l_pc_BinaireTelecharge);
									if(l_sc_Retour != 0)
									{
										vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Erreur écriture en FLASH %d\n", l_sc_Retour);
										l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
									}
									else
									{
										if(l_ul_IndexTelechargement != 0)	// Il en reste encore à télécharger -> on continue...
										{
											l_uc_FinTraitement = 0;
										}
										else
										{
											// Fin du téléchargement -> envoi acquittement version
											vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download Fin téléchargement -> envoi acquittement\n");
			
											strcpy(c_EthernetBufferTX, "POST /api/endversioncontent HTTP/1.1\r\n");
											strcat(c_EthernetBufferTX, c_EnteteTrameAccept());
											strcat(c_EthernetBufferTX, c_EnteteTrameContentType());
											strcat(c_EthernetBufferTX, c_EnteteTrameHost());
											strcat(c_EthernetBufferTX, c_EnteteTrameCache());
											strcat(c_EthernetBufferTX, c_EnteteTrameAcceptCharset());
											strcat(c_EthernetBufferTX, "Content-Length: 0\r\n");
											strcat(c_EthernetBufferTX, c_EnteteTrameAuthorisation());
											strcat(c_EthernetBufferTX, c_EnteteTrameMatricule());
											strcat(c_EthernetBufferTX, "\r\n");
											strcat(c_EthernetBufferTX, "\r\n");
											uc_EspionTacheEthernetDownload = 15;
											
											l_ul_Longueur = strlen(c_EthernetBufferTX);
											vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download TX\n");
											vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferTX);
											uc_EspionTacheEthernetDownload = 16;
											l_sl_Retour = send(l_ul_sock, c_EthernetBufferTX, l_ul_Longueur, 0);
											uc_EspionTacheEthernetDownload = 17;
			
											if(l_sl_Retour == RTCS_ERROR)
											{
												uc_EspionTacheEthernetDownload = 18;
												ul_DetailErreur = RTCS_geterror(l_ul_sock);
												uc_EspionTacheEthernetDownload = 19;
												vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur TX %d\n", ul_DetailErreur);
												l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
											}
											else
											{
												l_sc_Retour = sc_ETHERNET_RETOUR_OK;
												vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download TX OK\n");
												uc_EspionTacheEthernetDownload = 20;
												l_sl_Retour = recv(l_ul_sock, c_EthernetBufferRX, sizeof(c_EthernetBufferRX)-1, 0);
												RTCS_time_delay(10);	//xxx pourquoi ???
												uc_EspionTacheEthernetDownload = 21;
												if(l_sl_Retour == RTCS_ERROR)
												{
													vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur RX pb lg\n");
													l_sc_Retour = sc_ETHERNET_RETOUR_PB_RTCS;
												}
												else
												{
													c_EthernetBufferRX[l_sl_Retour] = 0;
													vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download RX\n");
													vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_TXRX,"%s\n", c_EthernetBufferRX);
										
													// Verification code retour serveur :
													if(strstr(c_EthernetBufferRX, "HTTP/1.1 200 OK") == NULL)
													{
														l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
														vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download -> code retour serveur incorrect\n");
													}
													else
													{
														// Telechargement terminé et acquitté -> contrôle zone nouveau
														vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"sc_CheckZoneNouveauProgramme\n");
														sc_CheckZoneNouveauProgramme();
													}
												}
												uc_EspionTacheEthernetDownload = 22;
											}
										}
									}								
								}
								else
								{
									vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"Download erreur analyse %d\n", l_sc_Retour);
									l_sc_Retour = sc_ETHERNET_RETOUR_PB_DATA;
								}
								uc_EspionTacheEthernetDownload = 23;
							}
						}
					}
				}
				uc_EspionTacheEthernetDownload = 50;
				shutdown(l_ul_sock, FLAG_ABORT_CONNECTION);	// Dans tous les cas fermeture de la socket
				uc_EspionTacheEthernetDownload = 51;
			}
			uc_EspionTacheEthernetDownload = 100;
		}
		uc_EspionTacheEthernetDownload = 101;
		
//		// Fichier FLASH ouvert -> a fermer dans tous les cas...
//		sc_CloseFlash();
	}
	uc_EspionTacheEthernetDownload = 102;

	// Fichier FLASH ouvert -> a fermer dans tous les cas...
	sc_CloseFlash();	// A Faire dans tous les cas...

	return l_sc_Retour;
}

char * c_EnteteTrameAccept(void)
{
	return "Accept: application/json,application/xhtml1+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
}
char * c_EnteteTrameHost(void)
{
	return "host: mon.essensys.fr\r\n";
}
char * c_EnteteTrameCache(void)
{
	return "Cache-Control: max-age=0\r\n";
}
char * c_EnteteTrameAcceptCharset(void)
{
	return "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n";
}
char * c_EnteteTrameAuthorisation(void)
{
	return "Authorization: Basic ";
}
char * c_EnteteTrameMatricule(void)
{
	return c_MatriculeCryptee;
}
char * c_EnteteTrameContentType(void)
{
	return "Content-type: application/json ;charset=UTF-8\r\n";
}

// xxx
// Alarme_AccesADistance,		// Autorise (1) ou non (0) la modification de l'état de l'alarme à distance
