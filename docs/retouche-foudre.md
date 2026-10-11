# Retouche foudre du SC944D : note de schéma

Cette note décrit la variante « retouche foudre » du boîtier principal SC944D (BP), révision D. La retouche se fait sur un BP déposé, à l'établi, sans refaire de circuit imprimé.

| Référence | Valeur |
|---|---|
| Change OpenSpec | `i2c-surge-protection-2026-10-007` (`openspec/changes/i2c-surge-protection-2026-10-007/`, décisions D2 et D3 du `design.md`) |
| Ticket | [essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1) |
| Tâches | 3.1 à 3.10 pour le BP, 2.9 pour les BA |
| BOM de la variante | `BOM_retouche-foudre.csv`, à la racine du dépôt |
| BOM d'origine | `BOM_[No Variations].csv`, inchangée |
| Schéma | `SC944D/Assembly/SC944D_Schematic.pdf` : page 7 (Liaison Boîtiers Auxiliaires), page 3 (Alimentation 2/2), page 2 (Alimentation 1/2) |

## 1. Contexte

Le 9 août 2026, une surtension d'origine atmosphérique a touché une installation Essensys. Un boîtier auxiliaire (BA) a été mis hors service, et le BP ne reçoit plus de réponse d'aucun BA. L'analyse du change montre des défauts communs à tout le parc. Sur le BP, ce sont les suivants :

- le courant injecté par le bus vers le traducteur de niveau U19 n'est pas limité ;
- BA_ALIM sort sur J31-4 sans TVS, sans fusible et sans PTC ;
- U16 fonctionne déjà hors marge en régime nominal.

Les retouches décrites ici corrigent ces défauts sur les cartes en service. Elles ne remplacent pas les autres volets du change, qui restent nécessaires : liaison équipotentielle du coffret (N2 ou N1, tâches 4.x), isolement de l'Ethernet du BP, firmware (tâches 5.x) et révision E.

### 1.1 Interface d'origine

D'après la page 7 du schéma, le fichier de placement et la BOM « DM » au format Excel de `SC944D/Assembly/` :

```text
MCF52259 br. 28 (SCL) / br. 29 (SDA) : 3,3 V, VIN max 4,0 V, VIL 1,155 V, aucune protection sur 55 mm
   │  R157 (SCL) / R158 (SDA) 4,7 kΩ vers +3V3S
U19 PCA9306DCUT : 7 V max ; −0,5 V min, ou 50 mA max en entrée
   │  R156 (SCL) / R155 (SDA) 1 kΩ vers +5VS        C80 (SCL) / C79 (SDA) 18 pF, non montés
L30 (SCL) / L32 (SDA) : ferrite Würth 742792643, 0603, 0,35 Ω, 400 mA
   │  D33 (SCL) / D34 (SDA) : SMF5V0A-GS08 vers le 0 V
J31 (Würth 66200421122) : 1 = BA_SDA, 2 = BA_SCL, 3 = 0 V, 4 = BA_ALIM

+15VS ─ T32 (NTD2955G, commandé par TE2) ─ +15VSP ─ L31 (ferrite 1812, 8 A, repère « 1A current ») ─ J31-4
                                             └─ U16 LD1117STR-E (entrée 15 V max) ─ +12VSP
                                                   R66 = 115 Ω (OUT–ADJ), R88 = 1 kΩ (ADJ–0 V)
```

La diode interne de T32 va de +15VSP (drain) vers +15VS (source). Quand +15VSP dépasse +15VS, elle conduit et expose +15VS, donc U2 et C110, à la tension présente sur J31-4.

### 1.2 Remarque sur la BOM CSV d'origine

`BOM_[No Variations].csv` ne liste que 331 des 491 repères physiques du fichier de placement `SC944D/Assembly/SC944D_Pick'nPlace.csv`. Elle compte 339 repères, dont 8 pseudo-repères : DOC1, MIR1 à MIR3, NOTE1 à NOTE3 et SI1. Il y manque notamment toute l'interface du bus : J31, D33, D34, L30, L31, L32, R154 à R158, U19, C86 et C87. Les valeurs d'origine de cette note viennent donc du fichier de placement, de la BOM « DM » et du schéma.

