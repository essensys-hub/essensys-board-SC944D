# 🔌 Hardware

Cette section détaille la conception matérielle du module SC944D, le boîtier principal (BP) d'Essensys. Elle décrit aussi les retouches de la variante « retouche foudre ».

## Spécifications Techniques

*   **Tension d'alimentation** : [À compléter]
*   **Consommation** : [À compléter]
*   **Dimensions** : [À compléter]

## Composants Principaux

*   **MCU** : ColdFire **MCF52259**, en 3,3 V.
*   **Connecteurs** :
    *   **J31** : bus I2C vers les boîtiers auxiliaires (BA) et leur alimentation +15 V (voir ci-dessous).
    *   **J32** : Ethernet, embase RJ45 avec transformateur, isolée à 1,5 kV.
    *   [À compléter]

## Routage (PCB)

Le PCB est conçu en [Nombre] couches.

## Interface du bus des boîtiers auxiliaires (J31)

Le protocole est décrit dans [Protocoles I2C](i2c.md). Les valeurs ci-dessous sont lues sur le schéma (page « Liaison boîtiers auxiliaires ») et sur la BOM de la modification DM.

### Côté BP (SC944D, révision D)

```text
MCF52259 br. 28 (SCL) / br. 29 (SDA)   3,3 V, VIN max 4,0 V ; environ 55 mm de piste sans protection
  ├─ R157 (SCL) / R158 (SDA) 4,7 kΩ → +3V3S
U19 PCA9306 (translateur 3,3 V ↔ 5 V ; 7 V max côté bus)
  ├─ R156 (SCL) / R155 (SDA) 1 kΩ → +5VS          pull-ups du bus
  ├─ C80 (SCL) / C79 (SDA) 18 pF, non montés       côté U19
L30 (SCL) / L32 (SDA)  perles ferrite 0603 (0,35 Ω, 400 mA)
D33 (SCL) / D34 (SDA)  SMF5V0A vers le 0 V
J31  1 = SDA, 2 = SCL, 3 = 0 V, 4 = BA_ALIM

BA_ALIM : +15VS → T32 (P-MOS NTD2955G, commandé par TE2 ; cavalier J58) → +15VSP → L31 (ferrite 1812, 8 A) → J31-4
          ni TVS, ni fusible, ni PTC
```

**Points faibles de la révision D**, détaillés dans le `design.md` du change :

- D33/D34 écrêtent vers 9 à 14 V en onde 8/20, au-delà des 7 V que tolère U19.
- En polarité négative, la TVS conduit en direct jusqu'à environ −2,5 V. U19 tolère −0,5 V ou 50 mA, et le MCF −0,3 V.
- U16 (LD1117, entrée de 15 V au plus) est alimenté par +15VSP, qui vaut 15,17 V en nominal : il est hors marge en régime normal.
- La diode interne de T32 relie J31-4 à +15VS quand +15VSP dépasse +15VS. Elle expose ainsi U2 (LMR12010) et C110 (22 µF / 25 V) à une surtension du bus.
- Avec J58 ouvert, +15VSP est toujours fourni : la sortie TE2 n'a alors aucun effet.
- La terminaison de J32 et son blindage (C63, 1 nF / 2 kV) couplent les huit fils du câble Ethernet au 0 V du BP.

### Côté BA (SC940D, SC941C, SC942C)

La topologie est identique sur les trois cartes :

```text
J1 ∥ J2 (chaînage)  1 = SDA, 2 = SCL, 3 = GNDBUS, 4 = +VBUS
SDA/SCL : D1 / D2 SMF5V0A vers le 0 V local, côté connecteur
        → R1 / R2 = 0 Ω (R1 / R3 sur le SC941C)
        → pull-ups 10 kΩ vers +5 V ; empreintes 22 pF non montées
        → PIC16F946 RC7 (SDA) / RC6 (SCL), module SSP esclave
+VBUS → R79 0 Ω → +Vin → FI1 (25 V) → D3 → rail « +12V » ≈ 14,3 V (C2 10 µF / 16 V ; D6 20 V)
      → U1 LMR12010 (VIN 20 V en fonctionnement) → +5 V
```

