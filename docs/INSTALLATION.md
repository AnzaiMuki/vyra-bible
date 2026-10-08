# Installation

## Avec l'installateur (recommandé)

1. Fermez OBS Studio.
2. Lancez `VYRA-Bible-Setup-<version>.exe` (droits administrateur demandés : OBS est dans Program Files).
3. Vérifiez que le dossier proposé est bien celui d'OBS Studio (par défaut `C:\Program Files\obs-studio`). L'installateur refuse un dossier sans `bin\64bit\obs64.exe`.
4. Démarrez OBS, puis menu **Docks** : cochez **VYRA Bible**.

Désinstallation : Paramètres Windows > Applications > VYRA Bible.

Prérequis : Windows 64 bits, OBS Studio 30 ou plus récent. Le plugin est construit avec OBS 31.1.1 ; la compatibilité avec OBS 30 reste à tester.

## Obtenir le setup .exe

Le setup n'est pas distribué dans le dépôt : il est construit par GitHub sur une machine Windows (un .exe Windows ne se fabrique pas sous Linux). Chaque exécution produit le setup, un zip d'installation manuelle (`VYRA-Bible-<version>-manual.zip`, à extraire sur le dossier d'OBS, utile sans droits administrateur ou avec OBS portable) et `SHA256SUMS.txt` pour vérifier les téléchargements.

**Option A : sans Visual Studio, via GitHub Actions** (le projet est déjà sur GitHub)
1. Onglet **Actions** > **Windows installer** > **Run workflow** (branche `main`).
2. Attendez la fin (la première fois, 15 à 30 minutes : téléchargement d'OBS et de Qt), ouvrez l'exécution, descendez à **Artifacts** et téléchargez `VYRA-Bible-Setup` (un .zip qui contient le .exe).
3. Pour une version publique téléchargeable par tous (page **Releases**) : `git tag v0.5.0` puis `git push origin v0.5.0`. Le workflow construit le setup et l'attache à la release. Le nom du tag doit être identique à la version de `buildspec.json`, sinon le workflow s'arrête.

Le script vérifie, avant de fabriquer le setup, que le plugin, la Bible, la page overlay et les deux fichiers de langue sont bien présents ; il refuse de produire un setup incomplet.

**Option B : sur votre PC Windows**
Voir [DEVELOPMENT.md](DEVELOPMENT.md), puis `.\scripts\build-installer.ps1`.

## Installation manuelle

Copiez, depuis le dossier produit par `cmake --install` :
- `vyra-bible\bin\64bit\vyra-bible.dll` vers `<OBS>\obs-plugins\64bit\`
- `vyra-bible\data\*` vers `<OBS>\data\obs-plugins\vyra-bible\`

## Installation silencieuse (pour plusieurs PC)

`VYRA-Bible-Setup-<version>.exe /SILENT /DIR="C:\Program Files\obs-studio"` (ajouter `/LANG=french` si besoin). `/VERYSILENT` n'affiche rien du tout.

## Mise à jour et désinstallation

Lancer le nouveau setup par-dessus l'ancien : il remplace le plugin (il propose de fermer OBS s'il est ouvert). La désinstallation retire le plugin ; **vos favoris et votre historique** (`library.txt`, dossier de configuration d'OBS) sont conservés.

## État de vérification

Vérifié : GitHub construit le plugin pour Windows et le setup (Inno Setup compile sans erreur avec l'icône, les images de l'assistant, les textes de fin d'installation, le zip manuel et les sommes de contrôle).
**Pas vérifié** : l'exécution du setup sur un PC (je ne peux pas le lancer ici). Points à contrôler au premier essai :
- la détection du dossier d'OBS par la clé de registre `HKLM\SOFTWARE\OBS Studio` (sinon, repli sur `C:\Program Files\obs-studio`) ;
- l'absence de besoin d'un runtime Visual C++ séparé sur un PC neuf (OBS embarque normalement le sien) ;
- le comportement si OBS est ouvert pendant une mise à jour ;
- l'affichage correct des accents dans l'écran de fin d'installation, l'icône, les images de l'assistant ;
- l'avertissement SmartScreen de Windows : le setup n'est pas signé numériquement (un certificat de signature est payant), Windows peut afficher « Éditeur inconnu » : « Informations complémentaires » puis « Exécuter quand même ».
