
#ifdef DEBUG

extern void vd_EspionRS_Init(void);
extern unsigned char vd_EspionMainActive(void);

extern void vd_EspionRS_Printf(unsigned char uc_Source, const char _PTR_ String, ...);

extern void vd_EspionRS_PrintfSansHorodatage(unsigned char uc_Source, const char _PTR_ String, ...);
extern void vd_EspionRS_PrintfLibelleTypeTexte(unsigned char Source);
extern void TypeTexte(char *pc_Texte);
extern char * puc_EspionFiltreEtat(unsigned char uc_Numero);
extern void vd_EspionRS(void);
extern void vd_GestionMenuPrincipal(unsigned char uc_Caractere);
extern void vd_GestionMenuMessagesEvenementiels(unsigned char uc_Caractere);
extern void vd_GestionMenuAfficherEspions(unsigned char uc_Caractere);
extern void vd_GestionMenuRAZEspions(unsigned char uc_Caractere);
extern void vd_GestionMenuActions(unsigned char uc_Caractere);
extern void vd_EspionRS_Afficher_Teleinfo(unsigned char uc_AffichageForce);
extern void vd_EspionRS_Afficher_ErreursTacheEcran(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursTacheEcran(void);
extern void vd_EspionRS_Afficher_Teleinfo(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_Teleinfo(void);
extern void vd_EspionRS_Afficher_ActiviteTacheEcran(void);
extern void vd_EspionRS_RAZ_ActiviteTacheEcran(void);
extern void vd_EspionRS_Afficher_ErreursDialogueBA(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursDialogueBA(void);
extern void vd_EspionRS_Afficher_ErreursTacheBA(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursTacheBA(void);
extern void vd_EspionRS_Afficher_ActiviteTacheBA(void);
extern void vd_EspionRS_RAZ_ActiviteTacheBA(void);
extern void vd_EspionRS_Afficher_ActiviteI2C(void);
extern void vd_EspionRS_RAZ_ActiviteI2C(void);
extern void vd_EspionRS_RAZ_Scenarios(void);
extern void vd_EspionRS_Afficher_Scenarios(void);
extern void vd_EspionRS_Afficher_ErreursAlarme(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursAlarme(void);
extern void vd_EspionRS_Afficher_ActiviteAlarme(void);
extern void vd_EspionRS_RAZ_ActiviteAlarme(void);
extern void vd_EspionRS_Afficher_ErreursTachePrincipale(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursTachePrincipale(void);
extern void vd_EspionRS_Afficher_ActiviteTachePrincipale(void);
extern void vd_EspionRS_RAZ_ActiviteTachePrincipale(void);
extern void vd_EspionRS_Afficher_ErreursRTC(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursRTC(void);
extern void vd_EspionRS_Afficher_ActiviteRTC(void);
extern void vd_EspionRS_RAZ_ActiviteRTC(void);
extern void vd_EspionRS_Afficher_ErreursHard(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursHard(void);
extern void vd_EspionRS_Afficher_ErreursChauffage(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_ErreursChauffage(void);
extern void vd_EspionRS_Afficher_ActiviteChauffage(void);
extern void vd_EspionRS_RAZ_ActiviteChauffage(void);
extern void vd_EspionRS_Afficher_TableEchange(unsigned char uc_AffichageForce);
extern void vd_EspionRS_Afficher_EtatBP(unsigned char uc_AffichageForce);
extern void vd_EspionRS_Afficher_ES(unsigned char uc_AffichageForce);

extern void vd_PrintTableEchange(void);
extern void vd_PrintValeurTableEchange(unsigned short us_Indice);
extern void vd_PrintValeurTableEchangePlanningChauffage(const char *pc_Libelle, unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeConfigAlarme(const char *pc_Libelle, unsigned short us_Indice, unsigned char uc_ConfigScenario);
extern void vd_PrintValeurTableEchangePlanningArrosage(const char *pc_Libelle, unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeVariateurs(const char *pc_Libelle, unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeLampes(const char *pc_Libelle, unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeVolets(const char *pc_Libelle, unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeCleAccesDistance(void);
extern const char *puc_LibelleScenario(unsigned short us_Indice);
extern void vd_PrintValeurTableEchangeScenario(const char *pc_Libelle, unsigned short us_Indice);
extern unsigned char *puc_AfficherLibelleTableEchange(unsigned short us_Indice);
extern unsigned char *puc_AfficherLibelleConfigAlarme(unsigned short us_Indice);
extern unsigned char *puc_AfficherLibelleConfigScenario(unsigned short us_Indice);
extern unsigned char *puc_AfficherLibelleAvecEspaces(const char *pc_Libelle);
extern unsigned char *puc_AfficherLibelleAvecEspaces2(const char *pc_Libelle, unsigned short us_Indice);

extern void vd_EspionRS_Afficher_EthernetActivite(unsigned char uc_AffichageForce);
extern void vd_EspionRS_RAZ_EthernetActivite(void);

#else

//xxx ca passe mais ce n'est pas tres propre...
#define vd_EspionRS_Printf
#define vd_EspionRS_PrintfSansHorodatage
#define puc_AfficherLibelleTableEchange

#endif
