#include "grid.h"

//--------------------------------------------------------
int init() {

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("[error] sdl_init failed: %s\n", SDL_GetError());
    return 1;
  }

  win = SDL_CreateWindow("grid", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         screen_width, screen_height, SDL_WINDOW_SHOWN);
  if (!win) {
    printf("[error] create_window failed: %s\n", SDL_GetError());
    return 1;
  }

  if (filename) {
    char *fmt = "grid - %s";
    size_t filename_len = strlen(filename);
    size_t fmt_len = strlen(fmt);
    char str[fmt_len + filename_len];
    sprintf(str, fmt, filename);
    SDL_SetWindowTitle(win, str);
  }

  ren = SDL_CreateRenderer(win, -1, 0);
  if (!ren) {
    printf("[error] create_renderer failed: %s\n", SDL_GetError());
    return 1;
  }

  kbd = SDL_GetKeyboardState(NULL);

  zoom = min(screen_width / width, screen_height / height) * 0.75;

  resize_grid(width, height);

  load_font();

  load_from_grd();

  load_palette();

  return 0;
}

//--------------------------------------------------------
int quit() {
  free(grid);

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}

//--------------------------------------------------------
void parse_args(int argc, char **argv) {
  int opt = -1;

  while ((opt = getopt(argc, argv, "W:H:o:p:f:zh")) != -1) {
    switch (opt) {

    case 'W':
      width = atoi(optarg);
      if (width < 1)
        width = 1;
      break;

    case 'H':
      height = atoi(optarg);
      if (height < 1)
        height = 1;
      break;

    case 'o':
      filename = optarg;
      break;

    case 'p':
      palette = optarg;
      break;

    case 'f':
      font = optarg;
      break;

    case 'z':
      ignore_zero_flag = true;
      break;

    case 'h': {
      printf("\n[hotkeys]\n");
      printf("escape: quits program\n");
      printf("left/right/up/down: move left/right/up/down\n");
      printf("z: decrease value\n");
      printf("x: increase value\n");
      printf("0-9,A-F: set value\n");
      printf("v: toggle select mode\n");
      printf("i: zoom in\n");
      printf("o: zoom out\n");
      printf("m: toggle view mode\n");
      printf("S: save as .grd\n");
      printf("L: load from .grd\n");
      printf("\n");
    } break;

    default: {
      printf("try:\n");
      printf("\t-h: print [h]otkeys\n");
      printf("\t-W width: set [W]idth\n");
      printf("\t-H height: set [H]eight\n");
      printf("\t-o filename: set file to [o]pen\n");
      printf("\t-p palette: set [p]alette to use\n");
      printf("\t-f font: set [f]ont to use\n");
      printf("\t-z: don't draw [z]eros\n");
      printf("\n");
    } break;
    }
  }
}

//--------------------------------------------------------
void update() {
  events();
  draw();
}

//--------------------------------------------------------
void draw() {
  SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderClear(ren);

  SDL_SetRenderTarget(ren, grid_tex);
  SDL_RenderClear(ren);

  draw_grid();
  draw_cursor();

  SDL_SetRenderTarget(ren, NULL);

  float zw = width * zoom;
  float zh = height * zoom;
  float zx = ((float)screen_width - zw) / 2.0;
  float zy = ((float)screen_height - zh) / 2.0;

  SDL_FRect dest = {.x = zx, .y = zy, .w = zw, .h = zh};
  SDL_RenderCopyF(ren, grid_tex, NULL, &dest);

  SDL_RenderPresent(ren);
}

//--------------------------------------------------------
void resize_screen(int w, int h) {}

//--------------------------------------------------------
void events() {
  SDL_Event evt;

  while (SDL_PollEvent(&evt)) {
    switch (evt.type) {
    case SDL_QUIT:
      quit_flag = true;
      break;

    case SDL_KEYDOWN:
      keydown(evt.key.keysym.sym);
      break;

    case SDL_KEYUP:
      keyup(evt.key.keysym.sym);
      break;

    case SDL_WINDOWEVENT:
      if (evt.window.event == SDL_WINDOWEVENT_RESIZED)
        resize_screen(evt.window.data1, evt.window.data2);
      break;
    }
  }
}

