#include "gol.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef struct data{
  grid_t *g;
  int start;
  int end;

  int *changed_x;
  int *changed_y;
  int s_count;
  int count;

  int done;
  int id;
}data_t;


int *changed_x = NULL;
int *changed_y = NULL;
int *changed_x_next = NULL;
int *changed_y_next = NULL;

int global_count = 0;
int change_count = 0;
int generation_count = 0;
int **gt = NULL;

int offset;
int next_offset;

grid_t *ng;

data_t *d;

// Initilize everything needed to run the simulation
grid_t* create_grid(int N, int nr_threads){
  printf("N = %d\n", N);
  grid_t* g = (grid_t*) malloc(sizeof(grid_t));
  ng = (grid_t*) malloc(sizeof(grid_t));
  g->size = N;
  g->nr_threads = nr_threads;      
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
    changed_x = calloc(sizeof(int), N*N*2);
    changed_x_next = calloc(sizeof(int), N*N);
  }
  if(changed_y == NULL){
    changed_y = calloc(sizeof(int), N*N*2);
    changed_y_next = calloc(sizeof(int), N*N);
  }
  if(gt == NULL){
    gt = calloc(sizeof(int*), N+3);
    for(int i=0; i<N+3; i++){
      gt[i] = (int*) calloc(sizeof(int), N+3);
    }
  }

  d = calloc(sizeof(data_t), nr_threads);
  for(int i=0; i<nr_threads; i++){
    d[i].changed_x = calloc(sizeof(int), (N)*(N));
    d[i].changed_y = calloc(sizeof(int), (N)*(N));
    d[i].id = i;
    d[i].g = g;
  }
  return g;
}

// Deallocates all memory used in the simulation
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
  free(d);
  free(changed_x);
  free(changed_y);
  free(changed_x_next);
  free(changed_y_next);
}

// Randomly assign cells to live or dead, depenring on the value of r
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

// count live neighbours
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

// count dead neighbours
int dead_neighbours(grid_t *g, int x, int y){
  return 8 - live_neighbours(g, x, y);
}

// Iterates through all cells and add the ones that change durirung this generation to the changed_x and change_y list.
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

// Generates a new state for the cell with coordinates x,y.
static inline void shift_cell(grid_t *g, int x, int y){
  int live_count,N;
  N = ng->size;
 if(gt[y+1][x+1] == generation_count)
    return;
 gt[y+1][x+1] += 1;
 if(gt[y+1][x+1] > generation_count){
   gt[y+1][x+1] = generation_count;
   return;
 }

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
}

// same as shift_cell, except that this function stores the new cell coordinates in the running threads change_lists., 
static inline void shift_cell_threaded(grid_t *g, int x, int y, data_t *d){

  if(gt[y+1][x+1] == generation_count)
    return;
  gt[y+1][x+1] += 1;
  if(gt[y+1][x+1] > generation_count){
    gt[y+1][x+1] = generation_count;
    return;
  }
  int live_count,N;
  N = ng->size;
  if(x < g->start_index || x >= g->end_index || y < g->start_index || y >= g->end_index)
     return; 

  // If cell is live
  if (g->cells[y][x]) {
    live_count = live_neighbours(g, x, y);
    if (live_count < 2 || live_count > 3) {
      ng->cells[y][x] = 0;
      d->changed_x[d->count] = x;
      d->changed_y[d->count++] = y;
    }
    
  }
  
  // If cell is dead
  else if (!g->cells[y][x]) {
    live_count = live_neighbours(g, x, y);
    if (live_count == 3) {
      ng->cells[y][x] = 1;
      d->changed_x[d->count] = x;
      d->changed_y[d->count++] = y;
    }
  } 
}

// Is used as a staring function for a thread. Calls shift_cell_threaded for all cells asigned to this thread.
void* shift_worker(void *data){
  
  pthread_detach(pthread_self());
  data_t *d = (data_t*)data;
  grid_t *g = d->g;
  int x, y, index;
  for (int i = d->start; i < d->end; i++){
    index = i+offset;
    x = changed_x[index];
    y = changed_y[index];
    shift_cell_threaded(g,x-1,y-1, d);
    shift_cell_threaded(g,x-1,y, d);
    shift_cell_threaded(g,x-1,y+1, d);
    shift_cell_threaded(g,x,y-1, d);
    shift_cell_threaded(g,x,y, d);
    shift_cell_threaded(g,x,y+1, d);
    shift_cell_threaded(g,x+1,y-1, d);
    shift_cell_threaded(g,x+1,y, d);
    shift_cell_threaded(g,x+1,y+1, d);
  }
  d->done = 1;
  return NULL;
}

// Same as shift_generation, but this function devides the work between multiple threads.
int shift_generation_threaded(grid_t *g){
  int nr_threads = g->nr_threads;
  int N = g->size;
  next_offset = (N*N)*(generation_count % 2);
  offset = (N*N) - next_offset;
  int workload = global_count/nr_threads;
  pthread_t threads[nr_threads];
  int start = 0;
  for(int i=0; i<nr_threads; i++){
    d[i].count = 0;
    d[i].s_count = 0;
    d[i].done = 0;
    
    d[i].start = start;
    if(i == nr_threads-1)
      d[i].end = global_count;
    else
      d[i].end =  start + workload;
    d[i].id = i;
    d[i].g = g;
    
    pthread_create(&(threads[i]), NULL, shift_worker, (void*)&d[i]);
    start = d[i].end;
  }

 int c = 0;
 int x,y;
 int go = 1;

 while(go == 1){
   go = 0;
   for(int i=0; i<nr_threads; i++){
     while(d[i].s_count < d[i].count){
       x = d[i].changed_x[d[i].s_count];
       y = d[i].changed_y[d[i].s_count];
       changed_x[c+next_offset] = x;
       changed_y[c+next_offset] = y;
       g->cells[y][x] = ng->cells[y][x];
       c++;
       d[i].s_count++;
     }
     if(d[i].done == 0)
       go = 1;
   }     
 }  
  global_count = c;
  change_count = 0;
  generation_count++;
  return global_count;
}

// Calculates the new state of all cells that where changed in the prevoius generation, and their neighbouring cells. Redirects the work to shift_generation_threads if more than 1 thread is is set in g->nr_threads.
int shift_generation(grid_t *g){

  if(global_count == 0)
    return shift_generation_first(g);
  if(g->nr_threads > 1)
    return shift_generation_threaded(g);
  int x, y;
  for (int i = 0; i < global_count; i++){
    x = changed_x[i];
    y = changed_y[i];
      shift_cell(g,changed_x[i]-1,changed_y[i]-1);
      shift_cell(g,changed_x[i]-1,changed_y[i]);
      shift_cell(g,changed_x[i]-1,changed_y[i]+1);
    
      shift_cell(g,changed_x[i],changed_y[i]-1);
      shift_cell(g,changed_x[i],changed_y[i]);
      shift_cell(g,changed_x[i],changed_y[i]+1);

      shift_cell(g,changed_x[i]+1,changed_y[i]-1);
      shift_cell(g,changed_x[i]+1,changed_y[i]);
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

// print the grid of cells.
void print_grid(grid_t *g){
  printf("\n");
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
      printf("%d ", g->cells[i][j]);
    }
    printf("\n");
  }
}
