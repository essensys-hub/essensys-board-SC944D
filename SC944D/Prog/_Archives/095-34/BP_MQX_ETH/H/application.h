
// Liaison I2C BP <-> BA
#define I2C_DEVICE_POLLED    "i2c0:"
// ne sert pas #define I2C_DEVICE_INTERRUPT "ii2c0:"
// ne sert pas #define ENABLE_I2C_INTERRUPT  BSPCFG_ENABLE_II2C0

// adresses sur bus I2C
#define	I2C_BUS_ADDRESS_BP	0x10
#define	I2C_BUS_ADDRESS_BA	0x11	// boitier pièces de vie
						//	0x12	// boitier chambres
						//	0x13	// boitier pièces d'eau

// Liaison série BP <-> ECRAN
//#define ECRAN_DEVICE_POLLED     "ttya:"
#define ECRAN_DEVICE_INTERRUPT	"ittya:"
#define	ECRAN_BAUD_RATE			9600

// Liaison compteur LINKY
//#define TELEINFO_DEVICE_POLLED     "ttyb:"
#define TELEINFO_DEVICE_INTERRUPT	"ittyb:"
#define	TELEINFO_BAUD_RATE			1200

// 0 CARTE FREESCALE M52259EVB
// 1 BP PROTO 1
#define	SC944_0A 	1	 
						

// GPIO board specifications
#if SC944_0A

//----------------------------------------------------------------------------------------
// carte BP
//----------------------------------------------------------------------------------------
#define BP_I_OUVERTURE_SIRENE_INTERIEURE	   			(GPIO_PORT_AS | GPIO_PIN2)
#define BP_I_OUVERTURE_SIRENE_INTERIEURE_MUX_GPIO 		(LWGPIO_MUX_AS2_GPIO)

