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


extern void __boot(void);	//extern asm void asm_startmeup(void);


#pragma define_section _APP_JUMP ".APP_JUMP" far_absolute R
#pragma section _APP_JUMP begin

asm void APP_CALL(void)
{
	jmp   __boot;	//jmp   asm_startmeup;
}
#pragma section _APP_JUMP end


#pragma define_section _APP_CRC ".APP_CRC" far_absolute R
#pragma section _APP_CRC begin

unsigned short CRC = 0x0102;

#pragma section _APP_CRC end


#pragma define_section _APP_VERSION ".APP_VERSION" far_absolute R
#pragma section _APP_VERSION begin

unsigned short VERSION = us_BP_VERSION_SERVEUR;

#pragma section _APP_VERSION end