//--------------------------------------------------------
void keydown(SDL_KeyCode k) {
  switch (k) {

  case SDLK_ESCAPE: {
    SDL_Event q;
    q.type = SDL_QUIT;
    SDL_PushEvent(&q);
  } break;

  case 'i': {
    zoom += 4;
  } break;

  case 'o': {
    zoom -= 4;
    if (zoom < 4)
      zoom = 4;
  } break;

  case SDLK_LEFT:
    cursor_move(-1, 0);
    break;

  case SDLK_RIGHT:
    cursor_move(1, 0);
    break;

  case SDLK_UP:
    cursor_move(0, -1);
    break;

  case SDLK_DOWN:
    cursor_move(0, 1);
    break;

  case 'z': {
    if (cursor.select_flag) {
      grid_add_rect(min(cursor.x, cursor.sx), min(cursor.y, cursor.sy),
                    abs(cursor.x - cursor.sx) + 1,
                    abs(cursor.y - cursor.sy) + 1, -1);
    } else {
      grid_add(cursor.x, cursor.y, -1);
    }
    draw_key_held_flag = true;
  } break;

  case 'x': {
    if (cursor.select_flag) {
      grid_add_rect(min(cursor.x, cursor.sx), min(cursor.y, cursor.sy),
                    abs(cursor.x - cursor.sx) + 1,
                    abs(cursor.y - cursor.sy) + 1, 1);
    } else {
      grid_add(cursor.x, cursor.y, 1);
    }
    draw_key_held_flag = true;
  } break;

  case 'v': {
    cursor.select_flag ^= true;
    if (cursor.select_flag) {
      cursor.sx = cursor.x;
      cursor.sy = cursor.y;
    }
  } break;

  case 's': {
    if (SDL_GetModState() & KMOD_SHIFT)
      save_as_grd();
  } break;

  case 'l': {
    if (SDL_GetModState() & KMOD_SHIFT)
      load_from_grd();
  } break;

  case 'm': {
    mode = (mode + 1) % MODE_NUM;
  } break;

  case '0':
    cursor_fill(0);
    draw_key_held_flag = true;
    break;
  case '1':
    cursor_fill(1);
    draw_key_held_flag = true;
    break;
  case '2':
    cursor_fill(2);
    draw_key_held_flag = true;
    break;
  case '3':
    cursor_fill(3);
    draw_key_held_flag = true;
    break;
  case '4':
    cursor_fill(4);
    draw_key_held_flag = true;
    break;
  case '5':
    cursor_fill(5);
    draw_key_held_flag = true;
    break;
  case '6':
    cursor_fill(6);
    draw_key_held_flag = true;
    break;
  case '7':
    cursor_fill(7);
    draw_key_held_flag = true;
    break;
  case '8':
    cursor_fill(8);
    draw_key_held_flag = true;
    break;
  case '9':
    cursor_fill(9);
    draw_key_held_flag = true;
    break;
  case 'a':
    cursor_fill(10);
    draw_key_held_flag = true;
    break;
  case 'b':
    cursor_fill(11);
    draw_key_held_flag = true;
    break;
  case 'c':
    cursor_fill(12);
    draw_key_held_flag = true;
    break;
  case 'd':
    cursor_fill(13);
    draw_key_held_flag = true;
    break;
  case 'e':
    cursor_fill(14);
    draw_key_held_flag = true;
    break;
  case 'f':
    cursor_fill(15);
    draw_key_held_flag = true;
    break;

  default:
    break;
  }
}

//--------------------------------------------------------
void keyup(SDL_KeyCode k) {
  switch (k) {

  case 'z':
  case 'x':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    draw_key_held_flag = false;
    break;

  default:
    break;
  }
}

