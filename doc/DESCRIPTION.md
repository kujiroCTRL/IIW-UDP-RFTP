# Descrizione della soluzione proposta
di Lorenzo Casavecchia **< <lnzcsv@gmail.com> >**

## Messaggi
L'interazione tra client e server avviene tramite lo scambio di datagrammi UDP il cui campo dati è una sequenza di caratteri separate da punti e virgola `;`

Le stringhe così delimitate costituiscono i campi del messaggio

Client e server dispongono ciascuno di una struttura
- `send_msg` in cui il mittente caricherà i campi del messaggio da inviare al destinatario
- `recv_msg` in cui verranno inseriti i valori dei campi dell'ultimo messaggio ricevuto dal mittente
- `addr` per l'indirizzo del destinatario del messaggio da inviare o dell'indirizzo del mittente dell'ultimo messaggio ricevuto

La generazione, trasmissione e ricezione dei datagrammi è gestita rispettivamente dalle funzioni
```c
void UDP_RFTP_send_pckt(int signo);
void UDP_RFTP_recv_pckt(void);
```
dove `UDP_RFTP_send_pckt` converte la struttura `send_msg` nella sequenza di caratteri delimitata da `;` e la invia all'indirizzo specificato in `addr`, mentre `UDP_RFTP_recv_pckt` attende la ricezione di un pacchetto e ne salva i campi in `recv_msg`

`send_msg` e `recv_msg` sono entrambe strutture `UDP_RFTP_msg` e dispongono dei campi
- `port_no` per il numero di porta su cui il mittente vorrà essere contattato dal ricevente (virtualmente usato solo dal server all'inizializzazione della connessione)
- `msg_type` per il tipo di comunicazione in atto (`UDP_RFTP_LIST`, `UDP_RFTP_GET`, `UDP_RFTP_PUT` per `list`, `get`, `put` ed `UDP_RFTP_ERR` qualora il file richiesto non esistesse o fosse inaccessibile infine `0` qualora non sia stato ricevuto nessun nuovo messaggio)
- `progressive_id` per identificare diverse porzioni del file in scambio (analogo al numero di sequenza nei protocolli TCP-like)
- `data` per la porzione del file scambiata oppure per specificare altre informazioni riguardo il file trasferito (ad inizializzazione viene usato per dichiarare il nome e la taglia del file)

La verifica dell'indirizzo e il numero di porta del mittente alla ricezione di un pacchetto viene gestita all'esterno di `UDP_RFTP_recv_pckt` confrontandoli con i valori delle strutture `client_addr` e `server_addr` definiti all'inizializzazione della connessione
## Interazione client-server
### Messaggio di connessione
Le operazioni di `list`, `get` e `put` vengono generate dal client che, ricevuto il comando dall'utente, genera ed inoltra al server un messaggio di connessione

Un messaggio di connessione è un messaggio generato solamente dal client con
- `msg_type` pari al tipo di servizio richiesto (`UDP_RFTP_LIST`, `UDP_RFTP_GET`, `UDP_RFTP_PUT` per `list`, `get`, `put`)
- `progressive_id` pari a `0`
- `data` contentente specifiche riguardo il servizio richiesto
	- vuoto per comandi `list`
	- il nome del file desiderato per richieste `get`
	- il nome del file ed il numero di messaggi che il server dovrà ricevere per richieste `put`

Inviato un messaggio di connessione il client rimarrà in attesa di una risposta dal server oppure, allo scadere di un timeout impostato prima dell'invio, reinviare la richiesta al server (attualmente pari al timeout base impostato su tutti i pacchetti)
### Risposta alla connessione
Ricevuta la richiesta di connessione dal client il server dovrà
- creare un processo figlio per la gestione della richiesta
- generare una socket associata a quella richiesta
- comunicare al client del numero di porta della socket generata
Nel comunicare il numero di porta al client il processo figlio del server dovrà inviare sulla socket generata dal padre (con numero di porta di default) un messaggio che nel campo `port_no` abbia il numero di porta della socket del figlio

In questo modo il client riceverà la risposta alla sua richiesta di connessione dalla socket che aveva originariamente contattato e con una direttiva esplicita a chi contattare d'ora in poi per continuare la comunicazione

Gli altri campi della risposta del server dipendono dal tipo di richiesta effettuata e dal valore del campo dati e verranno discussioni nei paragrafi dedicati ai singoli tipi di richiesta

Per evitare che un client invii un messaggio di connessione senza portarla avanti, il processo figlio generato rimarrà in attesa non oltre una quantità di tempo specificata dalla connection timeout `UDP_RFTP_CONN_TOUT` (e comunque più grande del valore di timeout base `UDP_RFTP_BASE_TOUT`)

Nel caso in cui il figlio dovesse ricevere una risposta la comunicazione verrà portata avanti, altrimenti il processo figlio terminerà

È importante osservare che per ogni richiesta di un client il server generi una sola risposta quindi se quest'ultima non dovesse raggiungere il client, sarà quest ultimo a dover generare una nuova richiesta (mentre il server si limiterà a terminare l'esecuzione del processo generato)
### Riscontri selettivi
Come accenato precedentemente il campo `data` di un messaggio può essere usato per comunicare ad uno dei due attori informazioni sul nome di un file e sul numero di messaggi che verranno inviati per trasmettere completamente il file

