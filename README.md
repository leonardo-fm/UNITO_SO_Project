# UNITO_SO_Project
Programma per corso di Sistemi Operativi 2022/2023

Introduzione

Il progetto è costituito da da 5 principali processi, master, nave, porto, meteo e l'analyzer .
Sono tutti dei fork di master con iniettato codice tramite execve().

## Generalità

Tutti e cinque i principali processi sono composti di 3 parti principali:
- **Inizializzazione**, dove si va a generare tutti i valori e i PCI necessari al normale funzionamento del processo
- **Work**, dove gira il fulcro della simulazione, un ciclo while che continua a fare una serie predefinita di istruzioni
- **Cleaning**, dove processi puliscono la loro memoria cercando di non lasciare nulla appeso

E' presente un **Makefile**, dove le principali operazioni sono:
- **make build**, fa la build della soluzione
- **make run**, fa la build della soluzione e la lancia
- **make build-d**, fa la build in DEBUG della suluzione
- **make debug**, fa la build in DEBUG della soluzione e la lancia

__Attenzione__, quando si scarica il progetto bisogna creare due cartelle nella main directory, una è **bin** e l'altra è **out**, rispettivamente conterranno i file binari e il dump della simulazione

### Processo master

Il processo master è il cervello dell'applicazione, ha il compito di creare e scandire i ritmi dell'applicazione e dei suoi componenti.
Inizialmente tramite il file config.c carica in memoria la configurazione, che essendo su un file esterno può essere modificata senza dover fare la build dell'applicazione. Va anche a creare tutti gli shared object e li mette a disposizione di tutti i processi che li necessitano.
Genera anche le merci da cui i porti si "riforniranno" durante la loro inizializzazione.
Nel cuore del master abbiamo un ciclo while che scandice il tempo nell'applicazione e tramite dei signal propaga le informazioni relative allo stato della simulazione a tutto il gruppo di processi.

### Processo porto

Il processo porto dopo l'inizializzazione di master genera due memorie condivise dove saranno locate le varie merci di scabio. Un array sarà per le merci rhieste e uno per quelle in loco, tutti e due protetti da semafori.
Tramite una generazione casuale di coordinate gli viene data una posizione iniziale, escludendo i primi 4 che saranno posizionati agli estremi della mappa.
Il dialogo tra porto e nave e fatto tramite un semplice protocollo fatto ad hoc per il progetto. Le varie funzioni si possono trovare nel file msgPortProtocol.h.
Il vero e proprio mezzo di comunicazione invece sono delle code di messaggi (una in lettura e una in scrittura per ogni barca) che permettono uno scabio di informazioni sicuro e senza possibilità di perdita di informazioni.

### Processo barca

Il processo barca anch'esso come il porto dopo l'inizializzazione di master genera una memoria sotto forma di malloc per gestire la sua stiva.
All'inizio della simulazione cerca un porto randm dove attraccare, quando arriva inizia il dialogo tramite protocollo con il porto, nel caso il porto non accettasse la barca cerca un'altro porto dove sostare, mentre se viene accettata si susseguiranno scambi di merce in buono stato (non scaduta).

### Processo analyzer

Il processo analyzer è colui che si occupa di raccogliere, analizzare e generare il dump dei dati giornalieri e finali della simulazione

### Processo meteo

Il processo meteo (weather) serve a generare in maniera randomica degli eventi meteorologici che vanno a interferire con il lavoro di navi e porti.
Esso comunica con le navi e porti tramite segnali per gestire i vari eventi.

**Config.c**
File che permette la gestione della confugrazione presente nel file config.txt

**Models.h** 
Tutti i modelli necessari all'applicazione

**MsgPortProtocol.c** 
Gestione del protocollo tra porto e nave

**Utilities.c**
Funzioni utili riutilizzate ove possibile nell'applicazione

**CustomMacro.h**
Funzioni per il debug e per gestione degli errori

**killPCS.sh**
Da configurare mettendo il nome dell'utente dentro il file, usato per pulire eventuali residui di PCI in memoria.
