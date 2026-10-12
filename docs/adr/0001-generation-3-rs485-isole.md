# ADR 0001 : bus BP ↔ BA de génération 3 en RS-485 isolé

| | |
|---|---|
| Statut | **Proposé** |
| Date | 2026-10-11 |
| Feature | `i2c-surge-protection-2026-10-007` (tâche 6.8) |
| Ticket | [essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1), sub-issue [#7](https://github.com/essensys-hub/essensys-board-SC944D/issues/7) |
| Décideurs | Mainteneurs d'essensys-hub, par la relecture et le merge de cet ADR |
| Portée | Génération 3 uniquement. Ce change ne réalise rien de cette décision. |

## Statut

**Proposé.** L'ADR passe à « Accepté » après relecture et merge, ce qui est la vérification de la tâche 6.8. Il ne modifie ni les cartes révision D ou E, ni les firmwares en service, ni le protocole HTTP legacy IoT.

## Contexte

### Le bus actuel

- **Maître.** Le boîtier principal SC944D (BP) pilote le bus avec un MCF52259, par le module I2C0 sur les broches 28 (SCL) et 29 (SDA), à travers le traducteur de niveau U19 (PCA9306) et le connecteur J31.
- **Esclaves.** Les boîtiers auxiliaires (BA) SC940D, SC942C et SC941C, aux adresses 0x11, 0x12 et 0x13, utilisent le module SSP esclave du PIC16F946 sur RC6 (SCL) et RC7 (SDA).
- **Câblage.** Le bus tourne à 50 kHz. Il est chaîné par J1/J2, qui portent aussi le +15 V et le 0 V.
- **Trames.** Le BP écrit CMD, DATA et un CRC-16 de 2 octets. Il relit ensuite 5 octets (écho de CMD, écho du CRC, CRC de la réponse), ou 6 avec l'octet `comm_stat` du firmware BA 1.8.

### Ce que l'événement a montré

Une installation touchée le 9 août 2026 a perdu un BA, et le BP n'a plus reçu de réponse d'aucun BA. L'analyse (`design.md`, Context) établit les points suivants :

- les îlots TBTS du BP et des BA ne sont reliés que par le câble de bus. Au-delà de 6,3 à 7,8 V d'écart entre leurs 0 V, SDA et SCL deviennent des chemins parallèles au fil 0 V. Ce seuil est atteint vers 20 A en onde 8/20 ;
- en I2C non isolé, un BA non alimenté tient SDA et SCL entre 0,6 et 3 V, car son PIC est réalimenté par ses diodes de protection. Il bloque tout le bus ;
- le module SSP de base du PIC16F946 n'offre pas de contrôle logiciel de l'ACK ;
- le BSP I2C de production du BP est perdu, et un rebuild réintroduit des attentes infinies.

### Ce que le change en cours apporte, et ses limites

Le change `i2c-surge-protection-2026-10-007` garde l'I2C. Il apporte :

- des retouches des cartes ;
- l'étoile 0 V ;
- l'isolement de l'Ethernet du BP ;
- la révision E avec la carte d'interconnexion pour le coffret unique ;
- le module d'isolement par BA (MIB) pour les BA déportés ;
- la résilience du firmware.

Trois limites restent propres à l'I2C :

- le MIB est qualifié jusqu'à 3 m de câble, avec un plafond de 400 pF côté bus ;
- sans MIB, le confinement d'un BA non alimenté reste manuel ;
- la carte d'interconnexion rend les 0 V équipotentiels mais ne coupe aucun chemin.

### Le déclencheur

L'approvisionnement du PIC16F946 est tendu. Son remplacement imposera de réécrire le firmware des BA. C'est le bon moment pour changer de couche physique, et le design (D9) retient pour cela le RS-485 isolé. Cet ADR formalise la décision.

### Contraintes permanentes

- TBTS : aucune liaison entre le 0 V et le PE.
- Protocole HTTP legacy IoT inchangé, y compris le codage de l'index 12 (`comm_faults`).
- Essais sur banc uniquement, jamais sur une armoire en service.
- Aucun BP n'est reflashé avec un build non qualifié.

## Critères de décision

| Critère | Question posée |
|---|---|
| **C1. Immunité** | L'option coupe-t-elle la boucle galvanique entre îlots, données et alimentation comprises ? Que devient le bus quand un nœud est mort ou non alimenté ? Quelle longueur de câble admet-elle ? |
| **C2. Coût par carte** | Quel surcoût de BOM par nœud (estimation hors assemblage) ? |
| **C3. Compatibilité PIC16F946 / MCF52259** | Les périphériques et les broches existent-ils sur les deux microcontrôleurs, sans conflit ? |
| **C4. Firmware** | Quelle part réécrire ? Les trames, le CRC et la couche application sont-ils conservés ? La dépendance au BSP I2C perdu disparaît-elle ? |
| **C5. Approvisionnement du PIC16F946** | L'option permet-elle de garder le PIC16F946 pendant la transition, puis de choisir librement son successeur ? |

## Options considérées

| | **A. RS-485 isolé** | B. CAN isolé | C. I2C isolé permanent (isolateur par nœud) | D. I2C non isolé + carte d'interconnexion |
|---|---|---|---|---|
| **C1. Immunité** | Barrière par nœud (données et alimentation). Liaison différentielle. Un transceiver non alimenté est en haute impédance : un nœud mort libère le bus. Câbles longs possibles, donc BA déportés sans limite de 3 m. | Barrière par nœud. Liaison différentielle. En plus, le confinement des erreurs du protocole CAN (état *bus-off*) retire de lui-même un nœud défaillant. | Barrière par nœud. Mais 3 m au plus et 400 pF côté bus. La libération du bus par un nœud mort repose sur l'astuce VCC1 et sur un superviseur, soit trois points de défaillance de plus par nœud. | Aucune coupure. Le seuil d'équilibrage monte vers 875 A en onde 8/20 avec les plots 0 V, mais seulement dans un coffret unique. Un BA non alimenté bloque toujours le bus. |
| **C2. Coût par carte** (estimation) | Transceiver isolé de 5 à 12 €, plus une alimentation isolée du nœud de 20 à 40 €. | Du même ordre que A, plus un contrôleur CAN externe (MCP2515) ou un microcontrôleur avec CAN. | ISO1640 de 4 à 6 €, plus la même alimentation isolée, plus un superviseur. Sous forme de MIB : environ 50 à 85 € par BA (150 à 250 € pour trois). | Rien par carte au-delà de la révision E ; 60 à 100 € par coffret pour la carte d'interconnexion. |
| **C3. Compatibilité** | PIC16F946 : AUSART sur RC6/RC7, mêmes broches que le SSP, avec détection d'adresse 9 bits (ADDEN). MCF52259 : UART2 en fonction alternative des broches 28/29. Signal DE à trouver sur chaque BA (voir plus bas). | PIC16F946 : pas de contrôleur CAN. Un MCP2515 en SPI demande SDO sur RC4, qui sert d'entrée TOR (`ENTREE16` dans `hard.h`). MCF52259 : contrôleur FlexCAN présent (vecteurs FlexCAN du PSP MCF5225). | Inchangée : SSP et I2C0 conservés. | Inchangée. |
| **C4. Firmware** | Couche liaison réécrite des deux côtés. Trames, CRC-16 et couche application conservés. Pilote série MQX standard, déjà utilisé par le BP pour l'écran et la téléinformation, à la place du BSP I2C perdu. Console Espion à déplacer. | Couche liaison réécrite. Les trames de plus de 8 octets (configurations : jusqu'à 19 octets avec le code et le CRC, dont 16 de données pour TPS_EXTINCTION) doivent être segmentées : les trames ne sont plus conservées telles quelles. | Aucun changement de protocole. Le superviseur est à concevoir. Le pilote I2C à attentes bornées reste nécessaire (tâche 5.1). | Aucun, au-delà du travail en cours (tâches 5.x). |
| **C5. Approvisionnement** | Fonctionne avec le PIC16F946 pendant la transition, et avec tout successeur doté d'un UART. L'adressage 9 bits est courant, et s'émule sinon par parité forcée. | Interdit de garder le PIC16F946 sans retouche de carte. Impose un successeur avec CAN sur tous les BA à la fois. | Le successeur doit être un esclave I2C tolérant l'étirement d'horloge. Les particularités du SSP sont reconduites. | Idem C. |

