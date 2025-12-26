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
#include "spi.h"

#define extern
#include "eepromspi.h"
#undef extern


// Ouvre le port SPI - Ne configure par le CS à utiliser !!!
void vd_SpiOpen(void)
{
	signed long l_sl_Retour;
	signed long l_sl_param;

	
	uc_PortSPIouvert = 0;
	fd_SPI_EEPROM = fopen("spi0:", NULL);
	if(fd_SPI_EEPROM == NULL)
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tOPEN PB\n");
	}
	else
	{
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tOPEN OK\n");

		// Configuration
		l_sl_param = SPI_DEVICE_MASTER_MODE;
		l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_TRANSFER_MODE, &l_sl_param);
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG MASTER %d\n", l_sl_Retour);
		
		if(l_sl_Retour == SPI_OK)
		{
			l_sl_param = SPI_CLK_POL_PHA_MODE0;
			l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_MODE, &l_sl_param);
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG MODE %d\n", l_sl_Retour);
		}

		if(l_sl_Retour == SPI_OK)
		{
			l_sl_param = SPI_DEVICE_BIG_ENDIAN;
			l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_ENDIAN, &l_sl_param);
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG MSB/LSB %d\n", l_sl_Retour);
		}

		if(l_sl_Retour == SPI_OK)
		{
			l_sl_param = 500000;
			l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_BAUD, &l_sl_param);
			vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG VITESSE %d\n", l_sl_Retour);
		}
		
		if(l_sl_Retour == SPI_OK)
		{
			uc_PortSPIouvert = 1;
		}
	}
}

void vd_SpiSelectEEPROMAdresseMac(void)
{
	signed long l_sl_Retour;
	signed long l_sl_param;

	if(uc_PortSPIouvert != 0)
	{
		l_sl_param = MCF5XXX_QSPI_QDR_QSPI_CS0;
		l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_CS, &l_sl_param);
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG CS %d\n", l_sl_Retour);
	}
}

void vd_SpiSelectEEPROMSoft(void)
{
	signed long l_sl_Retour;
	signed long l_sl_param;

	if(uc_PortSPIouvert != 0)
	{
		l_sl_param = MCF5XXX_QSPI_QDR_QSPI_CS2;
		l_sl_Retour = ioctl(fd_SPI_EEPROM, IO_IOCTL_SPI_SET_CS, &l_sl_param);
		vd_EspionRS_Printf(uc_ESPION_TACHE_PRINCIPALE_ACTIVITE,"SPI\tCONFIG CS %d\n", l_sl_Retour);
	}
}
