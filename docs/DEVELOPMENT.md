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
| `src/search/book_catalog.*` | Les 66 livres : noms, abréviations, normalisation des accents |
| `src/search/passage_query.*` | Comprendre « Jn 3 16 » et le vérifier contre la Bible chargée |
| `src/stage/stage_controller.*` | Règles Preview/Program (sans Qt) : seul ON AIR change le direct |
| `ui/vyra_dock.*` | Dock opérateur : traduit clavier et clics en appels au contrôleur |
| `ui/stage_panel.*` | Écran 16:9 PREVIEW / PROGRAM |
| `data/locale/` | Textes fr-FR et en-US |
| `data/bibles/` | Bibles embarquées (installées avec le plugin) |
| `tests/` | Tests du code indépendant d'OBS (`bible_module_test`, `search_test`) et leurs données |
| `installer/` | Script Inno Setup |
| `scripts/` | Build du setup, conversion et audit des Bibles, extraction des renvois de test |

Les dossiers `bible/`, `search/`, `database/`, `presentation/`, `server/`, `obs/`, `themes/`, `settings/`, `overlay/` de la spécification seront créés quand un milestone en aura besoin, pas avant.

## Tests unitaires (Linux, macOS ou Windows, sans OBS)

```bash
cmake -S tests -B build_tests
cmake --build build_tests
ctest --test-dir build_tests --output-on-failure
```

Ils demandent un compilateur C++17 et CMake ; le test du dock (`dock_ui_test`) n'est construit que si Qt6 (Widgets, Test) est trouvé, et se lance avec la plateforme `offscreen`, sans écran ni OBS. Sous Linux, ajoutez `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` pour détecter les erreurs mémoire.

## Vérification faite sous Linux (sans OBS)

Le code se compile contre les en-têtes réels d'OBS et de Qt6, ce qui détecte les erreurs de syntaxe et d'API, pas les erreurs d'exécution dans OBS. Le chargement dans OBS se teste uniquement sur Windows : démarrez OBS, ouvrez le journal (Aide > Fichiers journaux) et cherchez `[vyra-bible] dock registered`.

## Checklist de test (sur Windows, jusqu'au Milestone 04)

1. Le build se termine sans erreur.
2. OBS démarre ; le journal contient `[vyra-bible] loading (version 0.4.0)` puis `dock registered`.
3. Menu Docks > VYRA Bible : le dock s'affiche avec Preview et Program en 16:9.
4. Redimensionner le dock : les écrans gardent leur ratio.
5. Le journal contient `Bible loaded: Louis Segond 1910 (LSG1910), 31170 verses` et le bas du dock affiche « Louis Segond 1910 chargée (31170 versets) ».
6. Fermer OBS : le journal contient `unloaded`, sans plantage.
7. Changer la langue d'OBS en français : les libellés passent en français.
8. Cliquer dans la barre de recherche, taper `Jn 3 16` : la ligne d'état affiche « Jean 3:16 (1 versets) ».
9. Entrée : Jean 3:16 apparaît dans PREVIEW, rien dans PROGRAM.
10. Ctrl+Entrée : le texte passe dans PROGRAM, cadre rouge. Si OBS intercepte cette combinaison, notez-le : c'est une information à corriger, pas à ignorer.
11. Haut/Bas : PREVIEW change de verset, PROGRAM ne bouge pas.
12. Échap : PROGRAM se vide (« Masqué : Jean 3:16 (prêt) »). Ctrl+Entrée avec la barre vide le remet.
13. Taper `Jn 3:99` : message rouge, et Entrée / Ctrl+Entrée ne changent rien.