La variante recopie la BOM CSV ligne pour ligne, avec une seule ligne modifiée : U16 est retiré de la ligne LD1117STR-E. Les lignes de la retouche sont ajoutées à la fin du fichier. Compléter la BOM CSV d'origine sort du périmètre de ce change.

## 2. Synthèse des modifications

| Repère | Fonction | Avant | Après | Tâche | Statut |
|---|---|---|---|---|---|
| D33, D34 | Écrêteurs de BA_SCL et BA_SDA | SMF5V0A-GS08 | SMF5V0A-E3-08 | 3.2 | Base |
| U19 | Traducteur de niveau | PCA9306DCUT | Inchangé ; remplacé à l'identique si son test de 1.8 échoue ou si D33/D34 ont claqué | 3.2 | Conditionnel, hors BOM |
| L31 | Élément série de BA_ALIM | Ferrite 1812 Würth 74279226101 | PTC CMS 1812 de 30 V au moins, à dimensionner | 3.3 (a) | Base |
| DRF1 (ajout) | Écrêteur de BA_ALIM | — | SMAJ16A entre J31-4 et J31-3, côté soudure | 3.3 (a) | Base |
| U16 | Régulateur de +12VSP | LD1117STR-E | LM317 en SOT-223 (LM317DCYR) | 3.5 | Base |
| R66 | Résistance OUT–ADJ de U16 | 115 Ω | Inchangée | 3.5 | — |
| R155, R156 | Pull-ups côté bus | 1 kΩ | 2,2 kΩ, ou 1,5 kΩ selon 2.6 | 3.7 | **OPTION** |
| L30, L32 | Élément série des lignes | Ferrite 0603 Würth 742792643 | 47 Ω anti-impulsion, Panasonic ERJ-P03J470V | 3.7 | **OPTION** |
| C79, C80 (empreintes) | Écrêteur secondaire côté U19 | Non montés (18 pF) | BAT54S | 3.7 | **OPTION** |

DRF1 est un repère propre à la retouche : le circuit imprimé SI944D n'a pas d'empreinte pour cet écrêteur. Dans la BOM de la variante, les lignes de l'option renforcée ont un nom qui commence par `OPTION_` et une description qui commence par « OPTION RENFORCÉE ».

En **variante (b)** de la tâche 3.3, un module sur rail DIN protège le +15 V en aval de J31. Le BP garde alors L31 d'origine, et DRF1 n'est pas posée (section 3.2).

## 3. Détail des modifications

### 3.1 D33 et D34 : SMF5V0A-E3-08 (tâche 3.2)

- **Même composant électrique.** VRWM 5 V, VBR de 6,4 à 7,1 V, environ 1120 pF, VC de 9,2 V à 21,7 A en onde 10/1000. Seule la référence de commande change : le suffixe GS08 ne figure plus dans le tableau de commande du fabricant.
- **Remplacement systématique sur un BP déposé après l'événement.** Un écrêteur qui a conduit peut garder une fuite ou un court-circuit latent. Un court-circuit sur une ligne bloque tout le bus.
- **Pas d'écrêteur 5 V plus puissant** (SMBJ5.0A, SMF4L5.0A). Leur fuite peut atteindre 800 µA. Avec quatre écrêteurs par ligne sous les 769 Ω de pull-ups équivalentes, environ 2,5 V seraient perdus sur le niveau haut.
- **Limite.** Un bus préparé pour un module d'isolement par BA (MIB) exige au contraire des écrêteurs de 50 pF au plus à la place de D33/D34. Ce cas relève d'une retouche dédiée ou de la révision E, pas de cette variante.

### 3.2 +15 V de J31 : PTC à la place de L31, ajout de DRF1 (tâche 3.3)

**Avant.** BA_ALIM part de +15VSP par la seule ferrite L31 (6 mΩ, 8 A). Une surtension sur J31-4 atteint directement +15VSP, donc U16, C98 et les sorties alimentées par +15VSP (pages 13 et 16 à 19 du schéma). Par la diode interne de T32, elle atteint aussi +15VS, donc U2 et C110.

