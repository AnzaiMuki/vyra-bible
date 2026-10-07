# Recherche d'un passage

Le moteur (`src/search/`) transforme ce que l'opérateur tape en un passage vérifié. Il ne dépend ni d'OBS ni de Qt. Il est relié au dock au Milestone 04 : aujourd'hui, seuls les tests l'utilisent.

**Pas dans ce milestone :** la recherche par mots-clés (« amour », « berger »), prévue en V1.x avec l'index plein texte.

## Formes acceptées

Majuscules, accents, espaces et points sont libres.

| Ce qu'on tape | Passage |
|---|---|
| `Jean 3:16`, `Jn 3 16`, `jn 3.16`, `Jean 3,16`, `Jn3:16`, `Jean 3 v 16`, `Jean chapitre 3 verset 16`, `John 3:16` | Jean 3:16 |
| `1 Co 13.4-7`, `1Co 13 4-7`, `I Co 13:4–7`, `1 Corinthiens 13:4 à 7`, `1ère Corinthiens 13:4-7` | 1 Corinthiens 13:4 à 7 |
| `Jean 3:16-4:2` | de Jean 3:16 à Jean 4:2 (à cheval sur deux chapitres) |
| `Jean 3` | tout le chapitre 3 |
| `Jean 3-4` | chapitres 3 à 4 |
| `Jude 5`, `Jude 5-7`, `Jude 1:5` | verset 5 (livres d'un seul chapitre : Abdias, Philémon, 2 Jean, 3 Jean, Jude) |
| `Genèse 1:1`, `genese 1 1`, `Gn 1:1`, `Genesis 1:1` | Genèse 1:1 (nom français, sans accent, abréviation, nom anglais) |

La virgule `3,16` est la notation française chapitre,verset. Les tirets longs `–` et `—` sont acceptés.

## Noms de livres

- Les abréviations françaises sont celles de la Segond elle-même : `No` (Nombres), `Ép` (Éphésiens), `Ha` (Habacuc), `Joë` (Joël), `1 S` (1 Samuel), `Jé`, `És`, `Éz`, `Hé`, `Ap`… Elles ont été vérifiées sur les renvois de la source (voir plus bas).
- Les noms anglais sont aussi reconnus, pour la KJV à venir. En cas de conflit entre les deux langues, le français l'emporte (`Jud` = Jude, `Judg` = Juges).
- Un nom ou une abréviation **exacte** l'emporte toujours sur un préfixe (`Job` est Job, pas « Josué »).
- Un **préfixe unique** est accepté (`Jea` = Jean). Un préfixe qui convient à plusieurs livres est refusé avec la liste des candidats (`Jo` : Job, Josué, Joël, Jonas ; `Ju` : Juges, Jude). On ne devine jamais : un mauvais verset à l'antenne serait pire qu'une question.
- `Phil` est Philippiens ; Philémon s'écrit `Phm` ou `Philem`.
- Tous les mots avant le premier chiffre forment le nom du livre (`Jn trois` est un livre inconnu).

## Réponses du moteur

| Statut | Signification | Affichage conseillé |
|---|---|---|
| `Ok` | Passage valide, tous ses versets existent | Preview possible |
| `Empty`, `NeedChapter` (`Jn`), `Incomplete` (`Jn 3:`, `Jn 3:16-`) | L'opérateur tape encore | Neutre, pas d'erreur rouge |
| `AmbiguousBook` | Plusieurs livres possibles (liste fournie) | Proposer les candidats |
| `UnknownBook`, `Syntax`, `Unsupported` (`Jn 3:16;18`) | Non compris | Erreur |
| `ChapterOutOfRange`, `VerseOutOfRange` | Numéro trop grand (le maximum valide est fourni) | « Jean 3 a 36 versets » |
| `ReversedRange` (`Jn 3:18-16`) | La fin précède le début | Erreur |
| `BookNotInModule` | La Bible chargée ne contient pas ce livre | Erreur |

Une fin de plage trop grande (`Jn 3:16-40`) est une **erreur**, pas un raccourcissement silencieux : l'opérateur doit voir ce qui sera affiché.

Le livre reconnu est fourni même en cas d'erreur, pour que l'interface puisse l'afficher pendant la frappe.

## Fonctions annexes

- `passageVerses` : les versets d'un passage, dans l'ordre de lecture.
- `formatPassage` : forme imprimée, par exemple `Jean 3:16–18`, `Jean 3`, `Jean 3:16–4:2`, `Jude 5`, `1 Corinthiens 13:4–7`.
- `suggestBooks` : livres dont le nom commence par ce qui est tapé (complétion).

## Vérification

- **Aller-retour sur toute la Bible :** chacun des 31 170 versets, imprimé par `formatPassage` puis relu par `resolveQuery`, donne le même passage ; de même pour chacun des 1 189 chapitres.
- **Renvois réels de la Segond :** les notes de la source contiennent 18 927 renvois (1 152 couples abréviation-chapitre, 61 abréviations pour 61 livres). Chacun doit se résoudre vers un verset qui existe. Une abréviation attribuée au mauvais livre échouerait sur de nombreux renvois.
- **45 couples non résolus, qui semblent tous être des coquilles de la source :** par exemple Apocalypse 19:16 renvoie à « 1 Th 6:15 » alors que 1 Thessaloniciens n'a que 5 chapitres (le texte cité, « le Roi des rois », est en 1 Timothée 6:15) ; une note de 1 Pierre 4:9 renvoie à « 2 Pi 4:8 » alors que 2 Pierre n'a que 3 chapitres (probablement 1 Pierre 4:8) ; la Genèse renvoie à « 2 Ch 17:34 » (probablement 2 Rois 17:34). J'ai retrouvé 9 de ces 45 cas tels quels dans la source ; les 36 autres suivent le même profil (seul un numéro dépasse, jamais un livre inconnu ou ambigu) mais n'ont pas été examinés un par un, et les corrections proposées sont des déductions, pas des vérifications. Ils sont figés dans `tests/data/segond_crossref_anomalies.tsv` : le test échoue si cet ensemble change. Ces coquilles sont dans les notes de renvois, pas dans le texte des versets.
- 200 000 requêtes aléatoires (plausibles, abîmées, octets invalides) ne provoquent ni plantage ni erreur mémoire (AddressSanitizer et UBSan) ; tout passage `Ok` est un vrai intervalle de versets.
- Vitesse : environ 0,4 microseconde par requête (compilation Release, machine de développement Linux), très en dessous de l'objectif de 50 ms.
- Contrôle de sensibilité : attribuer volontairement l'abréviation de Marc à Matthieu fait échouer les tests (conflit détecté, `Mt` inconnu, renvois en échec).

## Limites connues

- Le moteur suit la numérotation de la Segond (titres de psaumes numérotés). Une autre traduction avec une autre numérotation donnera d'autres limites de versets.
- Les listes (`Jn 3:16,18` en notation anglaise, `Jn 3:16; 4:2`) ne sont pas gérées ; `;` est refusé explicitement, et la virgule suit la notation française.
- Les mots tapés avant le premier chiffre sont tous pris pour le nom du livre : `Jean verset 16` sans chapitre n'est pas compris.

## Saisie abrégée (suivre une lecture)

Quand un passage est dans le Preview, les numéros seuls sont relatifs à lui (le chapitre où il se termine) :

| Tapé | Résultat (Preview = Jean 3:16) |
|---|---|
| `17`, `:17`, `v17`, `verset 17` | Jean 3:17 |
| `17-19`, `17 à 19` | Jean 3:17–19 |
| `4:1`, `4 1`, `4.1` | Jean 4:1 (même livre) |
| `Jn 17` | Jean 17 en entier (un nom de livre n'est jamais relatif) |

Sans passage dans le Preview, un numéro seul n'est pas compris (rien n'est deviné). Un numéro hors limites est refusé avec le maximum (« ce chapitre n'a que 36 versets »). Dans un livre d'un seul chapitre (Jude), le numéro est un verset du chapitre 1.
