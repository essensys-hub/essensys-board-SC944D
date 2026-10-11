## Context

Ticket : [essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1). La motivation est dans `proposal.md` (Why), les exigences dans `specs/`. Cette section décrit l'état actuel chiffré, lu sur les schémas, les BOM et le code des cartes SC944D, SC940D, SC941C et SC942C, ainsi que les ordres de grandeur déduits pour l'installation touchée le 9 août 2026. Les valeurs déduites restent à confirmer par la mesure.

### Interface I2C des BA (SC940D, SC941C, SC942C, topologie identique)

- **Lignes de bus.**
  - J1 ∥ J2 portent SDA, SCL, GNDBUS et +VBUS.
  - D1/D2 (SMF5V0A, VBR 6,4 à 7,1 V, 1120 pF) écrêtent vers le 0 V local, côté connecteur.
  - Elles sont suivies de **R1/R2 = 0 Ω** (R1/R3 sur le SC941C), puis des broches RC7/RC6 du PIC16F946.
  - Pull-ups : 10 kΩ vers +5 V (R40/R41, R43/R44, R51/R52). C23/C25 (C24/C26, C27/C29) ne sont pas montés : rien ne sépare leur pastille de la broche du PIC.
- **Le PIC écrête à la place de la TVS.** Sa diode interne conduit vers 5,5 V, avant la TVS. Son maximum absolu IIK est de ±20 mA. Dès environ 30 A de courant d'équilibrage, la broche reçoit 1 à 2 A.
- **Alimentation.**
  - Chaîne : +VBUS → R79 (0 Ω en 0603 ou 0805 ; R93 sur le SC941C, R84 sur le SC942C) → FI1 (25 V) → D3 → rail « +12V » à 14,3 V → C2 (10 µF / 16 V) → U1 (LMR12010, VIN 20 V en fonctionnement, 24 V en absolu) → +5 V. D6 est en 20 V.
  - Aucune TVS sur cette chaîne. L3 et C40 (0603) sont retirés depuis la modification DM de 2017 : R79/R80 sont montées.
- **Firmware v1.7.**
  - Pas de chien de garde : `__CONFIG(HS & WDTDIS)` (`main.c:17`).
  - Aucun traitement d'erreur du module SSP (`slavenode.c:361`).
  - Un PIC bloqué avec CKP = 0 retient SCL jusqu'à une coupure d'alimentation.
  - Le module SSP de base du PIC16F946 n'offre pas de contrôle logiciel de l'ACK.
- **Isolement 230 V / TBT.** J1/J2 sont estimés à 2 ou 3 mm du relais K1 et de VR1, distance non mesurée sur les fichiers de fabrication. Le BP s'impose lui-même 5,5 mm pour le 230 V. Le SC941C dessert des pièces d'eau.
- **Un BA non alimenté bloque le bus.** Son PIC, réalimenté par ses diodes de protection, tient SDA et SCL entre 0,6 et 3 V. Un écrêteur de ligne en court-circuit a le même effet.

### Interface I2C du BP (SC944D)

- **Chaîne de la ligne.**
  - Broches 28/29 du MCF52259 : VIN maximal 4,0 V, VIL 1,155 V, aucune protection sur 55 mm de piste.
  - R157/R158 de 4,7 kΩ, puis **U19** (PCA9306, 7 V maximum, −0,5 V minimum ou IIK de 50 mA au plus).
  - R155/R156 de 1 kΩ vers +5VS.
  - Perles L30/L32 (0,35 Ω, 400 mA).
  - D33/D34 (SMF5V0A, qui écrêtent vers 9 à 14 V en onde 8/20), puis J31, connecteur unique du bus.
- **Alimentation du bus.**
  - BA_ALIM (+15VSP) passe par T32 (P-MOS, « 2 A ») et L31 (empreinte 1812) jusqu'à J31-4, **sans TVS, sans fusible et sans PTC**.
  - U16 (LD1117, entrée 15 V maximum) est alimenté par +15VSP, qui vaut 15,17 V en nominal : il est déjà hors marge.
  - +15VS alimente aussi U2 (LMR12010, VIN 20 V en fonctionnement) et C110 (22 µF / 25 V). La diode interne de T32 les relie à J31-4 quand +15VSP dépasse +15VS.
  - Avec J58 ouvert, +15VSP est toujours fourni : la sortie TE2 n'a alors aucun effet.
- **Couplages de l'Ethernet.** La terminaison Bob Smith de J32 est reliée au 0 V par environ 1 nF / 2 kV, et le blindage par C63 (1 nF / 2 kV). J32 est isolé à 1,5 kV et se trouve à 18 mm de J31.

### Le bus dans son ensemble

| Grandeur | Valeur | Commentaire |
|---|---|---|
| Pull-ups équivalents | 769 Ω | Environ 6,5 mA à l'état bas, contre 3 mA en mode standard |
| Capacité | 2 à 3 nF | Fixée par les quatre TVS de chaque ligne |
| Temps de montée de 0 à 3,45 V | 1,4 à 3,2 µs | — |
| Marge de niveau bas BA → BP | 0,70 V | Pour un écart de masse nul |
| Marge de niveau bas BP → BA | Inconnue | La sortance par défaut du MCF est faible (2 mA) ; aucune écriture de PDSR n'a été trouvée |
| Ligne au repos, au BA le plus éloigné | 0,1 à 0,25 V sous son +5 V | Les +5 V des cartes (4,83 à 5,03 V) et ΔVg diffèrent : la fuite se mesure carte par carte |

### Firmware BP 099-37

- **Pas de récupération de bus.** La reprise se limite à `fclose`/`fopen`, jusqu'à 50 fois.
- **Commandes abandonnées.** Au-delà de 21 échecs, une commande est abandonnée et **marquée acquittée**.
- **Configurations perdues.** Les trames de configuration perdues ne sont jamais renvoyées (`ba.c:202-209`, `313-316`).
- **Index 12 en retard.** L'index 12 (`comm_faults`, champ `Information` de `H/TableEchange.h`) n'est mis à jour qu'après une commande. Ses bits 0 et 1 signalent les défauts de communication du compteur et de l'IHM, ses bits 2 à 4 ceux des BA.
- **BSP I2C introuvable.** Le BSP I2C de production, celui qui avait des timeouts, est perdu. Un rebuild réintroduit cinq boucles d'attente infinies.
- **Image de production.** L'image de production `.S19` 099-37 est disponible dans `SC944D/Prog/099-37/`.

