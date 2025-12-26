#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx
#include "EspionRS.h"
#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "global.h"

#define extern
#include "filpilote.h"
#undef extern

//#include "c:\freescale\cw mcu v10.4\mcu\processorExpert\lib\MCF\iofiles\MCF52259.h"	// xxx a changer...


typedef unsigned char           byte;
typedef unsigned short          word;
typedef unsigned long           dword;
#define setReg8(RegName, val)                                   (RegName = (byte)(val))
#define clrReg8Bit(RegName, BitValue)                            (RegName &= (byte)(~BitValue))
#define setReg16(RegName, val)                                   (RegName = (word)(val))
#define setReg8Bit(RegName, BitValue)                            (RegName |= (byte)(BitValue))
#define clrReg8Bits(RegName, ClrMask)                           (RegName &= (byte)(~(byte)(ClrMask)))
#define setReg8Bits(RegName, SetMask)                           (RegName |= (byte)(SetMask))
#define setReg16(RegName, val)                                   (RegName = (word)(val))
#define getReg16(RegName)                                        (RegName)
#define setReg16Bit(RegName, BitValue)                           (RegName |= (word)(BitValue))
#define clrReg16Bit(RegName, BitValue)                           (RegName &= (word)(~BitValue))
#define setReg32Bit(RegName, BitName)                           (RegName |= (dword)(BitName))
#define clrReg32Bit(RegName, BitName)                           (RegName &= (dword)(~BitName))
#define setReg32(RegName, val)                                   (RegName = (dword)(val))

//typedef unsigned char           uint8;   /*  8 bits */
//typedef volatile uint8          vuint8;  /*  8 bits */
#define SecteurSync_PIN_MASK    ((byte)0x80) /* Pin mask */

#define MCF_EPORT_EPFR                    (*(volatile unsigned char *) (0x40130006UL))
#define MFC_EPIER                         (*(volatile unsigned char *) (0x40130003UL))
#define MFC_GPTFLG1                       (*(volatile unsigned char *) (0x401A000EUL))
#define MFC_GPTSCR1                       (*(volatile unsigned char *) (0x401A0006UL))
#define MFC_GPTIE                         (*(volatile unsigned char *) (0x401A000CUL))
#define MFC_GPTSCR1                       (*(volatile unsigned char *) (0x401A0006UL))
#define MFC_GPTIOS                        (*(volatile unsigned char *) (0x401A0000UL))
#define MFC_GPTCTL1                       (*(volatile unsigned char *) (0x401A0009UL))
#define MFC_GPTCTL2                       (*(volatile unsigned char *) (0x401A000BUL))
#define MFC_GPTC0                         (*(volatile unsigned short *)(0x401A0010UL))
#define MFC_SETTA                         (*(volatile unsigned char *) (0x4010003EUL))
#define MFC_CLRTA                         (*(volatile unsigned char *) (0x40100056UL))
#define MFC_SETDD                         (*(volatile unsigned char *) (0x40100044UL))
#define MFC_CLRDD                         (*(volatile unsigned char *) (0x4010005CUL))
#define MFC_GPTSCR2                       (*(volatile unsigned char *) (0x401A000DUL))
#define MFC_GPTC3                         (*(volatile unsigned short *)(0x401A0016UL))

#define MFC_PCSR1                         (*(volatile unsigned short *)(0x40160000UL))
#define MFC_PMR1                          (*(volatile unsigned short *)(0x40160002UL))

#define MFC_DTMR0                         (*(volatile unsigned short *)(0x40000400UL))
#define MFC_DTXMR0                        (*(volatile unsigned char *) (0x40000402UL))
#define MFC_DTER0                         (*(volatile unsigned char *) (0x40000403UL))
#define MFC_DTRR0                         (*(volatile unsigned long *) (0x40000404UL))
#define MFC_DTCN0                         (*(volatile unsigned long *) (0x4000040CUL))

#define MFC_DTMR1                         (*(volatile unsigned short *)(0x40000440UL))
#define MFC_DTXMR1                        (*(volatile unsigned char *) (0x40000442UL))
#define MFC_DTER1                         (*(volatile unsigned char *) (0x40000443UL))
#define MFC_DTRR1                         (*(volatile unsigned long *) (0x40000444UL))
#define MFC_DTCN1                         (*(volatile unsigned long *) (0x4000044CUL))

