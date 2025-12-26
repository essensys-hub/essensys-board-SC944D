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
#include "anemo.h"
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


//IPSBAR Offset:0x13_0006 (EPFR)
//#define MCF_EPORT_EPFR                    (*(volatile unsigned char *) (0x40130006UL))


#define MFC_GPTIOS							(*(volatile unsigned char *)(0x401A0000UL))
#define MFC_GPCFORC							(*(volatile unsigned char *)(0x401A0001UL))
#define MFC_GPTOC3M							(*(volatile unsigned char *)(0x401A0002UL))
#define MFC_GPTSCR1							(*(volatile unsigned char *)(0x401A0006UL))
#define MFC_GPTSCR2							(*(volatile unsigned char *)(0x401A000DUL))
#define MFC_GPTC3							(*(volatile unsigned short *)(0x401A0016UL))
#define MFC_GPTPACTL						(*(volatile unsigned char *)(0x401A0018UL))
#define MFC_GPTPACNT						(*(volatile unsigned short *)(0x401A001AUL))
#define MFC_GPTDDR							(*(volatile unsigned char *)(0x401A001EUL))
#define MFC_GPTFLG1							(*(volatile unsigned char *)(0x401A000EUL))
#define MFC_GPTFLG2							(*(volatile unsigned char *)(0x401A000FUL))
#define MFC_GPTPAFLG						(*(volatile unsigned char *)(0x401A0019UL))
#define MFC_GPTPORT							(*(volatile unsigned char *)(0x401A001DUL))
#define MFC_GPTCTL1							(*(volatile unsigned char *)(0x401A009DUL))


void vd_AnemoInit(void)
{
	MFC_GPTPACTL = 0;
	MFC_GPTSCR1 = 0;
	//MFC_GPTSCR2 = 0; // pas de pull up sur entrée
	MFC_GPTSCR2 = 0x20; // pull up sur entrée
	MFC_GPTIOS = 0x0F;
	MFC_GPTCTL1 = 0;
	MFC_GPTOC3M = 0;
	MFC_GPTFLG1 = 0x0F;
	MFC_GPTPORT &= ~0xF8;	//clrReg8Bits(GPTPORT, 0xF8U);
	MFC_GPTDDR &= ~0xF8;	//clrReg8Bits(GPTDDR, 0xF8U);
	MFC_GPTFLG2 = 0x80;		//setReg8(GPTFLG2, 0x80U);
	MFC_GPTPAFLG = 0x03;	//setReg8(GPTPAFLG, 0x03U);
	//MFC_GPTPACTL = 0x40 +	// Pulse accumulator enabled
	//			   0x00;	// Event counter mode
	MFC_GPTPACTL = 0x50;	//setReg8(GPTPACTL, 0x50U); Rising PAI edge increments counter
	//MFC_GPTPACTL = 0x40;	//setReg8(GPTPACTL, 0x50U); Falling PAI edge increments counter
	
	MFC_GPTSCR1 = 0x80;	// Active GPT timer
}

// A appeler chaque seconde
// Effectue l'acquisition de la fréquence
// Compare les 3 dernières valeurs -> ordonne la remontée du store si les 3 valeurs dépassent le seuil (si store pas utilisé en volet)
// Envoyer l'ordre au maximum toutes les 30 secondes
// Forcer temps pilotage relais au maxi -> FAIT PAR ECRAN LORS ACTIVATION / DESACTIVATION FONCTION REMONTEE AUTO
//
//Store_VR = Cle_Acces_Distance + Cle_Acces_Distance_TAILLE, // Ce registre ne sert que pour l'IHM.
//                                            // 0x00 = Store utilisé comme store.
//                                            // 0x01 = Store utilisé comme 15ème VR.
//                                            // ETAT INITIAL = 0x00.
//Store_Vitesse_Vent_Repliage,
//                                            // 0d = Pas de repliage automatique. De 1d à 255d = Vitesse du vent en km/h qui fait replier le store automatiquement.
//                                            // ETAT INITIAL = 0x00.
//Store_Vitesse_Vent_Instantane,
//                                            // Vitesse du vent en km/h de 0d à 255d.               
//                                            // ETAT INITIAL = 0x00.
void vd_AnemoGestion(void)
{
	static unsigned char s_uc_TempoEnvoiOrdresBA = 30;
	unsigned char l_uc_Compteur;
	unsigned short l_us_VitesseVent;
	unsigned char l_uc_SeuilDepassee;
	
	
	if(s_uc_TempoEnvoiOrdresBA <= 30)	s_uc_TempoEnvoiOrdresBA++;	
	
	l_us_VitesseVent = MFC_GPTPACNT;	// Acquisition vitesse vent
	MFC_GPTPACNT = 0;
	
	l_us_VitesseVent = l_us_VitesseVent / 2;	// Conversion points -> vitesse de vent : diviser par 2
	
	if(l_us_VitesseVent > 200)		l_us_VitesseVent = 200;
	
	for(l_uc_Compteur = 0;l_uc_Compteur < (sizeof(uc_VitesseVent)-1);l_uc_Compteur++)
	{
		uc_VitesseVent[l_uc_Compteur] = uc_VitesseVent[l_uc_Compteur+1];
	}
	uc_VitesseVent[sizeof(uc_VitesseVent)-1] = (unsigned char)l_us_VitesseVent;	// Nouvelle valeur
	Tb_Echange[Store_Vitesse_Vent_Instantane] = (unsigned char)l_us_VitesseVent;

	
	vd_EspionRS_Printf(uc_ENTREE_VENT,"VENT N-3 A N :");
	l_uc_SeuilDepassee = 1;
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_VitesseVent);l_uc_Compteur++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ENTREE_VENT," %3d", uc_VitesseVent[l_uc_Compteur]);
		if(uc_VitesseVent[l_uc_Compteur] <= Tb_Echange[Store_Vitesse_Vent_Repliage])	l_uc_SeuilDepassee = 0;
	}
	vd_EspionRS_PrintfSansHorodatage(uc_ENTREE_VENT,"\n");
	if((MFC_GPTPORT & 0x08) == 0)			uc_Etat_DIN_VITESSE_VENT = 0;
	else									uc_Etat_DIN_VITESSE_VENT = 1;
	
	
	// Gestion remontee automatique du store si vent > seuil
	if(Tb_Echange[Store_VR] == 0 && Tb_Echange[Store_Vitesse_Vent_Repliage] != 0)	// Store utilisé store et fonction remontée auto activée
	{
		if(l_uc_SeuilDepassee != 0)	// Les x dernières valeurs sont > seuil -> remontée store
		{
			if(s_uc_TempoEnvoiOrdresBA >= 30)
			{
				//Tb_Echange[Volets_PDE_Temps+3] = 255;	// Temps pilotage relais au maximum		// +3 store terrasse - FAIT PAR ECRAN LORS ACTIVATION / DESACTIVATION FONCTION REMONTEE AUTO
				
				us_Tb_Echangeboitier[us_SVolets_BA1_Cdeouvre+uc_BOITIER_PIECES_EAU] |= 0x08;	//xxx mutex
				vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"REMONTEE STORE A CAUSE DU VENT\n");
				s_uc_TempoEnvoiOrdresBA = 0;
			}
		}
	}
	else
	{
		s_uc_TempoEnvoiOrdresBA = 30;
	}
}

