#include "gol.h"
#include "./graphics/graphics.h"
#include <unistd.h>
#include <time.h>
const float LiveColor=0, DeadColor=1;

// Default window size. scales if N > 1000.
int windowSize = 1000;
float rectangleSide;// = 0.01;


int write_cell_to_file(grid_t *g, const char *fileName) {

  // Remove existing outputfile.
  remove(fileName);
  
  FILE *output_file = fopen(fileName, "a");
  if (!output_file) {
    printf("write_cell_to_file error: failed to open output file '%s'.\n",
           fileName);
    return -1;
  }

  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=0; j<g->size; j++){
      fwrite(&g->cells[i][j], sizeof(char), 1, output_file);
    }
  }
  fclose(output_file);
  
  return 0;
}

int read_cells_from_file(grid_t *g, const char* fileName) {
  int n = g->size;
  /* Open input file and determine its size. */
  FILE* input_file = fopen(fileName, "rb");
  if(!input_file) {
    printf("read_cells_from_file error: failed to open input file '%s'.\n", fileName);
    return -1;
  }
  /* Get filesize using fseek() and ftell(). */
  fseek(input_file, 0L, SEEK_END);
  size_t fileSize = ftell(input_file);
  /* Now use fseek() again to set file position back to beginning of the file. */
  fseek(input_file, 0L, SEEK_SET);
  if(fileSize != n * n * sizeof(char)) {
    printf("read_cells_from_file error: size of input file '%s' does not match the given n.\n", fileName);
    printf("For n = %d the file size is expected to be (n * sizeof(char)) = %lu but the actual file size is %lu.\n",
	   n, n * n * sizeof(char), fileSize);
    return -1;
  }

  for(int i=g->start_index; i<g->end_index; i++){
    for(int j=g->start_index; j<g->end_index; j++){
    /* Read contents of input_file into buffer. */
      fread(&(g->cells[i+1][j+1]), sizeof(char), 1, input_file);
    }
  }
  
  /* OK, now we have the file contents in the buffer.
     Since we are finished with the input file, we can close it now. */
  if(fclose(input_file) != 0) {
    printf("read_cells_from_file error: error closing input file.\n");
    return -1;
  }
  /* Return 0 to indicate success. */
  return 0;
}



int main(int argc, char** argv){

  int N, graphics, tick_max, nr_threads;
  float tick_time;
  clock_t start, end;
  double time_used;
  if(argc < 6 || argc > 8){
    printf("Usage: ./gol inputfile N birth tick_time tick_steps graphics nr_threads\n");
    exit(0);
  }
  
  N = atoi(argv[2]);
  // birth = atoi(argv[3]);
  tick_time = atof(argv[3]) * 1000000;
  tick_max = atoi(argv[4]);
  graphics = atoi(argv[5]);
  if(argc == 7)
    nr_threads = atoi(argv[6]);
  else
    nr_threads = 1;
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
  
  grid_t *grid = create_grid(N, nr_threads);
  //  init_grid(grid, 10);

  read_cells_from_file(grid, argv[1]);
  
  int current_tick = 0;

  while (current_tick++ < tick_max) {

    //start = clock();
    if(shift_generation(grid) == 0){
      break;
    }
    //end = clock();
    //time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    // printf("Time used in shift_generation() = %f\n", time_used);

    if (graphics) {
      ClearScreen();
      start = clock();
      for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
          if (grid->cells[y][x] == 1) {
            DrawRectangle(x * rectangleSide, y * rectangleSide, 1, 1,
                          rectangleSide, rectangleSide, LiveColor);
          }
        }
      }
      Refresh();
      //end = clock();
      //time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
      //printf("Time used redering generation: %f\n", time_used);
      usleep(tick_time);
    }
  }

  if(graphics){
    FlushDisplay();
    CloseDisplay();
  }

  if(argc == 8)
    print_grid(grid);
  
  write_cell_to_file(grid, "result.gen");

  delete_grid(grid);

  return 0;
}
