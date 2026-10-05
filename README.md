# VYRA Bible

Plugin OBS Studio gratuit, édité par **VYRA Concept**, pour afficher la Bible en direct avec une logique de régie : recherche d'un passage, **Preview**, puis **ON AIR**, sans jamais couper l'affichage.

> **État : Milestone 03 sur 14.** Le dock charge la Louis Segond 1910 et le moteur de recherche comprend « Jn 3 16 », « 1 Co 13.4-7 », etc. (testé), mais **il n'est pas encore relié au dock** : la barre de recherche, la Preview et ON AIR restent désactivées. Voir [CHANGELOG.md](CHANGELOG.md).

## Architecture visée

Un PC « Bible » exécute OBS avec ce plugin et envoie son affichage (avec transparence) en NDI vers l'OBS principal, installé sur un autre PC. Ce dernier n'a besoin d'aucun plugin VYRA.

## Installer (utilisateur)

Voir [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Compiler (développeur)

Voir [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Licence

GPL-2.0-or-later (obligatoire : le plugin se lie à OBS Studio, lui-même sous GPL). Les textes bibliques ont leurs propres licences (voir [docs/BIBLES.md](docs/BIBLES.md)) : aucune traduction protégée n'est distribuée avec le plugin.
