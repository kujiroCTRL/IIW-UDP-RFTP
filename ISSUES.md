# Problemi della soluzione proposta
## Problemi architetturali
    
## Problemi implementativi
Ad aggiornamento odierno l'applicazione presenta i seguenti problemati implementativi:
- Il server non è in grado di gestire client successivi al primo [DA INVESTIGARE] Potrebbe in realtà essere che la `list` tende a fallire un paio di volte se il loss rate venisse applicato in ogni scenario possibile
- Può capitare che la `get` o la `put` (in entrambi i casi l'attore che svolge il ruolo di mittente) porti all'invio di un pacchetto dell'ultima finestra di spedizione con un indice progressivo più alto di quello effettivo [DA INVESTIGARE]
- Non sono presenti meccanismi di file locking per attori che agiscono sullo stesso file [ANCORA NON IMPLEMENTATO]
