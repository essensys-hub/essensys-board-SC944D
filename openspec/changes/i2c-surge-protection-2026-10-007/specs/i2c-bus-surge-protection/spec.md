## Purpose

Cette capacité fixe la tenue aux surtensions et aux courants d'équilibrage entre îlots TBTS des interfaces du bus I2C qui relie le boîtier principal SC944D (BP) aux boîtiers auxiliaires SC940D, SC941C et SC942C (BA). Elle couvre :

- le courant injecté dans les circuits intégrés ;
- l'écrêtage coordonné des lignes ;
- la protection du +15 V et les marges des composants d'alimentation ;
- les niveaux I2C à 50 kHz ;
- la compatibilité TBTS ;
- les niveaux d'immunité visés sur banc et les critères B et C ;
- la défaillance maîtrisée.

## ADDED Requirements

### Requirement: Courant injecté dans les broches I2C borné

Chaque ligne SDA et SCL d'un BA retouché ou neuf SHALL comporter, entre la broche du connecteur de bus et la broche du PIC16F946, un élément série de 47 Ω au moins. Aucun chemin d'impédance nulle ne relie ainsi l'écrêteur de ligne au circuit intégré. Un BA d'origine (0 Ω) reste admis sur le bus jusqu'à sa prochaine dépose, où il SHALL être retouché.

Sur le BP, l'élément série de 47 Ω entre la ligne de bus et le traducteur de niveau U19 SHALL être présent en option renforcée et en révision E. Un BP retouché sans option renforcée garde ses perles L30/L32 : le risque résiduel sur U19, écrêté de 9 à 14 V par D33/D34, SHALL être consigné sur la fiche de la carte jusqu'à son passage en révision E.

Le courant injecté dans une broche SDA ou SCL du PIC16F946 SHALL rester dans les limites suivantes :

| Carte | Ligne portée à +12 V au connecteur | Ligne portée à −2,5 V au connecteur |
|---|---|---|
| BA retouché à 47 Ω, avec ou sans écrêteur secondaire | ≤ 150 mA | ≤ 50 mA |
| BA en option renforcée à 100 Ω, ou BA révision E | ≤ 20 mA | ≤ 20 mA |

Sans écrêteur secondaire, le courant de broche SHALL être déduit de la tension aux bornes de l'élément série. Avec un écrêteur secondaire, il SHALL être mesuré sur un échantillon de banc instrumenté, avec un shunt inséré entre l'écrêteur secondaire et la broche, ou estimé à partir de V(broche) − VDD sur une courbe de diode caractérisée.

#### Scenario: Remise en service d'un BA déposé

- **WHEN** un BA est déposé pour réparation ou pour retouche
- **THEN** la résistance mesurée hors tension entre J1-1 (SDA) et la broche RC7 du PIC, puis entre J1-2 (SCL) et la broche RC6, SHALL être comprise entre 44 et 50 Ω, ou entre 95 et 105 Ω en option renforcée à 100 Ω, avant la remise en service

#### Scenario: Élément série d'un BP renforcé

- **WHEN** un BP en option renforcée ou en révision E est contrôlé hors tension
- **THEN** la résistance entre chaque ligne de bus de J31 et la broche côté bus correspondante de U19 SHALL être comprise entre 44 et 50 Ω

#### Scenario: BP retouché sans option renforcée

- **WHEN** un BP est remis en service après retouche sans l'option renforcée
- **THEN** sa fiche SHALL consigner le risque résiduel sur U19
- **AND** son étiquette SHALL indiquer l'absence d'option

#### Scenario: Impulsion positive sur un BA retouché

- **WHEN** une onde 8/20 injectée sur banc porte SDA à +12 V au connecteur d'un échantillon de BA retouché à 47 Ω sans écrêteur secondaire
- **THEN** le courant dans la broche RC7 du PIC, déduit de la tension aux bornes de R1, SHALL rester inférieur ou égal à 150 mA
- **AND** le BA SHALL acquitter son adresse dans les 10 s qui suivent l'impulsion

