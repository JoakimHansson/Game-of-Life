#ifndef GOL_H
#define GOL_H

#include <stdlib.h>
#include <stdio.h>

/** @struct grid
 *  @brief This struct represents the universe in Game of Life.
 *
 *  @var size The length of the grids rows/columns.
 *  @var cells Array of cells. Use the indexing cells[y * N + x] to access cell on row y in column x. The cell is alive  *             if char is set to 1, otherwise False.
 *
 *  @var start_index Index of the first row/column in cells. 
 *  @var end_index Index of the last row/column in cells.
 *  @var nr_threads The number of threads to split the work on .
 */
typedef struct grid{
  int size;
  char *cells;
  int start_index;
  int end_index;
  int nr_threads;
}grid_t;

/** @brief Allocates memory for a NxN grid. All cells is set to 0.
*
*  @param N The number of rows and number of cells in each row.
*  @param nr_threads The number of available threads.
*  @return Pointer to the grid.
*/
grid_t* init_grid(int N, int nr_threads);

/** @brief Free all memory used by g.
*
*/
void destroy_grid();

/** @brief Set the cells in g to alive or dead, depending on if a random generated value is larger than r.
*
*  @param r A number between 1 and 100.
*/
void random_grid(int r);

/** @brief Updates all cells in g to a new generation.
*
*  1. Any live cell with two or three live neighbours survives.
*  2. Any dead cell with three live neighbours becomes a live cell.
*  3. All other live cells die in the next generation. Similarly, all other dead cells stay dead.
*
*  @return Number of cells that changed in this generation shift.
*/
int evolve_grid();

/** @brief Print g to stdout.
*
*/
void print_grid();

/** @brief Reads the state of each cell from file. Make sure that the file
contains the same amount of cells as the N used to initilize the grid.
*
*  @param filename The file to read from
*  @return 0 if reads was OK, otherwise -1.
*/
int read_grid_from_file(const char *fileName);

/** @brief Write the state of each cell to file.
*
*  @param filename The file to read from
*  @return 0 if write was OK, otherwise -1.
*/
int write_grid_to_file(const char* fileName);

#endif
