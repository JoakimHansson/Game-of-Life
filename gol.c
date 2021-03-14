#include "gol.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

typedef struct data{
  grid_t *g;
  int start;
  int end;

  int *pos_of_changed;
  int s_count;
  int count;

  int id;
}data_t;

int *pos_of_changed = NULL;

int global_count = 0;
int generation_count = 0;

int N;

char* looked_at;

grid_t *ng;
grid_t *g;

data_t *d;

int running_threads = 0;

// Initilize everything needed to run the simulation
grid_t* create_grid(int n, int nr_threads){
  g = (grid_t*) malloc(sizeof(grid_t));
  ng = (grid_t*) malloc(sizeof(grid_t));
  N = n;
  g->size = N;
  g->nr_threads = nr_threads;      
  g->start_index = 1;
  g->end_index = N+1;
  ng->start_index = 1;
  ng->end_index = N+1;
  ng->size = N;
  g->cells = (char*)calloc(sizeof(char*), (N+2)*(N+2));
  ng->cells = (char*)calloc(sizeof(char*), (N+2)*(N+2));

  if(pos_of_changed == NULL){
    pos_of_changed = calloc(sizeof(int), N*N*4);
  }
  d = calloc(sizeof(data_t), nr_threads);
  for(int i=0; i<nr_threads; i++){
    d[i].pos_of_changed = calloc(sizeof(int), N*N*4);
    d[i].id = i;
    d[i].g = g;
  }
  looked_at = (char*) calloc(sizeof(char), (N+2)*(N+2));
  return g;
}

// Deallocates all memory used in the simulation
void delete_grid(grid_t *g){
  free(g->cells);
  free(ng->cells);
  free(g);
  free(ng);
  
  free(d->pos_of_changed);
  free(d);
  
  free(pos_of_changed);
  free(looked_at);
}

// Randomly assign cells to live or dead, depenring on the value of r
void init_grid(grid_t* g, int birth){
  srand(time(0));
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
      if(birth > (rand() % 100)){
	g->cells[i * N + j] = 1;
	ng->cells[i * N + j] = 1;
      }
      else{
	g->cells[i * N + j] = 0;
	ng->cells[i * N + j] = 0;
      }
    }
  }
}


// count live neighbours
static inline int live_neighbours(int x, int y){
  int count = 0;
  count += g->cells[(y-1) * N + (x-1)];
  count += g->cells[(y-1) * N + x];
  count += g->cells[(y-1) * N + x+1];
  count += g->cells[y * N + x-1];
  count += g->cells[y * N + x+1];
  count += g->cells[(y+1) * N + x-1];
  count += g->cells[(y+1) * N + x];
  count += g->cells[(y+1) * N + x+1];
  return count;
}

// count dead neighbours
int dead_neighbours(int x, int y){
  return 8 - live_neighbours(x, y);
}

