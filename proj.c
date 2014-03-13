#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "parse_util.h"
#include "mat4.h"

//Constants
#define MAX_CMDS 500
#define MAX_CHARS 128
#define COLOR_MAX 255

//Macros
#define swap(j, k, temp) temp = j; j = k; k = temp;

//Input and output files
FILE *i_file;
FILE *o_file;

//Other vars
int p_width, p_height;
double sx_max, sx_min, sy_max, sy_min;

unsigned char curr_color[3];
unsigned char image_buffer[500][500][3];
Mat4 *edge_mat;
Mat4 *trans_mat;


// Variable initialization
void init() {
    //Pixel buffer
    clear_pixels();

    //Matrices
    edge_mat = mat4_create(0);
    trans_mat = mat4_create_identity();
}

//Cleanup
void clean() {
    mat4_delete(edge_mat);
    mat4_delete(trans_mat);
}

//Clear the buffer
void clear_pixels() {
    //Set default color and populate pixels with the color
    curr_color[0] = 0; curr_color[1] = 0; curr_color[2] = 0;

    int j, k;
    for (j = 0; j < p_height; j++)
        for (k = 0; k < p_width; k++)
            memcpy(&image_buffer[j][k], &curr_color, sizeof(curr_color));
}

// Write headers to file
void write_headers() {
    fprintf(o_file, "P3\n%d %d\n%d\n", p_width, p_height, COLOR_MAX);
}

void write_buffer() {
    int j, k;

    for (j = 0; j < p_height; j++)
        for (k = 0; k < p_width; k++)
            fprintf(o_file, "%d %d %d ", image_buffer[j][k][0], image_buffer[j][k][1], image_buffer[j][k][2]);
}

int map_pt(double x, double max, double min, int width) {
    int new_x;
    new_x = ( (x - min) / (max - min) ) * width;

    return new_x;
}

// Most basic rendering: render all lines in the edge matrix
void render_pll() {
    int col, x0, x1, y0, y1;
    curr_color[0] = 255;
    curr_color[1] = 255;
    curr_color[2] = 255;

    for (col = 0; col < edge_mat->cols; col += 2) {
        x0 = map_pt(mat4_get(edge_mat, 0, col), sx_max, sx_min, p_width);
        x1 = map_pt(mat4_get(edge_mat, 0, col + 1), sx_max, sx_min, p_width);
        y0 = map_pt(mat4_get(edge_mat, 1, col), sy_max, sy_min, p_height);
        y1 = map_pt(mat4_get(edge_mat, 1, col + 1), sy_max, sy_min, p_height);

        draw_line(x0, y0, x1, y1);
    }
}

// Plot point
void plot(int x, int y) {
    memcpy(&image_buffer[y][x], &curr_color, sizeof(curr_color));
}

// Bresenham's line algorithm for all octants
void draw_line(int x0, int y0, int x1, int y1) {
    int temp;
    int steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        swap(x0, y0, temp);
        swap(x1, y1, temp);
    }

    if (x0 > x1) {
        swap(x0, x1, temp);
        swap(y0, y1, temp);
    }

    int deltax = x1 - x0,
        deltay = abs(y1 - y0);
    int d = 2 * deltay - deltax;
    int x = x0, y = y0;
    int ystep = y1 > y0 ? 1: -1;

    while (x <= x1) {
        if (steep) plot(y, x); else plot(x, y);
        
        if (d > 0) {
            y += ystep;
            d += -2 * deltax;
        }

        x += 1;
        d += 2 * deltay;
    }
}

// Draw a sphere
void draw_sphere(double rad, double x, double y, double z) {
}

//Removes whitespace at the end of strings
//From stack overflow
void rtrim(char *s) {
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back + 1) = '\0';
}

// Separate file into lines
int read_data(char data[][MAX_CHARS]) {
    int n = 0;

    while(fgets(data[n], sizeof(data[n]), i_file) != NULL) {
        rtrim(data[n]);

        if(data[n][0] != '#')
            n++;
        else
            data[n][0] = '\0';
    }

    return n;
}

// Read numbers from string
void read_nums(double nums[], char **line) {
    int len, i;
    len = parse_numwords(line);

    for (i = 0; i < len - 1; i++) {
        sscanf(line[i + 1], "%lf", &nums[i]);
    }
}

