## Why

Le 9 août 2026, une surtension d'origine atmosphérique a mis hors service un boîtier auxiliaire (BA) d'une installation Essensys, et le boîtier principal SC944D (BP) ne reçoit plus de réponse d'aucun BA ([essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1)). L'analyse montre des défauts communs à tout le parc : îlots TBTS reliés par le seul bus, courant non limité vers les circuits intégrés et sur le +15 V, firmware incapable de contenir un défaut local (détail dans `design.md`, Context). En pleine saison orageuse, il faut corriger maintenant sans refaire de circuit imprimé, puis par une révision des cartes.

## What Changes

- **Diagnostic et remise en service de l'installation touchée.** L'intervention suit un protocole sûr :
  - mesurage préalable par une personne habilitée, puis consignation, y compris de toute source autonome du site ;
  - essais d'isolement à 500 V continu sur les trois BA et sur le BP ;
  - identification du BA défaillant ;
  - réparation, puis renvoi des configurations de **tous** les BA.
- **Retouches des BA (SC940D, SC941C, SC942C).**
  - R1/R2 (R1/R3 sur le SC941C) passent de 0 Ω à 47 Ω anti-impulsion. Si l'écart de masse mesuré dépasse 0,12 V, la liaison équipotentielle est posée avant la remise en service.
  - Chaque BA reçoit sa propre PTC suivie d'une TVS sur +Vin.
  - C2 passe en 35 V et D6 en 40 V.
  - Option renforcée : écrêteur secondaire BAT54S, et 100 Ω seulement avec les pull-ups du BP à 2,2 kΩ.
  - Relevé des distances 230 V / TBT des cartes révision D.
- **Retouches du BP (SC944D).**
  - BA_ALIM est protégé par une PTC suivie d'une TVS ; l'exposition de +15VS (U2, C110) est vérifiée.
  - U16 est remplacé par un régulateur dont la tenue d'entrée est suffisante.
  - D33/D34 sont remplacées.
  - Option renforcée : pull-ups de 2,2 kΩ, 47 Ω en série et BAT54S.
- **Interconnexion du coffret.**
  - Le 0 V est rendu équipotentiel par une étoile 0 V doublée (N2), ou à défaut par un renfort 0 V (N1). Aucune liaison 0 V–PE n'est créée : la TBTS est conservée.
  - L'Ethernet du BP est isolé galvaniquement (fibre ou isolateur). **Aucun parafoudre RJ45 relié au PE** n'est posé côté BP.
- **Firmware BP** (client-essensys-legacy, pilote I2C dans essensys-gcc).
  - Le pilote I2C est reconstitué avec des attentes bornées. C'est le préalable à tout nouveau build.
  - Ajouts : récupération de bus bloqué, commandes conservées jusqu'à leur acquittement, resynchronisation des configurations, télémétrie par BA.
  - Le cycle d'alimentation des BA ne sert qu'en dernier recours, avec des cycles espacés, limités et suspendus s'ils sont sans effet.
  - Le protocole HTTP legacy IoT **n'est pas modifié**. L'index 12 (`comm_faults`) garde son codage et il est désormais tenu à jour.
- **Firmware BA 1.8.** Il ajoute un chien de garde, la gestion des erreurs du module SSP avec un timeout et un NACK quand le tampon est occupé. Un octet d'état rétrocompatible permet de signaler les redémarrages.
- **Long terme.**
  - Révision E des BA et du BP : zonage, isolement 230 V / TBT d'au moins 6 mm, plots 0 V, et protections dans l'ordre connecteur → TVS → résistance → écrêteur → circuit intégré.
  - Carte d'interconnexion équipotentielle (fond de panier) pour le coffret unique.
  - Module d'isolement par BA (MIB) pour les BA déportés.
  - Le choix d'un RS-485 isolé pour la génération 3 est formalisé dans un ADR, sans être réalisé ici.
- **Essais sur banc uniquement.**
  - Tests de non-régression `NR-board-sc944d-1` à `-13`, harnais dans `tests/bench/`.
  - Pré-qualification IEC 61000-4-2, -4-4 et -4-5, plus des essais de courant d'équilibrage.
  - Jamais sur une armoire réelle.
- **Documentation.**
  - Retour d'expérience public et anonymisé sur essensys-doc.
  - Consignation anonymisée dans essensys-memory.
  - Correction des écarts de la documentation matériel.
