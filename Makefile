kilo: kilo.c
	$(CC) kilo.c -o ./build/kilo -Wall -Wextra -pedantic -std=c99

build:
	$(CC) kilo.c -o ./build/kilo -std=c99

run:
	./build/kilo hello.txt


 
