#include <mictcp.h>
#include <api/mictcp_core.h>

#define nb_socket 10

struct mic_tcp_sock tableau_sockets[nb_socket] ;
int pe; // c'est aussi Pa
struct mic_tcp_pdu buffer[1] ; // buffer pour stocker le PDU
unsigned long timer = 1000 ; //timer avant renvoie d'un PDU
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   tableau_sockets[socket].local_addr = addr ;
   return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    tableau_sockets[socket].remote_addr = addr ;
    return 0;
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

    // incrémenter Pe
    pe = (pe+1)%2 ;

    // envoyer le message (dont la taille et le contenu sont passés en paramètres)
    int sent_size = IP_send(buffer[0], le_socket.remote_addr.ip_addr) ; /* 2e arg = structure mic_tcp_socket_addr contenue 
    dans la structure mic_tcp_socket correspondant au socket identifié par mic_sock passé en paramètre*/

    bool recu = false ;
    int retour_recv = -1 ;
    int k = 0 ; /* variable qui permet de ne pas rester éternellement dans la boucle si on ne reçoit pas de message
    comme ça on n'en envoie pas 10 000 à la suite*/

    mic_tcp_pdu pdu_recu ; //TODO: initialiser le pdu

    while(!recu && k<100) { /* tant qu'on n'a pas fait trop ditérations, 
        et tant que l'accusé de réception n'est pas reçu, 
        sachant que son numéro doit correspondre au numéro de séquence du pdu contenu dans le buffer*/
        retour_recv = IP_recv(&pdu_recu, &le_socket.local_addr.ip_addr, &le_socket.remote_addr.ip_addr, timer) ; // TODO: mettre les arguments FAIT

        if((retour_recv != -1) && (pdu_recu.header.ack_num == buffer[0].header.seq_num)) {
            recu = true ;
        } else {
            sent_size = IP_send(buffer[0], le_socket.remote_addr.ip_addr) ; // TODO: mettre le deuxième argument FAIT
        }
        k++ ;
    }
    if(k < 100) {
        return sent_size;
    } else {
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
    if (pdu.payload.data==NULL){
        app_buffer_put(pdu.payload);
    }

    //envoyer le ack avec le bon numero 
        //creer un header
    mic_tcp_pdu pdu_ack;
    pdu_ack.header.ack_num=pe;
    pdu_ack.header.ack=1;
    pdu_ack.header.dest_port=pdu.header.source_port;
    pdu_ack.header.source_port=pdu.header.dest_port;
        //payload 
    pdu_ack.payload.data=NULL;
    pdu_ack.payload.size=0;

    buffer[0]=pdu_ack;
    IP_send(pdu,remote_addr);
    pe=(pe+1)%2;

    
}
