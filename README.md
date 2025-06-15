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
Version fonctionnelle, grace à une implémentation de garantie de fiabilité partielle sous forme de fenêtre glissante. 

## Version 4.1
Version visant à implémentée l'établissement de connexion et la négociation du taux de pertes admissibles. Version non fonctionnelle à cause d'un segmentation fault. 
### Outils dont nous nous sommes servis pour essayer de debugger 
Premièrement, nous avons tenté de savoir où se situe le segmentation fault. Avec les printf déjà présent, nous avons observé que l'erreur se trouve du côté du serveur, au niveau de process_received_pdu. Nous nous sommes donc assurées dans un premier temps que les fonctions en amont se déroule correctement (socket et bind). Ensuite, nous avons vérifié que chaque champ que nous modifions et traitons dans la fonction process_received_pdu soit correct. Cela comprend l'identification du bon socket et la manipulation des adresses. Nous avons également affiché le moment où nous quittons la fonction. Nous constatons donc que le segmentation fault n'apparait pas dans cette fonction. 

Nous nous sommes penchées sur la fonction accept qui se déroule en parallèle de process_received_pdu, sur un autre thread, toujours du côté du serveur. Le client a le comportement attendu et renvoie son SYN puisqu'il ne reçoit aucun SYN-ACK. 

 
## Choix d'implémentation 
### Tableau de socket 
Afin de tenir compte du fait que mic-tcp peut avoir plusieurs sockets à gérer, nous avons implémenté un tableau de sockets pour les stocker. Quand les sockets sont crées, on les affecte à la première case du tableau vide. Leur descripteur devient le numéro de la case où ils ont été mis. Pour savoir si une case est vide, on regarde si le descripteur du socket de la case est égale à -1. Ainsi, il a fallu créer une fonction d'initialisation pour mettre les descripteurs de toutes les cases à -1. Pour s'assurer que cette initialisation ne se fait qu'une seule fois, nous avons ajouté un int qui joue le rôle de boolean qui s'appelle tableau_initialise. 

### Pe et Pa 
Afin que le client et le serveur se mettent d'accord sur Pe et Pa, nous avons décidé que le client envoyait son Pe lors du SYN. Il est alors stocké dans le champ ack_num du header. Le serveur reprend ce Pe pour en faire son Pa. 

### Code de retour de IP_send 
Nous avons décidé de systématiquement tester le code de retour de la fonction IP_send. Si, après 10 itérations, le code de retour de IP_send est de -1, nous renvoyons -1 à la fonction. 

### Buffer de PDU 
Pour stocker le PDU transmis afin de pouvoir le retransmettre dans le cadre d'une garantie de fiabilité ( partielle ou totale), nous avons crée un tableau de mic_tcp_pdu d'une seule case. En effet, nous ne transmettons qu'un PDU à la fois. 

### Fenêtre glissante 
Ce système de fenêtre glissante prend la forme d'une matrice tableau_fenetres. Elle correspond à un tableau de fenêtres glissantes pour que chaque socket puisse avoir sa fenêtre. Ensuite, chaque fenêtre glissante prend la forme d'un tableau qu'on implémente de manière circulaire pour recenser les pertes et les succès de transmission. Pour stocker chaque index pour se repérer dans les tableaux des fenêtres glissantes, nous avons crée un tableau d'index : index_fenetres. 

Pour initialiser tout ce système, nous avons programmé la fonction init_mat_fg avec la variable matrice_implementee pour s'assurer que l'initialisation ne s'exécute qu'une seule fois. 

Pour mettre à jour les fenêtres glissantes, nous avons mis en place une fonction maj_fenetre_glissante, qui permet d'analyser le PDU reçu et d'effectuer les calculs nécessaires pour savoir si le PDU rendre ou non dans le pourcentage de pertes admissibles. Le cas échéant, le code retour de la fonction permet d'indiquer la nécessité, ou non, du renvoi. Le taux de pertes admssibles est fixé de manière globale, et peut être modifié au besoin par l'utilisateur.
Les tests du code en faisant varier le taux de pertes admissibles a permis de conclure:
- Un pourcentage de pertes admissibles faible donne un résultat similaire à la v2 (mécanisme de reprise totale des pertes). La vidéo est globalement fluide, malgré quelques légers arrêts sur image.
- Un pourcentage de pertes admissibles plus élevé donne une vidéo peu fluide avec de nombreux sauts d'images.





