# Changelog

## [0.2.0] - 2026-10-05 - Milestone 02 : chargement d'une Bible

### Ajouté
- Moteur de chargement des Bibles (`src/bible/`), sans dépendance à OBS ni Qt : format « module VYRA », recherche d'un verset par (livre, chapitre, verset), nombre de chapitres et de versets. Un fichier invalide est rejeté en entier, avec la ligne fautive dans le journal.
- Louis Segond 1910 embarquée (`data/bibles/lsg1910.tsv`, 31 170 versets), chargée au démarrage d'OBS ; l'état s'affiche en bas du dock (en rouge en cas de problème, sans jamais bloquer OBS).
- Convertisseur USFM (`scripts/usfm_to_vyra_tsv.py`) et audit indépendant (`scripts/audit_usfm_conversion.py`).
- Tests unitaires (`tests/`, 128 vérifications) et workflow GitHub « Tests ».
- Documentation des textes : `docs/BIBLES.md` (origine, licence déclarée, versification).

### Corrigé pendant le milestone
- Le convertisseur perdait du texte après une ligne vide dans les Psaumes (Psaume 23:1 incomplet). Corrigé, et détecté désormais par l'audit.
- Coquille de la source corrigée : Psaume 119:115.

### Vérifié
- 128 vérifications passent, y compris sous AddressSanitizer et UBSan : versets connus mot pour mot (Genèse 1:1, Jean 3:16, Psaume 23:1, Apocalypse 22:21), structure du canon (66 livres, 1 189 chapitres), 24 cas de fichiers invalides, fins de ligne Windows et BOM.
- Chargement complet de la Segond en 13 ms (machine de développement Linux).
- Compilation sans avertissement (`-Wall -Wextra -Wpedantic -Werror` sur le code du projet) contre les en-têtes d'OBS 31.1.1.
- Rendu du dock hors écran, états « chargée » et « erreur ».

### Non vérifié
- Tout ce qui touche à OBS sous Windows : build MSVC, chargement du plugin, résolution du chemin du fichier de Bible par `obs_module_file`, installateur. Les workflows GitHub ne se sont pas encore exécutés.
- La **relecture du texte contre une édition imprimée** : seuls des versets célèbres ont été comparés. Le texte vient d'une source tierce déclarée « Public Domain » ; ses corrections d'éditeur n'ont pas été examinées une à une.
- Les droits : voir `docs/BIBLES.md`.
- KJV et Fon : non ajoutées.

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