unsigned char uc_DerniereITFilPiloteVue = 0;

// Pilotage FIL PILOTE : deux modes de pilotage - différent en fonction du type de carte (uc_VersionHard)
// uc_VersionHard == 0 -> pilotage sur IT Secteur puis seconde alternance par timer 10 ms
// uc_VersionHard == 1 -> pilotage idem MAIS
//							- Etat signal pilotage sortie inversé
//							- Modes HG et OFF -> arrêt pilotage après 8.72 ms
//							  Dans ce mode on pilote des TRIAC qui s'arrêtent automatiquement au prochain passage à 0 si la commande a disparu
//							  Donc la commande doit être coupée AVANT la prochaine alternance pour qu'ils se coupent automatiquement au prochain passage à 0


// Derniers ordres de pilotage de FP effectués
// Avant étaient en local dans vd_IT_FilPilote_Detection0AlternanceSecteur() et Timer1_Interrupt()
// Mis en global car la fonction Timer2_Interrupt() a besoin de connaitre le type d'ordre pour chacun pour couper le signal de commande des TRIAC
unsigned char uc_Consigne_ZJ = uc_CHAUFFAGE_CONFORT;
unsigned char uc_Consigne_ZN = uc_CHAUFFAGE_CONFORT;
unsigned char uc_Consigne_SDB1 = uc_CHAUFFAGE_CONFORT;
unsigned char uc_Consigne_SDB2 = uc_CHAUFFAGE_CONFORT;

// Routine interruption sur détection 0 alternance secteur -> pilotage fil pilote
// Gestion en direct sans passer par MQX
__declspec(interrupt) void vd_IT_FilPilote_Detection0AlternanceSecteur(void)
{
	// Désactive l'IT FIL PILOTE -> sera réactivé par le timer 10 ms
	clrReg8Bit(MFC_EPIER, 0x80U);
	
	setReg8(MCF_EPORT_EPFR /*EPFR*/,SecteurSync_PIN_MASK);  /* Clear flag */
	ul_EspionCompteurITAlternanceSecteur++;

	// Fonction appelée au passage à 0 de l'aternance et en direction de l'alternance négative
	// Détection avec un retard de 420 µs NON COMPENSE
	// On arme un timer :
	// 		Correspond au passage à 0 VERS l'alternance positive qui declenchera dans 10 ms - 420 µs
	TimerFilPilote1_Init10ms();
	
	uc_Consigne_ZJ = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_ZJ, &uc_ConsigneFilPilotePourITPrecedent_ZJ, &uc_RAZCompteurSecondesEcoEtEcoPlus_ZJ, &uc_CompteurITModeHGForce_ZJ);
	uc_Consigne_ZN = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_ZN, &uc_ConsigneFilPilotePourITPrecedent_ZN, &uc_RAZCompteurSecondesEcoEtEcoPlus_ZN, &uc_CompteurITModeHGForce_ZN);
	uc_Consigne_SDB1 = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_SDB1, &uc_ConsigneFilPilotePourITPrecedent_SDB1, &uc_RAZCompteurSecondesEcoEtEcoPlus_SDB1, &uc_CompteurITModeHGForce_SDB1);
	uc_Consigne_SDB2 = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_SDB2, &uc_ConsigneFilPilotePourITPrecedent_SDB2, &uc_RAZCompteurSecondesEcoEtEcoPlus_SDB2, &uc_CompteurITModeHGForce_SDB2);
	
	FilPiloteZJ_PutVal(uc_PiloterSignalAlternanceNegative(uc_Consigne_ZJ, uc_EcoEnCours_ZJ, uc_EcoPlusEnCours_ZJ));
	FilPiloteZN_PutVal(uc_PiloterSignalAlternanceNegative(uc_Consigne_ZN, uc_EcoEnCours_ZN, uc_EcoPlusEnCours_ZN));
	FilPiloteSDB1_PutVal(uc_PiloterSignalAlternanceNegative(uc_Consigne_SDB1, uc_EcoEnCours_SDB1, uc_EcoPlusEnCours_SDB1));
	FilPiloteSDB2_PutVal(uc_PiloterSignalAlternanceNegative(uc_Consigne_SDB2, uc_EcoEnCours_SDB2, uc_EcoPlusEnCours_SDB2));

	TimerFilPilote2_Init();	// Pour stopper le signal de commande des TRIAC
	
	// Coupure EVArrosage
	if(uc_CouperEVArrosage != 0)
	{
		uc_CouperEVArrosage = 0;
		setReg8(MFC_CLRDD, 0x7FU);
	}
	
	uc_DerniereITFilPiloteVue = 1;

	// Fait dans IT Timer
	//setReg8(MCF_EPORT_EPFR ,SecteurSync_PIN_MASK);  // Clear flag
	//setReg8Bit(MFC_EPIER, 0x80U);	// Autorise IT FIL PILOTE
}

