#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include <string.h>
#include <rtcs.h>
#include <ipcfg.h>

// Contient toutes les fonctions qui permettent d'extraire les données reçues du serveur, et de créer les données à envoyer au serveur
#include <string.h>

#include "www.h"
#include "TableEchangeAcces.h"
#include "TableEchange.h"
#include "espionrs.h"
#include "global.h"

#define extern
#include "json.h"
#undef extern


// Analyse la réponse serveur sur la trame "GetServerInformation"
// Ressort les informations suivantes :
// 		IsConnected
//		NewVersion
//		Infos demandées par le serveur et à envoyer dans la trame PostInformationToServer
// Retourne 0 si OK ou <0 si pb
#define sc_JsonGetServerInformation_ERREUR_PAS_CODE_200				-1
#define sc_JsonGetServerInformation_ERREUR_ISCONNECTED				-2
#define sc_JsonGetServerInformation_ERREUR_ISCONNECTED_VALEUR		-3
#define sc_JsonGetServerInformation_ERREUR_NEWVERSION				-4
#define sc_JsonGetServerInformation_ERREUR_NEWVERSION_VALEUR		-5
#define sc_JsonGetServerInformation_ERREUR_INFOS					-6
#define sc_JsonGetServerInformation_ERREUR_INFOS_FIN_TRAME			-7
#define sc_JsonGetServerInformation_ERREUR_INFOS_VALEUR				-8
#define sc_JsonGetServerInformation_ERREUR_INFOS_NB_VALEURS_MAX		-9

