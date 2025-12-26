
extern void vd_AlarmeInit(void);
extern void vd_Alarme(void);
extern unsigned char uc_ControleCodeSaisie(unsigned char *uc_NouveauCodeSaisiLSB, unsigned char *uc_NouveauCodeSaisiMSB);
extern unsigned char uc_AlimentationsCorrectesPourDemarrerAlarme(void);
extern void vd_ControleDetecteursEtFraudes(void);
extern void vd_Alarme_Espion(void);
extern unsigned char uc_DetectionFrontMontant(unsigned char uc_ValeurPrecedente, unsigned char uc_ValeurNouvelle);
