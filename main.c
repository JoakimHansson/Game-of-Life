#include "gol.h"
#include "./graphics/graphics.h"
#include <unistd.h>
#include <time.h>
const float LiveColor=0, DeadColor=1;

// Default window size. scales if N > 1000.
int windowSize = 1000;
float rectangleSide;

int main(int argc, char** argv){

  int N, graphics, tick_max, nr_threads;
  float tick_time;
  grid_t *grid;
  
  if(argc != 7){
    printf("Usage: ./gol inputfile N tick_time tick_steps graphics nr_threads\nOR\n");
    printf("Usage: ./gol random_biths N tick_time tick_steps graphics nr_threads\n");
    exit(0);
  }
  N = atoi(argv[2]);
  tick_time = atof(argv[3]) * 1000000;
  tick_max = atoi(argv[4]);
  graphics = atoi(argv[5]);
  nr_threads = atoi(argv[6]);
  grid = init_grid(N, nr_threads);

  
  if(atoi(argv[1]) == 0){
    read_grid_from_file(argv[1]);
  }else{
    srand(time(0));
    random_grid(atoi(argv[1]));
  }
   
  if(N % 2)
    exit(-1);

  while(N > windowSize){
    windowSize+=100;
  }

  if(graphics){
    rectangleSide = 1 / (float)N;
    InitializeGraphics(argv[0], windowSize, windowSize);
    SetCAxes(0, 1);
  }
 
  int current_tick = 0;

  while (current_tick++ < tick_max) {

    if(evolve_grid(grid) == 0){
      break;
    }

    if (graphics) {
      ClearScreen();
      for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
          if (grid->cells[y * N + x] == 1) {
            DrawRectangle(x * rectangleSide, y * rectangleSide, 1, 1,
                          rectangleSide, rectangleSide, LiveColor);
          }
        }
      }
      Refresh();
      usleep(tick_time);
    }
  }

  if(graphics){
    FlushDisplay();
    CloseDisplay();
  }

  destroy_grid();

  return 0;
}
