# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
    Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
        - Il server non è in grado di gestire il client successivo al primo [DA INVESTIGARE]
        - La get smarrisce sempre un pacchetto, probabilmente l'ultimo [DA INVESTIGARE]
        - Qualora il client non dovesse ricevere una risposta dal server riguardo la taglia del file richiesto in `get`, esso non tenterà nuovamente ad istanziare la connessione con il server [ANCORA NON IMPLEMENTATO]
        - Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
        - La dimensione della finestra di spedizione non viene aggiornata né per matchare quella in ricezione dell'altro attore, tantomeno in caso di congestione nella rete (o perdita di pacchetti simulata) [ANCORA NON IMPLEMENTATO]
        - `put` virtualmente non operativa [ANCORA NON IMPLEMENTATO]
