#include <mictcp.h>
#include <api/mictcp_core.h>

#define nb_socket 10
#define taille_fenetre_glissante 10

struct mic_tcp_sock tableau_sockets[nb_socket] ;
int pe; // c'est aussi Pa _ on met ici la valeur de 0 car cette valeur sera échangée lors de la phase d'établissement de connexion implémentée dans les prochaines versions 
struct mic_tcp_pdu buffer[1] ; // buffer pour stocker le PDU
unsigned long timer = 1000 ; //timer avant renvoie d'un PDU
float tx_pertes_admissible = 0.2; // pourcentage de pertes admissibles
// int fenetre_glissante[taille_fenetre_glissante] ;
int index_fenetre[nb_socket] ;
int tableau_fenetres[nb_socket][taille_fenetre_glissante] ;
int matrice_implementee = 0 ;
int retour_recv=-1; // -1 si on n'a pas reçu, 0 si on a reçu un PDU, 1 si le timer est arrivé à expiration
int tableau_initialise = 0 ;

/* initialisation du tableau des sockets */
int init_tableau_socket() {
    for (int i = 0 ; i < nb_socket ; i++) {
        tableau_sockets[i].fd = -1 ;
    }
    return 1 ;
}

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */

   if (tableau_initialise == 0) {
    tableau_initialise = init_tableau_socket () ;
   }

   struct mic_tcp_sock socket ;
   int i = 0 ;
   while (tableau_sockets[i].fd != -1){
    i++ ;
   } 
   socket.state = IDLE ;
   if (i == 0){
    socket.fd = i ;
    tableau_sockets[i] = socket ; 
   } else{
    socket.fd = i-1 ;
    tableau_sockets[i-1] = socket ; 
   } 
   set_loss_rate(20);
    result = socket.fd ;
   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   unsigned short port =addr.port;
   int retour =0;
   for (int i=0; i<nb_socket;i++){
        if (tableau_sockets[i].local_addr.port==port){
            retour =-1; // le numéro de port est déjà utilisé par un autre socket
        }else{
            tableau_sockets[socket].local_addr = addr ;
        }
    }
    return retour;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu pdu_recu ;
    mic_tcp_pdu pdu_synack ;
    int k = 0 ;
    int sent_size ;

    tableau_sockets[socket].state = IDLE ;

    // se bloque dans un while le temps de savoir si on a reçu un paquet de données (on bloque dans IDLE)
    // si on passe en SYN_RECEIVED, on fait IP_send et on bloque en SYN_SENT
    // si on repasse en SYN_RECEIVED, on se met en ESTABLISHED, sinon on renvoie

    while(tableau_sockets[socket].state != SYN_RECEIVED) {
        k++ ;
    }

    if (tableau_sockets[socket].state == SYN_RECEIVED) {
        sent_size = IP_send(buffer[0], addr->ip_addr) ;

        // test du code retour de sent_size
        while (sent_size == -1) {
            sent_size = IP_send(buffer[0], addr->ip_addr) ;
        }

        tableau_sockets[socket].state = SYN_SENT ;
    }

    while(tableau_sockets[socket].state =! SYN_RECEIVED) {
        if (retour_recv == 1) {
            sent_size = IP_send(buffer[0], addr->ip_addr) ;

            // test du code retour de sent_size
            while (sent_size == -1) {
                sent_size = IP_send(buffer[0], addr->ip_addr) ;
            }
        }
    }

    tableau_sockets[socket].state = ESTABLISHED ;

    if (tableau_sockets[socket].state == ESTABLISHED) {
        return 0 ;
    } else {
        tableau_sockets[socket].state = IDLE ;
        return -1 ;
    }

    /*int retour_recv = IP_recv(&pdu_recu, &addr->ip_addr, &tableau_sockets[socket].remote_addr, timer) ;
    
    if (pdu_recu.header.syn != 1) { 
        return -1 ; 

    } 
    
    pe = pdu_recu.header.ack_num ;
    pdu_synack.header.ack_num = pdu_recu.header.ack_num ;
    buffer[0] = pdu_synack ;

    int sent_size = IP_send(buffer[0], addr->ip_addr) ;

    bool recu = false ;
    retour_recv = -1 ;
    int i = 0 ;

    while((!recu) && (pdu_recu.header.ack_num == buffer[0].header.seq_num) && (i < 100)) {

        retour_recv = IP_recv(&pdu_recu, &addr->ip_addr, &tableau_sockets[socket].remote_addr, timer) ;

        if ((retour_recv != -1)&&(pdu_recu.header.ack_num == buffer[0].header.seq_num)) {
            recu = true ;
        } else {
            sent_size = IP_send(buffer[0], addr->ip_addr) ;
        }

        i++ ;
    }

    if(i<100) {
        return 0 ;
    } else {
        return -1 ;
    }*/
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) 
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    tableau_sockets[socket].remote_addr = addr ; //stockage de l'adresse de destination
    
    //construction du PDU SYN 
    mic_tcp_pdu pdu_syn;
    pe=0; //on initialise la valeur de Pe et on va la transmettre au destinataire 
    pdu_syn.header.syn=1;
    pdu_syn.header.ack_num=pe;
    pdu_syn.payload.size=0;
    buffer[0]=pdu_syn; //stockage du pdu dans le buffer

    int size_send =-1;
    while (size_send==-1){ // on teste le code de retour de IP_send pour être sures que le PDU est bien envoyé 
        size_send=IP_send(buffer[0],tableau_sockets[socket].remote_addr.ip_addr);
    }
    
    tableau_sockets[socket].state=SYN_SENT;

    int k = 0 ; /* variable qui permet de ne pas rester éternellement dans la boucle si on ne reçoit pas de message comme ça on n'en envoie pas 10 000 à la suite*/

    while((tableau_sockets[socket].state!=ESTABLISHED) && k<10) { /* tant qu'on n'a pas fait trop d'itérations, et tant que l'accusé de réception n'est pas reçu, sachant que son numéro doit correspondre au numéro de séquence du pdu contenu dans le buffer*/
        
        if (retour_recv==1){
            size_send = IP_send(buffer[0], tableau_sockets[socket].remote_addr.ip_addr) ; 
            retour_recv = -1;
            k++ ;
        }   
    }
    // on renvoie 0 si la connexion est établie, -1 sinon 
    if(k < 10) {
        //on envoie le ack 
        mic_tcp_pdu pdu_ack;
        pdu_ack.header.ack=1;
        pdu_ack.payload.size=0;
        buffer[0]=pdu_ack; //stockage du pdu dans le buffer

        size_send =-1;
        while (size_send==-1){ // on teste le code de retour de IP_send pour être sures que le PDU est bien envoyé 
            size_send=IP_send(buffer[0],tableau_sockets[socket].remote_addr.ip_addr);
        }

        tableau_sockets[socket].state=ESTABLISHED;

        return 0;
    } else {
        printf("k : %d\n", k);

        tableau_sockets[socket].state=IDLE;

        return -1 ;
    }


}

