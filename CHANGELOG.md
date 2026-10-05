# Changelog

## [0.1.0] - 2026-10-05 - Milestone 01 : initialisation

### Ajouté
- Projet créé à partir du modèle officiel `obs-plugintemplate` (OBS 31.1.1, Qt6), renommé `vyra-bible`.
- Plugin minimal : se charge dans OBS, enregistre un dock « VYRA Bible », journalise son chargement.
- Dock en disposition de régie : barre de recherche, écrans PREVIEW et PROGRAM en 16:9, boutons Précédent / Suivant / Masquer / ON AIR, ligne d'état. **Toutes les commandes sont désactivées.**
- Textes en français (fr-FR) et en anglais (en-US).
- Installateur Windows (Inno Setup), script de build PowerShell et workflow GitHub Actions. **Non testés** (voir docs/INSTALLATION.md).

### Vérifié
- Compilation sans avertissement (`-Wall -Wextra`, C++17) contre les en-têtes réels d'OBS 31.1.1 et de Qt 6.4, sous Linux.
- Rendu du dock hors écran (français et anglais, largeurs 420 et 640 px) : disposition et ratio 16:9 corrects.

### Non vérifié
- Chargement réel du plugin dans OBS sous Windows (build MSVC, dock visible dans OBS).
- Installateur, script de build et workflow CI.
- Aperçu identique à l'antenne dans le dock (piste CEF, prévue au Milestone 05).