**Règle de topologie (décision D3).** Une TVS d'alimentation finit en court-circuit. Elle doit donc se trouver en aval d'un limiteur réarmable propre au nœud qu'elle protège. Une TVS seule sur J31-4 éteindrait tous les BA en mourant.

**Variante (a), retenue dans la BOM.**

- **PTC CMS 1812 sur l'empreinte de L31.**
    - Tension maximale de 30 V au moins. Elle couvre la tension d'écrêtage crête de la SMAJ16A (26 V).
    - Courant de maintien à 50 °C d'au moins 1,5 fois le courant de BA_ALIM mesuré, relais monostables collés. Ce courant est la somme des courants d'entrée des BA raccordés, mesurés par type à la tâche 2.1. Le courant de maintien se lit sur la courbe de déclassement en température de la fiche technique, et non sur la valeur à 23 °C de l'en-tête.
    - **À dimensionner selon la mesure de la tâche 2.1** : aucune consommation fiable des BA n'existe dans le dossier. À titre d'ordre de grandeur, une hypothèse de 2 à 5 W par BA donne 0,5 à 1,1 A pour trois BA. Le courant de maintien visé serait alors de 0,75 à 1,65 A à 50 °C.
    - Les PTC 1812 de 30 V au moins ont en général un courant de maintien faible. Si aucune n'atteint le seuil, la variante (b) s'impose.
    - La chute de tension dans la PTC, à sa résistance maximale après un premier déclenchement, doit laisser BA_ALIM à 14 V ou plus au dernier BA.
- **SMAJ16A (DRF1)**, soudée côté soudure sur fils courts, la cathode sur J31-4 et l'anode sur J31-3, donc en aval de la PTC. Ce composant figure déjà dans la BOM du BP (D8, D63).
    - VRWM de 16 V, au moins égale à la tension nominale du nœud (environ 15,2 V).
    - VBR de 17,8 V au minimum ; écrêtage entre 19 et 26 V selon le courant.
- **Effets attendus.**
    - J31-4 est borné vers 19 à 26 V, alors qu'il ne l'était pas du tout.
    - Une DRF1 en court-circuit fait déclencher la PTC au lieu d'effondrer +15VSP.
    - Pendant un déclenchement, +15VS s'affaisse jusqu'à l'ouverture de la PTC : il faut s'attendre à un redémarrage du BP.
- **Montage.** Une PTC déclenchée dépasse 100 °C. Elle ne doit pas être enfermée sous une gaine thermorétractable au contact d'autres conducteurs.
- **Sélectivité avec N2.** Avec l'étoile 0 V N2, chaque port porte sa propre PTC. Un court-circuit sur un port ne doit pas déclencher la PTC de L31 (tâche 4.4) : son courant de maintien se choisit aussi en fonction des PTC de port.
- **Sélectivité en chaîne.** Sans N2, les BA retouchés portent leur propre PTC à la place de R79 (R93 sur le SC941C, R84 sur le SC942C ; tâche 2.3). Un court-circuit franc sur +Vin d'un BA ne doit priver d'alimentation que ce BA : la PTC du BA doit déclencher avant celle de L31. Deux conditions, à vérifier sur les courbes des fiches techniques lors du choix des références (tâche 2.3) :
    - le courant de maintien de la PTC du BA est nettement inférieur à celui de la PTC de L31 ;
    - au courant de court-circuit, le temps de déclenchement de la PTC du BA est plus court que celui de la PTC de L31.

    L'essai « écrêteur d'un BA en court-circuit » de `NR-board-sc944d-9` le vérifie sur banc.

**Variante (b), module sur rail DIN.**

- Un module est intercalé entre J31 et les BA : une PTC radiale, puis une TVS (SMAJ16A) entre les fils +15 V et 0 V.
- Exemple de PTC radiale : la famille Bourns MF-R, déjà présente dans la BOM (F5). Le MF-R110 tient 1,10 A à 23 °C et environ 0,85 A à 50 °C. On prend un MF-R135 ou un MF-R160 si nécessaire.
- Cette variante n'est admise que si un court-circuit de J31-4 est tenu sans limiteur sur le BP (essai sur banc de la tâche 3.3).
- Le BP garde alors L31 d'origine, et DRF1 n'est pas posée : une TVS sans limiteur en amont violerait la règle de topologie.
- Comme toute protection du coffret, le module n'a aucun raccordement au PE.