// Détecte un changement de consigne -> renvoie la consigne à appliquer
// x -> ECO/ECO+		-> REINIT COMPTEUR 3S / 7S POUR ECO/ECO+
// ECO++ -> ECO/ECO+	-> PASSER PAR HORS GEL PENDANT QUELQUES IT (EN PLUS DU REINIT COMPTEUR 3S / 7S)
// ECO/ECO+ -> CONFORT	-> PASSER PAR HORS GEL PENDANT QUELQUES IT
unsigned char uc_GestionChangementConsigne(unsigned char uc_ConsigneEnCours, 
										   unsigned char *uc_ConsignePrecedent,
										   unsigned char *uc_RAZCompteurSecondesEcoEtEcoPlus,
										   unsigned char *uc_CompteurITModeHGForce)
{
	if(uc_ConsigneEnCours != *uc_ConsignePrecedent)
	{
		if(uc_ConsigneEnCours == uc_CHAUFFAGE_ECO || uc_ConsigneEnCours == uc_CHAUFFAGE_ECO_PLUS)
		{
			*uc_RAZCompteurSecondesEcoEtEcoPlus = 1;
			if(*uc_ConsignePrecedent == uc_CHAUFFAGE_ECO ||
			   *uc_ConsignePrecedent == uc_CHAUFFAGE_ECO_PLUS ||
			   *uc_ConsignePrecedent == uc_CHAUFFAGE_ECO_PLUS_PLUS)		*uc_CompteurITModeHGForce = uc_NB_IT_MODE_HG_FORCE;
		}
		if(uc_ConsigneEnCours == uc_CHAUFFAGE_CONFORT && (*uc_ConsignePrecedent == uc_CHAUFFAGE_ECO || *uc_ConsignePrecedent == uc_CHAUFFAGE_ECO_PLUS))
		{
			*uc_CompteurITModeHGForce = uc_NB_IT_MODE_HG_FORCE;
		}
		
		*uc_ConsignePrecedent = uc_ConsigneEnCours;
	}
	
	if(*uc_CompteurITModeHGForce > 0)
	{
		(*uc_CompteurITModeHGForce)--;
		*uc_RAZCompteurSecondesEcoEtEcoPlus = 1;	// On force pendant le forçage du HG la réinit de la tempo ECO/ECO+ qui ne devra démarrer qu'après fin du forçage HG !
		uc_ConsigneEnCours = uc_CHAUFFAGE_HORS_GEL;
	}
	
	return uc_ConsigneEnCours;
}

unsigned char uc_PiloterSignalAlternanceNegative(unsigned char uc_Consigne, unsigned char uc_EcoEnCours, unsigned char uc_EcoPlusEnCours)
{
	unsigned char l_uc_Signal;
	
	l_uc_Signal = 0;
	switch(uc_Consigne)
	{
		case uc_CHAUFFAGE_OFF:				// Alternance positive
			l_uc_Signal = 0;
		break;
		case uc_CHAUFFAGE_CONFORT:			// Pas de signal
			l_uc_Signal = 0;
		break;
		case uc_CHAUFFAGE_ECO:				// Signal complet de temps en temps...
			if(uc_EcoEnCours != 0)			l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_ECO_PLUS:			// Signal complet de temps en temps...
			if(uc_EcoPlusEnCours != 0)		l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_ECO_PLUS_PLUS:	// Signal complet
			l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_HORS_GEL:			// Alternance négative
			l_uc_Signal = 1;
		break;
	}

	if(uc_VersionHard != 1)	// Hard V0 -> inversion état pilotage - Hard V1 -> pas d'inversion
	{
		if(l_uc_Signal == 0)	l_uc_Signal = 1;
		else					l_uc_Signal = 0;
	}
	
	return l_uc_Signal;
}