### Mécanisme sur l'installation touchée

- **Topologie.** L'installation compte un coffret unique et quatre îlots TBTS flottants (BP et trois BA). Ils sont reliés par trois tronçons d'environ 33 cm de câble cat. 5e en AWG26, dans l'ordre BP-J31 → SC942C → SC941C → SC940D.
- **Impédance d'un tronçon.** Entre une ligne et son 0 V, un tronçon présente R ≈ 0,09 Ω, dont les contacts représentent la moitié, et une inductance différentielle L − M de 0,09 à 0,16 µH.
- **Le câble de bus est un pont, pas une antenne.** Au-delà de 6,3 à 7,8 V d'écart entre les 0 V, SDA et SCL deviennent des chemins parallèles au fil 0 V. Ce seuil est atteint, au SC940D, vers **20 A en onde 8/20** et vers **11 A sur un front de 1 Ipk/µs**.
- **Effet sur le +15 V.** Le fil +15 V ne porte pas le courant d'équilibrage. Il charge alors le rail « +12V » du BA dont le 0 V descend : C2 dépasse 16 V dès 2 V d'écart, et U1 atteint 24 V vers 10 V d'écart (environ 28 A).
- **Porte d'entrée inconnue.** La porte d'entrée de l'énergie n'est pas établie. Quatre hypothèses restent concurrentes : côté photovoltaïque puis Ethernet, réseau en mode commun, surtension temporaire, événements indépendants. La conception doit donc traiter **tous** les chemins.

## Goals / Non-Goals

**Goals:**

- Lever, sans refaire de circuit imprimé, les défauts qui transforment un courant d'équilibrage modéré en destruction : courant non limité vers les circuits intégrés, +15 V non protégé, composants hors marge.
- Relever d'un facteur 4 au moins le courant d'équilibrage que supporte un coffret unique, sans jamais relier le 0 V au PE.
- Couper la seule liaison métallique directe de l'îlot BP vers des équipements reliés au PE : l'Ethernet.
- Rendre le système résilient : une panne locale est détectée, signalée et contenue, et le BP rétablit seul la communication et l'état de chaque BA dès que le défaut disparaît ou que le BA en cause est déconnecté.
- Préparer la révision E et la carte d'interconnexion, puis qualifier l'ensemble sur banc.
- Garder la compatibilité avec le parc : BA 1.7, BP 099-37, protocole HTTP legacy IoT inchangé.

**Non-Goals:**

- Établir la cause exacte de l'événement du 9 août 2026. Elle relève du diagnostic de l'installation (tâches 1.x) et ne change pas la conception.
- Libérer automatiquement le bus d'un BA non isolé et non alimenté : sans MIB ni interface isolée intégrée, c'est physiquement impossible. Le confinement est alors manuel, guidé par l'index 12.
- Réaliser la génération 3 en RS-485 isolé. Seule la décision est formalisée dans un ADR.
- Modifier le protocole HTTP entre le BP et le serveur, ou le serveur lui-même. L'alerte sur `comm_faults` fait l'objet d'un ticket séparé.
- Intervenir sur l'installation photovoltaïque ou sur le tableau électrique. Ces travaux relèvent de l'installateur PV et de l'électricien. Les règles correspondantes ne sont que recommandées dans le guide d'installation.
- Obtenir une certification. Les essais visent une pré-qualification sur banc, puis des essais en laboratoire décidés plus tard.

## Decisions

### D1. Résistance série des BA : 47 Ω anti-impulsion, liaison équipotentielle si l'écart de masse l'impose

R1/R2 (R1/R3 sur le SC941C) passent de 0 Ω à 47 Ω en 0805 anti-impulsion (ERJ-P06J470V ou CRCW080547R0FKEAHP).

| R série (pull-ups du BP) | Injection au seuil de la TVS | Injection à VC ≈ 12 V | Injection à −2,5 V | Niveau bas au MCF pour ΔVg = 0 / 0,1 / 0,25 V | Verdict |
|---|---|---|---|---|---|
| 0 Ω (actuel, 1 kΩ) | Non limitée | Non limitée | Non limitée | 0,46 / 0,55 / 0,69 V | Refusé |
| 33 Ω (1 kΩ) | — | ≈ 200 mA | ≈ 58 mA | 0,63 / 0,72 / 0,85 V | Refusé : dépasse les plafonds de 150 mA et de 50 mA |
| **47 Ω (1 kΩ)** | 19 à 34 mA | ≈ 140 mA | ≈ 40 mA | 0,70 / 0,78 / 0,92 V | **Retenu si ΔVg ≤ 0,12 V** |
| 100 Ω (1 kΩ) | 9 à 16 mA | ≈ 65 mA | ≈ 19 mA | 0,94 / 1,02 / 1,15 V | Refusé avec les pull-ups de 1 kΩ (marge de 0,01 à 0,22 V) |
| 47 Ω au BA et 47 Ω au BP (1 kΩ) | — | — | — | 0,88 / 0,97 / 1,09 V | Refusé avec les pull-ups de 1 kΩ |

**Pourquoi 47 Ω.** C'est le meilleur compromis entre le courant injecté et la marge de niveau bas au MCF, qui doit rester d'au moins 0,3 V sous le seuil de 1,155 V. Une résistance de 47 Ω ne ramène pas le PIC sous ses 20 mA. Elle transforme une destruction quasi certaine en survie probable. Le courant de broche se déduit de la tension aux bornes de R1, seul chemin vers la broche.

**33 Ω est écarté.** Il dépasse par construction les plafonds de courant de la spec, et n'apporte qu'une marge de niveau que la liaison équipotentielle donne mieux.

