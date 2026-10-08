# VYRA Bible

Plugin OBS Studio gratuit, édité par **VYRA Concept**, pour afficher la Bible en direct avec une logique de régie : recherche d'un passage, **Preview**, puis **ON AIR**, sans jamais couper l'affichage.

> **État : Milestone 14 sur 14 (version 0.14.0, candidate à validation).** Toutes les fonctions prévues sont écrites et testées sous Linux (tests automatiques, vrai Chromium, vrais sockets) ; le plugin compile avec le vrai OBS 31.1.1 sous Windows et macOS (GitHub CI) et le setup se construit. **Il n'a jamais été lancé dans OBS ni sous Windows, et la transparence NDI n'est pas encore validée** : la liste de contrôle de [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) est à dérouler sur une vraie machine avant tout usage en direct.

## Fonctions

- Recherche rapide (« Jn 3:16 », puis « 17 », « 17-19 », « 4:1 ») et sélecteur Livres / Chapitres / Versets.
- **Preview** puis **ON AIR** : seul ON AIR change ce qui est à l'antenne ; un échec ne change rien ; Échap retire l'affichage sans perdre le passage.
- Overlay transparent (source Navigateur, créée par un bouton), fondu sans clignotement, trois thèmes (bandeau bas, plein écran, texte seul), pagination des longs passages.
- Historique et favoris, six raccourcis globaux OBS.
- Interface aux couleurs de VYRA Studio, français et anglais.

## Documentation

- [Guide de l'opérateur](docs/GUIDE.md) : utilisation en régie.
- [Installation](docs/INSTALLATION.md) · [Architecture](docs/ARCHITECTURE.md) · [Overlay](docs/OVERLAY.md) · [Recherche](docs/SEARCH.md) · [Bibles](docs/BIBLES.md) · [Design](docs/DESIGN.md) · [Performances](docs/PERFORMANCE.md) · [Développement](docs/DEVELOPMENT.md) · [Changelog](CHANGELOG.md)

## Architecture visée

Un PC « Bible » exécute OBS avec ce plugin et envoie son affichage (avec transparence) en NDI vers l'OBS principal, installé sur un autre PC. Ce dernier n'a besoin d'aucun plugin VYRA.

## Installer (utilisateur)

Voir [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Compiler (développeur)

Voir [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Licence

GPL-2.0-or-later (obligatoire : le plugin se lie à OBS Studio, lui-même sous GPL). Les textes bibliques ont leurs propres licences (voir [docs/BIBLES.md](docs/BIBLES.md)) : aucune traduction protégée n'est distribuée avec le plugin.
