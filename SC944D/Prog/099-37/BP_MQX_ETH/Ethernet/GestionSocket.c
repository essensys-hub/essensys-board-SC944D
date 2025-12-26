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

#define extern
#include "GestionSocket.h"
#undef extern


extern void EtatMemoire(void);

// Ouvre la socket avec le server dont l'adresse est spécifiée
// Retourne < 0 si erreur
#define sc_SOCKET_OPEN_ERREUR_CREATION_SOCKET	-1
#define sc_SOCKET_OPEN_ERREUR_CONFIG_TIME_OUT	-2
#define sc_SOCKET_OPEN_ERREUR_BIND				-3
#define sc_SOCKET_OPEN_ERREUR_OPEN_SOCKET		-4
signed char sc_SocketOpen(_ip_address AdresseIP, unsigned long *ul_DetailErreur, unsigned long *ul_Socket)
{
	signed char l_sc_Retour;
	unsigned long l_ul_Socket;
	unsigned long l_ul_Valeur;
	sockaddr_in l_Adresse;


	l_sc_Retour = 0;
	*ul_DetailErreur = 0;
	*ul_Socket = 0;
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"sc_SocketOpen start\n");
	EtatMemoire();
	
	uc_EspionTacheEthernetOpenSocket = 1;
	_io_socket_install("socket:");	// Install device driver for socket and Http ? xxx a faire a chaque fois ???
	uc_EspionTacheEthernetOpenSocket = 2;

	l_ul_Socket = socket(PF_INET, SOCK_STREAM, 0);
	uc_EspionTacheEthernetOpenSocket = 3;
	if(l_ul_Socket == RTCS_SOCKET_ERROR)
	{
		uc_EspionTacheEthernetOpenSocket = 4;
		*ul_DetailErreur = RTCS_geterror(l_ul_Socket);
		l_sc_Retour = sc_SOCKET_OPEN_ERREUR_CREATION_SOCKET;
		uc_EspionTacheEthernetOpenSocket = 5;
	}
	else
	{
		uc_EspionTacheEthernetOpenSocket = 6;
		//l_ul_Valeur = 60000;	// Time out socket : 1 minute
		l_ul_Valeur = 10000;	// Time out socket : 10 secondes
		*ul_DetailErreur = setsockopt(l_ul_Socket, SOL_TCP, OPT_CONNECT_TIMEOUT, &l_ul_Valeur, sizeof(uint_32));
		uc_EspionTacheEthernetOpenSocket = 7;
		if(*ul_DetailErreur != RTCS_OK)
		{
			uc_EspionTacheEthernetOpenSocket = 8;
			l_sc_Retour = sc_SOCKET_OPEN_ERREUR_CONFIG_TIME_OUT;
		}
		else
		{
			uc_EspionTacheEthernetOpenSocket = 9;
			l_Adresse.sin_family = AF_INET;
			l_Adresse.sin_port = 0;
			l_Adresse.sin_addr.s_addr = INADDR_ANY;
			*ul_DetailErreur = bind(l_ul_Socket, &l_Adresse, sizeof(l_Adresse));
			uc_EspionTacheEthernetOpenSocket = 10;
			if(*ul_DetailErreur != RTCS_OK)
			{
				uc_EspionTacheEthernetOpenSocket = 11;
				l_sc_Retour = sc_SOCKET_OPEN_ERREUR_BIND;
			}
			else
			{
				uc_EspionTacheEthernetOpenSocket = 12;
				l_Adresse.sin_port = IPPORT_HTTP;
				l_Adresse.sin_addr.s_addr = AdresseIP;
				uc_EspionTacheEthernetOpenSocket = 13;
				*ul_DetailErreur = connect(l_ul_Socket, &l_Adresse, sizeof(l_Adresse));
				uc_EspionTacheEthernetOpenSocket = 14;
				if(*ul_DetailErreur != RTCS_OK)
				{
					uc_EspionTacheEthernetOpenSocket = 15;
					shutdown(l_ul_Socket, FLAG_ABORT_CONNECTION);
					l_sc_Retour = sc_SOCKET_OPEN_ERREUR_OPEN_SOCKET;
				}
				else
				{
					uc_EspionTacheEthernetOpenSocket = 16;
					*ul_Socket = l_ul_Socket;
				}
			}
		}
	}
	uc_EspionTacheEthernetOpenSocket = 17;
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"sc_SocketOpen stop\n");
	EtatMemoire();
	
	return l_sc_Retour;
}
