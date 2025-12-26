# 💻 Documentation Firmware

Le firmware du module SC944D est basé sur le RTOS **MQX** et tourne sur un microcontrôleur **ColdFire MCF52259**.

## Environnement de Développement

*   **IDE** : CodeWarrior for ColdFire.
*   **OS** : MQX RTOS.
*   **Langage** : C.

## Structure du Projet

*   `C/` : Sources (.c) dont `main.c`, `ba.c`, `Ethernet/www.c`.
*   `H/` : Headers (.h) dont `Hard.h`, `TableEchange.h`.

## Algorithmes Principaux

### 1. Démarrage du Client (`main.c`)

Au reset, la tâche **Main_task** exécute la séquence suivante :

1.  **Initialisation Hardware** : Appel de `vd_InitHard()` (configuration des I/O, PWM, ADC).
2.  **Lecture Configuration (EEPROM)** :
    *   Ouverture du port SPI (`vd_SpiOpen`).
    *   Lecture de l'adresse MAC.
    *   Lecture de la **Clé Serveur** (chiffrée).
    *   Lecture du **Code Alarme**.
3.  **Initialisation Applicative** :
    *   Initialisation des modules : Chauffage (`vd_Chauffage`), Alarme (`vd_AlarmeInit`), Timers.
4.  **Démarrage des Tâches** :
    *   `vd_StartTacheEcran()` : IHM locale.
    *   `vd_StartTacheBA()` : Communication I2C avec les cartes filles.
    *   `vd_StartTacheTeleinfo()` : Lecture compteur Linky.
    *   `vd_StartTacheEthernet()` : Connexion serveur.
5.  **Boucle Principale** : Surveillance alimentation et batterie (Watchdog soft).

### 2. Communication Inter-Board (SC940, SC941, etc.)

Le SC944D agit comme **Maître I2C** vis-à-vis des modules auxiliaires (BA).
Cette gestion est assurée par `C/ba.c` et `C/ba_i2c.c`.

*   **Protocole** : I2C Polled Mode.
*   **Cycle de Communication** :
    *   La tâche parcourt cycliquement tous les BA configurés (`uc_NB_BOITIER_AUXILIAIRE`).
    *   Elle compare l'état désiré (issu de `Tb_Echange`) avec le dernier état envoyé.
    *   Si diffèrent, elle envoie une trame `uc_TRAME_BA_ACTIONS`.
*   **Forçage** :
    *   Si un message prioritaire arrive (via Message Queue), une trame `uc_TRAME_BA_FORCAGE_SORTIES` est envoyée immédiatement (ex: coupure d'urgence).
*   **Trame Physique** : `[Code Commande] [Data...] [CRC]`

### 3. Connexion Serveur et Mise à Jour Firmware

La tâche Ethernet (`Ethernet/www.c`) gère la machine d'état réseau :

1.  **Connexion Réseau** : Vérification lien physique, obtention IP (DHCP ou Fixe), Résolution DNS de `mon.essensys.fr`.
2.  **Dialogue Serveur (`sc_DialogueAvecServeur`)** :
    *   **GET /api/serverinfos** (`sc_GetInformationServer`) : Récupère l'état de la connexion (`isconnected`), la présence d'une nouvelle version (`newversion`) et la liste des infos à envoyer.
    *   **POST /api/mystatus** (`sc_PostInformationServer`) : Envoie les statuts demandés par le serveur.
    *   **GET /api/myactions** (`sc_ActionManagment`) : Récupère les actions à exécuter (ex: pilotage relais).
3.  **Téléchargement Firmware (`sc_Download`)** :
    *   Si `newversion` != 0 dans la réponse serveur :
        *   Le système passe en mode "Download" (suspend I2C).
        *   Télécharge le binaire S19 via HTTP.
        *   Le stocke en Flash (zone réservée).
        *   Vérifie le CRC.
        *   Sauvegarde la `TableEchange` en Flash.
        *   **Reboot** (`vd_MCF52259_REBOOT`) pour appliquer la mise à jour (Bootloader).

### 4. Mise à jour Table de Référence et Statuts

Le système repose sur une table centrale **`Tb_Echange`** (`TableEchange.h`) qui contient tout l'état du système.

*   **Réception des Actions (Server -> SC944D)** :
    *   La fonction `sc_TraiterActions` reçoit un JSON du serveur (GUID + Paramètres).
    *   Elle parse les paires `(Index, Valeur)`.
    *   Elle met à jour `Tb_Echange` via `uc_TableEchange_Ecrit_Data(Index, Valeur)`.
    *   *Cas particulier* : Les scénarios sont appliqués en dernier pour garantir que tous les paramètres sont prêts.
    *   Acquittement : Envoi d'un `POST /api/done/{GUID}` au serveur.

*   **Envoi des Statuts (SC944D -> Server)** :
    *   Lors du `GET /api/serverinfos`, le serveur fournit une liste d'ID de données qu'il souhaite connaître (`infos: [10, 24, ...]`).
    *   Le SC944D répond via `POST /api/mystatus` en lisant ces ID dans `Tb_Echange` (`uc_TableEchange_Lit_Data`) et en construisant un JSON `{k:ID, v:Valeur}`.

## Mapping Mémoire

Le projet utilise :
*   **Flash Interne** : Code et constantes.
*   **MRAM/EEPROM** : Sauvegarde des paramètres (Mac Address, Soft Params, etc.).