void TimerFilPilote1_InitIsr(void)
{
	// Ajout routine timer (en remplacement routine MQX)
	_int_install_kernel_isr(64+19, Timer1_Interrupt);	// DTIM0
	_bsp_int_init(64+19, 7, 1, TRUE);
}

void TimerFilPilote2_InitIsr(void)
{
	// Ajout routine timer (en remplacement routine MQX)
	_int_install_kernel_isr(64+20, Timer2_Interrupt);	// DTIM0
	_bsp_int_init(64+20, 7, 2, TRUE);
}

void TimerFilPilote1_Init10ms(void)
{
	clrReg16Bit(MFC_DTMR0, 0x06);	//setReg16BitGroupVal(DTMR0,CLK,0);  /* Stop counter */

	/* DTMR0: PS=0,CE=0,OM=0,ORRI=1,FRR=0,CLK=0,RST=1 */
	setReg16(MFC_DTMR0, 0x11U);              /* Set up mode register */ 
	/* DTXMR0: DMAEN=0,HALTED=0,??=0,??=0,??=0,??=0,??=0,MODE16=0 */
	setReg16(MFC_DTXMR0, 0x00U);              
	/* DTER0: ??=0,??=0,??=0,??=0,??=0,??=0,REF=1,CAP=0 */
	setReg8(MFC_DTER0, 0x02U);                
	/* DTRR0: REF=0x000C34FF */
	setReg32(MFC_DTRR0, 0x000C34FFUL);       /* Store given value to the reference register */ 
	/* DTCN0: CNT=0 */
	setReg32(MFC_DTCN0, 0x00UL);             /* Reset counter */ 
		
	setReg8Bit(MFC_DTER0,0x02);	//REF);               /* Reset interrupt request flag */
	
	setReg16Bit(MFC_DTMR0, 0x02L);	//setReg16BitGroupVal(DTMR0,CLK,0x01); /* Run counter */
}

void TimerFilPilote1_Init23ms(void)
{
	clrReg16Bit(MFC_DTMR0, 0x06);	//setReg16BitGroupVal(DTMR0,CLK,0);  /* Stop counter */

	/* DTMR0: PS=0,CE=0,OM=0,ORRI=1,FRR=0,CLK=0,RST=1 */
	setReg16(MFC_DTMR0, 0x11U);              /* Set up mode register */ 
	/* DTXMR0: DMAEN=0,HALTED=0,??=0,??=0,??=0,??=0,??=0,MODE16=0 */
	setReg16(MFC_DTXMR0, 0x00U);              
	/* DTER0: ??=0,??=0,??=0,??=0,??=0,??=0,REF=1,CAP=0 */
	setReg8(MFC_DTER0, 0x02U);                
	/* DTRR0: REF=0x000C34FF */
	setReg32(MFC_DTRR0, 0x001C137FUL);       /* Store given value to the reference register */
	/* DTCN0: CNT=0 */
	setReg32(MFC_DTCN0, 0x00UL);             /* Reset counter */ 
		
	setReg8Bit(MFC_DTER0,0x02);	//REF);               /* Reset interrupt request flag */
	
	setReg16Bit(MFC_DTMR0, 0x02L);	//setReg16BitGroupVal(DTMR0,CLK,0x01); /* Run counter */
}

