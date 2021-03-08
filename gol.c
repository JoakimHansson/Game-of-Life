#include "gol.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int *changed_x = NULL;
int *changed_y = NULL;
int *changed_x_next = NULL;
int *changed_y_next = NULL;



int global_count = 0;
int generation_count = 0;
int **gt = NULL; // generation_tracker

grid_t *ng; // next generation


grid_t* create_grid(int N){
  grid_t* g = (grid_t*) malloc(sizeof(grid_t));
  ng = (grid_t*) malloc(sizeof(grid_t));
  g->size = N;
  ng->size = N;
  g->cells = (char**)calloc(sizeof(char*), N);
  ng->cells = (char**)calloc(sizeof(char*), N);
  for(int i=0; i<N; i++){
      g->cells[i] = (char*)calloc(sizeof(char), N);
      ng->cells[i] = (char*)calloc(sizeof(char), N);
  }

  if(changed_x == NULL){
    changed_x = calloc(sizeof(int), N*N);
    changed_x_next = calloc(sizeof(int), N*N);
  }
  if(changed_y == NULL){
    changed_y = calloc(sizeof(int), N*N);
    changed_y_next = calloc(sizeof(int), N*N);
  }
  if(gt == NULL){
    gt = calloc(sizeof(int*), N+2);
    for(int i=0; i<N+2; i++){
      gt[i] = (int*) calloc(sizeof(int), N+2);
    }
  }
  
  return g;
}

void delete_grid(grid_t *g, int N){
  for(int i=0; i<N; i++){
    free(g->cells[i]);
    free(ng->cells[i]);
    free(gt[i]);
  }
  free(gt[N]);
  free(gt[N+1]);
  free(g->cells);
  free(ng->cells);
  free(g);
  free(ng);
  free(gt);
  free(changed_x);
  free(changed_y);
  free(changed_x_next);
  free(changed_y_next);
}

void init_grid(grid_t* g, int birth){
  srand(time(0));
  for(int i=0; i<g->size; i++){
    for(int j=0; j<g->size; j++){
      if(birth > (rand() % 100)){
	g->cells[i][j] = 1;
	ng->cells[i][j] = 1;
      }
      else{
	g->cells[i][j] = 0;
	ng->cells[i][j] = 0;
      }
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

int shift_generation_first(grid_t *g, int N){
  int live_count;
  int change_count = 0;
  for(int i=0; i<N; i++){
    for(int j=0; j<N; j++){

      // If cell is live
      if(g->cells[i][j] == 1){
	live_count = live_neighbours(g, j, i, N);
	if(live_count != 2 && live_count != 3){
	  ng->cells[i][j] = 0;
	  changed_x[change_count] = j;
	  changed_y[change_count++] = i;
	}
	 
      }

      // If cell is dead
      else if(g->cells[i][j] == 0){
	live_count = live_neighbours(g, j, i, N);
	if(live_count == 3){
	  ng->cells[i][j] = 1;
	  changed_x[change_count] = j;
	  changed_y[change_count++] = i;
	}
      }
    }
  }

  int c = 0;
  int x,y;
  while(c < change_count){
    x = changed_x[c];
    y = changed_y[c];
    g->cells[y][x] = ng->cells[y][x];
    c++;
  }
  global_count = change_count;
  generation_count++;
  return change_count;
}

int shift_generation(grid_t *g, int N){

  if(global_count == 0)
    return shift_generation_first(g, N);
  
  int live_count, x, y;
  int change_count = 0;
  for (int i = 0; i < global_count; i++){
      for (int k = -1; k < 2; k++) {
        for (int m = -1; m < 2; m++) {
          x = m + changed_x[i];
          y = k + changed_y[i];
	  if(gt[y+1][x+1] == generation_count || x < 0 || x >= N || y < 0 || y >= N)
	    continue;
	  //checked[(x+1)*(y+1)] = 1;
	  gt[y+1][x+1] = generation_count;
          // If cell is live
          if (g->cells[y][x] == 1) {
            live_count = live_neighbours(g, x, y, N);
            if (live_count != 2 && live_count != 3) {
              ng->cells[y][x] = 0;
              changed_x_next[change_count] = x;
              changed_y_next[change_count++] = y;
            }

          }

          // If cell is dead
          else if (g->cells[y][x] == 0) {
            live_count = live_neighbours(g, x, y, N);
            if (live_count == 3) {
              ng->cells[y][x] = 1;
              changed_x_next[change_count] = x;
              changed_y_next[change_count++] = y;
            }
          }
        }
    }
  }
  int c = 0;
  while (c < change_count) {
    x = changed_x_next[c];
    y = changed_y_next[c];
    changed_x[c] = x;
    changed_y[c] = y;
    g->cells[y][x] = ng->cells[y][x];
    c++;
  }
  global_count = change_count;
  generation_count++;
  return change_count;
}

void print_grid(grid_t *g){
  printf("\n");
  for(int i=0; i<g->size; i++){
    for(int j=0; j<g->size; j++){
      printf("%d ", g->cells[i][j]);
    }
    printf("\n");
  }
}
