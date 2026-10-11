## Purpose

Cette capacité garantit qu'un défaut localisé sur le bus I2C BP ↔ BA est détecté, signalé et contenu. Les défauts visés sont un BA hors service, une ligne collée, un PIC bloqué ou une commande perdue. Le boîtier principal rétablit seul la communication et l'état de chaque BA dès que le défaut disparaît ou que le BA en cause est déconnecté. La libération automatique du bus par un BA non alimenté n'est exigée que derrière un module d'isolement ou une interface isolée intégrée. Elle repose sur :

- les attentes bornées et la récupération de bus ;
- la resynchronisation des configurations ;
- la télémétrie par BA ;
- le chien de garde et la gestion d'erreur des BA.

## ADDED Requirements

### Requirement: Attentes I2C bornées dans le temps

Le firmware du BP SHALL borner chaque attente d'un événement du contrôleur I2C à 2 ms au plus, et chaque transaction avec un BA à 20 ms au plus. La borne par attente SHALL valoir au moins deux fois l'étirement d'horloge maximal mesuré sur banc sur un BA 1.7. Si cet étirement dépasse 1 ms, la borne par attente SHALL être relevée à deux fois sa valeur, la borne par transaction restant de 20 ms au plus.

Un bus bloqué MUST NOT figer les autres fonctions du BP : les échanges HTTP avec le serveur, la téléinformation et les scénarios locaux SHALL continuer.

#### Scenario: SDA forcée basse pendant 60 s

- **WHEN** SDA est maintenue à l'état bas sur J31 pendant 60 s sur banc
- **THEN** chaque transaction I2C SHALL se terminer en erreur en 20 ms au plus
- **AND** le BP SHALL continuer ses échanges avec le serveur de banc à sa période habituelle

#### Scenario: SCL forcée basse pendant 60 s

- **WHEN** SCL est maintenue à l'état bas sur J31 pendant 60 s sur banc
- **THEN** chaque transaction I2C SHALL se terminer en erreur en 20 ms au plus
- **AND** la téléinformation SHALL continuer à être lue et transmise
- **AND** les scénarios locaux SHALL continuer à s'exécuter

#### Scenario: Étirement d'horloge d'un BA 1.7

- **WHEN** un BA 1.7 étire l'horloge à sa durée maximale mesurée, pendant un trafic soutenu sur banc
- **THEN** les transactions avec ce BA MUST NOT se terminer en erreur d'attente

### Requirement: Récupération d'un bus bloqué

Si le BP trouve SDA ou SCL à l'état bas alors que le bus devrait être libre, il SHALL tenter une récupération dans les 100 ms. La récupération comprend jusqu'à neuf impulsions d'horloge, une condition STOP et la réinitialisation du contrôleur. Tant que le défaut persiste, le BP SHALL renouveler la tentative au plus une fois par seconde.

Dès que le défaut disparaît, la communication avec chaque BA présent SHALL reprendre en moins de 10 s. Cette reprise se fait sans redémarrage du BP ni coupure de l'alimentation des BA.

#### Scenario: Esclave bloqué au milieu d'un octet

- **WHEN** un BA retient SDA à l'état bas après une transaction interrompue
- **THEN** le BP SHALL libérer le bus par ses impulsions d'horloge et sa condition STOP
- **AND** chaque BA présent SHALL acquitter de nouveau ses commandes en moins de 10 s, sans cycle d'alimentation

#### Scenario: Ligne forcée basse puis relâchée

- **WHEN** SDA ou SCL est forcée à l'état bas pendant 500 ms sur banc, puis relâchée
- **THEN** la communication avec chaque BA présent SHALL reprendre en moins de 10 s après le relâchement

#### Scenario: Défaut permanent

- **WHEN** une ligne reste à l'état bas de façon permanente
- **THEN** le BP SHALL limiter ses tentatives de récupération à une par seconde au plus
- **AND** le BP SHALL rester joignable par le serveur

### Requirement: Isolement d'un BA défaillant

Un BA non isolé, hors service ou non alimenté, peut bloquer le bus tant qu'il reste raccordé : son PIC, réalimenté par ses diodes de protection, ou un écrêteur de ligne en court-circuit tient alors SDA ou SCL, et ni le BP ni son firmware ne peuvent libérer le bus. Pour un tel BA :

- le BP SHALL signaler dans l'index 12 (`comm_faults`), en moins de 120 s, chaque BA qui ne répond plus ;
- le BP SHALL rester joignable par le serveur ;
- dès que le BA en cause est déconnecté du bus, les autres BA SHALL de nouveau acquitter leurs commandes en moins de 10 s, sans redémarrage du BP.

Un BA raccordé par un module d'isolement (MIB) ou par une interface isolée intégrée SHALL laisser le bus libre quand il n'est pas alimenté, sans aucune intervention. Les autres BA SHALL alors ne subir aucune interruption de plus de 1 s.