**Règle selon l'écart de masse.** La règle dépend de l'écart de masse statique ΔVg, **mesuré** entre J31-3 et J1-3 du BA le plus éloigné, relais monostables collés. La tâche 2.1 en donne une estimation à l'établi ; la mesure de la tâche 1.11 la confirme.

| ΔVg mesuré | Règle |
|---|---|
| ≤ 0,1 V | 47 Ω |
| Entre 0,1 et 0,12 V | 47 Ω, en vérifiant que le niveau bas au MCF reste sous 0,85 V ; sinon, liaison équipotentielle |
| > 0,12 V | Liaison équipotentielle (D4) posée avant la remise en service des BA retouchés, puis 47 Ω |

Sur l'installation touchée, ΔVg attendu est de 0,06 à 0,16 V sans renfort, et de 0,02 à 0,05 V avec N1 ou N2.

### D2. Écrêteur secondaire BAT54S (option renforcée)

**Montage côté BA.** Une BAT54S est montée « en l'air » sur les empreintes C23/C25 (C24/C26 sur le SC941C, C27/C29 sur le SC942C), côté PIC, après R1/R2. Son anode va au 0 V, sa cathode au +5 V par la pastille haute de R40/R41. R1/R2 restent à 47 Ω. Elles ne passent à 100 Ω qu'une fois l'option posée côté BP avec des pull-ups de 2,2 kΩ, la sortance du MCF vérifiée (tâche 3.6), ΔVg d'au plus 0,1 V et le niveau bas mesuré au MCF sous 0,85 V. Dans tous les autres cas, elles restent à 47 Ω.

**Limite de la BAT54S.** À 135 mA (47 Ω, ΔVg > 0,1 V), sa VF atteint 0,8 à 0,9 V : la diode du PIC, en parallèle, peut alors prendre bien plus de 20 mA. L'objectif de 20 mA ne vaut donc qu'avec 100 Ω. Le courant de broche ne peut pas se mesurer sur une carte de série : il se mesure sur un échantillon de banc instrumenté (shunt inséré entre la BAT54S et la broche), ou s'estime par V(broche) − VDD sur une courbe de diode caractérisée. La révision E ajoute une résistance entre l'écrêteur secondaire et le PIC.

**Montage côté BP.**

- Une BAT54S est montée sur les empreintes C79/C80, côté U19, avec sa cathode sur +5VS.
- L30/L32 sont remplacées par des résistances de 47 Ω en 0603 anti-impulsion (ERJ-P03J470V).
- R155/R156 passent à 2,2 kΩ, ce qui ramène le courant à l'état bas de 6,4 à 3,9 mA.

**Effet attendu.** U19 reste vers 5,8 V au lieu de 9 à 14 V. En injection négative, U19 descend vers −0,6 V pour environ 40 mA : c'est admis par le maximum absolu du PCA9306, qui borne VI à −0,5 V **ou** le courant d'entrée à 50 mA. Une injection résiduelle d'environ −0,5 V subsiste sur les broches 28/29 du MCF (limite −0,3 V). Elle est consignée comme écart admis, et n'est levée qu'en révision E, avec une protection au pied du MCF.

**BP retouché sans option.** Il garde L30/L32 : U19 reste écrêté vers 9 à 14 V par D33/D34. Ce risque résiduel est consigné sur la fiche de la carte jusqu'à la révision E.

**Condition d'application.** L'option dépend du temps de montée mesuré avec les pull-ups d'origine de 1 kΩ :

| Temps de montée mesuré | Pull-ups du BP | Résistances série |
|---|---|---|
| ≤ 2,3 µs | 2,2 kΩ | 47 Ω au BP ; 47 Ω au BA, 100 Ω possible selon la règle ci-dessus |
| De 2,3 à 2,97 µs | 1,5 kΩ | 47 Ω au BP et 47 Ω au BA |
| Au-delà | Option non appliquée | On reste sur D1 |

**Alternatives écartées.**

- **TVS 5 V plus puissantes** (SMBJ5.0A, SMF4L5.0A). Leur fuite peut atteindre 800 µA, soit environ 2,5 V perdus sur le niveau haut.
- **Condensateurs sur C23/C25.** Avec 47 à 100 Ω, la constante de temps n'est que de 1 à 2 ns, sans aucun effet contre la foudre.
- **Tampon TCA4307 par nœud.** TI déconseille l'architecture à nœud commun, qui crée de faux fronts quand le PIC étire l'horloge.

### D3. Le +15 V protégé par une PTC placée en amont de la TVS

**Règle de topologie.** Une TVS d'alimentation finit en court-circuit. Elle doit donc toujours se trouver **en aval** d'un limiteur réarmable propre au nœud qu'elle protège.

- **Côté BP**, deux variantes :
  - (a) L31 (empreinte 1812) est remplacée par une PTC CMS 1812 de 30 V au moins, et une SMAJ16A (déjà dans la BOM du BP) est posée entre J31-4 et J31-3, côté soudure ;
  - (b) un module sur rail DIN, avec une PTC radiale puis une TVS, est intercalé entre J31 et les BA. Il n'est admis que si le court-circuit de J31-4 est tenu sans limiteur sur le BP (essai de la tâche 3.3).

  Le courant de maintien à 50 °C doit valoir au moins 1,5 fois le courant de BA_ALIM mesuré, relais collés. Les PTC 1812 de 30 V au moins ont en général un courant de maintien faible : si aucune n'atteint ce seuil, la variante (b) s'impose. Pendant un déclenchement, il faut s'attendre à un redémarrage du BP.
