# Textes bibliques

## Ce qui est embarqué

| Module | Fichier | Langue | Statut |
|---|---|---|---|
| Louis Segond 1910 | `data/bibles/lsg1910.tsv` | fr | Embarqué (Milestone 02) |
| King James Version | non encore ajoutée | en | Prévue (domaine public) |
| Fon | non fournie | fon | Par import uniquement (licence à confirmer) |

Aucune traduction protégée n'est distribuée avec le plugin.

## Origine de la Segond 1910

- Source : dépôt [BibleCorps/FRA-B-LSG1910-PD-UBS](https://github.com/BibleCorps/FRA-B-LSG1910-PD-UBS) (format USFM, « Updated 2011 by Moon Sun Kim, 2012 by E. Canales, 2024 to p.sfm »).
- Licence déclarée par la source : **Public Domain** (champ `contributor` de `metadata.xml`).
- Il s'agit de l'édition de 1910 avec quelques mises à jour d'éditeur (corrections de coquilles). C'est probablement ce que le projet appelait « Louis Segond mis à jour ». Aucune révision protégée (NEG 1979, Segond 21) n'est utilisée.

**Limite de la vérification des droits :** le statut de domaine public est celui que la source déclare. Je n'ai pas fait d'analyse juridique, et les « mises à jour » des éditeurs n'ont pas été examinées une à une. À confirmer avant toute distribution large.

## Format « module VYRA » (version 1)

Texte UTF-8, un enregistrement par ligne :

```
#vyra-module<TAB>1
#code<TAB>LSG1910
#name<TAB>Louis Segond 1910
#language<TAB>fr
#license<TAB>Public Domain
#source<TAB>...
<livre><TAB><chapitre><TAB><verset><TAB><texte>
```

- Livres numérotés de 1 (Genèse) à 66 (Apocalypse).
- Enregistrements en ordre strictement croissant.
- Le chargeur rejette un fichier invalide en entier, avec la ligne fautive dans le journal d'OBS (fichier introuvable, en-tête manquant, numéro hors limites, texte vide, ordre incorrect, UTF-8 invalide).

## Versification de la Segond

Le module compte **31 170 versets**, plus que les 31 102 habituels : la Segond numérote certains titres de psaumes comme des versets. Exemple : Psaume 23:1 = « Cantique de David. L'Éternel est mon berger : je ne manquerai de rien. » (titre et premier vers dans un seul verset). Le Psaume 51 compte ainsi 21 versets (19 dans la KJV). 3 Jean compte 15 versets (14 dans la KJV). La recherche par référence (voir `SEARCH.md`) suit cette numérotation, pas celle de la KJV.

## Fabriquer ou régénérer un module

```bash
python3 scripts/usfm_to_vyra_tsv.py <dossier_usfm> data/bibles/lsg1910.tsv --code LSG1910 \
  --name "Louis Segond 1910" --language fr --license "Public Domain" --source "..."
python3 scripts/audit_usfm_conversion.py <dossier_usfm> data/bibles/lsg1910.tsv
```

Le convertisseur ne garde que le texte des versets (ni introductions, ni titres, ni renvois, ni notes) et refuse un fichier dont les versets ne se suivent pas 1, 2, 3… dans un chapitre. L'audit compare, livre par livre, le nombre de caractères de la source et du module : une différence signale du texte perdu ou ajouté.

### Corrections de la source

La source contient une coquille, corrigée explicitement par le convertisseur (liste `ERRATA`) : Psaume 119:115 est écrit `\v 11 5` au lieu de `\v 115`. Une correction qui ne trouve pas exactement une occurrence arrête le script, pour qu'un changement de la source ne passe pas inaperçu.

### Bug trouvé et corrigé pendant la conversion

Une première version du convertisseur perdait le texte placé après une ligne vide (`\b`) dans les Psaumes, par exemple « L'Éternel est mon berger » au Psaume 23:1. L'audit de comptage de caractères existe à cause de ce bug ; il détecte la perte (vérifié par une simulation), et un test garde le Psaume 23:1.
