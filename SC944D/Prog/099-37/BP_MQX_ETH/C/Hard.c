#include <mqx.h>
#include <bsp.h>
#include <mutex.h>
#include <timer.h>
#include <message.h>
#include "application.h"	//xxx

#include "tableechange.h"
#include "global.h"
#include "EspionRS.h"
#include "espionrs.h"
#include "arrosage.h"
#include "chauffage.h"
#include "filpilote.h"
#include "tableechangeflash.h"
#include "anemo.h"
#include  "adcdirect.h"

#define extern
#include  "Hard.h"
#undef extern


// entrées analogiques

//#define MY_ADC "adc:"
//#define MY_ADC_CHANNEL_1 "adc:1"
//#define MY_ADC_CHANNEL_2 "adc:2"
//#define MY_ADC_CHANNEL_3 "adc:3"
//
//const ADC_INIT_STRUCT adc_init = {
//    ADC_RESOLUTION_DEFAULT,     // resolution 
//};

// Logical channel #0 init struct 
//const ADC_INIT_CHANNEL_STRUCT adc_channel_param_VBAT = 
//{
//	ADC_VBAT, // physical ADC channel 
//	ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW, // runs continuously  after fopen 
//    1,             // number of samples in one run sequence 
//    150000,         // time offset from trigger point in us 
//    500000,         // period in us (= 0.6 sec) 
//    0x10000,        // scale range of result (not used now) 
//    1,             // circular buffer size (sample count) 
//    ADC_TRIGGER_1, // logical trigger ID that starts this ADC channel
//#if MQX_USE_LWEVENTS
//    NULL
//#endif
//};

// Logical channel #1 init struct 
//const ADC_INIT_CHANNEL_STRUCT adc_channel_param_FUITE_1 = 
//{
//	ADC_CH_FUITE1, // physical ADC channel 
//	ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW, // runs continuously  after fopen 
//    1,             // number of samples in one run sequence 
//    150000,         // time offset from trigger point in us 
//    500000,         // period in us (= 0.6 sec) 
//    0x10000,        // scale range of result (not used now) 
//    1,             // circular buffer size (sample count) 
//    ADC_TRIGGER_1,	//xxx2, // logical trigger ID that starts this ADC channel
//#if MQX_USE_LWEVENTS
//    NULL
//#endif
//};

// Logical channel #2 init struct 
//const ADC_INIT_CHANNEL_STRUCT adc_channel_param_FUITE_2 = 
//{
//	ADC_CH_FUITE2, 	// physical ADC channel 
//	ADC_CHANNEL_MEASURE_LOOP | ADC_CHANNEL_START_NOW, // runs continuously  after fopen 
//    8,             // number of samples in one run sequence 
//    150000,         // time offset from trigger point in us 
//    600000,         // period in us (= 0.6 sec) 
//    0x10000,        // scale range of result (not used now) 
//    8,             // circular buffer size (sample count) 
//    ADC_TRIGGER_1,	//xxx3,   // logical trigger ID that starts this ADC channel 
//#if MQX_USE_LWEVENTS
//    NULL
//#endif
//};

void vd_InitHard(void)
{
	unsigned char l_uc_EtatD5;
	unsigned char l_uc_EtatD6;
	unsigned char l_uc_EtatD7;
	
	_int_install_unexpected_isr();	// xxx dans exemple flashx - essayer sans ???
	
	vd_Init_IO();
	vd_Init_PWM();
	//vd_Init_ADC();
	vd_ADCInit();
	
	// Avant de lancer le pilotage des fils pilotes -> détection version carte !!!
	printf("Détection version Hard :\n");
	l_uc_EtatD5 = 0;	if(lwgpio_get_value(&IO_DIN_D5) == LWGPIO_VALUE_HIGH)	l_uc_EtatD5 = 1;
	l_uc_EtatD6 = 0;	if(lwgpio_get_value(&IO_DIN_D6) == LWGPIO_VALUE_HIGH)	l_uc_EtatD6 = 1;
	l_uc_EtatD7 = 0;	if(lwgpio_get_value(&IO_DIN_D7) == LWGPIO_VALUE_HIGH)	l_uc_EtatD7 = 1;
	
	printf("D5 : %d\n", l_uc_EtatD5);
	printf("D6 : %d\n", l_uc_EtatD6);
	printf("D7 : %d\n", l_uc_EtatD7);
	
	// Première génération de carte : les 3 entrées à 1
	if(l_uc_EtatD5 == 1 && l_uc_EtatD6 == 1 && l_uc_EtatD7 == 1)	uc_VersionHard = 0;
	else															uc_VersionHard = 1;
	
	printf("Version Hard : %d\n", uc_VersionHard);
	
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TimerFilPilote1_InitIsr\n");
	TimerFilPilote1_InitIsr();
	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TimerFilPilote1_Init23ms\n");
	TimerFilPilote1_Init23ms();

	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"TimerFilPilote2_InitIsr\n");
	TimerFilPilote2_InitIsr();

	vd_AnemoInit();
}

// initialisation des entrées analogiques
//void vd_Init_ADC(void)
//{
//
//    // opening ADC device
//    f_adc = fopen(MY_ADC, (const char*)&adc_init);
//    if(f_adc == NULL)
//    {    
//    	DETECTION_ERREUR_HARD_OPEN_ADC;
//		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"adc\n");
//    }
//
//    // opening ADC channel
//    f_adc_Fuite1 = fopen(MY_ADC "FUITE 1" /*MY_ADC_CHANNEL_1*/, (const char*)&adc_channel_param_FUITE_1);
//    if(f_adc_Fuite1 == NULL)
//    {    
//    	DETECTION_ERREUR_HARD_OPEN_ADC_FUITE_1;
//		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"adc_Fuite1\n");
//    }
//
//    f_adc_Fuite2 = fopen(MY_ADC "FUITE 2" /*MY_ADC_CHANNEL_2*/, (const char*)&adc_channel_param_FUITE_2);
//    if(f_adc_Fuite2 == NULL)
//    {    
//    	DETECTION_ERREUR_HARD_OPEN_ADC_FUITE_2;
//		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"adc_Fuite2\n");
//    }
//
//    f_adc_vbat = fopen(MY_ADC "VBAT" /*MY_ADC_CHANNEL_3*/, (const char*)&adc_channel_param_VBAT);
//    if(f_adc_vbat == NULL)
//    {    
//    	DETECTION_ERREUR_HARD_OPEN_ADC_VBAT;
//		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"adc_vbat\n");
//    }
//}

//void vd_Close_ADC(void)
//{
//	fclose(f_adc_vbat);
//	fclose(f_adc_Fuite1);
//    fclose(f_adc_Fuite2);
//    fclose(f_adc);
//}