- **Côté BA.** R79 (R93, R84), en 0603 ou 0805, est remplacée par une PTC de 30 V au moins en montage volant : aucune PTC de cette tension n'existe dans ce boîtier. Une TVS de 16 V (SMF16A ou SMAJ16A) est posée en montage volant, sur fils courts, sur les pastilles 0603 de C40 (C42 sur le SC942C), entre +Vin et le 0 V. **Jamais sur +VBUS en amont de R79** : c'est le nœud de chaînage partagé, et une TVS morte y couperait les trois BA. Avec l'étoile 0 V N2, ces éléments peuvent aussi se trouver sur le port dédié du BA.
- **Marges.**
  - La tension assignée retenue est la tension maximale recommandée en fonctionnement. Les écrêteurs, les PTC et les bobines de relais suivent leurs propres règles.
  - C2 passe en 10 µF / 35 V X5R, et D6 en Schottky 40 V (SOD-323F).
  - Sur le BP, U16 passe en LM317 SOT-223 (même brochage ; R66 = 115 Ω assure la charge minimale), ou en régulateur dont l'entrée tient au moins 40 V.
  - Exceptions assumées jusqu'à la révision E : U1 du BA retouché (LMR12010, 20 V en fonctionnement, sous 14,3 V) et U2 du BP (LMR12010, sous +15VS).
  - Sur le BP, U2 et C110 voient la tension d'écrêtage de J31-4 (19 à 26 V) par la diode interne de T32. La tension de +15VS est mesurée sur banc pendant la conduction de la SMAJ16A ; au-delà de 24 V, une protection de +15VS est ajoutée, ou l'exception est consignée.
- **Limite assumée de la retouche.** Au courant crête, la SMAJ16A laisse monter le rail vers 26 V, soit environ 25 V après D3. Cela dépasse le VIN de 24 V du LMR12010 et frôle les 25 V de FI1. U1 lâchera donc avant la TVS : sur une carte retouchée, la spec n'exige que le confinement de la défaillance. La révision E passe à un LMR16006 (4 à 60 V, même brochage), C2 en 50 V et FI1 en tenue supérieure.

**Alternatives écartées.**

- **TVS seule.** Une TVS en court-circuit éteint tout le bus.
- **Fusible.** Il n'est pas réarmable, ce qui impose une intervention à chaque déclenchement.
- **Résistance série sur la ligne d'alimentation.** Elle crée une chute de tension et ne coordonne rien.
- **L3 seule, ou retrait de R79/R80.** Le retour de SDA et SCL passe par GNDBUS. L3 y ajouterait environ 314 Ω à 50 kHz et pousserait le mode commun dans D1/D2.

### D4. Étoile 0 V doublée (N2) en premier, renfort 0 V (N1) à défaut

| Configuration entre le SC940D et le BP | Seuil en onde 8/20 | Seuil sur un front de 1 Ipk/µs | Seuil sur un front de 4 Ipk/µs |
|---|---|---|---|
| Actuelle (trois tronçons AWG26) | ≈ 20 A | ≈ 11 A | ≈ 4 A |
| N1 : 0 V de 4 mm² plaqué et soudé | ≈ 97 A | ≈ 25 A | ≈ 7 A |
| **N2 : étoile, chaque BA par J1 et J2 (AWG20, 15 cm au plus), BP par J31** | **≈ 80 à 120 A** | **≈ 50 à 65 A** | < 26 A (non recalculé) |
| Carte d'interconnexion, BP et BA révision E avec plot 0 V | ≈ 875 A | ≈ 292 A | ≈ 83 A |

**Correction du modèle de N2.** Le premier calcul (≈ 173 A et ≈ 79 A) doublait la liaison vers le BP, alors que J31 est unique, et ne comptait qu'un contact par liaison, alors que chaque faisceau en a deux. Avec R ≈ 45 à 75 mΩ, les seuils tombent aux valeurs du tableau, et l'écart statique ΔVg vaut 16 à 52 mV. La queue de cochon soudée sur la broche traversante J31-3 (ci-dessous) rapproche N2 du haut de la fourchette. La spec exprime donc les seuils par rapport à la référence mesurée sur le même banc (au moins 4 fois, en onde 8/20 comme en front rapide) et fixe ΔVg à 0,05 V au plus, ce qui suffit pour 47 Ω. De même, les ≈ 875 A de la carte d'interconnexion supposent un plot 0 V sur le BP révision E (tâche 6.5) : par J31 seul, 800 A sont hors d'atteinte.

**N2 est retenue.** C'est une petite carte passive sur rail DIN, au bord du coffret où se trouvent les J1/J2. Elle porte :

- un plan 0 V ;
- deux faisceaux AWG20 par BA (J1 et J2), et J31 pour le BP, doublé d'une queue de cochon AWG20 soudée sur la broche traversante J31-3 pendant la dépose du BP ;
- par port, une PTC suivie d'une TVS sur le +15 V ;
- par port, un cavalier qui coupe SDA, SCL et +15 V en gardant le 0 V.

Elle ne demande ni retouche des BA ni firmware : un BA d'origine raccordé à un port protégé de N2 est conforme pour le +15 V.

**N1 à défaut.** C'est un conducteur H07V-K de 2,5 à 4 mm², **plaqué contre chaque tronçon** (un collier tous les 5 cm). Il est soudé par une queue de cochon AWG20 sur la broche traversante J1-3 ou J2-3, avec un arrêt de traction, et jamais sur une pastille CMS. Les anodes de D1/D2 sont reliées au même point ; au BP, les anodes de D33/D34 et la broche J31-3. Son gain tombe de 5 fois à environ 2 fois sur un front raide : la spec lui fixe des seuils propres (3 fois et 2 fois la référence). Le chaînage est conservé : la déconnexion d'un BA se fait par retrait de son câble, et coupe les BA en aval.

**Variante écartée en solution durable.** Relier le J1 libre du SC940D au moins de la batterie du BP est une variante dégradée : le courant traverse alors le plan logique du BP.

**Repérage et TBTS.** Chaque renfort est isolé et repéré « 0 V TBTS – NE PAS RELIER À LA TERRE », jamais en vert-jaune ni en bleu clair. Le coffret ne reçoit aucune borne ni aucun raccordement PE ; le PE d'un départ 230 V qui le traverse reste gainé avec son circuit, dans les goulottes 230 V. Ces règles sont à valider par un électricien avant leur publication.

### D5. Ethernet du BP isolé galvaniquement, sans parafoudre relié au PE

**Retenu.** Deux solutions satisfont l'exigence de 4 kV efficaces pendant 60 s :