// Réponse exemple :
// HTTP/1.1 200 OK
// Cache-Control: no-cache
// Pragma: no-cache
// Content-Type: application/json; charset=utf-8
// Expires: -1
// Server: Microsoft-IIS/7.5
// Set-Cookie: ASP.NET_SessionId=pys3cwtzh42ss10b3f4dfmvq; path=/; HttpOnly
// X-AspNet-Version: 4.0.30319
// X-Powered-By: ASP.NET
// Date: Tue, 19 Nov 2013 13:29:32 GMT
// Content-Length: 83
// ... plusieurs sauts de lignes
//{"isconnected":false,"infos":[360,346,347,348,349,350,8,404,405],"newversion":"no"}
//{"isconnected":false,"infos":[363,349,350,351,352,353,11,920],"newversion":"V1"}
//xxx a quoi sert us_NbOctetsDonnees ???
signed char sc_JsonGetServerInformation(char *pc_RxBuffer, unsigned short us_TailleBufferRx, unsigned short us_NbOctetsDonnees, struct StructServerInformation *pst_ServerInformation)
{
	signed char l_sc_Retour;
	char *l_pc_RxBuffer;
	unsigned short l_us_Valeur;
	unsigned char l_uc_CompteurDigit;
	unsigned short l_us_CompteurOctets;
		
	
	l_sc_Retour = 0;
	
	// Test retour serveur HTTP : code 200
	if(strstr(pc_RxBuffer, "HTTP/1.1 200 OK") == NULL)
	{
		l_sc_Retour = sc_JsonGetServerInformation_ERREUR_PAS_CODE_200;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonGetServerInf -> code retour serveur incorrect\n");
	}
	
	// Recherche l'information "isconnected"
	pst_ServerInformation->uc_IsConnected = 0;
	if(l_sc_Retour == 0)
	{
		if(strstr(pc_RxBuffer, "\"isconnected\":") != NULL)
		{
			if(strstr(pc_RxBuffer, "true") != NULL)
			{
				pst_ServerInformation->uc_IsConnected = 1;
			}
			else if(strstr(pc_RxBuffer, "false") != NULL)
			{
				pst_ServerInformation->uc_IsConnected = 0;
			}
			else
			{
				l_sc_Retour = sc_JsonGetServerInformation_ERREUR_ISCONNECTED_VALEUR;
			}
		}
		else
		{
			l_sc_Retour = sc_JsonGetServerInformation_ERREUR_ISCONNECTED;
		}
	}
	
	// Recherche l'information "NewVersion"
	pst_ServerInformation->us_NewVersion = 0;
	if(l_sc_Retour == 0)
	{
		l_pc_RxBuffer = strstr(pc_RxBuffer, "\"newversion\":\"");
		if(l_pc_RxBuffer != NULL)
		{
			l_pc_RxBuffer += 14;
			if(l_pc_RxBuffer[0] == 'n' && l_pc_RxBuffer[1] == 'o')
			{
				pst_ServerInformation->us_NewVersion = 0;
			}
			else
			{
				if(l_pc_RxBuffer[0] != 'V')
				{
					l_sc_Retour = sc_JsonGetServerInformation_ERREUR_NEWVERSION;
				}
				else
				{
					l_pc_RxBuffer++;
					l_us_Valeur = 0;
					l_uc_CompteurDigit = 0;
					while(l_uc_CompteurDigit < 5)	// 5 digits max (0 -> 65535)
					{
						if(l_pc_RxBuffer[l_uc_CompteurDigit] == '"')
						{
							l_uc_CompteurDigit = 5;
						}
						else
						{
							if(l_pc_RxBuffer[l_uc_CompteurDigit] >= '0' && l_pc_RxBuffer[l_uc_CompteurDigit] <= '9')
							{
								l_us_Valeur = l_us_Valeur * 10;
								l_us_Valeur = l_us_Valeur + (unsigned short)(l_pc_RxBuffer[l_uc_CompteurDigit] - '0');
							}
							else
							{
								l_uc_CompteurDigit = 5;
								l_sc_Retour = sc_JsonGetServerInformation_ERREUR_NEWVERSION_VALEUR;
							}
						}
						l_uc_CompteurDigit++;
					}
					pst_ServerInformation->us_NewVersion = l_us_Valeur;
				}
			}
		}
		else
		{
			l_sc_Retour = sc_JsonGetServerInformation_ERREUR_NEWVERSION;
		}
	}
	
	// Recherche informations demandées
	pst_ServerInformation->uc_NbInfosDemandees = 0;
	if(l_sc_Retour == 0)
	{
		l_pc_RxBuffer = strstr(pc_RxBuffer, "\"infos\":[");
		if(l_pc_RxBuffer != NULL)
		{
			l_pc_RxBuffer += 9;
			if(strstr(l_pc_RxBuffer, "]") == NULL)
			{
				l_sc_Retour = sc_JsonGetServerInformation_ERREUR_INFOS_FIN_TRAME;
			}
			else
			{
				pst_ServerInformation->uc_NbInfosDemandees = 0;
				l_us_Valeur = 0;
				l_us_CompteurOctets = 0;
				l_uc_CompteurDigit = 0;
				while(l_us_CompteurOctets < us_TailleBufferRx)
				{
					if(l_pc_RxBuffer[l_us_CompteurOctets] == ']' || l_pc_RxBuffer[l_us_CompteurOctets] == ',')
					{
						if(pst_ServerInformation->uc_NbInfosDemandees < uc_NB_MAX_INFOS_DEMANDEES_PAR_SERVEUR &&
						   l_uc_CompteurDigit <= 5)
						{
							pst_ServerInformation->us_InfosDemandees[pst_ServerInformation->uc_NbInfosDemandees] = l_us_Valeur;
							pst_ServerInformation->uc_NbInfosDemandees++;
							l_us_Valeur = 0;
							l_uc_CompteurDigit = 0;
							if(l_pc_RxBuffer[l_us_CompteurOctets] == ']')	l_us_CompteurOctets = us_TailleBufferRx;	// Sortie traitement
						}
						else
						{
							l_sc_Retour = sc_JsonGetServerInformation_ERREUR_INFOS_NB_VALEURS_MAX;
							l_us_CompteurOctets = us_TailleBufferRx;	// Sortie traitement
						}
					}
					else if(l_pc_RxBuffer[l_us_CompteurOctets] >= '0' && l_pc_RxBuffer[l_us_CompteurOctets] <= '9')
					{
						l_us_Valeur = l_us_Valeur * 10;
						l_us_Valeur = l_us_Valeur + (unsigned short)(l_pc_RxBuffer[l_us_CompteurOctets] - '0');
						l_uc_CompteurDigit++;
					}
					else
					{
						l_us_CompteurOctets = us_TailleBufferRx;	// Sortie traitement
						l_sc_Retour = sc_JsonGetServerInformation_ERREUR_INFOS_VALEUR;
					}
					l_us_CompteurOctets++;
				}
			}
		}
		else
		{
			l_sc_Retour = sc_JsonGetServerInformation_ERREUR_INFOS;
		}
	}
	return l_sc_Retour;
}

