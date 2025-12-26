
//extern unsigned char uc_fct_Calcul_CheckSum_I2C(unsigned char *p_uc_buffer, unsigned short us_NbOctets);
extern signed long sl_fct_write_polled(MQX_FILE_PTR fd,		// [IN] The file pointer for the I2C channel
		unsigned char uc_BANumber,							// [IN] BA number (it's not then slave number on i2c...)
		unsigned char *p_uc_buffer,							// [IN] The array of characters are to be written in slave
		unsigned short us_n,								// [IN] Number of bytes in that buffer
		unsigned char *p_uc_Answer							// [OUT] Value received in answer from I2C slave
		);
