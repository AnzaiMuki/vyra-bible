# L'overlay : ce que voit OBS

VYRA Bible démarre un petit serveur web **sur ce seul ordinateur** (adresse `127.0.0.1`, port 17420, ou le premier libre jusqu'à 17429). Une source Navigateur d'OBS affiche sa page ; le dock lui envoie ce qui est à l'antenne.

## Ajouter la source dans OBS

1. Lancez OBS, le dock VYRA Bible affiche en bas : `Source navigateur OBS : http://127.0.0.1:17420/`.
2. Sources > + > Navigateur. Collez cette adresse. Largeur 1920, hauteur 1080.
3. Laissez le CSS personnalisé par défaut : la page a déjà un fond transparent.

Le même schéma (cas A) envoie ensuite cette scène en NDI vers l'OBS principal. **Cette dernière étape n'a pas encore été testée.**

## Ce que fait la page

- Rien n'est affiché tant que rien n'est ON AIR.
- ON AIR : le lower third apparaît en fondu (0,3 s). Un nouveau verset se fond sur l'ancien : l'écran ne passe jamais par le vide.
- Masquer / Échap : fondu de sortie puis la page est vidée.
- Le texte est réduit pour tenir dans le cadre. Si un passage est trop long même à la plus petite taille (un psaume entier), les derniers versets sont retirés **par versets entiers** et « … » est ajouté. Il n'y a pas encore de pagination : elle viendra avec les thèmes.
- Si la liaison avec le plugin est coupée, la dernière image reste à l'écran (un écran en direct ne doit pas se vider sur un incident réseau) ; la page se reconnecte seule.
- La page peut être redimensionnée dans OBS pendant l'antenne : le texte est réajusté.

## Sécurité

Le serveur n'écoute que `127.0.0.1`, n'accepte que `GET`, ne sert que trois fichiers fixes (`index.html`, `overlay.css`, `overlay.js`) plus `/state` et `/events`. Aucun chemin envoyé par un client n'est jamais lu sur le disque. Le texte de la Bible est inséré dans la page comme du texte, jamais comme du HTML.

## Pour déboguer

- `http://127.0.0.1:17420/state` : l'image courante en JSON.
- `http://127.0.0.1:17420/?manual=1` : la page sans connexion au plugin ; dans la console du navigateur, `vyraShow({rev:1, visible:true, reference:"Jean 3:16", verses:[{c:3,v:16,t:"..."}]})` affiche ce que vous voulez.