// initialisation des PWM
void vd_Init_PWM(void)
{
	PWM_DEV_CONFIG_DATA l_pwm_conf_data;
	

	if((fdpwm = fopen("pwm:", NULL )) == NULL)
	{
		DETECTION_ERREUR_HARD_OPEN_PWM;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Driver PWM\n");
	}
	
	// Configure PWM module :
	l_pwm_conf_data.clock_Sys_to_A_divisor = 0;  // 2**n, 0<=n<=7 
	l_pwm_conf_data.clock_Sys_to_B_divisor = 0;  // 2**n, 0<=n<=7
	l_pwm_conf_data.clock_A_to_AS_divisor = 60;   // 2*n, or 512 if n=0 
	l_pwm_conf_data.clock_B_to_BS_divisor = 60;   // 2*n, or 512 if n=0 
	if(ioctl(fdpwm, PWM_IOCTL_CONFIG, &l_pwm_conf_data)) {
		DETECTION_ERREUR_HARD_CONFIG_PWM;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Config PWM\n");
	}

	//Configure a pin and set initial period and duty cycle :
	pwm_pin_data.pwm_pin = BP_OPWM_SIRENE;         // 0-7 
	pwm_pin_data.polarity = 1;        // 1 for active high 
	pwm_pin_data.xS_clock_select = 1; // 1 for AS/BS clock instead of A/B; A for 0,1,4,5 / B for 2,3,6,7 
	pwm_pin_data.center_aligned = 0;  // 1 for center aligned 
	pwm_pin_data.concatenate = 0;     // 1 to concatenate odd channel n with channel n-1 
	pwm_pin_data.stop_doze = 0;       // stop PWM in doze mode 
	pwm_pin_data.stop_debug = 0;      // stop PWM in debug mode 
	pwm_pin_data.pcm_enable = 0;      // 1 to use PCM instead of PWM 
	pwm_pin_data.period = 200;        // 0-255, or 0-65535 if concatenate=1 
	pwm_pin_data.duty = 100;            // 0-period; 0:0%; period:100% 	par defaut a 100 % pour eviter de
	if(ioctl(fdpwm, PWM_IOCTL_ENABLE_PIN, &pwm_pin_data)) {
		DETECTION_ERREUR_HARD_CONFIG_DOUT_PWM;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Enable PWM\n");
	}
}

void vd_Close_PWM(void)
{
    fclose(fdpwm);
}

// Changement de rapport d'un PWM
// n° de PWM / rapport de sortie
void vd_Duty_PWM(uint_8 pwm_pin, unsigned char uc_duty) 
{
	PWM_DEV_CHANGE_DUTY_DATA l_pwm_change_data;

	// vérification du rapport : 0 à 100%
	if(uc_duty > 100)
	{
		uc_duty = 100;
	}
	
	//Change period and duty cycle :
	l_pwm_change_data.pwm_pin = pwm_pin;
	l_pwm_change_data.period = pwm_pin_data.period;
	l_pwm_change_data.duty = uc_duty * pwm_pin_data.period / 100; 
	if(ioctl(fdpwm, PWM_IOCTL_CHANGE_DUTY, &l_pwm_change_data)) {
		DETECTION_ERREUR_HARD_MODIF_DUTY_PWM;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Erreur change duty PWM %d = %d %%\n", pwm_pin, uc_duty);
	}
}

