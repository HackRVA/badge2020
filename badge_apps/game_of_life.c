/*********************************************

An implementation of Conway's Game of Life

 Author: Paul Chang <paulc1231@gmail.com>
 (c) 2021 Paul Chang
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
#include "timer1_int.h" /* for wclock */

/* TODO: I shouldn't have to declare these myself. */
#define size_t int
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcat(char *dest, const char *src);

#endif

#include "build_bug_on.h"
#include "xorshift.h"

static unsigned int gen_count = 0;
static volatile int current_time;
static volatile int last_time;

#define ROW_SIZE 10
#define COL_SIZE 10
#define GRID_SIZE (ROW_SIZE * COL_SIZE)

#define GRID_X_PADDING 8
#define GRID_Y_PADDING 2
#define CELL_SIZE 12
#define CELL_PADDING 3

#define ALIVE 1
#define DEAD 0

#define TRUE 1
#define FALSE 0

#define STARTX(x) (GRID_X_PADDING + CELL_PADDING + ((x)*CELL_SIZE))
#define ENDX(x) (GRID_X_PADDING + CELL_SIZE + ((x)*CELL_SIZE))
#define STARTY(y) (GRID_Y_PADDING + CELL_PADDING + ((y)*CELL_SIZE))
#define ENDY(y) (GRID_Y_PADDING + CELL_SIZE + ((y)*CELL_SIZE))

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
} grid, next_generation_grid;

/* Program states.  Initial state is GAME_OF_LIFE_INIT */
enum game_of_life_state_t
{
	GAME_OF_LIFE_INIT,
	GAME_OF_LIFE_SPLASH_SCREEN,
	GAME_OF_LIFE_RUN,
	GAME_OF_LIFE_EXIT,
};

enum game_of_life_cmd_t
{
	CMD_PAUSE,
	CMD_RESUME
};

static enum game_of_life_state_t game_of_life_state = GAME_OF_LIFE_INIT;
static enum game_of_life_cmd_t game_of_life_cmd = CMD_RESUME;

static int is_next_row(int current_cell_index)
{
	return (current_cell_index + 1) % ROW_SIZE == 0;
}

