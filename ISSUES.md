# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
    Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
        - `list` non funzionante (`get` con pacchetti sufficentemente piccoli) [DA INVESTIGARE]
        - Il server non è in grado di gestire client successivi al primo [DA INVESTIGARE]
        - La `get` smarrisce occasionalmente porzioni di file [DA INVESTIGARE]
        - Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
        - La dimensione della finestra di spedizione non viene aggiornata né per matchare quella in ricezione dell'altro attore, tantomeno in caso di congestione nella rete (o perdita di pacchetti simulata) [ANCORA NON IMPLEMENTATO]
        - `put` virtualmente non operativa [ANCORA NON IMPLEMENTATO]
