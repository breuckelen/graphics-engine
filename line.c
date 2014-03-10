#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//Constants
#define MAX_CMDS 500
#define MAX_CHARS 128
#define IMAGE_WIDTH 500
#define IMAGE_HEIGHT 500
#define COLOR_MAX 255

//Macros
#define swap(j, k, temp) temp = j; j = k; k = temp;

//Input and output files
FILE *i_file;
FILE *o_file;

//Current color and 2D array for all pixels
unsigned char curr_color[3];
unsigned char image_buffer[IMAGE_HEIGHT][IMAGE_WIDTH][3];

//Change to write to o_file
void plot(int x, int y) {
    memcpy(&image_buffer[y][x], &curr_color, sizeof(curr_color));
}

//Bresenham's line algorithm for all octants
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

//Removes whitespace at the end of strings
//From stack overflow
void rtrim(char *s) {
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back + 1) = '\0';
}

//Read from i_file and give back array with instructions
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

//Run commands from data
void gen_image() {
    int n, i, j, k, points[4];
    char data[MAX_CMDS][MAX_CHARS];
    n = read_data(&data);

    for (i = 0; i < n; i++) {
        switch(data[i][0]) {
            case 'c':
                i++;
                sscanf(data[i], "%d %d %d", &curr_color[0], &curr_color[1], &curr_color[2]);
                break;

            case 'l':
                i++;
                sscanf(data[i], "%d %d %d %d", &points[0], &points[1], &points[2], &points[3]);
                draw_line(points[0], points[1], points[2], points[3]);
                break;

            case 'g':
                i++;

                if ((o_file = fopen(data[i], "ab")) == NULL) {
                    perror("Could not open output file\n");
                    exit(1);
                }

                break;

            default:
                break;
        }
    }

    //Headers
    fprintf(o_file, "P3\n%d %d\n%d\n", IMAGE_WIDTH, IMAGE_HEIGHT, COLOR_MAX);

    //Write data
    for (j = 0; j < IMAGE_HEIGHT; j++)
        for (k = 0; k < IMAGE_WIDTH; k++)
            fprintf(o_file, "%d %d %d ", image_buffer[j][k][0], image_buffer[j][k][1], image_buffer[j][k][2]);
}

int main(int argc, char *argv[]) {
    //Set i_file
    if ((i_file = fopen(argv[1], "r")) == NULL) {
        perror("Could not open input file\n");
        exit(1);
    }

    //Set default color and populate pixels with the color
    curr_color[0] = 0; curr_color[1] = 0; curr_color[2] = 0;

    int j, k;
    for (j = 0; j < IMAGE_HEIGHT; j++)
        for (k = 0; k < IMAGE_WIDTH; k++)
            memcpy(&image_buffer[j][k], &curr_color, sizeof(curr_color));

    //Generate the image
    gen_image();

    fclose(o_file);
    fclose(i_file);

    return 0;
}
