CFLAGS = -std=c89 -Wpedantic

run: build
	clear 
	./bin/master

build: master nave porto

master: master.c
	gcc -Wall -Wextra -g -o bin/master master.c lib/config.c lib/utilities.c 

nave: nave.c
	gcc -Wall -Wextra -g -o bin/nave nave.c lib/config.c lib/utilities.c lib/msgPortProtocol.c -lm

porto: porto.c
	gcc -Wall -Wextra -g -o bin/porto porto.c lib/config.c lib/utilities.c lib/msgPortProtocol.c

clean:
	$(RM) bin/*