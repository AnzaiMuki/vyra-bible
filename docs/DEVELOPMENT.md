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
| `src/overlay/overlay_frame.*` | Ce que la page affiche, en données, et son JSON (sans Qt) |
| `src/overlay/http_routes.*` | Vocabulaire HTTP minimal : routes, têtes de réponse, SSE (sans Qt, sans socket) |
| `src/overlay/overlay_server.*` | Serveur local 127.0.0.1 (Qt Network) : page, `/state`, `/events` |
| `data/overlay/` | La page HTML/CSS/JS affichée par la Browser Source d'OBS |
| `src/obs/obs_source_setup.*` | Crée ou corrige la source Navigateur « VYRA Bible » (seul code qui crée des sources OBS) |
| `ui/vyra_dock.*` | Dock opérateur : traduit clavier et clics en appels au contrôleur |
| `ui/picker_panel.*` | Sélecteur Livres, Chapitres, Versets à la souris (ne décide rien : il signale les clics) |
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

Ils demandent un compilateur C++17 et CMake ; les tests du serveur (`overlay_server`, `overlay_demo`) demandent Qt6 Network ; les tests de la page (`overlay_page`, `overlay_e2e`) demandent Python avec Playwright et un Chromium (`pip install playwright`, puis `playwright install chromium`) ; le test du dock (`dock_ui_test`) n'est construit que si Qt6 (Widgets, Test) est trouvé, et se lance avec la plateforme `offscreen`, sans écran ni OBS. Sous Linux, ajoutez `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"` pour détecter les erreurs mémoire.

## Vérification faite sous Linux (sans OBS)

Le code se compile contre les en-têtes réels d'OBS et de Qt6, ce qui détecte les erreurs de syntaxe et d'API, pas les erreurs d'exécution dans OBS. Le chargement dans OBS se teste uniquement sur Windows : démarrez OBS, ouvrez le journal (Aide > Fichiers journaux) et cherchez `[vyra-bible] dock registered`.

## Checklist de test (sur Windows, jusqu'au Milestone 05 (et la sélection rapide))

1. Le build se termine sans erreur.
2. OBS démarre ; le journal contient `[vyra-bible] loading (version 0.6.0)` puis `dock registered`.
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
14. Le bas du dock affiche « Source navigateur OBS : http://127.0.0.1:17420/ » (le port peut être 17421 à 17429 si 17420 est pris). Le journal contient `overlay server listening on …`.
15. Dans OBS : Sources > + > Navigateur. URL = l'adresse affichée, largeur 1920, hauteur 1080, rien d'autre à changer (voir `docs/OVERLAY.md`). Le fond doit être transparent dans l'aperçu OBS.
16. Dans le dock : Ctrl+Entrée sur `Jn 3:16` : le lower third apparaît dans OBS avec un fondu. Entrée sur `Jn 3:17` ne change rien à l'écran d'OBS ; Ctrl+Entrée le remplace sans clignoter.
17. Échap : le lower third disparaît. Fermer puis rouvrir la source navigateur : elle affiche tout de suite l'état courant.
18. Taper `Jn 3:16` + Entrée, puis `17` + Entrée : Jean 3:17 en Preview, sans retaper le livre. Le champ est sélectionné après chaque envoi.
19. Taper `19-21`, puis `4:1` : Jean 3:19–21, puis Jean 4:1.
20. Sélecteur : cliquer Jn dans Livres, 3 dans Chapitres, 16 dans Versets : Preview. Maj+clic sur 18 : plage 16 à 18. Ctrl+clic ou double-clic sur un verset : ON AIR (cellule rouge).
21. Cliquer « Ajouter la source dans OBS » : la source « VYRA Bible » apparaît dans la scène courante (1920x1080) ; le message du dock le confirme. Recliquer : « déjà en place ». Changer de scène et recliquer : elle est ajoutée à cette scène. Si OBS a démarré sur un autre port, le clic corrige l'adresse.
22. Menu de thème du dock : Bandeau bas, Plein écran, Texte seul. Mettre Ps 119:1-40 à l'antenne : le texte est coupé en pages, le dock affiche « Page 1/N ».
23. PageDown / PageUp dans la barre de recherche (ou les boutons) : l'écran d'OBS change de page avec un fondu, le PREVIEW ne bouge pas.
24. Changer de thème pendant l'antenne : l'écran d'OBS change de style et revient à la page 1. Vérifier la lisibilité de chaque thème sur l'écran final.
25. OBS > Paramètres > Raccourcis : six lignes « VYRA Bible : … », sans touche attribuée. Les traduire (français / anglais) en changeant la langue d'OBS.
26. Attribuer une touche à « Mettre à l'antenne », mettre le focus sur une autre fenêtre (ou une autre partie d'OBS) : la touche met le passage du Preview à l'antenne.
27. Attribuer verset suivant/précédent, page suivante/précédente, Masquer : chacune agit comme le bouton correspondant du dock, et le verset à l'antenne ne change qu'avec ON AIR.
28. Fermer puis rouvrir OBS : les touches choisies sont toujours là. Noter tout conflit avec d'autres touches OBS.
29. Mettre `Jn 3:16` puis `Ps 23` à l'antenne : onglet Historique, « Psaumes 23 » au-dessus de « Jean 3:16 ». Un simple Preview n'ajoute rien.
30. Mettre un passage en Preview, cliquer « ☆ Favori » (ou Ctrl+D) : l'étoile se remplit, l'onglet Favoris le liste. Clic = Preview, double-clic = ON AIR.
31. Fermer et rouvrir OBS : historique et favoris sont toujours là. Le fichier est `library.txt` dans le dossier de configuration du plugin (ouvrir via %APPDATA%\obs-studio\plugin_config\vyra-bible\ ; à confirmer).
32. Lire les onglets sur le thème sombre ET le thème clair d'OBS : texte lisible, élément sélectionné visible.
33. Sur le PC de l'église : mesurer le temps de démarrage d'OBS avec et sans le plugin (chronomètre, 3 essais), puis la charge processeur d'OBS (Gestionnaire des tâches) pendant une diffusion, avec ON AIR sur un long passage. Noter les chiffres : `docs/PERFORMANCE.md` n'a que des mesures Linux.
34. Regarder le dock sous les thèmes d'OBS (Yami, Acri, Rachni, clair) et avec l'échelle Windows à 125 % / 150 % : rien de coupé ni illisible, le dock reste bien à 300 px de large. Comparer l'ambiance avec VYRA Studio (docs/DESIGN.md).
