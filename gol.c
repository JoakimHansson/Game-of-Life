#include "gol.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct data {
  grid_t *g;
  int start;
  int end;

  int *pos_of_changed;
  int s_count;
  int count;

  int id;
} data_t;

int N;
int threads;
int global_count = 0;
int generation_count = 0;
char *looked_at;
int *pos_of_changed;

grid_t *ng;
grid_t *g;

data_t *d;

int running_threads = 0;

grid_t *init_grid(int n, int nr_threads) {
  g = (grid_t *)malloc(sizeof(grid_t));
  ng = (grid_t *)malloc(sizeof(grid_t));
  N = n;
  g->size = N;
  threads = nr_threads;
  g->nr_threads = nr_threads;
  g->start_index = 1;
  g->end_index = N + 1;
  ng->start_index = 1;
  ng->end_index = N + 1;
  ng->size = N;
  g->cells = (char *)calloc(sizeof(char *), (N + 2) * (N + 2));
  ng->cells = (char *)calloc(sizeof(char *), (N + 2) * (N + 2));
  looked_at = (char *)calloc(sizeof(char), (N + 2) * (N + 2));
  pos_of_changed = calloc(sizeof(int), N * N * 4);
  d = calloc(sizeof(data_t), nr_threads);
  for (int i = 0; i < nr_threads; i++) {
    d[i].pos_of_changed = calloc(sizeof(int), N * N * 4);
    d[i].id = i;
    d[i].g = g;
  }
  return g;
}

void destroy_grid() {
  free(g->cells);
  free(ng->cells);
  free(g);
  free(ng);
  for (int i = 0; i < threads; i++) {
    free(d[i].pos_of_changed);
  }
  free(d);

  free(pos_of_changed);
  free(looked_at);
}

void random_grid(int birth) {
  srand(time(0));
  for (int i = g->start_index; i < g->end_index; i++) {
    for (int j = g->start_index; j < g->end_index; j++) {
      if (birth > (rand() % 100)) {
        g->cells[i * N + j] = 1;
        ng->cells[i * N + j] = 1;
      } else {
        g->cells[i * N + j] = 0;
        ng->cells[i * N + j] = 0;
      }
    }
  }
}

static inline int live_neighbours(int x, int y) {
  int count = 0;
  count += g->cells[(y - 1) * N + (x - 1)];
  count += g->cells[(y - 1) * N + x];
  count += g->cells[(y - 1) * N + x + 1];
  count += g->cells[y * N + x - 1];
  count += g->cells[y * N + x + 1];
  count += g->cells[(y + 1) * N + x - 1];
  count += g->cells[(y + 1) * N + x];
  count += g->cells[(y + 1) * N + x + 1];
  return count;
}

