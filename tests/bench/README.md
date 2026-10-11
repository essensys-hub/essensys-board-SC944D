# Tests de non-régression sur banc du SC944D

Ce dossier contient le harnais pytest et les tests `NR-board-sc944d-1` à `-13` du change OpenSpec
`i2c-surge-protection-2026-10-007`, qui traite la protection contre les surtensions du bus I2C entre
le BP et les BA ([essensys-hub/essensys-board-SC944D#1](https://github.com/essensys-hub/essensys-board-SC944D/issues/1)).
Il correspond aux tâches 7.2 à 7.15 de `tasks.md` et à la décision D10 de `design.md`.

Ces tests pilotent un **banc d'essai** : un BP, trois BA (un de chaque type), des câbles représentatifs,
des charges factices et un serveur legacy local. Sans banc déclaré, ils sont tous « skipped ».

## Sécurité

- **Jamais d'armoire réelle.** Aucun test, aucun pilote et aucun script ne vise une armoire en service.
  Les opérations faites sur l'installation touchée le 9 août 2026 (groupes 1 et 4 de `tasks.md`) sont des
  opérations de maintenance manuelles, hors de ce harnais.
- **Cartes de banc uniquement.** Le registre refuse un banc dont une carte n'a pas la destination `banc`.
  Les courts-circuits et les défauts provoqués visent des échantillons de banc. Les injections (tâches 7.16
  à 7.19) ne visent que des cartes déclarées `echantillon_injection`.
- **Réseau isolé.** Le banc n'est jamais raccordé au serveur de production. Le registre refuse :
  - tout réseau de banc isolé qui n'est pas inclus dans une plage réservée aux essais : 198.18.0.0/15
    (RFC 2544) en IPv4, 2001:2::/48 (RFC 5180) en IPv6. Les plages privées des installations (10/8,
    172.16/12, 192.168/16, 100.64/10, 169.254/16) sont donc refusées ;
  - tout réseau de banc qui n'est pas inclus dans un réseau de banc isolé déclaré ;
  - toute cible réseau qui n'est pas une adresse IP littérale du réseau du banc. Sont contrôlés l'adresse
    du BP, le serveur legacy et, dans les paramètres des instruments, toute valeur rangée sous une clé de
    cible (`adresse`, `addr`, `host`, `hostname`, `hote`, `ip`, `serveur`, `server`, `url`, `endpoint`,
    `cible`…), toute URL, toute ressource VISA TCPIP et toute adresse IPv4 ou IPv6 citée. Un nom d'hôte est
    refusé ;
  - un pilote `maitre` dont le paramètre `url` n'a pas le même hôte et le même port que
    `reseau.serveur_legacy`.

  Avant tout autre instrument, le serveur legacy de banc doit annoncer l'identifiant du banc déclaré. Le
  pilote lit cet identifiant sur le serveur de banc, à un point d'accès dédié configuré côté serveur. Il ne
  le tire jamais de `self.banc`, de ses paramètres ni du registre. La revue de PR de chaque pilote vérifie
  ce point : c'est la seule barrière dynamique avant tout stimulus.
- **TBTS.** Le 0 V du banc n'est jamais relié au PE, ni directement ni par un instrument. Les sorties des BA
  ne portent aucune tension secteur. En pratique :
  - la console Espion et les programmateurs BDM/ICSP passent par un adaptateur USB isolé, ou par un PC
    portable sur batterie ;
  - l'oscilloscope a des voies isolées ou des sondes différentielles ; la pince de masse d'un instrument
    relié au PE ne touche jamais le 0 V du banc ;
  - l'alimentation de laboratoire a une sortie flottante ;
  - la fiche du banc (tâche 7.1) prévoit un contrôle de l'isolement entre le 0 V et le PE avant chaque
    session. Un 0 V relié au PE fausserait aussi les injections des tâches 7.16 à 7.18, dont le courant
    reviendrait par le PE.
- **Protocole HTTP legacy IoT inchangé.** Le serveur de banc capture les échanges et ne les modifie pas.
- **Firmware du BP.** Aucun build non qualifié n'est flashé sur un BP en service. Le programmateur du banc
  (NR-board-sc944d-11) ne programme que les cartes du banc. Un candidat n'est déployé en service qu'après
  la suite NR au vert et un `/checkup` vert (tâche 5.12).

## Déclarer le banc

Le banc se déclare par une variable d'environnement :

```bash
export ESSENSYS_BENCH=<identifiant>
```

L'identifiant est cherché dans le registre versionné [`benches.yaml`](benches.yaml).

| Situation | Comportement |
|---|---|
| `ESSENSYS_BENCH` absente ou vide (CI, nightly) | Les tests de banc sont « skipped ». Le registre n'est pas lu et aucun instrument n'est instancié. |
| Banc absent du registre | Arrêt en erreur (code de sortie 4), avant toute collecte et tout stimulus. |
| Banc d'exemple (`exemple: true`) ou banc non conforme | Arrêt en erreur (code de sortie 4), avec la liste des motifs, avant tout stimulus. |
| Banc reconnu | Instruments ouverts une fois par session. Chaque test part de l'état nominal du banc et y revient. |

Les tests dont le banc n'a pas les capacités requises sont « skipped », avec la capacité manquante en
raison du saut.

### Ajouter un banc au registre

1. Monter le banc et rédiger sa fiche (tâche 7.1).
2. Écrire les pilotes de ses instruments dans `harnais_banc/pilotes/`. Chaque pilote hérite de l'interface
   de son rôle (`harnais_banc/instruments.py`).
3. Ajouter l'entrée du banc dans `benches.yaml`, sur le modèle documenté de `exemple-fictif`, par une PR
   relue par un mainteneur. Les éléments à déclarer sont :
   - le réseau isolé et les adresses IP littérales ;
   - la topologie et l'ordre des BA ;
   - les sorties témoins et le volet témoin ;
   - les cartes, toutes de destination `banc` ;
   - les plages d'alimentation : `bp` de 22 à 24 V, `ba_seul` à 15,0 V au plus ;
   - la valeur de R155/R156 du BP ;
   - les capacités et les instruments ;
   - avec la capacité `firmware_essai_ba`, le BA d'essai (`ba_essai`) et la version de base de son firmware
     d'essai (`firmware_ba_essai`, « 1.7 » ou « 1.8 ») ;
   - les index et les en-têtes HTTP dont la valeur varie à état identique (`indices_http_variables`,
     `entetes_http_variables`).
4. Le dépôt est public : aucune donnée d'installation, aucun chemin local, aucun nom de personne.

Ajouter un réseau de banc isolé demande aussi une PR relue. Le réseau doit être physiquement isolé, sans
route vers Internet ni vers une installation, et inclus dans 198.18.0.0/15 ou 2001:2::/48.

La fiche du banc précise aussi :

- le BP du banc n'a pas de batterie VBATT raccordée, sinon la coupure de NR-board-sc944d-6 ne le redémarre
  pas (le test vérifie le démarrage par la console) ;
- le serveur de banc annonce un utilisateur connecté (`isconnected`) : le BP 099-37 scrute alors le
  serveur toutes les 2,15 s environ, au lieu de 5,15 s (`Ethernet/www.h`) ;
- l'état du cavalier J58 du BP du banc ;
- la position des points de mesure, notamment `+15V:port`, en amont du cavalier du port
  (`instruments.Oscilloscope`).

### Instruments (rôles du registre)

| Rôle | Interface | Usage |
|---|---|---|
| `alimentation` | `AlimentationLabo` | Alimentation flottante. Voie `bp` de 22 à 24 V, voie `ba_seul` à 15,0 V. Une consigne hors limites est refusée avant d'atteindre l'instrument. |
| `oscilloscope` | `Oscilloscope` | Niveaux bas, temps de montée, tensions continues et capacité de ligne aux points nommés (`seuils.point_*`). |
| `analyseur` | `AnalyseurLogique` | Trames décodées, intervalles bas, fronts et configuration acquittée, côté bus et côté BA d'un MIB. |
| `relais` | `RelaisBanc` | Câbles, déconnexions, mires de forçage, courts-circuits, isolement d'une carte, défauts du firmware d'essai, lecture des charges factices. |
| `maitre` | `MaitreCommande` | Serveur legacy de banc : commandes et instant de leur récupération par le BP, configuration, index remontés, capture HTTP avec la requête brute. |
| `console` | `ConsoleDiagnostic` | Console de diagnostic du BP (UART2, 9600 bit/s), en lecture : compteurs, démarrages, cycles TE2 datés, tentatives de récupération, commandes en attente ou expirées, tas. |
| `injection` (option) | `GenerateurInjection` | Tâches 7.16 à 7.19. N'injecte que sur un échantillon d'injection. |
| `programmateur` (option) | `ProgrammateurBanc` | NR-board-sc944d-11. Monte les firmwares sur les seules cartes du banc. |

### Capacités

| Capacité | Tests concernés |
|---|---|
| `cavaliers_par_port` | NR-2 (retrait par cavalier) et NR-9 (ouverture du cavalier d'un port en court-circuit) |
| `sorties_15v_par_port` | NR-9 (court-circuit d'un port) |
| `court_circuit_15v` | NR-9 (port et J31-4) |
| `court_circuit_vin_ba` | NR-9 (écrêteur d'un BA retouché en court-circuit, banc chaîné) |
| `firmware_essai_ba` | NR-3 (étirement d'horloge), NR-4 (esclave bloqué), NR-6 (EEPROM vierge, base 1.8), NR-8 (base 1.8) et NR-13 (PIC figé) |
| `teleinfo`, `scenario_temoin` | NR-3 (ligne forcée basse pendant 60 s) |
| `mib` | NR-13 (superviseur et niveaux du bus à 1 m et 3 m derrière le module) |
| `programmateur` | NR-11 |

## Lancer

Les commandes se lancent depuis la racine du dépôt. `pytest` et `pyyaml` sont fournis par `uv` :

```bash
# Sans banc : le test de garde et les tests d'analyse passent, les tests de banc sont « skipped ».
uv run --with pytest --with pyyaml pytest tests/bench -q --junitxml=.artifacts/sc944d-bench.xml

# Sur le banc déclaré, avec le journal des stimulus à joindre au rapport /checkup.
ESSENSYS_BENCH=<identifiant> ESSENSYS_BENCH_JOURNAL=.artifacts/sc944d-stimulus.jsonl \
  uv run --with pytest --with pyyaml pytest tests/bench --junitxml=.artifacts/sc944d-bench.xml

# Endurance de 72 h (NR-board-sc944d-12), seulement si elle est demandée.
ESSENSYS_BENCH=<identifiant> uv run --with pytest --with pyyaml pytest tests/bench --banc-endurance -k endurance
```

Les tests de banc longs durent plusieurs minutes chacun (absences de 5 min, forçages de 60 s, courts-circuits
de 60 s). Le journal `ESSENSYS_BENCH_JOURNAL` consigne au format JSON Lines :
- l'autorisation du banc ;
- chaque instanciation d'instrument ;
- chaque appel d'instrument ;
- la fermeture.

## Points d'attention des essais

- **Latence de scrutation HTTP.** Toute commande passe par le serveur de banc, que le BP scrute toutes les
  2,15 s environ (5,15 s sans utilisateur connecté). Cette latence entre dans le budget de 10 s des
  critères de reprise, mesurés depuis l'envoi de la commande.
- **Trafic soutenu.** `Banc.trafic` attend que le BP ait récupéré chaque tour de commandes avant le
  suivant : deux ordres contraires récupérés dans le même lot ne donneraient que l'état final, ce que la
  spec admet. Une commande est perdue si son état n'est pas appliqué avant que le BP ne récupère la
  commande suivante, ou dans les 10 s pour la dernière.
- **Câble.** `remettre_etat_nominal` remonte le câble nominal (3 × 33 cm) avant et après chaque test.
  NR-board-sc944d-1 applique les critères du coffret à 1 m et 3 m sur un bus non isolé (tâche 7.3) ; les
  critères du MIB sont vérifiés par NR-board-sc944d-13.
- **Cycle TE2 et NR-board-sc944d-3.** Le cycle d'alimentation des BA n'est permis qu'au-delà de 60 s de
  blocage. NR-3 relâche la ligne 60 s exactement après le début du forçage. Un cycle daté avant 60 s est un
  défaut ; un cycle à la limite est toléré et consigné (propriété `cycle_te2`), car il consomme le budget de
  3 cycles par 24 h du BP de banc.
- **Défauts d'essai.** Chaque ordre provoque le défaut une seule fois, à la première transaction du BP vers
  le BA. Les tests provoquent cette transaction par une commande, attendent le début du défaut, puis
  l'observent au-delà du délai de libération : un blocage encore en cours à la fin de l'observation est un
  défaut.
- **Échanges HTTP (NR-board-sc944d-11).** Les requêtes sont comparées brutes, octet pour octet. Seuls sont
  masqués le GUID de `/api/done/`, les index et en-têtes déclarés variables, et la valeur de
  `Content-Length`, rapportée à la longueur réelle du corps.

## JUnit XML et consolidation

Chaque test NR porte le marqueur et le commentaire lus par la non-régression Essensys :

```python
# NR: NR-board-sc944d-1 #1
@pytest.mark.nr("NR-board-sc944d-1", "#1")
def test_...(banc): ...
```

Le harnais écrit l'identifiant et l'issue dans la propriété `nr` de chaque cas du JUnit XML (`--junitxml`).
Il y ajoute l'identifiant du banc quand un banc est déclaré. La consolidation se fait avec :

```bash
python3 ../essensys-feature-lifecycle/scripts/feature_lifecycle/nonreg_report.py \
  .artifacts/sc944d-bench.xml --sources tests/bench \
  --out .artifacts/nonreg.json --markdown .artifacts/nonreg.md
```

Un identifiant est « passed » si tous ses cas passent, « failed » si l'un échoue. Il est « skipped » si
tous ses cas sont sautés, ce qui est le cas sans banc. Le CI et le nightly ne voient donc les tests de banc
ni réussir ni échouer, et aucun Bug `regression` n'est ouvert à tort. Les JUnit XML et le journal du banc
sont joints au rapport `/checkup` de la feature (tâche 7.20).

## Tests

| Identifiant | Fichier | Tâche | Objet |
|---|---|---|---|
| NR-board-sc944d-1 | `test_nr01_niveaux_i2c.py` | 7.3 | Niveaux I2C à 50 kHz (3 × 33 cm, 1 m, 3 m) et fuite par carte |
| NR-board-sc944d-2 | `test_nr02_retrait_ba.py` | 7.4 | Retrait d'un BA par son câble, puis par son cavalier |
| NR-board-sc944d-3 | `test_nr03_attentes_bornees.py` | 7.5 | Attentes bornées : SDA, puis SCL forcée basse 60 s ; étirement d'horloge d'un BA 1.7 |
| NR-board-sc944d-4 | `test_nr04_recuperation_bus.py` | 7.6 | Récupération de bus : esclave bloqué, forçage de 500 ms, défaut permanent |
| NR-board-sc944d-5 | `test_nr05_commandes_conservees.py` | 7.7 | Commandes conservées, état final seul, volet expiré |
| NR-board-sc944d-6 | `test_nr06_resynchronisation.py` | 7.8 | Resynchronisation : absence, redémarrage du BP, EEPROM vierge |
| NR-board-sc944d-7 | `test_nr07_telemetrie.py` | 7.9 | Index 12 : bit posé puis effacé, `0x1C` avec les trois BA absents |
| NR-board-sc944d-8 | `test_nr08_chien_de_garde_ba.py` | 7.10 | Chien de garde et erreurs SSP du BA 1.8 |
| NR-board-sc944d-9 | `test_nr09_court_circuit_15v.py` | 7.11 | Court-circuit du +15 V d'un port (cavalier fermé, puis ouvert), de l'écrêteur d'un BA, puis de J31-4 |
| NR-board-sc944d-10 | `test_nr10_branchement_a_chaud.py` | 7.12 | Branchement à chaud pendant un trafic soutenu |
| NR-board-sc944d-11 | `test_nr11_compatibilite.py` | 7.13 | Matrice BP 099-37/candidat × BA 1.7/1.8 et échanges HTTP |
| NR-board-sc944d-12 | `test_nr12_endurance.py` | 7.14 | Endurance de 72 h |
| NR-board-sc944d-13 | `test_nr13_superviseur_mib.py` | 7.15 | Superviseur du MIB, et niveaux du bus à 1 m et 3 m derrière le module (6.7) |

Ces tests ne passent par aucun instrument et s'exécutent toujours :

- `test_guard.py` : test de garde (tâche 7.2). Il lance pytest en sous-processus et vérifie deux choses :
  - sans banc, tous les tests de banc sont « skipped » et le journal des stimulus reste vide ;
  - avec un banc inconnu ou d'exemple, pytest sort en erreur sans stimulus.

  Il vérifie aussi les garde-fous du registre et des instruments.
- `test_analyse.py` : fonctions d'analyse pures de `harnais_banc/analyse.py`.

## Seuils

Les seuils sont des constantes nommées dans [`harnais_banc/seuils.py`](harnais_banc/seuils.py). Chacune
cite l'exigence de la spec qui la fixe. Les valeurs marquées « choix du harnais » sont des paramètres
d'essai qui ne viennent pas de la spec : fenêtres d'observation, tolérances, critère de stabilité du tas.
Elles sont à confirmer lors de la première campagne sur le banc et à faire relire.
