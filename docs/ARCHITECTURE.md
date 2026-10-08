# Architecture

Principe : la logique est séparée d'OBS et de Qt, pour qu'elle soit testable partout et que les couches fines qui touchent OBS restent petites.

```
 saisie ──► search ──► Passage ──► stage (Preview / Program, thèmes, pages)
                                        │
        ┌───────────────────────────────┼────────────────────────┐
        ▼                               ▼                        ▼
   ui (dock Qt)                 overlay (cadre JSON)       library (historique,
   boutons, touches,            ──► serveur HTTP local         favoris)
   sélecteur, onglets               127.0.0.1 : /state, /events
        │                               │
        ▼                               ▼
   plugin (OBS) : dock, raccourcis,   page web transparente (data/overlay)
   création de la source              = source Navigateur d'OBS
```

## Couches

| Dossier | Rôle | Dépend de |
|---|---|---|
| `src/bible/` | charge un module biblique (TSV) et donne le texte d'un verset | rien |
| `src/search/` | comprend ce qu'on tape, vérifie contre la Bible, formate | bible |
| `src/stage/` | machine d'état Preview/Program, thèmes, pagination | bible, search |
| `src/library/` | historique et favoris, fichier texte atomique | bible, search |
| `src/overlay/` | cadre envoyé à la page, routes HTTP, serveur local (Qt Network) | stage |
| `src/obs/` | tout ce qui appelle libobs : création de la source, raccourcis | stage |
| `ui/` | le dock Qt : widgets et style, aucune règle métier | stage, search, library |
| `src/plugin/` | point d'entrée OBS : assemble le tout, ne contient aucune logique | tout |
| `data/overlay/` | la page web (HTML, CSS, JS) affichée par la source Navigateur | — |

La partie « sans OBS » (bible, search, stage, library, cadre et routes) est compilée et testée seule ; seuls `ui/`, `src/overlay/overlay_server.*`, `src/obs/` et `src/plugin/` ont besoin de Qt ou d'OBS.

## Règles qui protègent l'antenne

- Seul `StageController::takeOnAir()` change le passage à l'antenne. Taper, naviguer, changer de page du Preview : rien de tout cela n'y touche.
- Une opération qui échoue ne change rien (ni Preview ni Program).
- Un passage absent de la Bible est refusé avant d'arriver à l'écran (`isInside`, `isValidPassage`).
- Les pages ne déplacent que le Program ; mettre un passage à l'antenne recommence à la page 1.
- La page overlay garde sa dernière image si la connexion tombe, et refuse d'exécuter du texte venant du message (tout est inséré comme texte, jamais comme HTML).

## Le chemin d'un verset, de la saisie à l'écran

1. `search::resolveQuery` transforme le texte en `Passage` vérifié.
2. Le dock le place en Preview (`StageController::showInPreview`) ; Ctrl+Entrée appelle `takeOnAir`.
3. Le dock prévient son « programListener » ; `plugin_main` fabrique un `Frame` (thème, page, versets) et le donne à `OverlayServer::publish`.
4. Le serveur envoie le cadre en Server-Sent Events à la page ; la page ajuste la police, fait le fondu et affiche.

## Tests

Voir [DEVELOPMENT.md](DEVELOPMENT.md). Sans cadriciel : `tests/check.hpp`. Les tests incluent des mutations volontaires (on casse le code pour vérifier que le test échoue) et des tests de robustesse (entrées aléatoires, clients HTTP hostiles) sous AddressSanitizer et UBSan.

## Ajouter une traduction

Une traduction est un fichier TSV (en-tête `#vyra-module`, puis `livre<TAB>chapitre<TAB>verset<TAB>texte`) chargé par `BibleModule`. Aujourd'hui une seule est chargée (`data/bibles/lsg1910.tsv`) ; le choix de plusieurs traductions n'est pas encore dans l'interface. Voir [BIBLES.md](BIBLES.md) pour les licences.
