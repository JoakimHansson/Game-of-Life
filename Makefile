CC = gcc
LD = gcc
CFLAGS=-Wall -g 
INCLUDES=-I/opt/X11/include
LDFLAGS=-L/opt/X11/lib -lX11 -lm -pthread
OBJS = main.o gol.o ./graphics/graphics.o
EXECUTABLE = gol

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(EXECUTABLE)

main.o: main.c gol.h ./graphics/graphics.h
	$(CC) $(CFLAGS) $(INCLUDES) -c main.c

gol.o: gol.c
	$(CC) $(CFLAGS) $(INCLUDES) -c gol.c

graphics.o: graphics.c ./graphics/graphics.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ./graphics/graphics.c

clean:
	rm -f ./gol *.o ./graphics/*.o
