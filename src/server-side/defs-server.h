#include "defs.h"

// La `put` gestisce richieste di put del client                                                          
// Una tipica sequenza di indici progressivi e campo dati
// in una comunicazione put è data dalla seguente lista:
// - client: 0, nome file, numero pacchetti
// - server: -1 (riscontro)
// - client: n, porzione dati
// - server: -1, indici delle porzioni riscontrate (riscontro)
// - client: -1 se tutti i pacchetti son stati riscontrati
// - server: -1 (termine)
void UDP_RFTP_handle_put(char* fname){
    pid_t pid = fork();

    if(pid == -1){
        perror("errore in fork");
        exit(-1);
    }
    
    if(pid != 0)
        return;
    
    puts("SERVER HANDLING PUT REQUEST!");
    process_type = UDP_RFTP_PUT;
    
    char* fname_dup = strdup(fname);
    fname_dup       = strtok(fname_dup, ";");
    chdir("server-side/server_files");
    
    file = fopen(fname_dup, "r");
    if(file != NULL){
        snprintf(fname, UDP_RFTP_MAXLINE, "%s.dup", fname_dup);
        fclose(file);
    } else 
        snprintf(fname, UDP_RFTP_MAXLINE, "%s", fname_dup);
    
    if((file = fopen(fname, "w+")) == NULL){
        send_msg.msg_type = UDP_RFTP_ERR;
        UDP_RFTP_send_pckt();
        
        exit(-1);
    }
    
    fname_dup = strtok(NULL, ";");
    pckt_count = (size_t) strtoul(fname_dup, NULL, 10);
    
    if(pckt_count == 0){
        send_msg.msg_type = UDP_RFTP_ERR;
        UDP_RFTP_send_pckt();

        exit(-1);
    }

    printf("Expected %zu packets!\n", pckt_count);
    fflush(stdout);

    win = UDP_RFTP_MIN(pckt_count, UDP_RFTP_MAX_RECV_WIN);
    
    buffs = (char**) malloc(sizeof(char*) * win);
    for(size_t k = 0; k < win; k++){
        if((buffs[k] = (char*) malloc(UDP_RFTP_MAXLINE + 1)) == NULL){
            perror("errore in malloc");
            exit(-1);
        }

        memset(buffs[k], 0, UDP_RFTP_MAXLINE + 1);
    }
    pckts = calloc(1, sizeof(char*) * win);

    memcpy(&client_addr, &addr, sizeof(addr));

    // Associo alla socket in ascolto con indice `chosen` il
    // prioprio numero di porta
    // Questo numero di porta, che verrà comunicato al client con
    // l'invio del prossimo messaggio, sarà lo stesso a cui il client
    // invierà i prossimi pacchetti 
    cl_sockfd[chosen]   = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_port  = htons(UDP_RFTP_SERV_PT + chosen + 1);
    bind(cl_sockfd[chosen], (struct sockaddr*) &serv_addr, sizeof(serv_addr)); 
     
    send_msg.port_no        = htons(UDP_RFTP_SERV_PT + chosen + 1); 
    send_msg.msg_type       = process_type;
    send_msg.progressive_id = (size_t) -1;
    memset(send_msg.data, 0, UDP_RFTP_MAXLINE + 1); 
     
    // Il primo timer serve per evitare che il client invii
    // un messaggio di richiesta senza inviarne altri dopo
    // oppure, equivalentemente, se il client non dovesse
    // ricevere la comunicazione del numero di porta dal server
    // in qual caso, il client reinvierà sulla porta di default
    // del server il messaggio di richiesta 
    sa.sa_handler           = &UDP_RFTP_exit;
    sa.sa_flags             = 0;
    sigemptyset(&sa.sa_mask);
    
    set_timer.it_value.tv_usec  = UDP_RFTP_CONN_TOUT;
    set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
    set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;
     
    if(sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("errore in sigaction");
        exit(-1);
    }
    
    // `K` verrà impostato ad 1 quando il server
    // riceverà dal client un messaggio valido
    // successivo all'invio, da parte del server,
    // del numero di porta su cui comunicare
    int K = 0; 
    UDP_RFTP_send_ack(0);

    sock_fd = cl_sockfd[chosen]; 

    setitimer(ITIMER_REAL, &set_timer, NULL);
    UDP_RFTP_start_watch();
     
    // A differenza della `recv` del client il server
    // potrà solo ricevere pacchetti con indice progressivo
    // positivo di una porzione del file da caricare
    // Pertanto preleverà fino a compimento della richiesta
    // pacchetti dalla socket e per ogni finestra di 
    // pacchetti ricevuti invierà un riscontro cumulativo
    // (selective repeat sui pacchetti della finestra)
    while(1){
        UDP_RFTP_recv_pckt();

        if(recv_msg.msg_type == 0){
            // puts("Got nothing");
            // fflush(stdout);
            continue;
        }
        
        /* if((recv_msg.msg_type != process_type && recv_progressive_id != (size_t) -1) || addr.sin_port != client_addr.sin_port || addr.sin_addr.s_addr != client_addr.sin_addr.s_addr){
            puts("Unknown sender");
            fflush(stdout);
            continue;
        }*/

        // Quando `K = 0` la socket su cui il figlio del server
        // andrà a scivere sarà ancora la stessa aperta originariamente
        // dal padre
        // Pertanto il figlio dovrà interrompere il timer che avrebbe
        // altrimenti terminato il figlio, impostare l'evento
        // del timeout all'invio dei riscontri e cambiare la socket su cui
        // andrà a leggere e scrivere, quindi ignorando qualsiasi messaggio
        // proveniente dalla socket del padre
        if(K == 0){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);
            
            sa.sa_handler           = &UDP_RFTP_send_ack;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
            
            if(sigaction(SIGALRM, &sa, NULL) < 0){
                perror("errore in sigaction");
                exit(-1);
            }
            
            set_timer.it_value.tv_usec  = UDP_RFTP_BASE_TOUT;
            set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
            set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;

            K = 1;
        }
         
        recv_progressive_id = recv_msg.progressive_id;
        rel_progressive_id  = (ssize_t) (recv_progressive_id - ackd_wins * win - 1); 

        // Il messaggio ricevuto appartiene virtualmente alla finestra di ricezione
        // precedente a quella attuale, pertanto il server invierà l'ultimo riscontro
        // relativo la precedente finestra di ricezione
        if(rel_progressive_id < 0){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);

            send_msg.progressive_id = (size_t) -1;
            send_msg.msg_type = process_type;
            snprintf(send_msg.data, sizeof(size_t) + 1, "%zu;", recv_progressive_id);

            UDP_RFTP_send_pckt();

            setitimer(ITIMER_REAL, &set_timer, NULL);
            continue;
        }
        
        // Il messaggio ricevuto appartiene virtualmente alla finestra di ricezione
        // successiva a quella attuale
        if(rel_progressive_id >= (ssize_t) win || pckts[rel_progressive_id] != NULL)
            continue;

        // Rispetto alla precedente finestra di ricezione
        // è stato ricevuta una nuova porzione del file
        // In questo caso la gestione del timeout verrà
        // affidata all'invio dei riscontri 
        if(sa.sa_handler == &UDP_RFTP_send_pckt){ /**/
            sa.sa_handler           = &UDP_RFTP_send_ack;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
            
            if(sigaction(SIGALRM, &sa, NULL) < 0) {
                perror("errore in sigaction");
                exit(-1);
            }
        }
        
        // Interrompo il cronometro avviato all'ultimo riscontro
        // inviato
        setitimer(ITIMER_REAL, &cancel_timer, NULL);

        printf("OK\t%zu\n", recv_progressive_id);
        fflush(stdout);

        if(S == 0){
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);
            S = 1;
        }

        snprintf(buffs[rel_progressive_id], UDP_RFTP_MAXLINE + 1, "%s", recv_msg.data);
        pckts[rel_progressive_id] = buffs[rel_progressive_id];
 
        // Avendo riscontrato un numero di pacchetti maggiore
        // o uguale al totale numero di pacchetti del file,
        // il server può terminare la comunicazione con il client
        // Nello specifico attenderà che il client invii un messaggio
        // con indice progressivo pari a -1
        if((++ackd_pckts) >= pckt_count){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            puts("Received all packets!");
            
            UDP_RFTP_flush_pckts();

            memset((void*) pckts, 1, win * sizeof(char*));
            UDP_RFTP_send_ack(0);
            
            puts("Closing connection");
            send_msg.msg_type       = process_type;
            send_msg.progressive_id = (size_t) -1;
            memset(send_msg.data, 0, UDP_RFTP_MAXLINE + 1);
             
            UDP_RFTP_bye(); 
            return;
        }
        
        // La finestra di ricezione è piena, quindi il server può
        // spostarla e accettare i rimanenti pacchetti dal client
        if(ackd_pckts % win == 0 && ackd_pckts >= ackd_wins * win){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
                
            printf("New window!\n");
            fflush(stdout);

            UDP_RFTP_flush_pckts();
            UDP_RFTP_send_ack(0);
 
            retrans_count = 0;
            
            memset((void*) pckts, 0, win * sizeof(char*));
            ++ackd_wins;
            
            /* FORSE RIMUOVIBILE 
            // Finché il server non riceverà un nuovo pacchetto
            // l'invio del riscontro sarà affidato alla `send_last_pckt`
            // che reinvierà l'ultimo riscontro inviato e relativo
            // alla precedente finestra di ricezione 
            sa.sa_handler           = &UDP_RFTP_send_pckt;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
            
            if(sigaction(SIGALRM, &sa, NULL) < 0) {
                perror("errore in sigaction");
                exit(-1);
            }
            */
        }
        
        setitimer(ITIMER_REAL, &set_timer, NULL);
    }

    puts("PUT REQUEST FULFILLED!");
    
    return;
}

