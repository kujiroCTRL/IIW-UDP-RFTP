#include <netinet/in.h> 
#include <errno.h>
#include <fcntl.h> 
#include <netdb.h>

#include <time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/types.h> 
#include <sys/socket.h> 

#include <arpa/inet.h>

#include <unistd.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define UDP_RFTP_SERV_PT            (5193)
#define UDP_RFTP_SERV_IP            ("127.0.0.1")

#define UDP_RFTP_MAXPCKT            (1280)
#define UDP_RFTP_MAXLINE            (1024)
#define UDP_RFTP_MAXCLIENT          (12)

#define UDP_RFTP_GET                (51)
#define UDP_RFTP_PUT                (12)
#define UDP_RFTP_LIST               (4)

#define UDP_RFTP_ERR                (60)

// La finestra di ricezione ha taglia fissa
#define UDP_RFTP_MAX_RECV_WIN       (5)

// La finestra di spedizione può variare
// entro un valore minimo e massimo entrambi
// fissi
#define UDP_RFTP_BASE_SEND_WIN      (10)
#define UDP_RFTP_MIN_SEND_WIN       (2)
#define UDP_RFTP_MAX_SEND_WIN       (128)

#define UDP_RFTP_SAVE_ACK           (1)

#define UDP_RFTP_BASE_TOUT          (500)
#define UDP_RFTP_MIN_TOUT           (100)
#define UDP_RFTP_CONN_TOUT          (3000000)
#define UDP_RFTP_MAXBYE             (4)
#define UDP_RFTP_MAXRETRANS         (64)

// Il timeout è gestito secondo una politica
// multiplicative increase, averaging decrease
// con rapporti d'incremento specificati nelle
// macro `MULT_TOUT` e `AVRG_TOUT`
#define UDP_RFTP_MULT_TOUT(t)       (17 * (t) / 16)
#define UDP_RFTP_AVRG_TOUT(t, T)    (1 * (t) / 2 + 1 * (T) / 2)
#define UDP_RFTP_UPDT_TOUT(t, T)    (t < T ? UDP_RFTP_MULT_TOUT(t) : UDP_RFTP_AVRG_TOUT(t, T))

#define UDP_RFTP_SET_WATCH          (1)
#define UDP_RFTP_LOSS_RATE          (50)

#define UDP_RFTP_MIN(x, y)          ((x) < (y) ? (x) : (y))
#define UDP_RFTP_MAX(x, y)          ((x) > (y) ? (x) : (y))

struct timespec measured_time;

typedef struct {
    uint16_t        port_no;
    unsigned int    msg_type;
    size_t          progressive_id;
    char*           data;
} UDP_RFTP_msg;

unsigned int process_type;                                                                
FILE* file;

// Variabili per la gestione della ricezione
// e spedizione di pacchetti
size_t  pckt_count          = 0,
        ackd_pckts          = 0,
        ackd_wins           = 0,
        win                 = 0,
        recv_progressive_id = 0,
        retrans_count       = 0;

ssize_t rel_progressive_id = -1;

UDP_RFTP_msg last_win_ack;

// Strutture rispettivamente per la spedizione
// e ricezione dei pacchetti
UDP_RFTP_msg send_msg;
UDP_RFTP_msg recv_msg;

char** buffs;
char** pckts;

// Variabili per la gestione degli indirizzi
struct sockaddr_in      serv_addr;
struct sockaddr_in      client_addr;
struct sockaddr_in      addr;

socklen_t               len;
int chosen;
int sock_fd;
int cl_sockfd[UDP_RFTP_MAXCLIENT];

struct itimerval set_timer      =
{
    .it_interval.tv_sec         = 0,
    .it_interval.tv_usec        = 0,
    .it_value.tv_sec            = 0, 
    .it_value.tv_usec           = UDP_RFTP_CONN_TOUT
};

struct itimerval cancel_timer   =
{
    .it_interval.tv_sec         = 0,
    .it_interval.tv_usec        = 0,
    .it_value.tv_sec            = 0,
    .it_value.tv_usec           = 0
};

