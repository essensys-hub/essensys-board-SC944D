"""Pilotes concrets des instruments du banc d'essai.

Aucun pilote n'est livré tant que le banc n'est pas monté (tâche 7.1). Chaque pilote :

- hérite de l'interface de son rôle (``harnais_banc.instruments``) et en implémente toutes les
  méthodes abstraites ;
- ne joint que des adresses IP littérales du réseau de banc isolé déclaré dans le registre ;
- convertit ses horodatages dans la base de temps du harnais (``time.monotonic``) ;
- est référencé dans ``tests/bench/benches.yaml`` sous la forme
  ``harnais_banc.pilotes.<module>:<Classe>``.

Aucun pilote ne doit pouvoir joindre le serveur de production, ni une armoire en service.
"""