#define BP_I_OUVERTURE_SIRENE_EXTERIEURE	   			(GPIO_PORT_TE | GPIO_PIN5)
#define BP_I_OUVERTURE_SIRENE_EXTERIEURE_MUX_GPIO 		(LWGPIO_MUX_TE5_GPIO)
#define BP_I_OUVERTURE_PANNEAU_DOMOTIQUE	   			(GPIO_PORT_AN | GPIO_PIN7)
#define BP_I_OUVERTURE_PANNEAU_DOMOTIQUE_MUX_GPIO 		(LWGPIO_MUX_AN7_GPIO)
#define BP_O_SIRENE_EXTERIEURE	   						(GPIO_PORT_TC | GPIO_PIN0)	// Ancien Hard (GPIO_PORT_TE | GPIO_PIN4)
#define BP_O_SIRENE_EXTERIEURE_MUX_GPIO 				(LWGPIO_MUX_TC0_GPIO)		// Ancien Hard (LWGPIO_MUX_TE4_GPIO)
#define BP_O_15VSP_ALIM_BA 								(GPIO_PORT_TE | GPIO_PIN2)	
#define BP_O_15VSP_ALIM_BA_MUX_GPIO						(LWGPIO_MUX_TE2_GPIO)
#define BP_O_UC_LED_ETAT_BP	   							(GPIO_PORT_TE | GPIO_PIN0)
#define BP_O_UC_LED_ETAT_BP_MUX_GPIO 					(LWGPIO_MUX_TE0_GPIO)
#define BP_O_UC_BATT_CTRL	   							(GPIO_PORT_TE | GPIO_PIN1)
#define BP_O_UC_BATT_CTRL_MUX_GPIO	 					(LWGPIO_MUX_TE1_GPIO)
#define BP_O_VANNE_ARROSAGE					   			(GPIO_PORT_DD | GPIO_PIN7)	
#define BP_O_VANNE_ARROSAGE_MUX_GPIO				   	(LWGPIO_MUX_DD3_GPIO)
#define BP_O_PRISE_SECURITE	   							(GPIO_PORT_DD | GPIO_PIN4)	
#define BP_O_PRISE_SECURITE_MUX_GPIO					(LWGPIO_MUX_DD4_GPIO)
#define BP_O_MACHINE_A_LAVER	   						(GPIO_PORT_DD | GPIO_PIN6)
#define BP_O_MACHINE_A_LAVER_MUX_GPIO	 			  	(LWGPIO_MUX_DD6_GPIO)
#define BP_O_CUMULUS	   								(GPIO_PORT_DD | GPIO_PIN5)
#define BP_O_CUMULUS_MUX_GPIO	   						(LWGPIO_MUX_DD5_GPIO)
#define BP_O_FP_ZJ		   								(GPIO_PORT_TA | GPIO_PIN0)	
#define BP_O_FP_ZJ_MUX_GPIO								(LWGPIO_MUX_TA0_GPIO)
#define BP_O_FP_ZN		   								(GPIO_PORT_DD | GPIO_PIN0)
#define BP_O_FP_ZN_MUX_GPIO								(LWGPIO_MUX_DD0_GPIO)
#define BP_O_FP_SDB1	   								(GPIO_PORT_DD | GPIO_PIN1)
#define BP_O_FP_SDB1_MUX_GPIO							(LWGPIO_MUX_DD1_GPIO)
#define BP_O_FP_SDB2	   								(GPIO_PORT_DD | GPIO_PIN2)
#define BP_O_FP_SDB2_MUX_GPIO							(LWGPIO_MUX_DD2_GPIO)
#define BP_I_DETECT_OUV    								(GPIO_PORT_TF | GPIO_PIN0)	
#define BP_I_DETECT_OUV_MUX_GPIO 						(LWGPIO_MUX_TF0_GPIO)
#define BP_I_DETECT_PRES1  								(GPIO_PORT_TE | GPIO_PIN7)	
#define BP_I_DETECT_PRES1_MUX_GPIO 						(LWGPIO_MUX_TE7_GPIO)
#define BP_I_DETECT_PRES2  								(GPIO_PORT_TE | GPIO_PIN6)
#define BP_I_DETECT_PRES2_MUX_GPIO 						(LWGPIO_MUX_TE6_GPIO)
#define BP_I_PLUIE		   								(GPIO_PORT_DD | GPIO_PIN3)
#define BP_I_PLUIE_MUX_GPIO 							(LWGPIO_MUX_DD3_GPIO)
#define BP_O_TELEINF_LED	   							(GPIO_PORT_TC | GPIO_PIN3)
#define BP_O_TELEINF_LED_MUX_GPIO 						(LWGPIO_MUX_TC3_GPIO)
#define BP_I_SECTEUR_SYNCHRO  							(GPIO_PORT_NQ | GPIO_PIN7)
#define BP_I_SECTEUR_SYNCHRO_MUX_IRQ  					(1)	// primary function
//#define BP_I_SECTEUR_SYNCHRO_MUX_GPIO					(LWGPIO_MUX_NQ7_GPIO)
#define BP_I_SECTEUR_ETAT_ALIM_PRINCIPALE				(GPIO_PORT_AN | GPIO_PIN0)	
#define BP_I_SECTEUR_ETAT_ALIM_PRINCIPALE_MUX_GPIO		(LWGPIO_MUX_AN0_GPIO)

