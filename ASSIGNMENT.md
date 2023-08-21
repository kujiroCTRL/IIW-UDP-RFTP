# Reti di Calcolatori ed Ingegneria del Web - A.A. 2022/23
## Progetto B: Trasferimento file su UDP

Lo scopo del progetto è quello di progettare ed implementare in linguaggio `C` usando l’API del socket di Berkeley  un’applicazione  client-server  per  il trasferimento  di  file  che  impieghi il  servizio  di  rete  senza connessione (socket tipo `SOCK_DGRAM`, ovvero UDP come protocollo di strato di trasporto).

Il software deve permettere:
    - Connessione client-server senza autenticazione;
    - La visualizzazione sul client dei file disponibili sul server (comando `list`);
    - Il download di un file dal server (comando `get`);
    - L'upload di un file sul server (comando `put`);
    - Il trasferimento file in modo affidabile.

La  comunicazione  tra  client  e  server  deve  avvenire  tramite  un  opportuno  protocollo.

Il  protocollo  di comunicazione deve prevedere lo scambio di due tipi di messaggi:
    - messaggi di comando: vengono inviati dal client al server per richiedere l’esecuzione delle diverse operazioni;
    - messaggi di risposta: vengono inviati dal server al client in risposta ad un comando con l’esito dell’operazione. 

### Funzionalità del server
Il server, di tipo concorrente, deve fornire le seguenti funzionalità:
    - L’invio del messaggio di risposta al comando `list` al client richiedente;
    - Il messaggio di risposta contiene la filelist, ovvero la lista dei nomi dei file disponibili per la condivisione;
    - L’invio  del  messaggio  di  risposta  al  comando `get` contenente  il  file  richiesto,  se  presente,  od  un opportuno messaggio di errore;
    - La ricezione di un messaggio `put` contenente il file da caricare sul server e l’invio di un messaggio di risposta con l’esito dell’operazione.

### Funzionalità del client
Il client, di tipo concorrente, deve fornire le seguenti funzionalità:
    - L'invio del messaggio `list` per richiedere la lista dei nomi dei file disponibili;
    - L'invio del messaggio `get` per ottenere un file;
    - La ricezione di un file richiesta tramite il messaggio di `get` o la gestione dell'eventuale errore;
    - L'invio del messaggio `put` per effettuare l'upload di un file sul server e la ricezione del messaggio di risposta con l'esito dell'operazione.

### Trasmissione affidabile
Lo scambio di messaggi avviene usando un servizio di comunicazione non affidabile.

Al fine di garantire la corretta spedizione/ricezione dei messaggi e dei file sia i client che il server implementano a livello applicativo un protocollo TCP-like con controllo di congestione.

I candidati possono usare un qualsiasi algoritmo (purché ben motivato) che adotti una finestra di spedizione variabile che aumenta in caso di assenza di perdite e diminuisce in caso di perdita di pacchetti.

Per simulare la perdita dei messaggi in rete (evento alquanto improbabile in una rete locale per non parlare di quando client e server sono eseguiti sullo stesso host), si assume che ogni messaggio sia scartato dal mittente con probabilità $p$.

La probabilità di perdita dei messaggi $p$, e la durata del timeout $T$, sono tre costanti configurabili ed uguali per tutti i processi.

Oltre all’uso di un timeout fisso, deve essere possibile scegliere l’uso di un valore per il timeout adattativo calcolato dinamicamente in base alla evoluzione dei ritardi di rete osservati.

I client ed il server devono essere eseguiti nello spazio utente senza richiedere privilegi di root.

Il server deve essere in ascolto su una porta di default (configurabile).

### Scelta e consegna del progetto
Il progetto può essere realizzato da un gruppo composto al massimo da 3 studenti.

Per poter sostenere l'esame nell'A.A. 2022/23, è necessario prenotarsi per il progetto, comunicando al docente i nominativi ed indirizzi di e-mail dei componenti del gruppo.

Per ogni comunicazione via e-mail è necessario specificare `[IW23]` nel subject della mail.

Eventuali modifiche relative al gruppo devono essere tempestivamente comunicate e concordate con il docente.

La consegna del progetto deve avvenire almeno dieci giorni prima della data stabilita per la discussione del progetto.

La consegna del progetto consiste in:
- un file `.zip` contenente tutti i sorgenti (opportunamente commentati) necessair per il funzionamento e la copia elettronica della relazione (in formato `.pdf`);
- la copia cartacea della relazione.

La relazione contiene:
- la descrizione dettagliata dell'architettura del sistema e delle scelte progettuali effettuate;
- la descrizione dell'implementazione;
la descrizione delle eventuali limitazizoni riscontrate;
- l'indicazione della piattaforma software usata per lo sviluppo ed il testing del sistema;
- alcuni esempi di funzionamento;
- valutazione delle prestazioni del protocollo al variare delle probabilità di perdita dei messaggi $p$, e della durata del timeout $T$ (incluso il caso di $T$ adattivo)
- un manuale per l'installazione, la configurazione e l'esecuzione del sistema.

### Valutazione del progetto
I principali criteri di valutazione del progetto saranno:
- rispondenza ai requisiti;
- originalità;
- efficenza;
- leggibilità del codice;
- modularità del codice;
- organizzazione, chiarezza e completezza della relazione;
- semplicità di installazione e configurazionedel software realizzato in ambiente Linux.
