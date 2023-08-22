# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
    Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
        - Il comando `get` non permette la trasmissione (o il salvataggio) del file richiesto nella sua interezza. Nello specifico sembrerebbe che se l'ultimo pacchetto non coincida con l'ultimo pacchetto della finestra di ricezione allora esso non verrà caricato nel file [DA INVESTIGARE]
        - Qualora il client non dovesse ricevere una risposta dal server riguardo la taglia del file richiesto in `get`, esso non tenterà nuovamente ad istanziare la connessione con il server [ANCORA NON IMPLEMENTATO]
        - Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
        - La dimensione della finestra di spedizione non viene aggiornata né per matchare quella in ricezione dell'altro attore, tantomeno in caso di congestione nella rete (o perdita di pacchetti simulata) [ANCORA NON IMPLEMENTATO]
        - `put` virtualmente non operativa [ANCORA NON IMPLEMENTATO]
