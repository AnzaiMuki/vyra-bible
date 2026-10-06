# VYRA Bible

Plugin OBS Studio gratuit, édité par **VYRA Concept**, pour afficher la Bible en direct avec une logique de régie : recherche d'un passage, **Preview**, puis **ON AIR**, sans jamais couper l'affichage.

> **État : Milestone 04 sur 14.** Le dock est utilisable au clavier : on tape « Jn 3 16 », Entrée l'envoie en Preview, Ctrl+Entrée en ON AIR, Haut/Bas change de verset dans le Preview, Échap masque le Program. Le texte s'affiche en brut dans les écrans du dock : **rien n'est encore envoyé à OBS** (pas de source, pas de NDI) et le rendu final arrive au Milestone 05. Testé en simulation sous Linux ; jamais lancé dans OBS ni sous Windows. Voir [CHANGELOG.md](CHANGELOG.md).

## Architecture visée

Un PC « Bible » exécute OBS avec ce plugin et envoie son affichage (avec transparence) en NDI vers l'OBS principal, installé sur un autre PC. Ce dernier n'a besoin d'aucun plugin VYRA.

## Installer (utilisateur)

Voir [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Compiler (développeur)

Voir [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Licence

GPL-2.0-or-later (obligatoire : le plugin se lie à OBS Studio, lui-même sous GPL). Les textes bibliques ont leurs propres licences (voir [docs/BIBLES.md](docs/BIBLES.md)) : aucune traduction protégée n'est distribuée avec le plugin.
