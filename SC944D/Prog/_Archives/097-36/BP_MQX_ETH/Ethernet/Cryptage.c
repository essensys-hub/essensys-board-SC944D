#include <mqx.h>
#include <bsp.h>
#include <timer.h>
#include <mutex.h>
#include <message.h>
#include "application.h"	//xxx

#include <spi.h>

#include "TableEchange.h"
#include "TableEchangeAcces.h"
#include "EspionRS.h"
#include "ba_i2c.h"
#include "global.h"
#include "crc.h"
#include "eepromspi.h"

#include "Cryptagerijndael_mode.h"
#include "Cryptagemd5.h"
#include "cryptagecencode.h"

#define extern
#include "cryptage.h"
#undef extern


/*unsigned char* ASCII_2_UTF16(unsigned char * TexteASCII, unsigned short size)
{
	short i;
	unsigned char * UTF16LE=malloc(sizeof(char)*2*size);
	//vidage par des 0
	for(i=0;i<size*2;i++)
	{
		UTF16LE[i]=0;
	}
	size--;	
	for(i=size;i>=0;i--)
	{
		UTF16LE[i*2]=TexteASCII[i];
	}
	return UTF16LE;
	
	
}*/

// Crypte le code numérique du système (donné par le site web au moment de l'inscription)
// Code numérique : 24 caractères (chiffres ou lettes de A à F)
// -> Cryptage en MD5
// -> Transformation sous la forme 16premierschiffres:16dernierschiffres
// -> Encodage en Base64
// -> A envoyer à chaque requête au serveur dans le header Authorization
extern char c_MatriculeCryptee[46];//xxx pas joli ca !
extern char c_CleDecryptageAlarme[46];//xxx pas joli ca !
void cryptage(void)	//char * chaine)
{
	//unsigned	char * data;
	unsigned char md5[16];//todo mettre a 16
	unsigned char uc_CleServeur[33];
	unsigned char l_uc_Compteur;
	char md5chaine[33];
	base64_encodestate base64State;
	//char base64[46]={0};
	char tempHex[4];
	unsigned char i=0;
	MD5_CTX TestMD5;
	//ASCII to UTF16
	//data=ASCII_2_UTF16((unsigned char *)chaine,24);
	//MD5 
	
	for(l_uc_Compteur = 0;l_uc_Compteur < sizeof(uc_Cle_Acces_Distance);l_uc_Compteur++)
	{
		uc_CleServeur[(l_uc_Compteur*2)] = ((uc_Cle_Acces_Distance[l_uc_Compteur]) & 0x0F) + '0';
		uc_CleServeur[(l_uc_Compteur*2)+1] = ((uc_Cle_Acces_Distance[l_uc_Compteur] >> 4) & 0x0F) + '0';
	}
	uc_CleServeur[32] = 0;
	
	md5chaine[0] = 0;
	c_CleDecryptageAlarme[0] = 0;
		
	//MD5 
	MD5Init(&TestMD5);
	MD5Update(&TestMD5,(unsigned char *)uc_CleServeur,32);	//24
	MD5Final(md5,&TestMD5);
	
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"chaine a crypter : %s\n", uc_CleServeur);
	
	//FORMATAGE DU MD5
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"md5 : ");
	for(i=0;i<8;i++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%02x", md5[i]);
		sprintf(tempHex,"%02x",md5[i]);
		strcat(md5chaine,tempHex);
		strcat(c_CleDecryptageAlarme,tempHex);
	}
	strcat(md5chaine,":");
	for(i=8;i<16;i++)
	{
		vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"%02x", md5[i]);
		sprintf(tempHex,"%02x",md5[i]);
		strcat(md5chaine,tempHex);			
	}
	base64_init_encodestate(&base64State);
	c_MatriculeCryptee[0] = 0;
	base64_encode_block(md5chaine,33,c_MatriculeCryptee,&base64State);
	vd_EspionRS_PrintfSansHorodatage(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"\n");
	vd_EspionRS_Printf(uc_ESPION_TACHE_ETHERNET_ACTIVITE,"base64 : %s\n",c_MatriculeCryptee);
	//return c_MatriculeCrypteex;
}

struct stAESParse UnParceAES (char * AESFormater)
{
	char * temp;
	unsigned char i=0;
	unsigned char taille=0;
	struct stAESParse Data;
	Data.taille=0;
	temp=strstr(AESFormater,";");
	while(temp!=NULL)
	{
		//TODO , faire une fonction "tab Nombre DecoupeurStringToNombre(String,ChaineARechercher)"
		taille=temp-AESFormater;
		if (taille != 0) {
			temp = malloc(taille + 1);
			for (i = 0; i < taille; i++) {
				temp[i] = AESFormater[i];
			}
			temp[taille] = '\0';
			Data.data[Data.taille] = (unsigned char) atoi(temp);
			free(temp);
		}
		Data.taille++;
		AESFormater=strstr(AESFormater,";");
		AESFormater++;
		temp=strstr(AESFormater,";");
	}
	return Data;
}
//char * DechiffrageAlarme(char * Message,char * matricul)
//{
//	struct stAESParse AESParse;
//	MD5_CTX md5;
//	char tempHex[4];
//	char IV[16]={0};
//	unsigned char md5Final[16]={0};
//	unsigned char i=0;
//	RIJNDAEL_context Rij;
//	char Decryptag[32]={0};
//	
//	AESParse=UnParceAES(Message);
//	MD5Init(&md5);
//	MD5Update(&md5,(unsigned char *)matricul,strlen((const char *)matricul));
//	MD5Final(md5Final,&md5);
//	for(i=0;i<8;i++)
//	{
//		sprintf(tempHex,"%02x",md5Final[i]);
//		strcat(IV,tempHex);
//	}
//	Rij.mode=MODE_CBC;
//	rijndael_setup(&Rij,24,(unsigned char *)matricul);
//
//	block_decrypt(&Rij,(unsigned char *)AESParse.data,AESParse.taille,(unsigned char *)Decryptag,(unsigned char *)IV);
//	Decryptag[31]='\0';
//	printf("\r\n");
//	printf((const char *)Decryptag);
//	printf("\r\n");
//	return Decryptag;
//}
