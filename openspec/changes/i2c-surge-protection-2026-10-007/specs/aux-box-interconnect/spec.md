## Purpose

Cette capacité définit l'interconnexion entre le boîtier principal SC944D et les boîtiers auxiliaires installés dans un même coffret, et le traitement des ports qui sortent de ce coffret. Elle couvre :

- l'équipotentialité des 0 V à basse impédance, sans liaison au PE ;
- la déconnexion individuelle d'un BA ;
- la distribution protégée du +15 V ;
- l'isolement de l'Ethernet du BP et des autres ports sortants ;
- les règles de câblage dans le coffret ;
- le module d'isolement des BA déportés.

## ADDED Requirements

### Requirement: 0 V équipotentiels à basse impédance dans un coffret

Dans un coffret qui réunit le BP et des BA, l'interconnexion SHALL relier le 0 V de chaque BA à celui du BP à basse impédance.

L'écart statique de 0 V entre J31-3 du BP et J1-3 de chaque BA SHALL rester inférieur ou égal à 0,05 V à consommation maximale.

Les écrêteurs I2C sont réputés conduire dès que le courant dans SDA ou dans SCL dépasse 100 mA, mesuré par une sonde de courant au connecteur du BA en bout de chaîne. Le courant d'équilibrage à partir duquel ils conduisent SHALL atteindre au moins les valeurs suivantes, la référence étant le seuil mesuré sur le même banc avec la chaîne d'origine :

| Interconnexion | Onde 8/20 | Front de 1 Ipk/µs |
|---|---|---|
| Étoile 0 V (N2), BA non modifiés ou retouchés | 4 fois la référence | 4 fois la référence |
| Renfort 0 V (N1), admis là où N2 n'est pas posable | 3 fois la référence | 2 fois la référence |
| Carte d'interconnexion, BP et BA révision E raccordés par leur plot 0 V | 800 A | 250 A |

#### Scenario: Écart statique de 0 V

- **WHEN** les trois BA sont à consommation maximale, relais monostables collés
- **THEN** l'écart de tension entre J31-3 du BP et J1-3 de chaque BA SHALL être inférieur ou égal à 0,05 V

#### Scenario: Référence de la chaîne d'origine

