/* MD5 context. */
typedef struct {
	unsigned long int state[4];                                   /* state (ABCD) */
	unsigned long int count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

//void MD5Init PROTO_LIST ((MD5_CTX *));
//void MD5Update PROTO_LIST
//  ((MD5_CTX *, unsigned char *, unsigned int));
//void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *)); ///XXX ces prototype etait ceux donner dan slexemple .. mais marche pas donc j'ai refait les mien
/*
 * 3 Etapes pour utilise le MD5
 * d'abord on crée une variable de type MD5_CTX que l'on donne l'adresse la fonction MD5Init
 */
void MD5Init (MD5_CTX *);
/**
 * ensuite on utilise cette fonction en passant l'adresse du md5_ctx initialisé , notre chaine d'octet et la taille de cette chaine
 * (dans notre exemple on utilise une chaine de caractère)
 */
void MD5Update (MD5_CTX *context,                                    /* context */

unsigned char *input,                                /* input block */
unsigned int inputLen);
/**
 * cette fonction "digere" (terme utilisé ) la chaine pour produire notre MD5
 */
void MD5Final(unsigned char digest[16],                       /* message digest */
MD5_CTX *context                                 /* context */
);
