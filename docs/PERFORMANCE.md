# Performance

Mesures du Milestone 10. Principe : **mesurer avant d'optimiser**. Aucune de ces mesures n'a justifié de modifier le code du plugin : tout est sous le seuil d'une image à 60 Hz (16 ms), souvent très en dessous.

## Conditions

Machine de développement Linux (conteneur), compilation Release (`-O2`), Qt en mode `offscreen`, Chromium de Playwright. **Ce ne sont pas des mesures sous OBS ni sous Windows** : elles servent à repérer un ordre de grandeur et à comparer avant/après sur la même machine, pas à promettre un chiffre sur votre PC.

## Résultats

| Ce que l'opérateur fait ou ce qui se passe | Mesure |
|---|---|
| Chargement de la Louis Segond 1910 (31 170 versets, 4,5 Mo) au démarrage d'OBS | 14 ms |
| Mémoire prise par la Bible chargée | environ 5 Mo |
| Comprendre une saisie (« Jn 3:16 », « Ps 119:1-176 »…) | moins de 1 µs |
| Mettre en pages Psaumes 119 (59 pages) | 0,05 ms |
| Fabriquer et sérialiser le message envoyé à l'overlay | environ 1 µs |
| Création du dock + premier affichage | 30 ms |
| Taper un passage + Entrée (Preview) | 3 ms |
| Flèche Bas (verset suivant) | 1,4 ms |
| Ctrl+Entrée (ON AIR) | 2,2 ms |
| Saisie abrégée (« 17 » + Entrée) | 0,5 ms |
| Page suivante à l'antenne | 7 ms |
| Changer de livre (le pire cas : reconstruire les cases, ex. Psaumes) | 16 ms |
| Page overlay : ajuster la police et préparer l'affichage | 1 à 6 ms en médiane, 12 ms au pire |

## Ce qui n'est pas mesuré

- La source Navigateur d'OBS (CEF) : sa vitesse de rendu et son coût processeur sur un PC modeste.
- Le NDI : latence, bande passante, coût en processeur de l'encodage avec transparence.
- Toute mesure sous Windows, ou avec une vraie charge (OBS qui diffuse en même temps).
- Le temps de démarrage d'OBS avec le plugin : à comparer avec et sans plugin, sur la machine de l'église.

## Refaire les mesures

```
cmake -S tests -B build_rel -DCMAKE_BUILD_TYPE=Release
cmake --build build_rel --target dock_bench
QT_QPA_PLATFORM=offscreen build_rel/dock_bench      # affiche les millisecondes (ignorer les avertissements de traduction)
python3 tests/overlay_bench.py                      # page overlay, dans Chromium
```

`dock_bench` et `overlay_bench.py` ne sont pas des tests : ils n'échouent jamais, car les durées dépendent de la machine. Si un chiffre devient nettement pire après une modification, c'est un signal à examiner.

## Si un jour il faut optimiser

Dans l'ordre : (1) le changement de livre dans le sélecteur reconstruit toutes les cases (chapitres et versets) ; on pourrait les réutiliser. (2) L'affichage riche des deux moniteurs du dock est recomposé à chaque changement de page. Rien de cela ne gêne aujourd'hui ; ne pas le faire sans une mesure qui le justifie.