### 3.3 U16 : LM317 à la place du LD1117 (tâche 3.5)

- **Défaut d'origine.**
    - L'entrée du LD1117 tient 15 V au maximum. +15VSP vaut environ 15 V en nominal (15,17 V théoriques en sortie de U12, page 2) : il n'y a aucune marge.
    - Une fois DRF1 posée, l'écrêtage de J31-4 (19 à 26 V) atteint +15VSP à travers la PTC. Il détruirait le LD1117.
- **LM317 en SOT-223** (référence proposée : Texas Instruments LM317DCYR).
    - Même brochage : 1 = ADJ, 2 = OUT, 3 = IN, languette = OUT.
    - Le LM317 est limité à 40 V entre son entrée et sa sortie. Avec 26 V en entrée et 12 V en sortie, l'écart n'est que de 14 V.
    - La spec exige une tension d'entrée assignée d'au moins 22,8 V (1,5 × 15,2 V) : c'est satisfait.
- **R66 = 115 Ω et R88 = 1 kΩ sont inchangées.**
    - Tension de sortie : 1,25 V × (1 + 1000 / 115) = 12,12 V. Le courant d'ajustement (environ 50 µA dans R88) ajoute environ 0,05 V : +12VSP vaut environ 12,2 V.
    - Aux extrêmes de la référence du LM317 (1,20 à 1,30 V), la sortie couvre environ 11,7 à 12,7 V, à la limite haute de la plage de la tâche 3.5 (12,1 V ± 5 %, soit 11,5 à 12,7 V). D'où la mesure obligatoire.
    - Charge minimale : R66 impose 1,25 V / 115 Ω = 10,9 mA, au-dessus des 10 mA que le LM317 exige au plus.
- **Dissipation inchangée** (régulateur linéaire) : environ (15,2 − 12,1) V × 0,25 A ≈ 0,78 W au courant maximal noté page 3. C'est l'ordre de grandeur de la note de la page 3 (0,75 W, SOT-223 donné pour 1,5 W).
- **Point à vérifier sur banc.**
    - La chute minimale du LM317 (environ 2 V) dépasse celle du LD1117 (environ 1,1 V). En nominal, l'écart d'environ 3 V suffit.
    - Sur batterie avec J58 ouvert, +15VSP peut descendre à 10,5 V (note de la page 16). Aucun des deux régulateurs ne tient alors 12,1 V, et le LM317 donne une sortie plus basse, d'environ 0,5 à 1 V.
    - Il faut vérifier sur banc que le convertisseur isolé U4, alimenté par +12VSP, fonctionne encore dans ce cas.

### 3.4 Option renforcée (tâche 3.7)

**Conditions.** L'option n'est posée que si deux mesures l'autorisent :

- le temps de montée mesuré à la tâche 2.6, avec les pull-ups d'origine de 1 kΩ, est d'au plus 2,97 µs ;
- le niveau bas émis par le BP et mesuré au PIC du BA le plus éloigné (tâche 3.6) est inférieur à 1,2 V.

Les trois éléments vont ensemble. Avec les pull-ups de 1 kΩ, 47 Ω au BP plus 47 Ω au BA donneraient 0,88, 0,97 et 1,09 V au MCF pour ΔVg = 0, 0,1 et 0,25 V. Cette combinaison est refusée par la décision D1.

| Temps de montée mesuré (1 kΩ) | R155/R156 | Pull-ups équivalentes (avec les trois 10 kΩ des BA) | Facteur sur le temps de montée | Temps de montée attendu |
|---|---|---|---|---|
| ≤ 2,3 µs | 2,2 kΩ (Vishay CRCW06032K20FKEA) | 769 → 1325 Ω | × 1,72 | ≤ 3,96 µs |
| De 2,3 à 2,97 µs | 1,5 kΩ (Vishay CRCW06031K50FKEA) | 769 → 1034 Ω | × 1,34 | ≤ 3,99 µs |
| > 2,97 µs | 1 kΩ conservées, option non appliquée | — | — | — |

