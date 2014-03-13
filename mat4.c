/*	Basic functions for 4-row matrices */
#include <stdio.h>
#include <stdlib.h>
#include "mat4.h"


/* 	==================== mat4_create() ========================================================
        Create a matrix (with 4 rows) with ncolumns, where ncolumns >= 0.  
        If ncolumns > 0, then set all cells to 0.
        Returns pointer to Mat4.

Usage: Mat4 *a = mat4_create(10);  // creates a 4 x 10 matrix, zero-filled
*/
Mat4 *mat4_create(int ncolumns) {
    Mat4 *result;

    if (ncolumns < 0) {
        printf("Error in mat4_create().  ncolumns must be >= 0\n");
        return NULL;
    }

    result = calloc(1,sizeof(Mat4));
    if (ncolumns == 0)
        return result;

    if ((result->cells = calloc(ncolumns*4,sizeof(double))) == NULL) {
        printf("Error in mat4_create(): calloc failed on ncolumns == %d\n", ncolumns);
        return NULL;
    }

    result->cols = ncolumns;
    return result;
}

/* ===================== mat4_delete() ===================================================
   Delete a previously created mat4 matrix.

Usage: mat4_delete(a);
*/

void mat4_delete(Mat4 *m) {
    if (m->cols > 0)
        free(m->cells);
    free(m);
}

/* ======================  mat4_create_identity()  ====================================================
   Will create an 4x4 identity matrix.
   Returns pointer to Mat4.

Usage:  Mat4 *a = mat4_create_identity()
*/
Mat4 *mat4_create_identity() {
    int i;
    Mat4 *m = mat4_create(4);
    for (i = 0; i < 4; ++i)
        mat4_set(m, i, i, 1);
    return m;
}

/* ============================== mat4_copy() ============================================================
   Create a copy of a matrix

Usage: Mat4 *c = mat4_copy(a);
*/
Mat4 *mat4_copy(Mat4 *original) {
    int row, col;
    Mat4 *result = mat4_create(original->cols);
    if (original->cols > 0)
        for (row = 0; row < 4; ++row)
            for (col = 0; col < original->cols; ++col)
                mat4_set(result,row,col,mat4_get(original,row,col));
    return result;
}

/* =============================== mat4_add_column() ====================================================
   Adds a column to the right-hand end of an existent matrix.
   Returns the modified matrix (old matrix is modified).

Usage:  
double new_cells[4] = {1.0, 2.3, 4.0, 1.0};
Mat4 *new = mat4_add_column(old, new_cells);
*/
Mat4 *mat4_add_column(Mat4 *old, double *new_cells) {
    int row;

    if (old->cols == 0)
        old->cells = calloc(4, sizeof(double));
    else 
        old->cells = realloc(old->cells,(old->cols+1) * 4 * sizeof(double));
    old->cols += 1;
    for (row = 0; row < 4; ++row)
        mat4_set(old, row, old->cols-1, new_cells[row]);
    return old;
}

/* ============================== mat4_set() =========================================================
   Sets a cell in the matrix to a value

Usage:
mat4_set(m, row, col, new_value);
NOTE: row and col indices start at 0.
Returns m;
*/
Mat4 *mat4_set(Mat4 *matrix, int row, int col, double value) {
    if (row < 0 || row >= 4 || col < 0 || col >= matrix->cols) {
        printf("Error in mat4_set(), index out of range, row = %d, col = %d\n",row,col);
        return matrix;
    }
    matrix->cells[col*4+row] = value;
    return matrix;
}

/* =============================== mat4_get() ===========================================================
   Returns the value in a cell.

Usage:
double c = mat4_get(m, row, col);
NOTE: row and col indices start at 0.
*/
double mat4_get(Mat4 *matrix, int row, int col) {
    if (row < 0 || row >= 4 || col < 0 || col >= matrix->cols) {
        printf("Error in mat4_get(), index out of range, row = %d, col = %d\n",row,col);
        return 0.0;
    }
    return matrix->cells[col*4+row];
}

/* ================================= mat4_columns ===================================================
   Returns the number of columns in a matrix.

Usage: int n = mat4_columns(m);
*/
int mat4_columns(Mat4 *m) {
    return m->cols;
}

/* ================================== mat4_print ===================================== 
   Prints a matrix

Usage:  mat4_print(m, "the edge matrix");
*/
void mat4_print(char *s, Mat4 *matrix) {
    int row, col;
    printf("%s\n",s);
    for (row = 0; row < 4; ++row) {
        for (col = 0; col < mat4_columns(matrix); ++col)
            printf("%5.2f  ", mat4_get(matrix,row,col));
        printf("\n");
    }
}

/* ================================== mat4_print ===================================== 
*/

Mat4 *mat4_multiply(Mat4 *m1, Mat4 *m2) {
    int row, col;
    Mat4 *result = mat4_create(m2->cols);

    for (row = 0; row < 4; row++) {
        for (col = 0; col < m2->cols; col++) {
            double sum = mat4_get(m1, row, 0) * mat4_get(m2, 0, col) + mat4_get(m1, row, 1) * mat4_get(m2, 1, col) + mat4_get(m1, row, 2) * mat4_get(m2, 2, col) + mat4_get(m1, row, 3) * mat4_get(m2, 3, col);

            mat4_set(result, row, col, sum);
        }
    }

    return result;
}