#### Scenario: Retrait d'un BA dans un coffret

- **WHEN** le port d'un BA est déconnecté du bus sur banc, par son câble ou par son cavalier d'interconnexion
- **THEN** les autres BA SHALL acquitter de nouveau leurs commandes en moins de 10 s
- **AND** en moins de 120 s, `(index 12 & 0x1C)` SHALL ne contenir que le bit du BA retiré

#### Scenario: BA isolé mis hors tension

- **WHEN** un BA raccordé par un MIB est mis hors tension
- **THEN** le bus SHALL rester libre
- **AND** les autres BA SHALL ne subir aucune interruption de communication de plus de 1 s

#### Scenario: BA non isolé laissé raccordé sans alimentation

- **WHEN** un BA sans isolement reste raccordé au bus alors qu'il n'est plus alimenté
- **THEN** le BP SHALL signaler dans l'index 12, en moins de 120 s, chaque BA qui ne répond plus
- **AND** le BP SHALL rester joignable par le serveur
- **AND** après la déconnexion de ce BA, les autres BA SHALL acquitter de nouveau leurs commandes en moins de 10 s

### Requirement: Libération automatique d'un nœud isolé bloqué

Sur un BA raccordé par un MIB ou par une interface isolée intégrée, l'interface SHALL libérer le côté bus si SDA ou SCL reste à l'état bas côté BA pendant plus de 100 ms. La libération SHALL intervenir moins de 150 ms après le début du défaut.

L'interface SHALL ensuite tenter de débloquer le côté BA, par jusqu'à neuf impulsions d'horloge côté BA, puis réessayer périodiquement le raccordement au bus, à un intervalle croissant plafonné à 5 s. Chaque essai qui retrouve la ligne basse SHALL rendre le bus libre en moins de 150 ms.

#### Scenario: Ligne forcée basse côté BA

- **WHEN** SDA ou SCL est forcée à l'état bas côté BA d'un nœud isolé, par une mire à transistor
- **THEN** le côté bus SHALL être libéré moins de 150 ms après le début du forçage
- **AND** les autres BA SHALL continuer à acquitter leurs commandes, sans interruption de plus de 150 ms par essai de raccordement

#### Scenario: Esclave bloqué derrière un nœud isolé

- **WHEN** le PIC d'un BA raccordé par un MIB retient SDA au milieu d'un octet, sans activité de SCL
- **THEN** l'interface SHALL libérer le côté bus moins de 150 ms après le début du défaut
- **AND** le BA SHALL de nouveau acquitter ses commandes en moins de 10 s, sans intervention

#### Scenario: Fin du défaut côté BA

- **WHEN** le forçage côté BA cesse
- **THEN** le BA concerné SHALL de nouveau acquitter ses commandes en moins de 10 s

### Requirement: Chien de garde et gestion d'erreur du BA

Le firmware des BA SHALL activer le chien de garde du PIC. Il SHALL traiter les erreurs du module SSP (débordement, collision, horloge retenue) sans coupure d'alimentation.

Un PIC bloqué SHALL relâcher SCL en moins de 2 s.

Si SDA ou SCL reste tenue par le BA plus de 50 ms sans front sur SCL, alors que sa boucle principale tourne, le BA SHALL réinitialiser son module SSP.

Un BA dont le tampon de commande est occupé SHALL refuser par un NACK toute nouvelle commande, au lieu de l'accepter puis de la perdre.

Après un redémarrage par le chien de garde, le BA SHALL le signaler au BP par son octet d'état. Le BP SHALL alors le resynchroniser en moins de 10 s.

#### Scenario: PIC bloqué avec SCL retenue

- **WHEN** un firmware de test fige le PIC d'un BA en retenant SCL à l'état bas, sur banc
- **THEN** SCL SHALL être relâchée en moins de 2 s
- **AND** le BA SHALL être resynchronisé par le BP dans les 10 s qui suivent

#### Scenario: Module SSP figé, boucle principale vivante

- **WHEN** un firmware de test laisse la boucle principale tourner mais fige le module SSP en retenant SDA, sur banc
- **THEN** le BA SHALL réinitialiser son module SSP et relâcher SDA moins de 100 ms après le dernier front de SCL
- **AND** le BA SHALL de nouveau acquitter ses commandes sans cycle d'alimentation

#### Scenario: Débordement du module SSP

- **WHEN** un débordement du module SSP est provoqué sur un BA
- **THEN** le BA SHALL acquitter de nouveau ses commandes sans cycle d'alimentation

#### Scenario: Tampon de commande occupé

- **WHEN** le BP envoie une commande à un BA dont le tampon de commande est occupé
- **THEN** le BA SHALL répondre par un NACK
- **AND** le BP SHALL retransmettre la commande, qui SHALL être appliquée

