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
#include "fb.h"

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
static unsigned int max_gen = 100;
static volatile int current_time;
static volatile int last_time;

#define ROW_SIZE 12
#define COL_SIZE 12
#define GRID_SIZE (ROW_SIZE * COL_SIZE)

#define GRID_X_PADDING 8
#define GRID_Y_PADDING 2
#define CELL_SIZE 10
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

static int is_in_range(int current_index)
{
	return current_index >= 0 && current_index < GRID_SIZE;
}

static int is_valid_pos(int pos)
{
	return pos >= 0 && pos < ROW_SIZE;
}

static int find_index(int neighbor_x_pos, int neighbor_y_pos)
{
	// Note: this formula only works if X and Y starting pos is 1 instead of 0
	return COL_SIZE * ((neighbor_x_pos + 1) - 1) + ((neighbor_y_pos + 1 ) - 1);
}

static int is_cell_alive(struct Grid *grid, int neighbor_x_pos, int neighbor_y_pos)
{
	if (is_valid_pos(neighbor_x_pos) && is_valid_pos(neighbor_y_pos))
	{
		int index = find_index(neighbor_x_pos, neighbor_y_pos);

		if (is_in_range(index))
		{
			return grid->cells[index].alive == ALIVE;
		}

		return FALSE;
	}

	return FALSE;
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

static void update_current_generation_grid(void)
{
	grid = next_generation_grid;
}

static int get_cell_x_pos(int cell_index)
{
	return cell_index / ROW_SIZE;
}

static int get_cell_y_pos(int cell_index)
{
	return cell_index % COL_SIZE;
}

static void figure_out_alive_cells(void)
{
	unsigned int alive_count = 0;
	// neighbors: LEFT, RIGHT, TOP, DOWN, TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT
	int neighbor_x_offset[8] = { -1, 1, 0, 0, -1, -1, 1, 1};
	int neighbor_y_offset[8] = { 0, 0, -1, 1, -1, 1, -1, 1};
	int n;

	for (n = 0; n < GRID_SIZE; n++)
	{
		int i, neighbor_x, neighbor_y;

		for (i = 0; i < 8; i++)
		{
			neighbor_x = get_cell_x_pos(n) + neighbor_x_offset[i];
			neighbor_y = get_cell_y_pos(n) + neighbor_y_offset[i];

			if (is_cell_alive(&grid, neighbor_x, neighbor_y))
			{
				alive_count++;
			}
		}

		next_generation(alive_count, grid.cells[n].alive, n);
		alive_count = 0;
	}

	update_current_generation_grid();
}

static void move_to_next_gen_every_second(void)
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

static void init_cells(void)
{
	int i;

	for (i = 0; i < GRID_SIZE; i++)
	{
		grid.cells[i].alive = xorshift((unsigned int *)&timestamp) % 2;
	}

	next_generation_grid = grid;
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
	char next_gen_text[13];
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

static void render_cells(void)
{
	int i;

	for (i = 0; i < GRID_SIZE; i++)
	{
		render_cell(get_cell_x_pos(i), get_cell_y_pos(i), grid.cells[i].alive);
	}
}

static void render_game(void)
{
	FbClear();

	render_cells();
	render_next_gen_text(gen_count);
	FbSwapBuffers();
}

static void render_end_game_screen(void)
{
	FbClear();
	FbColor(WHITE);
	FbMove(20, 40);
	FbWriteString("Thank you\nfor planning!\n\n\nPress button\nto leave");
	FbSwapBuffers();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		game_of_life_state = GAME_OF_LIFE_EXIT;
	}
}

static void check_buttons(void)
{
	if (LEFT_BTN_AND_CONSUME || RIGHT_BTN_AND_CONSUME)
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

	if (gen_count == max_gen)
	{
		game_of_life_cmd = CMD_PAUSE;
		render_end_game_screen();
	}

}

static void render_splash_screen(void)
{
	FbColor(WHITE);
	FbMove(10, 30);
	FbWriteString("Game of Life\n\n\n\nLeft/Right Dpad\nto exit");
	FbSwapBuffers();
}

static void game_of_life_init(void)
{
	FbInit();
	FbClear();

	game_of_life_state = GAME_OF_LIFE_SPLASH_SCREEN;
}

static void game_of_life_splash_screen(void)
{
	render_splash_screen();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		init_cells();
		game_of_life_state = GAME_OF_LIFE_RUN;
	}
	else if (LEFT_BTN_AND_CONSUME || RIGHT_BTN_AND_CONSUME)
	{
		game_of_life_state = GAME_OF_LIFE_EXIT;
	}
}

static void game_of_life_run(void)
{
	move_to_next_gen_every_second();
	check_buttons();
}

static void game_of_life_exit(void)
{
	game_of_life_state = GAME_OF_LIFE_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int game_of_life_cb(void)
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