#define BP_I_OUVERTURE_DETECTEUR_1						(GPIO_PORT_TG | GPIO_PIN7)	
#define BP_I_OUVERTURE_DETECTEUR_1_MUX_GPIO				(LWGPIO_MUX_TG7_GPIO)
#define BP_I_OUVERTURE_DETECTEUR_2						(GPIO_PORT_TH | GPIO_PIN1)	
#define BP_I_OUVERTURE_DETECTEUR_2_MUX_GPIO				(LWGPIO_MUX_TH1_GPIO)
#define BP_I_DIN_VITESSE_VENT							(GPIO_PORT_TA | GPIO_PIN3)	
#define BP_I_DIN_VITESSE_VENT_MUX_GPIO					1	//(LWGPIO_MUX_TA3_GPIO)
#define BP_I_ETOR_RESERVE2								(GPIO_PORT_NQ | GPIO_PIN1)	
#define BP_I_ETOR_RESERVE2_MUX_GPIO						(LWGPIO_MUX_NQ1_GPIO)
#define BP_I_ERREUR_ECRAN_HARD							(GPIO_PORT_TF | GPIO_PIN1)	
#define BP_I_ERREUR_ECRAN_HARD_MUX_GPIO					(LWGPIO_MUX_TF1_GPIO)
#define BP_O_ECRAN_DIRECTION							(GPIO_PORT_UA | GPIO_PIN3)	
#define BP_O_ECRAN_DIRECTION_MUX_GPIO					(LWGPIO_MUX_UA3_GPIO)
#define BP_O_DEBUG_J1									(GPIO_PORT_TF | GPIO_PIN6)	
#define BP_O_DEBUG_J1_MUX_GPIO							(LWGPIO_MUX_TF6_GPIO)
#define BP_O_DEBUG_J2									(GPIO_PORT_TF | GPIO_PIN5)	
#define BP_O_DEBUG_J2_MUX_GPIO							(LWGPIO_MUX_TF5_GPIO)
#define BP_O_DEBUG_J3									(GPIO_PORT_TF | GPIO_PIN4)	
#define BP_O_DEBUG_J3_MUX_GPIO							(LWGPIO_MUX_TF4_GPIO)
#define BP_O_DEBUG_J4									(GPIO_PORT_TF | GPIO_PIN3)	
#define BP_O_DEBUG_J4_MUX_GPIO							(LWGPIO_MUX_TF3_GPIO)
#define BP_O_DEBUG_J5									(GPIO_PORT_TF | GPIO_PIN2)	
#define BP_O_DEBUG_J5_MUX_GPIO							(LWGPIO_MUX_TF2_GPIO)

// CS EEPROM : géré directement par SPI mais init à faire manuellement
#define BP_O_SPI_CS_EEPROM_ADRESSE_MAC					(GPIO_PORT_QS | GPIO_PIN3)	
#define BP_O_SPI_CS_EEPROM_ADRESSE_MAC_MUX_GPIO			(LWGPIO_MUX_QS3_GPIO)
#define BP_O_SPI_CS_EEPROM_SOFT							(GPIO_PORT_QS | GPIO_PIN5)	
#define BP_O_SPI_CS_EEPROM_SOFT_MUX_GPIO				(LWGPIO_MUX_QS5_GPIO)

#define BP_OPWM_SIRENE	   			4	// pwm 4	

// ADC channels
//#define ADC_VBAT 			(ADC_SOURCE_AN4)
//#define ADC_CH_FUITE1		(ADC_SOURCE_AN6)	
//#define ADC_CH_FUITE2		(ADC_SOURCE_AN5)

//----------------------------------------------------------------------------------------
// fin carte BP
//----------------------------------------------------------------------------------------

#else
	
