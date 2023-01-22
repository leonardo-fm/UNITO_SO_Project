build: master nave porto

run: master
	./bin/master

master: master.c
	gcc -std=c89 -o bin/master master.c lib/config.c

nave: nave.c
	gcc -std=c89 -o bin/nave nave.c

porto: porto.c
	gcc -std=c89 -o bin/porto porto.c