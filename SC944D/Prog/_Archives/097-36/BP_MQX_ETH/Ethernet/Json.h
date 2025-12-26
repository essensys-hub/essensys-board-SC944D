extern signed char sc_JsonGetServerInformation(char *pc_RxBuffer, unsigned short us_TailleBufferRx, unsigned short us_NbOctetsDonnees, struct StructServerInformation *pst_ServerInformation);
extern signed char sc_JsonPostServerInformation(char *pc_RxBuffer, unsigned short us_TailleBuffer, struct StructServerInformation *pst_ServerInformation);
extern void vd_ConvertirOctetEnChaineBinaire(unsigned char uc_Valeur, char *c_Chaine9Caracteres);
extern signed char sc_JsonAlarme(char *c_EthernetBufferRX, char *p_c_Guid[], char *p_c_Obl[]);
extern signed char sc_JsonActions(char *c_EthernetBufferRX, char *p_c_Guid[], char *p_c_Params[], char *p_c_ParamsFin[], unsigned char uc_PremierPassage);
extern signed char sc_JsonGetServerUpdateInformation(char *pc_Params, struct StructServerUpdateInformation st_InfosDemandees[], unsigned char *uc_NbInfosDemandees);
extern signed char sc_JsonGetDownloadInformation(char *pc_RxBuffer, unsigned short us_NbOctetsDonnees, char *c_BinaireTelecharge[], unsigned long *pul_IndexTelechargement);

// Codes retour de la fonction sc_JsonAlarme
#define sc_JSON_ALARME_PB_VALEUR		1	// une des étiquettes recherchées est manquante
#define sc_JSON_ALARME_AUCUNE_ACTION	2	// pas d'action alarme
#define sc_JSON_ALARME_ACTION			3	// action alarme trouvée

// Codes retour de la fonction sc_JsonActions
#define sc_JSON_ACTION_PB_VALEUR		1	// une des étiquettes recherchées est manquante
#define sc_JSON_ACTION_AUCUNE_ACTION	2	// pas d'action
#define sc_JSON_ACTION_TROUVEE			3	// action trouvée
