# VYRA Bible

Plugin OBS Studio gratuit, édité par **VYRA Concept**, pour afficher la Bible en direct avec une logique de régie : recherche d'un passage, **Preview**, puis **ON AIR**, sans jamais couper l'affichage.

> **État : Milestone 01 sur 14.** Le projet compile et le dock s'affiche, mais **aucune fonction biblique n'existe encore** : pas de texte, pas de recherche, pas d'envoi à l'antenne. Les boutons du dock sont volontairement désactivés. Voir [CHANGELOG.md](CHANGELOG.md).

## Architecture visée

Un PC « Bible » exécute OBS avec ce plugin et envoie son affichage (avec transparence) en NDI vers l'OBS principal, installé sur un autre PC. Ce dernier n'a besoin d'aucun plugin VYRA.

## Installer (utilisateur)

Voir [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Compiler (développeur)

Voir [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Licence

GPL-2.0-or-later (obligatoire : le plugin se lie à OBS Studio, lui-même sous GPL). Les textes bibliques ont leurs propres licences : aucune traduction protégée n'est distribuée avec le plugin.
