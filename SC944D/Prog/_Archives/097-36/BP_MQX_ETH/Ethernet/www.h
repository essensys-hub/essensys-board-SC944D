
//#define MATRICULE_CRYPTEE						"NGFkOWY1MWMxYmRjZTlkODo5YjMxMDlhYmE0ZDRjMjg0"
//#define MATRICULE_CRYPTEE						"OTFlZjQyMDg2MDU0N2E3MDo3ZGJhZmJhNjQ1NmQzNDdj"
//#define MATRICULE								"6904037515351B78A85E26A0"

// Nouveau format mis en place en Janvier 2014
//#define MATRICULE		"7574 7440 3212 5457 8455 1162 8282 2684" // COMPTE CEDRIC.PALABOST@CPCODE.FR !! ESPACES RAJOUTER POUR FACILITER LA LECTURE - A ENLEVER POUR QUE CA MARCHE
// -> UTILISATION DE LA CLE LUE DANS L'EEPROM ADRESSE MAC

extern char c_MatriculeCryptee[46];	// xxx verifier taille
extern char c_CleDecryptageAlarme[46]; // xxx verifier taille

#define us_TRAITEMENT_ETHERNET_USER_CONNECTE_NON_MS				5000
#define us_TRAITEMENT_ETHERNET_USER_CONNECTE_OUI_MS				2000
#define us_TEMPO_INTER_ACTION_ETHERNET							50	//2000 pour les tests

#define us_BUFFER_RX1							1500	//xxx tailles a revalider
#define us_BUFFER_RX2							1024
#define us_BUFFER_TX							1024

extern char c_EthernetBufferRX[us_BUFFER_RX1];
extern char c_EthernetBufferRX2[us_BUFFER_RX2];
extern char c_EthernetBufferTX[us_BUFFER_TX];
extern char c_EthernetBufferTX2[us_BUFFER_TX];

#define uc_NB_MAX_INFOS_DEMANDEES_PAR_SERVEUR	30

#define ENET_DEVICE		0

// Adresses par défaut
#define IP_ADRESSE_PAR_DEFAUT	IPADDR(192,168,0,150)
#define IP_MASQUE_PAR_DEFAUT	IPADDR(255,255,255,0)
#define IP_GATEWAY_PAR_DEFAUT	IPADDR(0,0,0,0)	//IPADDR(192,168,0,254) ADSL ESTRABLIN

extern IPCFG_IP_ADDRESS_DATA IPConfig;

#define uc_CONFIG_IP_FIXE	0
#define uc_CONFIG_IP_DHCP	1
extern unsigned char uc_ConfigIP;

#define NOM_SERVEUR_ESSENSYS	"mon.essensys.fr"
extern _ip_address  IPServeur;


#define uc_NB_MAX_INFOS_DEMANDEES_PAR_SERVEUR	30
#define uc_NB_MAX_INFOS_ENVOYEES_PAR_SERVEUR	30

// Contient les informations recues du serveur en reponse a la trame GetInformationServer
struct StructServerInformation
{
	unsigned char uc_IsConnected;
	unsigned short us_NewVersion;
	unsigned short us_InfosDemandees[uc_NB_MAX_INFOS_DEMANDEES_PAR_SERVEUR];
	unsigned char uc_NbInfosDemandees;
};
extern struct StructServerInformation st_ServerInformation;

// Contient les valeurs reçues du serveur et a mettre dans la table d'échange
struct StructServerUpdateInformation
{
	unsigned short us_Indice;
	unsigned char uc_Donnee;
};

extern void vd_EthernetInitVariables(void);
extern void Ethernet_task(uint_32 dummy);
extern void vd_RTCSInit();
extern signed char sc_DialogueAvecServeur(void);
extern signed char sc_GetInformationServer(void);
extern signed char sc_PostInformationServer(void);
extern signed char sc_ActionManagment(void);
extern signed char sc_TraiterActions(void);//pppunsigned long ul_sock);
extern void vd_InitBufferTXRX(char *c_Buffer, unsigned short us_Taille);
extern signed char sc_TraitementAlarme(char *p_c_Guid, char *p_c_Obl);//pppunsigned long ul_sock, 
extern signed char sc_TraitementAction(char *p_c_Guid, char *p_c_Params);//pppunsigned long ul_sock, 
extern signed char sc_AnalyseOrdreAlarmeServeur(char *p_c_OrdreAlarme);
extern signed char sc_Download(void);

extern char * c_EnteteTrameAccept(void);
extern char * c_EnteteTrameHost(void);
extern char * c_EnteteTrameCache(void);
extern char * c_EnteteTrameAcceptCharset(void);
extern char * c_EnteteTrameAuthorisation(void);
extern char * c_EnteteTrameMatricule(void);
extern char * c_EnteteTrameContentType(void);

// Codes retour pour les fonctions suivantes :
//		sc_DialogueAvecServeur()
//		sc_GetInformationServer()
//		sc_PostInformationServer()
//		sc_ActionManagment()
//		sc_TraiterActions()
//		sc_TraitementAlarme()
//		sc_TraitementAction()
#define sc_ETHERNET_RETOUR_OK		0
#define sc_ETHERNET_RETOUR_PB_RTCS	-1
#define sc_ETHERNET_RETOUR_PB_DATA	-2

enum enumEthernetTraitement
{
	uc_EthernetPasDemarre,
	uc_EthernetReseauOK,
	uc_EthernetAdresseIPOK,
	uc_EthernetDNSOK
};

extern unsigned char uc_EthernetTraitement;	// Etat d'avancement du traitement Ethernet

// Utilisés par sc_FlashMemoriserBinaireRecu()
// uc_LigneS19 contient la ligne S19 réceptionnée pour stockage en flash
// De par le découpage effectué par le serveur, il se peut que l'on recoive une ligne S19 incomplète
// Dans ce cas, elle est conservée dans uc_LigneS19 jusqu'à ce qu'on reçoive la suite
#define uc_LigneS19_TAILLE		200
extern unsigned char uc_LigneS19[uc_LigneS19_TAILLE];
extern unsigned short us_CompteurLigneS19;