- une paire de convertisseurs fibre, le convertisseur côté BP étant alimenté **uniquement depuis l'îlot TBTS** du BP (12-24 V) ;
- un isolateur Ethernet de 4 kV efficaces au moins.

**Raison.** Le Bob Smith et C63 couplent les huit fils au 0 V du BP. Le port Ethernet est le seul de l'îlot qui ait une liaison métallique directe vers des équipements potentiellement reliés au PE. Le choix entre fibre et isolateur reste ouvert (voir Open Questions).

**Alternatives écartées.**

- **Parafoudre RJ45 relié au PE côté BP.** Lors d'une montée du PE, il porterait les paires à ce potentiel et injecterait directement dans le 0 V de l'îlot, par J32 (1,5 kV) et le Bob Smith. Il ouvrirait ainsi un nouveau chemin pendant l'événement même qu'on redoute.
- **Bloc d'alimentation dédié pour le convertisseur, même TBTS flottant.** Il recouple le BP au réseau par ses capacités Y ou primaire–secondaire.
- **Câble blindé avec écran raccordé côté BP.** Un C63 percé suffit alors à relier le 0 V au PE.

Si un équipement relié au PE doit être protégé, le parafoudre se pose à **son** extrémité. Mieux encore, on supprime le cuivre (Wi-Fi ou fibre).

### D6. Carte d'interconnexion pour le coffret unique, MIB pour les BA déportés

| Critère | A. MIB par BA | B. Protection des ports à la frontière | C. Carte d'interconnexion (fond de panier) |
|---|---|---|---|
| Principe | Couper le chemin galvanique entre îlots | Réduire le courant capté | Rendre l'écart de 0 V négligeable |
| Effet | Courant résiduel ≈ C·dV/dt (0,2 A pour 200 pF à 1 kV/µs) | Courant réduit, écart entre cartes inchangé | Seuil de N2, puis ≈ 875 A avec un plot 0 V sur le BP et les BA révision E |
| Effets de bord | Reporte la tension sur la barrière la plus faible (J1/J2 près de K1) ; trois points de défaillance de plus | Ne traite pas les nappes intérieures | Ne coupe rien : 47 Ω et BAT54S restent nécessaires |
| Coût estimé | 150 à 250 € pour trois MIB | 60 à 200 € (Ethernet) | 60 à 100 € |
| Coffret unique | Surdimensionné | Nécessaire | **Retenu** |
| BA déporté | **Retenu** | Nécessaire | Insuffisant |

**Contenu de C.**

- Un plan 0 V, sur un PCB 2 couches en cuivre de 2 oz, raccordé au BP par J31 et par son plot 0 V en révision E (ou une queue de cochon sur J31-3 pour un BP révision D).
- Un bus en étoile.
- Par port : une TVS de 50 pF au plus sur SDA et SCL, **sans résistance série** ; une PTC de 30 V au moins et une SMCJ16A sur le +15 V ; un cavalier. Une résistance de port s'ajouterait à R1 : un BA retouché présenterait 94 Ω en série, soit environ 0,94 V au MCF avec les pull-ups de 1 kΩ.
- Des traversées IDC 26 points pour les nappes J3, avec une TVS par entrée. Sans elles, seuls 4 à 6 fils sur 26 seraient déviés.

**MIB, pour les BA déportés.**

- Un BA est déporté s'il est hors du coffret du BP, ou si sa liaison de bus dépasse 0,5 m. Le MIB est qualifié jusqu'à 3 m de câble ; au-delà, un tampon de bus qualifié ou la génération 3.
- Un ISO1640DW (barrière renforcée).
- Un convertisseur isolé 9-36 V → 12 V de 6 à 10 W, renforcé de 4 à 5 kVAC, avec une tenue en onde de choc spécifiée.
- VCC1 est pris sur le +5 V du BA : un BA mort met l'isolateur en haute impédance.
- Un superviseur surveille le côté BA. Si une ligne y reste basse plus de 100 ms, il coupe VCC1, ce qui libère le côté bus en moins de 150 ms. Il émet alors jusqu'à neuf impulsions d'horloge côté BA, puis réactive VCC1 à un intervalle croissant plafonné à 5 s. Chaque essai qui retrouve la ligne basse est coupé en moins de 150 ms. Un esclave figé au milieu d'un octet est ainsi libéré sans intervention ; le timeout SSP du BA 1.8 (D8) couvre le même cas côté firmware.
- Les TVS D1/D2 du BA sont déposées et C1 reste au plus à 80 pF.
- Le connecteur côté isolé est d'une autre série, et J2 est neutralisé.
- **Prérequis sur le bus.** Le plafond de 400 pF côté bus inclut le BP : D33/D34 (1120 pF chacune) y sont remplacées par des TVS de 50 pF au plus, par une retouche dédiée ou en révision E, et aucun BA non isolé du même bus ne garde ses TVS d'origine. Le temps de montée se mesure de 30 % à 70 % de la tension d'alimentation, comme dans la norme I2C : 0,85 × 2,2 kΩ × 400 pF ≈ 0,75 µs.

**Alternatives écartées.**

- **I2C différentiel PCA9615.** Il n'est pas isolé et ne tolère que 2,5 V de décalage de masse : aucune tenue à la foudre.
- **LTC4331.** Il est point à point et non isolé.
- **Isolement au seul BP, ou ISO1640 à la place de U19.** Les BA restent reliés entre eux. La capacité C2 de 400 pF au plus est incompatible avec les quatre TVS d'environ 1 nF.
- **Étoile active TCA9548A.** Elle demande un recâblage et du firmware. Elle est réservée à la génération 3. Dans un coffret, l'étoile passive suffit : des piquages de 15 cm ne posent aucun problème de ligne à 50 kHz.

### D7. Éclateur à gaz (GDT) entre le 0 V et la terre : rejeté