// Recopie dans la chaine passée en paramètre les valeurs demandées dans pst_ServerInformation
// Retourne 0 si OK, ou -1 si nb valeurs > taille buffer
signed char sc_JsonPostServerInformation(char *pc_RxBuffer, unsigned short us_TailleBuffer, struct StructServerInformation *pst_ServerInformation)
{
	signed char l_sc_Retour;
	char l_c_Buffer[50];	//xxx a redimentionner
	char lc_Chaine9Caracteres[9];
	unsigned char l_uc_CompteurValeurs;
	
	l_sc_Retour = 0;
	
	// Copie version BP
	sprintf(l_c_Buffer, "{\"version\":\"V%d\",\"ek\":[",us_BP_VERSION_SERVEUR);	//xxxBP_VERSION_ETHERNET);
	strcpy(pc_RxBuffer, l_c_Buffer);
	
	for(l_uc_CompteurValeurs = 0;l_uc_CompteurValeurs < pst_ServerInformation->uc_NbInfosDemandees;l_uc_CompteurValeurs++)
	{
		// Pour les valeurs suivantes -> renvoyer l'état de chaque bit sous forme binaire : 01001100 - Toujours 8 caractères
		if(pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs] == Alerte ||
		   pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs] == EtatBP1 ||
		   pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs] == EtatBP2)
		{
			vd_ConvertirOctetEnChaineBinaire(uc_TableEchange_Lit_Data(pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs]), lc_Chaine9Caracteres);
			sprintf(l_c_Buffer, "{k:%d,v:\"%s\"}", pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs], lc_Chaine9Caracteres);
		}
		else
		{
			sprintf(l_c_Buffer, "{k:%d,v:\"%d\"}", pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs], uc_TableEchange_Lit_Data(pst_ServerInformation->us_InfosDemandees[l_uc_CompteurValeurs]));
		}
		if((strlen(pc_RxBuffer) + strlen(l_c_Buffer)) < us_TailleBuffer)
		{
			strcat(pc_RxBuffer, l_c_Buffer);
			if(l_uc_CompteurValeurs < (pst_ServerInformation->uc_NbInfosDemandees-1))	strcat(pc_RxBuffer, ",");
		}
		else
		{
			l_uc_CompteurValeurs = pst_ServerInformation->uc_NbInfosDemandees;
			l_sc_Retour = -1;
		}
	}
	strcat(pc_RxBuffer, "]}");
	//xxx gerer cas aucune valeur a envoyer
	//xxx , apres la derniere valeur
	return l_sc_Retour;
}

void vd_ConvertirOctetEnChaineBinaire(unsigned char uc_Valeur, char *c_Chaine9Caracteres)
{
	unsigned char l_uc_Compteur;
	unsigned char l_uc_Masque;
	
	l_uc_Masque = 1;
	for(l_uc_Compteur = 0;l_uc_Compteur < 8;l_uc_Compteur++)
	{
		if((uc_Valeur & l_uc_Masque) == 0)	c_Chaine9Caracteres[l_uc_Compteur] = '0';
		else								c_Chaine9Caracteres[l_uc_Compteur] = '1';
		l_uc_Masque = l_uc_Masque << 1;
	}
	c_Chaine9Caracteres[8] = 0;
}

