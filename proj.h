//Constants
#define MAX_CMDS 500
#define MAX_CHARS 128
#define COLOR_MAX 255
#define SPH_REP 18.0

//Macros
#define apply_trans(t, m) \
    new_m = mat4_multiply(t, m); \
    mat4_delete(m); \
    m = new_m;

#define create_trans(sx, sy, sz, rx, ry, rz, mx, my, mz, l_trans) \
    apply_trans(t_scale(sx, sy, sz), l_trans); \
    apply_trans(t_rotate_x(rx), l_trans); \
    apply_trans(t_rotate_y(ry), l_trans); \
    apply_trans(t_rotate_z(rz), l_trans); \
    apply_trans(t_move(mx, my, mz), l_trans);

#define print_triangle() mat4_print("Triangle Matrix", triangle_mat);
#define print_trans() mat4_print("Tranformation Matrix", trans_mat);
#define swap(j, k, temp) temp = j; j = k; k = temp;

//Primary functions
int main(int argc, char *argv[]);
void run();
void init();
void free_ptrs();
void clear_buffer();
void write_headers();
void write_buffer();

//Calculation heavy rendering functions
void convert_wld();
void render_pll(char color[3]);
void render_eye(double ex, double ey, double ez, char color[3]);

//Math functions
void cross_product(double u[3], double v[3], double r[]);
double dot_product(double *u, double *v);

//Drawing functions
void plot(int x, int y);
void draw_line(int x0, int y0, int x1, int y1);

//Drawing functions for specific shapes
void add_box(double sx, double sy, double sz, double ry, double rx, double rz, double mx, double my, double mz);
void add_sphere(double sx, double sy, double sz, double rx, double ry, double rz, double mx, double my, double mz);

//Matrix functions
void identity();
void transform();
Mat4 *t_move(double x, double y, double z);
Mat4 *t_scale(double sx, double sy, double sz);
Mat4 *t_rotate_x(double rads);
Mat4 *t_rotate_y(double rads);
Mat4 *t_rotate_z(double rads);

//Input functions
void rtrim(char *s);
int read_data(char data[][MAX_CHARS]);
void read_nums(double nums[], char **line);
