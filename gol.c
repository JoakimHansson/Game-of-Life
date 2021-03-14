#include "gol.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

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
int generation_count = 1;
int *gt= NULL;
int *gt2= NULL;

int offset;
int next_offset;
int N;

char* test;
char* test2;

grid_t *ng;
grid_t *g;

data_t *d;

pthread_mutex_t lock;
int running_threads = 0;

// Initilize everything needed to run the simulation
grid_t* create_grid(int N, int nr_threads){
  if (pthread_mutex_init(&lock, NULL) != 0) { 
    printf("\n mutex init has failed\n"); 
    exit(1); 
  } 
  printf("N = %d\n", N);
  g = (grid_t*) malloc(sizeof(grid_t));
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
    gt =  calloc(sizeof(int), (N+2)*(N+2)*2);
    gt2 =  calloc(sizeof(int), (N+2)*(N+2)*2);
  }

  d = calloc(sizeof(data_t), nr_threads);
  for(int i=0; i<nr_threads; i++){
    d[i].changed_x = calloc(sizeof(int), (N)*(N));
    d[i].changed_y = calloc(sizeof(int), (N)*(N));
    d[i].id = i;
    d[i].g = g;
  }
  test = (char*) calloc(sizeof(char), (N+2)*(N+2));
  test2 = (char*) calloc(sizeof(char), (N+2)*(N+2));
  return g;
}

// Deallocates all memory used in the simulation
void delete_grid(grid_t *g){
  for(int i=0; i<g->size+2; i++){
    free(g->cells[i]);
    free(ng->cells[i]);
  }
  free(g->cells);
  free(ng->cells);
  free(g);
  free(ng);
  free(d);
  free(changed_x);
  free(changed_y);
  free(changed_x_next);
  free(changed_y_next);
  free(gt);
  free(gt2);
  free(test);
  free(test2);
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

int add_neighbourhood(int x, int y, int *cx, int *cy, int i) {
  int index;
  int N = ng->size;
  index = (y - 1) * N + (x - 1);

  if (x - 1 > g->start_index) {
    if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0 && y > g->start_index){
      cx[i] = x - 1;
      cy[i] = y - 1;
      i++;
    }
    index = (y)*N + (x - 1);
    if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0){
      cx[i] = x - 1;
      cy[i] = y;
      i++;
    }
    index = (y + 1) * N + (x - 1);
    if(__atomic_exchange_n(&((test[index])), 1, __ATOMIC_RELAXED) == 0 && y < g->end_index){
      cx[i] = x - 1;
      cy[i] = y + 1;
      i++;
    }
  }
  
  index = (y - 1) * N + (x);
  if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0 && y > g->start_index){
    cx[i] = x;
    cy[i] = y - 1;
    i++;
  }
  index = (y)*N + (x);
  if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0){
    cx[i] = x;
    cy[i] = y;
    i++;
  }
  index = (y + 1) * N + (x);
  if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0 && y < g->end_index){
    cx[i] = x;
    cy[i] = y + 1;
    i++;
  }
  
  if (x + 1 < g->end_index) {
    index = (y - 1) * N + (x + 1);
    if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0 && y > g->start_index){
      cx[i] = x + 1;
      cy[i] = y - 1;
      i++;
    }
    index = (y)*N + (x + 1);
    if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0){
      cx[i] = x + 1;
      cy[i] = y;
      i++;
    }
    index = (y + 1) * N + (x + 1);
    if(__atomic_exchange_n(&(test[index]), 1, __ATOMIC_RELAXED) == 0 && y < g->end_index){
      cx[i] = x + 1;
      cy[i] = y + 1;
      i++;
    }
  }
  return i;
}

