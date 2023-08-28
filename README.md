# IIW-UDP-RFTP
di Lorenzo Casavecchia **< <lnzcsv@gmail.com> >**

## Presentazione del progetto
Questo progetto consiste nella realizzazione di un sistema per il trasferimento affidabile di file tra un client e un server, utilizzando come protocollo di trasporto UDP e rispettando le linee guida imposte per il progetto previsto dal corso di **[Ingegneria di Internet e Web dell'A.A. 2022-2023](https://didatticaweb.uniroma2.it/informazioni/index/insegnamento/204002-Ingegneria-Di-Internet-E-Web/0)**

**[In questo archivio](https://github.com/kujiroCTRL/IIW-UDP-RFTP)** sono presenti:
- la descrizione della soluzione proposta e della sua implementazione (vedere [`DESCRIPTION.md`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/doc/DESCRIPTION.md) sotto la cartella [`doc`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/doc))
- i codici sorgenti utili per l'esecuzione di entrambi client e server (sotto la cartella [`src`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src))
- una descrizione dei problemi architetturali e implementativi riscontrati fino la versione corrente sistema (vedere [`ISSUES.md`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/doc/ISSUES.md) sotto la cartella [`doc`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/doc))
- la traccia originale della consegna del progetto (vedere [`ASSIGNMENT.md`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/doc/ASSIGNMENT.md) sotto la cartella [`doc`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/doc))

## Installazione
Per la generazione degli eseguibili è necessario:
1. Cambiare cartella di lavoro ad [`src`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src), per esempio eseguendo
```bash
cd ./src
```

2. Eseguire
	- `make client` o `make clientd` per la compilazione dell'applicazione client
	- `make server` o `make serverd` per la compilazione dell'applicazione server
dove `client` e `server` corrispondono a versioni del client e server che non gestiscono dinamicamente i timeout di ricezione / spedizione, mentre `clientd` e `serverd` prevedono l'aggiornamento dei timeout in base all'evoluzione dei ritardi misurati da client e server

È possibile inoltre generare tutti gli eseguibili con `make all`, che risulterà nella presenza dei file `client`, `clientd`, `server` e `serverd` all'interno della cartella [`src`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src)

Infine tutti gli eseguibili possono essere rimossi con `make clean`

3. Avviare gli eseguibili così generati (`./client` o `./clientd` per il client, `./server` o `./serverd` per il server)

All'interno di una stesso spazio di lavoro è possibile generare ed eseguire ambi client e server in quanto il codice ed i file previsti per il funzionamento del client e del server risiedono rispettivamente nelle cartelle [`./src/client-side`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/client-side) e [`./src/server-side`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/server-side)

4. Nel caso sia stata avviata una versione del client (`./client` o `./clientd`) sarà allora possibile inviare una richiesta al server eseguendo
	- `list` per ottenere una lista dei file nella cartella del server
	- `get <nome>` per ottenere dalla cartella del server il file `<nome>`
	- `put <nome>` per caricare nella cartella del server il file `<nome>`

Nello specifico `list` genererà un file `list.txt` nella cartella [`./src/client-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/client-side/server_files)

I file (o la lista dei file) richiesti dal client verranno salvati in [`./src/client-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/client-side/server_files) mentre i file caricati dal client tramite `put` dovranno essere salvati in [`./src/client-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/client-side/server_files)  prima dell'esecuzione del comando

Questo significa che per il client tutti i file, caricati o scaricati, risiederanno nella cartella [`./src/client-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/client-side/server_files)

Un simile ragionamento è valido per il server: qualora venisse eseguita una versione del server
- tutti gli aggiornamenti dei suoi file dovuti da richieste `put` di un client saranno visibili nella cartella [`./src/server-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/server-side/server_files)
- tutte le richieste di tipo `get` effettuate da un client dovranno essere precedute dal caricamento in [`./src/server-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/server-side/server_files) dei file custoditi dal server
- nel caso di richieste `list` in [`./src/server-side/server_files`](https://github.com/kujiroCTRL/IIW-UDP-RFTP/tree/main/src/server-side/server_files) verrà creato un file `list.txt` contenente la lista dei file correntemente posseduti dal server

5. Per terminare l'applicazione premere `<Ctrl>+C`

## Servizi non implementati
Il sistema non prevede la visualizzazione a schermo dei file scambiati: terminata l'applicazione dovrà essere l'utente ad aprire e visionare il contenuto dei file in questione, per esempio eseguendo
```shell
less ./src/client-side/server_files/<nome>
```
oppure
```shell
less ./src/server-side/server_files/<nome>
```
rispettivamente per client e server

Il sistema inoltre non preve la creazione e il caricamento di file da tastiera a tempo di esecuzione: il caricamento dei file in `server_files` va effettuata prima dell'avvio dell'applicazione per esempio copiando un file da un altra cartella
```shell
cp <cartella>/<nome> ./src/client-side/server_files/<nome>
```
per client, oppure
```shell
cp <cartella>/<nome> ./src/server-side/server_files/<nome>
```
per server

Nel caso in cui venga scaricato (lato client e server) un file `<nome>` già presente nella cartella `server_files` il contenuto del nuovo file verrà invece salvato in `<nome>.dup`, quindi in un file con lo stesso nome del file scaricato con un estensione `.dup`; mentre il file `<nome>` verrà lasciato intatto

Questa politica permette di avere lato server e lato client una copia originale del file in questione e una copia grezza che verrà ricaricata ad ogni scaricamento

Il sistema non prevede il rimpiazzo dei file originali con le loro copie grezze tantomeno l'eliminazione automatica di file vuoti (possibile risultato di un'istruzione `list`, `get` o `put` fallita)

Infine il sistema non prevede il trasferimento di file sufficentemente grandi e la taglia massima consentiva è sull'ordine dei mega byte (per maggiori dettagli vedere [`DESCRIPTION.md`](https://github.com/kujiroCTRL/doc/DESCRIPTION.md))