//--------------------------------------------------------
void cursor_move(int dx, int dy) {
  SDL_Keymod kmod = SDL_GetModState();

  if (dx) {
    if (kmod & KMOD_SHIFT) {
      cursor.x = dx < 0 ? 0 : width - 1;
    } else if (kmod & KMOD_CTRL) {
      int c = grid[cursor.x + cursor.y * width];
      if (dx > 0) {
        for (int i = cursor.x; i < width; i++) {
          if (grid[i + cursor.y * width] != c) {
            cursor.x = i;
            break;
          }
        }
      } else {
        for (int i = cursor.x; i >= 0; i--) {
          if (grid[i + cursor.y * width] != c) {
            cursor.x = i;
            break;
          }
        }
      }
    } else {
      cursor.x += dx;
      if (dx > 0 && cursor.x > width - 1)
        cursor.x = 0;
      else if (dx < 0 && cursor.x < 0)
        cursor.x = width - 1;
    }
  }

  if (dy) {
    if (kmod & KMOD_SHIFT) {
      cursor.y = dy < 0 ? 0 : height - 1;
    } else if (kmod & KMOD_CTRL) {
      int c = grid[cursor.x + cursor.y * width];
      if (dy > 0) {
        for (int i = cursor.y; i < height; i++) {
          if (grid[cursor.x + i * width] != c) {
            cursor.y = i;
            break;
          }
        }
      } else {
        for (int i = cursor.y; i >= 0; i--) {
          if (grid[cursor.x + i * width] != c) {
            cursor.y = i;
            break;
          }
        }
      }
    } else {
      cursor.y += dy;
      if (dy > 0 && cursor.y > height - 1)
        cursor.y = 0;
      else if (dy < 0 && cursor.y < 0)
        cursor.y = height - 1;
    }
  }

  if (!draw_key_held_flag)
    return;

  if (kbd[SDL_SCANCODE_Z]) {
    if (cursor.select_flag) {
      grid_add_rect(min(cursor.x, cursor.sx), min(cursor.y, cursor.sy),
                    abs(cursor.x - cursor.sx) + 1,
                    abs(cursor.y - cursor.sy) + 1, -1);
    } else {
      grid_add(cursor.x, cursor.y, -1);
    }
  } else if (kbd[SDL_SCANCODE_X]) {
    if (cursor.select_flag) {
      grid_add_rect(min(cursor.x, cursor.sx), min(cursor.y, cursor.sy),
                    abs(cursor.x - cursor.sx) + 1,
                    abs(cursor.y - cursor.sy) + 1, 1);
    } else {
      grid_add(cursor.x, cursor.y, 1);
    }
  } else if (kbd[SDL_SCANCODE_0]) {
    cursor_fill(0);
  } else if (kbd[SDL_SCANCODE_1]) {
    cursor_fill(1);
  } else if (kbd[SDL_SCANCODE_2]) {
    cursor_fill(2);
  } else if (kbd[SDL_SCANCODE_3]) {
    cursor_fill(3);
  } else if (kbd[SDL_SCANCODE_4]) {
    cursor_fill(4);
  } else if (kbd[SDL_SCANCODE_5]) {
    cursor_fill(5);
  } else if (kbd[SDL_SCANCODE_6]) {
    cursor_fill(6);
  } else if (kbd[SDL_SCANCODE_7]) {
    cursor_fill(7);
  } else if (kbd[SDL_SCANCODE_8]) {
    cursor_fill(8);
  } else if (kbd[SDL_SCANCODE_9]) {
    cursor_fill(9);
  } else if (kbd[SDL_SCANCODE_A]) {
    cursor_fill(10);
  } else if (kbd[SDL_SCANCODE_B]) {
    cursor_fill(11);
  } else if (kbd[SDL_SCANCODE_C]) {
    cursor_fill(12);
  } else if (kbd[SDL_SCANCODE_D]) {
    cursor_fill(13);
  } else if (kbd[SDL_SCANCODE_E]) {
    cursor_fill(14);
  } else if (kbd[SDL_SCANCODE_F]) {
    cursor_fill(15);
  }
}

