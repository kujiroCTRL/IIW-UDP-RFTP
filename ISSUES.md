# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
- La politica di aggiornamento della finestra di spedizione non è adeguata (non è stabile) [DA INVESTIGARE]
- Il codice del mittente non può gestire finestre di spedizione variabili in taglia in quanto il controllo degli indici progressivi si basava su finestre di taglia fissa. Pur modificandolo si osservano problemi di ricalcolo della finestra [FORSE RISOLTO] 
- Il client non termina automaticamente dopo che il server è stato ucciso preentivamente [DA INVESTIGARE]
- `list` non funzionante (`get` con pacchetti sufficentemente piccoli) [DA INVESTIGARE]
- Il server non è in grado di gestire client successivi al primo [DA INVESTIGARE]
- La `get` smarrisce occasionalmente porzioni di file [FORSE RISOLTO] Probabilmente perde l'ultimo pacchetto perché esso non è grande quanto `UDP_RFTP_MAXLINE`
- Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
- La dimensione della finestra di spedizione non viene aggiornata né per matchare quella in ricezione dell'altro attore, tantomeno in caso di congestione nella rete (o perdita di pacchetti simulata) [FORSE RISOLTO]
- `put` virtualmente non operativa [ANCORA NON IMPLEMENTATO]