#### Scenario: Impulsion négative sur un BA retouché

- **WHEN** une onde 8/20 injectée sur banc porte SDA à −2,5 V au connecteur d'un échantillon de BA retouché à 47 Ω sans écrêteur secondaire
- **THEN** le courant dans la broche RC7 du PIC SHALL rester inférieur ou égal à 50 mA
- **AND** le courant d'alimentation du BA SHALL revenir à sa valeur de repos, à ±10 % près, dans la seconde qui suit l'impulsion, sans verrouillage du PIC

#### Scenario: BA à 100 Ω avec écrêteur secondaire

- **WHEN** les injections à +12 V puis à −2,5 V sont appliquées à un échantillon instrumenté de BA en option renforcée à 100 Ω ou en révision E
- **THEN** le courant dans la broche du PIC SHALL rester inférieur ou égal à 20 mA dans les deux polarités

### Requirement: Écrêtage coordonné des lignes SDA et SCL

Chaque ligne SDA et SCL SHALL être protégée dans cet ordre, depuis le connecteur :

1. un écrêteur primaire vers le 0 V local ;
2. l'élément série ;
3. l'écrêteur secondaire, s'il existe ;
4. le circuit intégré.

Le courant de fuite total des écrêteurs SHALL rester inférieur ou égal à 50 µA par ligne et par carte, sous 5 V, jusqu'à 50 °C. Il SHALL être mesuré sur la carte seule, déconnectée des lignes du bus et alimentée par son connecteur : la ligne n'est alors tirée que par la pull-up de la carte, et la fuite vaut l'écart entre l'alimentation de cette pull-up et la ligne, divisé par sa valeur.

Pendant une injection sur un BP en option renforcée ou en révision E :

- la tension côté bus de U19 SHALL rester inférieure ou égale à +7,0 V ;
- elle SHALL rester supérieure ou égale à −0,5 V, sauf si le courant d'entrée de U19 reste inférieur ou égal à 50 mA en valeur absolue, conformément au maximum absolu du PCA9306 ;
- la tension des broches I2C du MCF52259 SHALL rester inférieure ou égale à 4,0 V.

Sur un BP en révision E, la tension des broches I2C du MCF52259 SHALL en outre rester supérieure ou égale à −0,3 V. Sur un BP en option renforcée, l'injection négative résiduelle d'environ −0,5 V sur les broches 28/29 du MCF52259 SHALL être consignée comme écart admis jusqu'à la révision E.

#### Scenario: Impulsion positive sur un BP renforcé

- **WHEN** une onde 8/20 injectée sur banc porte BA_SDA à +12 V sur J31 d'un échantillon de BP en option renforcée
- **THEN** la tension côté bus de U19 SHALL rester inférieure ou égale à 7,0 V
- **AND** la tension de la broche 29 du MCF52259 SHALL rester inférieure ou égale à 4,0 V
- **AND** le BP SHALL de nouveau échanger avec chaque BA dans les 10 s

#### Scenario: Impulsion négative sur un BP renforcé

- **WHEN** une onde 8/20 injectée sur banc porte BA_SCL à −2,5 V sur J31 d'un échantillon de BP en option renforcée
- **THEN** la tension côté bus de U19 SHALL rester supérieure ou égale à −0,5 V, ou son courant d'entrée SHALL rester inférieur ou égal à 50 mA en valeur absolue
- **AND** la tension mesurée sur la broche 28 du MCF52259 SHALL être consignée
- **AND** le BP SHALL de nouveau échanger avec chaque BA dans les 10 s

#### Scenario: Impulsion négative sur un BP révision E

- **WHEN** une onde 8/20 injectée sur banc porte BA_SCL à −2,5 V sur J31 d'un BP en révision E
- **THEN** la tension de la broche 28 du MCF52259 SHALL rester supérieure ou égale à −0,3 V