## Décision

**Proposée : la génération 3 du bus BP ↔ BA utilise un RS-485 isolé, semi-duplex et multipoint, avec un adressage 9 bits.** Les trames, le CRC-16 et les adresses sont conservés.

Jusqu'à la génération 3, les solutions du change en cours restent la référence : révision E avec carte d'interconnexion dans un coffret unique, MIB pour les BA déportés.

### Couche physique

- Paire A/B et commun, en multipoint semi-duplex. Terminaison de 120 Ω aux deux extrémités. Polarisation de repos (*fail-safe*) en un seul point, au BP.
- TVS de ligne asymétrique (SM712, −7 V / +12 V) côté ligne de chaque nœud.
- Un **transceiver isolé par nœud**, BP compris, et une **alimentation isolée par nœud**. L'alimentation est soit un convertisseur isolé alimenté par le +15 V du bus, soit une alimentation locale de classe II. Le choix se fait à la conception, en gardant la relance des BA par TE2.
- Candidats relevés : TI ISO1410, ADI ADM2587E (alimentation isolée du côté ligne intégrée), MAX14943. Ils sont à qualifier.
- La barrière suit la règle des cartes à isolation intégrée : 8 mm sans cuivre entre Z1 et Z2 (voir [les règles de conception de la révision E](../rev-e-regles-conception.md), § 3.1).