int add_neighbourhood(int x, int y, int *cx, int i) {
  int index;
  int N = ng->size;

  index = (y - 1) * N + (x - 1);
  if (__atomic_exchange_n(&((looked_at[index])), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y - 1;
  }
  if (__atomic_exchange_n(&((looked_at[index + 1])), 1, __ATOMIC_RELAXED) ==
      0) {
    cx[i++] = x;
    cx[i++] = y - 1;
  }
  if (__atomic_exchange_n(&((looked_at[index + 2])), 1, __ATOMIC_RELAXED) ==
      0) {
    cx[i++] = x + 1;
    cx[i++] = y - 1;
  }

  index = (y)*N + (x - 1);
  if (__atomic_exchange_n(&(looked_at[index]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y;
  }
  if (__atomic_exchange_n(&(looked_at[index + 1]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x;
    cx[i++] = y;
  }
  if (__atomic_exchange_n(&(looked_at[index + 2]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x + 1;
    cx[i++] = y;
  }

  index = (y + 1) * N + (x - 1);
  if (__atomic_exchange_n(&(looked_at[index]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x - 1;
    cx[i++] = y + 1;
  }
  if (__atomic_exchange_n(&(looked_at[index + 1]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x;
    cx[i++] = y + 1;
  }
  if (__atomic_exchange_n(&(looked_at[index + 2]), 1, __ATOMIC_RELAXED) == 0) {
    cx[i++] = x + 1;
    cx[i++] = y + 1;
  }

  return i;
}

void *copy_changed(void *data) {
  data_t *d = (data_t *)data;
  int c = d->start;
  int x, y;
  for (int i = 0; i < d->count; i += 2) {
    x = d->pos_of_changed[i];
    y = d->pos_of_changed[i + 1];
    pos_of_changed[c++] = x;
    pos_of_changed[c++] = y;
    g->cells[y * N + x] = ng->cells[y * N + x];
  }

  return NULL;
}

// Generates a new state for the cell with coordinates x,y.
static inline void look_at_cell(int x, int y, int *cx, int *count) {
  int live_count;
  if (x < g->start_index || x >= g->end_index || y < g->start_index ||
      y >= g->end_index)
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

void *look_at_changed_help(void *data) {

  data_t *d = (data_t *)data;
  int x, y, index;
  for (int i = d->start; i < d->end; i += 2) {
    index = i;
    x = pos_of_changed[index];
    y = pos_of_changed[index + 1];
    look_at_cell(x, y, d->pos_of_changed, &d->count);
  }
  return NULL;
}

void *look_at_all_help(void *data) {

  data_t *d = (data_t *)data;
  for (int i = d->start; i < d->end; i++) {
    for (int j = g->start_index; j < g->end_index; j++) {
      look_at_cell(j, i, d->pos_of_changed, &d->count);
    }
  }
  return NULL;
}

void look_at_all() {

  int start, nr_threads, workload;
  nr_threads = g->nr_threads;
  workload = N / nr_threads;
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
    pthread_create(&(threads[i]), NULL, look_at_all_help, (void *)&d[i]);
    start = d[i].end;
  }
  for (int i = 0; i < nr_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}

void look_at_changed() {

  int start, nr_threads, workload;
  nr_threads = g->nr_threads;
  workload = global_count / nr_threads;
  if (workload % 2 != 0)
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
    pthread_create(&(threads[i]), NULL, look_at_changed_help, (void *)&d[i]);
    start = d[i].end;
  }
  for (int i = 0; i < nr_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}

int collect_changed() {

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

int evolve_grid() {

  int N = g->size;
  memset(looked_at, 0, (N + 2) * (N + 2));

  if (generation_count == 0)
    look_at_all();
  else
    look_at_changed();

  global_count = collect_changed();
  generation_count++;
  return global_count / 2;
}

void print_grid() {

  printf("\n");
  for (int i = g->start_index; i < g->end_index; i++) {
    for (int j = g->start_index; j < g->end_index; j++) {
      printf("%d ", g->cells[i * N + j]);
    }
    printf("\n");
  }
}

int write_grid_to_file(const char *fileName) {

  // Remove existing outputfile.
  remove(fileName);

  FILE *output_file = fopen(fileName, "a");
  if (!output_file) {
    printf("write_cell_to_file error: failed to open output file '%s'.\n",
           fileName);
    return -1;
  }

  for (int i = g->start_index; i < g->end_index; i++) {
    for (int j = 0; j < g->size; j++) {
      fwrite(&g->cells[i * g->size + j], sizeof(char), 1, output_file);
    }
  }
  fclose(output_file);

  return 0;
}

int read_grid_from_file(const char *fileName) {
  int N = g->size;
  /* Open input file and determine its size. */
  FILE *input_file = fopen(fileName, "rb");
  if (!input_file) {
    printf("read_cells_from_file error: failed to open input file '%s'.\n",
           fileName);
    return -1;
  }
  /* Get filesize using fseek() and ftell(). */
  fseek(input_file, 0L, SEEK_END);
  size_t fileSize = ftell(input_file);
  /* Now use fseek() again to set file position back to beginning of the file.
   */
  fseek(input_file, 0L, SEEK_SET);
  if (fileSize != N * N * sizeof(char)) {
    printf("read_cells_from_file error: size of input file '%s' does not match "
           "the given N.\n",
           fileName);
    printf("For n = %d the file size is expected to be (n * sizeof(char)) = "
           "%lu but the actual file size is %lu.\n",
           N, N * N * sizeof(char), fileSize);
    return -1;
  }

  for (int i = g->start_index; i < g->end_index; i++) {
    for (int j = g->start_index; j < g->end_index; j++) {
      /* Read contents of input_file into buffer. */
      fread(&(g->cells[i * N + j]), sizeof(char), 1, input_file);
      ng->cells[i * N + j] = g->cells[i * N + j];
    }
  }

  if (fclose(input_file) != 0) {
    printf("read_cells_from_file error: error closing input file.\n");
    return -1;
  }
  /* Return 0 to indicate success. */
  return 0;
}