#### Scenario: Fuite après remplacement d'un écrêteur

- **WHEN** un écrêteur de ligne est remplacé
- **THEN** la fuite de chaque ligne, mesurée sur la carte seule, SHALL être inférieure ou égale à 50 µA, soit un écart d'au plus 0,5 V sous la pull-up de 10 kΩ d'un BA, ou d'au plus 50 mV sous la pull-up de 1 kΩ du BP

### Requirement: Protection de l'alimentation +15 V du bus

Côté BP, le +15 V distribué au bus SHALL être protégé par un limiteur de courant réarmable placé en amont d'un écrêteur de tension vers le 0 V. Ces deux éléments sont placés soit sur le BP, à la place de L31, soit dans un module placé en aval de J31. La variante en module n'est admise que si le court-circuit de J31-4 est tenu sans limiteur sur le BP.

Chaque BA SHALL disposer de son propre limiteur réarmable, suivi de son propre écrêteur. Ces deux éléments SHALL être placés sur le BA, en aval du nœud de chaînage J1 ∥ J2, ou sur le port dédié de ce BA dans l'interconnexion du coffret. Ils MUST NOT être placés sur un nœud partagé par plusieurs BA.

En cas de court-circuit :

- un court-circuit franc du +15 V d'un BA SHALL ne priver d'alimentation que ce BA : après le déclenchement de son limiteur, les autres BA SHALL retrouver 14 V ou plus dans les 5 s et les garder ;
- un BA privé d'alimentation reste relié à SDA et SCL et peut bloquer le bus : les autres BA SHALL de nouveau acquitter leurs commandes en moins de 10 s après la déconnexion de son port ;
- un court-circuit sur J31-4 MUST NOT dégrader le BP de façon permanente.

#### Scenario: Court-circuit du +15 V à la sortie du BP

- **WHEN** J31-4 est court-circuité à J31-3 pendant 60 s sur un échantillon de banc
- **THEN** le BP MUST NOT subir de dégradation permanente
- **AND** après la suppression du court-circuit, BA_ALIM SHALL remonter à 14 V ou plus au dernier BA en 120 s au plus, sans intervention
- **AND** chaque BA SHALL de nouveau acquitter ses commandes dans les 10 s qui suivent

#### Scenario: Écrêteur d'un BA en court-circuit

- **WHEN** l'écrêteur +15 V d'un BA est remplacé par un court-circuit sur banc
- **THEN** BA_ALIM SHALL revenir à 14 V ou plus à l'entrée des autres BA dans les 5 s qui suivent le court-circuit, et y rester
- **AND** après la déconnexion du port de ce BA, les autres BA SHALL acquitter de nouveau leurs commandes en moins de 10 s

#### Scenario: Surtension ligne-ligne sur l'alimentation du bus

- **WHEN** un essai IEC 61000-4-5 de 0,5 kV ligne-ligne est appliqué sur banc à la paire +15 V / 0 V d'un BA, par le réseau de couplage des lignes d'interconnexion (40 Ω + 2 Ω)
- **THEN** un BA en révision E SHALL satisfaire le critère B, sans dégradation permanente
- **AND** sur un BA retouché, toute défaillance SHALL rester confinée à ce BA, et les autres BA SHALL acquitter leurs commandes au plus tard 10 s après la déconnexion de son port

### Requirement: Marges de tension des composants d'alimentation

Dans cette exigence, la tension assignée d'un composant est sa tension maximale recommandée en fonctionnement, et non son maximum absolu. Les éléments de protection (écrêteurs, limiteurs réarmables) et les bobines de relais sont exclus de la règle de 1,5 : un écrêteur d'alimentation SHALL avoir une tension de veille (VRWM) au moins égale à la tension nominale maximale de son nœud, et une bobine de relais SHALL fonctionner sous sa tension maximale admissible de bobine.

