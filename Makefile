all: grid

grid: src/grid.c src/grid.h
	gcc -o grid src/grid.c -lSDL2 -lSDL2_image -lSDL2_gfx -lm
