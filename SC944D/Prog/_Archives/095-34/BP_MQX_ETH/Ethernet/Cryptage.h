
extern unsigned char* ASCII_2_UTF16(unsigned char * TexteASCII, unsigned short size);
extern void cryptage(void);//char * cryptage(char * chaine);
extern struct stAESParse UnParceAES (char * AESFormater);
extern char * DechiffrageAlarme(char * Message,char * matricul);

struct stAESParse
{
	unsigned char taille;
	char data[255];
};