En régime nominal, chaque composant du BP ou d'un BA SHALL fonctionner sous sa tension assignée. Tout autre composant relié au +15 V du bus, au +15VS ou au +15VSP du BP, ou au rail « +12V » d'un BA SHALL avoir une tension assignée au moins égale à 1,5 fois la tension nominale qu'il supporte. Les exceptions suivantes sont assumées jusqu'à la révision E et SHALL être listées dans la note de retouche :

- le régulateur U1 (LMR12010, 20 V en fonctionnement) d'un BA retouché, sous 14,3 V ;
- le régulateur U2 (LMR12010) du BP, sous +15VS.

Sur le BP, U2 et C110 sont exposés à la tension d'écrêtage de J31-4 par la diode interne de T32. Leur tenue SHALL être vérifiée au courant d'essai de cet écrêteur.

Sur un BA retouché, le condensateur du rail « +12V » SHALL être assigné à 35 V au moins, et la diode de roue libre du régulateur à 40 V au moins.

Sur une carte révision E, tout composant en aval de l'écrêteur +15 V SHALL avoir une tension assignée au moins égale à la tension d'écrêtage maximale de cet écrêteur.

#### Scenario: Revue de BOM d'un BA retouché

- **WHEN** la BOM d'un BA retouché est revue
- **THEN** le condensateur du rail « +12V » SHALL être assigné à 35 V ou plus
- **AND** la diode de roue libre du régulateur SHALL être assignée à 40 V ou plus
- **AND** chaque composant hors règle SHALL figurer dans la liste des exceptions assumées

#### Scenario: Régulateur +12VSP du BP

- **WHEN** un BP retouché est alimenté avec +15VSP à sa valeur nominale de 15,2 V
- **THEN** le régulateur qui produit +12VSP SHALL avoir une tension d'entrée assignée d'au moins 22,8 V
- **AND** +12VSP SHALL mesurer 12,1 V à ±5 % près

#### Scenario: Exposition de +15VS du BP

- **WHEN** l'écrêteur de J31-4 d'un échantillon de BP retouché conduit son courant d'essai sur banc
- **THEN** la tension de +15VS SHALL être mesurée et consignée
- **AND** si elle dépasse 24 V, une protection de +15VS SHALL être ajoutée, ou U2 et C110 SHALL figurer dans les exceptions assumées jusqu'à la révision E

#### Scenario: Revue de conception en révision E

- **WHEN** le schéma d'une carte révision E est revu
- **THEN** chaque composant situé en aval de l'écrêteur +15 V SHALL avoir une tension assignée supérieure ou égale à la tension d'écrêtage maximale de cet écrêteur

### Requirement: Niveaux I2C conservés à 50 kHz

Toute retouche, toute nouvelle carte et toute nouvelle interconnexion SHALL conserver le bus à 50 kHz et les adresses 0x11, 0x12 et 0x13. Elles SHALL respecter les critères ci-dessous, mesurés sur la configuration BP et trois BA à consommation maximale (relais monostables collés), sauf la fuite, mesurée carte par carte :

| Critère | Seuil |
|---|---|
| Niveau bas lu au MCF quand un BA émet | < 0,85 V |
| Niveau bas lu au PIC quand le BP émet | < 1,2 V |
| Temps de montée de 0 à 3,45 V au BA le plus éloigné | < 4 µs |
| Fuite de chaque ligne, carte seule | ≤ 50 µA |

Le choix de la résistance série dépend de l'écart statique de 0 V (ΔVg), mesuré entre J31-3 et J1-3 du BA le plus éloigné. Au-delà de 0,12 V, la liaison équipotentielle du coffret SHALL être posée avant la remise en service d'un BA retouché. La résistance série d'un BA SHALL passer à 100 Ω seulement si les pull-ups du BP sont à 2,2 kΩ, si le niveau bas émis par le BP est vérifié au PIC, si ΔVg est d'au plus 0,1 V et si le niveau bas mesuré au MCF reste sous 0,85 V. Sinon, elle SHALL rester à 47 Ω.