- **WHEN** un courant d'équilibrage croissant est injecté sur banc entre le 0 V du BA en bout de chaîne et le 0 V du BP, sur la chaîne d'origine (trois tronçons de câble d'environ 33 cm, sans liaison équipotentielle), en onde 8/20 puis sur un front de 1 Ipk/µs
- **THEN** le seuil de conduction mesuré SHALL être consigné comme référence dans le rapport d'essai

#### Scenario: Seuil de conduction avec l'étoile 0 V

- **WHEN** la même injection est faite avec l'étoile 0 V (N2)
- **THEN** les écrêteurs I2C MUST NOT conduire en dessous de 4 fois la référence, en onde 8/20 comme sur le front de 1 Ipk/µs

#### Scenario: Seuil de conduction avec le renfort 0 V

- **WHEN** la même injection est faite avec le renfort 0 V (N1)
- **THEN** les écrêteurs I2C MUST NOT conduire en dessous de 3 fois la référence en onde 8/20, ni en dessous de 2 fois la référence sur le front de 1 Ipk/µs

#### Scenario: Seuil de conduction en révision E

- **WHEN** la même injection est faite sur la carte d'interconnexion, avec un BP et des BA révision E raccordés par leur plot 0 V
- **THEN** les écrêteurs I2C MUST NOT conduire en dessous de 800 A en onde 8/20, ni en dessous de 250 A sur le front de 1 Ipk/µs

### Requirement: Aucune liaison au PE et repérage du 0 V

Tout conducteur, toute tresse ou tout plan qui renforce le 0 V SHALL être isolé. Il MUST NOT être vert-jaune ni bleu clair. Il SHALL porter à chaque extrémité le repère « 0 V TBTS – NE PAS RELIER À LA TERRE ».

Le coffret domotique MUST NOT comporter de borne ni de raccordement PE, et un conducteur PE MUST NOT être raccordé à un élément TBT. Le PE d'un départ 230 V qui traverse le coffret SHALL rester gainé avec les conducteurs de son circuit, dans les goulottes 230 V, séparé de la TBT. Ces règles d'installation SHALL être validées par un électricien avant leur publication dans le guide d'installation.

Un renfort de 0 V SHALL suivre le câble de bus sur toute sa longueur, plaqué contre lui.

#### Scenario: Contrôle à la mise en service

- **WHEN** l'interconnexion d'un coffret est contrôlée à la mise en service
- **THEN** chaque conducteur de renfort du 0 V SHALL être isolé, d'une couleur autorisée et repéré à ses deux extrémités
- **AND** le coffret domotique MUST NOT comporter de borne ni de raccordement PE
- **AND** un conducteur PE de passage SHALL être gainé avec son circuit et séparé de la TBT

#### Scenario: Renfort posé par un autre chemin

- **WHEN** un renfort de 0 V suit un chemin distinct de celui du câble de bus
- **THEN** l'installation SHALL être déclarée non conforme et le renfort reposé le long du câble

### Requirement: Déconnexion individuelle d'un BA

Avec une étoile 0 V ou une carte d'interconnexion, l'interconnexion SHALL permettre de déconnecter du bus les lignes SDA, SCL et +15 V d'un BA par une seule manœuvre. Cette manœuvre se fait sans déposer de carte ni toucher au câblage 230 V, et le BA reste relié au 0 V. La déconnexion d'un BA MUST NOT interrompre plus de 10 s la communication avec les autres.

Avec le renfort 0 V (N1), le chaînage est conservé : la déconnexion d'un BA SHALL se faire par le retrait de son câble de bus, et la coupure des BA situés en aval est alors admise.

#### Scenario: Déconnexion d'un BA

- **WHEN** le port d'un BA est déconnecté par sa manœuvre unique
- **THEN** SDA, SCL et +15 V de ce port SHALL être ouverts, et son 0 V SHALL rester relié
- **AND** les autres BA SHALL acquitter de nouveau leurs commandes en moins de 10 s

#### Scenario: Reconnexion d'un BA

- **WHEN** le port d'un BA est reconnecté
- **THEN** ce BA SHALL être resynchronisé et acquitter ses commandes en moins de 10 s

#### Scenario: Déconnexion avec le renfort 0 V

- **WHEN** le câble de bus d'un BA est retiré dans un coffret équipé du renfort 0 V
- **THEN** les BA restés raccordés au BP SHALL acquitter de nouveau leurs commandes en moins de 10 s

### Requirement: Distribution protégée du +15 V

L'interconnexion SHALL alimenter chaque BA par une sortie +15 V protégée par son propre limiteur réarmable, suivi de son propre écrêteur.

À 50 °C, le courant de maintien de chaque limiteur SHALL être au moins égal à 1,5 fois le courant mesuré du BA, relais monostables collés.

En cas de court-circuit franc sur une sortie :

- le limiteur de cette sortie SHALL limiter le courant, sans déclencher la protection du BP ;
- après le déclenchement de ce limiteur, les autres sorties SHALL retrouver 14 V ou plus dans les 5 s et les garder ;
- le BA de la sortie en court-circuit, privé d'alimentation, peut bloquer le bus : les autres BA SHALL de nouveau acquitter leurs commandes en moins de 10 s après l'ouverture du cavalier de son port.

#### Scenario: Court-circuit sur une sortie

- **WHEN** une sortie +15 V de l'interconnexion est court-circuitée au 0 V pendant 60 s sur banc
- **THEN** les autres sorties SHALL être à 14 V ou plus dans les 5 s qui suivent le court-circuit, et y rester
- **AND** la protection du BP MUST NOT déclencher
- **AND** après l'ouverture du cavalier du port en court-circuit, les BA raccordés aux autres sorties SHALL acquitter de nouveau leurs commandes en moins de 10 s

#### Scenario: Retour après court-circuit

- **WHEN** le court-circuit est supprimé
- **THEN** la sortie SHALL remonter à 14 V ou plus en 120 s au plus, sans intervention

#### Scenario: Tenue thermique en service

- **WHEN** l'interconnexion fonctionne 24 h à 50 °C, les trois BA à consommation maximale
- **THEN** les limiteurs MUST NOT déclencher

### Requirement: Isolement galvanique de l'Ethernet du BP

La liaison Ethernet du BP SHALL être isolée galvaniquement entre le réseau du bâtiment et l'îlot TBTS du BP, avec une tenue diélectrique d'au moins 4 kV efficaces pendant 60 s. Un équipement d'isolement actif côté BP SHALL être alimenté uniquement depuis l'îlot TBTS du BP.

Le port Ethernet du BP MUST NOT être équipé d'un parafoudre relié au PE. L'écran éventuel d'un câble blindé MUST NOT être raccordé côté BP.

#### Scenario: Mesure d'isolement de la liaison

- **WHEN** une tension de 500 V continu est appliquée entre les huit conducteurs du câble côté bâtiment, reliés ensemble, et le 0 V du BP
- **THEN** la résistance mesurée SHALL être d'au moins 100 MΩ

#### Scenario: Onde de mode commun sur l'Ethernet

- **WHEN** une onde de mode commun est appliquée sur banc entre les huit conducteurs du câble Ethernet et le 0 V du BP, avec puis sans l'isolement
- **THEN** avec l'isolement, le courant mesuré dans le fil 0 V du bus SHALL être inférieur au dixième de celui mesuré sans isolement
- **AND** le banc SHALL satisfaire le critère B

#### Scenario: Parafoudre réseau relié au PE

- **WHEN** l'installation est contrôlée
- **THEN** le port Ethernet du BP et son cordon côté BP MUST NOT comporter de parafoudre relié au PE

### Requirement: Ports qui sortent du coffret

Tout port TBT du BP ou d'un BA dont la ligne sort du coffret vers un équipement potentiellement relié au PE SHALL être protégé de l'une des deux façons suivantes :

- isolé galvaniquement, avec une tenue d'au moins 4 kV efficaces pendant 60 s ;
- ou protégé par un dispositif dont la fin de vie ne relie pas le 0 V au PE.

Une varistance ou un parafoudre MUST NOT être ajouté dans le coffret domotique, en plus des composants d'origine des cartes. La protection des circuits 230 V commutés qui sortent du bâtiment SHALL faire l'objet d'une recommandation du guide d'installation (parafoudre conforme à la NF EN 61643-11, type et emplacement validés par un électricien).

#### Scenario: Ligne TBT extérieure

- **WHEN** une ligne TBT raccordée au BP ou à un BA sort du bâtiment
- **THEN** elle SHALL être isolée galvaniquement avec une tenue d'au moins 4 kV efficaces pendant 60 s, ou protégée par un dispositif dont la fin de vie ne relie pas le 0 V au PE

#### Scenario: Départ 230 V extérieur

- **WHEN** un circuit 230 V commuté par un BA ou par le BP alimente une charge extérieure au bâtiment
- **THEN** le guide d'installation SHALL recommander sa protection par un parafoudre conforme, hors du coffret domotique
- **AND** une varistance ou un parafoudre MUST NOT être ajouté dans le coffret domotique

### Requirement: Câblage du bus dans un coffret

Dans un coffret, les règles de câblage suivantes SHALL s'appliquer :

- chaque liaison de bus mesure au plus 0,5 m ;
- SCL MUST NOT être torsadée avec le +15 V ;
- le câble de bus et les nappes d'entrées sont tenus à distance des bornes de phase, sous un manchon isolant là où ils les longent ;
- ils MUST NOT partager une goulotte avec des conducteurs 230 V ;
- le cordon Ethernet du BP sort directement du coffret.

#### Scenario: Contrôle du câblage à la mise en service

- **WHEN** le câblage d'un coffret est contrôlé à la mise en service
- **THEN** chaque liaison de bus SHALL mesurer au plus 0,5 m
- **AND** une goulotte MUST NOT contenir à la fois des conducteurs TBT et 230 V
- **AND** le câble de bus et les nappes SHALL être sous un manchon isolant là où ils longent des bornes de phase

### Requirement: Module d'isolement pour BA déportés

Un BA déporté SHALL être raccordé par un module d'isolement (MIB) qui isole ensemble les données et l'alimentation. Un BA est déporté s'il est installé hors du coffret du BP, ou si sa liaison de bus dépasse 0,5 m. La barrière du module SHALL être renforcée : au moins 4 kVAC, et au moins 8 mm de distance d'isolement sur le module.

Le MIB est qualifié pour un câble de 3 m au plus en AWG20. Avec ce câble, le bus SHALL respecter les critères suivants :

| Critère | Seuil |
|---|---|
| Capacité côté bus, BP compris | ≤ 400 pF |
| Niveau bas au MCF | ≤ 0,85 V |
| Niveau bas côté BA | < 0,48 V |
| Temps de montée de 30 % à 70 % de la tension d'alimentation | ≤ 1 µs |

Le plafond de capacité impose, sur le bus concerné, des écrêteurs de ligne de 50 pF au plus sur le BP à la place de D33/D34, et aucun BA non isolé équipé des écrêteurs d'origine. Au-delà de 3 m, la liaison SHALL passer par un tampon de bus qualifié ou attendre la génération 3.

Un BA préparé pour le module, dont les écrêteurs de ligne sont déposés, MUST NOT pouvoir être raccordé au bus sans son module. Pour cela :

- le connecteur côté isolé SHALL être d'une autre série que celui du bus ;
- le second connecteur de chaînage du BA SHALL être neutralisé et étiqueté.

#### Scenario: Niveaux avec un câble de 3 m

- **WHEN** un BA déporté est raccordé par son module et 3 m de câble AWG20 sur banc, à consommation maximale, avec un BP équipé d'écrêteurs de ligne de 50 pF au plus
- **THEN** la capacité côté bus SHALL être inférieure ou égale à 400 pF
- **AND** le niveau bas au MCF SHALL être inférieur ou égal à 0,85 V
- **AND** le niveau bas côté BA SHALL être inférieur à 0,48 V
- **AND** le temps de montée de 30 % à 70 % SHALL être inférieur ou égal à 1 µs

#### Scenario: Bus non préparé pour un MIB

- **WHEN** le BP du bus porte encore D33/D34 d'origine, ou qu'un BA non isolé du même bus porte ses écrêteurs d'origine
- **THEN** un MIB MUST NOT être mis en service sur ce bus avant le remplacement de ces écrêteurs

#### Scenario: Raccordement direct impossible

- **WHEN** un intervenant tente de raccorder au bus, sans module, un BA préparé pour le module
- **THEN** le connecteur MUST NOT pouvoir s'accoupler
- **AND** le second connecteur de chaînage du BA SHALL être neutralisé et porter l'étiquette « ne pas raccorder sans module »

#### Scenario: Câble plus long que 3 m

- **WHEN** un BA déporté demande un câble de bus de plus de 3 m
- **THEN** la liaison SHALL passer par un tampon de bus dont les niveaux et le temps de montée sont vérifiés sur banc avant la mise en service
