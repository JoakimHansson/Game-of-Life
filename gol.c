#include "gol.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int *changed_x = NULL;
int *changed_y = NULL;
int *changed_x_next = NULL;
int *changed_y_next = NULL;

int global_count = 0;
int change_count = 0;
int generation_count = 0;
int **gt = NULL; // generation_tracker

grid_t *ng; // next generation

grid_t* create_grid(int N){
  printf("N = %d\n", N);
  grid_t* g = (grid_t*) malloc(sizeof(grid_t));
  ng = (grid_t*) malloc(sizeof(grid_t));
  g->size = N;
  g->start_index = 1;
  g->end_index = N+1;
  ng->start_index = 1;
  ng->end_index = N+1;
  ng->size = N;
  g->cells = (char**)calloc(sizeof(char*), N+2);
  ng->cells = (char**)calloc(sizeof(char*), N+2);
  for(int i=0; i<N+2; i++){
      g->cells[i] = (char*)calloc(sizeof(char), N+2);
      ng->cells[i] = (char*)calloc(sizeof(char), N+2);
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
    gt = calloc(sizeof(int*), N+3);
    for(int i=0; i<N+3; i++){
      gt[i] = (int*) calloc(sizeof(int), N+3);
    }
  }

  return g;
}

void delete_grid(grid_t *g){
  for(int i=0; i<g->size+2; i++){
    free(g->cells[i]);
    free(ng->cells[i]);
    free(gt[i]);
  }
  free(gt[g->size+2]);
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
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
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

static inline int live_neighbours(grid_t *g, int x, int y){
  int count = 0;
  count += g->cells[y-1][x-1];
  count += g->cells[y-1][x];
  count += g->cells[y-1][x+1];
  count += g->cells[y][x-1];
  count += g->cells[y][x+1];
  count += g->cells[y+1][x-1];
  count += g->cells[y+1][x];
  count += g->cells[y+1][x+1];
  return count;
}

int dead_neighbours(grid_t *g, int x, int y){
  return 8 - live_neighbours(g, x, y);
}

int shift_generation_first(grid_t *g){
  int live_count;
  int change_count = 0;
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){

      // If cell is live
      if(g->cells[i][j] == 1){
	live_count = live_neighbours(g, j, i);
	if(live_count < 2 || live_count > 3){
	  ng->cells[i][j] = 0;
	  changed_x[change_count] = j;
	  changed_y[change_count++] = i;
	}
	 
      }

      // If cell is dead
      else if(g->cells[i][j] == 0){
	live_count = live_neighbours(g, j, i);
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

static inline void shift_cell(grid_t *g, int x, int y){
  int live_count,N;
  N = ng->size;
  gt[y+1][x+1] = generation_count;
  if(x < g->start_index || x >= g->end_index || y < g->start_index || y >= g->end_index)
     return; 

  // If cell is live
  if (g->cells[y][x] == 1) {
    live_count = live_neighbours(g, x, y);
    if (live_count < 2 || live_count > 3) {
      ng->cells[y][x] = 0;
      changed_x_next[change_count] = x;
      changed_y_next[change_count++] = y;
    }
    
  }
  
  // If cell is dead
  else if (g->cells[y][x] == 0) {
    live_count = live_neighbours(g, x, y);
    if (live_count == 3) {
      ng->cells[y][x] = 1;
      changed_x_next[change_count] = x;
      changed_y_next[change_count++] = y;
    }
  } 
  //printf("%d\n", change_count);
}

int shift_generation(grid_t *g){

  if(global_count == 0)
    return shift_generation_first(g);
  int x, y;
  for (int i = 0; i < global_count; i++){
    x = changed_x[i];
    y = changed_y[i];
    if(gt[y+1-1][x+1-1] != generation_count) 
      shift_cell(g,changed_x[i]-1,changed_y[i]-1);
    if(gt[y+1][x+1-1] != generation_count) 
      shift_cell(g,changed_x[i]-1,changed_y[i]);
    if(gt[y+1+1][x+1-1] != generation_count) 
      shift_cell(g,changed_x[i]-1,changed_y[i]+1);
    
    if(gt[y+1-1][x+1] != generation_count) 
      shift_cell(g,changed_x[i],changed_y[i]-1);
    if(gt[y+1][x+1] != generation_count) 
      shift_cell(g,changed_x[i],changed_y[i]);
    if(gt[y+1+1][x+1] != generation_count) 
      shift_cell(g,changed_x[i],changed_y[i]+1);

    if(gt[y+1-1][x+1+1] != generation_count) 
      shift_cell(g,changed_x[i]+1,changed_y[i]-1);
    if(gt[y+1][x+1+1] != generation_count) 
      shift_cell(g,changed_x[i]+1,changed_y[i]);
    if(gt[y+1+1][x+1+1] != generation_count) 
      shift_cell(g,changed_x[i]+1,changed_y[i]+1);
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
  change_count = 0;
  generation_count++;
  return global_count;
}

void print_grid(grid_t *g){
  printf("\n");
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
      printf("%d ", g->cells[i][j]);
    }
    printf("\n");
  }
}
