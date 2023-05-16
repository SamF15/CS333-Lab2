CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
		-Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
		-Wdeclaration-after-statement -Wno-return-local-addr \
		-Wuninitialized -Wextra -Wunused

HEADERS = cmd_parse.h
SOURCES = psush.c cmd_parse.c
OBJECTS = psush.o cmd_parse.o
EXEC = psush

all: $(EXEC)

$(EXEC) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJECTS)

$(OBJECTS) : $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -c $(SOURCES)

clean:
	rm -f $(EXEC) $(OBJECTS) *~ \#* *.tar.gz

tar:
	tar cvfa ${LOGNAME}_lab2.tar.gz *.[ch] [mM]akefile