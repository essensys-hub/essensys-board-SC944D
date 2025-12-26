
extern void vd_InitHard(void);
extern void vd_Init_ADC(void);
extern void vd_Close_ADC(void);
extern void vd_Init_PWM(void);
extern void vd_Close_PWM(void);
extern void vd_Duty_PWM(uint_8 pwm_pin, unsigned char uc_duty);
extern void vd_Init_IO(void);
extern unsigned char uc_DetectionFuiteEau(struct st_AIN st_AINATraiter, unsigned char uc_EtatPrecedent);
extern void vd_PiloterSorties(void);
extern void vd_DetectionFuitesEau(void);
extern void vd_PiloterSirenes(void);
extern void vd_SurveillanceAlimentations(void);
extern void vd_AcquisitionVBat(void);
extern unsigned short us_Abs(unsigned short us_Valeur1, unsigned short us_Valeur2);
extern void vd_GestionCoupureAlimentation(void);
extern void vd_VerifPrecenceBatterie(void);
extern void vd_MCF52259_REBOOT(void);