//--------------------------------------------------------
void cursor_fill(byte v) {
  if (cursor.select_flag) {
    for (int i = 0; i < abs(cursor.x - cursor.sx) + 1; ++i)
      for (int j = 0; j < abs(cursor.y - cursor.sy) + 1; ++j)
        grid[(i + min(cursor.x, cursor.sx)) +
             (j + min(cursor.y, cursor.sy)) * width] = v;
  } else {
    grid[cursor.x + cursor.y * width] = v;
  }
}

//--------------------------------------------------------
void resize_grid(int w, int h) {
  width = w > 0 ? w : 1;
  height = h > 0 ? h : 1;

  free(grid);
  if (grid_tex)
    SDL_DestroyTexture(grid_tex);

  grid_tex =
      SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                        width * FONT_SIZE_X, height * FONT_SIZE_Y);
  if (!grid_tex) {
    printf("[error] create_texture failed (grid_tex): %s\n", SDL_GetError());
    quit_flag = true;
    return;
  }

  grid = calloc(width * height, sizeof(byte));
  memset(grid, 0, width * height);
}

//--------------------------------------------------------
void grid_add(int x, int y, int d) {
  if (d) {
    int i = (x + y * width) % (width * height);
    if (d < 0 && grid[i] == 0)
      grid[i] = 15;
    else if (d > 0 && grid[i] == 15)
      grid[i] = 0;
    else
      grid[i] += d > 0 ? 1 : -1;
  }
}

//--------------------------------------------------------
void grid_add_rect(int x, int y, int w, int h, int d) {
  for (int i = x; i < min(width, x + w); ++i)
    for (int j = y; j < min(height, y + h); ++j)
      grid_add(i, j, d);
}

//--------------------------------------------------------
// drawing
//--------------------------------------------------------
void draw_thick_rect(int x, int y, int w, int h, int t) {
  Uint8 r = 0;
  Uint8 g = 0;
  Uint8 b = 0;
  Uint8 a = 0;
  SDL_GetRenderDrawColor(ren, &r, &g, &b, &a);

  thickLineRGBA(ren, x, y, x + w, y, t, r, g, b, a);
  thickLineRGBA(ren, x + w, y, x + w, y + h, t, r, g, b, a);
  thickLineRGBA(ren, x + w, y + h, x, y + h, t, r, g, b, a);
  thickLineRGBA(ren, x, y + h, x, y, t, r, g, b, a);
}

//--------------------------------------------------------
void draw_char(int x, int y, byte i) {
  int sx = i % 4;
  int sy = floor((float)i / 4);

  SDL_Rect src = {.x = sx * FONT_SIZE_X,
                  .y = sy * FONT_SIZE_Y,
                  .w = FONT_SIZE_X,
                  .h = FONT_SIZE_Y};

  SDL_Rect dest = {.x = x * FONT_SIZE_X,
                   .y = y * FONT_SIZE_Y,
                   .w = FONT_SIZE_X,
                   .h = FONT_SIZE_Y};

  SDL_RenderCopy(ren, font_tex, &src, &dest);
}

//--------------------------------------------------------
void draw_col(int x, int y, byte i) {
  SDL_Rect rect = {.x = x * FONT_SIZE_X,
                   .y = y * FONT_SIZE_Y,
                   .w = FONT_SIZE_X,
                   .h = FONT_SIZE_Y};

  SDL_SetRenderDrawColorFromColor(ren, &colors[i % num_colors]);
  SDL_RenderFillRect(ren, &rect);
}