### Requirement: Commandes conservées jusqu'à acquittement

Le BP MUST NOT considérer comme acquittée une commande qu'un BA n'a pas acquittée.

Pour chaque BA, le BP SHALL conserver le dernier état demandé de chaque sortie tant qu'il n'a pas été acquitté, et l'appliquer dès que le BA répond de nouveau, sans rejouer les états intermédiaires devenus obsolètes :

- un état maintenu (sortie tout-ou-rien, variateur, consigne) SHALL être appliqué quel que soit son âge ;
- une commande de volet ou une commande impulsionnelle SHALL être abandonnée si elle n'a pas pu être appliquée dans un délai configurable, de 5 min par défaut. Elle SHALL alors être signalée comme expirée sur la console de diagnostic, sans être marquée acquittée.

#### Scenario: Commande émise pendant l'absence d'un BA

- **WHEN** une commande d'éclairage est émise pour un BA déconnecté depuis 5 min
- **THEN** la console de diagnostic du BP SHALL montrer cette commande en attente, et non acquittée
- **AND** la commande SHALL être appliquée moins de 10 s après le retour du BA

#### Scenario: Commandes successives pendant l'absence

- **WHEN** une sortie reçoit successivement les ordres marche, arrêt puis marche pendant l'absence de son BA
- **THEN** seul l'état final « marche » SHALL être appliqué au retour du BA

#### Scenario: Commande de volet expirée

- **WHEN** une commande de volet reste non appliquée au-delà du délai configuré, faute de réponse de son BA
- **THEN** elle MUST NOT être appliquée au retour du BA
- **AND** la console SHALL la signaler comme expirée

### Requirement: Resynchronisation des configurations

Le BP SHALL resynchroniser un BA dans deux cas : à son propre démarrage, et à chaque retour de ce BA à l'état OK (après une absence, un défaut ou un redémarrage signalé par le BA). Il SHALL alors renvoyer :

1. l'intégralité de la configuration du BA : modes des variateurs, temps d'extinction, temps des volets ;
2. l'état demandé de toutes ses sorties.

L'acquittement de chaque trame SHALL être obtenu en moins de 10 s.

#### Scenario: Configuration modifiée pendant l'absence d'un BA

- **WHEN** la configuration d'un BA est modifiée pendant qu'il est déconnecté
- **THEN** la nouvelle configuration SHALL être appliquée et acquittée moins de 10 s après son retour

#### Scenario: Redémarrage du BP

- **WHEN** le BP redémarre avec trois BA présents
- **THEN** chaque BA SHALL avoir reçu et acquitté sa configuration complète moins de 10 s après la mise en service du bus par le BP

#### Scenario: BA dont le PIC a été remplacé

- **WHEN** un BA revient sur le bus avec une mémoire de configuration vierge
- **THEN** il SHALL recevoir sa configuration complète sans aucune action manuelle
- **AND** ses variateurs MUST NOT rester dans le mode par défaut

### Requirement: Télémétrie de l'état de chaque BA

Le BP SHALL tenir pour chaque BA un état de santé : OK, SUSPECT ou ABSENT. Cet état est mis à jour par les échanges et, même en l'absence de commande, par une interrogation de présence au moins toutes les 60 s.

L'index 12 (`comm_faults`) SHALL refléter cet état en moins de 120 s. Son codage actuel est conservé. Les bits 0 (défaut de communication du compteur) et 1 (défaut de communication de l'IHM) gardent leur signification ; les critères de cette capacité portent sur le masque `0x1C` :

| Bit | BA |
|---|---|
| 0x04 | 0x11 (SC940D) |
| 0x08 | 0x12 (SC942C) |
| 0x10 | 0x13 (SC941C) |

La console de diagnostic locale du BP SHALL afficher les compteurs suivants :

- récupérations de bus ;
- cycles d'alimentation des BA ;
- défauts simultanés de tous les BA ;
- commandes expirées.

#### Scenario: BA déconnecté sans commande

- **WHEN** un BA est déconnecté et qu'aucune commande ne lui est adressée
- **THEN** le bit de ce BA SHALL être positionné dans l'index 12 en moins de 120 s

#### Scenario: BA reconnecté

- **WHEN** un BA signalé en défaut est reconnecté
- **THEN** son bit SHALL être effacé de l'index 12 en moins de 120 s
- **AND** le BA SHALL avoir été resynchronisé

#### Scenario: Codage de l'index 12 inchangé

- **WHEN** les trois BA sont en défaut
- **THEN** `(index 12 & 0x1C)` SHALL valoir `0x1C` (28), comme avec le firmware 099-37
- **AND** les bits 0 et 1 SHALL garder leur valeur, indépendante de l'état des BA

