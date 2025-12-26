
extern __declspec(interrupt) void vd_IT_FilPilote_Detection0AlternanceSecteur(void);
extern unsigned char uc_PiloterSignalAlternanceNegative(unsigned char uc_Consigne, unsigned char uc_EcoEnCours, unsigned char uc_EcoPlusEnCours);
extern void TimerFilPilote_InitIsr(void);
extern void TimerFilPilote_Init10ms(void);
extern void TimerFilPilote_Init23ms(void);
extern unsigned char uc_PiloterSignalAlternancePositive(unsigned char uc_Consigne, unsigned char uc_EcoEnCours, unsigned char uc_EcoPlusEnCours);
extern __declspec(interrupt) void Timer10Ms_Interrupt(void);
extern void FilPiloteZJ_PutVal(unsigned char Val);
extern void FilPiloteZN_PutVal(unsigned char Val);
extern void FilPiloteSDB1_PutVal(unsigned char Val);
extern void FilPiloteSDB2_PutVal(unsigned char Val);
extern void vd_GestionFilPilote(void);
extern void vd_GestionCompteurEcoEtEcoPlus(unsigned char *puc_Compteur, unsigned char *puc_FlagRAZ, unsigned char *puc_EcoEnCours, unsigned char *puc_EcoPlusEnCours);
extern unsigned char uc_GestionChangementConsigne(unsigned char uc_ConsigneEnCours, unsigned char *uc_ConsignePrecedent, unsigned char *uc_RAZCompteurSecondesEcoEtEcoPlus, unsigned char *uc_CompteurITModeHGForce);