//--------------------------------------------------------
void draw_grid() {
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      byte i = grid[x + y * width];

      switch (mode) {

      case MODE_HEX:
        if (i == 0 && ignore_zero_flag) {
          draw_col(x, y, 0);
        } else {
          draw_char(x, y, i);
        }
        break;

      case MODE_COL:
        draw_col(x, y, i);
        break;

      default:
        break;
      }
    }
  }
}

//--------------------------------------------------------
void draw_cursor() {
  switch (mode) {

  case MODE_HEX:
    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0xff, 0xff);
    break;

  case MODE_COL: {
    SDL_Color *col = &colors[grid[cursor.x + cursor.y * width] % num_colors];
    SDL_SetRenderDrawColor(ren, 0xff - col->r, 0xff - col->g, 0xff - col->b,
                           0xff);
  } break;

  default:
    break;
  }

  if (cursor.select_flag) {
    draw_thick_rect(min(cursor.x, cursor.sx) * FONT_SIZE_X,
                    min(cursor.y, cursor.sy) * FONT_SIZE_Y,
                    (abs(cursor.x - cursor.sx) + 1) * FONT_SIZE_X,
                    (abs(cursor.y - cursor.sy) + 1) * FONT_SIZE_Y, 2);
  }

  draw_thick_rect(cursor.x * FONT_SIZE_X, cursor.y * FONT_SIZE_Y,
                  cursor.w * FONT_SIZE_X, cursor.h * FONT_SIZE_Y,
                  draw_key_held_flag ? 5 : 3);
}

//--------------------------------------------------------
void load_font() {
  if (font_tex)
    SDL_DestroyTexture(font_tex);

  font_tex = IMG_LoadTexture(ren, font);
  if (!font_tex) {
    printf("[error] LoadTexture failed (font): %s\n", SDL_GetError());
    return;
  }
}

//--------------------------------------------------------
void save_as_grd() {
  FILE *file = fopen(filename, "wb");

  if (file) {
    byte output[2 + width * height];

    output[0] = width;
    output[1] = height;

    for (int i = 0; i < width * height; ++i)
      output[2 + i] = grid[i];

    fwrite(output, sizeof(output), 1, file);

    printf("saved as %s\n", filename);
    fclose(file);
  } else {
    printf("[error] failed to save %s\n", filename);
  }
}

//--------------------------------------------------------
void load_from_grd() {
  FILE *file = fopen(filename, "rb");

  if (file) {
    fseek(file, 0, SEEK_END);
    long sz = ftell(file);
    rewind(file);

    byte *buf = calloc(sz, sizeof(byte));
    fread(buf, 1, sz, file);

    int width = buf[0];
    int height = buf[1];
    resize_grid(width, height);

    for (int i = 0; i < width * height; ++i)
      grid[i] = buf[2 + i];

    printf("loaded from %s\n", filename);
    free(buf);
    fclose(file);
  } else {
    printf("[error] failed to open %s\n", filename);
  }
}

//--------------------------------------------------------
void load_palette() {
  FILE *file = fopen(palette, "r");

  if (file) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    byte i = 0;

    while ((read = getline(&line, &len, file)) != -1) {
      if (i > MAX_COLORS)
        break;

      Uint32 col = strtol(line, NULL, 16);
      colors[i].r = (col >> 16) & 0xff;
      colors[i].g = (col >> 8) & 0xff;
      colors[i].b = col & 0xff;

      ++i;
    }

    num_colors = i;

    if (line)
      free(line);
  } else {
    printf("[error] failed to open palette %s\n", palette);
  }
}

//--------------------------------------------------------
//--------------------------------------------------------
//--------------------------------------------------------
int main(int argc, char **argv) {
  srand(time(NULL));

  parse_args(argc, argv);

  if (!init()) {
    while (!quit_flag) {
      double st = SDL_GetPerformanceCounter();
      update();
      double et = (SDL_GetPerformanceCounter() - st) /
                  (double)SDL_GetPerformanceFrequency() * 1000.0;

      SDL_Delay(fclamp(16.666 - et, 0, 1000));
    }
  }

  return quit();
}
