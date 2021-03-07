#include "gol.h"
#include <stdlib.h>
#include <time.h>

grid_t* create_grid(int N){
  grid_t* m = (grid_t*) malloc(sizeof(grid_t));
  m->size = N;
  m->cells = (char**)calloc(sizeof(char*), N);
  for(int i=0; i<N; i++){
      m->cells[i] = (char*)calloc(sizeof(char), N);
  }
  return m;
}

void init_grid(grid_t* g, int birth){
  srand(time(0));
  for(int i=0; i<g->size; i++){
    for(int j=0; j<g->size; j++){
      if(birth > (rand() % 100))
	g->cells[i][j] = 1;
      else
	g->cells[i][j] = 0;
    }
  }
}

int live_neighbours(grid_t *g, int x, int y, int N){
  int dx, dy;
  int count = 0;

  for(int i=-1; i<2; i++){
    for(int j=-1; j<2; j++){
      dx = i + x;
      dy = j + y;
      if(dx == x && dy == y)
	continue;
      if(dx > 0 && dx < N && dy > 0 && dy < N){
	count += g->cells[dy][dx];
      }
    }
  }
  //printf("live_count = %d\n", count);
  return count;
}

int dead_neighbours(grid_t *g, int x, int y, int N){
  int dx, dy;
  int count = 0;

  for(int i=-1; i<2; i++){
    for(int j=-1; j<2; j++){
      dx = i + x;
      dy = j + y;
      if(dx > 0 && dx < N && dy > 0 && dy < N){
	count += (1 - g->cells[dy][dx]);
      }
    }
  }

  return count;
}

void shift_generation(grid_t *g, grid_t *ng, int N){
  int live_count;
  
  for(int i=0; i<N; i++){
    for(int j=0; j<N; j++){

      // If cell is live
      if(g->cells[i][j] == 1){
	live_count = live_neighbours(g, j, i, N);
	if(live_count != 2 && live_count != 3)
	  ng->cells[i][j] = 0;
      }

      // If cell is dead
      if(g->cells[i][j] == 0){
	live_count = live_neighbours(g, j, i, N);
	if(live_count == 3)
	  ng->cells[i][j] = 1;
      }
    }
  }
}