### Périphériques et broches

- **BP.** L'UART2 est prise en fonction alternative des broches 28/29, qui portent aujourd'hui SCL et SDA de l'I2C0. UTXD2 remplace SCL et URXD2 remplace SDA. C'est le seul UART libérable :
    - l'UART0 (`ittya:`) sert à l'écran, à 9 600 bit/s ;
    - l'UART1 (`ittyb:`) sert à la téléinformation, à 1 200 bit/s ;
    - l'UART2 (`ittyc:`) sert aujourd'hui à la console Espion.
- **BA.** L'AUSART du PIC16F946 utilise RC6 (TX) et RC7 (RX), les broches du SSP actuel (SCL et SDA). Le SSP est désactivé (SSPEN = 0).

### Signal DE et cas du SC941C

- **BP.** La direction est commandée par une sortie GPIO dédiée, sur le modèle de la liaison semi-duplex de l'écran, avec attente de fin d'émission par `IO_IOCTL_SERIAL_WAIT_FOR_TC` (`C/Ecran.c` de client-essensys-legacy). La sortie UA3 n'est pas disponible : c'est déjà la direction de l'écran (`BP_O_ECRAN_DIRECTION`, `H/application.h`). Une sortie libre du MCF52259 reste à identifier (voir les points à vérifier). La commande automatique de RTS du module UART du MCF52259 est une variante, à vérifier.
- **SC940D et SC942C.** DE est pris sur une broche libre du PIC : RB1 sur le SC940D, RA1 ou RE2 sur le SC942C, d'après les schémas, à reconfirmer. Il ne faut pas confondre ces broches avec les macros `RB1_*` de `hard.h`, qui désignent des sorties relais.
- **SC941C.** Aucune broche n'est libre. Le SC941C reçoit donc un **montage à direction automatique** :
    - l'émetteur est activé par TXD lui-même, et ne pilote la ligne que pendant les bits à 0 ;
    - l'état 1 et le repos sont imposés par la polarisation de repos du bus ;
    - le récepteur reste actif, si bien que le PIC reçoit son propre écho, que le firmware ignore ou utilise pour détecter une collision.

    Ce montage exige une polarisation franche et un débit modéré. Il se valide sur banc au débit retenu. Un transceiver isolé à direction automatique intégrée est une alternative, s'il en existe un qui convient.