double secs, nanosecs;
struct sigaction sa;
int S;

void UDP_RFTP_exit(int signo){    
    if(!send_msg.data)
        free(send_msg.data);
    if(!recv_msg.data)
        free(recv_msg.data);
    close(sock_fd);
    fclose(file);
    cl_sockfd[chosen] = 0;

    for(size_t k = 0; k < win; k++){
        if(!buffs[k])
            free(buffs[k]);
    }
    if(!buffs)
        free(buffs);
    if(!pckts)
        free(pckts);
    
    puts("Exiting, bye bye");
    fflush(stdout); 
    exit(signo);
}

// Interrompe il cronometro per la stima del tempo
// di andata-ritorno
void UDP_RFTP_stop_watch(int update){
    #ifdef UDP_RFTP_DYN_TOUT
    clock_gettime(CLOCK_REALTIME, &measured_time);
    secs        = measured_time.tv_sec - secs;
    nanosecs    = measured_time.tv_nsec - nanosecs; 
    
    // printf("Recorded time consists of %f secs and %f nanosecs\n", secs, nanosecs); 
    if(secs < 0 || nanosecs < 0)
        return;
     
    if(update != UDP_RFTP_SET_WATCH)
        return;
    
    set_timer.it_value.tv_usec =
        UDP_RFTP_UPDT_TOUT(
            set_timer.it_value.tv_usec + set_timer.it_value.tv_sec * 1000000,
            (long) (secs * 1000000 + nanosecs / 1000)
        );
    
    set_timer.it_value.tv_usec =
        UDP_RFTP_MIN(set_timer.it_value.tv_usec, UDP_RFTP_BASE_TOUT);
    set_timer.it_value.tv_usec =
        UDP_RFTP_MAX(set_timer.it_value.tv_usec, UDP_RFTP_MIN_TOUT);

    set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
    set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;
   
    printf("New timer consists of %ld secs and %ld microsecs\n", set_timer.it_value.tv_sec, set_timer.it_value.tv_usec); 
    #endif
    return;
}

// Avvia il cronometro per la stima del tempo
// di andata-ritorno
void UDP_RFTP_start_watch(void){
   #ifdef UDP_RFTP_DYN_TOUT
   clock_gettime(CLOCK_REALTIME, &measured_time); 
   secs         = measured_time.tv_sec;
   nanosecs     = measured_time.tv_nsec;
   #endif
   
   return;
}

// Converte la struttura messaggio `msg` nella sua forma
// equivalente di stringa
void UDP_RFTP_msg2str(UDP_RFTP_msg* msg, char* str){
    snprintf(str, UDP_RFTP_MAXPCKT,
            "%d,%u,%zu,%s",
            msg -> port_no,
            msg -> msg_type,
            msg -> progressive_id,
            msg -> data
            );
    
    return;
}

// Converte la stringa `str` nel suo formato equivalente
// di messaggio `msg`
void UDP_RFTP_str2msg(char* str, UDP_RFTP_msg* msg){
    char* str_dup1 = strdup(str);
    char* str_dup2;
    int n = 0;
    
    str_dup2 = strtok(str_dup1, ",");
    n += strlen(str_dup2);
    msg -> port_no          = (uint16_t) strtoul(str_dup2, NULL, 10);
    
    str_dup2 = strtok(NULL, ",");
    n += strlen(str_dup2);
    msg -> msg_type         = (unsigned int) strtoul(str_dup2, NULL, 10);
    
    str_dup2 = strtok(NULL, ",");
    n += strlen(str_dup2);
    msg -> progressive_id   = (size_t) strtoul(str_dup2, NULL, 10);
    
    // Per il campo `data` non uso la `strtok` in quanto 
    // nel file (list) trasmesso potrebbero esserci caratteri
    // `,`
    memset(msg -> data, 0, UDP_RFTP_MAXLINE);
    snprintf(msg -> data, UDP_RFTP_MAXLINE + 1, "%s", str + n + 3);
    
    free(str_dup1);
    return;
}