void TimerFilPilote2_Init(void)
{
	clrReg16Bit(MFC_DTMR1, 0x06);	//setReg16BitGroupVal(DTMR0,CLK,0);  /* Stop counter */

	/* DTMR0: PS=0,CE=0,OM=0,ORRI=1,FRR=0,CLK=0,RST=1 */
	setReg16(MFC_DTMR1, 0x11U);              /* Set up mode register */ 
	/* DTXMR0: DMAEN=0,HALTED=0,??=0,??=0,??=0,??=0,??=0,MODE16=0 */
	setReg16(MFC_DTXMR1, 0x00U);              
	/* DTER0: ??=0,??=0,??=0,??=0,??=0,??=0,REF=1,CAP=0 */
	setReg8(MFC_DTER1, 0x02U);                
	/* DTRR0: REF=0x000C34FF */
	
	//setReg32(MFC_DTRR1, 0x00088B7FUL); // 7 ms       /* Store given value to the reference register */
	//setReg32(MFC_DTRR1, 0x000ABDFFUL); // 8.8 ms
	setReg32(MFC_DTRR1, 0x000AA500UL); // 8.72 ms
	
	/* DTCN0: CNT=0 */
	setReg32(MFC_DTCN1, 0x00UL);             /* Reset counter */ 
		
	setReg8Bit(MFC_DTER1,0x02);	//REF);               /* Reset interrupt request flag */
	
	setReg16Bit(MFC_DTMR1, 0x02L);	//setReg16BitGroupVal(DTMR0,CLK,0x01); /* Run counter */
}

void TimerFilPilote2_Stop(void)
{
	clrReg16Bit(MFC_DTMR1, 0x06);	//setReg16BitGroupVal(DTMR0,CLK,0);  /* Stop counter */

	setReg8Bit(MFC_DTER1,0x02);	//REF);               /* Reset interrupt request flag */
}

__declspec(interrupt) void Timer1_Interrupt(void)
{
	// Le timer est redémarré pour une IT dans 23 ms - Ca permet de générer une IT dans 23 ms si signal FP enlevé (pour pouvoir couper l'EV arrosage dans tous les cas)
	TimerFilPilote1_Init23ms();

	ul_EspionCompteurITTimerFilPilote1++;

	if(uc_DerniereITFilPiloteVue != 0)	// Derniere IT vue : FP -> faire traitement FP
	{
		uc_Consigne_ZJ = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_ZJ, &uc_ConsigneFilPilotePourITPrecedent_ZJ, &uc_RAZCompteurSecondesEcoEtEcoPlus_ZJ, &uc_CompteurITModeHGForce_ZJ);
		uc_Consigne_ZN = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_ZN, &uc_ConsigneFilPilotePourITPrecedent_ZN, &uc_RAZCompteurSecondesEcoEtEcoPlus_ZN, &uc_CompteurITModeHGForce_ZN);
		uc_Consigne_SDB1 = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_SDB1, &uc_ConsigneFilPilotePourITPrecedent_SDB1, &uc_RAZCompteurSecondesEcoEtEcoPlus_SDB1, &uc_CompteurITModeHGForce_SDB1);
		uc_Consigne_SDB2 = uc_GestionChangementConsigne(uc_ConsigneFilPilotePourIT_SDB2, &uc_ConsigneFilPilotePourITPrecedent_SDB2, &uc_RAZCompteurSecondesEcoEtEcoPlus_SDB2, &uc_CompteurITModeHGForce_SDB2);
		
		FilPiloteZJ_PutVal(uc_PiloterSignalAlternancePositive(uc_Consigne_ZJ, uc_EcoEnCours_ZJ, uc_EcoPlusEnCours_ZJ));
		FilPiloteZN_PutVal(uc_PiloterSignalAlternancePositive(uc_Consigne_ZN, uc_EcoEnCours_ZN, uc_EcoPlusEnCours_ZN));
		FilPiloteSDB1_PutVal(uc_PiloterSignalAlternancePositive(uc_Consigne_SDB1, uc_EcoEnCours_SDB1, uc_EcoPlusEnCours_SDB1));
		FilPiloteSDB2_PutVal(uc_PiloterSignalAlternancePositive(uc_Consigne_SDB2, uc_EcoEnCours_SDB2, uc_EcoPlusEnCours_SDB2));

		TimerFilPilote2_Init();	// Pour stopper le signal de commande des TRIAC
		
		setReg8(MCF_EPORT_EPFR /*EPFR*/,SecteurSync_PIN_MASK);  /* Clear flag */
		setReg8Bit(MFC_EPIER, 0x80U);	// Autorise IT FIL PILOTE
	}
	else
	{
		// Pas d'IT FP vue précédemment -> mettre FP à 0 et faire coupure EVArrosage si demandée
		// Si coupure EVArrosage -> ne pas réarmer l'IT FP pour ne pas prendre en compte les perturbations dues à la coupure EVArrosage
		FilPiloteZJ_PutVal(0);
		FilPiloteZN_PutVal(0);
		FilPiloteSDB1_PutVal(0);
		FilPiloteSDB2_PutVal(0);

		// Coupure EVArrosage
		if(uc_CouperEVArrosage != 0) //xxx risque si flag remis rapidement, on ne rearmera jamais l'it fp !!! -> a limiter dans applicatif ???
		{
			uc_CouperEVArrosage = 0;
			setReg8(MFC_CLRDD, 0x7FU);
		}
		else
		{
			setReg8(MCF_EPORT_EPFR ,SecteurSync_PIN_MASK);  // Clear flag
			setReg8Bit(MFC_EPIER, 0x80U);	// Autorise IT FIL PILOTE
		}
	}
	
	uc_DerniereITFilPiloteVue = 0;
}

