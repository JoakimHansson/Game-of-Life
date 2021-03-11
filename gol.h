#ifndef GOL_H
#define GOL_H

#include <stdlib.h>
#include <stdio.h>

// Each cell is represented by a boolean. The cell is alive if the boolean is True, otherwise False.
typedef struct grid{
  int size;
  int start_index;
  int end_index;
  int live;
  char **cells;
}grid_t;

/** @brief Allocates memory for a NxN grid. All cells is set to 0.
*
*  @param N number of rows and number of cells in each row.
*  @return Pointer to the grid.
*/
grid_t* create_grid(int N);

/** @brief Free all memory used by g.
*
*  @param g grid to be removed.
*/
void delete_grid(grid_t *g);

/** @brief Set the cells in g to alive or dead, depending on if a random generated value is larger than r.
*
*  @param g The grid to initilize the cells in.
*  @param r A number between 1 and 100.
*/
void init_grid(grid_t *g, int r);

/** @brief count all live cells in the neighbourhood.
*
*  @param g The grid holding the cells in.
*  @param x The x coordinate.
*  @param y The y coordinate.
*  @return Number of live neighbours
*/
//static inline int live_neighbours(grid_t *g, int x, int y);

/** @brief count all dead cells in the neighbourhood.
*
*  @param g The grid holding the cells.
*  @param x The x coordinate.
*  @param y The y coordinate.
*  @return Number of live neighbours
*/
int dead_neighbours(grid_t *g, int x, int y);

/** @brief Updates all cells in g to a new generation.
*
*  1. Any live cell with two or three live neighbours survives.
*  2. Any dead cell with three live neighbours becomes a live cell.
*  3. All other live cells die in the next generation. Similarly, all other dead cells stay dead.
*
*  @param g The grid holding the current generation.
*  @return Number of cells that changed in this generation shift.
*/
int shift_generation(grid_t *g);
int shift_generation_first(grid_t *g);

/** @brief Print g to stdout.
*
*  @param g The grid to print.
*/
void print_grid(grid_t *g);

#endif