#### Scenario: Réception après retouche

- **WHEN** une carte retouchée est remise sur le bus
- **THEN** les critères de niveau et de temps de montée SHALL être respectés
- **AND** la fuite de chacune de ses lignes, mesurée avant la remise sur le bus, SHALL être inférieure ou égale à 50 µA
- **AND** chaque BA SHALL acquitter une commande

#### Scenario: Écart de masse élevé

- **WHEN** l'écart statique de 0 V mesuré entre J31-3 et J1-3 du BA le plus éloigné dépasse 0,12 V sans liaison équipotentielle
- **THEN** un BA retouché MUST NOT être remis en service avant la pose de la liaison équipotentielle du coffret
- **AND** après la pose, l'écart SHALL être inférieur ou égal à 0,05 V

#### Scenario: Écart de masse intermédiaire

- **WHEN** l'écart statique de 0 V mesuré est compris entre 0,1 et 0,12 V
- **THEN** le niveau bas mesuré au MCF avec les BA retouchés à 47 Ω SHALL être inférieur à 0,85 V, faute de quoi la liaison équipotentielle SHALL être posée

#### Scenario: Passage à 100 Ω refusé

- **WHEN** les pull-ups du BP sont restées à 1 kΩ ou à 1,5 kΩ, ou que l'écart statique de 0 V dépasse 0,1 V
- **THEN** la résistance série des BA MUST NOT passer à 100 Ω

#### Scenario: Temps de montée insuffisant pour l'option renforcée

- **WHEN** le temps de montée mesuré avec les pull-ups d'origine de 1 kΩ dépasse 2,97 µs
- **THEN** la valeur des pull-ups du BP MUST NOT être augmentée

### Requirement: Compatibilité TBTS

Une protection, une retouche ou une interconnexion MUST NOT créer de liaison galvanique entre le 0 V des cartes et le conducteur de protection (PE). Un éclateur, une varistance ou un parafoudre MUST NOT être raccordé entre le 0 V et la terre.

La résistance d'isolement entre l'ensemble des bornes 230 V et l'ensemble des bornes TBT d'une carte est mesurée sous 500 V continu :

- elle SHALL être d'au moins 100 MΩ pour une carte neuve ou retouchée ;
- une carte en dessous de 10 MΩ MUST NOT être remise en service.

Dans un coffret, la résistance entre le 0 V TBTS et le PE, mesurée sous 250 V continu, SHALL être d'au moins 10 MΩ.

Les cartes révision E SHALL présenter, entre tout conducteur 230 V et tout conducteur TBT, au moins 6 mm avec une fente fraisée, et au moins les distances d'isolement renforcé exigées par l'EN 60664-1 et par la norme produit applicable.

Pour les cartes révision D en service, les distances entre conducteurs 230 V et TBT, notamment entre J1/J2 et K1 ou VR1, SHALL être relevées sur les fichiers de fabrication et consignées. Toute distance inférieure à 6 mm SHALL faire l'objet d'une mesure compensatoire documentée (barrière isolante, vernis) ou d'une restriction d'usage dans le guide d'installation.

#### Scenario: Essai d'isolement avant remise en service

- **WHEN** une carte est préparée pour sa remise en service après un événement de surtension
- **THEN** la résistance d'isolement entre ses bornes 230 V et ses bornes TBT SHALL être mesurée sous 500 V continu et consignée
- **AND** la zone qui sépare les bornes TBT des relais et des varistances SHALL être inspectée à la loupe, et toute trace d'amorçage consignée
- **AND** une carte mesurée sous 10 MΩ MUST NOT être remise en service

#### Scenario: Aucune liaison entre 0 V et PE après intervention