- **En fin de vie**, il se met en court-circuit et relie le 0 V à la terre. L'installation passe de fait en TBTP, contraire à la NF C 15-100 §414. Le SC941C dessert des pièces d'eau, où la §701 n'admettrait que la TBTS en volumes 1 et 2 : ce point est à confirmer avec l'électricien.
- **La variante avec une résistance de 1 MΩ** en parallèle crée déjà une liaison galvanique.
- **Il ne protège pas les ports I2C**, car il amorce à plusieurs centaines de volts. Il ne garantit pas non plus le déclenchement du différentiel.
- **Il crée une boucle** entre la masse du bus et la barrette de terre.

### D8. Firmware : pilote reconstitué d'abord, résilience ensuite, protocole inchangé

**BP.** Les étapes s'enchaînent dans cet ordre :

1. **Préalable bloquant.** L'étirement d'horloge maximal d'un BA 1.7 (CKP = 0 en émission, latence de l'interruption) est d'abord mesuré sur banc. Le pilote I2C MQX est ensuite réécrit et publié dans essensys-gcc, avec des timeouts **en temps** : 2 ms par attente, ou deux fois l'étirement mesuré s'il dépasse 1 ms, et 20 ms par transaction. D'ici là, le BP reste en 099-37 : aucun nouveau build n'est flashé.
2. **Récupération de bus.** PAS0/PAS1 passent en GPIO pour lire les niveaux. Le firmware émet jusqu'à neuf coups d'horloge en drain ouvert émulé, puis un STOP, puis remet `I2CR` à zéro et rétablit `IEN`. Les tentatives sont limitées à une par seconde.
3. **Corrections.** Le pointeur de fichier périmé après une réinitialisation (`ba_i2c.c:86`, `98`, `180`…) et la fuite mémoire quand `fclose` échoue sont corrigés.
4. **Résilience.**
   - Une file par BA, qui garde le dernier état demandé de chaque sortie. Un état maintenu est appliqué quel que soit son âge ; une commande de volet ou impulsionnelle expire après un délai configurable (5 min par défaut) et est signalée, jamais marquée acquittée.
   - Plus d'abandon marqué comme acquitté.
   - Un état de santé par BA : OK → SUSPECT → ABSENT.
   - Une interrogation de présence toutes les 30 à 60 s.
   - Une resynchronisation complète au démarrage, à chaque retour à OK, et quand l'octet d'état `comm_stat` d'un BA 1.8 signale un redémarrage. Le BP lit ce sixième octet s'il est présent, et fonctionne sans lui avec un BA 1.7.
5. **Dernier recours.** Un cycle de TE2 après 60 s de blocage persistant, avec au moins 1 h entre deux cycles et 3 cycles au plus par 24 h. Le compteur est conservé en mémoire non volatile à travers un redémarrage du BP. Après un cycle qui ne rétablit pas le bus (cas typique : un BA mort qui le tient), plus aucun cycle n'a lieu jusqu'au retour du bus ou jusqu'à une remise à zéro manuelle. L'état de J58 est déclaré dans la configuration : avec J58 ouvert, TE2 est sans effet et aucun cycle n'est tenté. Chaque cycle est compté et daté sur la console de diagnostic.

**Protocole.** Le protocole HTTP legacy IoT n'est pas touché. L'index 12 garde son codage : bits 0 et 1 pour le compteur et l'IHM, bits 0x04, 0x08 et 0x10 pour les BA. Il est seulement mis à jour sans attendre une commande. Les critères portent sur `(index 12 & 0x1C)`.

**BA 1.8.** Le firmware BA est reflashé par ICSP, avec un `.hex` par type. Il apporte :

- le chien de garde ;
- un état d'erreur SSP : SSPEN à 0 puis à 1, CKP à 1, purge de SSPOV et WCOL ;
- un timeout SSP : si SDA ou SCL reste tenue par le BA plus de 50 ms sans front sur SCL, le module SSP est réinitialisé, même si la boucle principale tourne ;
- un NACK quand `CmdBuf` est occupé. Le SSP de base n'a pas de contrôle logiciel de l'ACK : le firmware laisse SSPBUF non lu (BF = 1), ce qui fait refuser matériellement l'octet suivant (SSPOV) ; il purge SSPOV dès que `CmdBuf` se libère ;
- la LED d'état ;
- `comm_stat` dans un sixième octet de lecture, que le BP 099-37 ignore, avec un drapeau de redémarrage.

Le build essensys-gcc est corrigé : ses trois macros `TYPE_*` sont aujourd'hui définies ensemble. `hard.h` est également corrigé.

**Alternatives écartées.**

- **Matériel seul.** Le critère B (reprise en moins de 10 s avec resynchronisation) est inatteignable sans firmware.
- **Rebuild du BP depuis les sources actuelles.** Il réintroduit cinq attentes infinies.
- **Flasher un BA avec le build essensys-gcc actuel.** Les trois types y sont confondus.
- **Cycle TE2 répété sans limite.** Dans le cas le plus probable, un BA mort qui tient le bus, il ne sert à rien et fait retomber les relais monostables des BA sains à chaque fois.

### D9. Génération 3 : RS-485 isolé, décision par ADR

**Retenu pour la génération 3.** C'est la couche physique cible quand le PIC16F946, dont l'approvisionnement est tendu, devra être remplacé :

- côté BP, l'UART2 sur les broches 28/29 ;
- côté BA, l'AUSART avec détection d'adresse 9 bits ;
- un transceiver isolé et une alimentation isolée par nœud.

Les trames et le CRC-16 restent identiques. Le pilote série MQX standard remplace le BSP I2C perdu.

**Alternatives écartées.**

- **CAN isolé.** Le PIC16F946 n'a pas de contrôleur CAN, et un MCP2515 entrerait en conflit sur RC4.
- **I2C isolé permanent.** C'est la solution des MIB, limitée à environ 3 m.

Cette décision n'est pas réalisée ici. Elle est consignée dans un ADR (tâche 6.8).

### D10. Non-régression matérielle sur banc outillé

**Le banc.** Il réunit un BP, trois BA (un de chaque type) et un câble représentatif : trois tronçons de 33 cm, puis 1 m et 3 m. Il a des charges factices, aucune tension secteur sur les sorties, une alimentation de laboratoire flottante de 22 à 24 V et un serveur legacy local isolé qui capture le trafic HTTP. Le banc n'est jamais raccordé au serveur de production. Les injections se font sur des échantillons de banc, jamais sur des cartes destinées à une installation.