Le temps de montée attendu reste ainsi sous le plafond de 4 µs de la spec.

- **R155/R156.** À 2,2 kΩ, le courant à l'état bas passe de 6,4 à 3,9 mA. Les BA ne peuvent passer à 100 Ω (tâche 3.8) qu'avec des pull-ups de 2,2 kΩ ; avec 1,5 kΩ, ils restent à 47 Ω.
- **L30/L32 remplacées par 47 Ω anti-impulsion.** La ferrite (0,35 Ω) ne limite rien à l'échelle de la microseconde. La résistance de 47 Ω limite le courant vers l'écrêteur secondaire et vers U19.
- **BAT54S sur C79 (BA_SDA) et C80 (BA_SCL).** Une BAT54S par ligne, montée « en l'air » sur l'empreinte du condensateur non monté, côté U19 :
    - broche 1 (anode) sur la pastille GND ;
    - broche 3 (point commun) sur la pastille de la ligne ;
    - broche 2 (cathode) reliée à +5VS par un fil court, sur la pastille haute de R155 (pour C79) ou de R156 (pour C80).

    Le brochage se vérifie au testeur de diodes avant et après la pose. Les quelques pF ajoutés par ligne sont négligeables devant les 2 à 3 nF du bus.

- **Effet chiffré**, pour une tension d'écrêtage de D33/D34 de 12 V en onde 8/20.
    - **Injection positive.** (12 − 5,7) V / 47 Ω ≈ 135 mA dans la BAT54S. U19 reste vers 5,8 V, sous ses 7 V. 5,7 V correspond à +5VS plus la chute directe de la BAT54S à ce courant.
    - **Injection négative à −2,5 V.** (2,5 − 0,6) V / 47 Ω ≈ 40 mA. U19 descend vers −0,6 V pour environ 40 mA. C'est admis par le maximum absolu du PCA9306, qui borne la tension d'entrée à −0,5 V **ou** son courant à 50 mA.
    - **Écart admis.** Une injection résiduelle d'environ −0,5 V subsiste sur les broches 28/29 du MCF, dont la limite est −0,3 V. Elle est consignée comme écart admis jusqu'à la révision E, qui ajoute une protection au pied du MCF.
    - **Rail +5VS.** Un régulateur à découpage n'absorbe pas de courant entrant. L'impulsion est absorbée par la consommation et les découplages de +5VS. La montée de +5VS pendant l'injection se relève sur l'échantillon de banc de la tâche 3.7.

## 4. Risque résiduel sur U19 sans option

!!! warning "BP retouché sans option renforcée"
    Le BP garde L30/L32 (ferrites de 0,35 Ω) et ses pull-ups de 1 kΩ. Rien ne limite alors le courant entre D33/D34 et U19 à l'échelle de la microseconde.

- **Injection positive.** D33/D34 écrêtent vers 9 à 14 V en onde 8/20, à quelques dizaines d'ampères, alors que U19 tient 7 V au maximum : U19 partage le courant avec l'écrêteur.
- **Injection négative.** D33/D34 conduisent en direct jusqu'à environ −2,5 V à 50 A. U19 tolère −0,5 V, ou 50 mA en entrée. L'impulsion peut traverser le transistor passant de U19 jusqu'aux broches 28/29 du MCF, dont la limite est −0,3 V.
- **Ce qui réduit l'exposition sans la supprimer.**
    - La liaison équipotentielle du coffret relève le seuil du courant d'équilibrage. Avec N2, le gain est d'au moins 4 fois la référence. Avec N1, posée à défaut, il est d'au moins 3 fois en onde 8/20 et 2 fois sur un front raide.
    - L'isolement de l'Ethernet du BP coupe le seul lien métallique direct de l'îlot vers des équipements reliés au PE.
- **Consignation.** Ce risque est consigné sur la fiche de la carte jusqu'à son passage en révision E, et l'étiquette du BP porte « sans option » (tâche 3.10).
- **Règle pour les BA.** Avec un BP sans option, tous les BA restent à 47 Ω. Le passage à 100 Ω est exclu.

