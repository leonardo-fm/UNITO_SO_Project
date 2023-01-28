# UNITO_SO_Project
Programma per corso di Sistemi Operativi 2022/2023

Introduzione

Il progetto è costituito da da 3 principali processi, master, nave e porto.
Nave e porto sono dei fork di master con iniettato codice tramite execve().

Generalità

Tutti e tre i principali processi sono composti di 3 parti principali:
- Inizializzazione dove si va a generare tutti i valori e i PCI necessari al normale funzionamento del processo
- Working dove gira il fulcro della simulazione, un ciclo while che continua a fare una serie predefinita di istruzioni
- Cleaning i processi puliscono i loro rifiuti cercando di non lasciare nulla appeso in memoria

Processo master

Il processo master è il cervello dell'applicazione, ha il comito di creare e scandire i ritmi dell'applicazione e dei suoi componenti.
Inizialmente tramite il file config.c carica in memoria gli shared object e li mette a disposizione di tutti.
Genera anche le merci da cui i porti si "riforniranno" durante la loro inizializzazione.
Nel cuore del master abbiamo un ciclo while che scandice il tempo nell'applicazione e tramite dei signal propaga le informazioni relative allo stato della simulazione a tutto il gruppo di processi.

Processo porto

Il processo porto dopo l'inizializzazione di master genera due memorie condivise dove saranno locate le varie merci di scabio. Un array sarà per le merci rhieste e uno per quelle in loco, tutti e due protetti da dei semafori.
Tramite una generazione casuale di coordinate gli viene data una posizione.
Il dialogo tra porto e nave e fatto tramite un semplice protocollo fatto ad hoc per il progetto. Le varie funzioni si possono trovare nel file msgPortProtocol.c.
Il vero e proprio mezzo di comunicazione invece sono delle code di messaggi (una in lettura e una in scrittura per ogni barca) che permettono uno scabio di informazioni sicuro e senza possibilità di perdita di informazioni.

Processo barca

Il processo barca rappresenta l'utlimo tassello per conpletare la simulazione, anch'essa come il porto dopo l'inizializzazione di master genera una memoria sotto forma di malloc per gestire la sua stiva.
All'inizio della simulazione cerca un porto randm dove attraccare, quando arriva inizia il dialogo tramite protocollo con il porto, da li poi susseguiranno scambi di merce in buono stato (non scaduta).

Config.c
File che permette la gestione della confugrazione presente nel file config.txt

Models.h 
Tutti i modelli necessari all'applicazione

MsgPortProtocol.c 
Gestione del protocollo tra porto e nave

Utilities.c
Funzioni utili riutilizzate ove possibile nell'applicazione

Makefile
File per la comilazione, i comandi sono:
- make build
- make run

killPCS.sh
Da configurare mettendo il nome dell'utente dentro il file, usato per pulire eventuali residui di PCI in memoria.