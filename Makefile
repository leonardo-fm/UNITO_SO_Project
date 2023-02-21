CFLAGS = -std=c89 -Wpedantic -Wall -Wextra
CDFLAGS = -DDEBUG

SFLAGS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500
LFLAGS = lib/config.c lib/utilities.c lib/msgPortProtocol.c lib/customMacro.h

# RELEASE section

run: build
	clear 
	./bin/master

build: master nave porto analyzer

master: master.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/master master.c $(LFLAGS) -pthread

nave: nave.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/nave nave.c $(LFLAGS) -lm -pthread

porto: porto.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/porto porto.c $(LFLAGS) -pthread

analyzer: analyzer.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/analyzer analyzer.c $(LFLAGS) -pthread

# DEBUG Section

debug: build-debug
	clear 
	./bin/master

build-debug: master_d nave_d porto_d analyzer_d

master_d: master.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/master master.c $(LFLAGS) -pthread

nave_d: nave.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/nave nave.c $(LFLAGS) -lm -pthread

porto_d: porto.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/porto porto.c $(LFLAGS) -pthread

analyzer_d: analyzer.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/analyzer analyzer.c $(LFLAGS) -pthread

# ADDITIONAL stuff

clean:
	$(RM) bin/*