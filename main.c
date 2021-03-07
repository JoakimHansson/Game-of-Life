#include "gol.h"
#include "./graphics/graphics.h"
#include <unistd.h>
#include <time.h>
const float LiveColor=0, DeadColor=1;

// Default window size. scales if N > 1000.
int windowSize = 1000;
float rectangleSide;// = 0.01;


int main(int argc, char** argv){

  int N, birth, graphics, tick_max;
  float tick_time;
  clock_t start, end;
  double time_used;

  if(argc != 6){
    printf("Usage: ./gol N birth tick_time tick_steps graphics\n");
    exit(0);
  }
  
  N = atoi(argv[1]);
  birth = atoi(argv[2]);
  tick_time = atof(argv[3]) * 1000000;
  tick_max = atoi(argv[4]);
  graphics = atoi(argv[5]);
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
  
  grid_t *grid = create_grid(N);
  grid_t *grid_next = create_grid(N);
  init_grid(grid, birth);

  int current_tick = 0;

  while (current_tick++ < tick_max) {

    start = clock();
    shift_generation(grid, grid_next, N);
    end = clock();
    time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time used in shift_generation() = %f\n", time_used);

    if (graphics) {
      ClearScreen();
      start = clock();
      for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
          if (grid_next->cells[y][x] == 1) {
            DrawRectangle(x * rectangleSide, y * rectangleSide, 1, 1,
                          rectangleSide, rectangleSide, LiveColor);
          }
        }
      }
      Refresh();
      end = clock();
      time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
      printf("Time used redering generation: %f\n", time_used);
      usleep(tick_time);
    }

    grid = grid_next;
  }
  
  if(graphics){
    FlushDisplay();
    CloseDisplay();
  }
  return 0;
}
