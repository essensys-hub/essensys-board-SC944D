# Révision E : règles de conception communes

!!! warning "Statut : brouillon. Saisie dans la CAO et relecture à faire"
    Ce document est le livrable rédactionnel de la tâche 6.1 du change OpenSpec `i2c-surge-protection-2026-10-007`. **Les règles ci-dessous ne sont pas encore saisies dans les projets de CAO, et le document n'a pas encore été relu.** Restent à faire :

    - le calcul de la distance d'isolement renforcé (§ 3.3) ;
    - la saisie des règles dans les projets Altium Designer 16 des SC940D, SC941C, SC942C et SC944D, puis dans l'outil retenu pour la carte d'interconnexion et le MIB ;
    - la relecture, tracée sur la sub-issue [#7](https://github.com/essensys-hub/essensys-board-SC944D/issues/7).

    Les valeurs marquées *proposé* sont des points de départ. La relecture les confirme ou les corrige avant la saisie.

| | |
|---|---|
| Feature | `i2c-surge-protection-2026-10-007` |
| Ticket | [essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1) |
| Sources | `openspec/changes/i2c-surge-protection-2026-10-007/` : `design.md` (D1 à D6), specs `i2c-bus-surge-protection` et `aux-box-interconnect` |
| Cartes concernées | SC940D, SC941C, SC942C (BA), SC944D (BP), carte d'interconnexion, MIB |
| Décision liée | [ADR 0001 : génération 3 en RS-485 isolé](adr/0001-generation-3-rs485-isole.md) |

## 1. Objet

Le 9 août 2026, une surtension a détruit un boîtier auxiliaire (BA) d'une installation touchée, et bloqué tout le bus I2C entre le boîtier principal (BP) et les BA. L'analyse a montré des défauts communs aux cartes révision D :

- les îlots TBTS ne sont reliés que par le câble de bus ;
- rien ne limite le courant injecté dans les circuits intégrés ;
- le +15 V du bus n'est pas protégé ;
- la distance entre le 230 V et la TBT n'est pas démontrée près de J1/J2.

La révision E corrige ces défauts dans le dessin des cartes. Ce document fixe les règles communes à toutes les cartes. La conception propre à chaque carte relève des tâches 6.2 (carte d'interconnexion), 6.4 (BA), 6.5 (BP) et 6.7 (MIB).

**Principes.**

1. Dans un coffret, rendre les 0 V équipotentiels : plot 0 V sur chaque carte, puis carte d'interconnexion.
2. Sur chaque carte, dévier le courant de surtension avant les circuits intégrés, et hors du plan logique.
3. Limiter le courant que reçoit chaque broche de circuit intégré.
4. Garantir par construction l'isolement entre 230 V et TBT.
5. Ne jamais relier le 0 V au conducteur de protection (PE) : la TBTS est conservée.

**Vocabulaire.** « DOIT » et « NE DOIT PAS » traduisent les SHALL et MUST NOT des specs. « Recommandé » laisse un choix argumenté au concepteur.

**Invariants de compatibilité.** Une carte révision E DOIT conserver :

- le brochage de J1/J2 (BA) et de J31 (BP) : 1 SDA, 2 SCL, 3 0 V, 4 +15 V ;
- les adresses 0x11 (SC940D), 0x12 (SC942C) et 0x13 (SC941C) ;
- le bus à 50 kHz ;
- les critères de niveau de la spec `i2c-bus-surge-protection`, vérifiés par `NR-board-sc944d-1` : niveau bas lu au MCF < 0,85 V, niveau bas lu au PIC < 1,2 V, montée de 0 à 3,45 V < 4 µs, fuite ≤ 50 µA par ligne ;
- le protocole HTTP legacy IoT, que le matériel ne touche pas.

## 2. Zones

Chaque carte est découpée en trois zones, dessinées sur une couche mécanique dédiée et reprises par des *rooms* dans la CAO (§ 9).

| Zone | Contenu | Masse de référence |
|---|---|---|
| **Z1 « bus »** | Connecteurs de bus (J1/J2 d'un BA, J31 du BP, ports de la carte d'interconnexion), écrêteurs primaires (TVS) de ligne, premières résistances série, limiteur réarmable (PTC) et écrêteur du +15 V, plot 0 V. Sur une carte à isolation intégrée : côté bus de l'isolateur et entrée du convertisseur isolé. | `GND_BUS` |
| **Z2 « TBTS locale »** | Circuits intégrés et logique (PIC16F946, MCF52259, U19, régulateurs), écrêteurs secondaires et résistances de broche, ports TBT locaux (J3 et entrées TOR, liaison IHM, Ethernet côté logique). | Plan logique `GND` |
| **Z3 « 230 V »** | Contacts des relais, varistances (VR1…), variateurs, borniers et pistes reliés au réseau. | Aucune (230 V) |

**Règles.**

- Chaque composant appartient à une seule zone. Seuls les **composants de barrière** (relais, optocoupleurs, transformateur Ethernet, isolateur, convertisseur isolé) ont des broches dans deux zones (§ 3.4).
- Z1 est placée au bord de la carte, autour des connecteurs de bus. Elle DOIT être éloignée de Z3 d'au moins la distance du § 3.2 : sur les cartes révision D, J1/J2 sont estimés à 2 ou 3 mm du relais K1 et de VR1.
- Aucune piste d'une zone ne traverse une autre zone, sauf par les chemins définis aux § 4 à 6.

## 3. Distances entre zones

### 3.1 Entre Z1 et Z2

**Carte à isolation intégrée** (isolateur et convertisseur isolé sur la carte, comme le MIB ou une carte de génération 3) :

- au moins **8 mm sans cuivre, sur toutes les couches**, entre Z1 et Z2 ;
- aucun cuivre, aucune via et aucune piste sous l'isolateur et sous le convertisseur, hors de leurs propres pastilles ;
- barrière renforcée d'au moins 4 kVAC (spec `aux-box-interconnect`, exigence « Module d'isolement pour BA déportés ») ;
- un condensateur de barrière (1 nF au plus) seulement si les émissions l'exigent : il laisse passer environ 2 A pour 1 nF à 2 kV/µs.

**Carte non isolée** (BA et BP révision E sans isolateur intégré) : Z1 et Z2 partagent le 0 V, et aucune distance n'est imposée entre elles. En revanche :

- SDA et SCL ne passent de Z1 à Z2 **que** par le chemin protégé du § 4 ;
- le +15 V ne passe de Z1 à Z2 que par le chemin protégé du § 5 ;
- le 0 V ne passe de Z1 à Z2 que par le point de liaison unique du § 6.2 ;
- aucun autre conducteur ne franchit la frontière, et aucun signal de Z2 n'est routé sous Z1.

### 3.2 Entre Z2 et Z3, et entre Z1 et Z3

Entre tout conducteur 230 V et tout conducteur TBT (Z1 ou Z2), la carte DOIT présenter :

- au moins **D_iso = max(6 mm ; distance d'isolement renforcé)**, la distance d'isolement renforcé étant exigée par l'EN 60664-1 et par la norme produit applicable ;
- sur **toutes les couches**, externes et internes ;
- une **fente fraisée** le long de la frontière, partout où des conducteurs 230 V et TBT se font face.

**Changement par rapport à la révision D.** Les projets des SC940D, SC941C, SC942C et SC944D définissent une classe de nets `BT` et deux règles : `Clearance_BT_EXT` (5,5 mm, couches externes) et `Clearance_BT_MID` (0,7 mm, couches internes). Les cartes ont quatre couches. En révision E, une règle unique s'applique à toutes les couches avec la valeur D_iso, et la règle à 0,7 mm sur les couches internes disparaît.

**Fente.**

- Largeur d'au moins 1 mm, valeur X du degré de pollution 2 de l'EN 60664-1 (à confirmer à la relecture). Valeur *proposée* : 2 mm, pour la fabrication.
- La ligne de fuite qui contourne la fente par ses extrémités DOIT rester supérieure ou égale à D_iso.
- La fente allonge la ligne de fuite. Elle ne change pas la distance dans l'air, qui DOIT respecter à elle seule la valeur calculée.

Le vernis et la sérigraphie ne comptent pas comme isolation.

Pour les BA, J1/J2 DOIVENT se trouver à D_iso au moins de K1 et des varistances, avec une fente (tâche 6.4).

### 3.3 Distance d'isolement renforcé : calcul à faire

La valeur exacte (6 ou 8 mm) reste une question ouverte du design. Le calcul retient :

- la tension de service : réseau 230/400 V ;
- la catégorie de surtension : III pour un appareil installé dans le tableau (à confirmer) ;
- le degré de pollution : 2 ;
- le groupe de matériau du stratifié, d'après l'indice CTI donné par le fabricant de circuits imprimés ;
- une altitude inférieure ou égale à 2000 m.

La distance dans l'air et la ligne de fuite se calculent séparément, en isolation renforcée. La norme produit peut imposer davantage. Le résultat est consigné sur la sub-issue #7.

**Valeur à saisir dans la DRC en attendant le calcul : 8,0 mm**, la plus contraignante des deux valeurs candidates. Si le placement ne permet pas 8 mm, le calcul est fait avant toute réduction. La valeur saisie ne descend jamais sous 6,0 mm.

### 3.4 Composants de barrière

- Les relais (bobine en Z2, contacts en Z3), les optocoupleurs, le transformateur Ethernet, l'isolateur et le convertisseur isolé DOIVENT être spécifiés en **isolation renforcée** entre leurs deux côtés. Leur fiche technique doit couvrir la tension de service et la catégorie de surtension retenues.
- Leur empreinte place la frontière entre les deux groupes de broches, et la fente sous le corps du composant quand l'écart entre broches le permet.
- La DRC reçoit une exception **par composant** (§ 9.2, règle R2). Elle porte uniquement sur les broches de ce composant, et sa valeur est la distance que garantit le fabricant. L'exception est justifiée dans la relecture.

## 4. Ordre des protections sur une ligne de bus

### 4.1 Chaîne type

Chaque ligne SDA et SCL DOIT être protégée dans cet ordre, depuis le connecteur :

```
connecteur ── TVS ── Rs1 ──┬── Rs2 ── circuit intégré
   (Z1)        │    (Z1)   │   (Z2)
               │      écrêteur secondaire (Z2)
           GND_BUS     vers +V et 0 V du circuit intégré
```

- **TVS** : écrêteur primaire, vers `GND_BUS`.
- **Rs1** : résistance de ligne, anti-impulsion.
- **Écrêteur secondaire** : double diode Schottky (BAT54S), cathode sur l'alimentation du circuit intégré, anode sur son 0 V.
- **Rs2** : résistance de broche, entre l'écrêteur secondaire et le circuit intégré.

**Règles.**

- Entre le connecteur et la TVS, il n'y a que du cuivre : aucun autre composant n'est raccordé à ce segment.
- La TVS est à **10 mm de piste au plus** de la broche du connecteur. Sur les cartes révision D, D1/D2 sont à plusieurs centimètres de J1/J2.
- Caractéristiques de la TVS de ligne :
    - capacité de 50 pF au plus ;
    - tension de veille (VRWM) de 5 V au moins ;
    - fuite telle que le total des écrêteurs reste à 50 µA au plus par ligne et par carte, sous 5 V et jusqu'à 50 °C ;
    - Ipp en onde 8/20 de 5 à 10 A au moins (*proposé*) ;
    - boîtier soudable à la main (SOD-323 ou SOD-123).
- Rs1 et Rs2 sont des résistances anti-impulsion (familles ERJ-P ou CRCW-HP).
- Rs2 limite le courant de broche à **20 mA au plus**, pour une ligne portée à +12 V puis à −2,5 V au connecteur. Ce courant se mesure sur un échantillon de banc instrumenté (shunt entre l'écrêteur secondaire et la broche), ou s'estime par V(broche) − VDD sur une courbe de diode caractérisée.

### 4.2 Sur un BA (PIC16F946)

- Rs1 correspond à R1/R2 du schéma (R1/R3 sur le SC941C), qui valent aujourd'hui 0 Ω.
- Le **niveau bas** borne la somme Rs1 + Rs2. Avec les pull-ups du BP à 2,2 kΩ, la somme ne dépasse pas **100 Ω** (*proposé*, même borne que l'option renforcée du design D2). La répartition, par exemple 47 Ω + 47 Ω, est fixée par le calcul du courant de broche. Elle est ensuite vérifiée par `NR-board-sc944d-1` et sur un échantillon instrumenté.
- **Point ouvert pour la relecture.** Un BA dont Rs1 + Rs2 dépasse 47 Ω n'est compatible qu'avec un BP dont les pull-ups sont à 2,2 kΩ (BP révision E, ou BP retouché en option renforcée). Avec les pull-ups de 1 kΩ d'un BP révision D, environ 94 Ω en série porte le niveau bas lu au MCF vers 0,9 V, au-delà de 0,85 V. Il faut soit l'inscrire dans le tableau de cohabitation, soit prévoir une option de BOM à 47 Ω.

### 4.3 Sur le BP (MCF52259 et U19)

Chaîne de chaque ligne, de J31 vers le MCF :

1. TVS de 50 pF au plus, à la place de D33/D34 (1120 pF) ;
2. résistance série de 47 Ω : la résistance mesurée hors tension entre J31 et U19 est de 44 à 50 Ω ;
3. écrêteur secondaire (BAT54S, cathode sur +5VS), côté U19 ;
4. U19 (PCA9306) ;
5. résistance série vers le MCF (33 Ω au plus, *proposé*) ;
6. protection **au pied du MCF**, à 10 mm au plus des broches 28/29 (*proposé*). En révision D, cette piste de 55 mm n'a aucune protection ;
7. MCF52259, broches 28 (SCL) et 29 (SDA).

Les pull-ups R155/R156 passent à 2,2 kΩ.

Pendant une injection, les limites de la spec s'appliquent :

- côté bus de U19 : au plus +7,0 V, et au moins −0,5 V (ou courant d'entrée de 50 mA au plus en valeur absolue) ;
- broches I2C du MCF : au plus 4,0 V, et au moins −0,3 V.

### 4.4 Cartes sans circuit intégré sur la ligne

La carte d'interconnexion n'a pas de circuit intégré sur SDA et SCL. Par port, elle porte une TVS de 50 pF au plus vers le plan 0 V, à 10 mm au plus du connecteur, **sans résistance série**. Une résistance de port s'ajouterait à R1 du BA : un BA retouché présenterait 94 Ω en série, soit environ 0,94 V au MCF avec les pull-ups de 1 kΩ. Les traversées IDC 26 points des nappes J3 portent une TVS par entrée vers le plan 0 V.

### 4.5 Placement et routage des lignes

- Le segment entre le connecteur et Rs1, TVS comprise, reste sur une couche externe, sans changement de couche, et mesure 20 mm au plus (*proposé*).
- Après Rs1, SDA et SCL sont routées de préférence en couche interne, entre plans de masse, avec une garde de masse (recommandé).
- SDA et SCL ne sont jamais parallèles aux commandes de relais ni aux pistes 230 V.
- L'écrêteur secondaire et Rs2 sont placés près du circuit intégré : 10 mm au plus de la broche (*proposé*).

## 5. Alimentation +15 V du bus

Chaîne, depuis le connecteur :

```
J1-4 ∥ J2-4 (nœud de chaînage) ── PTC ── TVS 16 V ── filtre ── régulateur
                                          │
                                       GND_BUS
```

**Règles.**

- Limiteur réarmable (PTC) de 30 V au moins, dont le courant de maintien à 50 °C vaut au moins 1,5 fois le courant mesuré de la carte, relais monostables collés. Sur le BP, le +15 V de J31-4 passe par un eFuse à démarrage progressif.
- La TVS d'alimentation (16 V, SMCJ16A de référence) a une VRWM au moins égale à la tension nominale maximale du nœud. Elle est placée **en aval** du limiteur, jamais en amont : une TVS d'alimentation finit en court-circuit.
- Ni la PTC ni la TVS ne sont placées sur le **nœud de chaînage partagé** (+VBUS entre J1 et J2) : une TVS morte y couperait tous les BA en aval.
- La TVS revient vers `GND_BUS` par une piste de 5 mm au plus, ou par des vias directes (*proposé*).
- Tout composant en aval de la TVS a une tension assignée au moins égale à la tension d'écrêtage maximale de cette TVS. La tension assignée est la tension maximale recommandée en fonctionnement, et non le maximum absolu. En pratique :
    - BA : régulateur LMR16006 (4 à 60 V), C2 en 50 V, D6 en 40 V, FI1 de tenue adaptée ;
    - BP : régulateur de +12VSP et U2 avec une entrée d'au moins 40 V.
- La piste +15 V de chaînage (J1-4 ↔ J2-4) porte le courant de tous les BA en aval : 1,5 mm au moins en cuivre de 35 µm (*proposé*).

## 6. Masses, retour des TVS et plot 0 V

### 6.1 `GND_BUS`

- `GND_BUS` est un **net distinct** du schéma. Il réunit :
    - les broches 0 V des connecteurs de bus (J1-3 et J2-3, J31-3, ports de la carte d'interconnexion) ;
    - le plot 0 V ;
    - les retours des TVS de ligne et de la TVS +15 V.
- Il forme un plan sous Z1, sur les deux couches externes, cousu par des vias au pas de 5 mm au plus (*proposé*). Les pastilles des TVS et les broches 0 V des connecteurs y sont reliées sans réserve thermique (*proposé*).
- Sur un BA, le chemin J1-3 ↔ J2-3 ↔ plot 0 V reste entièrement dans `GND_BUS`. Le courant de retour des BA en aval et le courant d'équilibrage ne traversent pas le plan logique.

### 6.2 Courant de surtension hors du plan logique

- `GND_BUS` et le plan logique `GND` sont reliés en **un seul point**, par un composant de liaison (*net tie*) NT1. NT1 est placé à la frontière entre Z1 et Z2, au droit des écrêteurs secondaires : le petit courant qu'ils dérivent (borné par Rs1) rejoint ainsi `GND_BUS` sans traverser le plan logique.
- Aucune autre liaison entre `GND_BUS` et `GND`. Le plan logique ne s'étend pas sous Z1.
- **Ports locaux** (J3 et entrées TOR, capteurs extérieurs et contact de porte du BP) : leurs TVS reviennent vers une bande de retour de port, reliée à `GND_BUS` par un chemin de cuivre direct qui ne passe pas par le plan logique. *Recommandation à confirmer en relecture.* En révision D, chaque TVS d'entrée renvoie son courant dans le 0 V qui sert aussi de référence au bus.

### 6.3 Plot 0 V

- Sur un BA, le plot est placé près de J1/J2. Sur le BP, il est placé près de J31. Distance *proposée* : 20 mm au plus du connecteur.
- C'est un trou métallisé pour vis M3 (*proposé*, à harmoniser avec la carte d'interconnexion), avec une pastille sur les deux faces et une couronne de vias vers `GND_BUS`.
- Il est en Z1, à D_iso au moins de tout conducteur de Z3.
- La sérigraphie porte : « 0 V TBTS – NE PAS RELIER À LA TERRE ».
- Ce n'est pas un trou de fixation. Il NE DOIT PAS être relié au PE, au rail DIN ni à une pièce métallique du coffret. Le conducteur qui le relie à la carte d'interconnexion est isolé, et n'est ni vert-jaune ni bleu clair.
- Le plot conditionne les seuils de la carte d'interconnexion : environ 875 A en onde 8/20 et 292 A sur un front de 1 Ipk/µs, pour une exigence de 800 A et 250 A (spec `aux-box-interconnect`). Par J31 seul, ces valeurs sont hors d'atteinte.

### 6.4 Carte à isolation intégrée

Il y a un plan par îlot, sans recouvrement entre îlots : `GND_BUS` en Z1, `GND` en Z2, et aucune liaison NT1 entre eux. La distance du § 3.1 s'applique.

## 7. Règles propres au BP : ports qui sortent de la carte

- J31 et le RJ45 J32 sont placés sur des **bords opposés**. En révision D, ils sont à 18 mm l'un de l'autre.
- J32 est éloigné de J31 et de J53 (contact de porte), qui partent aujourd'hui du même coin.
- Ethernet : magnétiques isolés de 2,25 à 4 kV et capacité de terminaison Bob Smith de 3 kV au moins, ou Ethernet isolé intégré.
- Aucune carte ne prévoit de liaison entre le 0 V et le PE : ni condensateur Y, ni varistance, ni éclateur entre une zone TBT et une borne PE.
- L'éventuel écran d'un câble Ethernet blindé n'est pas raccordé côté BP (spec `aux-box-interconnect`). Aucun parafoudre relié au PE n'est posé sur le port Ethernet du BP.

## 8. Règles de faisceau

Ces règles complètent le dessin des cartes. Les règles d'installation sont validées par un électricien avant leur publication dans le guide d'installation (tâche 8.4).

**Dans un coffret.**

- Chaque liaison de bus mesure 0,5 m au plus.
- Le +15 V et le 0 V forment une paire dédiée. SDA et SCL sont chacune torsadées avec un 0 V de signal.
- SCL n'est **jamais** torsadée avec le +15 V : l'ondulation s'y couplerait, et un écrasement du câble porterait 15 V sur SCL.
- Le câble de bus et les nappes ne partagent jamais une goulotte avec des conducteurs 230 V. Ils sont sous un manchon isolant là où ils longent des bornes de phase.
- Le cordon Ethernet du BP sort directement du coffret.
- Le 0 V est renforcé par l'étoile 0 V (N2), par la carte d'interconnexion reliée aux plots 0 V, ou à défaut par le renfort N1.
- Faisceaux de N2 : AWG20, 15 cm au plus, sertis à l'outil, moins de 60 mΩ par conducteur, contacts compris.
- Tout renfort de 0 V est isolé, n'est ni vert-jaune ni bleu clair, porte à chaque extrémité le repère « 0 V TBTS – NE PAS RELIER À LA TERRE », et suit le câble de bus en lui restant plaqué.
- Le coffret domotique ne comporte ni borne ni raccordement PE. Le PE d'un départ 230 V qui le traverse reste gainé avec son circuit, dans les goulottes 230 V.

**Pour un BA déporté** (hors du coffret du BP, ou liaison de plus de 0,5 m) :

- raccordement par un MIB, avec 3 m au plus de câble AWG20. Au-delà, un tampon de bus qualifié ou la génération 3 ;
- connecteur côté isolé d'une autre série, et J2 du BA neutralisé, avec l'étiquette « ne pas raccorder sans module » ;
- écran éventuel : règle à fixer avec le MIB (tâche 6.7). Il n'est jamais relié au 0 V, ni raccordé dans le coffret domotique.

## 9. Saisie dans la CAO (Altium)

Cette table traduit chaque règle en contrainte de CAO. Les requêtes suivent la syntaxe des règles existantes des projets révision D (`InNetClass`, `IsElectrical`, `InComponent`). Les valeurs en mil sont données pour la saisie : Altium les exporte en mil dans les fichiers `.RUL`.

### 9.1 Classes et rooms

| Objet | Nom | Contenu |
|---|---|---|
| Classe de nets | `BT` (existante) | Tous les nets TBT, Z1 et Z2. Convention héritée à conserver : **tout net hors de `BT` est traité comme 230 V**, si bien qu'un net oublié reçoit la distance la plus grande. |
| Classe de nets | `Z1_BUS_SIG` | SDA et SCL, du connecteur jusqu'à Rs1 inclus (TVS comprise). |
| Classe de nets | `Z1_BUS_PWR` | +15 V du connecteur jusqu'à l'entrée du régulateur (nœud de chaînage, aval de la PTC). |
| Classe de nets | `GND_BUS` | 0 V du bus (§ 6.1). |
| Classe de nets | `Z2_I2C` | SDA et SCL, de Rs1 jusqu'au circuit intégré. |
| Classe de nets | `RELAIS_CMD` | Commandes TBT des relais et des variateurs. |
| Classes de nets | `ISO_BUS`, `ISO_LOC` | Cartes à isolation intégrée seulement : tous les nets de Z1, tous les nets de Z2. |
| Classes de composants | `CC_Z1`, `CC_Z2`, `CC_Z3` | Composants de chaque zone. |
| Classe de composants | `CC_TVS_<connecteur>` | TVS de ligne et TVS +15 V d'un connecteur de bus (par exemple `CC_TVS_J1`). |
| Classe de composants | `CC_BARRIERE` | Composants de barrière (§ 3.4). |
| Rooms | `ROOM_Z1`, `ROOM_Z2`, `ROOM_Z3` | Contours des zones. |
| Rooms | `ROOM_TVS_<connecteur>` | Polygone dont tout point est à 10 mm au plus des broches 1, 2 et 4 du connecteur. |
| Room | `ROOM_P0V` | Polygone à 20 mm au plus du connecteur de bus (*proposé*). |

### 9.2 Règles

| N° | Règle (§) | Type de règle Altium | Portée | Valeur | Remarque |
|---|---|---|---|---|---|
| R1 | Isolement 230 V / TBT (§ 3.2) | Electrical › Clearance | `IsElectrical and InNetClass('BT')` contre `IsElectrical and not InNetClass('BT')`, **toutes couches** | 8,0 mm (315 mil) en attendant le calcul du § 3.3 ; jamais moins de 6,0 mm (236 mil) | Remplace `Clearance_BT_EXT` (5,5 mm) et supprime `Clearance_BT_MID` (0,7 mm). Priorité supérieure aux règles de clearance par défaut. Les exceptions de portée héritées (par exemple `HasPad('K*-12')`) sont revues une à une. |
| R2 | Composant de barrière (§ 3.4) | Electrical › Clearance | `InComponent('<réf>') and InNetClass('BT')` contre `InComponent('<réf>') and not InNetClass('BT')`, **une règle par composant** | Distance garantie par la fiche technique | Priorité supérieure à R1. Justification consignée dans la relecture. |
| R3 | Z1 / Z2 isolées (§ 3.1) | Electrical › Clearance | `InNetClass('ISO_BUS')` contre `InNetClass('ISO_LOC')`, toutes couches | 8,0 mm (315 mil) | Cartes à isolation intégrée seulement. |
| R4 | Rien sous l'isolateur ni sous le convertisseur (§ 3.1) | Région de keep-out sur toutes les couches, découpes de polygone | Emprise de l'isolateur et du convertisseur | Aucun cuivre hors de leurs pastilles | Complétée par un contrôle visuel en relecture. |
| R5 | Fente (§ 3.2) | Découpe de carte (*board cutout*) | Frontières Z1/Z3 et Z2/Z3, et sous les composants de barrière | Largeur de 1 mm au moins ; 2 mm *proposés* | La clearance cuivre–contour existante s'applique aux bords de la fente. Présence et contournement contrôlés en relecture. |
| R6 | Zones (§ 2) | Placement › Room Definition (garder à l'intérieur) | `InComponentClass('CC_Z1')` dans `ROOM_Z1`, de même pour Z2 et Z3 | — | Les composants de barrière sont placés à cheval sur la frontière, hors de cette règle. |
| R7 | TVS à 10 mm au plus du connecteur (§ 4.1) | Placement › Room Definition (garder à l'intérieur) | `InComponentClass('CC_TVS_<connecteur>')` dans `ROOM_TVS_<connecteur>` | 10 mm | Une room par connecteur de bus (J1, J2, J31, ports). |
| R8 | Segment connecteur → Rs1 (§ 4.5) | High Speed › Length | `InNetClass('Z1_BUS_SIG')` | 20 mm au plus (*proposé*) | — |
| R9 | `GND_BUS` à faible impédance (§ 6.1) | Routing › Width | `InNetClass('GND_BUS')` | 2,0 mm au moins (*proposé*) | Le plan de `GND_BUS` est un polygone. |
| R10 | Liaison directe à `GND_BUS` (§ 6.1) | Plane › Polygon Connect Style | `InNetClass('GND_BUS')` | Connexion directe (*proposé*) | Pas de réserve thermique sur les TVS ni sur les broches 0 V des connecteurs. |
| R11 | +15 V de chaînage (§ 5) | Routing › Width | `InNetClass('Z1_BUS_PWR')` | 1,5 mm au moins (*proposé*) | — |
| R12 | Liaison unique `GND_BUS` – `GND` (§ 6.2) | Electrical › Short-Circuit | Court-circuit autorisé seulement pour `InComponent('NT1')` | — | La règle `ShortCircuit` existante (aucun court-circuit autorisé) reste valable partout ailleurs. NT1 est un composant de type *net tie*. |
| R13 | Pas de parallélisme avec les commandes de relais (§ 4.5) | High Speed › Parallel Segment | `InNetClass('Z1_BUS_SIG') or InNetClass('Z2_I2C')` contre `InNetClass('RELAIS_CMD')`, même couche et couches adjacentes | 5 mm de parallélisme au plus, pour un écart inférieur à 2 mm (*proposé*) | — |
| R14 | Plot 0 V (§ 6.3) | Placement › Room Definition (garder à l'intérieur) | `InComponent('P0V')` dans `ROOM_P0V` | 20 mm au plus du connecteur (*proposé*) | La distance à Z3 est couverte par R1. |
| R15 | BP : J32 loin de J31 et de J53 (§ 7) | Placement › Component Clearance | `InComponent('J32')` contre `InComponent('J31') or InComponent('J53')` | 40 mm au moins (*proposé*) | À ajuster à la taille de la carte. J31 et J32 sont en plus placés dans deux rooms de bord opposées. |
| R16 | BP : protection au pied du MCF (§ 4.3) | Placement › Room Definition (garder à l'intérieur) | Composants de protection de PAS0/PAS1 dans une room à 10 mm au plus des broches 28/29 | 10 mm (*proposé*) | — |

### 9.3 Contrôles hors DRC (relecture)

Ces points ne se traduisent pas en règle native. Ils sont vérifiés à la relecture, puis sur les Gerber :

- présence de la fente, et ligne de fuite qui la contourne (§ 3.2). Si la version d'Altium utilisée dispose d'une règle de ligne de fuite, elle est saisie avec la valeur D_iso ;
- retour de chaque TVS vers `GND_BUS` en 5 mm au plus, ou par vias directes (§ 4.1 et § 5) ;
- absence du plan logique `GND` sous Z1, et liaison `GND_BUS` – `GND` par NT1 seulement (§ 6.2) ;
- fiches techniques des composants de barrière : isolation renforcée (§ 3.4) ;
- routage de `Z2_I2C` en couche interne avec garde de masse (§ 4.5, recommandé) ;
- revue de BOM :
    - TVS de ligne : 50 pF au plus, VRWM, fuite ;
    - PTC : tension, courant de maintien à 50 °C ;
    - tensions assignées en aval de la TVS +15 V ;
    - valeurs de Rs1 et Rs2, pull-ups du BP ;
- sérigraphie du plot 0 V ;
- contrôle des distances sur les Gerber (vérification de la tâche 6.4).

## 10. Traçabilité

| Règle | Exigence (spec) | Vérification |
|---|---|---|
| Distances Z2/Z3 et Z1/Z3, fente (§ 3.2) | `i2c-bus-surge-protection`, « Compatibilité TBTS » (scénario « Distance d'isolement en révision E ») | Gerber (tâches 6.4 et 6.5) ; essai d'isolement à 500 V continu, 100 MΩ au moins |
| Barrière Z1/Z2 de 8 mm (§ 3.1) | `aux-box-interconnect`, « Module d'isolement pour BA déportés » | Gerber du MIB (tâche 6.7) |
| Ordre des protections, courant de broche (§ 4) | `i2c-bus-surge-protection`, « Courant injecté dans les broches I2C borné » et « Écrêtage coordonné des lignes SDA et SCL » | Échantillon instrumenté (tâche 6.4) ; injections de la tâche 7.16 |
| Chaîne du BP (§ 4.3) | Idem : MCF à −0,3 V au moins, 44 à 50 Ω entre J31 et U19 | Tâche 6.5 ; `NR-board-sc944d-1` |
| Niveaux et compatibilité (§ 1, § 4.2) | « Niveaux I2C conservés à 50 kHz » | `NR-board-sc944d-1` |
| +15 V (§ 5) | « Protection de l'alimentation +15 V du bus » et « Marges de tension des composants d'alimentation » | `NR-board-sc944d-9` ; revue de BOM |
| `GND_BUS` et plot 0 V (§ 6) | `aux-box-interconnect`, « 0 V équipotentiels à basse impédance dans un coffret » (800 A et 250 A) | Tâche 6.6 |
| Défaillance d'un élément de protection en premier | « Défaillance maîtrisée sous contrainte excessive » | Tâche 7.19 |
| Faisceau (§ 8) | `aux-box-interconnect`, « Câblage du bus dans un coffret » et « Aucune liaison au PE et repérage du 0 V » | Contrôle du coffret ; guide d'installation (tâche 8.4) |

Tous les essais se font **sur banc**, sur des échantillons dédiés, jamais sur une armoire en service.

## 11. Reste à faire

- [ ] Calculer la distance d'isolement renforcé (§ 3.3), et la consigner sur #7.
- [ ] Relire ce document, et trancher les valeurs *proposées* ainsi que le point ouvert du § 4.2.
- [ ] Saisir les classes, les rooms et les règles R1 à R16 dans les projets SC940D, SC941C, SC942C et SC944D, puis exporter les fichiers `.RUL`.
- [ ] Reprendre les mêmes règles dans le projet de la carte d'interconnexion et dans celui du MIB, une fois leur outil de CAO fixé.
- [ ] Faire relire la saisie, avec un ERC et une DRC sans erreur, et tracer la relecture sur #7.