// La `recv` gestisce entrambe le richieste di                                                            
// `get` che `list`
// Una comunicazione di tipo `recv` si sviluppa sempre
// con la seguente sequenza di indici progressivi
// e dati:
// - client: 0, nome file (o lista)
// - server: 0, dimensione in pacchetti oppure
//           -1 se dimensione è 1
// - client: -1, elenco pacchetti riscontrati
// - server: n, porzione file (o lista)
// - client: se tutti pacchetti sono stati riscontrati
//          -1, elenco pacchetti riscontrati
// - server: -1 (termine)
// Più in generale il client inserisce un indice progressivo
// nullo solo all'invio di una richiesta, -1 per riscontri
// ai pacchetti ricevuti dal server mentre il server porrà
// indice progressivo nullo alla prima risposta al client,
// -1 per confermare la chiusura della comunicazione e per 
// file interamente contenuti in un pacchetto, altrimenti
// un numero maggiore di 0 
void UDP_RFTP_handle_recv(char* fname){
    // Inizio del setup per il corretto funzionamento del server
    pid_t pid = fork();

    if(pid == -1){
        perror("errore in fork");
        exit(-1);
    }
    
    if(pid != 0)
        return;
    
    char* fname_dup;
    chdir("server-side/server_files"); 
    if(fname == NULL){
        puts("SERVER HANDLING LIST REQUEST");
        system("ls > list.txt");
        fname = "list.txt";
        
        process_type = UDP_RFTP_LIST;
    }else{
        fname_dup = strdup(fname);
        puts("SERVER HANDLING GET REQUEST");
        
        process_type = UDP_RFTP_GET;
        
        snprintf(fname, UDP_RFTP_MAXLINE, "%s", strtok(fname_dup, ";"));
        free(fname_dup); 
    } 
    
    // Se il file non dovesse esistere il
    // server invierà al client un messaggio
    // con tipo `ERR` e terminerà 
    file = fopen(fname, "r");
    if(file == NULL){
        send_msg.msg_type = UDP_RFTP_ERR;
        UDP_RFTP_send_pckt();

        exit(-1);
    }
    
    printf("Requested file is %s\n", fname); 
    fflush(stdout);
    
    fseek(file, 0L, SEEK_END);
    pckt_count = ftell(file);

    // Simimente se il file dovesse essere
    // vuoto il server invierà al client
    // un messaggio con tipo `ERR` e 
    // terminerà
    if(pckt_count == 0){
        send_msg.msg_type = UDP_RFTP_ERR;
        UDP_RFTP_send_pckt();

        exit(-1);
    }

    pckt_count = pckt_count / UDP_RFTP_MAXLINE + (pckt_count % UDP_RFTP_MAXLINE == 0 ? 0 : 1);
    rewind(file);

    printf("Expected packets\t\t[\t%zu\t]\n", pckt_count);
    fflush(stdout);
    
    // Ad ogni timeout corrisponderà il reinvio del primo
    // messaggio verso il client, quindi il messaggio
    // che nel campo dati contiene il numero di pacchetti
    // che costituisce il file richiesto
    sa.sa_handler           = &UDP_RFTP_send_pckt; /**/
    sa.sa_flags             = 0;
    sigemptyset(&sa.sa_mask);
    
    if(sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("errore in sigaction");
        exit(-1);
    }
    
    memcpy(&client_addr, &addr, sizeof(addr));
    // Fine del setup lato server

    // Se il file richiesto o la lista dei file può essere
    // interamente contenuta in un solo pacchetto, l'indice progressivo
    // verrà impostato a -1 e nel campo dati l'intero file (lista)
    if(pckt_count <= 1){
        puts("Sending unique packet");
   
        // Associo alla socket in ascolto con indice `chosen` il
        // prioprio numero di porta
        // Questo numero di porta, che verrà comunicato al client con
        // l'invio del prossimo messaggio, sarà lo stesso a cui il client
        // invierà i prossimi pacchetti 
        cl_sockfd[chosen]   = socket(AF_INET, SOCK_DGRAM, 0);
        serv_addr.sin_port  = htons(UDP_RFTP_SERV_PT + chosen + 1);
        bind(cl_sockfd[chosen], (struct sockaddr*) &serv_addr, sizeof(serv_addr));
        
        send_msg.msg_type       = process_type; 
        send_msg.port_no        = htons(UDP_RFTP_SERV_PT + chosen + 1); 
        send_msg.progressive_id = (size_t) -1;
        
        fread((void*) send_msg.data, UDP_RFTP_MAXLINE, 1, file);

        if(ferror(file)){
            perror("errore in fread");
            send_msg.msg_type = UDP_RFTP_ERR;
            UDP_RFTP_send_pckt();
            exit(-1);
        }
         
        UDP_RFTP_send_pckt();
        
        sock_fd = cl_sockfd[chosen];
        
        if(process_type == UDP_RFTP_LIST)
            puts("LIST REQUEST FULFILLED!");
        else
            puts("GET REQUEST FULFILLED!");
        fflush(stdout); 
        
        UDP_RFTP_bye();
        UDP_RFTP_exit(0);
        return;
    }
    
    // Qualora un solo pacchetto non dovesse bastare
    // impostiamo una finestra di spedizione pari
    // al più al valore massimo configurabile
    win = UDP_RFTP_MIN(pckt_count, UDP_RFTP_BASE_SEND_WIN);
    snprintf(send_msg.data, UDP_RFTP_MAXLINE + 1, "%zu", pckt_count);
    base_prev_win   = 0;
    base_next_win   = win;

    rewind(file);

    // Il ruolo di `buffs` e `pckts` nella `recv` del server
    // è lo stesso dei `buffs` e `pckts` nella `put` del client
    buffs = (char**) malloc(sizeof(char*) * win);
    for(size_t k = 0; k < win; k++){
        if((buffs[k] = (char*) malloc(UDP_RFTP_MAXLINE + 1)) == NULL){
            perror("errore in malloc");
            exit(-1);
        }
        memset(buffs[k], 0, UDP_RFTP_MAXLINE + 1);
    } 

    pckts = calloc(1, sizeof(char*) * win); 
    
    rel_progressive_id = 0;
    
    // Carichiamo nei buffer le prime porzioni del file (lista)
    for(size_t k = 0; k < UDP_RFTP_MIN(win, pckt_count); k++){
        fread((void*) (buffs[k]), UDP_RFTP_MAXLINE, 1, file);
        pckts[k] = buffs[k];
    }
    
    // Associo alla socket in ascolto con indice `chosen` il
    // prioprio numero di porta
    // Questo numero di porta, che verrà comunicato al client con
    // l'invio del prossimo messaggio, sarà lo stesso a cui il client
    // invierà i prossimi pacchetti 
    cl_sockfd[chosen]   = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_port  = htons(UDP_RFTP_SERV_PT + chosen + 1);
    bind(cl_sockfd[chosen], (struct sockaddr*) &serv_addr, sizeof(serv_addr));
   
    send_msg.port_no        = htons(UDP_RFTP_SERV_PT + chosen + 1); 
    send_msg.msg_type       = process_type;
    send_msg.progressive_id = 0;
    
    // Qualora il client non dovesse rispondere in tempo
    // al primo messaggio inviato dal server, il server terminerà 
    sa.sa_handler           = &UDP_RFTP_exit;
    sa.sa_flags             = 0;
    sigemptyset(&sa.sa_mask);
    
    set_timer.it_value.tv_usec = UDP_RFTP_CONN_TOUT;
    set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
    set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;
     
    if(sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("errore in sigaction");
        exit(-1);
    }

    int K = 0;

    // È importante osservare che questo pacchetto
    // verrà inviato sulla socket del processo padre
    // Solo dopo aver inviato questo pacchetto il server
    // cambierà la propria socket di scrittura con quella
    // appena creata
    UDP_RFTP_send_pckt();
    
    sock_fd = cl_sockfd[chosen];
    setitimer(ITIMER_REAL, &set_timer, NULL);
    UDP_RFTP_start_watch();

    while(1){ 
        UDP_RFTP_recv_pckt();
        
        if(recv_msg.msg_type == 0){
            // puts("Got nothing");
            // fflush(stdout);
            continue;
        }

        /* if((recv_msg.msg_type != process_type && recv_progressive_id != (size_t) -1) || addr.sin_port != client_addr.sin_port || addr.sin_addr.s_addr != client_addr.sin_addr.s_addr){
            puts("Unknown sender");
            fflush(stdout);
            continue;
        }*/

        // È stato ricevuto il primo riscontro dal client, quindi d'ora in poi
        // il server potrà, a ciascun suo timeout, ritrasmettere i pacchetti della corrente
        // finestra di spedizione
        if(K == 0){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);
    
            sa.sa_handler           = &UDP_RFTP_retrans_pckts;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
             
            if(sigaction(SIGALRM, &sa, NULL) < 0){
                perror("errore in sigaction");
                exit(-1);
            }
            
            set_timer.it_value.tv_usec  = UDP_RFTP_BASE_TOUT;
            set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
            set_timer.it_value.tv_usec  = set_timer.it_value.tv_usec % 1000000;
            
            K = 1;
        }
 
        if(S == 0){ 
            // Interrompo il timer impostato dall'ultima
            // `retrans_pckts` 
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH); 
            S = 1;
        } 

        char* data_dup  = strdup(recv_msg.data); 
        char* elm       = strtok(data_dup, ";");
        
        // Il riscontro con campo dati vuoto indica
        // la non ricezione di alcun pacchetto
        if(elm == NULL){
            UDP_RFTP_retrans_pckts(0);
            continue;
        }
        
        // Nel campo dati dell'ultimo pacchetti è presente una lista
        // separata da `;` di indici dei pacchetti correttamente riscontrati
        estimated_win = UDP_RFTP_MAX(acks_per_pckt, estimated_win);
        acks_per_pckt = 0;
        do{
            ++acks_per_pckt;

            recv_progressive_id = (size_t)  strtoul(elm, NULL, 10);
            rel_progressive_id  = (ssize_t) (recv_progressive_id - base_prev_win - 1);

            if(rel_progressive_id < (ssize_t) win && rel_progressive_id >= 0 && pckts[rel_progressive_id] != NULL){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                
                printf("OK\t%zu\n", recv_progressive_id);
                fflush(stdout);
                 
                pckts[rel_progressive_id] = NULL;
                ++ackd_pckts;
            }
        
            // Sono stati riscontrati tutti i segmenti del file (lista)
            if(ackd_pckts >= pckt_count){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);

                printf("Sent all desired packets!\n");
                fflush(stdout);
                
                memset((void*) pckts, 0, win * sizeof(char*));
                UDP_RFTP_retrans_pckts(0);
                
                send_msg.msg_type       = process_type;
                send_msg.progressive_id = (size_t) -1;
                send_msg.data           = "";
                
                if(process_type == UDP_RFTP_LIST)
                    puts("LIST REQUEST FULFILLED!");
                else
                    puts("GET REQUEST FULFILLED!");
                
                free(data_dup);

                UDP_RFTP_bye();
                return;
            }
            
            // Tutti i pacchetti nella finestra corrente sono stati riscontrati,
            // quindi la finestra di spedizione può essere traslata
            if(ackd_pckts - base_prev_win >= win && ackd_pckts != 0){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                 
                printf("New window!\n");
                fflush(stdout);
                
                memset((void*) pckts, 0, win * sizeof(char*));
                ++ackd_wins;
                
                // La politica di aggiornamento della finestra di spedizione
                // deve saper riconoscere la taglia della finestra di ricezione
                // così da allineare i propri slot di trasmissione con quelli
                // di ricezione del client
                // Ponendo non ci siano state ritrasmissioni `win` assumerà valore
                // `UDP_RFTP_MAX(estimated_win * 1, UDP_RFTP_MIN_SEND_WIN)`
                // e siccome `estimated_win` è il massimo numero di pacchetti per
                // riscontro finora ricevuto, `win` sarà esattamente la taglia
                // della finestra di ricezione
                // Se `retrans_count > 0` allora `win` assumerà un valore più piccolo
                // di `estimated_win` ma comunque suo sotto-multiplo
                size_t temp_win;
                if(win > estimated_win)
                    temp_win = UDP_RFTP_MAX(
                            estimated_win / (1 + retrans_count / (win * estimated_win)),
                            UDP_RFTP_MIN_SEND_WIN
                    );
                else
                    temp_win = UDP_RFTP_MAX(
                            estimated_win / (1 + retrans_count / estimated_win),
                            UDP_RFTP_MIN_SEND_WIN
                    );

                temp_win = UDP_RFTP_MIN(
                        temp_win,
                        UDP_RFTP_MAX_SEND_WIN
                );
                
                retrans_count = 0;
                
                base_prev_win += win;
                base_next_win = base_prev_win + temp_win;

                if(temp_win != win){
                    buffs = (char**) realloc((void*) buffs, sizeof(char*) * temp_win);
                    
                    pckts = (char**) realloc((void*) pckts, sizeof(char*) * temp_win);
                    memset((void*) pckts, 1, temp_win * sizeof(char*));    
                }

                // La finestra di spedizione viene ampiata, quindi dobbiamo
                // riallocare `buffs` e `pckts` così come riservare nuovo spazio
                // per le nuove porzioni di file `buffs[k]` 
                if(temp_win > win){
                    for(size_t k = win; k < temp_win; k++)
                        if((buffs[k] = (char*) malloc(UDP_RFTP_MAXLINE + 1)) == NULL){
                            perror("errore in malloc");
                            exit(-1);
                        }
                }
                
                win = temp_win;

                for(size_t k = 0; k < win; k++){
                    memset(buffs[k], 0, UDP_RFTP_MAXLINE + 1);
                    fread((void*) (buffs[k]), UDP_RFTP_MAXLINE, 1, file);
                     
                    if(ferror(file)){
                        perror("errore in fread");
                        exit(-1);
                    }

                    pckts[k] = buffs[k];
                }

                UDP_RFTP_retrans_pckts(0);
                S = 0;

                break;
            }
             
            elm = strtok(NULL, ";");
            setitimer(ITIMER_REAL, &set_timer, NULL);
        }while(elm != NULL);
        free(data_dup);
    }
 
    if(process_type == UDP_RFTP_LIST)
        puts("LIST REQUEST FULFILLED!");
    else
        puts("GET REQUEST FULFILLED!");
    
    UDP_RFTP_exit(0);
    return;
}
