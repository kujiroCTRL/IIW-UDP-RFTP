# Problemi della soluzione proposta
di Lorenzo Casavecchia **< <lnzcsv@gmail.com> >**
## Problemi implementativi
Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
- Gli attori in ricezione non inviano riscontri per l'ultima finestra di ricezione
- La terminazione dei processi figlio non è correttamente gestita
- Processi figlio potrebbero rimanere in stato `defunct` anche a seguito del riscontro con `wait` o `waitpid`
- Gli attori in trasmissione non impostano correttamente i valori dei bufferi dell'ultima finestra di spedizione
- Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file
- Non sono presenti meccanismi di logging per la valutazione dell'efficenza dell'architettura 