unsigned char uc_PiloterSignalAlternancePositive(unsigned char uc_Consigne, unsigned char uc_EcoEnCours, unsigned char uc_EcoPlusEnCours)
{
	unsigned char l_uc_Signal;
	
	l_uc_Signal = 0;
	switch(uc_Consigne)
	{
		case uc_CHAUFFAGE_OFF:				// Alternance positive
			l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_CONFORT:			// Pas de signal
			l_uc_Signal = 0;
		break;
		case uc_CHAUFFAGE_ECO:				// Signal complet de temps en temps...
			if(uc_EcoEnCours != 0)			l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_ECO_PLUS:			// Signal complet de temps en temps...
			if(uc_EcoPlusEnCours != 0)		l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_ECO_PLUS_PLUS:	// Signal complet
			l_uc_Signal = 1;
		break;
		case uc_CHAUFFAGE_HORS_GEL:			// Alternance négative
			l_uc_Signal = 0;
		break;
	}
	
	if(uc_VersionHard != 1)	// Hard V0 -> inversion état pilotage - Hard V1 -> pas d'inversion
	{
		if(l_uc_Signal == 0)	l_uc_Signal = 1;
		else					l_uc_Signal = 0;
	}

	return l_uc_Signal;
}

// Timer déclenché après 8.72 ms et après un changement d'alternance
// Ce timer est armé par l'it alternance positive -> négative ou par l'it timer 10 ms alternance négative -> positive
// Pour tous les FP, couper le signal dans les modes HG et OFF
__declspec(interrupt) void Timer2_Interrupt(void)
{
	TimerFilPilote2_Stop();	// RAZ flag IT Timer
	
	ul_EspionCompteurITTimerFilPilote2++;
	
	if(uc_VersionHard == 1)	// Hard V0 -> pas de coupure - Hard V1 -> coupure
	{
		if(uc_Consigne_ZJ == uc_CHAUFFAGE_OFF || uc_Consigne_ZJ == uc_CHAUFFAGE_HORS_GEL)		FilPiloteZJ_PutVal(0);
		if(uc_Consigne_ZN == uc_CHAUFFAGE_OFF || uc_Consigne_ZN == uc_CHAUFFAGE_HORS_GEL)		FilPiloteZN_PutVal(0);
		if(uc_Consigne_SDB1 == uc_CHAUFFAGE_OFF || uc_Consigne_SDB1 == uc_CHAUFFAGE_HORS_GEL)	FilPiloteSDB1_PutVal(0);
		if(uc_Consigne_SDB2 == uc_CHAUFFAGE_OFF || uc_Consigne_SDB2 == uc_CHAUFFAGE_HORS_GEL)	FilPiloteSDB2_PutVal(0);
	}
}

void FilPiloteZJ_PutVal(unsigned char Val)
{
  if (Val) {
    setReg8(MFC_SETTA, 0x01U);             /* SETTA0=0x01U */
  } else { /* !Val */
    setReg8(MFC_CLRTA, 0xFEU);             /* CLRTA0=0x00U */
  } /* !Val */
}