//----------------------------------------------------------------------------------------
// demo board 
//----------------------------------------------------------------------------------------
#define BP_I_DETECT_OUV    			(GPIO_PORT_DD | GPIO_PIN5)	// user switch 1
#define BP_I_DETECT_OUV_MUX_GPIO 	(LWGPIO_MUX_DD5_GPIO)
#define BP_I_DETECT_PRES1  			(GPIO_PORT_DD | GPIO_PIN6)	// user switch 2
#define BP_I_DETECT_PRES1_MUX_GPIO 	(LWGPIO_MUX_DD6_GPIO)
#define BP_I_DETECT_PRES2  			(GPIO_PORT_DD | GPIO_PIN7)	// user switch 3 !!!
#define BP_I_DETECT_PRES2_MUX_GPIO 	(LWGPIO_MUX_DD7_GPIO)
#define BP_I_SIRENE_EXT	   			(GPIO_PORT_DD | GPIO_PIN7)	// user switch 3 !!!
#define BP_I_SIRENE_EXT_MUX_GPIO 	(LWGPIO_MUX_DD7_GPIO)
#define BP_I_PLUIE		   			(GPIO_PORT_DD | GPIO_PIN7)	// user switch 3 !!!
#define BP_I_PLUIE_MUX_GPIO 		(LWGPIO_MUX_DD7_GPIO)
#define BP_I_SECTEUR_ETAT  			(GPIO_PORT_DD | GPIO_PIN7)	// user switch 3 !!!
#define BP_I_SECTEUR_ETAT_MUX_GPIO 	(LWGPIO_MUX_DD7_GPIO)
#define BP_I_SECTEUR_SYNC  			(GPIO_PORT_NQ | GPIO_PIN7)	// abort switch
#define BP_I_SECTEUR_SYNC_MUX_GPIO  (LWGPIO_MUX_NQ7_GPIO)
#define BP_I_SECTEUR_SYNC_MUX_IRQ  	(1)	// primary function
#define BP_O_SIRENE_EXT	   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_SIRENE_EXT_MUX_GPIO 	(LWGPIO_MUX_TC0_GPIO)
#define BP_O_12VSP		   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_12VSP_MUX_GPIO		   	(LWGPIO_MUX_TC0_GPIO)
#define BP_O_UC_LED	   				(GPIO_PORT_TC | GPIO_PIN2)	// led 3 ------------
#define BP_O_UC_LED_MUX_GPIO 		(LWGPIO_MUX_TC2_GPIO)
#define BP_O_VANNE		   			(GPIO_PORT_TC | GPIO_PIN1)	// led 2
#define BP_O_VANNE_MUX_GPIO		   	(LWGPIO_MUX_TC1_GPIO)
#define BP_O_M_A_LAVER	   			(GPIO_PORT_TC | GPIO_PIN1)	// led 2
#define BP_O_M_A_LAVER_MUX_GPIO	   	(LWGPIO_MUX_TC1_GPIO)
#define BP_O_CUMULUS	   			(GPIO_PORT_TC | GPIO_PIN1)	// led 2
#define BP_O_CUMULUS_MUX_GPIO	   	(LWGPIO_MUX_TC1_GPIO)
#define BP_O_P_SECURITE	   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_P_SECURITE_MUX_GPIO	(LWGPIO_MUX_TC0_GPIO)
#define BP_O_FP_ZJ		   			(GPIO_PORT_TA | GPIO_PIN0)	// J25 pin 8
#define BP_O_FP_ZJ_MUX_GPIO			(LWGPIO_MUX_TA0_GPIO)
#define BP_O_FP_ZN		   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_FP_ZN_MUX_GPIO			(LWGPIO_MUX_TC0_GPIO)
#define BP_O_FP_SDB1	   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_FP_SDB1_MUX_GPIO		(LWGPIO_MUX_TC0_GPIO)
#define BP_O_FP_SDB2	   			(GPIO_PORT_TC | GPIO_PIN0)	// led 1
#define BP_O_FP_SDB2_MUX_GPIO		(LWGPIO_MUX_TC0_GPIO)
#define BP_O_TELEINF_LED	   		(GPIO_PORT_TC | GPIO_PIN3)	// led 4 -------------
#define BP_O_TELEINF_LED_MUX_GPIO 	(LWGPIO_MUX_TC3_GPIO)

#define BP_OPWM_SIRENE	   			7	// pwm 7	// J25 pin 2
#define BP_OPWM_DEBUG	   			4	// pwm 4	// led 3	// xxx pour debug

// ADC channels
#define ADC_CH_FUITE1      (ADC_SOURCE_AN0)				// potentiomètre
#define ADC_CH_FUITE2      (ADC_SOURCE_AN1)

//----------------------------------------------------------------------------------------
// fin demo board 
//----------------------------------------------------------------------------------------
#endif