- **WHEN** une protection ou une interconnexion est ajoutée dans un coffret
- **THEN** la résistance entre le 0 V TBTS et le PE, mesurée sous 250 V continu, SHALL rester d'au moins 10 MΩ
- **AND** le 0 V MUST NOT être raccordé à la terre par un dispositif quelconque

#### Scenario: Distance d'isolement en révision E

- **WHEN** les fichiers de fabrication d'une carte révision E sont contrôlés
- **THEN** la distance entre tout conducteur 230 V et tout conducteur TBT SHALL être d'au moins 6 mm, avec une fente fraisée, et au moins égale à la distance d'isolement renforcé exigée

#### Scenario: Distances des cartes révision D

- **WHEN** les fichiers de fabrication d'une carte révision D montrent une distance 230 V / TBT inférieure à 6 mm
- **THEN** une mesure compensatoire ou une restriction d'usage SHALL être documentée avant la remise en service d'une carte retouchée de ce type

### Requirement: Niveaux d'immunité visés sur banc

Les configurations retenues SHALL être qualifiées sur un banc d'essai : d'abord les cartes retouchées avec l'interconnexion du coffret, puis les cartes révision E. Le banc comprend le BP et trois BA, un câble représentatif et des charges factices, sans aucune tension secteur sur les sorties. Ces configurations MUST NOT être essayées sur une armoire en service. Les injections SHALL être faites sur des échantillons de banc : une carte destinée à une installation ne reçoit que des contrôles non destructifs (mesures hors tension, niveaux, temps de montée).

Le critère B SHALL s'entendre ainsi :

- la communication peut s'interrompre ;
- elle reprend automatiquement en moins de 10 s, avec resynchronisation ;
- aucune sortie ne change d'état sans commande ;
- aucune dégradation permanente.

Le critère C SHALL s'entendre ainsi :

- la fonction peut s'interrompre ;
- elle est rétablie par une action automatique du BP (cycle d'alimentation des BA) ou par une intervention (déconnexion d'un port, réarmement) ;
- les sorties peuvent changer d'état pendant l'interruption, puis sont resynchronisées ;
- aucune dégradation permanente.

Les niveaux visés sont :

| Essai | Niveau, couplage | Configurations | Critère |
|---|---|---|---|
| IEC 61000-4-2 (ESD) | ±8 kV au contact, ±15 kV dans l'air | Toutes | B |
| IEC 61000-4-4 (EFT) | ±1 kV par pince capacitive sur le câble de bus, ±2 kV sur l'alimentation | Toutes | B |
| IEC 61000-4-5 (onde de choc) | 2 kV en mode commun entre îlots, réseau de couplage 40 Ω + 2 Ω (environ 48 A) | Cartes retouchées avec interconnexion, révision E | B |
| IEC 61000-4-5 (onde de choc) | 2 kV en mode commun entre îlots, générateur seul de 2 Ω (environ 1 kA) | Révision E avec carte d'interconnexion | Aucune dégradation permanente |
| IEC 61000-4-5 (onde de choc) | 0,5 kV ligne-ligne sur le bus et sur le +15 V, réseau de couplage 40 Ω + 2 Ω | Toutes | B en révision E, défaillance confinée sur une carte retouchée |

L'onde en mode commun entre îlots SHALL être injectée entre le 0 V du BP et le 0 V du BA en bout de chaîne.

#### Scenario: Décharges électrostatiques

- **WHEN** des décharges de ±8 kV au contact et de ±15 kV dans l'air sont appliquées sur banc aux connecteurs et au câble de bus
- **THEN** le banc SHALL satisfaire le critère B

#### Scenario: Transitoires rapides en salves

- **WHEN** des salves de ±1 kV sont couplées au câble de bus par une pince capacitive, puis des salves de ±2 kV sont appliquées à l'alimentation
- **THEN** le banc SHALL satisfaire le critère B

#### Scenario: Onde de choc en mode commun par réseau de couplage