// Inoltro di un messaggio: il messaggio `send_msg` viene convertito
// in stringa e inoltrato sulla socket in uscita
void UDP_RFTP_send_pckt(void){
    char sendline[UDP_RFTP_MAXPCKT];
    memset(sendline, 0, UDP_RFTP_MAXPCKT);
    UDP_RFTP_msg2str(&send_msg, sendline);
    
    len = (socklen_t) sizeof(addr); 
    
    int n = 0;
    if(rand() % 100 >= UDP_RFTP_LOSS_RATE)
        n = sendto(sock_fd, sendline, UDP_RFTP_MAXPCKT, 0, (struct sockaddr*) &addr, len);
    else {
        //printf("Lost packet!\n");
        fflush(stdout); 
    }
    
    if(n < 0 && errno != EINTR) {
        perror("errore in sendto");
        exit(1);
    }
    
    return;
}

// Ricezione di un messaggio: viene prelevato un datagramma
// della socket in ricezione e viene convertita in una struttura
// messaggio da cui è possibile estrarre i campi del messaggio
// La struttura sarà sempre `recv_msg`
void UDP_RFTP_recv_pckt(void){
    char recvline[UDP_RFTP_MAXPCKT];
    len = (socklen_t) sizeof(addr); 
     
    recv_msg.msg_type = 0; 
    int n = recvfrom(sock_fd, recvline, UDP_RFTP_MAXPCKT, 0, (struct sockaddr*) &addr, &len);
      
    if(errno != EINTR && n < 0){
        perror("errore in recvfrom");
        exit(-1);
    }
    
    if(n != UDP_RFTP_MAXPCKT)
        return;
     
    UDP_RFTP_str2msg(recvline, &recv_msg);
    
    #ifdef UDP_RFTP_DYN_TOUT 
    // Nel caso di  timeout il valore di timeout viene incrementato
    if(recv_msg.msg_type == 0){
        set_timer.it_value.tv_usec  =
            UDP_RFTP_MULT_TOUT(set_timer.it_value.tv_usec + 1000000 * set_timer.it_value.tv_sec);

        set_timer.it_value.tv_usec =
            UDP_RFTP_MIN(set_timer.it_value.tv_sec, UDP_RFTP_BASE_TOUT);
        set_timer.it_value.tv_usec =
            UDP_RFTP_MAX(set_timer.it_value.tv_usec, UDP_RFTP_MIN_TOUT);

        set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
        set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;
        
        printf("New timer consists of %ld secs and %ld microsecs\n", set_timer.it_value.tv_sec, set_timer.it_value.tv_usec); 
    }
    #endif

    return;
}

// Vengono stampati su file il contenuto dei buffer
void UDP_RFTP_flush_pckts(void){
    for(size_t k = 0; k < win; k++){
        if(pckts[k] == NULL)
            continue;
         
        fwrite(buffs[k], strlen(buffs[k]), 1, file); 
        // fputs(buffs[k], file);
        // fprintf(file, "%s", buffs[k]);
    }
    fflush(file);
    
    return;
}

// Viene inoltrato il contenuto di `send_msg` che, nella maggior parte
// dei casi, corrisponderà al reinviare la richiesta alla connessione
void UDP_RFTP_send_last_pckt(int signo){
    setitimer(ITIMER_REAL, &cancel_timer, NULL);
    puts("Didn't receive response, resending request");

    // UDP_RFTP_stop_watch(0);
    UDP_RFTP_send_pckt();
    // UDP_RFTP_start_watch();

    setitimer(ITIMER_REAL, &set_timer, NULL);
    return;
}

