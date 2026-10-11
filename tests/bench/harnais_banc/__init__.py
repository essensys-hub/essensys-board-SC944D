"""Harnais des tests de non-régression sur banc du SC944D (tâche 7.2 du change
``i2c-surge-protection-2026-10-007``, issue essensys-hub/essensys-board-SC944D#1).

- ``registre`` : déclaration du banc (``ESSENSYS_BENCH``) et garde-fous du registre ``benches.yaml`` ;
- ``instruments`` : interfaces abstraites des instruments ;
- ``banc`` : façade du banc validé, journal des stimulus, opérations communes ;
- ``seuils`` : seuils nommés, rattachés aux exigences des specs ;
- ``analyse`` : fonctions d'analyse pures, vérifiées hors banc ;
- ``pilotes`` : pilotes concrets des instruments du banc (écrits avec la fiche du banc, tâche 7.1).
"""
