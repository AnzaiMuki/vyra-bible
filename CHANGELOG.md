# Changelog

## [0.11.0] - 2026-10-08 - Design du dock (VYRA Studio)

### Modifié
- Le dock suit le design de VYRA Studio : palette relevée sur la capture de référence, en-tête avec la marque VYRA Concept (dessinée en code), cartes arrondies pour Preview et Program, thème choisi par un contrôle en pilule, onglets en pilule, pied d'état avec point vert/rouge, champ de recherche à focus bleu. Voir `docs/DESIGN.md`.
- Le bouton Favori devient une étoile à côté du champ de recherche ; les boutons de page (◀ ▶) n'apparaissent que quand le passage à l'antenne a plusieurs pages. Le dock tient à 300 px (minimum mesuré 318 px).
- Les cases du sélecteur étaient mal dessinées (fond clair) sur certaines cases : corrigé.

### Vérifié (Linux, sans OBS)
- `dock_ui_test` (162 vérifications), dont un dock étroit (300 px) : aucun bouton écrasé, largeur minimale ≤ 330 px. Contrôle visuel des rendus Qt hors écran (voir `docs/dock-apercu.png`).

### Non vérifié
- L'aspect réel dans OBS (thèmes Yami, Acri, Rachni, clair), les polices Windows (Segoe UI, Cascadia Mono), l'échelle d'affichage 125/150 %.
- Les contrastes n'ont pas été mesurés avec un outil d'accessibilité.

## [0.10.0] - 2026-10-08 - Milestone 10 : optimisation (mesures)

