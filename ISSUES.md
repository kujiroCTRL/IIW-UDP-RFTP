# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
- Il server non è in grado di gestire client successivi al primo [DA INVESTIGARE] Potrebbe in realtà essere che la `list` tende a fallire un paio di volte se il loss rate venisse applicato in ogni scenario possibile
- La `put` smarrisce porzioni di file ma solo quando il client non aggiorna dinamicamente il suo timeout, mentre il server sì [DA INVESTIGARE] 
- La `get` tende a terminare sempre con la terminazione per inattività da parte del client o del server [ANCORA NON IMPLEMENTATO] Un modo semplice di risolverlo potrebbe essere introducendo un nuovo codice per l'uscita di un attore dalla comunicazione
- Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