// Recherche dans la chaine passee en paramètre l'etiquette correspondant à l'action alarme
// Renvoie si trouvé l'indice de debut du guid (pour envoi acquittement, l'indice pointe sur le 1er ") - \0 rajouté juste après le " de fin
// Et l'indice de début du obl (contenant l'ordre reçu, l'indice pointe après le 1er ") - \0 rajouté sur le " de fin
// Retour l'une des valeurs suivantes :
//#define sc_JSON_ALARME_PB_VALEUR		1	// une des étiquettes recherchées est manquante
//#define sc_JSON_ALARME_AUCUNE_ACTION	2	// pas d'action alarme
//#define sc_JSON_ALARME_ACTION			3	// action alarme trouvée

// {"_de67f":null,
// {"_de67f":{"guid":"2a1a9ea2-941d-456f-9b0a-7e179cf3e864","obl":""},
// {"_de67f":{"guid":"3edb2305-2d47-43b3-a2ab-68a6cf6982ab","obl":"124;183;53;98;127;22;30;199;214;125;179;100;210;211;84;17;196;179;195;188;11;140;250;168;22;137;29;30;228;172;183;7"},}
signed char sc_JsonAlarme(char *c_EthernetBufferRX, char *p_c_Guid[], char *p_c_Obl[])
{
	signed char l_sc_Retour;
	char *l_pc_RxBuffer;
	
	
	l_sc_Retour = sc_JSON_ALARME_PB_VALEUR;
	
	// Recherche de l'étiquette {"_de67f":
	if(strstr(c_EthernetBufferRX, "{\"_de67f\":") == NULL)
	{
		l_sc_Retour = sc_JSON_ALARME_PB_VALEUR;
	}
	else
	{
		if(strstr(c_EthernetBufferRX, "{\"_de67f\":null,") != NULL)
		{
			l_sc_Retour = sc_JSON_ALARME_AUCUNE_ACTION;
		}
		else
		{
			l_pc_RxBuffer = strstr(c_EthernetBufferRX, "{\"_de67f\":{\"guid\":\"");
			if(l_pc_RxBuffer == NULL)
			{
				l_sc_Retour = sc_JSON_ALARME_PB_VALEUR;
			}
			else
			{
				// Guid trouvé -> retourne position début Guid (avec guillemet)
				*p_c_Guid = l_pc_RxBuffer + 19;
				
				// Recherche fin Guid et début obl
				l_pc_RxBuffer = strstr(l_pc_RxBuffer, "\",\"obl\":\"");
				if(l_pc_RxBuffer == NULL)
				{
					l_sc_Retour = sc_JSON_ALARME_PB_VALEUR;
				}
				else
				{
					// obl trouvé -> retourne position début olb (avec guillemet) + \0 à la fin du guid
					*p_c_Obl = l_pc_RxBuffer + 9;
					l_pc_RxBuffer[0] = 0;
					l_pc_RxBuffer++;	// Pour sauter le 0...
					
					// Recherche fin obl
					l_pc_RxBuffer = strstr(l_pc_RxBuffer, "\"},");
					if(l_pc_RxBuffer == NULL)
					{
						l_sc_Retour = sc_JSON_ALARME_PB_VALEUR;
					}
					else
					{
						// fin old trouvé -> met \0 à la fin
						l_pc_RxBuffer[0] = 0;
						l_sc_Retour = sc_JSON_ALARME_ACTION;
					}
				}
			}
		}
	}
	return l_sc_Retour;
}

