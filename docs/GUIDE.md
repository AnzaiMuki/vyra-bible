# Guide de l'opérateur

Ce guide explique comment utiliser VYRA Bible pendant un culte. Il décrit le comportement tel qu'il est testé en dehors d'OBS ; tout n'a pas encore été essayé dans OBS lui-même (voir l'état de vérification à la fin).

## Le principe : Preview puis ON AIR

Comme en régie, vous préparez avant d'envoyer.

- **PREVIEW** (à gauche) : ce que vous préparez. Rien n'est visible des fidèles.
- **PROGRAM** (à droite) : ce qui est à l'antenne. Un cadre rouge signale qu'un passage est en direct.
- **Seul ON AIR change l'antenne.** Taper, changer de verset, naviguer : tout cela ne touche que le Preview. On ne peut donc pas afficher un verset par accident en cherchant.

## Mise en place (une seule fois)

1. Installez le plugin (voir [INSTALLATION.md](INSTALLATION.md)) et cochez **Docks > VYRA Bible** dans OBS.
2. Cliquez **Ajouter la source dans OBS** : la source « VYRA Bible » (page transparente) est créée dans la scène courante. À refaire dans chaque scène où les versets doivent apparaître (le bouton ajoute la source existante, il n'en crée pas de doublon).
3. Si l'écran des fidèles est sur un autre PC (architecture NDI), envoyez la sortie de ce PC « Bible » en NDI avec transparence vers l'OBS principal ; l'OBS principal n'a besoin d'aucun plugin VYRA. Cette partie reste à valider sur votre matériel.

## Chercher un passage

Tapez dans la barre de recherche, puis :

| Touche | Effet |
|---|---|
| **Entrée** | le passage va dans le PREVIEW |
| **Ctrl+Entrée** | le passage va directement à l'antenne (ou, barre vide, le passage du Preview) |
| **Haut / Bas** | verset précédent / suivant (Preview seulement) |
| **PageHaut / PageBas** | page précédente / suivante du passage à l'antenne |
| **Échap** | retire l'antenne (le passage reste prêt) |
| **Ctrl+D** | ajoute ou retire le passage des favoris |

Formes acceptées : `Jn 3:16`, `Jean 3 16`, `1 Co 13:4-7`, `Ps 23`, `Jean 3:16-4:2`… Majuscules et accents sont libres. Une abréviation ambiguë (`Jo`) est refusée avec la liste des livres possibles : le plugin ne devine jamais. Détails : [SEARCH.md](SEARCH.md).

### Suivre une lecture : la saisie abrégée

Après `Jn 3:16`, tapez seulement `17` puis Entrée : Jean 3:17. Aussi `17-19` (plage), `4:1` (chapitre 4 verset 1 du même livre). Le champ est sélectionné après chaque envoi : le chiffre suivant remplace le précédent, sans effacer.

### À la souris

L'onglet **Sélecteur** : Livres, puis Chapitres, puis Versets. Clic = Preview ; Maj+clic = plage depuis le dernier verset cliqué ; Ctrl+clic ou double-clic = à l'antenne. Bleu = dans le Preview, rouge = à l'antenne.

## Changer de verset sans couper l'affichage

Préparez le verset suivant dans le Preview (la barre de recherche ou les flèches), puis **ON AIR** : l'affichage fait un fondu de l'ancien vers le nouveau, sans clignotement.

## Les thèmes et les longs passages

Trois thèmes, au choix dans le dock : **Bandeau bas** (le bandeau classique), **Plein écran** (texte centré sur fond sombre), **Texte seul** (texte avec ombre, sans cadre). Changer de thème pendant l'antenne ne coupe rien.

Un passage trop long pour un écran est coupé en **pages**, sans perdre un mot ; la taille du texte s'adapte pour que chaque page soit lisible. Le dock affiche « Page 2/5 » et les boutons ◀ ▶ ; PageHaut/PageBas font de même au clavier. Les pages ne bougent que l'antenne.

## Historique et favoris

- **Historique** : chaque passage mis à l'antenne y est gardé (50 derniers). Clic = Preview, double-clic = à l'antenne.
- **Favoris** : l'étoile à côté de la barre de recherche (ou Ctrl+D). Pratique pour les textes que vous reprenez souvent (200 au maximum).
- Ils sont conservés d'une séance à l'autre.

## Raccourcis globaux d'OBS

OBS > Paramètres > Raccourcis, six lignes « VYRA Bible : … » (ON AIR, Masquer, verset suivant/précédent, page suivante/précédente). Aucune touche n'est attribuée par défaut : choisissez-les vous-même. Elles marchent même quand le dock n'a pas le focus.

## Si quelque chose ne va pas

| Constat | Que faire |
|---|---|
| Texte rouge sous la recherche | Lire le message : il dit ce qui ne va pas (« Jean 3 a 36 versets »). Rien n'a été envoyé à l'antenne. |
| Rien n'apparaît dans OBS | Cliquer **Ajouter la source dans OBS** ; vérifier que la source « VYRA Bible » est visible dans la scène. Le bas du dock indique l'adresse locale du serveur. |
| Le port est déjà pris | Le plugin essaie 10 ports (17420 à 17429) ; le bas du dock affiche celui qui est utilisé. |
| Le dock dit que la Bible est introuvable | Réinstaller le plugin. |
| Le message « lignes illisibles ignorées » | Le fichier d'historique/favoris était abîmé ; ce qui était lisible est conservé. |

Le journal d'OBS (menu Aide > Fichiers journaux) contient les lignes du plugin, préfixées `[vyra-bible]`.

## État de vérification

Testé hors OBS (Linux : logique, dock, page overlay dans un vrai Chromium, serveur local). **Pas encore essayé dans OBS** : création de la source par le bouton, raccourcis globaux, fond transparent réel, NDI. La liste de contrôle à suivre au premier essai est dans [DEVELOPMENT.md](DEVELOPMENT.md).