**Les tests.**

- Le harnais vit dans `tests/bench/` de ce dépôt. Les tests sont écrits en pytest, avec le marqueur `@pytest.mark.nr("NR-board-sc944d-<n>", "#1")`, conforme à la convention `NR-<repo-court>-<n>` de la non-régression Essensys, et pilotent les instruments : oscilloscope, analyseur logique à sondes isolées, mires à transistor, charge électronique.
- Ils produisent un JUnit XML que `nonreg_report.py --sources tests/bench` consolide.
- Sans banc déclaré, les tests de banc sont marqués « skipped » : la CI et le nightly ne les voient ni réussir ni échouer, et aucun Bug `regression` n'est ouvert à tort. Avec un banc déclaré mais non reconnu, le harnais s'arrête en erreur sans appliquer de stimulus.
- Les tests hôtes du pilote I2C, sur bus simulé, vivent dans essensys-gcc et tournent dans sa CI. Ceux qui protègent un comportement de cette feature portent un identifiant `NR-gcc-<n>` et la référence `essensys-hub/essensys-board-SC944D#1`.

Les résultats de banc sont joints au rapport `/checkup`.

**Numérotation.**

| Test | Objet |
|---|---|
| NR-board-sc944d-1 | Niveaux I2C et fuite par carte |
| NR-board-sc944d-2 | Retrait d'un BA |
| NR-board-sc944d-3 | Attentes bornées |
| NR-board-sc944d-4 | Récupération de bus |
| NR-board-sc944d-5 | Commandes conservées |
| NR-board-sc944d-6 | Resynchronisation |
| NR-board-sc944d-7 | Télémétrie |
| NR-board-sc944d-8 | Chien de garde et gestion d'erreur du BA |
| NR-board-sc944d-9 | Court-circuit du +15 V d'un port |
| NR-board-sc944d-10 | Branchement à chaud |
| NR-board-sc944d-11 | Compatibilité croisée et HTTP |
| NR-board-sc944d-12 | Endurance 72 h |
| NR-board-sc944d-13 | Superviseur du MIB |

## Risks / Trade-offs