### Ajouté
- `docs/PERFORMANCE.md` : mesures de chaque geste de l'opérateur et du démarrage, et manière de les refaire.
- Deux outils de mesure (pas des tests, ils n'échouent jamais) : `tests/dock_bench.cpp` (le dock) et `tests/overlay_bench.py` (la page overlay dans Chromium).

### Conclusion
- **Aucune modification du plugin** : les mesures montrent que tout est sous 16 ms (le pire geste, un changement de livre dans le sélecteur, prend 16 ms ; le chargement de la Bible 14 ms et environ 5 Mo). Optimiser sans mesure qui le justifie aurait ajouté du risque pour rien.
- Une première version de la mesure de la page overlay donnait 0 ms : la page ignore un message identique au précédent. Corrigé en variant la référence à chaque appel ; le commentaire dans l'outil l'explique.

### Non vérifié
- Tout ce qui se passe dans OBS : source Navigateur (CEF), NDI, Windows, PC modeste, diffusion en cours. Les chiffres sont ceux d'une machine Linux de développement.

## [0.9.0] - 2026-10-08 - Milestone 09 : historique et favoris

### Ajouté
- **Historique** : chaque passage mis à l'antenne (ON AIR, double-clic, Ctrl+clic) est mémorisé, le plus récent en haut, sans doublon, 50 au maximum. Prévisualiser n'ajoute rien.
- **Favoris** : bouton « ☆ Favori » ou Ctrl+D ajoute / retire le passage du Preview (200 au maximum, message clair quand la liste est pleine).
- Deux onglets à côté du sélecteur : Historique et Favoris. Clic = Preview, double-clic = ON AIR. Bouton « Effacer l'historique » (les favoris restent).
- Sauvegarde dans le dossier de configuration d'OBS du plugin (`library.txt`, texte simple), réécrite à chaque changement, par fichier temporaire puis renommage : un plantage ne laisse jamais un demi-fichier. Les lignes illisibles ou qui ne correspondent plus à la Bible chargée sont ignorées et signalées en rouge.
- Code Qt-free dans `src/library/passage_library.*`.

### Corrigé
- Défaut trouvé par les tests (AddressSanitizer) avant publication : vider une liste pendant son propre signal de clic provoquait un accès à de la mémoire libérée. Les clics de listes sont maintenant traités après le retour du signal.

### Vérifié (Linux, sans OBS)
- `library_test` (277 vérifications) : ordre, doublons, plafonds, validité contre la Bible, aller-retour du fichier, fichiers abîmés ou étrangers, dossier non inscriptible, remplacement atomique ; deux altérations volontaires (doublons d'historique tolérés, validation retirée) sont détectées.
- `dock_ui_test` (146) : l'historique ne retient que l'antenne, favoris sans effet sur l'antenne, clic/double-clic, persistance entre deux docks, fichier abîmé, chemin impossible, plafond des favoris.

### Non vérifié
- L'emplacement réel du fichier (`obs_module_config_path` sous Windows), jamais exécuté. Le rendu des onglets sur le thème sombre d'OBS n'a pas été examiné.
- Un double-clic réel à la souris (les tests simulent clic puis double-clic).

## [0.8.0] - 2026-10-08 - Milestone 08 : raccourcis clavier OBS

### Ajouté
- Six raccourcis globaux dans OBS (Paramètres > Raccourcis, « VYRA Bible : … ») qui marchent même quand le dock n'a pas le focus : ON AIR, Masquer, verset suivant / précédent (Preview), page suivante / précédente (antenne). **Aucune touche n'est attribuée par défaut** : l'opérateur choisit, le plugin ne peut donc voler la touche d'aucun autre plugin. Les touches sont sauvegardées avec le profil OBS.
- Les raccourcis passent par la même fonction que les boutons (`VyraDock::perform`) : mêmes règles, seul ON AIR change le verset à l'antenne.
- OBS appelle les raccourcis depuis son propre thread ; l'action est transmise au thread de l'interface par un appel en file d'attente, et seule la pression de la touche compte (pas le relâchement).

### Vérifié (Linux, sans OBS)
- `dock_ui_test` : chaque action via `perform` (rien ne se passe sans Preview, les pas ne touchent pas l'antenne, Masquer deux fois n'envoie rien) ; une altération volontaire (verset suivant branché sur ON AIR) est détectée.
- Compilation avec avertissements activés contre les vrais en-têtes d'OBS.

### Non vérifié
- **Les raccourcis eux-mêmes** : `obs_hotkey_register_frontend`, la sauvegarde/chargement des touches et le passage du thread OBS au thread Qt n'ont jamais été exécutés (pas d'OBS ici). À tester sous Windows (liste de contrôle, points 25 à 28).

## [0.7.0] - 2026-10-07 - Milestone 07 : thèmes et pagination

### Ajouté
- **Trois thèmes** choisis dans le dock : Bandeau bas, Plein écran, Texte seul. Changer de thème ne change pas ce qui est à l'antenne, mais ramène le passage à la page 1.
- **Pagination** : un passage trop long pour un écran est coupé en pages (260 caractères pour bandeau/texte seul, 650 pour plein écran), aux espaces, sans perte de texte ; un verset coupé est signalé comme « suite ». La police est ensuite ajustée pour que la page tienne exactement.
- **Navigation par pages** : boutons « ◀ Page » / « Page ▶ », touches PageUp / PageDown dans la barre de recherche, indicateur « Page 2/5 ». Les pages ne bougent que le PROGRAM, jamais le PREVIEW. Mettre un passage à l'antenne commence toujours à la page 1.
- Le message envoyé à la page overlay contient maintenant `theme`, `page`, `pages` et les versets de la page.

### Vérifié (Linux, sans OBS)
- `paginator_test` : aucune perte de texte, coupures aux espaces, budgets respectés ; une altération volontaire (budget du bandeau à 900) est détectée.
- `stage_test`, `overlay_test`, `overlay_server_test` mis à jour ; `dock_ui_test` : vraies touches et vrais clics (PageUp/PageDown, boutons, thème, pages sans effet sur le Preview, aucun envoi aux bornes, touches inactives si masqué) ; une altération volontaire (page changée sans prévenir l'overlay) est détectée.
- `overlay_page_test` (Chromium) : 64 vérifications ; la plus petite police sur les pages les plus pleines reste lisible (environ 4,2 % de la hauteur de l'écran au minimum).
- Compilation sans erreur contre les vrais en-têtes d'OBS.

### Non vérifié
- Tout ce qui demande OBS, Windows, MSVC, le NDI et la transparence réelle, ainsi que les workflows GitHub. Le rendu des thèmes n'a été vu que dans Chromium sous Linux, pas dans le navigateur intégré d'OBS.
- La lisibilité réelle à distance (taille d'écran, projection) : à juger sur place.

## [0.6.0] - 2026-10-07 - Milestone 06 : intégration OBS

### Ajouté
- Bouton « Ajouter la source dans OBS » : crée la source Navigateur « VYRA Bible » (1920x1080, adresse du serveur local) dans la scène courante, la remet à la bonne adresse si le port a changé, ou l'ajoute à la scène courante si elle existe ailleurs. Ne supprime ni ne renomme rien ; refuse de toucher une source d'un autre type qui porte déjà ce nom. Le code OBS est isolé dans `src/obs/obs_source_setup.*`.

### Vérifié
- Le bouton du dock (visible seulement si une action lui est donnée, affiche son message) : `dock_ui_test`, 100 vérifications.
- Les nouveaux fichiers compilent sans avertissement contre les vrais en-têtes d'OBS.

### Non vérifié
- **Tout le comportement réel** : aucun appel à libobs (création de la source, ajout à la scène, mise à jour) n'a été exécuté, il n'y a pas d'OBS ici. Les options de la source (`url`, `width`, `height`, `shutdown`, `restart_when_active`) viennent de ma connaissance de la source Navigateur, non d'un essai. La méthode manuelle reste valable.
- Le fond transparent, le NDI, Windows, les workflows GitHub (voir les versions précédentes).

## [0.5.1] - 2026-10-07 - Sélection rapide des versets

Demandé après un premier essai : le plugin n'était pas assez rapide pour suivre un prédicateur.

### Ajouté
- **Saisie abrégée** : après « Jn 3:16 », taper seulement `17` + Entrée donne Jean 3:17 ; `17-19`, `v17`, `4:1` fonctionnent aussi (voir `docs/SEARCH.md`). Un nom de livre n'est jamais relatif.
- Après chaque envoi, le champ de recherche est **sélectionné** : le numéro suivant remplace le précédent, sans effacer.
- **Sélecteur à la souris** sous les boutons : Livres, Chapitres, Versets. Clic = Preview ; Maj+clic = plage depuis le dernier verset cliqué ; Ctrl+clic ou double-clic = ON AIR. Les versets du Preview sont en bleu, ceux à l'antenne en rouge ; le sélecteur suit aussi ce que l'on tape et les flèches Haut/Bas.
- Les clics du sélecteur ne prennent pas le focus : le clavier reste dans la barre de recherche.

### Vérifié
- Environ 280 vérifications de plus dans `search_test` : toutes les formes abrégées, les erreurs, l'absence de contexte, et chacun des 31 170 versets atteint par son seul numéro. Une altération volontaire (mauvais chapitre de contexte) est détectée.
- `dock_ui_test` (97 vérifications, vraies touches et vrais clics simulés) : suivre une lecture au clavier, Livres, Chapitres, Versets, Maj+clic, Ctrl+clic, double-clic, couleurs bleu/rouge, un chiffre impossible ne change rien, un clic de sélecteur n'envoie rien à l'antenne.
- Un défaut trouvé par ces tests et corrigé : après un changement de chapitre, les anciennes cellules restaient trouvables sous leur nom et un clic pouvait viser le mauvais chapitre.

### Non vérifié
- Tout ce qui touche OBS et Windows (voir les versions précédentes). Le rendu du sélecteur n'a été vu que sous le thème clair de Qt sous Linux, pas dans le thème sombre d'OBS.
- Le gain de vitesse réel en situation de culte n'est pas mesuré : il se juge à l'usage.
- Pas encore de recherche par mots, d'historique ni de favoris (Milestone 09).

## [0.5.0] - 2026-10-07 - Milestone 05 : overlay pour OBS

### Ajouté
- Une page overlay transparente (`data/overlay/`) : lower third, fondu entre versets sans passer par le vide, texte réduit pour tenir dans le cadre, réajustement si OBS redimensionne la source. Détail dans `docs/OVERLAY.md`.
- Un serveur local (`src/overlay/`), 127.0.0.1 uniquement, GET uniquement, trois fichiers fixes plus `/state` et `/events` (Server-Sent Events) ; port 17420, ou le premier libre jusqu'à 17429. Une page qui se connecte reçoit l'état courant tout de suite.
- Le dock publie le Program à chaque ON AIR / Masquer (et seulement alors : taper, Préc. et Suiv. ne touchent jamais l'overlay). Le bas du dock affiche l'adresse à coller dans la source Navigateur d'OBS.
- Le texte de la Bible est toujours inséré comme du texte (jamais du HTML) et échappé dans le JSON.

### Vérifié
- `tests/overlay_test.cpp` (2 439 vérifications) : les 31 170 versets passent par le JSON et reviennent identiques (lecteur JSON écrit séparément) ; caractères spéciaux, U+2028/2029 ; routes HTTP, 20 000 requêtes aléatoires ; ASan/UBSan.
- `tests/overlay_server_test.cpp` (55 vérifications, vrais sockets) : fichiers servis octet pour octet, 404/405/400/431, requête découpée, client trop lent coupé, port suivant libre, refus de l'écoute hors boucle locale, 500 publications dans l'ordre, clients qui partent, limite de connexions. Trois altérations volontaires du code (écoute sur toutes les interfaces, publication supprimée, limite supprimée) sont bien détectées.
- `tests/overlay_page_test.py` (38 vérifications, vrai Chromium, captures à fond transparent) : fond transparent, fondu sans vide, ancienne image ignorée, balises HTML affichées en texte, texte qui tient en 1280x720, 3840x2160 et portrait, redimensionnement pendant l'antenne (un test a réellement échoué avant que j'ajoute ce réajustement ; retiré, il échoue de nouveau).
- `tests/overlay_e2e_test.py` (18 vérifications) : Chromium contre le vrai serveur et les vraies règles Preview/Program : le Preview ne sort jamais, ON AIR si, Masquer vide, une page ouverte en retard est à jour.
- Les sources du plugin (dont `plugin_main.cpp`) compilent sans avertissement contre les vrais en-têtes d'OBS.

### Non vérifié
- Tout ce qui touche OBS et Windows : la source Navigateur d'OBS (son moteur CEF diffère de Chromium), le fond transparent dans OBS, le NDI, le build MSVC, les workflows GitHub (le workflow « Tests » installe maintenant Playwright : jamais observé).
- Qt6 Network dans le Qt fourni par le modèle OBS : OBS l'utilise lui-même, donc très probable, mais non essayé.
- Un seul thème (lower third) ; pas de pagination des longs passages (les derniers versets sont retirés avec « … ») ; l'écran Preview du dock reste en texte brut (pas identique à l'overlay).
- La police est celle du système (Segoe UI sous Windows) ; non vue sous Windows.

## [0.4.0] - 2026-10-06 - Milestone 04 : interface opérateur

### Ajouté
- La barre de recherche du dock est active. Pendant la frappe, la ligne d'état dit où en est la référence : en cours (gris), prête avec le nombre de versets, ou erreur en rouge (livre inconnu, livre ambigu avec les candidats, chapitre ou verset hors limites avec le maximum, plage inversée, liste `;` non gérée).
- Raccourcis, barre de recherche active : Entrée = Preview ; Ctrl+Entrée = ON AIR (le passage tapé, ou le Preview si la barre est vide) ; Haut/Bas = verset précédent/suivant dans le Preview ; Échap = retirer le Program de l'antenne (le passage reste prêt).
- Boutons Précédent, Suivant, Masquer, ON AIR, activés seulement quand ils ont un sens. Ils ne prennent pas le focus, pour ne pas casser le flux clavier.
- `src/stage/stage_controller` (sans Qt ni OBS) : toutes les règles Preview/Program. Seul `takeOnAir()` modifie le Program ; taper, Précédent et Suivant ne touchent jamais le direct. Précédent/Suivant passent les frontières de chapitre et de livre, et sautent les livres absents d'une Bible.
- Si une référence tapée est invalide, Ctrl+Entrée n'envoie rien à l'antenne.
- Écrans Preview/Program : le texte du passage s'affiche en brut ; le cadre du Program devient rouge quand il est en direct.

### Vérifié
- `tests/stage_test.cpp` (règles Preview/Program, y compris une marche de bout en bout sur les 31 170 versets dans les deux sens, une Bible à livres manquants, des passages refusés) : passe sous ASan/UBSan.
- `tests/dock_ui_test.cpp` : le vrai dock, plateforme Qt « offscreen », avec de vraies touches simulées (Entrée, Ctrl+Entrée, Haut, Bas, Échap) et des clics : 36 vérifications. Un contrôle négatif (Ctrl inversé dans le code) fait bien échouer 5 d'entre elles.
- Compilation sans avertissement du dock avec Qt 6 sous Linux.

### Non vérifié
- Tout ce qui touche OBS et Windows : le dock n'a jamais été lancé dans OBS ; le build MSVC n'a pas été essayé ; les workflows GitHub n'ont pas été observés. Le comportement du focus clavier à l'intérieur d'un dock OBS (qui peut avoir ses propres raccourcis) n'est donc pas confirmé.
- Les raccourcis ne fonctionnent que quand la barre de recherche a le focus ; les raccourcis globaux OBS arrivent au Milestone 08.
- Le texte long (psaume 119) est coupé dans l'écran du dock : le rendu réel est le Milestone 05. Rien n'est encore envoyé à OBS : ON AIR ne change que l'état du dock.
- Les textes de l'interface sont écrits en français ; la version anglaise est écrite aussi mais n'a pas été relue en situation.

## [0.3.0] - 2026-10-05 - Milestone 03 : moteur de recherche

### Ajouté
- Moteur de recherche d'un passage (`src/search/`), sans dépendance à OBS ni Qt : « Jn 3 16 », « 1 Co 13.4-7 », « Jean 3:16-4:2 », chapitre entier, livres d'un seul chapitre, accents, noms français et anglais, abréviations de la Segond, préfixes uniques. Détail dans `docs/SEARCH.md`.
- Réponses précises : passage valide, frappe en cours, livre ambigu (avec candidats), numéro hors limites (avec le maximum), plage inversée.
- `passageVerses`, `formatPassage`, `suggestBooks` (versets d'un passage, forme imprimée, complétion).
- Tests (`tests/search_test.cpp`, 1 712 vérifications), extraction des renvois de la Segond (`scripts/extract_crossrefs.py`) et son contrôle dans le workflow « Tests ».

### Vérifié
- Aller-retour sur toute la Bible : les 31 170 versets et 1 189 chapitres, imprimés puis relus, redonnent le même passage.
- 18 927 renvois réels de la Segond : 1 152 couples abréviation-chapitre, 61 abréviations, tous résolus sauf 45 couples qui sont des coquilles des notes de la source (jamais un livre inconnu).
- 200 000 requêtes aléatoires sans plantage ni erreur mémoire (ASan/UBSan). Environ 0,4 microseconde par requête en Release.
- Un alias volontairement faux est bien détecté par les tests.
- Compilation sans avertissement (`-Wall -Wextra -Wpedantic -Werror`).

### Non vérifié
- Le build MSVC sous Windows des nouveaux fichiers (le modèle active `/utf-8`, nécessaire aux noms accentués du catalogue), et tout ce qui touche OBS. Les workflows GitHub n'ont pas encore été observés.
- Les 36 couples de renvois non résolus que je n'ai pas examinés un par un dans la source (9 l'ont été, voir `docs/SEARCH.md`) : leur profil est celui de coquilles, ce n'est pas une certitude.
- Le moteur n'est pas encore relié au dock : la barre de recherche reste désactivée jusqu'au Milestone 04.

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
