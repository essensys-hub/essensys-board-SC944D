
extern unsigned char uc_EEEPROMSoft_Effacer(void);
extern unsigned char uc_EEEPROMSoft_StatusEnableEcriture(void);
extern unsigned char uc_EEEPROMSoft_EnableEcritureStatus(void);
extern unsigned char uc_EEPROMSoft_Lecture(unsigned long ul_Adresse, unsigned char uc_NbOctetsALire, unsigned char uc_ValeursLues[]);
extern unsigned char uc_EEPROMSoft_Ecriture(unsigned long ul_Adresse, unsigned char uc_OctetAEcrire);
extern unsigned char uc_EEEPROMSoft_EnableEcriture(void);
extern unsigned char uc_EEEPROMSoft_LectureStatus(unsigned char *uc_Status);
extern unsigned char uc_EEPROMSoft_AttenteBusyA0(void);
