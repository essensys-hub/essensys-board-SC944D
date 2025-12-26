
// Definition des droits d'acces de chaque valeur de la table d'échange
// ACCES LECTURE ECRITURE : VALABLE POUR ACCES ECRAN & SERVEUR (FONCTIONS uc_TableEchange_Ecrit_Data ET uc_TableEchange_Lit_Data) - ACCES LIBRE PAR BP
#define ACCES_AUCUN			0x00	// AUCUN ACCES
#define ACCES_LECTURE		0x01	// LECTURE AUTORISEE
#define ACCES_ECRITURE		0x02	// ECRITURE AUTORISEE
#define VALEUR_SAUVEGARDEE	0x80	// VALEUR SAUVEGARDEE EN FLASH *
//				* Toutes les valeurs sont sauvegardées en FLASH mais...
//				  Au démarrage, toutes les valeurs sont d'abord initialisées avec leurs valeurs par défaut
//				  Pour celles qui ont l'attribut 0x80, valeur lue en FLASH recopiée dans table d'échange
//				  Certaines valeurs (code alarme) ne sont pas sauvegardées en FLASH, mais gérées séparemment -> NE PAS METTRE VALEUR_SAUVEGARDEE pour ces valeurs

#define __	ACCES_AUCUN
#define R_	ACCES_LECTURE
#define RW	ACCES_LECTURE + ACCES_ECRITURE
#define _W	ACCES_ECRITURE
#define RWS	ACCES_LECTURE + ACCES_ECRITURE + VALEUR_SAUVEGARDEE


#ifdef extern
	extern const unsigned char Tb_Echange_Droits[];