Siccome sia i datagrammi che il file correntemente trasmessi sono di taglia fissa, il client e il server possono comunicare all'inizio della trasmissione il numero di messaggi che l'altra parte dovrà ricevere affinché la trasmissione si possa considerare conclusa

Il meccanismo con cui l'attore in ricezione comunichi all'attore in spedizione quali porzioni del file siano state correttamente ricevute è simile al selective repeat: l'attore in ricezione instaura una finestra dei pacchetti ordinati non ancora ricevuti e un buffer della stessa dimensione in cui verranno, di ricezione in ricezione, caricate le porzioni ricevute

A tal scopo a ciascuna porzione del file viene associato un `progressive_id` quindi un identificatore progressivo della porzione correntemente trasmessa

Il `progressive_id` è, rispetto all'insieme delle porzioni del file, un numero assoluto  e non è relativo alla corrente finestra del mittente (il che significa che se il file in questione potesse essere trasmesso in $N$ messaggi, il `progressive_id` potrà variare tra $1$ ed $N$ anche se la finestra di spedizione preveda l'invio simultaneo di $n\lt N$ messaggi)

Un attore in ricezione che riceva una porzione di file dovrà
- verificare che il `progressive_id` non appartenga ad una porzione precedentemente riscontrata (quindi sia maggiore al `progressive_id` della prima porzione di file nella corrente finestra di ricezione e la relativa porzione non sia stata marcata come ricevuta)
- verificare che il `progressive_id` non appartenga ad una porzione della finestra di ricezione successiva a quella attuale (`progressive_id` dovrà essere minore o uguale al `progressive_id` dell'ultima porzione della finestra)
- inviare un riscontro positivo (ACK) delle porzioni correttamente ricevute dell'attuale finestra

Il riscontro verrà inviato
- allo scadere del timeout di ricezione
- nel caso di `progressive_id` di porzioni di finestre precedenti 
- qualora vengano ricevute tutte le porzioni previste dalla finestra

D'altro canto la finestra di ricezione potrà essere traslata e ammettere nuove porzioni solo al riempimento di quella precedente

In tal senso possiamo dire che il meccanismo di riscontro delle porzioni dei file è
- di tipo stop and wait nel contesto della finestra di ricezione
- di tipo selettivo e cumulativo nel contesto delle porzioni della finestra
### Ritrasmissione
### Chiusura della connessione

## L'header `defs.h`
Nei file `defs.h` vengono definiti i parametri di esecuzione, le variabili di controllo e le funzioni comuni alle funzioni del client e server della gestione del trasferimento dei file

L'insieme dei parametri di esecuzione è costituito da tutte le macro definite nella prima parte di `defs.h` e comprendono
- l'indirizzo IP e numero di porta di default del server
    ```c
    #define UDP_RFTP_SERV_IP            ("127.0.0.1")
    #define UDP_RFTP_SERV_PT            (5193)
    ```
- i valori base, minimo e massimo per la taglia della finestra di spedizione 
    ```c
    #define UDP_RFTP_BASE_SEND_WIN      (4)
    #define UDP_RFTP_MIN_SEND_WIN       (2)
    #define UDP_RFTP_MAX_SEND_WIN       (256)
    ```
- la taglia della finestra di ricezione
    ```c
    #define UDP_RFTP_BASE_RECV_WIN      (5)
    ```
- i valori base, minimo, massimo del timeout così come il connection timeout (in microsecondi)
    ```c
    #define UDP_RFTP_BASE_TOUT          (500) 
    #define UDP_RFTP_MIN_TOUT           (10)
    #define UDP_RFTP_CONN_TOUT          (3000000)
    ```
- la politica di aggiornamento del timeout
    ```c
    #define UDP_RFTP_MULT_TOUT(t)       (9 * (t) / 8)
    #define UDP_RFTP_AVRG_TOUT(t, T)    (1 * (t) / 2 + 1 * (T) / 2)
    #define UDP_RFTP_UPDT_TOUT          ((t) < (T) ? UDP_RFTP_MULT_TOUT(t) : UDP_RFTP_AVRG_TOUT(t, T))
    ```

Le variabili di controllo vengono modificate a tempo d'esecuzione e comprendono
- informazioni sullo stato del trasferimento del file
	- `pckt_count`: il numero totale di pacchetti da trasferire
	- `akcd_pckts` ed `ackd_wins`: il numero di pacchetti e finestre riscontrati
	- `win`: l'attuale taglia della finestra di spedizione / ricezione
