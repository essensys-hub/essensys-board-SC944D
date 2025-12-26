# 💻 Documentation Firmware

Le firmware du module SC944D est basé sur le RTOS **MQX** et tourne sur un microcontrôleur **ColdFire MCF52259**.

## Environnement de Développement

*   **IDE** : CodeWarrior for ColdFire (impliqué par les fichiers `.launch` et `.cproject`).
*   **OS** : MQX RTOS.
*   **Langage** : C.

## Structure du Projet

Le projet est stocké dans le répertoire `Prog/099-37/BP_MQX_ETH`.
Les sources sont organisées en :

*   `C/` : Fichiers sources (.c).
*   `H/` : Fichiers d'en-tête (.h).

## Tâches (Tasks)

Le système est découpé en plusieurs tâches exécutées par le RTOS :

| ID | Nom | Priorité | Stack | Description |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **Main** | 8 | 1596 | Tâche principale (Auto-start). |
| 2 | **Ecran** | 8 | 1500 | Gestion de l'affichage / Dialogue écran. |
| 3 | **I2C** | 8 | 1796 | Communication avec les boîtiers auxiliaires. |
| 4 | **TeleInf** | 8 | 1396 | Réception télé-information (Compteur Linky). |
| 5 | **Ethernet**| 8 | 3000 | Gestion réseau et serveur Web. |

## Mapping I/O (Hard.h)

Les Entrées/Sorties sont définies dans `H/Hard.h`.
(Cette section serait complétée avec les définitions exactes des macros comme `LED_1`, `RELAY_1`, etc. extraites de `Hard.h`).

## Mapping Mémoire

Le projet utilise :
*   **Flash Interne** : Code et constantes.
*   **MRAM/EEPROM** : Sauvegarde des paramètres (Mac Address, Soft Params, etc.).
