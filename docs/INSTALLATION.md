# Installation

## Avec l'installateur (recommandé)

1. Fermez OBS Studio.
2. Lancez `VYRA-Bible-Setup-<version>.exe` (droits administrateur demandés : OBS est dans Program Files).
3. Vérifiez que le dossier proposé est bien celui d'OBS Studio (par défaut `C:\Program Files\obs-studio`). L'installateur refuse un dossier sans `bin\64bit\obs64.exe`.
4. Démarrez OBS, puis menu **Docks** : cochez **VYRA Bible**.

Désinstallation : Paramètres Windows > Applications > VYRA Bible.

Prérequis : Windows 64 bits, OBS Studio 30 ou plus récent. Le plugin est construit avec OBS 31.1.1 ; la compatibilité avec OBS 30 reste à tester.

## Obtenir le setup .exe

Le setup n'est pas encore produit : il faut le construire (aucun binaire n'existe pour l'instant).

**Option A : sans Visual Studio, via GitHub Actions**
1. Poussez le projet sur un dépôt GitHub.
2. Onglet **Actions** > **Windows installer** > **Run workflow**.
3. Téléchargez l'artefact `VYRA-Bible-Setup` en fin d'exécution.

**Option B : sur votre PC Windows**
Voir [DEVELOPMENT.md](DEVELOPMENT.md), puis `.\scripts\build-installer.ps1`.

## Installation manuelle

Copiez, depuis le dossier produit par `cmake --install` :
- `vyra-bible\bin\64bit\vyra-bible.dll` vers `<OBS>\obs-plugins\64bit\`
- `vyra-bible\data\*` vers `<OBS>\data\obs-plugins\vyra-bible\`

## État de vérification

L'installateur (`installer/vyra-bible.iss`), le script PowerShell et le workflow n'ont **pas été exécutés** : l'environnement de développement actuel est sous Linux, sans Inno Setup ni compilateur Windows. Points à vérifier au premier essai :
- la détection du dossier d'OBS par la clé de registre `HKLM\SOFTWARE\OBS Studio` (sinon, repli sur `C:\Program Files\obs-studio`) ;
- l'absence de besoin d'un runtime Visual C++ séparé sur un PC neuf (OBS embarque normalement le sien) ;
- le comportement si OBS est ouvert pendant une mise à jour.
