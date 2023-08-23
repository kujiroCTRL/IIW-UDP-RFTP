#include "defs.h"

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
void UDP_RFTP_generate_recv(char* fname){
    // Inizio del setup per il corretto funzionamento del client 
    pid_t pid = fork();

    if(pid == -1){
        perror("errore in fork");
        exit(-1);
    }
    
    if(pid != 0)
        return;
    
    if(fname == NULL){
        puts("LIST PROCESS HAS SUCCESFULLY STARTED IT'S EXECUTION!");
        process_type = UDP_RFTP_LIST; 
    }else{
        puts("GET PROCESS HAS SUCCESFULLY STARTED IT'S EXECUTION!");
        process_type = UDP_RFTP_GET;
    }
    
    chdir("client-side/server_files");
   
    char fname_dup[UDP_RFTP_MAXLINE];
    if(process_type == UDP_RFTP_LIST)
        snprintf(fname_dup, UDP_RFTP_MAXLINE, "list.txt");
    else{ 
        file = fopen(fname, "r");
        
        if(file != NULL){
            snprintf(fname_dup, UDP_RFTP_MAXLINE, "%s.dup", fname);
            fclose(file); 
        } else
            snprintf(fname_dup, UDP_RFTP_MAXLINE, "%s", fname);
    }

    if((file = fopen(fname_dup, "w+")) == NULL){
        perror("errore in fopen");
        exit(-1);
    } 
    
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("errore in socket");
        exit(-1);
    }
    
    // In attesa della risposta alla richiesta, al timeout
    // corrisponderà un inoltro della richiesta di connessione
    sa.sa_handler           = &UDP_RFTP_send_pckt; /**/
    sa.sa_flags             = 0;
    sigemptyset(&sa.sa_mask);
    
    if(sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("errore in sigaction");
        exit(-1);
    } 
    
    set_timer.it_value.tv_usec = UDP_RFTP_BASE_TOUT;    
    set_timer.it_value.tv_sec   = set_timer.it_value.tv_usec / 1000000;
    set_timer.it_value.tv_usec  %= 1000000;
    
    // Messaggio di richiesta all connessione
    send_msg.msg_type       = process_type;
    send_msg.progressive_id = 0;
    send_msg.data           = fname;
    // Fine del setup lato client 
     
    // Avvio cronometro per la prima volta
    // Questo cronometro verrà disattivato nel momento
    // si riceva un primo pacchetto, quindi con indice
    // progressivo nullo o -1 
    // UDP_RFTP_send_pckt();
    
    UDP_RFTP_start_watch(); 
    setitimer(ITIMER_REAL, &set_timer, NULL);
     
    // Viene impostato per la prima volta un timeout
    // per l'inoltro della richiesta di connessione
    // Il timeout verrà rigenerato ogni volta avviene
    // un timeout o qualora vengano ricevuti una quantità 
    // sufficiente di pacchetti
    while(1){
        UDP_RFTP_recv_pckt();
        
        if(recv_msg.msg_type == 0){
            // puts("Got nothing");
            // fflush(stdout);
            if(sa.sa_handler == &UDP_RFTP_send_pckt)
                setitimer(ITIMER_REAL, &set_timer, NULL);
            continue;
        }
        
        if(recv_msg.msg_type == UDP_RFTP_ERR){
            perror("errore nel server");
            exit(-1);
        }

        if(addr.sin_port != serv_addr.sin_port || addr.sin_addr.s_addr != serv_addr.sin_addr.s_addr){
            puts("Unknown sender");
            fflush(stdout);
            continue;
        } 

        recv_progressive_id = recv_msg.progressive_id;
        
        // Messaggio iterativo: un primo pacchetto è stato già
        // ricevuto e il numero di pacchetti che dovrebbero
        // essere ricevuti è maggiore di 1
        // Il campo dati dell'`n`-esimo messaggio iterativo è
        // l'`n`-esima porzione dei dati richiesti
        if(recv_progressive_id != 0 && pckt_count != 0){ 
            rel_progressive_id = (ssize_t) (recv_progressive_id - ackd_wins * win - 1);
            
            // Nel caso in cui il pacchetto ricevuto appartenga
            // alla finestra di ricezione precedente, inviamo un
            // riscontro
            // Questo in quanto i riscontri inviati dal client
            // sono soltanto dei pacchetti della finestra corrente
            // e, per finestre di ricezione-client e invio-server
            // di taglie sufficientemente diverse in taglia, questo
            // potrebbe portare ad uno stallo nella trasmissione del
            // file
            // Per ulteriori dettagli vedere il file `ISSUES.md` 
            if(rel_progressive_id < 0){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                
                send_msg.progressive_id = (size_t) -1;
                send_msg.msg_type       = process_type;
                snprintf(send_msg.data, sizeof(size_t) + 1, "%zu;", recv_progressive_id);
                
                UDP_RFTP_send_pckt();

                setitimer(ITIMER_REAL, &set_timer, NULL);
                continue;
            }

            if(rel_progressive_id >= (ssize_t) win || pckts[rel_progressive_id] != NULL)
                continue;
            
            setitimer(ITIMER_REAL, &cancel_timer, NULL); 
            
            printf("OK\t%zu\n", recv_progressive_id);
            fflush(stdout);
            
            // UDP_RFTP_send_ack(0); /**/ 

            // Viene interrotto il cronometro impostato nella precedente
            // chiamata della `send_ack` 
            // Il controllo su `S` è dovuto dal fatto che il cronometro 
            // viene avviato solo all'invio dei riscontri che non necessariamente
            // corrispondono biiunivocamente con i pacchetti ricevuti
            // Pertanto il cronometro verrà fermato solo alla ricezione
            // del primo pacchetto successivo al precedente invio dei riscontri
            if(S == 0){
                UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);
                S = 1;
            }

            snprintf(buffs[rel_progressive_id], UDP_RFTP_MAXLINE + 1, "%s", recv_msg.data);
            pckts[rel_progressive_id] = buffs[rel_progressive_id];
 
            // Sono stati riscontrati tutti i messaggi che sarebbero dovuti
            // essere inviati
            // Colui che ha richiesto dati invierà periodicamente dei messaggi
            // per la chiusura della connessione
            if((++ackd_pckts) >= pckt_count){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                puts("Received all packets!");
                
                UDP_RFTP_flush_pckts();

                memset((void*) pckts, 1, win * sizeof(char*));
                UDP_RFTP_send_ack(0);
                
                puts("Closing connection");
                send_msg.msg_type       = process_type;
                send_msg.progressive_id = (size_t) -1;
                send_msg.data           = "";
           
                UDP_RFTP_bye();
                return;
            }
            
            // I messaggi nella finestra di ricezione sono stati riscotrati
            if(ackd_pckts % win == 0 && ackd_pckts >= ackd_wins * win){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                
                printf("New window!\n");
                fflush(stdout);
 
                UDP_RFTP_flush_pckts();
                UDP_RFTP_send_ack(0);
                 
                retrans_count = 0;
                 
                memset((void*) pckts, 0, win * sizeof(char*));
                ++ackd_wins;
            }
            
            setitimer(ITIMER_REAL, &set_timer, NULL);
        }
        
        // È stato ricevuto il primo messaggio in risposta alla richiesta
        // di connessione e il numero di pacchetti in attesa è maggiore di 1
        if(recv_progressive_id == 0 && pckt_count == 0){ 
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            
            // È stato ricevuto un primo pacchetto quindi viene
            // interrotto il cronometro e il tempo registrato
            // sarà il tempo andata-ritorno stimato
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);

            puts("Received first packet");
            
            // Aggiorno il numero di porta del server a quello
            // ricevuto nel primo pacchetto 
            addr.sin_port = serv_addr.sin_port = (recv_msg.port_no);            
            
            // Non c'è più bisogno di reinviare la richiesta di connessione
            // Ad ogni timeout corrisponderà un riscontro ai pacchetti
            // ricevuti          
            sa.sa_handler           = &UDP_RFTP_send_ack;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
            
            if(sigaction(SIGALRM, &sa, NULL) < 0) {
                perror("errore in sigaction");
                exit(-1);
            }

            pckt_count = (size_t) strtoul(recv_msg.data, NULL, 10);

            printf("Expected packets\t\t[\t%zu\t]!\n", pckt_count);
            fflush(stdout);

            win = UDP_RFTP_MIN(pckt_count, UDP_RFTP_MAX_RECV_WIN);

            buffs = (char**) malloc(sizeof(char*) * win);
            for(size_t k = 0; k < win; k++)
                if((buffs[k] = (char*) malloc(UDP_RFTP_MAXLINE)) == NULL){
                    perror("errore in malloc");
                    exit(-1);
                }

            pckts = calloc(1, sizeof(char*) * win);
            UDP_RFTP_send_ack(0);
            
            setitimer(ITIMER_REAL, &set_timer, NULL);
            continue;
        }
        
        // È stato ricevuto il primo pacchetto in risposta alla richiesta di
        // connessione ma il numero di messaggi attesi è 1
        // Ricevuto il pacchetto verranno periodicamente inviati
        // dei messaggi per la chiusura della connessione
        if(recv_progressive_id == (size_t) -1 && pckt_count == 0){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            
            // Simile discorso sul tempo andata-ritorno
            // al caso in cui sia il `progressive_id` che
            // `pckt_count` eran nulli
            UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH); 
            
            puts("Received unique packet!");
            
            // Aggiorno il numero di porta del server con quello
            // ricevuto dal primo messaggio 
            addr.sin_port = serv_addr.sin_port = (recv_msg.port_no);
            
            fputs(recv_msg.data, file);
            
            puts("Closing connection");
            
            send_msg.msg_type       = process_type;
            send_msg.progressive_id = (size_t) -1;
            send_msg.data           = "";
            
            UDP_RFTP_bye();
            return;
        }    
    }
   
    if(process_type == UDP_RFTP_LIST)
        puts("LIST PROCESS HAS SUCCESFULLY ENDED IT'S EXECUTION!");
    else
        puts("GET PROCESS HAS SUCCESFULLY ENDED IT'S EXECUTION!");
    
    UDP_RFTP_exit(0); 
    return;
}

