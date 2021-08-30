#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

//--------------------------------------------------------
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

typedef unsigned char byte;

//--------------------------------------------------------
int clamp(int val, int min, int max) {
  return (val >= min) ? (val <= max) ? val : max : min;
}
float fclamp(float val, float min, float max) {
  return (val >= min) ? (val <= max) ? val : max : min;
}

void SDL_SetRenderDrawColorFromColor(SDL_Renderer *ren, SDL_Color *col) {
  SDL_SetRenderDrawColor(ren, col->r, col->g, col->b, col->a);
}

//--------------------------------------------------------

#define NUM_COLORS 16
#define FONT_SIZE_X 16
#define FONT_SIZE_Y 16

SDL_Window *win;
SDL_Renderer *ren;
SDL_Texture *font_tex, *grid_tex;
byte *grid;
int width = 32, height = 10, screen_width = 32 * 20, screen_height = 32 * 20;
bool quit_flag = false;
float zoom = 20;
char *filename = NULL;
char *font = "img/font.png";
const Uint8 *kbd;
enum { MODE_HEX, MODE_COL, MODE_NUM } mode = MODE_COL;
SDL_Color colors[NUM_COLORS] = {
    {0, 0, 0}, {0xff, 0xff, 0xff}, {0xff, 0, 0}, {0, 0xff, 0}, {0, 0, 0xff}};
struct {
  int x, y, w, h;
  int sx, sy, sw, sh;
  bool select_flag;
} cursor = {.x = 0,
            .y = 0,
            .w = 1,
            .h = 1,
            .sx = 0,
            .sy = 0,
            .sw = 1,
            .sh = 1,
            .select_flag = false};

//--------------------------------------------------------
int init();
int quit();
void parse_cmd(int argc, char **argv);
void update();
void draw();
void resize_screen(int w, int h);
void events();
void keydown(SDL_KeyCode k);
void keyup(SDL_KeyCode k);

void cursor_move(int dx, int dy);
void cursor_fill(byte v);

// grid
void resize_grid(int w, int h);
void grid_add(int x, int y, int d);
void grid_add_rect(int x, int y, int w, int h, int d);

// drawing
void draw_thick_rect(int x, int y, int w, int h, int t);
void draw_char(int x, int y, byte i);
void draw_col(int x, int y, byte i);
void draw_cursor();
void draw_grid();

// file
void save_as_grd();
void load_from_grd();