## 5. Exceptions de marge assumées

La règle de la spec veut que tout composant relié au +15 V du bus, à +15VS ou à +15VSP ait une tension assignée d'au moins 1,5 fois sa tension nominale. Les écrêteurs, les limiteurs réarmables et les bobines de relais suivent leurs propres règles. Les exceptions suivantes sont assumées :

| Composant | Contrainte | Situation | Levée |
|---|---|---|---|
| U2 du BP (LMR12010, VIN de 20 V en fonctionnement, 24 V en absolu) | Sous +15VS (environ 15,2 V), soit environ 1,3 fois au lieu de 1,5 fois | Exception assumée, prévue par la spec | Révision E |
| U2 et C110 (22 µF / 25 V) du BP | Tension d'écrêtage de J31-4 (19 à 26 V), reportée sur +15VS par la diode interne de T32 | **Listés en exception tant que la tâche 3.4 n'est pas faite.** La tâche 3.4 mesure +15VS sur un échantillon de banc pendant que DRF1 conduit. Au-delà de 24 V, une protection de +15VS est ajoutée à la retouche, ou l'exception est maintenue | Tâche 3.4, puis révision E |
| U1 des BA retouchés (LMR12010, sous 14,3 V) | 20 V en fonctionnement ; environ 25 V après D3 au courant crête de la TVS du BA | Exception assumée, listée dans la note de retouche de chaque BA (tâche 2.9) | Révision E des BA (LMR16006) |
| Broches 28/29 du MCF52259 (BP en option renforcée) | Injection résiduelle d'environ −0,5 V, pour une limite de −0,3 V | Écart admis | Révision E du BP |