- **BREAKING (matériel).** Un BA préparé pour un MIB, dont les TVS de ligne sont déposées, ne doit plus jamais être raccordé directement au bus. Un BA non alimenté ne doit plus rester raccordé au bus. Ces règles entrent dans la procédure de remise en service et dans le guide d'installation (tâches 8.3 et 8.4).

## Capabilities

### New Capabilities

- `i2c-bus-surge-protection` : tenue des interfaces du bus BP ↔ BA aux surtensions. Elle couvre :
  - la limitation du courant injecté dans les circuits intégrés ;
  - l'écrêtage coordonné ;
  - la protection du +15 V et les marges des composants d'alimentation ;
  - les niveaux I2C conservés à 50 kHz ;
  - la compatibilité TBTS ;
  - les niveaux d'immunité visés sur banc (IEC 61000-4-2, -4-4, -4-5) et les critères B et C ;
  - la défaillance maîtrisée.
- `i2c-bus-fault-containment` : un BA hors service ou une ligne collée est détecté, signalé et contenu, et le BP rétablit seul la communication dès que le défaut disparaît ou que le BA en cause est déconnecté. Elle couvre :
  - les attentes bornées et la récupération de bus ;
  - l'isolement d'un BA défaillant ;
  - le chien de garde du BA ;
  - les commandes conservées et la resynchronisation des configurations ;
  - la télémétrie par BA (`comm_faults`) ;
  - le cycle d'alimentation en dernier recours ;
  - le branchement à chaud ;
  - la compatibilité des firmwares et du protocole legacy.
- `aux-box-interconnect` : interconnexion des cartes d'un même coffret et traitement des ports qui en sortent. Elle couvre :
  - l'équipotentialité des 0 V à basse impédance, sans liaison au PE ;
  - la déconnexion individuelle d'un BA ;
  - la distribution protégée du +15 V ;
  - l'isolement de l'Ethernet du BP et des autres ports sortants ;
  - le câblage dans le coffret ;
  - le MIB pour les BA déportés.

### Modified Capabilities

Aucune. Le dépôt n'a encore aucune spec (`openspec list --specs`).

## Impact

- **Cartes.**
  - SC944D (ce dépôt) : variante de BOM « retouche foudre » et note de schéma, puis révision E.
  - SC940D, SC941C et SC942C (dépôts essensys-board-SC940, -SC941C et -SC942C) : variante de BOM « retouche foudre », puis révision E.
  - Nouvelle carte passive d'étoile 0 V (N2), puis carte d'interconnexion équipotentielle.
  - MIB pour les BA déportés.
- **Firmware BP** : client-essensys-legacy (`C/ba_i2c.c`, `C/ba.c`, `C/Hard.c`, `C/EspionRS.c`, `H/global.h`). La production reste en 099-37 tant que le pilote I2C n'est pas reconstitué et validé sur banc. Le BP n'est jamais reflashé avec un nouveau build avant cela.
- **essensys-gcc** : pilote I2C MQX à attentes bornées et ses tests hôtes, `ba/source/hard.h` corrigé, build BA séparé par type (adresses 0x11, 0x12, 0x13).
- **Firmware BA** : passage de la v1.7 à la v1.8 (sources `code_ba` des dépôts SC94x), reflash par ICSP avec le fichier `.hex` du type de la carte.
- **Serveur** : aucune modification du protocole HTTP legacy IoT. Une alerte sur un `comm_faults` non vide (essensys-server-backend) fera l'objet d'un ticket séparé.
- **Documentation.**
  - essensys-doc : retour d'expérience, procédure de remise en service, guide d'installation, pages `archi/hardware-*.md`.
  - essensys-memory.
  - `docs/i2c.md` de ce dépôt.
  - `essensys-board-SC940/docs/Architecture_Overview.md`.
- **Installation.** L'installation touchée est diagnostiquée, réparée, puis équipée de N2 et de l'Ethernet isolé, en opérations de maintenance manuelles. Les contrôles de l'électricien et de l'installateur PV restent hors du périmètre Essensys. Les règles de câblage et les recommandations de protection sont ajoutées au guide d'installation de tout le parc.
- **Outillage** : banc d'essais (BP et trois BA, échantillons dédiés aux injections, câble représentatif, charges factices, serveur legacy local), harnais de tests NR dans `tests/bench/`, location de générateurs ESD, EFT et onde de choc, laboratoire pour les essais finaux.
- **Contraintes permanentes.**
  - TBTS : jamais de liaison 0 V–PE.
  - Protocole HTTP legacy IoT inchangé.
  - Essais sur banc uniquement, jamais sur une armoire réelle.
  - Aucune donnée personnelle dans les dépôts publics.