// Iterates through all cells and add the ones that change durirung this generation to the changed_x and change_y list.
int shift_generation_first(grid_t *g){
  int live_count, N;
  int change_count = 0;
  N = g->size;
  memset(test, 0, (N+2)*(N+2));
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){

      // If cell is live
      if(g->cells[i][j] == 1){
	live_count = live_neighbours(g, j, i);
	if(live_count < 2 || live_count > 3){
	  ng->cells[i][j] = 0;
          change_count = add_neighbourhood(j, i, changed_x, changed_y, change_count);
	}
	 
      }

      // If cell is dead
      else if(g->cells[i][j] == 0){
	live_count = live_neighbours(g, j, i);
	if(live_count == 3){
	  ng->cells[i][j] = 1;
          change_count = add_neighbourhood(j, i, changed_x, changed_y, change_count);
	}
      }
    }
  }
  int c = 0;
  int c2 = 0;
  int x,y;
  global_count = change_count;
  generation_count++;
  return change_count;
}

// Generates a new state for the cell with coordinates x,y.
static inline void shift_cell(grid_t *g, int x, int y, int *cx, int*cy, int* count){
  int live_count;
  if(x < g->start_index || x >= g->end_index || y < g->start_index || y >= g->end_index)
    return; 
  
  // If cell is live
  if (g->cells[y][x] == 1) {
    live_count = live_neighbours(g, x, y);
    if (live_count < 2 || live_count > 3) {
      ng->cells[y][x] = 0;
      *count = add_neighbourhood(x, y, cx, cy, *count);
    }
  }
  
  // If cell is dead
  else if (g->cells[y][x] == 0) {
    live_count = live_neighbours(g, x, y);
    if (live_count == 3) {
      ng->cells[y][x] = 1;
      *count = add_neighbourhood(x, y, cx, cy, *count);
    }
  } 
}

// Is used as a staring function for a thread. Calls shift_cell_threaded for all cells asigned to this thread.
void* shift_worker(void *data){
  
  data_t *d = (data_t*)data;
  grid_t *g = d->g;
  int x, y, index;
  for (int i = d->start; i < d->end; i++){
    index = i;
    x = changed_x[index];
    y = changed_y[index];
    shift_cell(g, x, y, d->changed_x, d->changed_y, &d->count);
  }
  d->done = 1;
  return NULL;
}

void* copy_changed(void *data){
  data_t *d = (data_t*) data;
  int c = d->start;
  int end = d->end;
  int x, y;
  for(int i=0; i<d->count; i++){
      x = d->changed_x[i];
      y = d->changed_y[i];
      changed_x[c] = x;
      changed_y[c++] = y;
      g->cells[y][x] = ng->cells[y][x];
  }
  return NULL;
}


// Same as shift_generation, but this function devides the work between multiple threads.
int shift_generation_threaded(grid_t *g){
  int nr_threads = g->nr_threads;
  int N = g->size;
  memset(test, 0, (N+2)*(N+2));
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

  int x,y;
  int c = 0; 
  for(int i=0; i<nr_threads; i++){
    pthread_join(threads[i], NULL);
  }
  for(int i=0; i<nr_threads; i++){
    d[i].start = c;
    d[i].end = c + d[i].count;
    pthread_create(&(threads[i]), NULL, copy_changed, (void*)&d[i]);
    c = d[i].end;
  }
  for(int i=0; i<nr_threads; i++){
    pthread_join(threads[i], NULL);
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

  int N = g->size;
  memset(test, 0, (N+2)*(N+2));
  int x, y;
  for (int i = 0; i < global_count; i++){
    x = changed_x[i];
    y = changed_y[i];
    shift_cell(g, changed_x[i], changed_y[i], changed_x_next, changed_y_next, &change_count);
  }
  int c = 0;
  int c2 = 0;
  while (c < change_count) {
    x = changed_x_next[c];
    y = changed_y_next[c];
    if(gt[y * g->size + x] != generation_count){
      gt[y * g->size + x] = generation_count;
	  changed_x[c2] = x;
	  changed_y[c2++] = y;
	  g->cells[y][x] = ng->cells[y][x];
    }
    c++;
  }
  global_count = c2;
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