// Procedura inizializzata dal client per chiudere la connessione
// instaurata con il server
// Periodicamente viene inviato un messaggio di riscontro fino ad
// un numero fisso e prestabilito di tentativi o finché non si 
// riceve un messaggio di risposta dal server
void UDP_RFTP_bye(void){
    fflush(file);
    size_t bye_count = 0; 
    
    sa.sa_handler       = &UDP_RFTP_send_pckt; // send_last_pckt
    sa.sa_flags         = 0;
    sigemptyset(&sa.sa_mask);

    if(sigaction(SIGALRM, &sa, NULL) < 0){
        perror("errore in sigaction");
        exit(-1);
    }
     
    UDP_RFTP_send_pckt();

    do{  
        printf("Bye no %zu\n", ++bye_count);
        fflush(stdout);
        
        setitimer(ITIMER_REAL, &set_timer, NULL);
        UDP_RFTP_recv_pckt();
        setitimer(ITIMER_REAL, &cancel_timer, NULL);
         
        if(recv_msg.progressive_id == (size_t) -1 && recv_msg.msg_type != 0){
            puts("Received response, exiting");
            break;
        }
    } while(bye_count < UDP_RFTP_MAXBYE);
    
    UDP_RFTP_exit(0);
    return;
}

// Ogni componente non nulla in `pckts` corrisponde 
// ad un messaggio ricevuto e andrebbe pertanto riscontrato
// Un messaggio di riscontro avrà nella porzione dati
// una lista separata da `;` degli indici dei pacchetti ricevuti
void UDP_RFTP_send_ack(int signo){
    setitimer(ITIMER_REAL, &cancel_timer, NULL);
    
    char*   ack_data = calloc(1, UDP_RFTP_MAXLINE); 
    char    tmp[sizeof(size_t) + 1];
    size_t  q;
    
    for(size_t k = 0; k < win; k++){
        q = k + ackd_wins * win + 1;
        if(q > pckt_count)
            break;
        
        if(pckts[k] != NULL){
            snprintf(tmp, sizeof(size_t) + 1, "%zu;", q);
            strcat(ack_data, tmp);
            
            // È possibile che il ricevente rimanga in attesa
            // per una risposta dal mittente che non arriverà mai
            if(signo == SIGALRM)
                if((++retrans_count) >= win * UDP_RFTP_MAXRETRANS){
                    printf("Sender may be dead\n");
                    UDP_RFTP_exit(1);
                }
        }
    }

    send_msg.msg_type        = process_type;
    send_msg.progressive_id  = (size_t) -1;
    snprintf(send_msg.data, UDP_RFTP_MAXLINE, "%s", ack_data);
    
    // È possibile salvare nella variabile `last_win_ack`
    // l'ultimo ack inviato
    //if(signo == UDP_RFTP_SAVE_ACK)
    //    memcpy(&last_win_ack, &send_msg, sizeof(send_msg));
    UDP_RFTP_send_pckt();
    
    // Viene avviato un cronometro per la stima
    // del tempo andata-ritorno per pacchetti
    // successivo al primo
    S = 0;
    free(ack_data);
    UDP_RFTP_start_watch();

    setitimer(ITIMER_REAL, &set_timer, NULL);
    return;
}

// Al timeout del mittente può corrispondere il reinoltro
// di tutti i pacchetti non riscontrati in `win`
void UDP_RFTP_retrans_pckts(int signo){ 
    setitimer(ITIMER_REAL, &cancel_timer, NULL);
     
    send_msg.msg_type = process_type;
    
    for(size_t k = 0; k < win; k++){
        if(pckts[k] == NULL)
            continue;
        
        // È possibile che il mittente rimanga in attesa
        // per una risposta dal ricevente che non arriverà mai
        if(signo == SIGALRM)
            if((++retrans_count) >= win * UDP_RFTP_MAXRETRANS){
                printf("Receiver may be dead\n");
                UDP_RFTP_exit(1);
            }

        send_msg.progressive_id  = k + win * ackd_wins + 1;
        snprintf(send_msg.data, UDP_RFTP_MAXLINE + 1, "%s", buffs[k]);
        
        UDP_RFTP_send_pckt();
    }
    
    // Avvio unico cronometro per la sequenza di pacchetti ritrasmessi
    S = 0;
    UDP_RFTP_start_watch();

    setitimer(ITIMER_REAL, &set_timer, NULL);
    return;
}