void init_mat_fg() {
    for (int i = 0 ; i < nb_socket ; i++) {
        for (int j = 0 ; j < taille_fenetre_glissante ; j++) {
            tableau_fenetres[i][j] = 1 ;
        }
        index_fenetre[i] = 0 ;
    }
    matrice_implementee = 1 ;
}

/* calcule et mise à jour de la fenêtre glissante, 
return un int pour indiquer s'il faut renvoyer le PDU ou non
1 = renvoyer le pdu
0 = pas nécessaire de le renvoyer 
2= pas besoin de le renvoyer mais il y eu une perte donc on ne doit pas implémenter le Pe*/
 
int maj_fenetre_glissante(int retour_recv, int socket, mic_tcp_pdu pdu_recu, int seq_attendu) { 
    int nouvelle_case ;
    float moyenne = 0.0 ;

    if (retour_recv == -1) { // on n'a pas reçu le PDU
        nouvelle_case = 1 ; // il faut renvoyer le PDU (si on est au-dessus du taux  de pertes)
    } else { // on a reçu le PDU

        if ((pdu_recu.header.ack == 1) && (pdu_recu.header.ack_num == seq_attendu)) { // le pdu est un ack ET c'est le bon ack
            nouvelle_case = 0 ; // pas besoin de renvoyer le PDU
        } else { // le pdu n'est pas un ack, ou ce n'est pas celui qu'on attend, ce qui cosntitue également une perte
            nouvelle_case = 1 ;
        }
        
    }

    printf("valeur de nouvelle case : %d \n", nouvelle_case) ;

    tableau_fenetres[socket][index_fenetre[socket]] = nouvelle_case;
    // index_fenetre[socket]=(index_fenetre[socket]+1)%taille_fenetre_glissante;


    for (int i = 0 ; i < taille_fenetre_glissante ; i++) {
        moyenne += (float)tableau_fenetres[socket][i] ;
    }
    moyenne = moyenne / taille_fenetre_glissante ;
    printf("moyenne à la fin de maj : %f\n", moyenne) ;

    if (((pdu_recu.header.ack == 1) && (pdu_recu.header.ack_num == buffer[0].header.seq_num))) { //on a reçu le bon ack
        printf("on a reçu un ack, et ack_num : %d = seq_num : %d , tout est ok \n", pdu_recu.header.ack_num, buffer[0].header.seq_num) ;
        printf("tout est ok, on return 0\n") ;
        return 0 ;
    }else { //on n'a pas reçu le bon ack ou pas reçu de ack
        printf("On n'a pas reçu de ack/ le bon num de ack \n");
        if (moyenne < tx_pertes_admissible) {
            printf("moyenne : %f < tx_pertes : %f\n", moyenne, tx_pertes_admissible) ;
            return 2; // on ignore la perte mais on ne doit pas implémenter Pe
        }else{
            printf("moyenne : %f >= tx_pertes : %f\n", moyenne, tx_pertes_admissible) ;
            return 1; // on doit renvoyer le pdu 
        }
    }

}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    // créer un mic_tcp_pdu
    mic_tcp_pdu pdu;
    struct mic_tcp_sock le_socket ;
    
    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;
    int i = 0 ;
    int retour_fenetre_glissante;

    while (i != mic_sock) {
        i++ ;
    }

    le_socket = tableau_sockets[i] ;

    pdu.header.source_port =  le_socket.local_addr.port ; /*aller chercher dans la structure mic_tcp_socket 
    correspondant au socket identifié par mic_sock en paramètre*/ 
    pdu.header.dest_port =  le_socket.remote_addr.port ; /*port auquel on veut envoyer le message qui a été donné 
    par l'application lors du mic_tcp_connect et qu'on a stocké dans la structure 
    mic_tcp_socket correspondant au socket identifié par mic_sock passé en paramètre*/

    /* envoyer un message (dont la taille et le contenu sont passés en paramètres)*/

    // renseigner le numéro de séquence
    pdu.header.seq_num = pe ;
    /* stocker le pdu dans un buffer: o crée un tableau correspondant au PDU stocké dans le buffer en émission, et on
    stocke le PDU dans la seule case du tableau (on envoi un PDU à la fois) (struct mic_tcp_pdu buffer[1])*/
    buffer[0] = pdu ;

    // initialisation de la matrice des fenêtres glissantes
    // ajouter un mutex quand on passera au multithreading
    if (matrice_implementee == 0) {
        init_mat_fg() ;
    }
    // envoyer le message (dont la taille et le contenu sont passés en paramètres)
    int sent_size = IP_send(buffer[0], le_socket.remote_addr.ip_addr) ; /* 2e arg = structure mic_tcp_socket_addr contenue 
    dans la structure mic_tcp_socket correspondant au socket identifié par mic_sock passé en paramètre*/

    int recu = 0 ; // 0 faux et 1  juste
    int retour_recv = -1 ;
    int k = 0 ; /* variable qui permet de ne pas rester éternellement dans la boucle si on ne reçoit pas de message
    comme ça on n'en envoie pas 10 000 à la suite*/

    mic_tcp_pdu pdu_recu ; 

    while((recu < 1) && k<10) { /* tant qu'on n'a pas fait trop d'itérations, 
        et tant que l'accusé de réception n'est pas reçu, 
        sachant que son numéro doit correspondre au numéro de séquence du pdu contenu dans le buffer*/
        
        retour_recv = IP_recv(&pdu_recu, &le_socket.local_addr.ip_addr, &le_socket.remote_addr.ip_addr, timer) ; // TODO: mettre les arguments FAIT
        printf("\n \n     RETOUR_RECV : %d\n", retour_recv) ;
        retour_fenetre_glissante = maj_fenetre_glissante(retour_recv, mic_sock, pdu_recu, buffer[0].header.seq_num) ;

        printf("pdu_recu.header.ack_num : %d\n", pdu_recu.header.ack_num) ;
        printf("buffer[0].header.seq_num : %d\n", buffer[0].header.seq_num) ;

        //on vérifie que le PDU reçu soit bien un ack 
        if (retour_fenetre_glissante == 0){ 
            printf("retour fenetre glissante : %d, on met recu à 1\n", retour_fenetre_glissante) ;
            recu = 1 ;
        } else if (retour_fenetre_glissante == 1) {
            /*printf("pdu_recu.header.ack_num : %d\n", pdu_recu.header.ack_num) ;
            printf("buffer[0].header.seq_num : %d\n", buffer[0].header.seq_num) ;*/
            printf("retour fenetre glissante : %d, on met recu à -1 \n", retour_fenetre_glissante) ;
            sent_size = IP_send(buffer[0], le_socket.remote_addr.ip_addr) ; 
            retour_recv = -1;
            printf("     SENT_SIZE : %d\n",sent_size) ;
        }
        k++ ;
    }
    
    if (retour_fenetre_glissante !=2){
        // incrémenter Pe
        pe = (pe+1)%2 ;
    }

    //mise à jour de la fenêtre glissante 
    index_fenetre[mic_sock]=(index_fenetre[mic_sock]+1)%taille_fenetre_glissante;

    if(k < 10) {
        printf("k : %d\n", k);
        return sent_size;
    } else {
        printf("k : %d\n", k);
        return -1 ;
    }
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload payload ;
    payload.data=mesg;
    payload.size=max_mesg_size;
    int effective_data_size= app_buffer_get(payload);
    if (effective_data_size ==0){
        return -1;
    } 
    else{
        return effective_data_size;
    } 
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    if (pdu.payload.size!=0 && pdu.header.seq_num==pe){
        //printf("  ON RENTRE DANS LA RECUPERATION DU PAQUET \n");
        app_buffer_put(pdu.payload);
        pe=(pe+1)%2;
        printf("MIS A JOUR PA : ack_num : %d \n",pe);
    }
    printf("RECEPTION  : ack_num : %d \n",pe);

     //envoyer le ack avec le bon numero 
    //creer un header
    mic_tcp_pdu pdu_ack;
    pdu_ack.header.ack_num=pdu.header.seq_num;
    pdu_ack.header.ack=1;
    pdu_ack.header.dest_port=pdu.header.source_port;
    pdu_ack.header.source_port=pdu.header.dest_port;
        //payload 
    pdu_ack.payload.data=NULL;
    pdu_ack.payload.size=0;

    buffer[0]=pdu_ack;
        
    printf("ACK NUM : %d \n",buffer[0].header.ack_num);
    IP_send(pdu_ack,remote_addr);

   
}
