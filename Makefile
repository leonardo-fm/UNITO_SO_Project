CFLAGS = -std=c89 -Wpedantic -Wall -Wextra
SFLAGS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500
LFLAGS = lib/config.c lib/utilities.c lib/msgPortProtocol.c lib/customMacro.h

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

clean:
	$(RM) bin/*