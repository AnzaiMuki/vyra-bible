# Design du dock

Le dock reprend le langage visuel de **VYRA Studio** (capture de référence fournie) à la densité d'un panneau OBS.

| Élément | Valeur |
|---|---|
| Fond du dock | `#0E1318` |
| Cartes (Preview / Program) | `#151C23`, bord `#293036`, coins arrondis |
| Écran de contrôle | `#06090C` |
| Contrôles (boutons, pilule) | `#212A31` |
| Accent (sélection, focus) | bleu `#2E9BFF` |
| État correct | vert `#33C77B` (point du pied d'état) |
| Texte atténué | `#5E6B78` / `#8795A3` |
| ON AIR / à l'antenne | rouge `#D92D20` : réservé à ce qui est vraiment en direct |

- **En-tête** : la marque VYRA Concept (dessinée en code, sans fichier image) + « VYRA Bible / AFFICHAGE LIVE ».
- **Pilules** : le choix du thème (Bandeau bas | Plein écran | Texte seul) est un contrôle segmenté, comme « Grille | Focus » dans VYRA Studio.
- **Cartes** : Preview et Program sont deux cartes avec une étiquette en capitales ; la carte Program passe au rouge quand le passage est à l'antenne.
- **Pied d'état** : point vert (tout va bien) ou rouge (problème) + message.
- **Harmonie avec OBS** : le dock est entièrement stylé, il a donc le même aspect sous tous les thèmes d'OBS ; il ne modifie rien en dehors de lui. Il reste utilisable à 300 px de large (largeur minimale mesurée : 318 px).

L'aperçu `docs/dock-apercu.png` est un rendu Qt hors écran sous Linux, pas une capture d'OBS. Les couleurs viennent d'un relevé de pixels de la capture de VYRA Studio.