| Fonction | SC940D | SC941C | SC942C |
| :--- | :--- | :--- | :--- |
| Résistances série SDA / SCL (0 Ω) | R1 / R2 | R1 / R3 | R1 / R2 |
| Pull-ups 10 kΩ SDA / SCL | R40 / R41 | R43 / R44 | R51 / R52 |
| Empreintes 22 pF non montées SDA / SCL | C23 / C25 | C24 / C26 | C27 / C29 |
| 0 Ω sur +VBUS | R79 | R93 | R84 |
| Empreinte non montée entre +Vin et 0 V | C40 | C40 | C42 |
| Adresse I2C | 0x11 | 0x13 | 0x12 |

**Points faibles des BA.**

- La diode interne du PIC conduit vers 5,5 V, avant la TVS : le PIC écrête à sa place, sans limitation de courant. Son maximum absolu est de ±20 mA.
- Le rail « +12V » n'a pas de TVS. C2 ne garde que 1,7 V de marge.
- Un BA non alimenté tient SDA et SCL entre 0,6 et 3 V environ : il bloque tout le bus tant qu'il reste raccordé.

## Variante « retouche foudre »

La variante « retouche foudre » corrige, **sans refaire de circuit imprimé**, les défauts révélés par une installation touchée le 9 août 2026. Les cartes en révision D concernées sont le BP SC944D et les BA SC940D, SC941C et SC942C. Le choix des valeurs est justifié dans le change OpenSpec [`i2c-surge-protection-2026-10-007`](https://github.com/essensys-hub/essensys-board-SC944D/tree/main/openspec/changes/i2c-surge-protection-2026-10-007) (`design.md`, décisions D1 à D3 ; ticket [#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1)).

!!! info "Documents de référence"
    - **Note de schéma** : [`docs/retouche-foudre.md`](retouche-foudre.md). Elle donne le montage de chaque retouche, le risque résiduel et les exceptions de marge assumées.
    - **Variante de BOM** : [`BOM_retouche-foudre.csv`](https://github.com/essensys-hub/essensys-board-SC944D/blob/main/BOM_retouche-foudre.csv), à la racine du dépôt. La BOM d'origine `BOM_[No Variations].csv` n'est pas modifiée.
    - Les variantes des BA seront publiées dans les dépôts `essensys-board-SC940`, `-SC941C` et `-SC942C` (tâche 2.9).

!!! warning "Sécurité"
    - Les retouches se font **hors tension, sur une carte déposée**. Une installation en service est d'abord consignée, y compris la batterie du BP et toute source autonome du site.
    - Avant toute retouche lourde du BP, sa flash est sauvegardée par BDM (voir [Firmware](firmware.md#programmation-du-bp-et-retour-arriere)).
    - Les essais d'injection se font uniquement sur des échantillons de banc, jamais sur une carte destinée à une installation.
    - Le 0 V n'est jamais relié au PE : l'installation reste en TBTS.

### Retouches du BP (SC944D)

| Repère | Origine | Retouche | Objet |
| :--- | :--- | :--- | :--- |
| D33 / D34 | SMF5V0A | **SMF5V0A-E3-08** (remplacement) | Éliminer les TVS fragilisées : fuite ou court-circuit latent |
| U19 | PCA9306 | Remplacé si son test échoue, ou si D33/D34 ont claqué | — |
| L31 | Ferrite 1812 | **PTC CMS 1812 de 30 V au moins** (variante a) | Limiteur réarmable en amont de la TVS de J31-4 |
| DRF1 (ajout) | — | **SMAJ16A** entre J31-4 (cathode) et J31-3, côté soudure, en aval de la PTC ; déjà dans la BOM du BP | Écrêter le +15 V du bus |
| U16 | LD1117 (entrée 15 V max) | **LM317 en SOT-223**, même brochage, R66 = 115 Ω assurant la charge minimale ; ou régulateur dont l'entrée tient au moins 40 V | Supprimer un composant hors marge ; +12VSP = 12,1 V ± 5 % |

**Choix de la PTC du BP.**

- Son courant de maintien à 50 °C vaut au moins 1,5 fois le courant de BA_ALIM, mesuré relais monostables collés.
- Si aucune PTC 1812 de 30 V n'atteint ce courant, on applique la variante (b) : un module sur rail DIN (PTC radiale, puis TVS) est intercalé entre J31 et les BA.
- Un déclenchement de la PTC peut redémarrer le BP.

**Tension de +15VS.** Elle est mesurée sur banc pendant que la SMAJ16A conduit. Au-delà de 24 V, une protection de +15VS est ajoutée ; sinon U2 et C110 sont consignés comme exceptions jusqu'à la révision E.

### Retouches des BA (SC940D, SC941C, SC942C)

| Repère (SC940D / SC941C / SC942C) | Origine | Retouche | Objet |
| :--- | :--- | :--- | :--- |
| R1 / R2 (R1 / R3) | 0 Ω | **47 Ω, 0805 anti-impulsion** (ERJ-P06J470V ou CRCW080547R0FKEAHP) | Borner le courant injecté dans le PIC |
| R79 (R93, R84) | 0 Ω | **PTC de 30 V au moins**, en montage volant | Limiteur réarmable propre au BA |
| Pastilles de C40 (C40, C42) | Non monté | **TVS 16 V** (SMF16A ou SMAJ16A) entre +Vin et 0 V, sur fils courts, **en aval de la PTC** | Écrêter le +15 V du BA |
| C2 | 10 µF / 16 V | **10 µF / 35 V X5R** | Marge sur le rail « +12V » |
| D6 | 20 V | **Schottky 40 V, SOD-323F** | Marge sur le rail « +12V » |

**Pourquoi 47 Ω.**

- La résistance ne ramène pas le PIC sous ses 20 mA : environ 140 mA à 12 V d'écrêtage. Elle transforme toutefois une destruction quasi certaine en survie probable.
- Elle réduit la marge de niveau bas lue par le MCF. Le choix dépend donc de l'écart de masse statique ΔVg, mesuré entre J31-3 et J1-3 du BA le plus éloigné, relais collés :

| ΔVg mesuré | Règle |
| :--- | :--- |
| ≤ 0,1 V | 47 Ω |
| De 0,1 à 0,12 V | 47 Ω si le niveau bas au MCF reste sous 0,85 V ; sinon, liaison équipotentielle d'abord |
| > 0,12 V | Liaison équipotentielle du 0 V (étoile ou renfort, sans jamais relier le PE) avant la remise en service, puis 47 Ω |

**Exception de marge assumée.** U1 (LMR12010) reste en place. Au courant crête, la TVS laisse monter le rail vers 25 V après D3, au-delà de sa tenue. Une défaillance reste alors confinée au BA concerné, grâce à sa PTC propre. La révision E passera à un LMR16006, à C2 en 50 V et à un FI1 de tenue supérieure.

!!! danger "Jamais de TVS sur +VBUS"
    Ne jamais poser la TVS d'alimentation sur +VBUS, en amont de R79. C'est le nœud de chaînage partagé par J1 et J2 : une TVS en court-circuit y couperait tous les BA.

### Option renforcée

L'option renforcée ajoute un écrêteur secondaire et réduit le courant du bus. Elle n'est appliquée que si deux mesures le permettent :

- le temps de montée, mesuré de 0 à 3,45 V au BA le plus éloigné avec les pull-ups d'origine de 1 kΩ ;
- la sortance du MCF : niveau bas émis par le BP, mesuré au PIC du BA le plus éloigné, inférieur à 1,2 V.

| Temps de montée mesuré | Pull-ups du BP (R155/R156) | Résistances série |
| :--- | :--- | :--- |
| ≤ 2,3 µs | 2,2 kΩ | 47 Ω au BP ; 47 Ω au BA, 100 Ω possible (voir ci-dessous) |
| De 2,3 à 2,97 µs | 1,5 kΩ | 47 Ω au BP et 47 Ω au BA |
| Au-delà | Option non appliquée | — |

**Côté BP.**

- R155/R156 passent de 1 kΩ à **2,2 kΩ** (ou 1,5 kΩ selon le tableau).
- L30/L32 sont remplacées par des **47 Ω** en 0603 anti-impulsion (ERJ-P03J470V).
- Une **BAT54S** est montée sur les empreintes C79/C80, côté U19, cathode sur +5VS. U19 reste alors vers 5,8 V au lieu de 9 à 14 V.

**Côté BA.**

- Une **BAT54S** est montée « en l'air » sur C23/C25 (C24/C26 sur le SC941C, C27/C29 sur le SC942C), côté PIC, après R1/R2 : anode au 0 V, cathode au +5 V sur la pastille haute du pull-up.
- R1/R2 restent à 47 Ω. Elles ne passent à **100 Ω** que si quatre conditions sont réunies :
    - le BP est à 2,2 kΩ ;
    - la sortance du MCF est vérifiée ;
    - ΔVg vaut au plus 0,1 V ;
    - le niveau bas au MCF reste sous 0,85 V.

**Écarts consignés jusqu'à la révision E.**

- Un BP retouché **sans** option garde L30/L32 : U19 reste écrêté vers 9 à 14 V par D33/D34.
- Avec l'option, une injection résiduelle d'environ −0,5 V subsiste sur les broches 28/29 du MCF, dont la limite est de −0,3 V.

### Vérifications après retouche

| Carte | Contrôle | Critère |
| :--- | :--- | :--- |
| BA | Hors tension, entre J1-1 et RC7, puis J1-2 et RC6 | 44 à 50 Ω (95 à 105 Ω pour 100 Ω) |
| BA | Carte seule sous 15,0 V | +5 V entre 4,83 et 5,03 V ; écart entre le +5 V et chaque ligne d'au plus 0,5 V |
| BP | J31 débranché | Écart entre +5VS et chaque ligne d'au plus 50 mV ; EN de U19 entre 3,6 et 4,4 V ; test de U19 dans les deux sens |
| Bus | Sur l'installation, relais collés | Niveau bas au MCF < 0,85 V et au PIC < 1,2 V ; temps de montée < 4 µs ; BA_ALIM ≥ 14 V au dernier BA |

**Étiquetage.**

- Chaque BA retouché porte l'étiquette « retouche foudre », avec la valeur de sa résistance série et l'option éventuelle.
- Le BP porte « retouche foudre », avec l'option ou la mention « sans option ».

### Cohabitation des cartes

| Configuration | Règle |
| :--- | :--- |
| BA d'origine (0 Ω) | Compatible avec le bus actuel ; à retoucher à sa prochaine dépose |
| BA retouché à 47 Ω | Compatible avec un BP d'origine ou retouché ; niveaux vérifiés après retouche |
| BA à 100 Ω (option renforcée) | Seulement avec un BP renforcé à 2,2 kΩ. Revenir à 47 Ω si le BP perd son option |
| BA préparé pour un module d'isolement (D1/D2 déposées) | Uniquement derrière son module d'isolement ; jamais raccordé directement au bus |
| BP retouché sans option | Compatible avec tous les BA à 47 Ω ; risque résiduel sur U19 consigné |

### À ne pas faire

- **Monter L3 seule, ou retirer R79/R80 des BA.** Le retour de SDA et SCL passe par GNDBUS. L3 y ajouterait environ 314 Ω à 50 kHz.
- **Monter 100 Ω côté BA avec les pull-ups de 1 kΩ**, ou ajouter une résistance côté BP sans changer les pull-ups : la marge de niveau bas au MCF disparaît.
- **Monter des TVS 5 V « plus puissantes »** (SMBJ5.0A, SMF4L5.0A) : leur fuite peut atteindre 800 µA, soit environ 2,5 V perdus sur le niveau haut.
- **Monter des condensateurs sur C23/C25.** Avec 47 à 100 Ω, la constante de temps n'est que de 1 à 2 ns : aucun effet contre la foudre.
- **Poser un éclateur à gaz entre le 0 V et la terre**, ou toute autre liaison 0 V–PE : l'installation sortirait de la TBTS.
- **Poser un parafoudre RJ45 relié au PE côté BP.** Il injecterait le potentiel du PE dans le 0 V du BP. L'Ethernet du BP est isolé galvaniquement : fibre ou isolateur.

### Suite : interconnexion et révision E

La retouche ne relève pas, à elle seule, le courant d'équilibrage que supporte un coffret. Le change prévoit aussi :

- une étoile 0 V entre les cartes d'un même coffret, sans jamais relier le PE ;
- l'isolement galvanique de l'Ethernet du BP ;
- une révision E des cartes, qui suit les [règles de conception](rev-e-regles-conception.md) ;
- un module d'isolement pour les BA déportés.
