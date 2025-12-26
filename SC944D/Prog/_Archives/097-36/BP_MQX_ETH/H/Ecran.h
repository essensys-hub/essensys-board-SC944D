
extern unsigned char uc_fct_Calcul_CheckSum(unsigned char *p_uc_buffer, unsigned short us_Nb_Octets);
extern void vd_EnvoyerTrameStatus(MQX_FILE_PTR fd);
extern void vd_Screen_write(MQX_FILE_PTR fd, uchar_ptr buffer, uint_16 length);       
extern void vd_RS_Screen_read(MQX_FILE_PTR fd);
extern void vd_DecalerBufferReception(void);
extern void vd_AnalyserOctetsRecus(MQX_FILE_PTR fd);
extern void vd_EnvoyerTrameSynchro(MQX_FILE_PTR fd);
extern void vd_EnvoyerTrameLectureDonneesDiscretes(MQX_FILE_PTR fd);
extern void vd_EnvoyerTrameEcritureDonneesDiscretes(MQX_FILE_PTR fd);
extern void vd_EnvoyerTrameLectureDonneesBloc(MQX_FILE_PTR fd);
extern void vd_EnvoyerTrameEcritureDonneesBloc(MQX_FILE_PTR fd);
extern void vd_Ecran_task(uint_32 dummy);
extern void vd_RestartTimerEcranTimeOUT(void);
extern void vd_EcranTimeOUT(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr);
