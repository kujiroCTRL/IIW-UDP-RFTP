# IIW-UDP-RFTP
di Lorenzo Casavecchia **< <lnzcsv@gmail.com> >**

Questo progetto consiste nella realizzazione di un sistema per il trasferimento affidabile di file tra un client e un server, utilizzando come protocollo di trasporto UDP e rispettando le linee guida imposte per il progetto previsto dal corso di **[Ingegneria di Internet e Web dell'A.A. 2022-2023](https://didatticaweb.uniroma2.it/informazioni/index/insegnamento/204002-Ingegneria-Di-Internet-E-Web/0)**

**[In questo archivio](https://github.com/kujiroCTRL/IIW-UDP-RFTP)** sono presenti:
- la descrizione della soluzione proposta e della sua implementazione (vedere `DESCRIPTION.md` sotto la cartella [`doc`]())
- i codici sorgenti utili per l'esecuzione di entrambi client e server (sotto la cartella `src`)
- una descrizione dei problemi architetturali e implementativi presenti nella versione corrente sistema (vedere `ISSUES.md` sotto la cartella `doc`)
- la traccia originale della consegna del progetto (vedere `ASSIGNMENT.MD` sotto la cartella `doc`)

Per la generazione degli eseguibili è necessario:
1. cambiare cartella di lavoro ad `src`, per esempio eseguendo
```bash
cd ./src
```
su un qualsiasi terminale

2. eseguire
	- `make client` o `make clientd` per la compilazione dell'applicazione client
	- `make server` o `make serverd` per la compilazione dell'applicazione server
dove `client` e `server` corrispondono a versioni del client e server che non gestiscono dinamicamente i timeout di ricezione / spedizione, mentre `clientd` e `serverd` prevedono l'aggiornamento dei timeout in base all'evoluzione dei ritardi misurati da client e server

Per generare tutti gli eseguibili è possibile eseguire `make all`, che risulterà nella presenza dei file `client`, `clientd`, `server` e `serverd` all'interno della cartella `src`

3. avviare gli eseguibili così generati (`./client` o `./clientd` per il client, `./server` o `./serverd` per il server)

All'interno di una stesso spazio di lavoro è possibile generare ed eseguire ambi client e server in quanto il codice e i file previsti per il funzionamento del client e del server risiedono rispettivamente nelle cartelle `./src/client-side` e `./src/server-side`