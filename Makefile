DATE = $(shell date)
HOSTNAME = $(shell hostname)
CFLAGS = -Wall -DBUILDDATE="\"$(DATE)\"" -DBUILDHOST="\"$(HOSTNAME)\"" -g

mdsbrowser: goet_tables.o main.o jsmn.o gmd_json.o
	cc $(CFLAGS) -o mdsbrowser goet_tables.o main.o gmd_json.o jsmn.o

main.o: main.c goet_tables.h
	cc $(CFLAGS) -c main.c

goet_tables.o: goet_tables.c goet_tables.h gmd_json.o jsmn.o
	cc $(CFLAGS) -c goet_tables.c
	
jsmn.o: jsmn.c jsmn.h
	cc $(CFLAGS) -c jsmn.c
	
gmd_json.o: gmd_json.c gmd_json.h
	cc $(CFLAGS) -c gmd_json.c

clean:
	rm *o mdsbrowser			
