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
#define SPH_REP 20.0

//Macros
#define apply_trans(t) \
    Mat4 *new_trans = mat4_multiply(t, trans_mat); \
    mat4_delete(t); \
    mat4_delete(trans_mat); \
    trans_mat = new_trans;
#define print_edge() mat4_print("Edge Matrix", edge_mat);
#define print_trans() mat4_print("Tranformation Matrix", trans_mat);
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
    clear_buffer();

    //Matrices
    edge_mat = mat4_create(0);
    trans_mat = mat4_create_identity();
}

//Cleanup
void free_ptrs() {
    mat4_delete(edge_mat);
    mat4_delete(trans_mat);
}

//Clear the buffer
void clear_buffer() {
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

//Convert from world coords in edge matrix to pixels.
void convert_wld() {
    int col, x0, x1, y0, y1;

    identity();
    t_scale(p_width / (sx_max - sx_min), p_height / (sy_max - sy_min), 1.0);
    t_move((p_width * -sx_min) / (sx_max - sx_min), (p_height * -sy_min) / (sy_max - sy_min), 1.0);
    transform();

    for (col = 0; col < edge_mat->cols; col += 2) {
        x0 = mat4_get(edge_mat, 0, col);
        x1 = mat4_get(edge_mat, 0, col + 1);
        y0 = mat4_get(edge_mat, 1, col);
        y1 = mat4_get(edge_mat, 1, col + 1);

        draw_line(x0, y0, x1, y1);
    }
}

// Most basic rendering: render all lines in the edge matrix
void render_pll(char color[3]) {
    curr_color[0] = color[0]; curr_color[1] = color[1]; curr_color[2] = color[2];

    convert_wld();
}

//Rendering with perspective
void render_eye(double ex, double ey, double ez, char color[3]) {
    int col;
    double x0, x1, y0, y1;
    Mat4 *temp = mat4_copy(edge_mat);
    curr_color[0] = color[0]; curr_color[1] = color[1]; curr_color[2] = color[2];

    for (col = 0; col < edge_mat->cols; col += 2) {
        x0 = ex + (ex - mat4_get(edge_mat, 0, col)) * -ez / (ez - mat4_get(edge_mat, 2, col));
        x1 = ex + (ex - mat4_get(edge_mat, 0, col + 1)) * -ez / (ez - mat4_get(edge_mat, 2, col + 1));
        y0 = ey + (ey - mat4_get(edge_mat, 1, col)) * ez / (ez - mat4_get(edge_mat, 2, col));
        y1 = ey + (ey - mat4_get(edge_mat, 1, col + 1)) * ez / (ez - mat4_get(edge_mat, 2, col + 1));
    
        mat4_set(edge_mat, 0, col, x0);
        mat4_set(edge_mat, 0, col + 1, x1);
        mat4_set(edge_mat, 1, col, y0);
        mat4_set(edge_mat, 1, col + 1, y1);
    }

    convert_wld();
    mat4_delete(edge_mat);
    edge_mat = temp;
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
void draw_sphere(double rad, double sx, double sy, double sz) {
    double phi, theta, r, x, z;

    for (phi = 0; phi < M_PI; phi += M_PI / SPH_REP) {
        r = rad * sin(phi);
        z = rad * cos(phi);

        for (theta = 0; theta < 2.0 * M_PI; theta += M_PI / SPH_REP) {
            double p1[4] = {r * cos(theta), r * sin(theta), z, 1.0};
            theta += M_PI / SPH_REP;
            double p2[4] = {r * cos(theta), r * sin(theta), z, 1.0};
            theta -= M_PI / SPH_REP;

            mat4_add_column(edge_mat, p1);
            mat4_add_column(edge_mat, p2);
        }
    }

    for (theta = 0; theta < 2.0 * M_PI; theta += M_PI / SPH_REP) {
        x = cos(theta);

        for (phi = 0; phi < M_PI; phi += M_PI / SPH_REP) {
            r = rad * sin(phi);

            double p1[4] = {r * x, r * sin(theta), rad * cos(phi), 1.0};
            phi += M_PI / SPH_REP;
            r = rad * sin(phi);
            double p2[4] = {r * x, r * sin(theta), rad * cos(phi), 1.0};
            phi -= M_PI / SPH_REP;

            mat4_add_column(edge_mat, p1);
            mat4_add_column(edge_mat, p2);
        }
    }
}

void identity() {
    mat4_delete(trans_mat);
    trans_mat = mat4_create_identity();
}

void t_move(double x, double y, double z) {
    Mat4 *mv = mat4_create_identity();
    mat4_set(mv, 0, 3, x);
    mat4_set(mv, 1, 3, y);
    mat4_set(mv, 2, 3, z);

    apply_trans(mv);
}

void t_scale(double sx, double sy, double sz) {
    Mat4 *sc = mat4_create_identity();
    mat4_set(sc, 0, 0, sx);
    mat4_set(sc, 1, 1, sy);
    mat4_set(sc, 2, 2, sz);

    apply_trans(sc);
}

void t_rotate_x(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 1, 1, cos(rads));
    mat4_set(rt, 1, 2, -sin(rads));
    mat4_set(rt, 2, 1, sin(rads));
    mat4_set(rt, 2, 2, cos(rads));

    apply_trans(rt);
}

void t_rotate_y(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 0, 0, cos(rads));
    mat4_set(rt, 0, 2, sin(rads));
    mat4_set(rt, 2, 0, -sin(rads));
    mat4_set(rt, 2, 2, cos(rads));

    apply_trans(rt);
}