// Run commands from input file
void run() {
    int n, i, j, k;
    char data[MAX_CMDS][MAX_CHARS],
         **line;
    n = read_data(&data);

    for (i = 0; i < n; i++) {
        line = parse_split(data[i]);

        if (strcmp(*line, "line") == 0) {
            double points[6];
            read_nums(points, line);

            double col1[4] = {points[0], points[1], points[2], 1.0};
            double col2[4] = {points[3], points[4], points[5], 1.0};

            mat4_add_column(edge_mat, col1);
            mat4_add_column(edge_mat, col2);
        }
        else if (strcmp(*line, "sphere") == 0) {

        }
        else if (strcmp(*line, "identity") == 0) {
            mat4_delete(trans_mat);
            trans_mat = mat4_create_identity();
        }
        else if (strcmp(*line, "move") == 0) {
            double move[3];
            read_nums(move, line);

            Mat4 *mv = mat4_create_identity();
            mat4_set(mv, 0, 3, move[0]);
            mat4_set(mv, 1, 3, move[1]);
            mat4_set(mv, 2, 3, move[2]);

            Mat4 *new_trans = mat4_multiply(mv, trans_mat);
            mat4_delete(mv);
            mat4_delete(trans_mat);
            trans_mat = new_trans;
        }
        else if (strcmp(*line, "scale") == 0) {
            double scale[3];
            read_nums(scale, line);

            Mat4 *sc = mat4_create_identity();
            mat4_set(sc, 0, 0, scale[0]);
            mat4_set(sc, 1, 1, scale[1]);
            mat4_set(sc, 2, 2, scale[2]);

            Mat4 *new_trans = mat4_multiply(sc, trans_mat);
            mat4_delete(sc);
            mat4_delete(trans_mat);
            trans_mat = new_trans;
        }
        else if (strcmp(*line, "rotate-x") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            Mat4 *rt = mat4_create_identity();
            mat4_set(rt, 1, 1, cos(rads));
            mat4_set(rt, 1, 2, -sin(rads));
            mat4_set(rt, 2, 1, sin(rads));
            mat4_set(rt, 2, 2, cos(rads));

            Mat4 *new_trans = mat4_multiply(rt, trans_mat);
            mat4_delete(rt);
            mat4_delete(trans_mat);
            trans_mat = new_trans;
        }
        else if (strcmp(*line, "rotate-y") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            Mat4 *rt = mat4_create_identity();
            mat4_set(rt, 0, 0, cos(rads));
            mat4_set(rt, 0, 2, sin(rads));
            mat4_set(rt, 2, 0, -sin(rads));
            mat4_set(rt, 2, 2, cos(rads));

            Mat4 *new_trans = mat4_multiply(rt, trans_mat);
            mat4_delete(rt);
            mat4_delete(trans_mat);
            trans_mat = new_trans;
        }
        else if (strcmp(*line, "rotate-z") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            Mat4 *rt = mat4_create_identity();
            mat4_set(rt, 0, 0, cos(rads));
            mat4_set(rt, 0, 1, -sin(rads));
            mat4_set(rt, 1, 0, sin(rads));
            mat4_set(rt, 1, 1, cos(rads));

            Mat4 *new_trans = mat4_multiply(rt, trans_mat);
            mat4_delete(rt);
            mat4_delete(trans_mat);
            trans_mat = new_trans;
        }
        else if (strcmp(*line, "screen") == 0) {
            double bounds[4];
            read_nums(bounds, line);

            sx_min = bounds[0];
            sy_min = bounds[1];
            sx_max = bounds[2];
            sy_max = bounds[3];
        }
        else if (strcmp(*line, "pixels") == 0) {
            double dims[2];
            read_nums(dims, line);

            p_width = dims[0];
            p_height = dims[1];
        }
        else if (strcmp(*line, "transform") == 0) {
            Mat4 *new_edge = mat4_multiply(trans_mat, edge_mat);
            mat4_delete(edge_mat);
            edge_mat = new_edge;
        }
        else if (strcmp(*line, "render-parallel") == 0) {
            render_pll();
        }
        else if (strcmp(*line, "clear-edges") == 0) {
            mat4_delete(edge_mat);
            edge_mat = mat4_create(0);
        }
        else if (strcmp(*line, "clear-pixels") == 0) {
            clear_pixels();
        }
        else if (strcmp(*line, "file") == 0) {
            if ((o_file = fopen(line[1], "ab")) == NULL) {
                perror("Could not open output file\n");
                exit(1);
            }
        }
        else if (strcmp(*line, "end") == 0) {
            write_headers();
            write_buffer();
            clean();
            break;
        }
    }
}

// Main
int main(int argc, char *argv[]) {
    //Set i_file
    if ((i_file = fopen(argv[1], "r")) == NULL) {
        perror("Could not open input file\n");
        exit(1);
    }

    init();
    run();
}
