
extern void TeleInfo_task(uint_32 dummy);
extern void vd_StartTimerTeleinfo(void);
extern void vd_TeleinfoTimer(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr);
extern void vd_RSTeleinfoRead(MQX_FILE_PTR fd);
extern void vd_DecalerTableauReception(unsigned short us_NbOctets);
extern unsigned char uc_fct_Calcul_CheckSum_Tele(unsigned char *puc_buffer, uint_16 us_Nb_Octets);
extern void vd_AnalyserOctetsTeleinfoRecus(void);
extern void vd_GestionDataTeleinfo(void);
extern void vd_TeleInfExtractionEtiquetteDonnee(char_ptr p_Etiq, char_ptr p_Donnee);
extern unsigned long ul_ArrondirValeur(unsigned long ul_Valeur);
extern void vd_CalculerRepartition(unsigned long ul_Difference, unsigned long *ul_Cumul, unsigned short us_IndexCoefRepartition, unsigned short us_IndiceValeurLSB, unsigned short us_IndiceValeurMSB);
