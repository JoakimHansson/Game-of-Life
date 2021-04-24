CC = gcc
LD = gcc
CFLAGS=-Wall -O3
INCLUDES=-I/opt/X11/include
LDFLAGS=-L/opt/X11/lib -lX11 -lm -pthread -fopt-info-vec-all -ftree-vectorizer-verbose=2
OBJS = main.o gol.o ./graphics/graphics.o
EXECUTABLE = gol
TICK = 100
FILE = input/random_N_01000.gen 

N = 1000    # size of grid
R = 10      # random birts in initial setup / seed
TT = 0	    # time between generations/steps (in seconds)				
TS = 1000   # number of grnerations/steps to simulate
T = 1       # threads to use
G = 1       # graphics 1/0

all: $(EXECUTABLE)

run: $(EXECUTABLE)
	./gol $(R) $(N) $(TT) $(TS) $(G) $(T)

time: $(EXECUTABLE)
	time ./gol input/random_N_00500.gen 500 0 100 0 $(T);
	time ./gol input/random_N_01000.gen 1000 0 100 0 $(T);
	time ./gol input/random_N_01500.gen 1500 0 100 0 $(T);
	time ./gol input/random_N_02000.gen 2000 0 100 0 $(T);
	time ./gol input/random_N_05000.gen 5000 0 100 0 $(T);
	time ./gol input/random_N_10000.gen 10000 0 100 0 $(T);
	time ./gol input/random_N_20000.gen 20000 0 100 0 $(T);

diff: $(EXECUTABLE)
	time ./gol input/random_N_01000.gen 1000 0 100 0 $(T) 1 > test_N_01000_100.txt
	diff ref_N_01000_100.txt test_N_01000_100.txt

$(EXECUTABLE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(EXECUTABLE)

main.o: main.c gol.h ./graphics/graphics.h
	$(CC) $(CFLAGS) $(INCLUDES) -c main.c

gol.o: gol.c
	$(CC) $(CFLAGS) $(INCLUDES) -c gol.c

graphics.o: graphics.c ./graphics/graphics.h
	$(CC) $(CFLAGS) $(INCLUDES) -c ./graphics/graphics.c

clean:
	rm -f ./gol ./a.out *.o ./graphics/*.o *.gcda *.gcno vgcore.* gmon.out result.gen

profile: run_profile
	gprof -b -p ./a.out gmon.out;

run_profile: compile_prof
	./a.out $(FILE) $(N) 0 $(TICK) 0 $(T)

compile_prof:
	gcc -pg graphics/graphics.c gol.c main.c $(INCLUDES) $(LDFLAGS)

memcheck: all
	valgrind --leak-check=full --show-leak-kinds=all -s ./gol $(R) $(N) $(TT) 100 $(G) $(T)