static int is_in_range(int current_index)
{
	if (current_index >= 0 && current_index < GRID_SIZE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void next_generation(unsigned int alive_count, unsigned int cell, int current_index)
{
	if (cell == ALIVE)
	{
		if (alive_count == 2 || alive_count == 3)
		{
			next_generation_grid.cells[current_index].alive = ALIVE;
		}
		else
		{
			next_generation_grid.cells[current_index].alive = DEAD;
		}
	}
	else
	{
		if (alive_count == 3)
		{ // only way a dead cell can be revived if it has exactly 3 alive neighbors
			next_generation_grid.cells[current_index].alive = ALIVE;
		}
		else
		{
			next_generation_grid.cells[current_index].alive = DEAD;
		}
	}
}

static void update_current_generation_grid()
{
	for (int i = 0; i < GRID_SIZE; i++)
	{
		grid.cells[i].alive = next_generation_grid.cells[i].alive;
	}
}

static void figure_out_alive_cells()
{
	unsigned int alive_count = 0;

	for (int n = 0; n < GRID_SIZE; n++)
	{
		// LEFT neighbor
		if (is_in_range(n - 1))
		{

			// LEFT not out of bound cond
			if (grid.cells[n - 1].coordinate.x == grid.cells[n].coordinate.x)
			{
				if (grid.cells[n - 1].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// RIGHT neighbor
		if (is_in_range(n + 1))
		{
			// RIGHT not out of bound cond
			if (grid.cells[n + 1].coordinate.x == grid.cells[n].coordinate.x)
			{
				if (grid.cells[n + 1].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// TOP neighbor
		if (is_in_range(n - ROW_SIZE))
		{
			// TOP not out of bound cond
			if (grid.cells[n - ROW_SIZE].coordinate.x == (grid.cells[n].coordinate.x - 1))
			{
				if (grid.cells[n - ROW_SIZE].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// BOTTOM neighbor
		if (is_in_range(n + ROW_SIZE))
		{

			// BOTTOM not out of bound cond
			if (grid.cells[n + ROW_SIZE].coordinate.x == (grid.cells[n].coordinate.x + 1))
			{
				if (grid.cells[n + ROW_SIZE].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// TOP RIGHT neighbor
		if (is_in_range(n - (ROW_SIZE - 1)))
		{
			if (grid.cells[n - (ROW_SIZE - 1)].coordinate.x == (grid.cells[n].coordinate.x - 1))
			{
				if (grid.cells[n - (ROW_SIZE - 1)].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// TOP LEFT neighbor
		if (is_in_range(n - (ROW_SIZE + 1)))
		{
			if (grid.cells[n - (ROW_SIZE + 1)].coordinate.x == (grid.cells[n].coordinate.x - 1))
			{
				if (grid.cells[n - (ROW_SIZE + 1)].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// BOTTOM LEFT neighbor
		if (is_in_range(n + (ROW_SIZE - 1)))
		{
			if (grid.cells[n + (ROW_SIZE - 1)].coordinate.x == (grid.cells[n].coordinate.x + 1))
			{
				if (grid.cells[n + (ROW_SIZE - 1)].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		// BOTTOM RIGHT neighbor
		if (is_in_range(n + (ROW_SIZE + 1)))
		{
			if (grid.cells[n + (ROW_SIZE + 1)].coordinate.x == (grid.cells[n].coordinate.x + 1))
			{
				if (grid.cells[n + (ROW_SIZE + 1)].alive == ALIVE)
				{
					alive_count++;
				}
			}
		}

		next_generation(alive_count, grid.cells[n].alive, n);
		alive_count = 0;
	}

	update_current_generation_grid();
}

static void move_to_next_gen_every_second()
{
#ifdef __linux__
	struct timeval tv;

	gettimeofday(&tv, NULL);

	volatile int current_time = tv.tv_sec;
#else
	volatile int current_time = get_time();
#endif
	// TODO: figure out why doing a mod 60 is important here. Probably worth asking Stephen.
	if ((current_time % 60) != (last_time % 60))
	{
		if (game_of_life_cmd == CMD_RESUME)
		{
			last_time = current_time;
			figure_out_alive_cells();
			gen_count++;
			return;
		}

		/* endgame */
		return;
	}
}

static void init_cells()
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

	for (int i = 0; i < GRID_SIZE; i++)
	{
		next_generation_grid.cells[i].coordinate.x = grid.cells[i].coordinate.x;
		next_generation_grid.cells[i].coordinate.y = grid.cells[i].coordinate.y;
		next_generation_grid.cells[i].alive = grid.cells[i].alive;
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

static void render_next_gen_text(unsigned int gen_count)
{
	char next_gen_text[15];
	char gen_num_text[4];
	FbColor(WHITE);
	FbMove(LCD_XSIZE / 4, LCD_YSIZE - 7);

	strcpy(next_gen_text, "NEXT GEN ");
	itoa(gen_num_text, gen_count, 10);
	strcat(next_gen_text, gen_num_text);

	FbWriteLine(next_gen_text);
}

static void render_cell(int grid_x, int grid_y, int alive)
{
	int cell_color_state = alive ? BLUE : WHITE;
	render_box(grid_x, grid_y, cell_color_state);
}

static void render_cells()
{

	for (int i = 0; i <= GRID_SIZE - 1; i++)
	{
		render_cell(grid.cells[i].coordinate.x, grid.cells[i].coordinate.y, grid.cells[i].alive);
	}
}

static void render_game()
{

	FbClear();

	render_cells();
	render_next_gen_text(gen_count);
	FbSwapBuffers();
}

static void check_buttons()
{
	if (LEFT_BTN_AND_CONSUME)
	{
		game_of_life_state = GAME_OF_LIFE_EXIT;
	}
	else if (RIGHT_BTN_AND_CONSUME)
	{
		game_of_life_state = GAME_OF_LIFE_EXIT;
	}
	else if (UP_BTN_AND_CONSUME)
	{
		game_of_life_cmd = CMD_RESUME;
	}
	else if (DOWN_BTN_AND_CONSUME)
	{
		game_of_life_cmd = CMD_PAUSE;
	}

	render_game();
}

static void render_splash_screen()
{
	FbColor(WHITE);
	FbMove(LCD_XSIZE - 110, LCD_YSIZE / 2);
	FbWriteLine("Game of Life");
	FbSwapBuffers();
}

static void game_of_life_init(void)
{
#ifdef __linux__
	FbInit();
	FbClear();

	game_of_life_state = GAME_OF_LIFE_SPLASH_SCREEN;
#endif
}

static void game_of_life_splash_screen()
{
	render_splash_screen();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		init_cells();
		game_of_life_state = GAME_OF_LIFE_RUN;
	}
}

static void game_of_life_run()
{
	move_to_next_gen_every_second();
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