// Recherche dans la chaine passee en paramètre l'etiquette correspondant aux actions (si demandé)
// Renvoie si trouvé l'indice de debut du guid (pour envoi acquittement, l'indice pointe sur le 1er ") - \0 rajouté à la fin du guid (sur le " de fin)
// Renvoie l'indice de debut des valeurs à traiter (params) -> indice sur le 1er { - \0 rajouté à la fin de params (sur le ] de fin)
// Ainsi que l'indice pointant sur la fin de l'action trouvée sur le virgule après les params
// Retour l'une des valeurs suivantes :
//#define sc_JSON_ACTION_PB_VALEUR		1	// une des étiquettes recherchées est manquante
//#define sc_JSON_ACTION_AUCUNE_ACTION	2	// pas d'action
//#define sc_JSON_ACTION_TROUVEE		3	// action trouvée

//,"actions":[]}

//,"actions":
//[
//{"guid":"ec9026fe-25fc-4b2f-b4b0-c5402699f399","params":[{"k":363,"v":"255"}]},
//{"guid":"53ade398-8083-42d2-ba60-49b714b644bf","params":[{"k":349,"v":"1"}]},
//{"guid":"a62d78fa-001e-44aa-8266-f00037045b78","params":[{"k":350,"v":"1"}]},
//{"guid":"64ab402d-ad04-4445-9da3-150881f8975c","params":[{"k":351,"v":"1"}]},
//{"guid":"d57cc570-dd43-4a97-ade3-de3626362a70","params":[{"k":352,"v":"1"}]},
//{"guid":"dbaf90b9-9237-4271-9332-16ebec526ef0","params":[{"k":353,"v":"1"}]}
//]
//}

signed char sc_JsonActions(char *c_EthernetBufferRX, char *p_c_Guid[], char *p_c_Params[], char *p_c_ParamsFin[], unsigned char uc_PremierPassage)
{
	signed char l_sc_Retour;
	char *l_pc_RxBuffer;
	
	
	l_sc_Retour = sc_JSON_ACTION_PB_VALEUR;
	l_pc_RxBuffer = NULL;
	
	//printf("sc_JsonActions : %s\n", c_EthernetBufferRX);
	
	// Recherche de l'étiquette ,"actions":
	if(strstr(c_EthernetBufferRX, ",\"actions\":") == NULL && uc_PremierPassage != 0)	// Vérifié si 1er passage
	{
		l_sc_Retour = sc_JSON_ACTION_PB_VALEUR;
	}
	else
	{
		l_pc_RxBuffer = strstr(c_EthernetBufferRX, ",\"actions\":[]}");
		if(l_pc_RxBuffer != NULL && uc_PremierPassage != 0)	// Vérifié si 1er passage
		{
			l_sc_Retour = sc_JSON_ACTION_AUCUNE_ACTION;
		}
		else
		{
			if(uc_PremierPassage != 0)
			{	// On traite a partir de action (avant il peut y avoir un guid pour l'alarme qu'il ne faut pas prendre)
				l_pc_RxBuffer = strstr(c_EthernetBufferRX, ",\"actions\":[");
			}
			else
			{	// On prend le buffer à son début
				l_pc_RxBuffer = c_EthernetBufferRX;
			}
			if(l_pc_RxBuffer != NULL)	// Forcement != de NULL à ce stade mais on se sait jamais...
			{
				l_pc_RxBuffer = strstr(l_pc_RxBuffer, "{\"guid\":\"");
			}
			if(l_pc_RxBuffer == NULL)
			{
				l_pc_RxBuffer = c_EthernetBufferRX;
				if(strstr(l_pc_RxBuffer, "}]}") == NULL)	// Détection fin demande actions
				{
					l_sc_Retour = sc_JSON_ACTION_PB_VALEUR;	
				}
				else
				{
					l_sc_Retour = sc_JSON_ACTION_AUCUNE_ACTION;
				}				
			}
			else
			{
				// Guid trouvé -> retourne position début Guid (avec guillemet)
				*p_c_Guid = l_pc_RxBuffer + 9;
				
				// Recherche fin Guid et début params
				l_pc_RxBuffer = strstr(l_pc_RxBuffer, "\",\"params\":[");
				if(l_pc_RxBuffer == NULL)
				{
					l_sc_Retour = sc_JSON_ACTION_PB_VALEUR;
				}
				else
				{
					// params trouvé -> retourne position début params (avec guillemet) + \0 à la fin du guid
					l_pc_RxBuffer[0] = 0;
					l_pc_RxBuffer++;	// Pour sauter le 0...
					*p_c_Params = l_pc_RxBuffer + 11;
					
					// Recherche fin params
					l_pc_RxBuffer = strstr(l_pc_RxBuffer, "}]}");
					if(l_pc_RxBuffer == NULL)
					{
						l_sc_Retour = sc_JSON_ACTION_PB_VALEUR;
					}
					else
					{
						// fin params trouvé
						l_pc_RxBuffer[1] = 0;
						l_pc_RxBuffer+=2;	// Pour sauter le 0...
						*p_c_ParamsFin = l_pc_RxBuffer;
						l_sc_Retour = sc_JSON_ACTION_TROUVEE;
					}
				}
			}
		}
	}
	return l_sc_Retour;
}

