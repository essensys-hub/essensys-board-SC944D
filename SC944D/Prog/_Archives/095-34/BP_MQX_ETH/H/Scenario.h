
extern unsigned char uc_Inv_Octet(unsigned char uc_depart);
extern void vd_Scenario_Chauffage(unsigned char uc_ChauffageScenario, unsigned char *p_uc_Chauffage, unsigned char *uc_DernierEtatChauffageAvantScenario, unsigned char uc_ZoneSdb);
extern void vd_Scenario_Init(unsigned short us_AdresseScenario, unsigned char uc_ValeurInit);
extern void vd_Scenario_Init_Valeurs_JE_SORS(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_JE_PARS_EN_VACANCES(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_JE_RENTRE(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_JE_ME_COUCHE(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_JE_ME_LEVE(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_Perso1(unsigned short us_AdresseScenario);
extern void vd_Scenario_Init_Valeurs_Perso2(unsigned short us_AdresseScenario);
extern void vd_Scenario_Cde(unsigned char uc_Numero);
extern void vd_Scenario_Exec(unsigned short us_AdresseScenario, unsigned char uc_Numero);
extern char *puc_NomScenario(unsigned char uc_Numero);
extern void vd_GestionScenario(void);
extern void vd_Scenario_TraitementDemandeInit(unsigned short us_AdresseScenario);
