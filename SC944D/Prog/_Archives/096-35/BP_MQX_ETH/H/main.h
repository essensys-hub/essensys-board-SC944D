
extern void Main_task(uint_32 initial_data);
extern void vd_Init_Echange(void);
extern void vd_InitVariablesGlobales(void);
extern unsigned char uc_ErreurDetectee(void);
extern void vd_StartTacheEthernet(void);
extern void vd_StartTacheEcran(void);
extern void vd_StartTacheBA(void);
extern void vd_StartTacheTeleinfo(void);
extern void vd_MAJEtatBPDansTableEchange(void);
extern void vd_Timer50ms_Traitement(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr);
extern void vd_Timer50ms_Init(void);
extern void vd_Timer1sec_Traitement(_timer_id id, pointer data_ptr, MQX_TICK_STRUCT_PTR tick_ptr);
extern void vd_Timer1sec_Init(void);