// {"k":363,"v":"255"},{"k":364,"v":"255"}
//                    _
signed char sc_JsonGetServerUpdateInformation(char *pc_Params, struct StructServerUpdateInformation st_InfosDemandees[], unsigned char *uc_NbInfosDemandees)
{
	signed char l_sc_Retour;
	//char *l_pc_RxBuffer;
	unsigned long l_ul_Indice;
	unsigned short l_us_Valeur;
	unsigned char l_uc_CompteurDigit;
	unsigned char uc_FlagSortie;
		
	
	l_sc_Retour = 0;
	*uc_NbInfosDemandees = 0;
	uc_FlagSortie = 0;
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx : %s\n",pc_Params);
	
	// Recherche informations envoyées
	while(uc_FlagSortie == 0 && l_sc_Retour == 0)
	{
		if(pc_Params[0] != '{')
		{
			l_sc_Retour = -1;
		}
		else
		{
			pc_Params++;
			if(pc_Params[0] != '"' ||
			   pc_Params[1] != 'k' ||
			   pc_Params[2] != '"' ||
			   pc_Params[3] != ':')
			{
				l_sc_Retour = -2;
			}
			else
			{
				pc_Params += 4;
				l_ul_Indice = 0;
				l_uc_CompteurDigit = 0;
				while(l_uc_CompteurDigit < 5 && pc_Params[0] != 0)	// Boucle récup indice
				{
					if(pc_Params[0] == ',')
					{
						l_uc_CompteurDigit = 5;	// Sortie boucle récup indice
					}
					else if(pc_Params[0] >= '0' && pc_Params[0] <= '9')
					{
						l_ul_Indice = l_ul_Indice * 10;
						l_ul_Indice = l_ul_Indice + (unsigned long)(pc_Params[0] - '0');
						pc_Params++;
					}
					else
					{
						l_sc_Retour = -3;		// Indice incorrecte
						l_uc_CompteurDigit = 5;	// Sortie boucle récup indice
					}
					l_uc_CompteurDigit++;
				}
				if(l_sc_Retour == 0)
				{
					if(pc_Params[0] != ',' ||
					   pc_Params[1] != '"' ||
					   pc_Params[2] != 'v' ||
					   pc_Params[3] != '"' ||
					   pc_Params[4] != ':' ||
					   pc_Params[5] != '"')
					{
						l_sc_Retour = -4;
					}
					else
					{
						pc_Params += 6;
						l_us_Valeur= 0;
						l_uc_CompteurDigit = 0;
						while(l_uc_CompteurDigit < 3 && pc_Params[0] != 0)	// Boucle récup valeur
						{
							if(pc_Params[0] == '"' && pc_Params[1] == '}')
							{
								l_uc_CompteurDigit = 5;	// Sortie boucle récup valeur
							}
							else if(pc_Params[0] >= '0' && pc_Params[0] <= '9')
							{
								l_us_Valeur = l_us_Valeur * 10;
								l_us_Valeur = l_us_Valeur + (unsigned short)(pc_Params[0] - '0');
								pc_Params++;
							}
							else
							{
								l_sc_Retour = -5;		// Indice incorrecte
								l_uc_CompteurDigit = 5;	// Sortie boucle récup indice
							}
							l_uc_CompteurDigit++;
						}
						if(l_sc_Retour == 0)
						{
							if(l_ul_Indice >= Nb_Tbb_Donnees)	l_sc_Retour = -6;
							else if(l_us_Valeur > 255)			l_sc_Retour = -7;
							else
							{
								st_InfosDemandees[*uc_NbInfosDemandees].us_Indice = (unsigned short)l_ul_Indice;
								st_InfosDemandees[*uc_NbInfosDemandees].uc_Donnee = (unsigned char)l_us_Valeur;
								(*uc_NbInfosDemandees)++;
								
								// Vérification autre valeur ?
								pc_Params += 2;
								if(pc_Params[0] == ',')
								{
									pc_Params++;	// Valeur suivante...
								}
								else if(pc_Params[0] == 0)
								{
									// Plus de valeurs
									uc_FlagSortie = 1;
								}
								else
								{
									l_sc_Retour = -8;
								}
							}
						}
					}
				}
			}
		}
	}
	return l_sc_Retour;
}

