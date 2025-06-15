#include <mictcp.h>
#include <api/mictcp_core.h>

#define nb_socket 10
#define taille_fenetre_glissante 10

struct mic_tcp_sock tableau_sockets[nb_socket] ;
int pe; // c'est aussi Pa _ on met ici la valeur de 0 car cette valeur sera échangée lors de la phase d'établissement de connexion implémentée dans les prochaines versions 
struct mic_tcp_pdu buffer[1] ; // buffer pour stocker le PDU
unsigned long timer = 1000 ; //timer avant renvoie d'un PDU
float tx_pertes_admissible = 0.2; // pourcentage de pertes admissibles
int index_fenetre[nb_socket] ;
int tableau_fenetres[nb_socket][taille_fenetre_glissante] ;
int matrice_implementee = 0 ;
int tableau_initialise = 0 ;
int pertes_fixees ;

/* initialisation du tableau des sockets */
int init_tableau_socket() {
    for (int i = 0 ; i < nb_socket ; i++) {
        tableau_sockets[i].fd = -1 ;
        tableau_sockets[i].local_addr.ip_addr.addr_size=0;
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
   
    socket.fd = i ;
    tableau_sockets[i] = socket ; 
   
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
            //printf("bind: size addr : %d \n", tableau_sockets[socket].local_addr.ip_addr.addr_size);
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
    int k = 0 ;
    tableau_sockets[socket].state = IDLE ;
    int sent_size =-1 ;

    printf("accept 0 : tableau_sockets[%d].fd = %d\n", socket, tableau_sockets[socket].fd) ; 

    // création PDU SYN-ACK 
    mic_tcp_pdu pdu_sa; 
    pdu_sa.payload.size=0;
    pdu_sa.header.ack=1;
    pdu_sa.header.ack_num=pe;
    printf("accept 0 : pdu_sa.header.ack_num %d = pe %d\n", pdu_sa.header.ack_num, pe) ;
    pdu_sa.header.syn=1;
    pdu_sa.header.source_port=tableau_sockets[socket].local_addr.port;
    printf("accept 0 : tableau_sockets[%d].local_addr.port %d = %d\n", socket, pdu_sa.header.source_port, tableau_sockets[socket].local_addr.port) ; 
    pdu_sa.header.dest_port=addr->port;
    printf("accept 0 : pdu_sa.header.dest_port %d = addr->port %d\n", pdu_sa.header.dest_port, addr->port) ;

    buffer[0]=pdu_sa;  

    // se bloque dans un while le temps de savoir si on a reçu un paquet de données (on bloque dans IDLE)
    // si on passe en SYN_RECEIVED, on fait IP_send et on bloque en SYN_SENT
    // si on repasse en SYN_RECEIVED, on se met en ESTABLISHED, sinon on renvoie

    while(tableau_sockets[socket].state != SYN_RECEIVED) {
        k++ ;
        //printf("accept 1 : on bloque dans IDLE\n ") ;
        //printf("accept 1 : socket state : %d\n ", tableau_sockets[socket].state) ;
    }
    printf("accept 1 : on quitte IDLE au bout de %d itérations\n ", k) ;
    printf("accept 1 : socket state : %d\n ", tableau_sockets[socket].state) ;

    if (tableau_sockets[socket].state == SYN_RECEIVED) {

        printf("accept 2 : socket state : %d\n ", tableau_sockets[socket].state) ;
        printf("accept 2 : sent_size %d\n ", sent_size) ;
        // test du code de retour de IP_send 
        int j=0; 
        while (sent_size==-1){ 
            printf("on est rentré dans le while\n ") ;
            printf("on est avant IP_send\n ") ;
            //printf("accept 2 : addr->ip_addr = %s\n ", addr->ip_addr.addr) ;
            sent_size = IP_send(buffer[0], addr->ip_addr);
            printf("accept 2 : sent_size = %d\n ", sent_size) ;
            j++; 
            if (j>10){
                printf("Accept 2 : Erreur dans IP_send \n"); 
                return -1; 
            } 
        }

        tableau_sockets[socket].state = SYN_SENT ;
        printf("accept 2 : socket state : %d\n ", tableau_sockets[socket].state) ;
    }
    k = 0 ;

    while((tableau_sockets[socket].state =! SYN_RECEIVED)) {
        printf("accept 3 : socket state : %d\n ", tableau_sockets[socket].state) ;
        if (k > 100) {
            sent_size=-1; 
            int j=0; 
            while (sent_size==-1){ 
                sent_size = IP_send(buffer[0], addr->ip_addr);
                printf("accept 3 : sent_size = %d\n ", sent_size) ;
                j++; 
                if (j>10){
                    printf("Accept 3: Erreur dans IP_send \n"); 
                    return -1; 
                } 
            }
            k = 0 ;
        } else {
            k++ ;
        }
    }

    tableau_sockets[socket].state = ESTABLISHED ;
    printf("accept 4 : socket state : %d\n ", tableau_sockets[socket].state) ;

    if (tableau_sockets[socket].state == ESTABLISHED) {
        return 0 ;
    } else {
        tableau_sockets[socket].state = IDLE ;
        return -1 ;
    }

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
    //pdu_syn.payload.size=0;
    pdu_syn.header.source_port=tableau_sockets[socket].local_addr.port;
    pdu_syn.header.dest_port=addr.port;  
    

    /* négociation du pourcentage de pertes acceptées*/
    char arg[10];
    int pertes_client = (rand()%50) ; //le client ne veut pas de pertes supérieures à 50%
    sprintf(arg,"%d",pertes_client);
    pdu_syn.payload.size = sizeof(char);
    pdu_syn.payload.data = (char*)arg;
    printf("client: pertes souhaitées = %d\n", pertes_client) ;

    buffer[0]=pdu_syn; //stockage du pdu dans le buffer

    int size_send =-1;
    int j=0; 
    while (size_send==-1){ 
        size_send = IP_send(buffer[0], addr.ip_addr);
        j++; 
        if (j>10){
           printf("Connect1: Erreur dans IP_send \n"); 
            return -1; 
        } 
    }
    printf("SYN envoyé\n "); 

    tableau_sockets[socket].state=SYN_SENT;
    mic_tcp_pdu pdu_recu ; 

    int k = 0 ; /* variable qui permet de ne pas rester éternellement dans la boucle si on ne reçoit pas de message comme ça on n'en envoie pas 10 000 à la suite*/
    int recu = 0 ; // 0 faux et 1  juste
    int rtr_recv = -1 ;

    while((recu < 1) && k<10) { /* tant qu'on n'a pas fait trop d'itérations, et tant que l'accusé de réception n'est pas reçu, sachant que son numéro doit correspondre au numéro de séquence du pdu contenu dans le buffer*/
        
        /*while (rtr_recv==-1){ // on rend bloquant le recv 
            rtr_recv = IP_recv(&pdu_recu, &tableau_sockets[socket].local_addr.ip_addr, &tableau_sockets[socket].remote_addr.ip_addr, timer) ;
            printf("retour_recv : %d\n", rtr_recv) ;
        }*/

        rtr_recv = IP_recv(&pdu_recu, &tableau_sockets[socket].local_addr.ip_addr, &tableau_sockets[socket].remote_addr.ip_addr, timer) ;
        printf("retour_recv : %d\n", rtr_recv) ;
        int pertes_serveur = atoi(pdu_recu.payload.data) ;
        printf("Le serveur propose une perte de %d\n", pertes_serveur) ;
        // calcul du pourcentage de pertes final
        pertes_fixees = (pertes_serveur + pertes_client)/2 ;

        if ((rtr_recv!=-1)&&(pdu_recu.header.ack==1)&&(pdu_recu.header.syn==1)){ //on vérifie que le PDU reçu soit bien un synack 
            if((pdu_recu.header.ack_num - buffer[0].header.seq_num) == 0) {
                recu = 1 ;
            }
        }

        else {
            //test code retour de IP_send 
            size_send = -1; 
            int j=0; 
            while (size_send==-1){ 
                size_send = IP_send(buffer[0], addr.ip_addr);
                 j++; 
                if (j>10){
                    printf("Connect2: Erreur dans IP_send \n"); 
                    return -1; 
                } 
            }
            rtr_recv = -1;
            printf("sent_size : %d\n",size_send) ;
        }
    }
    // on renvoie 0 si la connexion est établie, -1 sinon 
    if(k < 10) {
        //on envoie le ack 
        mic_tcp_pdu pdu_ack;
        pdu_ack.header.ack=1;
        pdu_ack.payload.size=0;
        pdu_ack.header.dest_port=addr.port;
        pdu_ack.header.source_port=tableau_sockets[socket].local_addr.port; 
        //On envoie au serveur le pourcentage de pertes admissibles fixe finalement 
        sprintf(arg,"%d",pertes_fixees);
        pdu_ack.payload.size = sizeof(char);
        pdu_ack.payload.data = arg;
        printf("client: pertes fixees finalement : %d\n",pertes_fixees);
        buffer[0]=pdu_ack; //stockage du pdu dans le buffer

        //envoi de ack + test code retour 
        size_send =-1;
        int j=0; 
        while (size_send ==-1){ 
            size_send= IP_send(buffer[0], addr.ip_addr);
            j++; 
            if (j>10){
                printf("Connect (envoi ack): Erreur dans IP_send \n"); 
                return -1; 
            } 
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
            nouvelle_case = 1 ;}
        
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
        if (moyenne < pertes_fixees) {
            printf("moyenne : %f < tx_pertes : %f\n", moyenne, pertes_fixees) ;
            return 2; // on ignore la perte mais on ne doit pas implémenter Pe
        }else{
            printf("moyenne : %f >= tx_pertes : %f\n", moyenne, pertes_fixees) ;
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

    pdu.header.source_port =  le_socket.local_addr.port ; 
    pdu.header.dest_port =  le_socket.remote_addr.port ; 

    pdu.header.seq_num = pe ;
    buffer[0] = pdu ;

    // initialisation de la matrice des fenêtres glissantes
    if (matrice_implementee == 0) {
        init_mat_fg() ;
    }
    // envoyer le message (dont la taille et le contenu sont passés en paramètres)
    int sent_size=-1; 
    int j=0; 
    while (sent_size==-1){ 
        sent_size = IP_send(buffer[0], tableau_sockets[mic_sock].remote_addr.ip_addr);
        j++; 
        if (j>10){
            printf("Erreur dans IP_send \n"); 
            return -1; 
        } 
    }

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
            // renvoi PDU 
            sent_size=-1; 
            int j=0; 
            while (sent_size==-1){ 
                sent_size = IP_send(buffer[0], tableau_sockets[mic_sock].remote_addr.ip_addr);
                j++; 
                if (j>10){
                    printf("Erreur dans IP_send \n"); 
                    return -1; 
                } 
            }
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
    return 0;
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

    // on trouve le numéro de socket correspondant au PDU reçu
    int trouve=0; // 0 -> faux, 1 -> vrai 
    int i = 0 ;
    mic_tcp_sock socket ;

    while((!trouve)&&(i<nb_socket)){
        if (tableau_sockets[i].local_addr.ip_addr.addr_size!=0){ // on compare les adresses seulement si l'adresse du socket du tableau est initialisée i.e. sa taille est > à 0 
            printf("Process_received : taille d'une addr > 0, socket %d \n",i); 
            printf("Addresse distante du PDU : %s \n", remote_addr.addr);
            printf("Addresse locale du socket : %s \n", tableau_sockets[i].local_addr.ip_addr.addr);
            if (strcmp(tableau_sockets[i].local_addr.ip_addr.addr,remote_addr.addr)==0){
                printf("Process_received : socket %d correspondant à l'addr trouvé \n",i); 
                trouve=1; 
                socket = tableau_sockets[i] ;
            }
        }
        else{
            i++;
        } 
    }
    printf("Process_received : socket concerné : %d\n", i); 

    // dans le cas où l'état est IDLE
    printf("Process_received : socket.state = IDLE \n");
    if ((tableau_sockets[i].state == IDLE) && (pdu.header.syn==1)) {// vérification de si c'est un SYN 
        tableau_sockets[i].state = SYN_RECEIVED ;
        pe=pdu.header.ack_num;
        printf("Pe mis à jour : %d \n",pe);
        tableau_sockets[i].remote_addr.ip_addr=local_addr;
        printf(" Remote_addr du socket mis à jour : %s \n",tableau_sockets[i].remote_addr.ip_addr.addr); 
        tableau_sockets[i].remote_addr.port=pdu.header.source_port;
        printf("Port de remote_addr mis à jour : %d \n", tableau_sockets[i].remote_addr.port); 
    
    // si l'état est SYN_SENT    
    } else if ((tableau_sockets[i].state == SYN_SENT) && (pdu.header.ack==1)) {
        tableau_sockets[i].state = SYN_RECEIVED ;
        
    // si l'état est ESTABLISHED
    } else if(socket.state==ESTABLISHED){ // on se charge de mettre les données dans les buffer et envoyer les ack 

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

        // négociation du pourcentage de pertes
        int pertes_serveur;
        char arg[10];
        pertes_serveur = (rand() % 50); //le serveur ne souhaite pas avoir des pertes superieures à 50%
        sprintf(arg,"%d",pertes_serveur);
            //payload 
        pdu_ack.payload.data=(char*)arg;
        pdu_ack.payload.size=sizeof(char);

        printf("serveur: pertes souhaitees : %d\n",pertes_serveur);

        buffer[0]=pdu_ack;
            
        printf("ACK NUM : %d \n",buffer[0].header.ack_num);

        int sent_size=-1; 
        int j=0; 
        while (sent_size==-1){ 
            sent_size = IP_send(buffer[0], tableau_sockets[i].remote_addr.ip_addr);
            j++; 
            if (j>10){
                perror("Erreur dans IP_send \n"); 
            } 
        }

    }
    else{
        printf("Etat du socket différent de IDLE, SYN_SENT et ESTABLISHED \n"); 
    } 
    printf ("Fin process_received\n");
   
}