#else
	const unsigned char Tb_Echange_Droits[Nb_Tbb_Donnees] =
	{
			R_,	//	Version_SoftBP_Embedded
			R_,	//	Version_SoftBP_Web
			RWS,	//	Version_SoftIHM_Majeur
			RWS,	//	Version_SoftIHM_Mineur
			R_,	//	Version_TableEchange
			RW,	//	Minutes
			RW,	//	Heure
			RW,	//	Jour
			RW,	//	Mois
			RW,	//	Annee
			RW,	//	Status
			RW,	//	Alerte
			RW,	//	Information
			
			//	Chauf_zj_Auto - uc_PLANNING_CHAUFFAGE_TAILLE -> 84
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Chauf_zn_Auto - uc_PLANNING_CHAUFFAGE_TAILLE -> 84
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Chauf_zsb1_Auto - uc_PLANNING_CHAUFFAGE_TAILLE -> 84
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Chauf_zsb2_Auto - uc_PLANNING_CHAUFFAGE_TAILLE -> 84
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,

			RWS,	//	Chauf_zj_Mode
			RWS,	//	Chauf_zn_Mode
			RWS,	//	Chauf_zsb1_Mode
			RWS,	//	Chauf_zsb2_Mode
			
			RWS,	//	Cumulus_Mode
			RWS,	//	VacanceFin_H
			RWS,	//	VacanceFin_Mn
			RWS,	//	VacanceFin_J
			RWS,	//	VacanceFin_M
			RWS,	//	VacanceFin_A
			
			RWS,	//	VacanceFin_zj_Force
			RWS,	//	VacanceFin_zn_Force
			RWS,	//	VacanceFin_zsb1_Force
			RWS,	//	VacanceFin_zsb2_Force
			
			RWS,	//	Arrose_Mode
			
			//	Arrose_Auto - uc_PLANNING_ARROSAGE_TAILLE -> 42
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,
			
			RWS,	//	Arrose_Detect
			RWS,	//	Alarme_AccesADistance
			RW,	//	Alarme_Mode
			RW,	//	Alarme_Commande
			RW,	//	Alarme_CodeSaisiLSB
			RW,	//	Alarme_CodeSaisiMSB
			RW,	//	Alarme_Autorisation
			RW,	//	Alarme_SuiviAlarme
			RW,	//	Alarme_Detection
			RW,	//	Alarme_Fraude
			RW,	//	Alarme_SuiviChangementCode
			__,	//	Alarme_CodeUser1LSB - UNIQUEMENT EN ECRITURE INDIRECTE !!! - SAUVEGARDE INDEPENDANTE !!!
			__,	//	Alarme_CodeUser1MSB - UNIQUEMENT EN ECRITURE INDIRECTE !!! - SAUVEGARDE INDEPENDANTE !!!
			__,	//	Alarme_CodeUser2LSB - UNIQUEMENT EN ECRITURE INDIRECTE !!! - PAS UTILISE ACTUELLEMENT DONC PAS DE SAUVEGARDE !!!
			__,	//	Alarme_CodeUser2MSB - UNIQUEMENT EN ECRITURE INDIRECTE !!! - PAS UTILISE ACTUELLEMENT DONC PAS DE SAUVEGARDE !!!
			R_,	//	Alarme_CompteARebours
			RW,	//	Alarme_Reserve
			RW,	//	Alarme_TestRAZPresence
			RW,	//	Alarme_TestSirenes
			
			//	AlarmeConfig - AlarmeConfig_NB_VALEURS -> 11
			RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS, RWS,
			
			RWS,	//	Alerte_Intensite
			RWS,	//	Alerte_Duree
			
			RW,	//	Alerte_TestSirene
			RW,	//	Alerte_Acquit
			
			RWS,	//	Securite_PriseCoupe
			RWS,	//	Securite_FuiteLinge
			RWS,	//	Securite_FuiteVaisselle
			RWS,	//	Securite_FuiteAlerte
			
			RWS,	//	Reveil_ChambreGr_H
			RWS,	//	Reveil_ChambreGr_Mn
			RWS,	//	Reveil_ChambreGr_ON
			RWS,	//	Reveil_Chambre1_H
			RWS,	//	Reveil_Chambre1_Mn
			RWS,	//	Reveil_Chambre1_ON
			RWS,	//	Reveil_Chambre2_H
			RWS,	//	Reveil_Chambre2_Mn
			RWS,	//	Reveil_Chambre2_ON
			RWS,	//	Reveil_Chambre3_H
			RWS,	//	Reveil_Chambre3_Mn
			RWS,	//	Reveil_Chambre3_ON
			RWS,	//	Reveil_Bureau_H
			RWS,	//	Reveil_Bureau_Mn
			RWS,	//	Reveil_Bureau_ON
			
			RWS,	//	Delestage
			
			RW,	//	TeleInf_OPTARIF
			RW,	//	TeleInf_PTEC
			RW,	//	TeleInf_ADPS
			RW,	//	TeleInf_PAPP_LSB
			RW,	//	TeleInf_PAPP_MSB
			
			RWS,	//	TeleInf_HPB_Global_LSB
			RWS,	//	TeleInf_HPB_Global_MSB
			RWS,	//	TeleInf_HPB_Chauffage_LSB
			RWS,	//	TeleInf_HPB_Chauffage_MSB
			RWS,	//	TeleInf_HPB_Refroid_LSB
			RWS,	//	TeleInf_HPB_Refroid_MSB
			RWS,	//	TeleInf_HPB_EauChaude_LSB
			RWS,	//	TeleInf_HPB_EauChaude_MSB
			RWS,	//	TeleInf_HPB_Prises_LSB
			RWS,	//	TeleInf_HPB_Prises_MSB
			RWS,	//	TeleInf_HPB_Autres_LSB
			RWS,	//	TeleInf_HPB_Autres_MSB
			
			RWS,	//	TeleInf_HC_Global_LSB
			RWS,	//	TeleInf_HC_Global_MSB
			RWS,	//	TeleInf_HC_Chauffage_LSB
			RWS,	//	TeleInf_HC_Chauffage_MSB
			RWS,	//	TeleInf_HC_Refroid_LSB
			RWS,	//	TeleInf_HC_Refroid_MSB
			RWS,	//	TeleInf_HC_EauChaude_LSB
			RWS,	//	TeleInf_HC_EauChaude_MSB
			RWS,	//	TeleInf_HC_Prises_LSB
			RWS,	//	TeleInf_HC_Prises_MSB
			RWS,	//	TeleInf_HC_Autres_LSB
			RWS,	//	TeleInf_HC_Autres_MSB
			
			RWS,	//	TeleInf_Repartition_Chauffage
			RWS,	//	TeleInf_Repartition_Refroid
			RWS,	//	TeleInf_Repartition_EauChaude
			RWS,	//	TeleInf_Repartition_Prises
			RWS,	//	TeleInf_Repartition_Autres
			
			//	Variateurs_PDV_Conf - uc_NB_VARIATEURS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Variateurs_CHB_Conf - uc_NB_VARIATEURS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Variateurs_PDE_Conf - uc_NB_VARIATEURS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			
			//	Lampes_PDV_Temps - uc_NB_LAMPES_POSSIBLES_PAR_BA -> 16
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Lampes_CHB_Temps - uc_NB_LAMPES_POSSIBLES_PAR_BA -> 16
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Lampes_PDE_Temps - uc_NB_LAMPES_POSSIBLES_PAR_BA -> 16
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			
			//	Volets_PDV_Temps - uc_NB_VOLETS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Volets_CHB_Temps - uc_NB_VOLETS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,
			//	Volets_PDE_Temps - uc_NB_VOLETS_POSSIBLES_PAR_BA -> 8
			RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,

			RW,		//	Scenario
			RWS,	//	Scenario_DernierLance
			
			// Scenario -> Scenario_NB_VALEURS + AlarmeConfig_NB_VALEURS -> 30 + 11 -> 41
			//	Scenario1
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario2
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario3
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario4
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario5
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario6
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario7
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,
			//	Scenario8
			RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS,RWS, RWS,

			RW,	//	EtatBP1
			RW,	//	EtatBP2
			
			//	Cle_Acces_Distance - Cle_Acces_Distance_TAILLE -> 16 - ACCES ECRITURE UNIQUEMENT - - SAUVEGARDE INDEPENDANTE EN EEPROM ET PAS EN FLASH
			_W,_W,_W,_W, _W,_W,_W,_W, _W,_W,_W,_W, _W,_W,_W,_W,
			
			RWS,	//	Store_VR
			RWS,	//	Store_Vitesse_Vent_Repliage
			
			RW,		//	Store_Vitesse_Vent_Instantane
			
			R_,		//	Constructeur_CodeLSB - CODE EN DUR
			R_,		//	Constructeur_CodeMSB - CODE EN DUR
			
			R_,		//	Test_ETOR_1,// Read only
			R_,		//	Test_ETOR_2,// Read only
			
			R_,		//	EtatEthernet,// Read only
			
			RW,		//	Mode_Test Pas de sauvegarde !
			
			
			R_,		//	AdresseMAC_1,	// Adresse MAC du BP (6 octets)// Read only
			R_,		//	AdresseMAC_2,
			R_,		//	AdresseMAC_3,
			R_,		//	AdresseMAC_4,
			R_,		//	AdresseMAC_5,
			R_,		//	AdresseMAC_6,
	};
#endif
