#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "parse_util.h"
#include "mat4.h"
#include "proj.h"

/*
 * Problems:
 * 1) Y rotation is negated
 * 2) apply_trans is not a function, and some garbage collection needs to be done
 * 3) Dot product comes out positive for visible faces
 * 4) Sphere shows stem
 * */

//Input and output files
FILE *i_file;
FILE *o_file;

//Other vars
int p_width, p_height;
double sx_max, sx_min, sy_max, sy_min;

unsigned char curr_color[3];
unsigned char ***image_buffer;
Mat4 *triangle_mat;
Mat4 *trans_mat;
Mat4 *new_m;

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

            mat4_add_column(triangle_mat, col1);
            mat4_add_column(triangle_mat, col2);
        }
        else if (strcmp(*line, "box-t") == 0) {
            double input[9];
            read_nums(input, line);

            add_box(input[0], input[1], input[2], input[3], input[4], input[5], input[6], input[7], input[8]);
        }
        else if (strcmp(*line, "sphere-t") == 0) {
            double input[9];
            read_nums(input, line);

            add_sphere(input[0], input[1], input[2], input[3], input[4], input[5], input[6], input[7], input[8]);
        }
        else if (strcmp(*line, "identity") == 0) {
            identity();
        }
        else if (strcmp(*line, "move") == 0) {
            double move[3];
            read_nums(move, line);

            apply_trans(t_move(move[0], move[1], move[2]), trans_mat);
        }
        else if (strcmp(*line, "scale") == 0) {
            double scale[3];
            read_nums(scale, line);

            apply_trans(t_scale(scale[0], scale[1], scale[2]), trans_mat);
        }
        else if (strcmp(*line, "rotate-x") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            apply_trans(t_rotate_x(rads), trans_mat);
        }
        else if (strcmp(*line, "rotate-y") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            apply_trans(t_rotate_y(rads), trans_mat);
        }
        else if (strcmp(*line, "rotate-z") == 0) {
            double rotate[1], rads;
            read_nums(rotate, line);
            rads = rotate[0] * M_PI / 180.0;

            apply_trans(t_rotate_z(rads), trans_mat);
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

            image_buffer = (unsigned char ***)calloc(p_height, sizeof(char **));
            int i, j;
            for (i = 0; i < p_width; i++) {
                image_buffer[i] = (unsigned char **)calloc(p_width, sizeof(char *));

                for (j = 0; j < p_width; j++) {
                    image_buffer[i][j] = (unsigned char *)calloc(3, sizeof(char));
                }
            }

            clear_buffer();
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
        else if (strcmp(*line, "clear-triangles") == 0) {
            mat4_delete(triangle_mat);
            triangle_mat = mat4_create(0);
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

// Variable initialization
void init() {
    //Matrices
    triangle_mat = mat4_create(0);
    trans_mat = mat4_create_identity();
}

//Cleanup
void free_ptrs() {
    mat4_delete(triangle_mat);
    mat4_delete(trans_mat);
    free(image_buffer);
}

//Clear the buffer
void clear_buffer() {
    //Set default color and populate pixels with the color
    curr_color[0] = 0; curr_color[1] = 0; curr_color[2] = 0;

    int j, k;
    for (j = 0; j < p_height; j++)
        for (k = 0; k < p_width; k++)
            memcpy(image_buffer[j][k], &curr_color, sizeof(curr_color));
}

// Write headers to file
void write_headers() {
    fprintf(o_file, "P3\n%d %d\n%d\n", p_width, p_height, COLOR_MAX);
}

//Write from image buffer to the file
void write_buffer() {
    int j, k;

    for (j = 0; j < p_height; j++)
        for (k = 0; k < p_width; k++)
            fprintf(o_file, "%d %d %d ", image_buffer[j][k][0], image_buffer[j][k][1], image_buffer[j][k][2]);
}

//Convert from world coords in edge matrix to pixels.
void convert_wld() {
    int col, x0, x1, x2, y0, y1, y2;
    double wx0, wx1, wx2, wy0, wy1, wy2, wz0, wz1, wz2;
    Mat4 *l_trans, *copy = mat4_create(0);

    //Test if face is pointed towards the eye
    for (col = 0; col < triangle_mat->cols; col += 3) {
        wx0 = mat4_get(triangle_mat, 0, col);
        wx1 = mat4_get(triangle_mat, 0, col + 1);
        wx2 = mat4_get(triangle_mat, 0, col + 2);
        wy0 = mat4_get(triangle_mat, 1, col);
        wy1 = mat4_get(triangle_mat, 1, col + 1);
        wy2 = mat4_get(triangle_mat, 1, col + 2);
        wz0 = mat4_get(triangle_mat, 2, col);
        wz1 = mat4_get(triangle_mat, 2, col + 1);
        wz2 = mat4_get(triangle_mat, 2, col + 2);

        //Use dynamic values for eye vector later
        double u[3] = {wx0 - wx1, wy0 - wy1, wz0 - wz1};
        //printf("u: %lf %lf %lf\n", u[0], u[1], u[2]);
        double v[3] = {wx2 - wx1, wy2 - wy1, wz2 - wz1};
        //printf("v: %lf %lf %lf\n", v[0], v[1], v[2]);
        double eye_vector[3] = {wx1 - 0.0, wy1 - 0.0, wz1 - 4.0};

        double r[3];
        cross_product(u, v, r);

        //printf("r: %lf %lf %lf\n", r[0], r[1], r[2]);
        //printf("eye: %lf %lf %lf\n", eye_vector[0], eye_vector[1], eye_vector[2]);
        //printf("%lf\n", dot_product(r, eye_vector));
        double col1[4] = { mat4_get(triangle_mat, 0, col),
                        mat4_get(triangle_mat, 1, col),
                        mat4_get(triangle_mat, 2, col), 1.0};

        double col2[4] = { mat4_get(triangle_mat, 0, col + 1),
                        mat4_get(triangle_mat, 1, col + 1),
                        mat4_get(triangle_mat, 2, col + 1), 1.0};

        double col3[4] = { mat4_get(triangle_mat, 0, col + 2),
                        mat4_get(triangle_mat, 1, col + 2),
                        mat4_get(triangle_mat, 2, col + 2), 1.0};

        //Show if facing towards the eye
        if (dot_product(r, eye_vector) >= 0.0) {
            mat4_add_column(copy, col1);
            mat4_add_column(copy, col2);
            mat4_add_column(copy, col3);
        }
    }

    mat4_delete(triangle_mat);
    triangle_mat = copy;

    //Convert to pixel coordinates
    identity();

    l_trans = t_scale(p_width / (sx_max - sx_min), p_height / (sy_max - sy_min), 1.0);
    apply_trans(l_trans, trans_mat);
    mat4_delete(l_trans);

    l_trans = t_move((p_width * -sx_min) / (sx_max - sx_min), (p_height * -sy_min) / (sy_max - sy_min), 0);
    apply_trans(l_trans, trans_mat);
    mat4_delete(l_trans);

    transform();

    for (col = 0; col < triangle_mat->cols; col += 3) {
        x0 = mat4_get(triangle_mat, 0, col);
        x1 = mat4_get(triangle_mat, 0, col + 1);
        x2 = mat4_get(triangle_mat, 0, col + 2);
        y0 = mat4_get(triangle_mat, 1, col);
        y1 = mat4_get(triangle_mat, 1, col + 1);
        y2 = mat4_get(triangle_mat, 1, col + 2);

        draw_line(x0, y0, x1, y1);
        draw_line(x1, y1, x2, y2);
        draw_line(x2, y2, x0, y0);
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
    double x0, x1, x2, y0, y1, y2;
    Mat4 *triangle_save = mat4_copy(triangle_mat);
    curr_color[0] = color[0]; curr_color[1] = color[1]; curr_color[2] = color[2];

    for (col = 0; col < triangle_mat->cols; col += 3) {
        x0 = ex + (ex - mat4_get(triangle_mat, 0, col)) * -ez / (ez - mat4_get(triangle_mat, 2, col));
        x1 = ex + (ex - mat4_get(triangle_mat, 0, col + 1)) * -ez / (ez - mat4_get(triangle_mat, 2, col + 1));
        x2 = ex + (ex - mat4_get(triangle_mat, 0, col + 2)) * -ez / (ez - mat4_get(triangle_mat, 2, col + 2));
        y0 = ey + (ey - mat4_get(triangle_mat, 1, col)) * -ez / (ez - mat4_get(triangle_mat, 2, col));
        y1 = ey + (ey - mat4_get(triangle_mat, 1, col + 1)) * -ez / (ez - mat4_get(triangle_mat, 2, col + 1));
        y2 = ey + (ey - mat4_get(triangle_mat, 1, col + 2)) * -ez / (ez - mat4_get(triangle_mat, 2, col + 2));

        mat4_set(triangle_mat, 0, col, x0);
        mat4_set(triangle_mat, 0, col + 1, x1);
        mat4_set(triangle_mat, 0, col + 2, x2);
        mat4_set(triangle_mat, 1, col, y0);
        mat4_set(triangle_mat, 1, col + 1, y1);
        mat4_set(triangle_mat, 1, col + 2, y2);
    }

    convert_wld();
    mat4_delete(triangle_mat);
    triangle_mat = triangle_save;
}

//Calculate the cross product
void cross_product(double u[3], double v[3], double r[]) {
    r[0] = u[1] * v[2] - u[2] * v[1];
    r[1] = u[2] * v[0] - u[0] * v[2];
    r[2] = u[0] * v[1] - u[1] * v[0];
}

//Calculate the dot product
double dot_product(double *u, double *v) {
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}

// Plot point
void plot(int x, int y) {
    memcpy(image_buffer[y][x], &curr_color, sizeof(curr_color));
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

// Draw a box
void add_box(double sx, double sy, double sz, double rx, double ry, double rz, double mx, double my, double mz) {
    Mat4 *l_triangles = mat4_create(0);
    Mat4 *l_trans = mat4_create_identity();

    //Local transformations
    create_trans(sx, sy, sz, rx, ry, rz, mx, my, mz, l_trans);

    //Create vertices
    double p1[4] = {-0.5, 0.5, 0.5, 1.0};
    double p2[4] = {-0.5, -0.5, 0.5, 1.0};
    double p3[4] = {0.5, -0.5, 0.5, 1.0};
    double p4[4] = {0.5, 0.5, 0.5, 1.0};
    double p5[4] = {0.5, 0.5, -0.5, 1.0};
    double p6[4] = {0.5, -0.5, -0.5, 1.0};
    double p7[4] = {-0.5, -0.5, -0.5, 1.0};
    double p8[4] = {-0.5, 0.5, -0.5, 1.0};

    //Add triangles
    mat4_add_column(l_triangles, p1);
    mat4_add_column(l_triangles, p3);
    mat4_add_column(l_triangles, p2);

    mat4_add_column(l_triangles, p1);
    mat4_add_column(l_triangles, p4);
    mat4_add_column(l_triangles, p3);

    mat4_add_column(l_triangles, p4);
    mat4_add_column(l_triangles, p6);
    mat4_add_column(l_triangles, p3);

    mat4_add_column(l_triangles, p4);
    mat4_add_column(l_triangles, p5);
    mat4_add_column(l_triangles, p6);

    mat4_add_column(l_triangles, p5);
    mat4_add_column(l_triangles, p7);
    mat4_add_column(l_triangles, p6);

    mat4_add_column(l_triangles, p5);
    mat4_add_column(l_triangles, p8);
    mat4_add_column(l_triangles, p7);

    mat4_add_column(l_triangles, p8);
    mat4_add_column(l_triangles, p2);
    mat4_add_column(l_triangles, p7);

    mat4_add_column(l_triangles, p8);
    mat4_add_column(l_triangles, p1);
    mat4_add_column(l_triangles, p2);

    mat4_add_column(l_triangles, p8);
    mat4_add_column(l_triangles, p4);
    mat4_add_column(l_triangles, p1);

    mat4_add_column(l_triangles, p8);
    mat4_add_column(l_triangles, p5);
    mat4_add_column(l_triangles, p4);

    mat4_add_column(l_triangles, p2);
    mat4_add_column(l_triangles, p6);
    mat4_add_column(l_triangles, p7);

    mat4_add_column(l_triangles, p2);
    mat4_add_column(l_triangles, p3);
    mat4_add_column(l_triangles, p6);

    //Apply local transformations
    apply_trans(l_trans, l_triangles);
    apply_trans(trans_mat, l_triangles);

    //Combine local triangle matrix with current one
    mat4_combine(triangle_mat, l_triangles);
}

// Draw a sphere
void add_sphere(double sx, double sy, double sz, double rx, double ry, double rz, double mx, double my, double mz) {
    double phi, theta, r, x, z, rad = 1.0, inc = M_PI / SPH_REP;
    Mat4 *l_triangles = mat4_create(0);
    Mat4 *l_trans = mat4_create_identity();

    //Local transformations
    create_trans(sx, sy, sz, rx, ry, rz, mx, my, mz, l_trans);

    //Draw sphere
    for (phi = 0; phi < M_PI; phi += inc) {
        for (theta = 0; theta < 2.0 * M_PI; theta += inc) {
            if (phi == 0) {
                double p1[4] = {rad * sin(phi) * cos(theta), rad * sin(phi) * sin(theta), rad * cos(phi), 1.0};
                double p2[4] = {rad * sin(phi + inc) * cos(theta + inc), rad * sin(phi + inc) * sin(theta + inc), rad * cos(phi + inc), 1.0};
                double p3[4] = {rad * sin(phi + inc) * cos(theta), rad * sin(phi + inc) * sin(theta), rad * cos(phi + inc), 1.0};

                mat4_add_column(l_triangles, p1);
                mat4_add_column(l_triangles, p2);
                mat4_add_column(l_triangles, p3);
            //Not working
            } else if (phi + inc >= M_PI) {
                double p1[4] = {rad * sin(phi) * cos(theta), rad * sin(phi) * sin(theta), rad * cos(phi), 1.0};
                double p2[4] = {rad * sin(phi + inc) * cos(theta), rad * sin(phi + inc) * sin(theta), rad * cos(phi + inc), 1.0};
                double p3[4] = {rad * sin(phi) * cos(theta + inc), rad * sin(phi) * sin(theta + inc), rad * cos(phi), 1.0};

                mat4_add_column(l_triangles, p1);
                mat4_add_column(l_triangles, p2);
                mat4_add_column(l_triangles, p3);
            } else {
                double p1[4] = {rad * sin(phi) * cos(theta), rad * sin(phi) * sin(theta), rad * cos(phi), 1.0};
                double p2[4] = {rad * sin(phi) * cos(theta + inc), rad * sin(phi) * sin(theta + inc), rad * cos(phi), 1.0};
                double p3[4] = {rad * sin(phi + inc) * cos(theta + inc), rad * sin(phi + inc) * sin(theta + inc), rad * cos(phi + inc), 1.0};
                double p4[4] = {rad * sin(phi + inc) * cos(theta), rad * sin(phi + inc) * sin(theta), rad * cos(phi + inc), 1.0};

                mat4_add_column(l_triangles, p1);
                mat4_add_column(l_triangles, p2);
                mat4_add_column(l_triangles, p3);

                mat4_add_column(l_triangles, p1);
                mat4_add_column(l_triangles, p3);
                mat4_add_column(l_triangles, p4);
            }
        }
    }

    //Apply local transformations
    apply_trans(l_trans, l_triangles);
    apply_trans(trans_mat, l_triangles);

    //Combine local triangle matrix with current one
    mat4_combine(triangle_mat, l_triangles);
}

//Reset the transformation matrix
void identity() {
    mat4_delete(trans_mat);
    trans_mat = mat4_create_identity();
}

//Multiply the transformation matrix in to the triangle matrix
void transform() {
    apply_trans(trans_mat, triangle_mat);
}

Mat4 *t_move(double x, double y, double z) {
    Mat4 *mv = mat4_create_identity();
    mat4_set(mv, 0, 3, x);
    mat4_set(mv, 1, 3, y);
    mat4_set(mv, 2, 3, z);

    return mv;
}

Mat4 *t_scale(double sx, double sy, double sz) {
    Mat4 *sc = mat4_create_identity();
    mat4_set(sc, 0, 0, sx);
    mat4_set(sc, 1, 1, sy);
    mat4_set(sc, 2, 2, sz);

    return sc;
}

Mat4 *t_rotate_x(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 1, 1, cos(rads));
    mat4_set(rt, 1, 2, -sin(rads));
    mat4_set(rt, 2, 1, sin(rads));
    mat4_set(rt, 2, 2, cos(rads));

    return rt;
}

Mat4 *t_rotate_y(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 0, 0, cos(rads));
    mat4_set(rt, 0, 2, -sin(rads));
    mat4_set(rt, 2, 0, sin(rads));
    mat4_set(rt, 2, 2, cos(rads));

    return rt;
}

Mat4 *t_rotate_z(double rads) {
    Mat4 *rt = mat4_create_identity();
    mat4_set(rt, 0, 0, cos(rads));
    mat4_set(rt, 0, 1, -sin(rads));
    mat4_set(rt, 1, 0, sin(rads));
    mat4_set(rt, 1, 1, cos(rads));

    return rt;
}

//From stack overflow
void rtrim(char *s) {
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back + 1) = '\0';
}

//Separate file into lines
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

//Read numbers from string
void read_nums(double nums[], char **line) {
    int len, i;
    len = parse_numwords(line);

    for (i = 0; i < len - 1; i++) {
        sscanf(line[i + 1], "%lf", &nums[i]);
    }
}
