
extern void vd_Chauffage(void);
extern void vd_Chauffage_ParZone(pstruct_chauffage pst_chauf, unsigned char *p_uc_planning, unsigned char *uc_ModeDemande, unsigned char *uc_ConsignePourPilotage);
extern unsigned char uc_Plage(unsigned char *p_uc_planning, unsigned char uc_ind);
extern void vd_DelestageRAZ(void);
extern void vd_DelestageAugmenterNiveau(void);
extern void vd_DelestageDiminuerNiveau(void);
extern void vd_DelestageUpdateConsigneZones(void);
extern void vd_Chauffage_Espion(void);
extern void vd_Chauffage_EspionParZone(struct_chauffage st_EtatZone, pstruct_chauffage p_st_EtatZonePrecedent, const char *l_p_uc_Libelle);
extern void vd_CalculerEtatConsigne(struct_chauffage st_chauf, unsigned char *p_uc_buffer);
