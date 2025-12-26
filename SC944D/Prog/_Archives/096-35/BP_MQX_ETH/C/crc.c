#include "crc.h"


unsigned short us_CalculerCRCSurTrame(unsigned char uc_Buffer[], unsigned char uc_NbOctets)
{ 
	unsigned short l_us_CRC;
        unsigned char l_uc_Compteur;

	l_us_CRC = 0xFFFF;
	
	for(l_uc_Compteur = 0;l_uc_Compteur < uc_NbOctets;l_uc_Compteur++)
	{
		l_us_CRC = us_CalculerCRCSurUnOctet(l_us_CRC,(unsigned char)uc_Buffer[l_uc_Compteur]);							//poid faible
	}
	return l_us_CRC;
}

unsigned short us_CalculerCRCSurUnOctet(unsigned short l_us_CRC, unsigned char l_uc_Octet)
{
	unsigned char l_uc_Compteur2;

	l_us_CRC = (unsigned short)(l_us_CRC ^ (unsigned short)l_uc_Octet);
	for(l_uc_Compteur2 = 0 ; l_uc_Compteur2 < 8 ; l_uc_Compteur2++)
	{
		if((l_us_CRC & 0x0001) != 0)
		{
			l_us_CRC = (unsigned short)(l_us_CRC >> 1);
			l_us_CRC = (unsigned short)(l_us_CRC ^ (unsigned short)0xA001);
		}
		else
		{
			l_us_CRC = (unsigned short)(l_us_CRC >> 1);
		}
	}
	return l_us_CRC;
}

