# VYRA Bible

Plugin OBS Studio gratuit, édité par **VYRA Concept**, pour afficher la Bible en direct avec une logique de régie : recherche d'un passage, **Preview**, puis **ON AIR**, sans jamais couper l'affichage.

> **État : Milestone 08 sur 14.** Le dock pilote une page overlay transparente (lower third) que l'on ajoute dans OBS comme source Navigateur : ON AIR l'affiche avec un fondu, changer de verset ne la fait pas clignoter, Échap la retire. Testé sous Linux avec un vrai Chromium et de vrais sockets ; **jamais lancé dans OBS ni sous Windows, et pas encore testé en NDI**. Un bouton crée la source OBS à votre place. Sélection rapide : après « Jn 3:16 », taper seulement « 17 » (ou « 17-19 », « 4:1 ») ; sélecteur Livres, Chapitres, Versets à la souris. Trois thèmes (bandeau bas, plein écran, texte seul) et pagination des longs passages (PageUp/PageDown). Six raccourcis globaux OBS, à attribuer dans les paramètres d'OBS. Voir [CHANGELOG.md](CHANGELOG.md) et [docs/OVERLAY.md](docs/OVERLAY.md).

## Architecture visée

Un PC « Bible » exécute OBS avec ce plugin et envoie son affichage (avec transparence) en NDI vers l'OBS principal, installé sur un autre PC. Ce dernier n'a besoin d'aucun plugin VYRA.

## Installer (utilisateur)

Voir [docs/INSTALLATION.md](docs/INSTALLATION.md).

## Compiler (développeur)

Voir [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Licence

GPL-2.0-or-later (obligatoire : le plugin se lie à OBS Studio, lui-même sous GPL). Les textes bibliques ont leurs propres licences (voir [docs/BIBLES.md](docs/BIBLES.md)) : aucune traduction protégée n'est distribuée avec le plugin.