- **Risque.** Un nœud dont l'émetteur reste actif bloque encore le bus : c'est l'équivalent de SCL tenue basse. Le chien de garde du BA 1.8 est conservé, et une limitation matérielle de la durée d'émission (monostable sur DE) est à étudier.

### Adressage 9 bits (ADDEN)

- Chaque caractère compte 11 bits : start, 8 bits de données, 9ᵉ bit, stop.
- Le BP émet l'adresse du BA visé comme **caractère d'adresse** (9ᵉ bit à 1). Les adresses 0x11, 0x12 et 0x13 sont conservées. Viennent ensuite CMD, DATA et le CRC, en caractères de données (9ᵉ bit à 0).
- Sur chaque BA, RX9 = 1 et ADDEN = 1 : l'AUSART n'interrompt le PIC que sur un caractère d'adresse.
    - Sur sa propre adresse, le BA remet ADDEN à 0, reçoit la trame, puis remet ADDEN à 1.
    - Les autres BA ne sont pas réveillés par les octets de données. Un octet de donnée égal à une adresse n'est donc jamais pris pour elle.
- **Fin de trame.** En I2C, la condition de STOP ou de START répété marque la fin d'une écriture. En RS-485, elle est détectée par un silence d'au moins deux caractères (environ 1,15 ms à 19 200 bit/s), ou par le caractère d'adresse suivant. La trame reste validée par son CRC-16, comme aujourd'hui. La disponibilité d'un temporisateur du PIC pour mesurer ce silence est à vérifier.
- **Côté BP.** Le pilote série MQX du MCF52xx gère déjà le mode multipoint :
    - parités `IO_SERIAL_PARITY_MULTI_ADDRESS` et `IO_SERIAL_PARITY_MULTI_DATA` ;
    - adresse de station réglée par `IO_IOCTL_SERIAL_SET_ADDRESS_DEVICE` ;
    - filtrage en réception dans `serl_int_mcf52xx.c`.

    Proposition à valider sur banc : la réponse du BA commence par l'adresse du BP en caractère d'adresse, ce qui active le récepteur du BP en mode multipoint. La parité forcée (`MARK` / `SPACE`) produit les mêmes caractères sur la ligne et constitue une variante.

### Débits possibles avec le quartz de 20 MHz

Côté BA, l'AUSART est réglé avec BRGH = 1, et le débit vaut 20 MHz / (16 × (SPBRG + 1)). Côté BP, l'horloge système est de 80 MHz (`BSP_SYSTEM_CLOCK`) et le diviseur arrondi vaut 80 MHz / (32 × débit).

| Débit visé | PIC16F946 (SPBRG, écart) | MCF52259 (diviseur, écart) | Écart entre les deux | Verdict |
|---|---|---|---|---|
| 9 600 bit/s | 129, +0,16 % | 260, +0,16 % | 0 % | Possible, sans intérêt |
| **19 200 bit/s** | 64, +0,16 % | 130, +0,16 % | **0 %** | **Retenu par défaut** |
| 38 400 bit/s | 32, −1,36 % | 65, +0,16 % | 1,5 % | Admis si le besoin de débit le justifie |
| 57 600 bit/s | 21, −1,36 % | 43, +0,94 % | 2,3 % | Écarté |
| « 115 200 » bit/s | 10, −1,36 % | 22, −1,36 % | 0 % (113 636 bit/s des deux côtés) | Non retenu sans essai |

