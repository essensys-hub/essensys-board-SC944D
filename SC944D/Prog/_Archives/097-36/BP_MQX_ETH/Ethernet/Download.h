
//#define FLASH_NAME "flashx:bank0"	// Toute la zone FLASH

#define ul_FLASH_TAILLE				0x80000

#define ul_FLASH_APP_SOFT_START		0x03000
#define ul_FLASH_APP_SOFT_END		0x7E000
#define ul_FLASH_TAILLE_SECTEUR		0x01000

#define ul_FLASH_SOFT_LENGTH		ul_FLASH_APP_SOFT_END-ul_FLASH_APP_SOFT_START


// Adresses relatives
#define ul_FLASH_REL_SOFT_CRC		0x00000
#define ul_FLASH_REL_SOFT_CRC_SIZE	0x02
#define ul_FLASH_REL_SOFT_VERSION	0x00002	// Size : 2
#define ul_FLASH_REL_SOFT_ENTRY		0x00004	// Size : 8


extern signed char sc_OpenFlash(void);
extern signed char sc_EffacerZoneNouveauProgramme(void);
extern signed char sc_FlashMemoriserBinaireRecu(char *pc_BinaireTelecharge);
extern signed char sc_CheckZoneNouveauProgramme(void);
extern signed char sc_CloseFlash(void);
extern unsigned char uc_ConvertirAsciiToValue(unsigned char uc_Ascii);
extern unsigned char uc_ConvertirAsciiToByte(unsigned char uc_Ascii1, unsigned char uc_Ascii2);
extern unsigned char uc_MemoriseLigneS19EnFlash(unsigned char *p_uc_S19, unsigned char uc_S19Size);

extern unsigned short us_CalculerCRCZoneApp(void);
extern unsigned short us_CalculerCRCZoneNew(void);
extern void vd_CalculInfosZoneSoft(unsigned short *p_us_CRCCalcule, unsigned short *p_us_CRCLuEnFlash, unsigned short *p_us_Version);
extern void vd_CalculInfosZoneNew(unsigned short *p_us_CRCCalcule, unsigned short *p_us_CRCLuEnFlash, unsigned short *p_us_Version);
