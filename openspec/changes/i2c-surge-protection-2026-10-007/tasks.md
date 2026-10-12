## 1. Diagnostic et remise en service de l'installation touchée (#2)
Les tâches de ce groupe sont des opérations de maintenance manuelles, faites sur l'installation touchée par le propriétaire, un mainteneur ou une personne habilitée. Aucun harnais, script NR ni stimulus automatisé ne vise cette armoire. Toute publication sur l'issue #1 (dépôt public) se fait après accord, sous forme anonymisée.

- [ ] 1.1 Avant toute intervention, faire mesurer par une personne habilitée (BR ou BE Mesurage), avec le 230 V présent et sans toucher les cartes, la tension alternative entre un point 0 V TBT accessible et le PE, en mode basse impédance (LoZ). Vérification : quelques volts qui s'effondrent en LoZ. Sinon, arrêt et appel d'un électricien. La valeur est consignée dans le dossier privé de l'installation.
- [ ] 1.2 Observer sans rien toucher (étape E0, porte ouverte, en présence d'une personne habilitée) : filmer 10 s les fenêtres ÉTAT des trois BA et la LED TÉLÉINFO du BP. Envoyer ensuite une commande d'éclairage à chaque BA par l'interface habituelle et relire l'index 12 (`comm_faults`). Vérification : `(index 12 & 0x1C)` et l'état de DL1 sont relevés pour chaque BA, et un BA suspect est désigné.
- [ ] 1.3 Relever, avant l'expiration des historiques, les horodatages disponibles : journaux du réseau local, supervision des sources autonomes du site, dernière commande BA réussie côté serveur, historique du fournisseur d'énergie. Livrable : une chronologie dans le dossier privé, et une synthèse anonymisée en commentaire de l'issue #1, publiée après accord.
- [ ] 1.4 Couper toute liaison Ethernet en cuivre entre les équipements du site reliés au PE et le commutateur du BP : débranchement côté commutateur, puis passage en Wi-Fi ou en fibre si l'équipement est vivant. Vérification : le port est débranché et le câble étiqueté « ne pas rebrancher en cuivre ».
- [ ] 1.5 Faire consigner l'installation par une personne habilitée, selon la NF C 18-510 : disjoncteur du BP, tous les départs commandés par les BA, batterie du BP, et toute source autonome du site (PV, batterie, sortie secourue). Vérification : absence de tension constatée au VAT sur chaque bornier de BA et entre J1/J2 et le PE, avant toute introduction d'outil dans le coffret.
- [ ] 1.6 Mesurer l'isolement à 500 V continu, toutes les bornes 230 V reliées ensemble contre toutes les bornes TBT reliées ensemble. Commencer par le SC941C, puis les deux autres BA et le BP. Inspecter à la loupe la zone J1/J2–K1 et VR1 de chaque BA. Vérification :
  - plus de 100 MΩ : carte conforme ;
  - de 10 à 100 MΩ : nettoyer, sécher, puis refaire l'essai ;
  - moins de 10 MΩ : carte écartée.

  Les valeurs et toute trace d'amorçage sont consignées.
- [ ] 1.7 Isoler la carte fautive par débranchement (étapes E1/E2, mesure manuelle). Pendant ces étapes :
  - le 230 V est consigné ;
  - le BP est alimenté par une alimentation de laboratoire flottante de 22 à 24 V sur J56-1/J56-2.

  Retirer entièrement le câble du BA suspect, commander les BA restants et relire l'index 12. Rebrancher ensuite un BA à la fois en mesurant au BP l'écart entre +5VS et chaque ligne. Vérification : la carte fautive est désignée, c'est-à-dire celle dont le raccordement fait croître cet écart de plus de 50 mV ou empêche les autres BA d'acquitter.
- [ ] 1.8 Mesurer le BP à l'établi, hors tension puis sous tension :
  - D33/D34 en mode diode ;
  - continuité de L30, L31 et L32 ;
  - EN de U19 entre 3,6 et 4,4 V, et test fonctionnel de U19 dans les deux sens ;
  - J32 : environ 150 Ω entre les broches 1 et 3, et plus de 100 MΩ sous 500 V entre les contacts et le 0 V.

  Livrable : une fiche de mesures qui liste les composants défaillants.
- [ ] 1.9 Mesurer le BA défaillant à l'établi, en comparant à un BA sain dans la même polarité. Commencer par les mesures d'alimentation : rail « +12V » à environ 0,8 kΩ, D3, D6, U1 et FI1. Passer ensuite aux mesures I2C : environ 10 kΩ entre J1-1 et +5 V, environ 20 kΩ entre J1-1 et J1-2. Inspecter à la loupe la zone J1/J2–K1. Vérification : la cause de la panne est localisée, et le BA acquitte (ou non) son adresse 0x11, 0x12 ou 0x13 face à un maître I2C 5 V.
- [ ] 1.10 Réparer les cartes déposées.
  - **Avant toute retouche lourde du BP**, sauvegarder sa flash par BDM.
  - **Remplacer les composants défaillants** : D33/D34 en SMF5V0A-E3-08, et U19 si son test échoue. Sur le BA défaillant : U1, C2, D6, D3 et FI1 selon le diagnostic, ainsi que les varistances des SC943.
  - **Programmation.** Le BP ne reçoit que le `.S19` 099-37. Le PIC d'un BA ne reçoit que le `.hex` v1.7 de son type, après lecture de son EEPROM.

  Vérification :
  - le BP émet START, 0x22, NACK puis STOP à 50 kHz, avec un niveau bas inférieur à 0,6 V ;
  - le BA réparé acquitte son adresse face à un maître I2C 5 V.
- [ ] 1.11 Remonter les cartes, avec les retouches des groupes 2 et 3 faites pendant la dépose. Si l'écart de 0 V estimé en 2.1 dépasse 0,1 V, poser d'abord N2 ou N1 (4.7). Remettre l'installation sous tension, puis mesurer :
  - l'écart statique de 0 V (ΔVg) entre J31-3 et J1-3 du SC940D, relais collés, et appliquer la règle D1 : au-delà de 0,12 V sans liaison équipotentielle, les BA retouchés ne restent pas en service avant la pose de 4.7 ;
  - BA_ALIM au dernier BA (≥ 14 V) ;
  - le niveau bas au MCF (< 0,85 V) et au PIC (< 1,2 V) ;
  - le temps de montée (< 4 µs).

  L'électricien refait la mesure LoZ entre 0 V et PE. Vérification : tous les critères sont tenus et consignés.
- [ ] 1.12 Renvoyer la configuration de **tous** les BA : modifier chaque paramètre (modes des variateurs, temps d'extinction, temps des volets), puis revenir à sa valeur. Corriger le mode des variateurs dont le PIC a été remplacé. Vérification : chaque BA acquitte une commande ; pendant 24 h, au moins une commande par BA chaque heure est envoyée et consignée, et `(index 12 & 0x1C)` reste à 0. Une synthèse anonymisée est publiée sur l'issue #1 après accord.
- [ ] 1.13 Transmettre à l'électricien et à l'installateur PV la liste des contrôles hors du périmètre Essensys : parafoudre de tête et sa cartouche, parafoudre AC du coffret PV, journal de l'onduleur, équipotentialité, départs 230 V extérieurs. Ces contrôles ne bloquent pas la remise en service. Livrable : leur compte rendu, archivé dans le dossier privé.

## 2. Retouches des boîtiers auxiliaires (SC940D, SC941C, SC942C) (#3)
- [ ] 2.1 Mesurer à l'établi, sous 15,0 V et relais monostables collés, le courant d'entrée de chaque type de BA, puis la résistance de chaque tronçon de bus, contacts compris. En déduire l'écart de 0 V attendu au BA le plus éloigné. Appliquer la règle D1 du design, confirmée par la mesure de ΔVg en 1.11 :
  - jusqu'à 0,1 V : 47 Ω ;
  - de 0,1 à 0,12 V : 47 Ω, si le niveau bas au MCF reste sous 0,85 V ;
  - au-delà de 0,12 V : liaison équipotentielle (N2 ou N1) posée avant la remise en service, puis 47 Ω.

  Livrable : les mesures et la décision, consignées sur l'issue #1 après accord.
- [ ] 2.2 Sur chaque BA déposé, remplacer R1/R2 (R1/R3 sur le SC941C) par une résistance 0805 anti-impulsion de 47 Ω. Retoucher une carte à la fois. Vérification :
  - hors tension, 44 à 50 Ω entre J1-1 et RC7, puis entre J1-2 et RC6 ;
  - carte seule, alimentée sous 15,0 V par J1-4/J1-3, SDA et SCL non raccordées : écart entre le +5 V local et chaque ligne d'au plus 0,5 V (fuite ≤ 50 µA sous la pull-up de 10 kΩ) ;
  - sur le bus, niveau bas au MCF inférieur à 0,85 V et au PIC inférieur à 1,2 V (procédure de `NR-board-sc944d-1`).
- [ ] 2.3 Remplacer R79 (R93 sur le SC941C, R84 sur le SC942C), en 0603 ou 0805, par une PTC de 30 V au moins en montage volant (PTC CMS 1206 ou 1812 sur fils courts entre les pastilles de R79, collée), dont le courant de maintien à 50 °C vaut au moins 1,5 fois le courant mesuré en 2.1. Vérification : le BA démarre, le +5 V est entre 4,83 et 5,03 V, et la PTC ne déclenche pas pendant 1 h à consommation maximale.
- [ ] 2.4 Souder en montage volant, sur fils courts, une TVS de 16 V (SMF16A ou SMAJ16A) entre +Vin et le 0 V, sur les pastilles 0603 de C40 (C42 sur le SC942C), en aval de la PTC. Ne jamais la poser sur +VBUS. Vérification :
  - test en mode diode conforme ;
  - sur un échantillon de banc, avec la TVS remplacée par un court-circuit, la PTC déclenche, les autres BA retrouvent 14 V ou plus dans les 5 s et les gardent, et ils acquittent de nouveau leurs commandes moins de 10 s après la déconnexion du port de ce BA (procédure de `NR-board-sc944d-9`).
- [ ] 2.5 Remplacer C2 par un 10 µF / 35 V X5R et D6 par une Schottky 40 V en SOD-323F. Vérification : sous 15,0 V, le rail « +12V » est entre 14,2 V diminués de la chute mesurée dans la PTC et 14,4 V, et le +5 V entre 4,83 et 5,03 V.
- [ ] 2.6 Mesurer le temps de montée de 0 à 3,45 V au BA le plus éloigné, avec les pull-ups d'origine de 1 kΩ. Appliquer la table D2 du design :
  - jusqu'à 2,3 µs : 2,2 kΩ ;
  - de 2,3 à 2,97 µs : 1,5 kΩ, avec 47 Ω côté BA et côté BP ;
  - au-delà : pas d'option renforcée.

  Livrable : la mesure et la décision, consignées.
- [ ] 2.7 Si 2.6 l'autorise, poser l'écrêteur secondaire côté BA : une BAT54S sur les empreintes C23/C25 (C24/C26 sur le SC941C, C27/C29 sur le SC942C), avec la cathode sur le +5 V. R1/R2 restent à 47 Ω ; leur passage éventuel à 100 Ω relève de la tâche 3.8. Vérification :
  - test en mode diode de la BAT54S conforme ;
  - niveaux de `NR-board-sc944d-1` au vert ;
  - sur un échantillon de banc instrumenté (shunt entre la BAT54S et la broche), courant de broche d'au plus 150 mA à +12 V et d'au plus 50 mA à −2,5 V. Aucune injection n'est faite sur une carte de l'installation.
- [ ] 2.8 Pour les BA d'apparence saine, mesurer la fuite de D1/D2 au repos, câble de bus débranché et BA alimenté seul sous 15,0 V par J1-4/J1-3, et planifier leur retouche à la prochaine dépose. D1/D2 ne sont remplacées que si l'écart entre le +5 V local et la ligne dépasse 100 mV, soit environ 10 µA sous la pull-up de 10 kΩ. Livrable : le plan de retouche, consigné sur l'issue #1 après accord.
- [ ] 2.9 Publier la variante de BOM « retouche foudre » et une note de schéma dans les dépôts essensys-board-SC940, -SC941C et -SC942C, par une PR qui contient `Feature: i2c-surge-protection-2026-10-007` et `Refs essensys-hub/essensys-board-SC944D#1`. La note liste les exceptions de marge assumées (U1). Vérification : la PR est mergée, et la BOM liste R1/R2 à 47 Ω (100 Ω en option), la PTC, la TVS, C2 en 35 V et D6 en 40 V, sans modifier la BOM d'origine.
- [ ] 2.10 Étiqueter chaque BA retouché : « retouche foudre », valeur de la résistance série et option renforcée éventuelle. Vérification : l'étiquette est contrôlée visuellement à la remise en place.
- [ ] 2.11 Relever sur les fichiers de fabrication de la révision D des SC940D, SC941C, SC942C et SC944D les distances entre conducteurs 230 V et TBT, notamment entre J1/J2 et K1 ou VR1. Vérification : les distances sont consignées ; toute distance inférieure à 6 mm reçoit une mesure compensatoire documentée (barrière isolante, vernis) ou une restriction d'usage reprise en 8.4, avant la remise en service d'une carte retouchée de ce type.

## 3. Retouches du boîtier principal (SC944D) (#4)
- [ ] 3.1 Sauvegarder la flash du BP par BDM avant toute retouche, puis la comparer au `.S19` 099-37 de `SC944D/Prog/099-37/`. Vérification : la sauvegarde est archivée, et l'identité ou l'écart est documenté.
- [ ] 3.2 Remplacer D33/D34 par des SMF5V0A-E3-08. Remplacer aussi U19 si son test de 1.8 a échoué ou si D33/D34 ont claqué. Vérification : J31 débranché, écart entre +5VS et chaque ligne d'au plus 50 mV (fuite ≤ 50 µA sous la pull-up de 1 kΩ), EN de U19 entre 3,6 et 4,4 V, et test fonctionnel réussi dans les deux sens.
- [ ] 3.3 Protéger le +15 V de J31 de l'une des deux façons suivantes :
  - (a) remplacer L31 (empreinte 1812) par une PTC CMS 1812 de 30 V au moins, dont le courant de maintien à 50 °C vaut au moins 1,5 fois BA_ALIM mesuré relais collés, et souder une SMAJ16A entre J31-4 et J31-3, côté soudure ;
  - (b) si aucune PTC 1812 de 30 V n'atteint ce courant de maintien, installer le module DIN (PTC radiale, puis TVS) en aval de J31.

  Vérification :
  - BA_ALIM vaut 14 V ou plus au dernier BA ;
  - sur un échantillon de banc, un court-circuit entre J31-4 et J31-3 pendant 60 s ne cause aucune dégradation, y compris sans limiteur sur le BP pour la variante (b) ;
  - après le court-circuit, BA_ALIM remonte en 120 s au plus (procédure de `NR-board-sc944d-9`).
- [ ] 3.4 Sur un échantillon de banc retouché selon 3.3 (a), mesurer la tension de +15VS pendant que la SMAJ16A de J31-4 conduit son courant d'essai : la diode interne de T32 y expose U2 (LMR12010) et C110 (25 V). Vérification : la mesure est consignée ; au-delà de 24 V, une protection de +15VS est ajoutée à la retouche, ou U2 et C110 sont listés dans les exceptions assumées jusqu'à la révision E.
- [ ] 3.5 Remplacer U16 (LD1117) par un LM317 en SOT-223 (R66 = 115 Ω), ou par un régulateur dont l'entrée tient au moins 40 V. Vérification : +12VSP vaut 12,1 V à ±5 % près, sous +15VSP nominal.
- [ ] 3.6 Mesurer au PIC du BA le plus éloigné le niveau bas émis par le BP, pour vérifier la sortance du port du MCF. Vérification : moins de 1,2 V. Sinon, l'option renforcée côté BP est exclue, les BA restent à 47 Ω, et la cause est instruite sur l'issue #1 après accord.
- [ ] 3.7 Si 2.6 et 3.6 l'autorisent, appliquer l'option renforcée côté BP :
  - R155/R156 passent à 2,2 kΩ, ou à 1,5 kΩ selon 2.6 ;
  - L30/L32 sont remplacées par des résistances 0603 anti-impulsion de 47 Ω ;
  - une BAT54S est posée sur C79/C80, avec la cathode sur +5VS.

  Vérification :
  - hors tension, 44 à 50 Ω entre chaque ligne de J31 et U19 ;
  - sur un échantillon de banc, lors d'une injection à +12 V, la tension côté bus de U19 reste inférieure ou égale à 7,0 V et la broche 29 du MCF inférieure ou égale à 4,0 V ; à −2,5 V, U19 reste à −0,5 V ou plus, ou son courant d'entrée à 50 mA au plus, et la tension de la broche 28 est consignée ;
  - sur l'installation, le temps de montée reste inférieur à 4 µs et les niveaux de `NR-board-sc944d-1` sont au vert.
- [ ] 3.8 Faire passer R1/R2 des BA équipés de la BAT54S (2.7) de 47 Ω à 100 Ω seulement si R155/R156 sont à 2,2 kΩ (3.7), si 3.6 est vert et si ΔVg mesuré est d'au plus 0,1 V. Sinon, les laisser à 47 Ω. Vérification :
  - hors tension, 95 à 105 Ω entre J1-1 et RC7, puis entre J1-2 et RC6 ;
  - niveau bas au MCF inférieur à 0,85 V et au PIC inférieur à 1,2 V (`NR-board-sc944d-1`) ; sinon, retour à 47 Ω ;
  - sur un échantillon de banc instrumenté, courant de broche d'au plus 20 mA à +12 V et à −2,5 V.
- [ ] 3.9 Publier dans ce dépôt la variante de BOM « retouche foudre » du SC944D et une note de schéma (risque résiduel sur U19 sans option, exceptions de marge assumées), par une PR qui contient `Feature: i2c-surge-protection-2026-10-007` et `Refs #1`. Vérification : la PR est mergée, la variante est ajoutée sans modifier la BOM d'origine, et `/checkup essensys-board-SC944D` est vert.
- [ ] 3.10 Étiqueter le BP retouché : « retouche foudre » et option éventuelle, ou « sans option ». Vérification : l'étiquette est contrôlée visuellement à la remise en place.

## 4. Étoile 0 V et isolation de l'Ethernet du BP (#5)
- [ ] 4.1 Concevoir la carte passive N2, montée sur support DIN. Elle comprend :
  - un plan 0 V ;
  - deux embases par BA (J1 et J2) et une embase pour J31 ;
  - un point de raccordement pour une queue de cochon AWG20 soudée sur la broche traversante J31-3 du BP ;
  - par port, une PTC suivie d'une TVS sur le +15 V ;
  - par port, un cavalier qui coupe SDA, SCL et +15 V en gardant le 0 V.

  Livrable : schéma, BOM et Gerber dans le dépôt dédié [essensys-i2c-etoile-0v](https://github.com/essensys-hub/essensys-i2c-etoile-0v), référencés par une PR qui contient `Feature: i2c-surge-protection-2026-10-007` et `Refs #1`, avec un ERC et un DRC sans erreur.
- [ ] 4.2 Fabriquer les faisceaux AWG20 de 15 cm au plus, sertis à l'outil, avec le repère « 0 V TBTS – NE PAS RELIER À LA TERRE » aux deux extrémités des liaisons 0 V. Vérification : moins de 60 mΩ par conducteur, contacts compris, et repérage contrôlé.
- [ ] 4.3 Mesurer sur banc le seuil d'équilibrage, en onde 8/20 puis sur un front de 1 Ipk/µs, entre le 0 V du BA en bout de chaîne et le 0 V du BP. La conduction est un courant de plus de 100 mA dans SDA ou SCL, à la sonde de courant au connecteur du BA en bout de chaîne. Faire d'abord la mesure sur la chaîne d'origine, qui sert de référence, puis avec N2. Vérification : avec N2, au moins 4 fois la référence dans les deux formes d'onde, et écart statique de 0 V d'au plus 0,05 V. Livrable : un rapport d'essai qui consigne les seuils, repris en 7.18.
- [ ] 4.4 Valider sur banc la sélectivité des PTC de N2. Vérification :
  - une sortie court-circuitée pendant 60 s laisse les autres sorties à 14 V ou plus dans les 5 s, et durablement, sans déclencher la protection du BP ;
  - les autres BA acquittent de nouveau leurs commandes moins de 10 s après l'ouverture du cavalier du port en court-circuit ;
  - la sortie remonte en 120 s au plus après le court-circuit ;
  - aucun déclenchement en 24 h à 50 °C à consommation maximale.
- [ ] 4.5 Préparer la variante N1 pour les coffrets où N2 n'est pas posable :
  - un renfort H07V-K de 2,5 à 4 mm², plaqué le long de chaque tronçon ;
  - une queue de cochon AWG20 soudée sur la broche traversante J1-3 ou J2-3, avec un arrêt de traction ;
  - les anodes de D1/D2 et de D33/D34 reliées au même point.

  Vérification sur banc : écart de 0 V d'au plus 0,05 V, et seuil d'équilibrage d'au moins 3 fois la référence en onde 8/20 et 2 fois sur le front de 1 Ipk/µs.
- [ ] 4.6 Choisir et qualifier l'isolement de l'Ethernet du BP : fibre, avec le convertisseur côté BP alimenté uniquement depuis l'îlot TBTS du BP, ou isolateur de 4 kV efficaces pendant 60 s au moins. Vérification sur banc :
  - plus de 100 MΩ sous 500 V continu entre les huit conducteurs côté bâtiment et le 0 V du BP ;
  - sous une onde de mode commun, le courant dans le 0 V du bus est inférieur au dixième de la référence sans isolement ;
  - le critère B est satisfait.
- [ ] 4.7 Poser N2 (ou N1) et l'isolement Ethernet sur l'installation touchée, sous consignation comme en 1.5. Vérification :
  - écart de 0 V d'au plus 0,05 V ;
  - niveau bas au MCF inférieur à 0,85 V et au PIC inférieur à 1,2 V ;
  - aucun parafoudre relié au PE sur le port Ethernet du BP ;
  - cordon Ethernet sorti directement du coffret ;
  - au moins 10 MΩ entre 0 V et PE sous 250 V continu.
- [ ] 4.8 Contrôler le câblage du coffret de l'installation touchée :
  - liaisons de bus de 0,5 m au plus ;
  - aucune goulotte mixte TBT et 230 V ;
  - manchons isolants là où le bus et les nappes longent des bornes de phase ;
  - aucune borne PE dans le coffret, et PE de passage gainé avec son circuit ;
  - câbles PV signalés à l'installateur s'ils passent à moins de 30 à 50 cm du coffret.

  Livrable : la check-list remplie dans le dossier privé, et une synthèse anonymisée sur l'issue #1, publiée après accord.
- [ ] 4.9 Après 4.7, vérifier le fonctionnement de l'installation. Vérification : chaque BA acquitte une commande ; pendant 24 h, au moins une commande par BA chaque heure est envoyée et consignée, et `(index 12 & 0x1C)` reste à 0.

## 5. Firmware : récupération de bus, resynchronisation, télémétrie, watchdog BA (#6)
- [ ] 5.1 Mesurer sur banc l'étirement d'horloge maximal d'un BA 1.7 (CKP = 0 en émission, latence de l'interruption) à l'analyseur logique. Réécrire ensuite dans essensys-gcc le pilote I2C MQX du BP, avec des attentes bornées en temps : 2 ms au plus par attente (deux fois l'étirement mesuré s'il dépasse 1 ms), 20 ms au plus par transaction. Le BP reste en 099-37 jusqu'à la tâche 5.12. Vérification : la mesure est consignée ; un test hôte `NR-gcc-<n>` sur bus simulé (ligne collée, retour en erreur en 20 ms au plus) passe dans la CI d'essensys-gcc, puis `NR-board-sc944d-3` passe sur banc ; la PR contient `Feature: i2c-surge-protection-2026-10-007` et `Refs essensys-hub/essensys-board-SC944D#1`.
- [ ] 5.2 Ajouter la récupération de bus :
  - PAS0/PAS1 passent en GPIO pour lire les niveaux ;
  - jusqu'à neuf coups d'horloge, puis STOP ;
  - remise à zéro de `I2CR`, puis rétablissement de `IEN` ;
  - une tentative par seconde au plus.

  Vérification : le test hôte passe, ainsi que `NR-board-sc944d-4`.
- [ ] 5.3 Corriger dans client-essensys-legacy le pointeur de fichier périmé après une réinitialisation (`ba_i2c.c:86`, `98`, `180`…) et la fuite mémoire quand `fclose` échoue. Vérification : 10 000 réinitialisations en test hôte sans fuite, puis un tas stable dans `NR-board-sc944d-12` ; la PR contient `Feature: i2c-surge-protection-2026-10-007` et `Refs essensys-hub/essensys-board-SC944D#1`.
- [ ] 5.4 Remplacer l'abandon des commandes « marquées acquittées » (`ba.c:202-209`, `313-316`) par une file par BA qui garde le dernier état demandé de chaque sortie. Un état maintenu est appliqué quel que soit son âge ; une commande de volet ou impulsionnelle expire après un délai configurable (5 min par défaut) et est signalée comme expirée, sans être marquée acquittée. Vérification : `NR-board-sc944d-5`.
- [ ] 5.5 Ajouter la resynchronisation complète (configuration, puis états des sorties) au démarrage du BP, à chaque retour d'un BA à l'état OK, et sur le drapeau de redémarrage de l'octet `comm_stat` d'un BA 1.8. Le BP lit ce sixième octet s'il est présent et fonctionne sans lui avec un BA 1.7. Vérification : `NR-board-sc944d-6` en moins de 10 s, et resynchronisation en moins de 10 s après un redémarrage par le chien de garde dans `NR-board-sc944d-8`.
- [ ] 5.6 Ajouter :
  - l'état de santé par BA (OK, SUSPECT, ABSENT) ;
  - l'interrogation de présence au moins toutes les 60 s ;
  - la mise à jour de l'index 12 sans attendre une commande, avec le codage inchangé ;
  - les compteurs sur la console Espion, dont les commandes expirées.

  Vérification : `NR-board-sc944d-7`, et `(index 12 & 0x1C)` vaut `0x1C` quand les trois BA sont absents, les bits 0 et 1 restant inchangés.
- [ ] 5.7 Ajouter le cycle de TE2 en dernier recours : déclenché au-delà de 60 s de blocage, au moins 1 h entre deux cycles, 3 cycles au plus par période glissante de 24 h avec un compteur conservé à travers un redémarrage du BP, suspension après un cycle qui ne rétablit pas le bus jusqu'au retour du bus ou à une remise à zéro manuelle, et aucun cycle quand J58 est déclaré ouvert. Chaque cycle est compté et daté. Vérification : le test hôte (horloge simulée) couvre chacune de ces règles ; sur banc, aucun quatrième cycle n'a lieu en 24 h, aucun nouveau cycle ne suit un cycle sans effet, et le compteur survit à un redémarrage du BP.
- [ ] 5.8 Vérifier que le protocole HTTP legacy IoT est inchangé : capturer les échanges d'un BP modifié et d'un BP 099-37 à état identique, sur le serveur legacy de banc, puis les comparer. Vérification : la différence est vide (`NR-board-sc944d-11`).
- [ ] 5.9 Écrire le firmware BA 1.8 :
  - chien de garde activé ;
  - état d'erreur SSP : SSPEN à 0 puis à 1, CKP à 1, purge de SSPOV et WCOL ;
  - timeout SSP : réinitialisation du module si SDA ou SCL reste tenue par le BA plus de 50 ms sans front sur SCL ;
  - NACK quand `CmdBuf` est occupé, en laissant SSPBUF non lu (BF = 1) puis en purgeant SSPOV ;
  - LED d'état ;
  - `comm_stat` dans un sixième octet de lecture rétrocompatible, avec un drapeau de redémarrage.

  Vérification : `NR-board-sc944d-8`, plus un BA 1.8 qui fonctionne avec un BP 099-37 (`NR-board-sc944d-11`) ; les PR dans les dépôts SC94x contiennent `Feature: i2c-surge-protection-2026-10-007` et `Refs essensys-hub/essensys-board-SC944D#1`.
- [ ] 5.10 Corriger le build BA d'essensys-gcc (une seule macro `TYPE_*` par cible) ainsi que `ba/source/hard.h`. Vérification : le build produit trois `.hex`, et chacun n'acquitte que 0x11, 0x12 ou 0x13 sur banc.
- [x] 5.11 Proposer, puis ouvrir après confirmation, un ticket dans essensys-server-backend pour une alerte quand `system.comm_faults` n'est pas vide (`internal/armoire/decode.go`), sans modifier le protocole. Vérification : le ticket est créé et lié à #1. Ticket : essensys-hub/essensys-server-backend#8.
- [ ] 5.12 Qualifier un candidat firmware BP sur banc avant tout flash sur un BP en service : toute la suite NR au vert et `/checkup` vert. Le déployer ensuite sur un premier BP, après sauvegarde BDM, comme opération de maintenance manuelle : aucun harnais ni script NR ne vise ce BP. Vérification :
  - pendant 72 h, au moins une commande par BA chaque heure est envoyée et consignée, et `(index 12 & 0x1C)` reste à 0 ;
  - le retour arrière vers le `.S19` 099-37 a été essayé sur banc.

## 6. Révision E et carte d'interconnexion équipotentielle (#7)
- [ ] 6.1 Rédiger les règles de conception communes de la révision E :
  - zones Z1 « bus », Z2 « TBTS locale » et Z3 « 230 V » ;
  - pour une carte à isolation intégrée seulement, au moins 8 mm sans cuivre entre Z1 et Z2 ; sur une carte non isolée, SDA, SCL et le 0 V passent de Z1 à Z2 par le chemin protégé ;
  - entre Z2 et Z3, au moins 6 mm avec fente, et au moins la distance d'isolement renforcé exigée par l'EN 60664-1 et la norme produit ;
  - ordre connecteur → TVS (à 10 mm au plus) → résistance → écrêteur → résistance → circuit intégré ;
  - retour des TVS vers GND_BUS, et courant de surtension tenu hors du plan logique.

  Livrable : les règles DRC dans les projets de CAO, relues.
- [ ] 6.2 Concevoir la carte d'interconnexion équipotentielle (fond de panier) :
  - PCB 2 couches en cuivre de 2 oz, avec plan 0 V et bus en étoile ;
  - raccordement au BP par J31 et par son plot 0 V (révision E), ou par une queue de cochon sur J31-3 (révision D) ;
  - par port : une TVS de 50 pF au plus sur SDA et SCL, sans résistance série ; une PTC de 30 V au moins et une SMCJ16A sur le +15 V ; un cavalier ;
  - traversées IDC 26 points pour les nappes J3, avec une TVS par entrée ;
  - support DIN.

  Vérification : ERC et DRC sans erreur, et revue de conception tracée sur une sub-issue.
- [ ] 6.3 Prototyper la carte d'interconnexion et la qualifier sur banc, avec des BA non modifiés puis avec des BA retouchés à 47 Ω. Vérification : au moins 4 fois la référence de 4.3 en onde 8/20 et sur un front de 1 Ipk/µs ; niveau bas au MCF inférieur à 0,85 V avec les BA retouchés ; `NR-board-sc944d-2`, `-9` et `-10` au vert.
- [ ] 6.4 Concevoir les BA en révision E (SC940D, SC941C, SC942C) :
  - plot 0 V vissable près de J1/J2 ;
  - J1/J2 à au moins 6 mm de K1 et des varistances, avec une fente, selon 6.1 ;
  - R1/R2 de 47 à 100 Ω, écrêteur secondaire suivi d'une résistance vers le PIC, et TVS de ligne de 50 pF au plus ;
  - LMR16006, C2 en 50 V, D6 en 40 V et FI1 de tenue adaptée ;
  - PTC et TVS sur +Vin.

  Vérification : les distances sont contrôlées sur les Gerber, la revue de BOM confirme les marges (tension assignée = maximum recommandé en fonctionnement), et le courant de broche reste inférieur ou égal à 20 mA sur un échantillon de banc instrumenté.
- [ ] 6.5 Concevoir le BP en révision E :
  - protection au pied du MCF ;
  - résistance série de 47 Ω et écrêteur secondaire entre J31 et U19 ;
  - TVS de ligne de 50 pF au plus à la place de D33/D34 ;
  - plot 0 V vissable près de J31 ;
  - J31 et le RJ45 sur des bords opposés, J32 éloigné de J31 et de J53 ;
  - magnétiques de 2,25 à 4 kV et capacité Bob Smith de 3 kV au moins, ou Ethernet isolé intégré ;
  - eFuse à démarrage progressif et SMCJ16A sur le +15 V ;
  - régulateur +12VSP et régulateur U2 dont l'entrée tient au moins 40 V ;
  - R155/R156 en 2,2 kΩ.

  Vérification : lors d'une injection à −2,5 V sur un échantillon de banc, les broches du MCF restent à −0,3 V ou plus ; hors tension, 44 à 50 Ω entre J31 et U19 ; `NR-board-sc944d-1` est au vert.
- [ ] 6.6 Qualifier sur banc l'ensemble fond de panier, BP et BA révision E raccordés par leur plot 0 V. Vérification : au moins 800 A en onde 8/20 et 250 A sur un front de 1 Ipk/µs ; aucune dégradation permanente sous 1 kA en onde 8/20 ni sous 2 kV par le générateur seul de 2 Ω. Livrable : un rapport d'essai.
- [ ] 6.7 Concevoir le MIB pour les BA déportés (hors coffret ou liaison de plus de 0,5 m), qualifié jusqu'à 3 m :
  - prérequis sur le bus : D33/D34 du BP remplacées par des TVS de 50 pF au plus (retouche dédiée ou révision E), et aucun BA non isolé du même bus avec ses TVS d'origine ;
  - ISO1640DW et convertisseur isolé renforcé 9-36 V → 12 V ;
  - VCC1 pris sur le +5 V du BA ;
  - superviseur : coupure de VCC1 au-delà de 100 ms de ligne basse côté BA, neuf impulsions d'horloge côté BA, puis réactivation à intervalle croissant plafonné à 5 s ;
  - connecteur d'une autre série et étiquette sur J2.

  Vérification sur banc :
  - capacité côté bus, BP compris, de 400 pF au plus ;
  - niveaux tenus à 1 m et à 3 m, et temps de montée de 30 % à 70 % d'au plus 1 µs ;
  - un BA hors tension laisse le bus libre ;
  - `NR-board-sc944d-13` (libération en 150 ms au plus, esclave figé libéré, reprise en moins de 10 s) ;
  - relais enclenchés à 11 V et à chaud.
- [ ] 6.8 Rédiger l'ADR de la génération 3 en RS-485 isolé. Il compare le CAN isolé et l'I2C isolé, et traite deux contraintes : le signal DE est absent sur le SC941C, et la console Espion doit être déplacée sur telnet. Vérification : l'ADR est relu et mergé.

## 7. Plan d'essais et non-régression sur banc (#8)
- [ ] 7.1 Monter le banc d'essai :
  - un BP et trois BA, un de chaque type, plus des échantillons dédiés aux injections, jamais destinés à une installation ;
  - des câbles de 3 × 33 cm en cat. 5e AWG26, de 1 m et de 3 m ;
  - des charges factices, sans aucune tension secteur sur les sorties ;
  - une alimentation de laboratoire flottante de 22 à 24 V ;
  - un réseau isolé, sans accès au serveur de production, avec un serveur legacy local qui capture le trafic HTTP.

  Livrable : la fiche du banc. Vérification : chaque BA acquitte une commande sur le banc, et le serveur de banc capture les échanges du BP.
- [x] 7.2 Écrire le harnais pytest dans `tests/bench/` de ce dépôt : pilotage des instruments, marqueur `@pytest.mark.nr("NR-board-sc944d-<n>", "#1")` et JUnit XML. Sans banc déclaré, les tests de banc sont marqués « skipped » ; avec un banc déclaré mais non reconnu, le harnais s'arrête en erreur. Vérification :
  - le test de garde passe : sans banc déclaré, les tests sont « skipped » et aucun stimulus n'est appliqué ; avec un banc inconnu, le harnais sort en erreur sans stimulus ;
  - `nonreg_report.py --sources tests/bench` consolide le JUnit XML avec le code 0 et reconnaît les identifiants `NR-board-sc944d-1` à `-13`.
- [ ] 7.3 Écrire `NR-board-sc944d-1`, sur les niveaux I2C à 50 kHz avec 33 cm, 1 m et 3 m de câble, à consommation maximale. Vérification :
  - niveau bas au MCF inférieur à 0,85 V ;
  - niveau bas au PIC inférieur à 1,2 V ;
  - temps de montée inférieur à 4 µs ;
  - fuite de chaque ligne, carte seule, d'au plus 50 µA.
- [ ] 7.4 Écrire `NR-board-sc944d-2` : un BA est retiré par son câble, puis par son cavalier. Vérification : les autres BA reprennent en moins de 10 s, et en moins de 120 s `(index 12 & 0x1C)` ne contient que le bit du BA retiré.
- [ ] 7.5 Écrire `NR-board-sc944d-3` : SDA puis SCL sont forcées à l'état bas pendant 60 s. Vérification : chaque transaction se termine en 20 ms au plus, le BP reste joignable par le serveur de banc, la téléinformation est lue et transmise, et les scénarios locaux continuent ; un BA 1.7 qui étire l'horloge au maximum ne provoque aucune erreur d'attente.
- [ ] 7.6 Écrire `NR-board-sc944d-4` : un esclave est bloqué au milieu d'un octet, puis une ligne est forcée à l'état bas pendant 500 ms, puis le défaut devient permanent. Vérification : reprise en moins de 10 s, et une tentative de récupération par seconde au plus.
- [ ] 7.7 Écrire `NR-board-sc944d-5` : une commande d'éclairage est émise pendant les 5 min d'absence d'un BA, puis la séquence marche, arrêt, marche, puis une commande de volet reste en attente au-delà du délai configuré. Vérification : la commande n'est pas acquittée à tort, seul l'état final est appliqué moins de 10 s après le retour du BA, et la commande de volet expirée n'est pas appliquée et est signalée.
- [ ] 7.8 Écrire `NR-board-sc944d-6` : la configuration est modifiée pendant l'absence d'un BA, puis le BP est redémarré, puis un BA revient avec une EEPROM vierge. Vérification : la configuration complète est acquittée en moins de 10 s.
- [ ] 7.9 Écrire `NR-board-sc944d-7` : un BA est déconnecté puis reconnecté, sans aucune commande. Vérification : le bit de l'index 12 est positionné, puis effacé, en moins de 120 s, et `(index 12 & 0x1C)` vaut `0x1C` quand les trois BA sont absents.
- [ ] 7.10 Écrire `NR-board-sc944d-8` : un PIC est figé avec SCL retenue, puis le module SSP est figé avec la boucle principale vivante, puis un débordement SSP est provoqué, puis le tampon de commande est occupé. Vérification : SCL est relâchée en moins de 2 s, SDA est relâchée moins de 100 ms après le dernier front de SCL, sans cycle d'alimentation ; le BA redémarré par le chien de garde est resynchronisé en moins de 10 s ; la commande refusée par un NACK est retransmise puis appliquée.
- [ ] 7.11 Écrire `NR-board-sc944d-9` : la sortie +15 V d'un port est court-circuitée pendant 60 s, puis J31-4 est court-circuité. Vérification : les autres ports retrouvent 14 V ou plus dans les 5 s et les gardent, les autres BA reprennent moins de 10 s après l'ouverture du cavalier du port en court-circuit, le retour se fait en 120 s au plus, et aucune dégradation n'apparaît.
- [ ] 7.12 Écrire `NR-board-sc944d-10` : un BA est branché puis débranché pendant un trafic soutenu. Vérification : aucune commande destinée aux autres BA n'est perdue ; chaque trame perturbée (erreur de CRC ou NACK) est retransmise puis acquittée.
- [ ] 7.13 Écrire `NR-board-sc944d-11` sur la compatibilité croisée (BP 099-37 ou modifié, avec BA 1.7 ou 1.8) et sur les échanges HTTP. Vérification : les quatre combinaisons fonctionnent, et la capture HTTP du serveur de banc ne montre aucune différence.
- [ ] 7.14 Écrire `NR-board-sc944d-12` : endurance de 72 h. Vérification : 0 NACK d'adresse non expliqué, 0 commande perdue, 0 erreur de CRC et un tas du BP stable.
- [ ] 7.15 Écrire `NR-board-sc944d-13` : une ligne est forcée à l'état bas côté BA d'un MIB, puis le PIC est figé au milieu d'un octet. Vérification : le bus est libéré en 150 ms au plus, chaque essai de raccordement le bloque moins de 150 ms, les autres BA continuent, et le BA reprend en moins de 10 s une fois le défaut supprimé.
- [ ] 7.16 Injecter sur banc, sur des échantillons, des ondes 8/20 portant SDA à +12 V puis à −2,5 V : sur un BA retouché à 47 Ω sans écrêteur secondaire, puis sur un BP en option renforcée. Vérification :
  - BA : courant de broche, déduit de la tension aux bornes de R1, d'au plus 150 mA et 50 mA ; adresse acquittée dans les 10 s ; courant d'alimentation revenu à ±10 % en 1 s, sans verrouillage ;
  - BP : U19 entre −0,5 V (ou 50 mA au plus) et 7,0 V, broche 29 du MCF à 4,0 V au plus, tension de la broche 28 consignée, échanges repris dans les 10 s.
- [ ] 7.17 Pré-qualifier l'immunité avec des générateurs loués et un analyseur à sondes isolées. Niveaux :
  - IEC 61000-4-2 : ±8 kV au contact, ±15 kV dans l'air ;
  - IEC 61000-4-4 : ±1 kV par pince sur le bus, ±2 kV sur l'alimentation ;
  - IEC 61000-4-5 : 2 kV en mode commun entre le 0 V du BP et le 0 V du BA en bout de chaîne par un réseau de couplage de 40 Ω + 2 Ω, sur les cartes retouchées avec N2 puis sur la révision E ; 0,5 kV ligne-ligne sur le bus et sur le +15 V par le même réseau.

  Vérification : le critère B est satisfait ; sur un BA retouché, toute défaillance sous 0,5 kV ligne-ligne reste confinée à ce BA et les autres reprennent après la déconnexion de son port. Livrable : un rapport d'essai.
- [ ] 7.18 Essayer sur banc le courant d'équilibrage et l'onde de mode commun sur l'Ethernet. Injecter 0,8 fois le seuil mesuré en 4.3 pour la configuration essayée (BA retouchés avec N2, et avec N1 si elle est utilisée), en onde 8/20 ; puis appliquer l'onde de mode commun sur l'Ethernet, avec puis sans isolement. Vérification :
  - courant dans SDA et SCL inférieur à 100 mA ;
  - rail « +12V » de chaque BA sous 20 V ;
  - courant dans le 0 V du bus réduit d'au moins 10 fois par l'isolement.
- [ ] 7.19 Pousser deux ou trois échantillons de chaque configuration jusqu'à la défaillance. Vérification : en révision E avec carte d'interconnexion, le premier composant défaillant est un élément de protection ; sur les cartes retouchées, la défaillance reste confinée à une seule carte et se détecte au repos ; dans les deux cas, les autres BA communiquent après déconnexion du port défaillant.
- [ ] 7.20 Lancer `/checkup` sur la feature `i2c-surge-protection-2026-10-007` : build, lint, NR et gates `feature-gate` et `security-gate`, avec les JUnit XML du banc joints. Vérification : tout est vert, et le rapport est posté sur l'issue #1 après accord.

## 8. Documentation (retour d'expérience, essensys-memory, écarts de la doc matériel) (#9)
- [ ] 8.1 Publier dans essensys-doc la page de retour d'expérience, partie synthèse exécutive (une page) :
  - « une installation touchée le 9 août 2026 » ;
  - les symptômes ;
  - les défauts de conception révélés ;
  - les mesures prises ;
  - les consignes pour le parc.

  Vérification : la PR contient `Feature: i2c-surge-protection-2026-10-007` et `Refs essensys-hub/essensys-board-SC944D#1` ; elle est relue, et un contrôle par recherche et relecture confirme l'absence de toute donnée personnelle (adresse, IP, marque ou modèle des équipements du site, installateur, numéro de série).
- [ ] 8.2 Publier dans essensys-doc la partie analyse experte du retour d'expérience :
  - îlots TBTS et bus qui sert de pont ;
  - modèle R + (L − M) ;
  - seuils d'équilibrage, avec la correction du modèle de N2 ;
  - mécanisme du +15 V ;
  - choix de 47 Ω, de la PTC avant la TVS et de N2 ou N1 ;
  - Ethernet isolé ;
  - rejet du GDT et du parafoudre RJ45 relié au PE ;
  - plan d'essais.

  Vérification : `mkdocs build` passe sans erreur, la PR porte les mêmes lignes `Feature:` et `Refs` qu'en 8.1, et la page est relue avec le même contrôle de données personnelles qu'en 8.1.
- [ ] 8.3 Publier dans essensys-doc la procédure de diagnostic et de remise en service après une surtension :
  - sécurité E-1 (mesurage par une personne habilitée), observation E0, débranchements E1/E2 ;
  - mesures du BP et des BA à l'établi, dont la fuite carte par carte et l'inspection à la loupe ;
  - critères de réception ;
  - relance des BA par une coupure secteur de 13 s au moins, avec ses prérequis : J58 fermé, batterie du BP présente, `Mode_Test` à 0 ;
  - règles **BREAKING** : un BA non alimenté ne reste pas raccordé au bus, et un BA préparé pour un MIB n'est jamais raccordé directement ;
  - interdiction de flasher un build non qualifié.

  Vérification : la procédure est relue par un mainteneur, et la PR porte les lignes `Feature:` et `Refs`.
- [ ] 8.4 Compléter le guide d'installation dans essensys-doc :
  - parafoudre de tête et règle des 50 cm ;
  - valeur de terre ;
  - type et longueur du câble de bus ;
  - câbles PV à 30-50 cm au moins du coffret ;
  - équipements reliés au PE jamais en cuivre sur le commutateur du BP ;
  - TBT et 230 V jamais dans une même goulotte ;
  - aucune borne ni aucun raccordement PE dans le coffret, PE de passage gainé avec son circuit, et repérage du 0 V ;
  - recommandation de parafoudre pour les départs 230 V extérieurs, jamais de varistance ajoutée dans le coffret ;
  - MIB pour un BA déporté (hors coffret ou liaison de plus de 0,5 m) ;
  - règles **BREAKING** de 8.3 et restrictions d'usage issues de 2.11.

  Vérification : les règles PE et parafoudre sont validées par un électricien ; le guide est relu, chaque règle renvoie à l'exigence correspondante d'`aux-box-interconnect`, et la PR porte les lignes `Feature:` et `Refs`.
- [x] 8.5 Consigner dans essensys-memory une note anonymisée : décisions, valeurs de référence sans aucune identification du site, liens vers ce change et vers le retour d'expérience. Vérification : même contrôle de données personnelles qu'en 8.1, et la PR porte les lignes `Feature:` et `Refs`.
- [ ] 8.6 Corriger `essensys-doc/archi/hardware-sc944d.md` et `hardware-overview.md` : la liaison BA passe par J31, et non par J3 ou la « feuille 17 ». Vérification : la PR est relue, porte les lignes `Feature:` et `Refs`, et les affirmations sont recoupées avec le schéma du SC944D.
- [ ] 8.7 Corriger `essensys-doc/archi/hardware-sc94{0d,1c,2c}.md` :
  - J3 porte les entrées TOR, et le bus passe par J1/J2 ;
  - la logique est en 5 V, et non en 3,3 V ;
  - L3 n'est pas montée ;
  - les quantités de TVS et de VDR du SC940D sont fausses.

  Vérification : chaque correction est recoupée avec les schémas et les BOM DM, et la PR porte les lignes `Feature:` et `Refs`.
- [x] 8.8 Corriger `docs/i2c.md` de ce dépôt : le bus tourne à 50 kHz, et non 100 kHz ; le code ACTIONS vaut 5 ; les adresses vont de 0x11 à 0x13. Ajouter une section sur les protections, la récupération et la resynchronisation. Vérification : `mkdocs build` passe sans erreur, et la PR contient `Feature: i2c-surge-protection-2026-10-007` et `Refs #1`.
- [ ] 8.9 Corriger `essensys-board-SC940/docs/Architecture_Overview.md` : le SC943 est un variateur à IGBT, et non un module RF. Vérification : la PR porte les lignes `Feature:` et `Refs`, est relue et mergée.
- [x] 8.10 Mettre à jour `docs/hardware.md` et `docs/firmware.md` de ce dépôt avec la variante « retouche foudre » et la consigne de ne jamais reflasher le BP sans pilote I2C reconstitué. Vérification : `mkdocs build` passe sans erreur, et la PR contient `Feature: i2c-surge-protection-2026-10-007` et `Refs #1`.