- **WHEN** une onde de choc de 2 kV est appliquée par un réseau de couplage de 40 Ω + 2 Ω entre le 0 V du BP et le 0 V du BA en bout de chaîne, sur des cartes retouchées avec l'interconnexion du coffret, puis sur la révision E
- **THEN** le banc SHALL satisfaire le critère B

#### Scenario: Onde de choc de forte énergie sur la révision E

- **WHEN** une onde de choc de 2 kV est appliquée par le générateur seul de 2 Ω entre le 0 V du BP et le 0 V du BA en bout de chaîne, sur une configuration révision E avec carte d'interconnexion
- **THEN** les cartes MUST NOT subir de dégradation permanente

#### Scenario: Courant d'équilibrage entre îlots

- **WHEN** un courant en onde 8/20 égal à 0,8 fois le seuil de conduction mesuré pour la configuration essayée est injecté entre le 0 V du BA en bout de chaîne et le 0 V du BP, sur des BA retouchés avec la liaison équipotentielle du coffret (étoile 0 V ou renfort 0 V, nommée dans le rapport)
- **THEN** le courant dans SDA et dans SCL SHALL rester inférieur à 100 mA au connecteur du BA en bout de chaîne
- **AND** le rail « +12V » de chaque BA SHALL rester sous 20 V, l'écrêteur de +Vin pouvant conduire

#### Scenario: Injection de 1 kA sur la révision E

- **WHEN** un courant de 1 kA en onde 8/20 est injecté entre le 0 V du BA en bout de chaîne et le 0 V du BP, sur une configuration révision E avec carte d'interconnexion
- **THEN** les cartes MUST NOT subir de dégradation permanente

#### Scenario: Refus d'essai hors banc

- **WHEN** le harnais d'essai est lancé avec un banc déclaré qu'il ne reconnaît pas
- **THEN** il MUST NOT envoyer de commande ni appliquer de stimulus, et SHALL s'arrêter en erreur avec un message explicite

#### Scenario: Aucun banc déclaré

- **WHEN** le harnais d'essai est lancé sans banc déclaré, par exemple en intégration continue
- **THEN** les tests de banc SHALL être marqués « skipped », sans envoyer de commande ni appliquer de stimulus

### Requirement: Défaillance maîtrisée sous contrainte excessive

Sur une configuration révision E avec carte d'interconnexion, sous une contrainte croissante jusqu'à la défaillance, le premier composant à lâcher SHALL être un élément de protection : écrêteur, limiteur réarmable ou élément série. Ce ne doit pas être un circuit intégré (MCF, traducteur de niveau, PIC ou régulateur).

Sur une configuration de cartes retouchées, le PIC et le régulateur U1 peuvent lâcher avant les éléments de protection. La défaillance SHALL alors rester confinée à une seule carte.

Dans les deux cas, la défaillance SHALL être détectable par les mesures de diagnostic au repos (fuite de ligne, test en mode diode). Elle SHALL pouvoir être isolée en déconnectant un seul port du bus.

#### Scenario: Essai jusqu'à la défaillance en révision E

- **WHEN** deux ou trois échantillons de la configuration révision E avec carte d'interconnexion sont soumis sur banc à une contrainte croissante jusqu'à la défaillance
- **THEN** le premier composant défaillant de chaque échantillon SHALL être un élément de protection
- **AND** après déconnexion du seul port défaillant, les autres BA SHALL acquitter leurs commandes

#### Scenario: Essai jusqu'à la défaillance des cartes retouchées

- **WHEN** deux ou trois échantillons de la configuration de cartes retouchées sont soumis sur banc à une contrainte croissante jusqu'à la défaillance
- **THEN** la défaillance de chaque échantillon SHALL rester confinée à une seule carte et être détectable au repos
- **AND** après déconnexion du seul port défaillant, les autres BA SHALL acquitter leurs commandes