- **Ne pas confondre les U1.** L'exception sur U1 concerne le régulateur des BA. U1 du BP est le DRV8801PWP : il n'est pas concerné.
- **+15VS ne peut pas se décharger dans C109.** +15VS n'est relié au +15 V de l'alimentation qu'à travers D45 (PMEG4050EP), et à VBATT qu'à travers T14 (page 2). L'énergie injectée par la diode interne de T32 ne peut donc pas s'écouler vers C109 (1000 µF) : seuls les découplages de +15VS l'absorbent. C'est pourquoi la mesure de 3.4 est nécessaire.
- **Point relevé lors de la rédaction, absent du design.**
    - C98 (1 µF / 25 V, à l'entrée de U16) et les autres charges de +15VSP sont directement sur le nœud que DRF1 écrête à travers la PTC. Ces charges comprennent les sorties alimentées par +15VSP, pages 13 et 16 à 19 du schéma.
    - Au courant crête, ces composants voient jusqu'à environ 26 V. C98 respecte la règle de 1,5 fois (25 V ≥ 22,8 V), mais pas cette valeur crête.
    - Il est proposé de relever aussi +15VSP lors de la mesure de la tâche 3.4. Au-delà de 25 V, C98 rejoint la liste des exceptions ou passe en 50 V. Les bobines de relais sont exclues de la règle.

## 6. Procédure

!!! danger "Contraintes permanentes"
    - Ne jamais piloter une armoire réelle pendant les essais. Les essais se font **sur banc uniquement**.
    - Les injections, les courts-circuits et les essais jusqu'à la défaillance se font sur des **échantillons de banc**. Un BP destiné à une installation ne reçoit que des contrôles non destructifs : mesures hors tension, niveaux et temps de montée.
    - TBTS : aucune liaison entre le 0 V et le PE, ni par la retouche, ni par le module DIN, ni par un renfort 0 V.
    - Ne jamais reflasher le BP avec un build non qualifié. En cas de remplacement du MCF, flasher uniquement le `.S19` 099-37.
    - La retouche est purement matérielle : le protocole HTTP legacy IoT n'est pas touché.

1. **Préalables.**
    - L'installation est consignée (tâche 1.5) et le BP est déposé.
    - L'isolement à 500 V continu est mesuré (tâche 1.6).
    - La fiche de mesures de la tâche 1.8 liste les composants défaillants.
    - À l'établi, le BP sous tension est alimenté par une alimentation de laboratoire flottante de 22 à 24 V.
2. **Sauvegarde BDM (tâche 3.1)**, avant toute retouche.
    - Sauvegarder la flash du BP par BDM, puis la comparer au `.S19` de production `SC944D/Prog/099-37/BP_MQX_ETH 20170116 save 099-37.elf_BOOT_CRC.S19`.
    - Archiver la sauvegarde, et documenter l'identité ou l'écart.
    - Le `.S19` 099-37 sert de retour arrière.
3. **D33/D34 (tâche 3.2)** : remplacement par des SMF5V0A-E3-08. U19 n'est remplacé que si son test de 1.8 a échoué ou si D33/D34 ont claqué.
4. **+15 V de J31 (tâche 3.3)** : variante (a), avec la PTC sur L31 et DRF1 ; ou variante (b), avec le module DIN. Le choix dépend du courant de maintien disponible.
5. **U16 (tâche 3.5)** : remplacement par un LM317 en SOT-223. R66 et R88 restent en place.
6. **Renfort 0 V éventuel.** Si N2 ou N1 est prévu (tâches 4.1 et 4.5), souder pendant la dépose la queue de cochon AWG20 sur la broche traversante J31-3. Avec N1, les anodes de D33/D34 rejoignent le même point.
7. **Option renforcée (tâche 3.7).**
    - Elle n'est posée que si les mesures des tâches 2.6 et 3.6 sont disponibles et l'autorisent.
    - Sinon, le BP est remis en service « sans option », et l'option attend une prochaine dépose.
8. **Contrôle visuel.** Les éléments rapportés (DRF1, PTC, BAT54S) ne doivent pas réduire les 5,5 mm d'isolement que le BP s'impose vis-à-vis des circuits 230 V. Inspection à la loupe, comme pour le relevé de la tâche 2.11.
9. **Étiquetage (tâche 3.10).** L'étiquette porte « retouche foudre », puis :
    - « option 2,2 kΩ », « option 1,5 kΩ » ou « sans option » ;
    - « +15 V : PTC L31 » ou « +15 V : module DIN ».
10. **Fiche de la carte.** Elle consigne :
    - la référence de la sauvegarde BDM et le résultat de la comparaison ;
    - les composants remplacés ;
    - la référence et le courant de maintien de la PTC, et le courant de BA_ALIM mesuré ;
    - +12VSP mesuré ;
    - la valeur de R155/R156, le temps de montée et les niveaux bas ;
    - le risque résiduel sur U19 si l'option n'est pas posée.

    La fiche reste dans le dossier privé de l'installation. Seule une synthèse anonymisée est publiée, après accord.

11. **Remise en service.** Remontage et mesures de la tâche 1.11 (ΔVg, niveaux), puis renvoi des configurations de tous les BA (tâche 1.12).

La mesure de +15VS de la tâche 3.4 se fait sur un **échantillon de banc** retouché selon la variante (a), jamais sur le BP de l'installation. Son résultat met à jour la liste des exceptions (section 5).

## 7. Critères de vérification (tâches 3.2 à 3.8)

| Tâche | Vérification | Où |
|---|---|---|
| 3.2 | J31 débranché, écart d'au plus 50 mV entre +5VS et chaque ligne (fuite ≤ 50 µA sous la pull-up de 1 kΩ) ; EN de U19 entre 3,6 et 4,4 V ; test fonctionnel de U19 réussi dans les deux sens | BP retouché |
| 3.3 | BA_ALIM de 14 V ou plus au dernier BA | Installation |
| 3.3 | Court-circuit de J31-4 à J31-3 pendant 60 s sans dégradation, y compris sans limiteur sur le BP en variante (b) ; BA_ALIM revient en 120 s au plus (procédure de `NR-board-sc944d-9`) | Échantillon de banc |
| 3.4 | +15VS mesurée et consignée pendant que DRF1 conduit son courant d'essai. Au-delà de 24 V, protection de +15VS ou exception pour U2 et C110. Relevé proposé : +15VSP, pour C98 | Échantillon de banc |
| 3.5 | +12VSP à 12,1 V ± 5 % sous +15VSP nominal | BP retouché |
| 3.6 | Niveau bas émis par le BP, mesuré au PIC du BA le plus éloigné, inférieur à 1,2 V. Sinon, l'option est exclue et les BA restent à 47 Ω | Installation |
| 3.7 | Hors tension, 44 à 50 Ω entre chaque ligne de J31 et U19 | BP retouché |
| 3.7 | Injection à +12 V : U19 côté bus ≤ 7,0 V et broche 29 du MCF ≤ 4,0 V. Injection à −2,5 V : U19 ≥ −0,5 V ou courant d'entrée ≤ 50 mA, et tension de la broche 28 consignée | Échantillon de banc |
| 3.7 | Temps de montée inférieur à 4 µs ; niveaux de `NR-board-sc944d-1` au vert | Installation |
| 3.8 | BA à 100 Ω seulement avec R155/R156 à 2,2 kΩ, 3.6 au vert et ΔVg ≤ 0,1 V. Contrôles : 95 à 105 Ω hors tension ; niveau bas au MCF < 0,85 V et au PIC < 1,2 V, sinon retour à 47 Ω ; courant de broche ≤ 20 mA sur un échantillon de banc instrumenté | BA et banc |

## 8. Variante des BA et cohabitation

**Retouches des BA.** Elles font l'objet d'une variante de BOM « retouche foudre » et d'une note de schéma dans les dépôts essensys-board-SC940, essensys-board-SC941C et essensys-board-SC942C (tâche 2.9). En résumé :

- R1/R2 (R1/R3 sur le SC941C) passent de 0 Ω à 47 Ω anti-impulsion en 0805, ou à 100 Ω en option.
- Une PTC de 30 V au moins remplace R79 (R93 sur le SC941C, R84 sur le SC942C).
- Une TVS de 16 V est posée sur les pastilles de C40 (C42 sur le SC942C), entre +Vin et le 0 V, jamais sur +VBUS.
- C2 passe en 10 µF / 35 V X5R, et D6 en Schottky 40 V.
- Option : BAT54S sur C23/C25 (C24/C26 sur le SC941C, C27/C29 sur le SC942C).
- L'exception assumée sur U1 est listée dans chaque note.

**Cohabitation avec le BP** (d'après le design, Migration Plan) :

| Configuration du BP | BA admis | Règle |
|---|---|---|
| BP retouché sans option | Tous les BA à 47 Ω, et les BA d'origine jusqu'à leur prochaine dépose | Risque résiduel sur U19 consigné jusqu'à la révision E |
| BP en option renforcée, 1,5 kΩ | BA à 47 Ω | Pas de BA à 100 Ω |
| BP en option renforcée, 2,2 kΩ | BA à 47 Ω ; BA à 100 Ω avec BAT54S si ΔVg ≤ 0,1 V et niveaux au vert | Si le BP perd son option, les BA à 100 Ω reviennent à 47 Ω |
| BP d'origine | BA d'origine ou retouchés à 47 Ω | À retoucher à la prochaine dépose |

## 9. Points ouverts

- **Référence de la PTC et choix entre les variantes (a) et (b)**, après la mesure du courant des BA (tâche 2.1). La ligne de la BOM reste « à dimensionner » jusque-là.
- **Résultat de la tâche 3.4** : il fixe la liste définitive des exceptions sur U2 et C110, ainsi que sur C98 si +15VSP est relevé.
- **Comportement de +12VSP et de U4** avec le LM317, sur batterie avec J58 ouvert.
- **BOM CSV d'origine incomplète** (section 1.2) : les lignes de la variante qui couvrent des repères absents de cette BOM sont des ajouts, et non des remplacements de lignes.
- **Révision E du BP (tâche 6.5)**. Elle lève les exceptions et le risque résiduel décrits ici, avec :
    - une protection au pied du MCF ;
    - 47 Ω et un écrêteur secondaire entre J31 et U19 ;
    - des TVS de ligne de 50 pF au plus ;
    - un eFuse et une SMCJ16A sur le +15 V ;
    - des régulateurs +12VSP et U2 dont l'entrée tient au moins 40 V.