void FilPiloteZN_PutVal(unsigned char Val)
{
  if (Val) {
    setReg8(MFC_SETDD, 0x01U);             /* SETDD0=0x01U */
  } else { /* !Val */
    setReg8(MFC_CLRDD, 0xFEU);             /* CLRDD0=0x00U */
  } /* !Val */
}

void FilPiloteSDB1_PutVal(unsigned char Val)
{
  if (Val) {
    setReg8(MFC_SETDD, 0x02U);             /* SETDD1=0x01U */
  } else { /* !Val */
    setReg8(MFC_CLRDD, 0xFDU);             /* CLRDD1=0x00U */
  } /* !Val */
}

void FilPiloteSDB2_PutVal(unsigned char Val)
{
  if (Val) {
    setReg8(MFC_SETDD, 0x04U);             /* SETDD2=0x01U */
  } else { /* !Val */
    setReg8(MFC_CLRDD, 0xFBU);             /* CLRDD2=0x00U */
  } /* !Val */
}

// Cette fonction doit être appelée toutes les secondes
// Gère les candencements pour les modes ECO+ et ECO++
void vd_GestionFilPilote(void)//xxx a appeler sous timer ?
{
	vd_GestionCompteurEcoEtEcoPlus(&uc_CompteurSecondesEcoEtEcoPlus_ZJ,&uc_RAZCompteurSecondesEcoEtEcoPlus_ZJ,&uc_EcoEnCours_ZJ,&uc_EcoPlusEnCours_ZJ);
	vd_GestionCompteurEcoEtEcoPlus(&uc_CompteurSecondesEcoEtEcoPlus_ZN,&uc_RAZCompteurSecondesEcoEtEcoPlus_ZN,&uc_EcoEnCours_ZN,&uc_EcoPlusEnCours_ZN);
	vd_GestionCompteurEcoEtEcoPlus(&uc_CompteurSecondesEcoEtEcoPlus_SDB1,&uc_RAZCompteurSecondesEcoEtEcoPlus_SDB1,&uc_EcoEnCours_SDB1,&uc_EcoPlusEnCours_SDB1);
	vd_GestionCompteurEcoEtEcoPlus(&uc_CompteurSecondesEcoEtEcoPlus_SDB2,&uc_RAZCompteurSecondesEcoEtEcoPlus_SDB2,&uc_EcoEnCours_SDB2,&uc_EcoPlusEnCours_SDB2);
}

void vd_GestionCompteurEcoEtEcoPlus(unsigned char *puc_Compteur, unsigned char *puc_FlagRAZ, unsigned char *puc_EcoEnCours, unsigned char *puc_EcoPlusEnCours)
{
	if(*puc_FlagRAZ != 0)	// RAZ demandé suite changement de mode X -> ECO/ECO+
	{
		*puc_FlagRAZ = 0;
		*puc_Compteur = 0;	// Réinitialiser compteur pour regénérer tout de suite
	}
	
	if(*puc_Compteur < 3)		*puc_EcoEnCours = 1;
	else						*puc_EcoEnCours = 0;
		
	if(*puc_Compteur < 7)		*puc_EcoPlusEnCours = 1;
	else						*puc_EcoPlusEnCours = 0;
		
	(*puc_Compteur)++;
	if(*puc_Compteur >= 300)	*puc_Compteur = 0;	// 300 secondes -> 5 minutes
}

// xxx exemple code acces registres -> reecrire comme ca !
//VMCF5225_STRUCT_PTR mcf5225_ptr;
//   unsigned short us_PNQPAR;
//
//   if (device >= BSP_ENET_DEVICE_COUNT) 
//      return;
//
//   mcf5225_ptr = _PSP_GET_IPSBAR();
//   
//   us_PNQPAR = mcf5225_ptr->GPIO.PNQPAR;	// !!! Just change functions for IRQ 3 and 5
//   us_PNQPAR = us_PNQPAR & 0xF00F;
//   us_PNQPAR |= 0x0880;
//   mcf5225_ptr->GPIO.PNQPAR = us_PNQPAR;	//0x880
//   mcf5225_ptr->GPIO.PTIPAR = 0xFF;
//   mcf5225_ptr->GPIO.PTJPAR = 0xFF;
