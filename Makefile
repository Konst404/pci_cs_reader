CC=gcc 

OBJECTS = pci_cs_reader.o
CFLAGS  += -std=gnu99
LDFLAGS += -lpci


all: $(OBJECTS)
	$(CC) -o pci_cs_reader $(OBJECTS) $(LDFLAGS)

%.o: %.c 
	$(CC) -c $*.c $(CFLAGS) $(CPPFLAGS)

clean_obj:
	rm -f *.o

clean:
	rm -f pci_cs_reader *.o