// initialisation des ports IO
void vd_Init_IO(void)
{
    if(!lwgpio_init(&IO_DIN_OuvertureSireneInterieure, BP_I_OUVERTURE_SIRENE_INTERIEURE, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_OUVERTURE_SIRENE_INTERIEURE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN OuvSireneInt\n");
    }
    lwgpio_set_functionality(&IO_DIN_OuvertureSireneInterieure, BP_I_OUVERTURE_SIRENE_INTERIEURE_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_OuvertureSireneInterieure, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

    if(!lwgpio_init(&IO_DIN_OuvertureSireneExterieure, BP_I_OUVERTURE_SIRENE_EXTERIEURE, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_OUVERTURE_SIRENE_EXTERIEURE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN OuvSireneExt\n");
    }
    lwgpio_set_functionality(&IO_DIN_OuvertureSireneExterieure, BP_I_OUVERTURE_SIRENE_EXTERIEURE_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_OuvertureSireneExterieure, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

    if(!lwgpio_init(&IO_DIN_OuvertureTableauDominique, BP_I_OUVERTURE_PANNEAU_DOMOTIQUE, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_OUVERTURE_TABLEAU_DOMOTIQUE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN OuvTabDominique\n");
    }
    lwgpio_set_functionality(&IO_DIN_OuvertureTableauDominique, BP_I_OUVERTURE_PANNEAU_DOMOTIQUE_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_OuvertureTableauDominique, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);	// xxx existe sur ce micro ???

    if (!lwgpio_init(&IO_DOUT_Sirene_Exterieure, BP_O_SIRENE_EXTERIEURE, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_SIRENE_EXTERIEURE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeSireneExt\n");
    }
    lwgpio_set_functionality(&IO_DOUT_Sirene_Exterieure, BP_O_SIRENE_EXTERIEURE_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_15VSP_AlimBA, BP_O_15VSP_ALIM_BA, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_15VSP_ALIM_BA;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Cde15VSP\n");
    }
    lwgpio_set_functionality(&IO_DOUT_15VSP_AlimBA, BP_O_15VSP_ALIM_BA_MUX_GPIO);
    lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_LOW);

    if(!lwgpio_init(&IO_DOUT_LEDEtatBP, BP_O_UC_LED_ETAT_BP, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_LED_ETAT_BP;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeLEDEtatBP\n");
    }
    lwgpio_set_functionality(&IO_DOUT_LEDEtatBP, BP_O_UC_LED_ETAT_BP_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_ControleBatterie, BP_O_UC_BATT_CTRL, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_CTRL_BATTERIE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeControleBatterie\n");
    }
    lwgpio_set_functionality(&IO_DOUT_ControleBatterie, BP_O_UC_BATT_CTRL_MUX_GPIO);
    
    if(!lwgpio_init(&IO_DOUT_VanneArrosage, BP_O_VANNE_ARROSAGE, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_VANNE_ARROSAGE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeEVArrosage\n");
    }
    lwgpio_set_functionality(&IO_DOUT_VanneArrosage, BP_O_VANNE_ARROSAGE_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_PriseSecurite, BP_O_PRISE_SECURITE, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_PRISE_SECURITE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdePriseSecurite\n");
    }
    lwgpio_set_functionality(&IO_DOUT_PriseSecurite, BP_O_PRISE_SECURITE_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_MachineALaver, BP_O_MACHINE_A_LAVER, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_MACHINE_A_LAVER;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeMachineALaver\n");
    }
    lwgpio_set_functionality(&IO_DOUT_MachineALaver, BP_O_MACHINE_A_LAVER_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_Cumulus, BP_O_CUMULUS, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_CUMULUS;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeCumulus\n");
    }
    lwgpio_set_functionality(&IO_DOUT_Cumulus, BP_O_CUMULUS_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_FilPilote_ZJ, BP_O_FP_ZJ, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_FP_ZJ;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeFP_ZJ\n");
    }
    lwgpio_set_functionality(&IO_DOUT_FilPilote_ZJ, BP_O_FP_ZJ_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_FilPilote_ZN, BP_O_FP_ZN, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_FP_ZN;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeFP_ZN\n");
    }
    lwgpio_set_functionality(&IO_DOUT_FilPilote_ZN, BP_O_FP_ZN_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_FilPilote_SDB1, BP_O_FP_SDB1, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_FP_SDB1;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeFP_SDB1\n");
    }
    lwgpio_set_functionality(&IO_DOUT_FilPilote_SDB1, BP_O_FP_SDB1_MUX_GPIO);

    if(!lwgpio_init(&IO_DOUT_FilPilote_SDB2, BP_O_FP_SDB2, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_LOW))
    {
    	DETECTION_ERREUR_OPEN_IO_FP_SDB2;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeFP_SDB2\n");
    }
    lwgpio_set_functionality(&IO_DOUT_FilPilote_SDB2, BP_O_FP_SDB2_MUX_GPIO);

    if(!lwgpio_init(&IO_DIN_Detection_Ouverture, BP_I_DETECT_OUV, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DETECT_OUV;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN DetectOuverture\n");
    }
    lwgpio_set_functionality(&IO_DIN_Detection_Ouverture, BP_I_DETECT_OUV_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_Detection_Ouverture, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	if(!lwgpio_init(&IO_DIN_Detection_presence1, BP_I_DETECT_PRES1, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
		DETECTION_ERREUR_OPEN_IO_DETECT_PRES1;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN_DetPres1\n");
    }
    lwgpio_set_functionality(&IO_DIN_Detection_presence1, BP_I_DETECT_PRES1_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_Detection_presence1, LWGPIO_ATTR_PULL_DOWN, LWGPIO_AVAL_ENABLE);
    
    if(!lwgpio_init(&IO_DIN_Detection_presence2, BP_I_DETECT_PRES2, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DETECT_PRES2;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN_DetPres2\n");
    }
    lwgpio_set_functionality(&IO_DIN_Detection_presence2, BP_I_DETECT_PRES2_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_Detection_presence2, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
    
    if(!lwgpio_init(&IO_DIN_Detection_Pluie, BP_I_PLUIE, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DETECT_PLUIE;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN DetPluie\n");
    }
    lwgpio_set_functionality(&IO_DIN_Detection_Pluie, BP_I_PLUIE_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_Detection_Pluie, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

    if(!lwgpio_init(&IO_DOUT_TeleInfo_Led, BP_O_TELEINF_LED, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_LED_TELEINFO;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"CdeTeleInfoLed\n");
    }
    lwgpio_set_functionality(&IO_DOUT_TeleInfo_Led, BP_O_TELEINF_LED_MUX_GPIO);

    
    // Initialisation entree détection 0 secteur pour génération signaux fil pilote
	if(!lwgpio_init(&IO_DIN_Secteur_Synchro, BP_I_SECTEUR_SYNCHRO, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_SECTEUR_SYNCHRO;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"ERREUR HARD : IO_Secteur_Synchro\n");
	}
	lwgpio_set_functionality(&IO_DIN_Secteur_Synchro, 1);	//BP_I_SECTEUR_SYNCHRO_MUX_IRQ);		// IRQ mode
	lwgpio_set_attribute(&IO_DIN_Secteur_Synchro, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
	
	if(!lwgpio_int_init(&IO_DIN_Secteur_Synchro, LWGPIO_INT_MODE_FALLING))
	{
		DETECTION_ERREUR_OPEN_IO_SECTEUR_SYNCHRO_INIT_IT;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"ERREUR HARD : IO_Secteur_Synchro init IT\n");
	}
	
	_int_install_kernel_isr(lwgpio_int_get_vector(&IO_DIN_Secteur_Synchro), vd_IT_FilPilote_Detection0AlternanceSecteur);
	_bsp_int_init(lwgpio_int_get_vector(&IO_DIN_Secteur_Synchro), 7, 3, TRUE);
	
	lwgpio_int_clear_flag(&IO_DIN_Secteur_Synchro);
	lwgpio_int_enable(&IO_DIN_Secteur_Synchro, TRUE);
	// ------------------------------
	
	if(!lwgpio_init(&IO_DIN_Etat_AlimPrincipale, BP_I_SECTEUR_ETAT_ALIM_PRINCIPALE, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_ETAT_ALIM_PRINCIPALE;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN AlimPrinc\n");
	}
	lwgpio_set_functionality(&IO_DIN_Etat_AlimPrincipale, BP_I_SECTEUR_ETAT_ALIM_PRINCIPALE_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_Etat_AlimPrincipale, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	if(!lwgpio_init(&IO_DIN_OuvertureDecteurPresence1, BP_I_OUVERTURE_DETECTEUR_1, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_1;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN OuvDetPres1\n");
	}
	lwgpio_set_functionality(&IO_DIN_OuvertureDecteurPresence1, BP_I_OUVERTURE_DETECTEUR_1_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_OuvertureDecteurPresence1, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	if(!lwgpio_init(&IO_DIN_OuvertureDecteurPresence2, BP_I_OUVERTURE_DETECTEUR_2, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_OUVERTURE_DETECTEUR_PRESENCE_2;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN OuvDetPres2\n");
	}
	lwgpio_set_functionality(&IO_DIN_OuvertureDecteurPresence2, BP_I_OUVERTURE_DETECTEUR_2_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_OuvertureDecteurPresence2, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
		
	if(!lwgpio_init(&IO_DIN_VITESSE_VENT, BP_I_DIN_VITESSE_VENT, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DIN_ETOR_reserve1;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN Reserve1\n");
	}
	lwgpio_set_functionality(&IO_DIN_VITESSE_VENT, BP_I_DIN_VITESSE_VENT_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_VITESSE_VENT, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	if(!lwgpio_init(&IO_DIN_BOUTON_MAGIQUE, BP_I_ETOR_RESERVE2, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DIN_ETOR_reserve2;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN Reserve2\n");
	}
	lwgpio_set_functionality(&IO_DIN_BOUTON_MAGIQUE, BP_I_ETOR_RESERVE2_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_BOUTON_MAGIQUE, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	if(!lwgpio_init(&IO_DIN_ErreurEcranHard, BP_I_ERREUR_ECRAN_HARD, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_ERREUR_ECRAN_HARD;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN ErreurEcranHard\n");
	}
	lwgpio_set_functionality(&IO_DIN_ErreurEcranHard, BP_I_ERREUR_ECRAN_HARD_MUX_GPIO);
	lwgpio_set_attribute(&IO_DIN_ErreurEcranHard, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

	
	if(!lwgpio_init(&IO_DOUT_EcranDirection, BP_O_ECRAN_DIRECTION, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_ECRAN_DIRECTION;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DOUT EcranDir\n");
	}
	lwgpio_set_functionality(&IO_DOUT_EcranDirection, BP_O_ECRAN_DIRECTION_MUX_GPIO);
	lwgpio_set_value(&IO_DOUT_EcranDirection, LWGPIO_VALUE_HIGH);

	if(!lwgpio_init(&IO_DOUT_DebugJ1, BP_O_DEBUG_J1, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ1;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Debug1\n");
	}
	lwgpio_set_functionality(&IO_DOUT_DebugJ1, BP_O_DEBUG_J1_MUX_GPIO);

	if(!lwgpio_init(&IO_DOUT_DebugJ2, BP_O_DEBUG_J2, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ2;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Debug2\n");
	}
	lwgpio_set_functionality(&IO_DOUT_DebugJ2, BP_O_DEBUG_J2_MUX_GPIO);

	if(!lwgpio_init(&IO_DOUT_DebugJ3, BP_O_DEBUG_J3, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ3;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Debug3\n");
	}
	lwgpio_set_functionality(&IO_DOUT_DebugJ3, BP_O_TELEINF_LED_MUX_GPIO);

	if(!lwgpio_init(&IO_DOUT_DebugJ4, BP_O_DEBUG_J4, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ4;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Debug4\n");
	}
	lwgpio_set_functionality(&IO_DOUT_DebugJ4, BP_O_DEBUG_J4_MUX_GPIO);

	if(!lwgpio_init(&IO_DOUT_DebugJ5, BP_O_DEBUG_J5, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_DOUT_DebugJ5;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"Debug5\n");
	}
	lwgpio_set_functionality(&IO_DOUT_DebugJ5, BP_O_DEBUG_J5_MUX_GPIO);
	lwgpio_set_value(&IO_DOUT_DebugJ5, LWGPIO_VALUE_LOW);


	// CS EEPROM : géré directement par SPI mais init à faire manuellement
	if(!lwgpio_init(&IO_DOUT_SPI_CS_EepromAdresseMac, BP_O_SPI_CS_EEPROM_ADRESSE_MAC, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_EEPROM_ADRESSE_MAC;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"SPI_CS_EepromAdresseMac\n");
	}
	lwgpio_set_functionality(&IO_DOUT_SPI_CS_EepromAdresseMac, 1);
	lwgpio_set_value(&IO_DOUT_SPI_CS_EepromAdresseMac, LWGPIO_VALUE_HIGH);

	if(!lwgpio_init(&IO_DOUT_SPI_CS_EepromSoft, BP_O_SPI_CS_EEPROM_SOFT, LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE))
	{
		DETECTION_ERREUR_OPEN_IO_EEPROM_SOFT;
		vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"SPI_CS_EepromSoft\n");
	}
	lwgpio_set_functionality(&IO_DOUT_SPI_CS_EepromSoft, 1);
	lwgpio_set_value(&IO_DOUT_SPI_CS_EepromSoft, LWGPIO_VALUE_HIGH);
	
	// Nouvelles entrées Carte V2 2016
    if(!lwgpio_init(&IO_DIN_D5, BP_I_NEW_IMPUT_D5, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DIN_D5;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN IO_DIN_D5\n");
    }
    lwgpio_set_functionality(&IO_DIN_D5, BP_I_NEW_INPUT_D5_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_D5, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

    if(!lwgpio_init(&IO_DIN_D6, BP_I_NEW_IMPUT_D6, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DIN_D6;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN IO_DIN_D6\n");
    }
    lwgpio_set_functionality(&IO_DIN_D6, BP_I_NEW_INPUT_D6_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_D6, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

    if(!lwgpio_init(&IO_DIN_D7, BP_I_NEW_IMPUT_D7, LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE))
    {
    	DETECTION_ERREUR_OPEN_IO_DIN_D7;
    	vd_EspionRS_Printf(uc_ESPION_HARD_ERREURS,"DIN IO_DIN_D7\n");
    }
    lwgpio_set_functionality(&IO_DIN_D7, BP_I_NEW_INPUT_D7_MUX_GPIO);
    lwgpio_set_attribute(&IO_DIN_D7, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);
}

// Analyse AIN acquise (passée en paramètre)
// Retour 0 si pas de détection eau ou 1 si détection eau
unsigned char uc_DetectionFuiteEau(struct st_AIN st_AINATraiter, unsigned char uc_EtatPrecedent)
{
	unsigned char l_uc_FuiteEauDetectee;
	//_mqx_int l_Retour;
	//ADC_RESULT_STRUCT l_st_DataADC;

	
	l_uc_FuiteEauDetectee = 0;
	//l_Retour = read(f_ch, &l_st_DataADC, sizeof(l_st_DataADC));
//	if(l_Retour == 0)
//	{
//		ul_EspionTachePrincipaleAcquisitionAINEnCours++;	// xxx verifier que acquisition effectuee de temps en temps... wdg soft
//		l_uc_FuiteEauDetectee = uc_EtatPrecedent;	// acquisition en cours -> retourne état précédent
//	}
//	else
//	{	// entrée disponible
		//ul_EspionTachePrincipaleAcquisitionAINOK++;
		
		//st_AinATraiter->us_AINBrut = us_AIN;	//l_st_DataADC.result;
		
		// 0 	-> 0.55 V 	: ouvert		=> 585
		// 0.56	-> 1.65 V 	: OK			=> 2040
		// 1.65	-> 2.75 V	: NOK = fuite	=> 3390
		// 2.76	-> 3.3 V	: court circuit	=> 4092
		if(st_AINATraiter.us_AINBrut >= 3390)
		{
			// xxx info à remonter : écran / Ethernet
		}
		else
		{
			if(st_AINATraiter.us_AINBrut >= 2040)
			{
				l_uc_FuiteEauDetectee = 1;// fuite d'eau
			}
		}
	//}
	
	if(l_uc_FuiteEauDetectee != 0 && uc_EtatPrecedent == 0)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"DETECTION FUITE EAU D %d : %4d\n\n",st_AINATraiter.uc_NumeroAIN,st_AINATraiter.us_AINBrut);
	}
	return(l_uc_FuiteEauDetectee);
}

void vd_PiloterSorties(void)
{
	unsigned char l_uc_PilotageCumulus;
	
	
	vd_PiloterSirenes();	// xxx a faire dans tous les cas ? meme si pas d'alim
		
	// traitement du 12V secouru piloté
	// xxx voir comment le piloter : pour l'instant : toujours
	// xxx coupure après quelques secondes pour laisser le temps aux borniers de faire leur sauvagarde ?
	//lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_HIGH);		// 12V secouru piloté

	/*xxx ng ne couper que le 15 voltsif(st_EchangeStatus.uc_Secouru != 0)
	{	// Mode secouru (sur batterie) -> couper toutes les sorties
		
		// Fils pilotes
		lwgpio_set_value(&IO_DOUT_FilPilote_ZJ, LWGPIO_VALUE_LOW);
		lwgpio_set_value(&IO_DOUT_FilPilote_ZN, LWGPIO_VALUE_LOW);
		lwgpio_set_value(&IO_DOUT_FilPilote_SDB1, LWGPIO_VALUE_LOW);
		lwgpio_set_value(&IO_DOUT_FilPilote_SDB2, LWGPIO_VALUE_LOW);
		
		// Arrosage
		lwgpio_set_value(&IO_DOUT_VanneArrosage, LWGPIO_VALUE_LOW);
		
		// Prises de sécurité
		lwgpio_set_value(&IO_DOUT_PriseSecurite, LWGPIO_VALUE_LOW);
		
		// Machine à laver
		lwgpio_set_value(&IO_DOUT_MachineALaver, LWGPIO_VALUE_LOW);
		
		// Cumulus
		lwgpio_set_value(&IO_DOUT_Cumulus, LWGPIO_VALUE_LOW);
		
		// Led état téléinfo
		lwgpio_set_value(&IO_DOUT_TeleInfo_Led, LWGPIO_VALUE_LOW);
	}
	else*/
	{	// présence tension secteur OK -> controle commande normal

		// Fils pilotes -> gérés sous IT...

		// Arrosage
		if(uc_Arrosage() != 0)
		{
			lwgpio_set_value(&IO_DOUT_VanneArrosage, LWGPIO_VALUE_HIGH);
			//xxx*(gpio_ptr[5] + IO_DOUT_VanneArrosage.port_idx) = ~IO_DOUT_VanneArrosage.pinmask;
			//xxx*(gpio_ptr[5] + IO_DOUT_VanneArrosage.port_idx) = IO_DOUT_VanneArrosage.pinmask;
			uc_CouperEVArrosage = 0;
		}
		else
		{
			//lwgpio_set_value(&IO_DOUT_VanneArrosage, LWGPIO_VALUE_LOW);
			uc_CouperEVArrosage = 1;	// Coupure sous IT FIL PILOTE
		}

		// Prises de sécurité -> pilotées d'abord en fonction de leur paramètre, puis s'il n'y a pas d'ordre de coupure (scénario)
		if(Tb_Echange[Securite_PriseCoupe] == 0 && st_EchangeStatus.uc_SécuOFF == 0)	lwgpio_set_value(&IO_DOUT_PriseSecurite, LWGPIO_VALUE_HIGH);
		else																			lwgpio_set_value(&IO_DOUT_PriseSecurite, LWGPIO_VALUE_LOW);

		// Machine à laver - toujours pilotée sauf si fuite d'eau ou ordre de coupure (scénario)
		//xxx revoir fonctionnement uc_SécuOFF / Securite_PriseCoupe
		if(st_EchangeStatus.uc_MachinesOFF == 0 && st_EchangeStatus.uc_FuiteLL == FALSE && st_EchangeStatus.uc_FuiteLV == FALSE)
		   lwgpio_set_value(&IO_DOUT_MachineALaver, LWGPIO_VALUE_HIGH); 
		else
		   lwgpio_set_value(&IO_DOUT_MachineALaver, LWGPIO_VALUE_LOW);

		// Cumulus -> piloté si mode forcé ou si mode auto HC/HP -> si HC ou erreur comm avec compteur edf
		// Couper si demandé par délestage (uc_DelestageCouperCumulus)
		if(uc_DelestageCouperCumulus != 0)
		{
			lwgpio_set_value(&IO_DOUT_Cumulus, LWGPIO_VALUE_LOW);
		}
		else
		{
			l_uc_PilotageCumulus = Tb_Echange[Cumulus_Mode];	// Pour eviter un eventuel changement de valeur pendant test
			if(l_uc_PilotageCumulus == uc_CUMULUS_ON || (l_uc_PilotageCumulus == uc_CUMULUS_HC && (st_EchangeStatus.uc_HeuresCreuses != 0 || st_EchangeStatus.uc_DefTeleinfo != 0)))
			   lwgpio_set_value(&IO_DOUT_Cumulus, LWGPIO_VALUE_HIGH); 
			else
			   lwgpio_set_value(&IO_DOUT_Cumulus, LWGPIO_VALUE_LOW);
		}
		
		// Led état téléinfo -> pilotée dans tache teleinfo
	}
}

// Intègre le controle commande de pilotage des sirenes :
// Piloter si alarme ou si alerte détection fuite
//xxx piloter seconde sirene
void vd_PiloterSirenes(void)
{
	static unsigned short s_us_CompteurDureeAlerte = 0;
	static unsigned char s_uc_old_seconde;
	unsigned char l_uc_IntensiteSireneInterieure;
	unsigned char l_uc_IntensiteSireneExterieure;
	static unsigned char s_uc_IntensiteSireneInterieurePrecedent;
	static unsigned char s_uc_IntensiteSireneExterieurePrecedent;
	static unsigned char s_uc_FuiteDetectee = 0;

		
	//xxx
	// intrusion (tous les cas) : sirenes pednant 3 minutes - toute détection pendant les 3 minutes ne reinitialisent pas le temps
	//				ensuite pause 3 minutes, et redeclenchement si nouvelle détection détectée (front montant)
	// Fraudes a faire
	
	// Alerte détection eau
	if(Tb_Echange[Securite_FuiteAlerte] != 0 && (st_EchangeStatus.uc_FuiteLL != 0 || st_EchangeStatus.uc_FuiteLV != 0) && s_uc_FuiteDetectee == 0)
	{ 		// alerte sonore activée en cas de fuite d'eau
		
		s_uc_FuiteDetectee = 1;	// Email NG du 05/23/2014 : Pour pouvoir redéclencher une alerte, il faut obligatoirement que le capteur repasse par un état sec (= pas de détection)
		
		// Apparition de l'alarme -> init durée	
		Tb_Echange[Alerte_Acquit] = 0;
		if(Tb_Echange[Alerte_Duree] != 0)
		{	// durée de l'alerte : 1 à 255 => temps en mn
			s_us_CompteurDureeAlerte = (unsigned short)(Tb_Echange[Alerte_Duree]);
		}
		else
		{	// durée de l'alerte : 0 => jusqu'à disparition
			s_us_CompteurDureeAlerte = 1;
		}
	}
	
	if(st_EchangeStatus.uc_FuiteLL == 0 && st_EchangeStatus.uc_FuiteLV == 0)
	{
		s_uc_FuiteDetectee = 0;
	}
	
	// Durée alerte : à faire même si disparition "fuite"...
	if(s_us_CompteurDureeAlerte > 0)
	{
		// durée de l'alerte : 1 à 255= temps en mn
		// le décomptage se fait sans utiliser les timers : on n'a besoin ni d'être rapide, ni d'être précis
		if(s_uc_old_seconde != st_DateHeure.SECOND)
		{
			s_uc_old_seconde = (unsigned char)st_DateHeure.SECOND;
			if(Tb_Echange[Alerte_Duree] != 0)	s_us_CompteurDureeAlerte--;	// Si Alerte_Duree == 0 -> alerte permanente
		}
	}
	
	// Pilotage sirenes
	// Alerte provoque une sonnerie seulement si l'alarme n'est pas déclenchée
	l_uc_IntensiteSireneInterieure = uc_BUZ_TFORT;
	l_uc_IntensiteSireneExterieure = uc_BUZ_TFORT;
	if(st_EchangeStatus.uc_AlarmeDeclenchee != 0)
	{
		if(us_CompteurPilotageSireneInterieure_sec < us_DUREE_PILOTAGE_SIRENE_INTERIEURE_sec)
		{
			l_uc_IntensiteSireneInterieure = uc_BUZ_TFORT;
			uc_ReinitCompteursSirenes = 0;	// Ignorer toute réinit pendant le pilotage de la sirène
		}
		else
		{
			l_uc_IntensiteSireneInterieure = uc_BUZ_STOP;
		}
		if(us_CompteurPilotageSireneExterieure_sec < us_DUREE_PILOTAGE_SIRENE_EXTERIEURE_sec)
		{
			l_uc_IntensiteSireneExterieure = uc_BUZ_TFORT;
			uc_ReinitCompteursSirenes = 0;	// Ignorer toute réinit pendant le pilotage de la sirène
		}
		else
		{
			l_uc_IntensiteSireneExterieure = uc_BUZ_STOP;
		}
		
		// Reinit compteurs sirenes si demande et si les deux à l'arret !
		if(us_CompteurPilotageSireneInterieure_sec >= us_DUREE_PILOTAGE_SIRENE_INTERIEURE_sec &&
		   us_CompteurPilotageSireneExterieure_sec >= us_DUREE_PILOTAGE_SIRENE_EXTERIEURE_sec)
		{
			if(uc_ReinitCompteursSirenes != 0)
			{
				uc_ReinitCompteursSirenes = 0;
				us_CompteurPilotageSireneInterieure_sec = 0;
				us_CompteurPilotageSireneExterieure_sec = 0;
			}
		}
		
		// Gestion compteur pilotage sirène extérieure
		if(l_uc_IntensiteSireneExterieure != uc_BUZ_STOP)
		{
			if(uc_SireneExterieurePilotee == 0)
			{
				if(uc_CompteurNbPilotageSireneExterieure <= uc_NB_MAX_PILOTAGE_SIRENE_EXTERIEURE)	uc_CompteurNbPilotageSireneExterieure++;
			}
			uc_SireneExterieurePilotee = 1;
		}
		else
		{
			uc_SireneExterieurePilotee = 0;
		}
		if(uc_CompteurNbPilotageSireneExterieure > uc_NB_MAX_PILOTAGE_SIRENE_EXTERIEURE)
		{
			l_uc_IntensiteSireneExterieure = uc_BUZ_STOP;
		}
	}
	else
	{
		uc_ReinitCompteursSirenes = 0; // RAZ si alarme non déclenchée !
		us_CompteurPilotageSireneInterieure_sec = 0;
		us_CompteurPilotageSireneExterieure_sec = 0;
		uc_CompteurNbPilotageSireneExterieure = 0;
		uc_SireneExterieurePilotee = 0;
		
		l_uc_IntensiteSireneExterieure = 0;

		if(s_us_CompteurDureeAlerte > 0)	// Alerte non acquittée
		{
			l_uc_IntensiteSireneInterieure = Tb_Echange[Alerte_Intensite];
			if(l_uc_IntensiteSireneInterieure >= uc_BUZ_STOP)	l_uc_IntensiteSireneInterieure = uc_BUZ_MOYEN;	// sécurité
			if(Tb_Echange[Alerte_Acquit] != 0)
			{
				Tb_Echange[Alerte_Acquit] = 0;
				s_us_CompteurDureeAlerte = 0;
			}
			l_uc_IntensiteSireneExterieure = uc_BUZ_STOP;	// Ne pas piloter sirène extérieure en cas d'alerte
		}
		else	// Ne pas piloter la sirène
		{
			l_uc_IntensiteSireneInterieure = uc_BUZ_STOP;
			l_uc_IntensiteSireneExterieure = uc_BUZ_STOP;
			
			// Gestion mode test
			if(Tb_Echange[Alarme_Mode] == uc_ALARME_MODE_REGLAGE)
			{
				//	Alarme_TestSirenes,			// Mode réglage
												//		0x00 : Aucune sirène activée
												//		0x01 : Sirène d'intérieur activée en alarme
												//		0x02 : Sirène d'extérieur activée en alarme
				if((Tb_Echange[Alarme_TestSirenes] & 0x01) != 0)		l_uc_IntensiteSireneInterieure = uc_BUZ_TFORT;
				if((Tb_Echange[Alarme_TestSirenes] & 0x02) != 0)		l_uc_IntensiteSireneExterieure = uc_BUZ_TFORT;
			}
			else
			{
				// Mode réglage volume alerte
				if(Tb_Echange[Alerte_TestSirene] != 0)
				{
					l_uc_IntensiteSireneInterieure = Tb_Echange[Alerte_Intensite];
				}
			}
		}
	}
	
	// Ligne suivante : empêche la sirène de sonner si pas utilisée ET que pas mode test (sirène ou alerte) et que pas de Alerte
	// Dans ce sens, une perturbation (action volontaire de l'écran) de la table d'échange ne peut bloquer la sirène du moment qu'elle a été déclarée utilisée au lancement de l'alarme
	if(uc_AlarmeConfigEnCours[AlarmeConfig_SireneInt] == 0 &&
	   Tb_Echange[Alarme_Mode] != uc_ALARME_MODE_REGLAGE &&
	   Tb_Echange[Alerte_TestSirene] == 0 &&
	   s_us_CompteurDureeAlerte == 0)		l_uc_IntensiteSireneInterieure = uc_BUZ_STOP;
	if(l_uc_IntensiteSireneInterieure != s_uc_IntensiteSireneInterieurePrecedent)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SIRENE INTERIEURE %d -> %d\n", s_uc_IntensiteSireneInterieurePrecedent, l_uc_IntensiteSireneInterieure);
		s_uc_IntensiteSireneInterieurePrecedent = l_uc_IntensiteSireneInterieure;
	}
	vd_Duty_PWM(BP_OPWM_SIRENE, l_uc_IntensiteSireneInterieure);
	
	
	if(uc_AlarmeConfigEnCours[AlarmeConfig_SireneExt] == 0 && Tb_Echange[Alarme_Mode] != uc_ALARME_MODE_REGLAGE)		l_uc_IntensiteSireneExterieure = uc_BUZ_STOP;
	if(l_uc_IntensiteSireneExterieure != s_uc_IntensiteSireneExterieurePrecedent)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SIRENE EXTERIEURE %d -> %d\n", s_uc_IntensiteSireneExterieurePrecedent, l_uc_IntensiteSireneExterieure);
		s_uc_IntensiteSireneExterieurePrecedent = l_uc_IntensiteSireneExterieure;
	}
	
	if(l_uc_IntensiteSireneExterieure == uc_BUZ_TFORT)	lwgpio_set_value(&IO_DOUT_Sirene_Exterieure, LWGPIO_VALUE_LOW);
	else												lwgpio_set_value(&IO_DOUT_Sirene_Exterieure, LWGPIO_VALUE_HIGH);
}
// xxx a gerer Alerte_TestSirene
// xxx alerte que sur sirene interieure et si pas acquitté

void vd_DetectionFuitesEau(void)
{
	// Machines à laver - détection des fuites d'eau
	if(Tb_Echange[Securite_FuiteLinge] != 0)
	{	// détection de fuite sur le lave-linge activée
		st_EchangeStatus.uc_FuiteLL = uc_DetectionFuiteEau(st_AIN6_FuiteLL, st_EchangeStatus.uc_FuiteLL);
	}
	else
	{
		st_EchangeStatus.uc_FuiteLL = 0;
	}
	if(Tb_Echange[Securite_FuiteVaisselle] == 1)
	{		// détection de fuite sur le lave-vaisselle activée
		st_EchangeStatus.uc_FuiteLV= uc_DetectionFuiteEau(st_AIN5_FuiteLV, st_EchangeStatus.uc_FuiteLV);
	}
	else
	{
		st_EchangeStatus.uc_FuiteLV = 0;
	}
}

void vd_SurveillanceAlimentations(void)
{
	static unsigned char s_uc_TempoAvantModeSecouru = 0;
	static unsigned char s_uc_TempoAvantSuppModeSecouru = 0;

	// 0 = Mode de fonctionnement normal.
	// 1 = Mode de fonctionnement de test.
	// Mode test : pas de tempo au passage secouru / idem au retour en mode normal

	// surveillance présence alimentation principale (secteur convertie en 24 Volts)
	if(lwgpio_get_value(&IO_DIN_Etat_AlimPrincipale) == LWGPIO_VALUE_HIGH)
	{
		if(st_EchangeStatus.uc_Secouru == TRUE)
		{
			if(s_uc_TempoAvantSuppModeSecouru < uc_TEMPO_AVANT_SUPP_MODE_SECOURU_sec && Tb_Echange[Mode_Test] == 0)	s_uc_TempoAvantSuppModeSecouru++;
			else
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Rétablissement courant -> activation +15VSP\n");
				st_EchangeStatus.uc_Secouru = FALSE;
			}
			lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_HIGH);	//xxx les ba vont etre ralumes et recevoir a nouveau un ordre de sauvegarde !!!! ??? peut etre pas car envoi sur changement etat dans BA !!! a verifier !!!!
		}
		s_uc_TempoAvantModeSecouru = 0;
	}
	else
	{
		if(Tb_Echange[Mode_Test] == 0)
		{
			if(s_uc_TempoAvantModeSecouru <= (uc_TEMPO_AVANT_MODE_SECOURU_sec+2))
			{
				s_uc_TempoAvantModeSecouru++;
			}
			if(s_uc_TempoAvantModeSecouru == uc_TEMPO_AVANT_MODE_SECOURU_sec)
			{
				st_EchangeStatus.uc_Secouru = TRUE;
				s_uc_TempoAvantSuppModeSecouru = 0;
				ul_EspionTachePrincipaleNbCoupuresCourant++;
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Coupure courant -> déclenchement mode SECOURU -> envoi ordre sauvegarde BA !\n");
			}
			if(s_uc_TempoAvantModeSecouru == (uc_TEMPO_AVANT_MODE_SECOURU_sec+2))
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Coupure courant -> coupure +15VSP\n");
				lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_LOW);
				
				// Memorisation table échange en FLASH
				vd_TableEchangeSaveEnFLash();	//xxx mutex risque appel en meme temps que apres download
			}
			if(st_EchangeStatus.uc_Secouru == FALSE)
			{
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Coupure courant -> coupure +15VSP dans %d secondes\n", uc_TEMPO_AVANT_MODE_SECOURU_sec-s_uc_TempoAvantModeSecouru);
			}
		}
		else
		{
			if(st_EchangeStatus.uc_Secouru == FALSE)
			{
				st_EchangeStatus.uc_Secouru = TRUE;
				ul_EspionTachePrincipaleNbCoupuresCourant++;
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Coupure courant -> déclenchement mode SECOURU EN MODE TEST !\n");
	
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"Coupure courant -> coupure +15VSP\n");
				lwgpio_set_value(&IO_DOUT_15VSP_AlimBA, LWGPIO_VALUE_LOW);
			}
			s_uc_TempoAvantSuppModeSecouru = 0;
		}
	}	
}

// Calcul moyenne glissante sur 10 secondes de VBat
// Si mode secouru -> coupure BP si VBat < 1500 points (< 10.5 Volts) - Batterie déchargée !!!
void vd_GestionCoupureAlimentation(void)
{
	static unsigned char s_uc_PremierPassage = 1;
	static unsigned char s_uc_TempoAvantCoupure = 0;
	unsigned char l_uc_Compteur;
	unsigned long l_ul_Moyenne;
	
	
	// 1er passage -> init tableau avec VBat
	if(s_uc_PremierPassage != 0)
	{
		s_uc_PremierPassage = 0;
		for(l_uc_Compteur = 0; l_uc_Compteur < uc_VBAT_MOYENNE_GLISSANTE; l_uc_Compteur++)
		{
			us_VBat[l_uc_Compteur] = st_AIN4_VBat.us_AINBrut;
		}
	}
	
	// Prise en compte nouvelle tension VBat
	for(l_uc_Compteur = 1; l_uc_Compteur < uc_VBAT_MOYENNE_GLISSANTE; l_uc_Compteur++)
	{
		us_VBat[l_uc_Compteur-1] = us_VBat[l_uc_Compteur];
	}
	us_VBat[uc_VBAT_MOYENNE_GLISSANTE-1] = st_AIN4_VBat.us_AINBrut;
		
	// Calcul moyenne VBat
	l_ul_Moyenne = 0;
	for(l_uc_Compteur = 0; l_uc_Compteur < uc_VBAT_MOYENNE_GLISSANTE; l_uc_Compteur++)
	{
		l_ul_Moyenne = l_ul_Moyenne + (unsigned long)us_VBat[l_uc_Compteur];
	}
	l_ul_Moyenne = l_ul_Moyenne / uc_VBAT_MOYENNE_GLISSANTE;
	
	// Coupure BP si VBat < seuil min ET mode secouru en cours...
	if(st_EchangeStatus.uc_Secouru == TRUE && l_ul_Moyenne < us_VBAT_MIN_pts)
	{
		if(s_uc_TempoAvantCoupure < uc_TEMPO_AVANT_COUPURE_sec)
		{
			s_uc_TempoAvantCoupure++;
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"!!! VBat FAIBLE (%d / %d) - Coupure dans %d secondes...\n", st_AIN4_VBat.us_AINBrut, l_ul_Moyenne, uc_TEMPO_AVANT_COUPURE_sec-s_uc_TempoAvantCoupure);
		}
		else
		{
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"!!! VBat FAIBLE - Coupure BP ! Bye bye...\n");
			_time_delay(2000);
			lwgpio_set_value(&IO_DOUT_ControleBatterie, LWGPIO_VALUE_LOW);
		}
	}
	else
	{
		s_uc_TempoAvantCoupure = 0;
	}
}

// Ancienne version : détection oscillation VBAT
// A appeler chaque seconde
/*void vd_VerifPrecenceBatterie(void)
{
	if(us_AinVBatMin < us_OSCILLATION_SEUIL_BAS)	uc_OscillationCompteurSeuilBas_sec = 0;
	if(us_AinVBatMax > us_OSCILLATION_SEUIL_HAUT)	uc_OscillationCompteurSeuilHaut_sec = 0;
	
	//printf("MIN %4d MAX %4d -> %d - %d\n", us_AinVBatMin, us_AinVBatMax, uc_OscillationCompteurSeuilBas_sec, uc_OscillationCompteurSeuilHaut_sec);

	us_AinVBatMin = 65535;
	us_AinVBatMax = 0;

	if(uc_OscillationCompteurSeuilBas_sec >= uc_DUREE_DETECTION_OSCILLATION_sec ||
	   uc_OscillationCompteurSeuilHaut_sec >= uc_DUREE_DETECTION_OSCILLATION_sec)
	{	// Au moins 1 des seuils n'a pas été vu depuis plus de 5 secondes -> pas d'oscillation -> batterie présente
		uc_BatteriePresente = 1;
	}
	else
	{	// Les deux seuils ont été vus dans les 5 dernières secondes -> oscillation -> batterie non présente
		uc_BatteriePresente = 0;
	}

	if(uc_OscillationCompteurSeuilBas_sec <= uc_DUREE_DETECTION_OSCILLATION_sec)	uc_OscillationCompteurSeuilBas_sec++;
	if(uc_OscillationCompteurSeuilHaut_sec <= uc_DUREE_DETECTION_OSCILLATION_sec)	uc_OscillationCompteurSeuilHaut_sec++;

	if(uc_BatteriePresentePrecedent != uc_BatteriePresente)
	{
		uc_BatteriePresentePrecedent = uc_BatteriePresente;
		if(uc_BatteriePresente == 0)	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"BATTERIE NON PRESENTE\n");
		else							vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"BATTERIE PRESENTE\n");
	}
}*/

// Nouvelle version (voir email NG 28/09/2014 13:41) - METHODE 2
// Acquisition toutes les 100 ms
// Comparaison des 3 derniers echantillons : si écart entre le plus petit et le plus grand <= 20 pts -> batterie presente sinon batterie absente ou HS
// A appeler toutes les 100 ms !!!
void vd_VerifPrecenceBatterie(void)
{
	unsigned char l_uc_Compteur;
	unsigned short l_us_Min;
	unsigned short l_us_Max;
	
	
	// Recherche du MIN et du MAX
	l_us_Max = us_VBatDernieresValeurs[0];
	l_us_Min = us_VBatDernieresValeurs[0];
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_VBAT_NB_ECHANTILLONS;l_uc_Compteur++)
	{
		if(us_VBatDernieresValeurs[l_uc_Compteur] > l_us_Max)		l_us_Max = us_VBatDernieresValeurs[l_uc_Compteur];
		if(us_VBatDernieresValeurs[l_uc_Compteur] < l_us_Min)		l_us_Min = us_VBatDernieresValeurs[l_uc_Compteur];
	}
	
	// Test écart entre MIN et MAX
	if(l_us_Max > l_us_Min)	// Normalement toujours le cas !
	{
		if((l_us_Max - l_us_Min) <= 15)			uc_BatteriePresente = 1;
		else									uc_BatteriePresente = 0;
	}

	if(uc_BatteriePresentePrecedent != uc_BatteriePresente)
	{
		uc_BatteriePresentePrecedent = uc_BatteriePresente;
		if(uc_BatteriePresente == 0)	vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"BATTERIE NON PRESENTE\n");
		else							vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"BATTERIE PRESENTE\n");
	}
}

unsigned short us_Abs(unsigned short us_Valeur1, unsigned short us_Valeur2)
{
	unsigned short l_us_ValeurRetour;
	
	l_us_ValeurRetour = 0;
	if(us_Valeur1 > us_Valeur2)
	{
		l_us_ValeurRetour = us_Valeur1 - us_Valeur2;
	}
	else
	{
		l_us_ValeurRetour = us_Valeur2 - us_Valeur1;
	}
	return l_us_ValeurRetour;
}

void vd_MCF52259_REBOOT(void)
{
	VMCF5225_STRUCT_PTR reg_ptr = (VMCF5225_STRUCT_PTR)BSP_IPSBAR;
	
	reg_ptr->CCM.RCR |= 0x80;	//MCF5225_CCM_RCR_SOFTRST;
}