- **Tolérance.** Sur un caractère de 11 bits, l'écart total doit rester bien sous environ 4 %, pour que le bit de stop soit échantillonné correctement. On s'en tient à 2 % au plus. C'est pourquoi 38 400 bit/s reste admis, et 57 600 bit/s est écarté.
- **Cas de 113 636 bit/s.** Ce débit tombe juste des deux côtés. Mais il ne laisse au PIC qu'environ 480 cycles d'instruction par caractère (5 MIPS), et il contraint le montage à direction automatique du SC941C.
- **Charge du bus.** Une trame ACTIONS (4 octets écrits, 5 lus, deux caractères d'adresse, soit 11 caractères) dure environ 6,3 ms à 19 200 bit/s, contre environ 2 ms en I2C à 50 kHz. La trame la plus longue, TPS_EXTINCTION (19 octets écrits, 5 lus, deux caractères d'adresse, soit 26 caractères), dure environ 15 ms. Le BP n'émet que sur changement d'état, avec une interrogation de présence toutes les 30 à 60 s (design D8). La marge reste donc large.

### Trames et CRC conservés

- **Écriture.** CMD, DATA et CRC-16 sont inchangés octet pour octet. Le CRC est calculé de la même façon (`crc.c`).
- **Réponse.** L'écho de CMD, l'écho du CRC, le CRC de la réponse et l'octet `comm_stat` du BA 1.8 sont inchangés.
- **Adresse.** Seul l'octet d'adresse change de forme : il passe du premier octet I2C au caractère dont le 9ᵉ bit est à 1.
- **Couche application.** Elle est conservée sans changement :
    - `ba.c` ;
    - les files par BA ;
    - la resynchronisation ;
    - les états OK, SUSPECT et ABSENT ;
    - l'index 12.
- **Protocole HTTP legacy IoT.** Il n'est pas modifié. L'index 12 garde son codage : bits 0x04, 0x08 et 0x10 pour les BA.

### Console Espion sur telnet

- **Aujourd'hui.** La console Espion écrit sur le canal d'entrée-sortie par défaut, `ittyc:`, c'est-à-dire l'UART2 à 9 600 bit/s. `C/main.c` exige que ce canal soit défini. Son connecteur n'est pas monté en série : il faut souder J102 et R109/R110.
- **En génération 3.** L'UART2 est réutilisée pour le bus. La console passe donc sur une **session telnet**, par le serveur telnet de la pile réseau RTCS. Le canal par défaut est redirigé vers un périphérique nul ou vers la session, et le contrôle de compilation de `main.c` est adapté.
- **Sécurité.** Telnet circule en clair, et la console peut agir sur la carte. Elle est donc :
    - désactivée par défaut, et activée localement ;
    - limitée au réseau local ;
    - sans commande d'action sans authentification ;
    - protégée par un secret propre à chaque BP, plutôt que par la constante actuelle du source.

    La revue `security-gate` est obligatoire.
- **Gain.** Le diagnostic n'exige plus de souder un connecteur.

### Compatibilité et migration

- I2C et RS-485 ne cohabitent pas sur un même bus.
- Une installation passe en génération 3 entièrement, BP et BA ensemble. À défaut, le BP de génération 3 garde un port I2C distinct pour les BA existants, sur le second module I2C du MCF52259 si son multiplexage le permet. Ce point est à étudier.
- Tous les BA sont reflashés par ICSP, avec un `.hex` par type, après lecture de l'EEPROM.

## Conséquences

**Positives.**

- La boucle galvanique entre îlots est coupée nœud par nœud, données et alimentation comprises.
- Un BA mort ou non alimenté libère le bus : le confinement devient automatique sans MIB.
- Les BA déportés ne sont plus limités à 3 m, et ni le plafond de 400 pF ni le superviseur du MIB ne s'appliquent plus.
- Le pilote série MQX standard remplace le BSP I2C perdu. Ce pilote est déjà éprouvé sur le BP, pour l'écran et la téléinformation.
- Les trames, le CRC, la couche application et le protocole HTTP sont conservés : les tests `NR-board-sc944d-5`, `-6`, `-7` et `-11` restent applicables.
- Le choix du successeur du PIC16F946 devient indépendant de la couche physique.

**Négatives et coûts.**

