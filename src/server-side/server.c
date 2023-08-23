#include "defs-server.h"

int main(void){
    char str[UDP_RFTP_MAXPCKT];
    
    srand(2);
    
    last_win_ack.data       = (char*) malloc(UDP_RFTP_MAXLINE);
    last_win_ack.data[0]    = 0;
    chosen                  = 0;

    // L'indirizzo passato per le chiamate di `sendto` e `recvfrom`
    // sarà, per il client, quello del server
    // addr = &client_addr;
    
    send_msg.data       = (char*) malloc(UDP_RFTP_MAXLINE + 1);
    recv_msg.data       = (char*) malloc(UDP_RFTP_MAXLINE + 1);
    
    memset(send_msg.data, 0, UDP_RFTP_MAXLINE + 1);
    memset(recv_msg.data, 0, UDP_RFTP_MAXLINE + 1);

    memset((void*) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family        = AF_INET;
    serv_addr.sin_port          = htons(UDP_RFTP_SERV_PT);
    serv_addr.sin_addr.s_addr   = htonl(INADDR_ANY);
    
    // `sock_fd` è il descrittore della socket su cui il
    // server (o un suo child) scriverà nel momento in cui
    // vorrà inviare un datagramma al client
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("errore nella creazione del socket");
        exit(-1);
    }

    if(bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        printf("errore nell'esecuzione di bind");
        exit(-1);
    }       
    
    memset((void*) cl_sockfd, 0, sizeof(int*) * UDP_RFTP_MAXCLIENT);
    while(1){
        UDP_RFTP_recv_pckt();
        
        // Il processo padre del server gestisce soltanto
        // messaggi con indice progressivo nullo (caratteristico
        // dei messaggi di richieste put, get, list 
        UDP_RFTP_msg2str(&recv_msg, str);
        if(recv_msg.progressive_id != 0)
            continue;  
        
        puts("SERVER RECEIVED PACKET");
        puts(str);
        
        for(chosen = 0; chosen <= UDP_RFTP_MAXCLIENT && cl_sockfd[chosen] != 0; chosen++);
        
        if(chosen == UDP_RFTP_MAXCLIENT){
            waitpid(0, NULL, WNOHANG | WUNTRACED);
            continue;
        }

        switch(recv_msg.msg_type){
           case UDP_RFTP_GET :
               UDP_RFTP_handle_recv(recv_msg.data);
               break;
       
           case UDP_RFTP_LIST :
               UDP_RFTP_handle_recv(NULL);
               break;
       
           case UDP_RFTP_PUT :
               UDP_RFTP_handle_put(recv_msg.data);
               break;
       
           default :
               puts(str);
        }
    }

    exit(0);
}
