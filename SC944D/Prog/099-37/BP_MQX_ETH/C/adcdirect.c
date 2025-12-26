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

#define extern
#include  "adcdirect.h"
#undef extern



#define CTRL1						(*(volatile unsigned short *) (0x40190000UL))
#define CTRL2						(*(volatile unsigned short *) (0x40190002UL))
#define ADZCC						(*(volatile unsigned short *) (0x40190004UL))
#define ADLST1						(*(volatile unsigned short *) (0x40190006UL))
#define ADLST2						(*(volatile unsigned short *) (0x40190008UL))
#define ADSDIS						(*(volatile unsigned short *) (0x4019000AUL))
#define ADSTAT						(*(volatile unsigned short *) (0x4019000CUL))
#define ADLSTAT						(*(volatile unsigned short *) (0x4019000EUL))
#define ADZCSTAT					(*(volatile unsigned short *) (0x40190010UL))
#define ADRSLT0						(*(volatile unsigned short *) (0x40190012UL))
#define ADRSLT1						(*(volatile unsigned short *) (0x40190014UL))
#define ADRSLT2						(*(volatile unsigned short *) (0x40190016UL))
#define ADLLMT0						(*(volatile unsigned short *) (0x40190022UL))
#define ADLLMT1						(*(volatile unsigned short *) (0x40190024UL))
#define ADLLMT2						(*(volatile unsigned short *) (0x40190026UL))
#define ADHLMT0						(*(volatile unsigned short *) (0x40190032UL))
#define ADHLMT1						(*(volatile unsigned short *) (0x40190034UL))
#define ADHLMT2						(*(volatile unsigned short *) (0x40190036UL))
#define ADOFS0						(*(volatile unsigned short *) (0x40190042UL))
#define ADOFS1						(*(volatile unsigned short *) (0x40190044UL))
#define ADOFS2						(*(volatile unsigned short *) (0x40190046UL))
#define POWER						(*(volatile unsigned short *) (0x40190052UL))
#define CAL							(*(volatile unsigned short *) (0x40190054UL))


#define PANPAR						(*(volatile unsigned char *) (0x4010006AUL))

#define POWER_PSTS1_BITMASK           (0x800U)
#define POWER_PSTS2_BITMASK           (0x1000U)


void vd_ADCInit(void)
{
	volatile unsigned short i;
	
	CTRL1 = 0x4000;	// Stop ADC
	
	PANPAR |= 0x70;
	
	CAL = 0;
	POWER = 0xD1;	//xxx pourquoi le 0x01 ???
	ADOFS0 = 0;
	ADOFS1 = 0;
	ADOFS2 = 0;
	ADHLMT0 = 0x7FF8;
	ADHLMT1 = 0x7FF8;
	ADHLMT2 = 0x7FF8;
	ADLLMT0 = 0x00;
	ADLLMT1 = 0x00;
	ADLLMT2 = 0x00;
	ADZCSTAT = 0xFFFF;
	ADLSTAT = 0xFFFF;
	ADSTAT = 0x0800;
	ADSDIS = 0xF8;	// 3 premiers sample
	ADLST1 = 0x0654;	// AIN6 AIN5 AIN4
	ADZCC = 0;
	CTRL2 = 22;	//0x36;	// Clock diviser -> 80 000 000 / (2 x 22+1)
	for (i=0;i<0x64;i++) {}              /* Recovery time of voltage reference */
	while (POWER&((POWER_PSTS1_BITMASK|POWER_PSTS2_BITMASK))) {} /* Wait for device powered up */
	
	
	// CONFIGURATION CHANNEL -> BIT 7 6 5 4 -> TOUT A 0 POUR CONFIG NON DIFFERENTIEL
	// MODE -> BIT 2 1 0 -> 0 MODE SEQUENTIEL
	
	
	//CTRL2	CLOCK DIVISER - BITS 4 3 2 1 0
	//	-> SYSTEM CLOCK / (2 x DIV+1)
	//	ADC CLock < 5 MHz !!!
}

// Effectue l'acquisitions des 3 AIN
void vd_ADCAcquisition(void)
{
	unsigned char l_uc_Compteur;
	
	
	//	ADSTAT BIT 0 : A 1 SI SAMPLE 0 DISPO
	//	ADSTAT BIT 1 : A 1 SI SAMPLE 1 DISPO
	//	ADSTAT BIT 2 : A 1 SI SAMPLE 2 DISPO
	//	ADSTAT BIT 15 : A 1 SI SCAN EN COURS

	ADZCSTAT = 0xFFFF;
	ADLSTAT = 0xFFFF;

	CTRL1 &= ~0x4000;
	CTRL1 |= 0x2000;	// Start cycle de conversion

	while((ADSTAT & 0x8000) != 0);
	
	CTRL1 |= 0x4000;	// Stop

	// RESULTAT DANS ADRSLTx -> BITS 14-3
	vd_UpdateAin(ADRSLT0, &st_AIN4_VBat);
	vd_UpdateAin(ADRSLT1, &st_AIN5_FuiteLV);
	vd_UpdateAin(ADRSLT2, &st_AIN6_FuiteLL);

	// Ancienne version : détection oscillation VBAT
	// Memorisation MIN / MAX AIN VBAT pour détection présence batterie
	//if(st_AIN4_VBat.us_AINBrut > us_AinVBatMax)		us_AinVBatMax = st_AIN4_VBat.us_AINBrut;
	//if(st_AIN4_VBat.us_AINBrut < us_AinVBatMin)		us_AinVBatMin = st_AIN4_VBat.us_AINBrut;
	
	for(l_uc_Compteur = 0;l_uc_Compteur<(uc_VBAT_NB_ECHANTILLONS-1);l_uc_Compteur++)
	{
		us_VBatDernieresValeurs[l_uc_Compteur] = us_VBatDernieresValeurs[l_uc_Compteur+1]; 
	}
	us_VBatDernieresValeurs[uc_VBAT_NB_ECHANTILLONS-1] = st_AIN4_VBat.us_AINBrut;
}

void vd_UpdateAin(unsigned short us_Ain, struct st_AIN *st_Ain)
{
	st_Ain->us_AINBrut = us_Ain >> 3;

	
	if(uc_TempoStartAINMinMax_sec >= uc_TEMPO_START_AIN_MIN_MAXsec)
	{
		if(st_Ain->us_AINBrut > st_Ain->us_AINMax)		st_Ain->us_AINMax = st_Ain->us_AINBrut;
		if(st_Ain->us_AINBrut < st_Ain->us_AINMin)		st_Ain->us_AINMin = st_Ain->us_AINBrut;
	}
}