// La `put` gestisce richieste di put del client
// Una tipica sequenza di indici progressivi e campo dati
// in una comunicazione put è data dalla seguente lista:
// - client: 0, nome file, numero pacchetti
// - server: -1 (riscontro)
// - client: n, porzione dati
// - server: -1, indici delle porzioni riscontrate (riscontro)
// - client: -1 se tutti i pacchetti son stati riscontrati
// - server: -1 (termine)
void UDP_RFTP_generate_put(char* fname){
    // Inizio del setup per il corretto funzionamento del client
    pid_t pid = fork();

    if(pid == -1){
        perror("errore in fork");
        exit(-1);
    }
    
    if(pid != 0)
        return;

    puts("PUT PROCESS HAS SUCCESFULLY STARTED IT'S EXECUTION!");
     
    chdir("client-side/server_files");

    file = fopen(fname, "r");
    if(file == NULL){
        perror("errore in fopen");
        exit(-1);
    }
   
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("errore in socket");
        exit(-1);
    }
    
    memset((void*) &serv_addr, 0, sizeof(serv_addr));
    memset((void*) &addr, 0, sizeof(addr));
    addr.sin_family         = serv_addr.sin_family        = AF_INET;
    addr.sin_port           = serv_addr.sin_port          = htons(UDP_RFTP_SERV_PT);
    addr.sin_addr.s_addr    = serv_addr.sin_addr.s_addr   = htonl(INADDR_ANY);
    if(inet_pton(AF_INET, UDP_RFTP_SERV_IP, &serv_addr.sin_addr) <= 0) {
		perror("errore in inet_pton");
	    exit(-1);
    }
     
    // Il primo pacchetto che il client invierà al server
    // avrà un indice progressivo nullo e come dati il nome
    // del file da caricare così come il numero di pacchetti
    // il server dovrebbe ricevere 
    send_msg.msg_type           = process_type = UDP_RFTP_PUT;
    send_msg.progressive_id     = 0;
    
    fseek(file, 0L, SEEK_END);
    pckt_count = ftell(file); 
    pckt_count = pckt_count / UDP_RFTP_MAXLINE + (pckt_count % UDP_RFTP_MAXLINE == 0 ? 0 : 1);
    
    printf("Total number of packets is %zu\n", pckt_count);
    fflush(stdout);
    
    win = UDP_RFTP_MIN(pckt_count, UDP_RFTP_BASE_SEND_WIN);
    snprintf(send_msg.data, UDP_RFTP_MAXPCKT, "%s;%zu", fname, pckt_count);
    
    rewind(file);
    // Fine del setup lato client
    
    // Come per la `recv` impostiamo il cronometro
    // per la stima del tempo di andata-ritorno
    UDP_RFTP_start_watch();
    UDP_RFTP_send_pckt();
    
    // Affinché il client non riceverà il primo riscontro dal server,
    // esso reinvierà sempre la richiesta di put
    sa.sa_handler           = &UDP_RFTP_send_pckt; /**/
    sa.sa_flags             = 0;
    sigemptyset(&sa.sa_mask);
    
    if(sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("errore in sigaction");
        exit(-1);
    }
    
    buffs = (char**) malloc(sizeof(char*) * win);
    for(size_t k = 0; k < win; k++){
        if((buffs[k] = (char*) malloc(UDP_RFTP_MAXLINE + 1)) == NULL){
            perror("errore in malloc");
            exit(-1);
        }
    } 

    pckts = calloc(1, sizeof(char*) * win); 
    rel_progressive_id = 0;
    
    // Carichiamo nei buffer le prime porzioni del file
    for(size_t k = 0; k < win; k++){
        if(k + ackd_wins * win >= pckt_count)
            memset(buffs[k], 0, UDP_RFTP_MAXLINE);

        fread((void*) buffs[k], UDP_RFTP_MAXLINE, 1, file);
        pckts[k] = buffs[k];
    }
    
    int K = 0;
    while(1){
        setitimer(ITIMER_REAL, &set_timer, NULL);
        UDP_RFTP_recv_pckt();
       
        if(recv_msg.msg_type == UDP_RFTP_ERR){
            perror("errore nel server");
            exit(-1);
        }
        
        if(recv_msg.msg_type != process_type || addr.sin_port != serv_addr.sin_port){
            continue;
        }

        // L'interruzione di questo cronometro
        // sarà sia per quello avviato per il
        // messaggio di richiesta e per ogni
        // ritrasmissione successiva da parte del
        // client
        // A differenza della `get` dove l'interruzione
        // avviene solo alla ricezione del primo riscontro,
        // siccome la `retrans` avvierà il cronometro
        // solo dopo aver ritrasmesso tutti i pacchetti
        // possiamo semplicemente interrompere il cronometro
        // senza doverci preoccupare di interrompere
        // cronometri mai avviati
        UDP_RFTP_stop_watch(UDP_RFTP_SET_WATCH);
        
        // I pacchetti nella finestra di spedizione sono stati riscontrati
        // dal server, quindi nuovi pacchetti potranno essere trasmessi
        if(ackd_pckts % win == 0 && ackd_pckts >= ackd_wins * win && ackd_pckts != 0){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            printf("Send window succesfully filled!\n");
            fflush(stdout);
            
            retrans_count = 0; 

            memset((void*) pckts, 0, win * sizeof(char*));
            ++ackd_wins;
            for(size_t k = 0; k < win; k++){
                memset(buffs[k], 0, UDP_RFTP_MAXLINE);
                fread((void*) (buffs[k]), UDP_RFTP_MAXLINE, 1, file);
                
                if(ferror(file)){
                    perror("errore in fread");
                    exit(-1);
                }
                
                pckts[k] = buffs[k];
            }
            setitimer(ITIMER_REAL, &set_timer, NULL);
            
            continue;
        }
        
        // A ciascun timeout corrisponderà la ritrasmissione
        // di tutti i pacchetti non ancora riscontrati dal server
        if(K != 1){
            puts("Received first ack");
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            addr.sin_port = serv_addr.sin_port = (recv_msg.port_no);

            sa.sa_handler           = &UDP_RFTP_retrans_pckts;
            sa.sa_flags             = 0;
            sigemptyset(&sa.sa_mask);
            if(sigaction(SIGALRM, &sa, NULL) < 0){
                perror("errore in sigaction");
                exit(-1);
            }
            K = 1;
        }

        char* data_dup  = strdup(recv_msg.data); 
        char* elm       = strtok(data_dup, ";");
        
        // Nessun pacchetto della finestra corrente è
        // stato riscontrato
        if(elm == NULL){
            setitimer(ITIMER_REAL, &cancel_timer, NULL);
            UDP_RFTP_retrans_pckts(1);
            ++retrans_count;
            
            setitimer(ITIMER_REAL, &set_timer, NULL);
            continue;
        }
        
        // Scansioniamo la porzione dati del pacchetto ricevuto
        // Ciascun elemento seguito da `;` corrisponderà all'indice
        // di un pacchetto che è stato ricevuto, quindi riscontrato
        // dal server
        do{
            recv_progressive_id = (size_t)  strtoul(elm, NULL, 10);
            rel_progressive_id  = (ssize_t) (recv_progressive_id - ackd_wins * win - 1);

            if(rel_progressive_id < (ssize_t) win && rel_progressive_id >= 0 && pckts[rel_progressive_id] != NULL){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);
                printf("OK\t%zu\n", recv_progressive_id);
                fflush(stdout);
                 
                pckts[rel_progressive_id] = NULL;
                ++ackd_pckts;
            }
            
            // Tutti i pacchetti del file sono stati ricevuti, quindi riscontrati
            // dal server
            if(ackd_pckts >= pckt_count){
                setitimer(ITIMER_REAL, &cancel_timer, NULL);

                printf("Sent all desired packets!\n");
                fflush(stdout);
                 
                memset((void*) pckts, 0, win * sizeof(char*));
                UDP_RFTP_retrans_pckts(0);
                 
                send_msg.msg_type       = process_type;
                send_msg.progressive_id = (size_t) -1;
                send_msg.data           = "";
                
                free(data_dup);

                puts("PUT PROCESS HAS SUCCESFULLY ENDED IT'S EXECUTION!");
                UDP_RFTP_bye();
                return;
            }
            
            elm = strtok(NULL, ";");
        }while(elm != NULL);
        free(data_dup);
    }
     
    puts("PUT PROCESS HAS SUCCESFULLY ENDED IT'S EXECUTION!");
    UDP_RFTP_exit(0);
    return;
}