void t_rotate_z(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 0, 0, cos(rads));
    mat4_set(rt, 0, 1, -sin(rads));
    mat4_set(rt, 1, 0, sin(rads));
    mat4_set(rt, 1, 1, cos(rads));

    apply_trans(rt);
}

void transform() {
    Mat4 *new_edge = mat4_multiply(trans_mat, edge_mat);
    mat4_delete(edge_mat);
    edge_mat = new_edge;
}

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
            double input[4];
            read_nums(input, line);

            draw_sphere(input[0], input[1], input[2], input[3]);
        }
        else if (strcmp(*line, "identity") == 0) {
            identity();
        }
        else if (strcmp(*line, "move") == 0) {
            double move[3];
            read_nums(move, line);

            t_move(move[0], move[1], move[2]);
        }
        else if (strcmp(*line, "scale") == 0) {
            double scale[3];
            read_nums(scale, line);

            t_scale(scale[0], scale[1], scale[2]);
        }
        else if (strcmp(*line, "rotate-x") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            t_rotate_x(rads);
        }
        else if (strcmp(*line, "rotate-y") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            t_rotate_y(rads);
        }
        else if (strcmp(*line, "rotate-z") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            t_rotate_z(rads);
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
            transform();
        }
        else if (strcmp(*line, "render-parallel") == 0) {
            char color[3] = {255, 255, 255};
            render_pll(color);
        }
        else if (strcmp(*line, "render-perspective-cyclops") == 0) {
            double eye[3];
            char color[3] = {255, 255, 255};
            read_nums(eye, line);

            render_eye(eye[0], eye[1], eye[2], color);
        }
        else if (strcmp(*line, "render-perspective-stereo") == 0) {
            double eyes[6];
            char c1[3] = {255, 0, 0};
            char c2[3] = {0, 200, 200};
            read_nums(eyes, line);

            render_eye(eyes[0], eyes[1], eyes[2], c1);
            render_eye(eyes[3], eyes[4], eyes[5], c2);
        }
        else if (strcmp(*line, "clear-edges") == 0) {
            mat4_delete(edge_mat);
            edge_mat = mat4_create(0);
        }
        else if (strcmp(*line, "clear-pixels") == 0) {
            clear_buffer();
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
            free_ptrs();
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