signed char sc_JsonGetDownloadInformation(char *pc_RxBuffer, unsigned short us_NbOctetsDonnees, char *pc_BinaireTelecharge[], unsigned long *pul_IndexTelechargement)
{
	signed char l_sc_Retour;
	char *l_pc_RxBuffer;
	unsigned short l_us_Valeur;
	unsigned char l_uc_CompteurDigit;
	unsigned short l_us_CompteurOctets;
		
	
	l_sc_Retour = 0;
	*pul_IndexTelechargement = 0;

	// Test retour serveur HTTP : code 206
	if(strstr(pc_RxBuffer, "HTTP/1.1 200 OK") == NULL)
	{
		l_sc_Retour = -1;
		vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"JsonGetDownloadInformation -> code retour serveur incorrect\n");
	}

	// Recherche l'information "content"
	l_pc_RxBuffer = strstr(pc_RxBuffer, "content\":\"");
	if(l_pc_RxBuffer == NULL)
	{
		l_sc_Retour = -2;
	}
	else
	{
		l_pc_RxBuffer += 10;
		*pc_BinaireTelecharge = l_pc_RxBuffer;
		
		// Recherche de "nextindex"
		l_pc_RxBuffer = strstr(l_pc_RxBuffer, "\",\"nextindex\":");
		if(l_pc_RxBuffer == NULL)
		{
			l_sc_Retour = -3;
		}
		else
		{
			l_pc_RxBuffer[0] = 0;
			l_pc_RxBuffer += 14;
			
			// Recuperation de la valeur du nextindex
			*pul_IndexTelechargement = 0;
			l_uc_CompteurDigit = 0;
			while(l_uc_CompteurDigit < 6)	// 9 digits max (0 -> 999999)
			{
				if(l_pc_RxBuffer[l_uc_CompteurDigit] == '}')
				{
					l_uc_CompteurDigit = 6;
				}
				else
				{
					if(l_pc_RxBuffer[l_uc_CompteurDigit] >= '0' && l_pc_RxBuffer[l_uc_CompteurDigit] <= '9')
					{
						*pul_IndexTelechargement = *pul_IndexTelechargement * 10;
						*pul_IndexTelechargement = *pul_IndexTelechargement + (unsigned long)(l_pc_RxBuffer[l_uc_CompteurDigit] - '0');
					}
					else
					{
						l_uc_CompteurDigit = 6;
						l_sc_Retour = -4;
					}
				}
				l_uc_CompteurDigit++;
			}
		}
	}
	return l_sc_Retour;
}