### Requirement: Cycle d'alimentation des BA en dernier recours

Le BP SHALL couper l'alimentation des BA (sortie TE2) seulement en dernier recours, quand le bus reste bloqué au moins 60 s malgré les tentatives de récupération. Il SHALL respecter les règles suivantes :

- au moins 1 h entre deux cycles ;
- au plus 3 cycles par période glissante de 24 h, avec un compteur conservé à travers un redémarrage du BP ;
- après un cycle qui ne rétablit pas le bus, aucun nouveau cycle jusqu'au retour du bus ou jusqu'à une remise à zéro manuelle depuis la console de diagnostic.

L'état du cavalier J58 SHALL être déclaré dans la configuration du BP, ou détecté. Avec J58 ouvert, +15VSP reste fourni en permanence et TE2 n'a pas d'effet : le BP MUST NOT effectuer ni compter de cycle, et SHALL signaler le blocage sur la console.

Chaque cycle SHALL être compté et daté. Il relève du critère C de la capacité `i2c-bus-surge-protection` et SHALL être suivi d'une resynchronisation complète.

#### Scenario: Blocage persistant

- **WHEN** le bus reste bloqué plus de 60 s malgré les tentatives de récupération, J58 étant fermé
- **THEN** le BP SHALL effectuer un seul cycle d'alimentation des BA
- **AND** le compteur de cycles SHALL augmenter de un
- **AND** chaque BA qui répond SHALL être resynchronisé en moins de 10 s

#### Scenario: Cycle sans effet

- **WHEN** un cycle d'alimentation ne rétablit pas le bus, par exemple parce qu'un BA mort le tient
- **THEN** le BP MUST NOT effectuer de nouveau cycle avant le retour du bus ou une remise à zéro manuelle
- **AND** les BA concernés SHALL rester signalés en défaut dans l'index 12

#### Scenario: Limite de cycles atteinte

- **WHEN** trois cycles ont déjà eu lieu dans les 24 dernières heures, y compris avant un redémarrage du BP, et que le bus se bloque de nouveau
- **THEN** le BP MUST NOT effectuer de quatrième cycle
- **AND** les BA concernés SHALL rester signalés en défaut dans l'index 12

#### Scenario: Cavalier J58 ouvert

- **WHEN** le bus reste bloqué plus de 60 s alors que J58 est déclaré ouvert
- **THEN** le BP MUST NOT effectuer ni compter de cycle
- **AND** la console SHALL signaler le blocage

### Requirement: Branchement à chaud d'un BA

Le branchement ou le débranchement d'un BA pendant le trafic MUST NOT faire perdre de commande aux autres BA. Une trame perturbée (erreur de CRC ou NACK) SHALL être retransmise, puis appliquée.

#### Scenario: Branchement d'un BA pendant le trafic

- **WHEN** un BA est branché puis débranché sur banc pendant un trafic soutenu vers les autres BA
- **THEN** toute commande destinée aux autres BA SHALL être appliquée
- **AND** chaque trame perturbée SHALL être retransmise puis acquittée

### Requirement: Compatibilité des firmwares et du protocole legacy

Les évolutions de firmware MUST NOT modifier le protocole HTTP legacy IoT entre le BP et le serveur : requêtes, indices et codage restent identiques.

Les combinaisons de versions SHALL fonctionner ainsi :

- un firmware BP modifié SHALL fonctionner avec des BA en version 1.7 ;
- un firmware BA 1.8 SHALL fonctionner avec un BP en version 099-37, qui ignore son octet d'état supplémentaire.

Un nouveau firmware BP MUST NOT être flashé sur un BP en service tant que le pilote I2C à attentes bornées n'a pas passé la suite de non-régression sur banc.

Chaque BA SHALL être programmé uniquement avec le fichier de son type, et répondre ainsi à la seule adresse 0x11, 0x12 ou 0x13.

#### Scenario: Matrice de compatibilité croisée

- **WHEN** les combinaisons BP 099-37 ou BP modifié avec BA 1.7 ou BA 1.8 sont essayées sur banc
- **THEN** chaque BA SHALL acquitter ses commandes et sa configuration dans les quatre combinaisons

#### Scenario: Échanges HTTP inchangés

- **WHEN** les échanges HTTP d'un BP modifié sont capturés sur banc et comparés à ceux d'un BP 099-37, à état identique
- **THEN** les requêtes, les indices et leur codage SHALL être identiques

#### Scenario: Firmware BP non qualifié

- **WHEN** un build du firmware BP n'a pas passé les tests de non-régression sur banc du pilote I2C
- **THEN** il MUST NOT être flashé sur un BP en service

#### Scenario: Programmation d'un BA

- **WHEN** un BA est reprogrammé
- **THEN** il SHALL acquitter uniquement l'adresse de son type, 0x11, 0x12 ou 0x13