- **[47 Ω ne respecte pas le maximum absolu du PIC]** (environ 140 mA à VC = 12 V). → Option BAT54S (D2) sur chaque carte déposée, avec 100 Ω quand les pull-ups du BP passent à 2,2 kΩ. Révision E avec écrêteur secondaire et résistance vers le PIC. Liaison équipotentielle (D4) qui relève le seuil d'équilibrage.
- **[La marge de niveau bas BA → BP diminue]** (de 0,70 V à 0,46 V pour ΔVg = 0). → ΔVg mesuré ; au-delà de 0,12 V, liaison équipotentielle avant la remise en service. Niveau bas au MCF vérifié sous 0,85 V après chaque retouche (NR-board-sc944d-1).
- **[La marge BP → BA est inconnue]** (sortance du port du MCF). → Mesure obligatoire au PIC (< 1,2 V) avant et après retouche. Si elle échoue, pas d'option renforcée côté BP, et les BA restent à 47 Ω.
- **[La protection +15 V reste partielle sur une carte retouchée]** (environ 25 V après D3, contre 24 V pour U1). → Défaillance confinée au BA concerné (PTC propre). C2 et D6 relevés. Correction complète en révision E (LMR16006).
- **[+15VS du BP exposé par la diode interne de T32]** (U2, C110). → Mesure sur banc ; protection de +15VS ou exception consignée jusqu'à la révision E.
- **[Un déclenchement de PTC redémarre le BP ou le BA, et la PTC chauffe au-delà de 100 °C]**. → Courant de maintien choisi à 50 °C avec une marge de 1,5. Pas de gaine thermorétractable au contact d'autres conducteurs. Sélectivité des PTC par port démontrée sur banc (NR-board-sc944d-9).
- **[La carte d'interconnexion ne coupe rien]** : au-delà du seuil, les TVS conduisent encore. → 47 Ω et BAT54S protègent les circuits intégrés. Ethernet isolé à la frontière. MIB si les essais à 1 kA en onde 8/20 montrent des erreurs.
- **[Un BA non alimenté, ou une TVS de port en court-circuit, bloque le bus jusqu'à la déconnexion de son port]** : le confinement est manuel en coffret, et le cycle TE2 n'y peut rien. → Détection et signalement dans l'index 12 en moins de 120 s, diagnostic guidé par la console, cavalier par port avec N2. Le confinement n'est automatique qu'avec un MIB (NR-board-sc944d-13).
- **[Les distances 230 V / TBT des cartes révision D ne sont pas démontrées]** (J1/J2 estimés à 2 ou 3 mm de K1, alors que le SC941C dessert des pièces d'eau). → Relevé sur les fichiers de fabrication (tâche 2.11), inspection à la loupe lors du diagnostic, mesure compensatoire ou restriction d'usage documentée, révision E à 6 mm au moins.
- **[Un rebuild du BP figerait la tâche I2C]** (BSP perdu). → Le BP n'est jamais reflashé tant que le pilote n'est pas reconstitué et que NR-board-sc944d-3 et -4 ne sont pas au vert. La flash est sauvegardée par BDM avant toute retouche lourde. Le `.S19` 099-37 sert de retour arrière.
- **[Un BA est flashé avec le mauvais type, donc la mauvaise adresse]**. → Un `.hex` par type, la lecture de l'EEPROM avant un remplacement de PIC, et la vérification de l'adresse acquittée.
- **[Un renfort 0 V finit relié au PE par un intervenant]**, ce qui fait passer l'installation en TBTP. → Repérage aux deux extrémités, aucune borne PE dans le coffret, consigne dans le guide d'installation, contrôle de l'isolement 0 V–PE sous 250 V continu.
- **[La cause de l'événement reste inconnue]**. → La conception traite tous les chemins : équipotentialité à l'intérieur, isolement à la frontière, confinement des défauts.
- **[Des données de l'installation touchée fuitent dans des dépôts publics]**. → Les artefacts et les pages publiques ne citent que « une installation touchée le 9 août 2026 ». Les mesures nominatives restent dans le dossier privé de l'installation. Toute publication sur l'issue #1 se fait après accord, et un contrôle de relecture est prévu (tâches 8.x).
- **[Une intervention a lieu dans un coffret sous tension, ou sur une source autonome du site]**. → Les mesures préalables sous tension (tâches 1.1 et 1.2) sont faites par une personne habilitée (BR ou BE Mesurage), sans toucher les cartes. Ensuite, consignation selon la NF C 18-510, y compris de toute source autonome du site. Le côté PV est réservé à l'installateur PV. Aucun outil n'est introduit dans un coffret ouvert sous tension hors de ce mesurage.
- **[La maintenance de l'installation touchée est confondue avec un essai]**. → Les opérations des groupes 1 et 4 sur l'installation sont des opérations de maintenance manuelles. Aucun harnais, script NR ni stimulus automatisé ne vise une armoire réelle.

## Migration Plan

1. **Phase 0 : sécurisation.** Elle porte sur l'installation touchée et sur les règles du parc :
   - consignation, essais d'isolement et coupure du cuivre entre les équipements reliés au PE et le commutateur du BP ;
   - contrôles par l'électricien et l'installateur PV, hors du périmètre Essensys.
2. **Phase 1 : réparation et retouches court terme.**
   - Retouches D1, D3 et éventuellement D2 sur les cartes déposées.
   - Pose de N2, ou de N1 à défaut, avant la remise en service si ΔVg dépasse 0,12 V.
   - Isolement de l'Ethernet du BP (D5).
   - Les BA d'apparence saine sont retouchés à leur prochaine dépose.
3. **Phase 2 : firmware et prototypes.** Le pilote I2C est reconstitué, puis la résilience BP est ajoutée et la v1.8 des BA écrite. Le prototype de la carte d'interconnexion est réalisé, ainsi que le MIB pour les BA déportés. Le tout est qualifié sur banc.
4. **Phase 3 : révision E** des BA et du BP.
5. **Phase 4 : génération 3** en RS-485 isolé, sur décision de l'ADR.

**Cohabitation des cartes**

| Configuration | Compatibilité | Règle |
|---|---|---|
| BA d'origine (0 Ω) | Bus actuel, N2 | À retoucher à la prochaine dépose |
| BA retouché (47 Ω, PTC et TVS, C2 et D6) | BP d'origine ou retouché, N1, N2, carte d'interconnexion (sans résistance de port) | Niveaux vérifiés (NR-board-sc944d-1). Carte étiquetée « retouche foudre » |
| BA en option renforcée à 100 Ω | Seulement avec un BP renforcé à 2,2 kΩ | Revenir à 47 Ω si le BP perd son option |
| BA préparé pour MIB (D1/D2 déposées) | Uniquement derrière son MIB, sur un bus préparé (D33/D34 ≤ 50 pF) | Connecteur d'une autre série, J2 neutralisé et étiqueté |
| BA révision E | Mêmes brochages J1/J2, mêmes adresses | Niveaux revérifiés si le bus mélange des TVS d'origine (1120 pF) et des TVS à faible capacité |
| BP retouché, option renforcée | Seulement si le temps de montée mesuré le permet (D2) | Sinon BP retouché sans option |
| BP retouché sans option | Tous les BA à 47 Ω | Risque résiduel sur U19 consigné jusqu'à la révision E |

**Firmware**

- **BP.** Le BP reste en 099-37 jusqu'à ce que le pilote reconstitué passe NR-board-sc944d-3, -4 et -12 sur banc. Un candidat est d'abord qualifié sur banc avec toute la suite NR. Il est ensuite déployé sur un premier BP en service, après sauvegarde BDM, comme opération de maintenance. Retour arrière : flasher le `.S19` 099-37 par BDM.
- **BA.** Le passage de la v1.7 à la v1.8 se fait par ICSP, carte par carte, avec le `.hex` du type et après lecture de l'EEPROM. Retour arrière : reflasher le `.hex` v1.7 du type. La v1.8 est compatible avec le BP 099-37 (NR-board-sc944d-11) : les deux migrations sont indépendantes.

**Retour arrière matériel.** Les retouches sont réversibles composant par composant. Si le niveau bas au MCF échoue, on revient à la valeur précédente de la résistance série (de 100 Ω à 47 Ω), ou on pose d'abord la liaison équipotentielle.

## Open Questions

- **Références exactes des PTC et des TVS d'alimentation.** Le courant de maintien dépend du courant des BA, qui reste à mesurer à 50 °C. Il décide entre la PTC 1812 et le module DIN côté BP (règle fixée en D3). Ces choix n'affectent ni les exigences ni l'approche.
- **Fibre ou isolateur pour l'Ethernet du BP.** Les deux satisfont l'exigence de 4 kV efficaces. Le choix se fait par installation, selon la place disponible.
- **Valeur exacte de la distance d'isolement 230 V / TBT en révision E** : 6 ou 8 mm, selon l'EN 60664-1 et la norme produit. La spec impose déjà le plus contraignant des deux (6 mm au moins et la distance d'isolement renforcé exigée) ; seule la valeur à saisir dans les règles DRC reste à calculer.
- **Fonctionnement d'un BA derrière un MIB.** Il reste à vérifier sur banc la tension d'enclenchement des relais à 11 V et à chaud, ainsi que le comportement de l'ISO1640 quand son côté non alimenté est réalimenté par ses broches. Cela ne concerne que le MIB.
- **Outil de CAO et dépôt de la carte d'interconnexion et du MIB** : Altium Designer 16 comme les cartes existantes, ou un autre outil ; dossier de ce dépôt ou dépôt dédié. À fixer à l'ouverture des sub-issues.
- **Laboratoire et budget des essais finaux.** La pré-qualification se fait avec des générateurs loués. Les essais finaux en laboratoire accrédité se décident après la phase 2.
