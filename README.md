# BE RESEAU
## TPs BE Reseau - 3 MIC

Cette version finale correspond à la v4.1.

## Commandes de compilation et d'exécution
* Compilation : make
* Exécution : 
- puit ==> ./tsock_texte -p 9999 OU ./tsock_video -p 9999
- source ==> ./tsock_texte -s localhost 9999 OU ./tsock_video -s localhost 9999

## Version 1
Version fonctionnelle, qui permet d'échanger des messages entre un client et un serveur sans garantie de fiabilité.

## Version 2
Version fonctionnelle, qui permet de renvoyer tous les paquets perdus à l'aide du mécanisme de "Stop and Wait"

## Version 3 
Version fonctionnelle, grace à une implémentation de garantie de fiabilité partielle sous forme de fenêtre glissante. Pour cela, nous avons mis en place une fonction maj_fenetre_glissante, qui permet d'analyser le PDU reçu et d'effectuer les calculs nécessaires pour savoir si le PDU rendre ou non dans le pourcentage de pertes admissibles. Le cas échéant, le code retour de la fonction permet d'indiquer la nécessité, ou non, du renvoi. Le taux de pertes admssibles est fixé de manière globale, et peut être modifié au besoin par l'utilisateur.
Les tests du code en faisant varier le taux de pertes admissibles a permis de conclure:
- Un pourcentage de pertes admissibles faible donne un résultat similaire à la v2 (mécanisme de reprise totale des pertes). La vidéo est globalement fluide, malgré quelques légers arrêts sur image.
- Un pourcentage de pertes admissibles plus élevé donne une vidéo peu fluide avec de nombreux sauts d'images.

## Version 4.1
Pour la phase d'établissement de connexion, le client envoie un SYN au serveur pour demander la connexion. Le serveur renvoie un SYN_ACK pour signifier qu'il a bien reçu le SYN et est prêt à accepter la connexion. A la réception du SYN_ACK, le client bascule dans l'état ESTABLISHED et envoie un ACK au serveur pour lui indiquer.
 
## Choix d'implémentation 
### Tableau de socket 
Afin de tenir compte du fait que mic-tcp peut avoir plusieurs sockets à gérer, nous avons implémenté un tableau de sockets pour les stocker. Quand les sockets sont crées, on les affecte à la première case du tableau vide. Leur descripteur devient le numéro de la case où ils ont été mis. Pour savoir si une case est vide, on regarde si le descripteur du socket de la case est égale à -1. Ainsi, il a fallu créer une fonction d'initialisation pour mettre les descripteurs de toutes les cases à -1. Pour s'assurer que cette initialisation ne se fait qu'une seule fois, nous avons ajouté un int qui joue le rôle de boolean qui s'appelle tableau_initialise. 

### Pe et Pa 
Afin que le client et le serveur se mettent d'accord sur Pe et Pa, nous avons décidé que le client envoyait son Pe lors du SYN. Le serveur reprend ce Pe pour en faire son Pa. 



