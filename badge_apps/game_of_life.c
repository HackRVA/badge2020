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

#define GRID_SIZE 100
#define ROW_SIZE 10
#define COL_SIZE 10

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

static void debug_print()
{
	for (int i = 0; i < GRID_SIZE; i++)
	{
		printf("coordinate %d %d", grid.cells[i].coordinate.x, grid.cells[i].coordinate.y);
	}
}

static void game_of_life_init(void)
{
#ifdef __linux__
	printf("grid size %d\nrow size: %d\n\n", GRID_SIZE, ROW_SIZE);
#endif
	FbInit();
	FbClear();
	game_of_life_state = GAME_OF_LIFE_RUN;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME)
	{
		init_grid();
		/* Pressing the button exits the program. You probably want to change this. */
		game_of_life_state = GAME_OF_LIFE_EXIT;
	}
	else if (LEFT_BTN_AND_CONSUME)
	{
	}
	else if (RIGHT_BTN_AND_CONSUME)
	{
	}
	else if (UP_BTN_AND_CONSUME)
	{
	}
	else if (DOWN_BTN_AND_CONSUME)
	{
	}
}

static void draw_screen()
{
	FbColor(WHITE);
	FbMove(10, 40 / 2);
	FbWriteLine("Game of Life");
	FbSwapBuffers();
}

static void game_of_life_run()
{
	check_buttons();
	draw_screen();
}

static void game_of_life_exit()
{
	game_of_life_state = GAME_OF_LIFE_INIT; /* So that when we start again, we do not immediately exit */
	debug_print();
	returnToMenus();
}

static

	int
	game_of_life_cb(void)
{
	switch (game_of_life_state)
	{
	case GAME_OF_LIFE_INIT:
		game_of_life_init();
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
