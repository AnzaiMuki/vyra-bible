# Développement

## Prérequis (Windows 64 bits)

- Visual Studio 2022, charge de travail « Développement Desktop en C++ »
- CMake 3.28 ou plus récent
- Inno Setup 6.3 ou plus récent, pour le setup (`winget install JRSoftware.InnoSetup`)
- Internet au premier configure : le modèle télécharge les sources d'OBS 31.1.1 et Qt6.

## Compiler

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64 --config RelWithDebInfo
```

Pour tester sans installateur : `cmake --install build_x64 --config RelWithDebInfo --prefix release\stage`, puis copiez les fichiers comme décrit dans INSTALLATION.md (installation manuelle).

## Produire le setup

```powershell
.\scripts\build-installer.ps1
```

Résultat : `release\VYRA-Bible-Setup-<version>.exe`.

## Structure actuelle

| Chemin | Rôle |
|---|---|
| `src/plugin/plugin_main.cpp` | Point d'entrée OBS : enregistre et retire le dock |
| `src/plugin-support.*` | Journalisation `obs_log` (convention du modèle OBS, laissée à la racine de `src/`) |
| `src/bible/bible_module.*` | Chargement d'une Bible (module VYRA), sans dépendance à OBS ni Qt |
| `ui/vyra_dock.*` | Dock opérateur (disposition de régie) |
| `ui/stage_panel.*` | Écran 16:9 PREVIEW / PROGRAM |
| `data/locale/` | Textes fr-FR et en-US |
| `data/bibles/` | Bibles embarquées (installées avec le plugin) |
| `tests/` | Tests unitaires du code indépendant d'OBS |
| `installer/` | Script Inno Setup |
| `scripts/` | Build du setup, conversion et audit des Bibles |

Les dossiers `bible/`, `search/`, `database/`, `presentation/`, `server/`, `obs/`, `themes/`, `settings/`, `overlay/` de la spécification seront créés quand un milestone en aura besoin, pas avant.

## Tests unitaires (Linux, macOS ou Windows, sans OBS)

```bash
cmake -S tests -B build_tests
cmake --build build_tests
ctest --test-dir build_tests --output-on-failure
```

Ils ne demandent qu'un compilateur C++17 et CMake. Sous Linux, ajoutez `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` pour détecter les erreurs mémoire.

## Vérification faite sous Linux (sans OBS)

Le code se compile contre les en-têtes réels d'OBS et de Qt6, ce qui détecte les erreurs de syntaxe et d'API, pas les erreurs d'exécution dans OBS. Le chargement dans OBS se teste uniquement sur Windows : démarrez OBS, ouvrez le journal (Aide > Fichiers journaux) et cherchez `[vyra-bible] dock registered`.

## Checklist de test (sur Windows, jusqu'au Milestone 02)

1. Le build se termine sans erreur.
2. OBS démarre ; le journal contient `[vyra-bible] loading (version 0.2.0)` puis `dock registered`.
3. Menu Docks > VYRA Bible : le dock s'affiche avec Preview et Program en 16:9.
4. Redimensionner le dock : les écrans gardent leur ratio.
5. Le journal contient `Bible loaded: Louis Segond 1910 (LSG1910), 31170 verses` et le bas du dock affiche « Louis Segond 1910 chargée (31170 versets) ».
6. Fermer OBS : le journal contient `unloaded`, sans plantage.
7. Changer la langue d'OBS en français : les libellés passent en français.
