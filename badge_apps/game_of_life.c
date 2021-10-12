/*********************************************

This is a badge app template.


**********************************************/
#ifdef __linux__
#include <stdio.h>
#include <sys/time.h> /* for gettimeofday */
#include <string.h>	  /* for memset */

#include "../linux/linuxcompat.h"
#include "../linux/bline.h"
#else
#include "colors.h"
#include "menu.h"
#include "buttons.h"

/* TODO: I shouldn't have to declare these myself. */
#define size_t int
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcat(char *dest, const char *src);

#endif

#include "build_bug_on.h"
#include "xorshift.h"

static int screen_changed = 0;

#define GRID_SIZE 100
#define ROW_SIZE 10
#define COL_SIZE 10

#define GRID_PADDING 3
#define CELL_SIZE 20
#define CELL_PADDING 0

#define STARTX(x) (GRID_PADDING + CELL_PADDING + ((x)*CELL_SIZE))
#define ENDX(x) (GRID_PADDING + CELL_SIZE + ((x)*CELL_SIZE))
#define STARTY(y) (GRID_PADDING + CELL_PADDING + ((y)*CELL_SIZE))
#define ENDY(y) (GRID_PADDING + CELL_SIZE + ((y)*CELL_SIZE))

static struct Grid
{
	struct Cell
	{
		unsigned char alive;
		struct Coordinate
		{
			unsigned int x;
			unsigned int y;
		} coordinate;
	} cells[GRID_SIZE];
} grid;

/* Program states.  Initial state is GAME_OF_LIFE_INIT */
enum game_of_life_state_t
{
	GAME_OF_LIFE_INIT,
	GAME_OF_LIFE_SPLASH_SCREEN,
	GAME_OF_LIFE_RUN,
	GAME_OF_LIFE_EXIT,
};

static enum game_of_life_state_t game_of_life_state = GAME_OF_LIFE_INIT;

static int is_next_row(int current_cell_index)
{
	return (current_cell_index + 1) % ROW_SIZE == 0;
}

static void init_grid()
{
	unsigned int x = 0;
	unsigned int y = 0;

	for (int i = 0; i < GRID_SIZE; i++)
	{
		grid.cells[i].alive = xorshift((unsigned int *)&timestamp) % 2;

		if (!is_next_row(i))
		{
			grid.cells[i].coordinate.x = x;
			grid.cells[i].coordinate.y = y;
			y++;
		}

		if (is_next_row(i))
		{
			grid.cells[i].coordinate.x = x;
			grid.cells[i].coordinate.y = y;
			x++;
			// reset before going to the next row
			y = 0;
		}
	}
}

static void render_box(int grid_x, int grid_y, int color)
{
	FbColor(color);
	FbHorizontalLine(STARTX(grid_x), STARTY(grid_y), ENDX(grid_x), STARTY(grid_y));
	FbHorizontalLine(STARTX(grid_x), ENDY(grid_y), ENDX(grid_x), ENDY(grid_y));
	FbVerticalLine(STARTX(grid_x), STARTY(grid_y), STARTX(grid_x), ENDY(grid_y));
	FbVerticalLine(ENDX(grid_x), STARTY(grid_y), ENDX(grid_x), ENDY(grid_y));
}

static void render_cell(int grid_x, int grid_y)
{
	render_box(grid_x, grid_y, WHITE);
}

static void render_grid()
{

	for (int row = 0; row <= 6 - 1; row++)
	{

		for (int col = 0; col < 6 - 1; col++)
		{
			render_cell(row, col);
		}
	}
}

static void debug_print()
{
	for (int i = 0; i < GRID_SIZE; i++)
	{
		printf("%d %d %d \n", grid.cells[i].coordinate.x, grid.cells[i].coordinate.y, grid.cells[i].alive);
	}
}

static void render_game()
{
	// if (!screen_changed)
	// {
	// 	return;
	// }

	// FbClear();

	render_grid();

	screen_changed = 0;
}

static void check_buttons()
{
	// if (BUTTON_PRESSED_AND_CONSUME)
	// {
	// 	screen_changed = 1;
	// }
	// else if (LEFT_BTN_AND_CONSUME)
	// {
	// 	screen_changed = 1;
	// }
	// else if (RIGHT_BTN_AND_CONSUME)
	// {
	// 	screen_changed = 1;
	// }
	// else if (UP_BTN_AND_CONSUME)
	// {
	// 	screen_changed = 1;
	// }
	// else if (DOWN_BTN_AND_CONSUME)
	// {
	// 	screen_changed = 1;
	// }
	FbColor(WHITE);
	FbClear();
	render_game();
}

static void render_splash_screen()
{
	FbColor(WHITE);
	FbMove(10, LCD_YSIZE / 2);
	FbWriteLine("Game of Life");
	FbSwapBuffers();
}

static void game_of_life_init(void)
{
#ifdef __linux__
	FbInit();
	FbClear();

	game_of_life_state = GAME_OF_LIFE_SPLASH_SCREEN;
	printf("grid size %d row size: %d\n", GRID_SIZE, ROW_SIZE);
#endif
}

static void game_of_life_splash_screen()
{
	render_splash_screen();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		init_grid();
		game_of_life_state = GAME_OF_LIFE_RUN;
	}
}

static void game_of_life_run()
{
	check_buttons();
}

static void game_of_life_exit()
{
	game_of_life_state = GAME_OF_LIFE_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

static int game_of_life_cb(void)
{
	switch (game_of_life_state)
	{
	case GAME_OF_LIFE_INIT:
		game_of_life_init();
		break;
	case GAME_OF_LIFE_SPLASH_SCREEN:
		game_of_life_splash_screen();
		break;
	case GAME_OF_LIFE_RUN:
		game_of_life_run();
		break;
	case GAME_OF_LIFE_EXIT:
		game_of_life_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
	start_gtk(&argc, &argv, game_of_life_cb, 30);
	return 0;
}
#endif