- La couche liaison est réécrite des deux côtés, et tous les BA sont reflashés.
- Il faut une nouvelle génération de cartes, avec transceiver et alimentation isolés : 25 à 50 € par nœud (estimation).
- Le montage à direction automatique du SC941C est à concevoir et à qualifier.
- La console Espion passe sur telnet, avec la revue de sécurité qui va avec.
- Un échange ACTIONS dure environ 6,3 ms au lieu de 2 ms.
- La migration se fait par installation complète, ou avec un port I2C de compatibilité.

**Risques résiduels.**

- Un émetteur bloqué tient encore le bus. Parades : chien de garde, et limitation matérielle de la durée d'émission à étudier.
- Les TVS de ligne restent nécessaires contre les surtensions différentielles.

## Alternatives écartées

- **B. CAN isolé.** Il offre le meilleur confinement des erreurs, mais :
    - le PIC16F946 n'a pas de contrôleur CAN ;
    - un MCP2515 en SPI entre en conflit sur RC4 ;
    - il impose le successeur du PIC sur tous les BA à la fois ;
    - il oblige à segmenter les trames de plus de 8 octets.

    Cette option est à reconsidérer si un microcontrôleur avec CAN est adopté pour tous les BA en même temps.
- **C. I2C isolé permanent.** C'est la solution du MIB pour les BA déportés. Elle reste limitée à environ 3 m et à 400 pF. Elle dépend d'un superviseur et de l'astuce VCC1 pour libérer le bus, et elle reconduit les particularités du SSP.
- **D. I2C non isolé avec carte d'interconnexion.** C'est la solution de la révision E dans un coffret unique, et elle le reste. Mais elle ne coupe rien, ne traite pas les BA déportés, et laisse un BA non alimenté bloquer le bus.
- **Déjà écartés par le design :**
    - l'I2C différentiel PCA9615, non isolé, qui ne tolère que 2,5 V de décalage de masse ;
    - le LTC4331, point à point et non isolé ;
    - l'étoile active TCA9548A, qui demande un recâblage et du firmware ;
    - le tampon TCA4307 par nœud, déconseillé en architecture à nœud commun.

## Points à vérifier avant l'acceptation

1. Sur le manuel de référence du MCF52259, le multiplexage des broches 28/29 en UTXD2/URXD2 (fonction alternative de PAS0/PAS1), et la disponibilité d'un second module I2C pour un port de compatibilité.
2. Sur le schéma du SC944D et dans `H/application.h`, une sortie GPIO libre du MCF52259 pour le DE du bus : UA3 est déjà la direction de l'écran.
3. Sur banc, le mode multipoint du pilote MQX en interruption : passage du mode adresse au mode données à chaque trame, retournement de la ligne, attentes bornées.
4. Sur les schémas, les broches libres pour DE (RB1 du SC940D, RA1 ou RE2 du SC942C).
5. Sur banc, au débit retenu, le montage à direction automatique du SC941C.
6. Un temporisateur du PIC16F946 libre pour détecter la fin de trame.
7. Le choix du transceiver isolé, et l'architecture d'alimentation des nœuds (convertisseur isolé ou alimentation locale de classe II), en gardant la relance par TE2.
8. La revue `security-gate` de la console sur telnet.
9. La stratégie de migration : bascule par installation complète, ou port I2C de compatibilité.

## Références

- `openspec/changes/i2c-surge-protection-2026-10-007/design.md` : D6 (carte d'interconnexion et MIB), D8 (firmware), D9 (génération 3).
- Specs du change : `i2c-bus-surge-protection`, `i2c-bus-fault-containment`, `aux-box-interconnect`.
- [Règles de conception de la révision E](../rev-e-regles-conception.md).
- Code du projet :
    - client-essensys-legacy : `C/Ecran.c` (liaison semi-duplex de l'écran), `C/EspionRS.c`, `C/main.c`, `C/crc.c`, `H/application.h` ;
    - essensys-gcc : `bp/bsp/m52259evb/` (`gpio_init.c`, `init_uart2.c`, `m52259evb.h`) et pilote série MQX `serl_int_mcf52xx.c` ;
    - sources BA 1.7 : `hard.h` (RC4 = `ENTREE16`).
