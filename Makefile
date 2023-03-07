CFLAGS = -std=c89 -Wpedantic -Wall -Wextra
CDFLAGS = -DDEBUG

SFLAGS = -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=500
LFLAGS = lib/config.c lib/utilities.c lib/msgPortProtocol.c lib/customMacro.h

LOG_FILE := log/log_$(shell date +%Y%m%d%H%M%S).txt

# RELEASE section

run: build
	clear 
	./bin/master

build: master nave porto analyzer meteo

master: master.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/master master.c $(LFLAGS) -lm -pthread

nave: nave.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/nave nave.c $(LFLAGS) -lm -pthread

porto: porto.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/porto porto.c $(LFLAGS) -pthread

analyzer: analyzer.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/analyzer analyzer.c $(LFLAGS) -pthread

meteo: meteo.c
	gcc $(CFLAGS) $(SFLAGS) -o bin/meteo meteo.c $(LFLAGS) -pthread

# DEBUG Section

debug: build-d
	clear 
	./bin/master

build-d: master_d nave_d porto_d analyzer_d meteo_d

master_d: master.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/master master.c $(LFLAGS) -lm -pthread

nave_d: nave.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/nave nave.c $(LFLAGS) -lm -pthread

porto_d: porto.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/porto porto.c $(LFLAGS) -pthread

analyzer_d: analyzer.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/analyzer analyzer.c $(LFLAGS) -pthread

meteo_d: meteo.c
	gcc $(CFLAGS) $(CDFLAGS) $(SFLAGS) -o bin/meteo meteo.c $(LFLAGS) -pthread

# ADDITIONAL stuff

clean:
	$(RM) bin/*