int add_neighbourhood(int x, int y, int *cx, int i) {
  int index;
  int N = ng->size;
  
  index = (y - 1) * N + (x - 1);
  if (__atomic_exchange_n(&((looked_at[index])), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y - 1;
  }
  if (__atomic_exchange_n(&((looked_at[index+1])), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x;
    cx[i++] = y - 1 ;
  }
  if (__atomic_exchange_n(&((looked_at[index+2])), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x + 1;
    cx[i++] = y - 1;
  }

  index = (y) * N + (x-1);
  if (__atomic_exchange_n(&(looked_at[index]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y;
  }
  if (__atomic_exchange_n(&(looked_at[index+1]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x;
    cx[i++] = y;
  }
  if (__atomic_exchange_n(&(looked_at[index+2]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x + 1;
    cx[i++] = y;
  }
  
  index = (y + 1) * N + (x - 1);
  if (__atomic_exchange_n(&(looked_at[index]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y + 1;
  }
  if (__atomic_exchange_n(&(looked_at[index+1]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x;
    cx[i++] = y + 1;
  }
  if (__atomic_exchange_n(&(looked_at[index+2]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x + 1;
    cx[i++] = y + 1;
  }

  return i;
}

// Generates a new state for the cell with coordinates x,y.
static inline void shift_cell(int x, int y, int *cx, int* count){
  int live_count;
  if(x < g->start_index || x >= g->end_index || y < g->start_index || y >= g->end_index)
    return; 
  
  // If cell is live
  if (g->cells[y * N + x] == 1) {
    live_count = live_neighbours(x, y);
    if (live_count < 2 || live_count > 3) {
      ng->cells[y * N + x] = 0;
      *count = add_neighbourhood(x, y, cx, *count);
    }
  }
  
  // If cell is dead
  else {
    live_count = live_neighbours(x, y);
    if (live_count == 3) {
      ng->cells[y * N + x] = 1;
      *count = add_neighbourhood(x, y, cx, *count);
    }
  } 
}

// Is used as a staring function for a thread. Calls shift_cell_threaded for all cells asigned to this thread.
void* shift_worker(void *data){
  
  data_t *d = (data_t*)data;
  int x, y, index;
  for (int i = d->start; i < d->end; i+=2){
    index = i;
    x = pos_of_changed[index];
    y = pos_of_changed[index+1];
    shift_cell(x, y, d->pos_of_changed, &d->count);
  }
  return NULL;
}

void* copy_changed(void *data){
  data_t *d = (data_t*) data;
  // printf("thread id: %d, c: %d, d->count: %d\n",d->id, d->start, d->count);
  int c = d->start;
  int x, y;
    for(int i=0; i<d->count; i+=2){
      x = d->pos_of_changed[i];
      y = d->pos_of_changed[i+1];
      pos_of_changed[c++] = x;
      pos_of_changed[c++] = y;
      g->cells[y * N + x] = ng->cells[y * N + x];
  }

  
  return NULL;
}

// Iterates through all cells and add the ones that change durirung this generation to the changed_x and change_y list.
void* brute_force(void *data){
  
  data_t *d = (data_t*) data;
  int live_count;
  for(int i=d->start; i<d->end; i++){
    for(int j=g->start_index; j<g->end_index; j++){

      // If cell is live
      if(g->cells[i * N + j] == 1){
	live_count = live_neighbours(j, i);
	if(live_count < 2 || live_count > 3){
	  ng->cells[i * N + j] = 0;
          d->count = add_neighbourhood(j, i, d->pos_of_changed, d->count);
	}
      }

      // If cell is dead
      else{
	live_count = live_neighbours(j, i);
	if(live_count == 3){
	  ng->cells[i * N + j] = 1;
          d->count = add_neighbourhood(j, i, d->pos_of_changed, d->count);
	}
      }
    }
  }
  return NULL;
}

void look_at_all(){
  
  int start, nr_threads, workload;
  nr_threads = g->nr_threads;
  workload = N/nr_threads;
  pthread_t threads[nr_threads];
  start = g->start_index;
  for (int i = 0; i < nr_threads; i++) {
    d[i].count = 0;
    d[i].s_count = 0;
    d[i].start = start;
    if (i == nr_threads - 1)
      d[i].end = g->end_index;
    else
      d[i].end = start + workload;
    d[i].id = i;
    pthread_create(&(threads[i]), NULL, brute_force, (void *)&d[i]);
    start = d[i].end;
  }
  for (int i = 0; i < nr_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}

void look_at_changed(){
  
  int start, nr_threads, workload;
  nr_threads = g->nr_threads;
  workload = global_count / nr_threads;
  if(workload % 2 != 0)
    workload -= 1;
  pthread_t threads[nr_threads];
  start = 0;
  for (int i = 0; i < nr_threads; i++) {
    d[i].count = 0;
    d[i].s_count = 0;
    d[i].start = start;
    if (i == nr_threads - 1)
      d[i].end = global_count;
    else
      d[i].end = start + workload;
    d[i].id = i;
      pthread_create(&(threads[i]), NULL, shift_worker, (void *)&d[i]);
      start = d[i].end;
  }
  for (int i = 0; i < nr_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}

int collect_changed(){
  
  int nr_threads = g->nr_threads;
  pthread_t threads[nr_threads];
  int c = 0;
  for (int i = 0; i < nr_threads; i++) {
    d[i].start = c;
    d[i].end = c + d[i].count;
    pthread_create(&(threads[i]), NULL, copy_changed, (void *)&d[i]);
    c = d[i].end;
  }
  for (int i = 0; i < nr_threads; i++) {
    pthread_join(threads[i], NULL);
  }
  return c;
}

int shift_generation(grid_t *g){
  
  int N = g->size;
  memset(looked_at, 0, (N+2)*(N+2));

  if(generation_count == 0)
    look_at_all();
  else
    look_at_changed();

  global_count = collect_changed();
  generation_count++;
  return global_count/2;
}

// print the grid of cells.
void print_grid(grid_t *g){
  
  printf("\n");
  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
      printf("%d ", g->cells[i * N + j]);
    }
    printf("\n");
  }
}
