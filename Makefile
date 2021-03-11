CC = gcc
LD = gcc
CFLAGS=-Wall -g #-Ofast
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

profile: run_profile
	gprof -b ./a.out gmon.out

run_profile: compile_prof
	./a.out input/random_N_02000.gen 2000 0 100 0 1

compile_prof:
	gcc -pg graphics/graphics.c gol.c main.c $(INCLUDES) $(LDFLAGS)
