/******************************************************************
 A hacking simulator that's not much like real world hacking at all.

 Author: Dustin Firebaugh <dafirebaugh@gmail.com>
 (c) 2020 Dusitn Firebaugh

 ******************************************************************/
#ifdef __linux__
#include <stdio.h>
#include <sys/time.h> /* for gettimeofday */
#include <string.h>   /* for memset */

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

extern int timestamp;

#endif

#ifndef NULL
#define NULL 0
#endif

#include "build_bug_on.h"
#include "xorshift.h"

/* HACKSIM_DEBUG 
 * 1 debug mode on
 * 0 debug mode off
 */
#define HACKSIM_DEBUG 0
#define TRUE 1
#define FALSE 0
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))
#define FLOW_RATE 1
#define GRID_SIZE 5
#define CELL_SIZE 20
#define CELL_PADDING 0
#define PIPE_SCALE 480
#define PIPE_OFFSET 10
#define GRID_PADDING 3
#define STARTX(x) (GRID_PADDING + CELL_PADDING + ((x)*CELL_SIZE))
#define ENDX(x) (GRID_PADDING + CELL_SIZE + ((x)*CELL_SIZE))
#define STARTY(y) (GRID_PADDING + CELL_PADDING + ((y)*CELL_SIZE))
#define ENDY(y) (GRID_PADDING + CELL_SIZE + ((y)*CELL_SIZE))
#define WITHIN_GRID(x, y) ((x) >= 0 && (x) <= GRID_SIZE && (y) >= 0 && (y) <= GRID_SIZE - 1)
#define IS_NEIGHBOR(sX, sY, tX, tY) (((sX - 1) == (tX) && (sY) == (tY)) || ((sX - 1) == (tX) && (sY + 1) == (tY)) || ((sX + 1) == (tX) && (sY) == (tY)) || ((sX + 1) == (tX) && (sY - 1) == (tY)) || ((sX) == (tX) && (sY - 1) == (tY)) || ((sX - 1) == (tX) && (sY - 1) == (tY)) || ((sX) == (tX) && (sY + 1) == (tY)) || ((sX + 1) == (tX) && (sY + 1) == (tY)))
#define IS_SOURCE(x,y) ((x) == 0 && (y) == source)
#define GAME_TIME_LIMIT 75
#define HANDX 0
#define HANDY GRID_SIZE
#define TIMERX GRID_SIZE
#define TIMERY GRID_SIZE
/* Program states.  Initial state is HACKINGSIMULATOR_INIT */
enum hacking_simulator_state_t
{
	HACKINGSIMULATOR_INIT,
	HACKINGSIMULATOR_SPLASH_SCREEN,
	HACKINGSIMULATOR_RUN,
	HACKINGSIMULATOR_WIN_SCREEN,
	HACKINGSIMULATOR_FAIL,
	HACKINGSIMULATOR_EXIT,
};

static enum hacking_simulator_state_t hacking_simulator_state = HACKINGSIMULATOR_INIT;

enum difficulty_level
{
	EASY,
	MEDIUM,
	HARD,
	VERY_HARD
};

static enum difficulty_level difficulty_level_state = VERY_HARD;

static const struct point pipebr_points[] =
#include "hacking_simulator_drawings/pipe_b_r.h"
static const struct point pipebl_points[] =
#include "hacking_simulator_drawings/pipe_b_l.h"
static const struct point pipetr_points[] =
#include "hacking_simulator_drawings/pipe_t_r.h"
static const struct point pipetl_points[] =
#include "hacking_simulator_drawings/pipe_t_l.h"
static const struct point pipetb_points[] =
#include "hacking_simulator_drawings/pipe_t_b.h"
static const struct point pipelr_points[] =
#include "hacking_simulator_drawings/pipe_l_r.h"
static const struct point blocking_square_points[] =
{
	{-18, 18},
	{18, -18},
	{-128, -128},
	{-18, -18},
	{18, 18},
};

static const struct point arrow_points[] =
{
	{0, -9},
	{14, -9},
	{14, -17},
	{26, -3},
	{15, 9},
	{15, 0},
	{0, 0},
	{0, -7},
};

struct pipe_io
{
	signed int left, right, up, down;
};

struct pipe
{
	int npoints;
	const struct point *drawing;
	const struct pipe_io io;
};

#define INVALID_PIPE_INDEX -2
#define BLOCKING_INDEX -1
#define HORIZONTAL 0
#define VERTICAL 1
#define BOTTOM_RIGHT 2
#define BOTTOM_LEFT 3
#define TOP_RIGHT 4
#define TOP_LEFT 5

static struct pipe pipes[] = {
	{ARRAYSIZE(pipelr_points), pipelr_points, {1, 1, 0, 0}},
	{ARRAYSIZE(pipetb_points), pipetb_points, {0, 0, 1, 1}},
	{ARRAYSIZE(pipebr_points), pipebr_points, {0, 1, 0, 1}},
	{ARRAYSIZE(pipebl_points), pipebl_points, {1, 0, 0, 1}},
	{ARRAYSIZE(pipetr_points), pipetr_points, {0, 1, 1, 0}},
	{ARRAYSIZE(pipetl_points), pipetl_points, {1, 0, 1, 0}},
};

static int cursor_x_index, cursor_y_index;

/* cell_io is used to help determine direction in flow_path */
enum cell_io
{
	NOT_SET,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

struct cell
{
	int x, y, hidden, pipe_index, in_path;
	int locked; /* if the water has reached this pipe, it is locked */
	enum cell_io input;
	enum cell_io output;
	struct cell *next_cell;
};

static struct cell grid[GRID_SIZE + 1][GRID_SIZE];

/* hand is used to temporarily hold a pipe that you can swap */
static int hand;
static int hsim_seed;
static int fill_line;
static int source;
static int target;
static int game_tick;
static volatile int last_time;
static struct cell *cell_in_path;
static struct cell *tail = NULL;
static struct cell *flow_path_cell = NULL;
static struct cell *head;

static void do_flow()
{
/* TODO: add logic to break flow */

	if (tail == NULL)
	{
		tail = &grid[0][source];
	}

	tail->in_path = TRUE;

	if ((tail->x == GRID_SIZE && tail->y == target) &&
		pipes[tail->pipe_index].io.right)
	{
		hacking_simulator_state = HACKINGSIMULATOR_WIN_SCREEN;
		return;
	}

	tail = tail->next_cell;
}

/* place_blocker places a blocking piece on the grid
*  but we can't have a blocker on the source or the target 
*  or a neighbor of source or target
* and we don't want to overwrite a blocker
*
*/
static void place_blocker()
{
	int x,y;

	int placed_blocker = FALSE;
#ifdef __linux__
	printf("\nplacing blocker");
#endif

	do
	{
		hsim_seed++; /* update seed allows us to get a slightly different seed for a different random number */
		x = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
		y = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;

		/* prevent blocker from being placed on an invalid pipe spot */
		if (grid[x][y].pipe_index != INVALID_PIPE_INDEX)
			continue;

		/* prevent blocker from being placed on the source cell */
		if (x == 0 && y == source)
			continue;

		/* prevent blocker from being placed from being placed on the target cell */
		if (x == GRID_SIZE && y == target)
			continue;

		/* prevent blocker from being placed near the source or target */
		if (IS_NEIGHBOR(0, source, x, y) || IS_NEIGHBOR(GRID_SIZE, target, x, y))
			continue;

		/* prevent blocker from being placed on top of another blocker */
		if (grid[x][y].pipe_index == BLOCKING_INDEX)
			continue;

		placed_blocker = TRUE;
		grid[x][y].pipe_index = BLOCKING_INDEX; /* place blocking sqaure in random spot on grid */
	} while (!placed_blocker);
}

/* find_win determines a winning path and places those pieces on the board */
static void find_win()
{
	int vertical_direction;
	int currentX;
	int currentY = source;

	/* move horizontally */
	for (currentX = 0; currentX < GRID_SIZE; currentX++)
		grid[currentX][currentY].pipe_index = HORIZONTAL;

	/* handle the case where the target is on the same row as the source */
	if (currentY == target)
	{
		grid[currentX][currentY].pipe_index = HORIZONTAL;
	}

	/* move vertically */
	while (currentY != target)
	{
		if (currentX == GRID_SIZE)
		{
			if (target > currentY)
			{
				vertical_direction = -1;
				if (currentY == source)
				{ /* handle first elbow on last column */
					grid[currentX][currentY].pipe_index = BOTTOM_LEFT;
				}
				else
				{
					grid[currentX][currentY].pipe_index = VERTICAL;
				}
				currentY++;
				continue;
			}
			else
			{
				vertical_direction = 1;
				if (currentY == source)
				{ /* handle first elbow on last column */
					grid[currentX][currentY].pipe_index = TOP_LEFT;
				}
				else
				{
					grid[currentX][currentY].pipe_index = VERTICAL;
				}
				currentY--;
				continue;
			}
		}
	}

	/* handle the last elbow */
	if (vertical_direction == -1)
	{
		grid[currentX][currentY].pipe_index = TOP_RIGHT;
	}
	else if (vertical_direction == 1)
	{
		grid[currentX][currentY].pipe_index = BOTTOM_RIGHT;
	}
}

static void shuffle()
{
	int i, j, tmpX, tmpY, tmp_pindex;
	int source_neighbor_is_set = FALSE;
	for (i = 0; i <= GRID_SIZE; i++)
	{
		for (j = 0; j < GRID_SIZE; j++)
		{
			tmpX = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
			tmpY = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;

			/* put a VERTICAL pipe next to the source so that we block the flow at start */
			if (grid[i][j].pipe_index == VERTICAL && !source_neighbor_is_set)
			{
				source_neighbor_is_set = TRUE;
				grid[i][j].pipe_index = grid[1][source].pipe_index;
				grid[1][source].pipe_index = VERTICAL;
			}
			/* don't swap blockers */
			if (grid[i][j].pipe_index == BLOCKING_INDEX || grid[tmpX][tmpY].pipe_index == BLOCKING_INDEX)
			{
				continue;
			}

			/* don't swap the source or the cell next to source
			this ensures that we don't start the flow until 
			the player decides to swap something */
			if ((i == 0 && j == source) || (tmpX == 0 && tmpY == source) ||
				(i == 1 && j == source) || (tmpX == 1 && tmpY == source))
			{
				continue;
			}

			tmp_pindex = grid[i][j].pipe_index;
			grid[i][j].pipe_index = grid[tmpX][tmpY].pipe_index;
			grid[tmpX][tmpY].pipe_index = tmp_pindex;
		}
	}
}

/* place_random_pipe determines the next square to place in 
*  we first place a path that makes winning possible
*  then we place random pipes to fill the grid
*/
static void place_random_pipe(struct cell *cell)
{
	if (cell->pipe_index != INVALID_PIPE_INDEX)
	{
		return;
	}
	cell->pipe_index = xorshift((unsigned int *)&hsim_seed) % ARRAYSIZE(pipes);
}

/* handle difficulty setting
*  we probably shouldn't have more than 3 blockers 
*  because that could potentially block a route from source to target
*/
static void handle_difficulty()
{
	switch (difficulty_level_state)
	{
	case EASY:
		break;
	case MEDIUM:
		place_blocker();
		place_blocker();
		break;
	case HARD:
		place_blocker();
		place_blocker();
		place_blocker();
		break;
	case VERY_HARD:
		/* TODO: enable screen flicker */
		place_blocker();
		place_blocker();
		place_blocker();
		place_blocker();
		break;
	default:
		break;
	}
}

static void initialize_grid()
{
	int x, y;
	/* pick starting point and finishing point -- 
	*  for now we will assume the start is on the left and 
	*  the finish is on the right
	*
	*  adding source to target so that we how often these are on the same row
	*/
	source = xorshift((unsigned int *)&hsim_seed) % GRID_SIZE;
	target = (xorshift((unsigned int *)&hsim_seed) + source) % GRID_SIZE;

	for (x = 0; x <= GRID_SIZE; x++)
	{
		for (y = 0; y < GRID_SIZE; y++)
		{
			grid[x][y].x = x;
			grid[x][y].y = y;
			grid[x][y].hidden = TRUE;
			grid[x][y].locked = FALSE;
			grid[x][y].in_path = FALSE;
			grid[x][y].next_cell = NULL;
			if (grid[x][y].pipe_index == BLOCKING_INDEX)
			{
				continue;
			}
			/* initialize pipe_index to INVALID_PIPE_INDEX so that we can determine if we have placed a pipe */
			grid[x][y].pipe_index = INVALID_PIPE_INDEX;
		}
	}

	/* TODO: it might make it more difficult if we place blockers and then find the win-path */
	find_win();
	handle_difficulty();

	/* random grid */
	for (x = 0; x <= GRID_SIZE; x++)
	{
		for (y = 0; y < GRID_SIZE; y++)
		{
			if (grid[x][y].pipe_index == BLOCKING_INDEX && grid[x][y].pipe_index != INVALID_PIPE_INDEX)
			{
				continue;
			}
			place_random_pipe(&grid[x][y]);
		}
	}

	/* set source square as not hidden */
	grid[0][source].hidden = FALSE;
	grid[0][source].locked = TRUE;
	cell_in_path = &grid[0][source];
	tail = &grid[0][source];

	shuffle();
	hand = 0;
}

static void hackingsimulator_init(void)
{
	FbInit();
	FbClear();
	cursor_x_index = 0;
	cursor_y_index = 0;
	game_tick = 0; /* game_tick maybe this is seconds*/
	fill_line = 0;
	hacking_simulator_state = HACKINGSIMULATOR_SPLASH_SCREEN;
}

static void draw_splash_screen()
{
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteLine("Welcome to HackingSim");
	FbPaintNewRows();
}

static void hackingsimulator_splash_screen()
{
	draw_splash_screen();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		hsim_seed = timestamp;
		initialize_grid();
		hacking_simulator_state = HACKINGSIMULATOR_RUN;
	}
}

static void hackingsimulator_fail()
{
	FbClear();
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteLine("You FAILED!");
	FbPaintNewRows();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		hacking_simulator_state = HACKINGSIMULATOR_EXIT;
	}
}

static void hackingsimulator_win_screen()
{
	FbClear();
	FbColor(WHITE);
	FbMove(0, 20);
	FbWriteLine("You WON!");
	FbPaintNewRows();

	if (BUTTON_PRESSED_AND_CONSUME)
	{
		hacking_simulator_state = HACKINGSIMULATOR_EXIT;
	}
}

static void advance_tick()
{
#ifdef __linux__
	struct timeval tv;

	gettimeofday(&tv, NULL);

	volatile int current_time = tv.tv_sec;
#else
	volatile int current_time = get_time();
#endif
	if ((current_time % 60) != (last_time % 60))
	{
		if (game_tick < 100)
		{
			last_time = current_time;
			game_tick++;
			return;
		}

		/* endgame */
		return;
	}
}

static void swap_with_hand(struct cell *cell, int tmp_pipe)
{
	cell->output = NOT_SET;
	cell->input = NOT_SET;
	hand = cell->pipe_index;
	cell->pipe_index = tmp_pipe;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME)
	{
		if (grid[cursor_x_index][cursor_y_index].hidden == TRUE)
		{
			/* handle reveal */
			grid[cursor_x_index][cursor_y_index].hidden = FALSE;
		}
		else
		{
			/* Check to see if the cell is blocking */
			if (grid[cursor_x_index][cursor_y_index].pipe_index == BLOCKING_INDEX)
			{
				return;
			}
			if (grid[cursor_x_index][cursor_y_index].locked == TRUE)
			{
				return;
			}
			swap_with_hand(&grid[cursor_x_index][cursor_y_index], hand);
		}
	}
	else if (LEFT_BTN_AND_CONSUME)
	{
		cursor_x_index--;
	}
	else if (RIGHT_BTN_AND_CONSUME)
	{
		cursor_x_index++;
	}
	else if (UP_BTN_AND_CONSUME)
	{
		cursor_y_index--;
	}
	else if (DOWN_BTN_AND_CONSUME)
	{
		cursor_y_index++;
	}

	if (cursor_x_index < 0)
		cursor_x_index = 0;
	if (cursor_x_index > GRID_SIZE)
		cursor_x_index = GRID_SIZE;
	if (cursor_y_index < 0)
		cursor_y_index = 0;
	if (cursor_y_index > GRID_SIZE - 1)
		cursor_y_index = GRID_SIZE - 1;
}

/* determine_cell_output looks at the cell input and the cell's io. 
It will set the cell's output to other io (i.e. the one that's not set as the input) */
static void determine_cell_output(struct cell *cell)
{
	switch (cell->input)
	{
	case UP:
		if (pipes[cell->pipe_index].io.left)
		{
			cell->output = LEFT;
		}
		else if (pipes[cell->pipe_index].io.down)
		{
			cell->output = DOWN;
		}
		else if (pipes[cell->pipe_index].io.right)
		{
			cell->output = RIGHT;
		}
		break;
	case DOWN:
		if (pipes[cell->pipe_index].io.left)
		{
			cell->output = LEFT;
		}
		else if (pipes[cell->pipe_index].io.up)
		{
			cell->output = UP;
		}
		else if (pipes[cell->pipe_index].io.right)
		{
			cell->output = RIGHT;
		}
		break;
	case LEFT:
		if (pipes[cell->pipe_index].io.up)
		{
			cell->output = UP;
		}
		else if (pipes[cell->pipe_index].io.down)
		{
			cell->output = DOWN;
		}
		else if (pipes[cell->pipe_index].io.right)
		{
			cell->output = RIGHT;
		}
		break;
	case RIGHT:
		if (pipes[cell->pipe_index].io.left)
		{
			cell->output = LEFT;
		}
		else if (pipes[cell->pipe_index].io.down)
		{
			cell->output = DOWN;
		}
		else if (pipes[cell->pipe_index].io.up)
		{
			cell->output = UP;
		}
		break;

	default:
		break;
	}
}

/* deterimine_cell_input verifies that the next_cell has an accomadating input to match the last output*/
static void determine_cell_input(struct cell *cell, enum cell_io last_output)
{
	switch (last_output)
	{
	case LEFT:
		if (!pipes[cell->pipe_index].io.right)
		{
			return;
		}
		cell->input = RIGHT;
		break;
	case RIGHT:
		if (!pipes[cell->pipe_index].io.left)
		{
			return;
		}
		cell->input = LEFT;
		break;
	case UP:
		if (!pipes[cell->pipe_index].io.down)
		{
			return;
		}
		cell->input = DOWN;
		break;
	case DOWN:
		if (!pipes[cell->pipe_index].io.up)
		{
			return;
		}
		cell->input = UP;
		break;
	default:
		cell->input = NOT_SET;
		break;
	}
}

static void setup_flow_path()
{
	/* iterate through flow_path by determing if the next cell has an accomadating input */
	int x_offset = 0;
	int y_offset = 0;
	int last_output;

	/* handle source cell */
	if (cell_in_path->x == 0 && cell_in_path->y == source)
	{
		cell_in_path->in_path = TRUE;
		cell_in_path->output = RIGHT;
		head = cell_in_path;
	}

	if (cell_in_path->pipe_index == INVALID_PIPE_INDEX || cell_in_path->pipe_index == BLOCKING_INDEX || cell_in_path->pipe_index < 0)
	{
		return;
	}

	/* match the current cells output with the next cells input */
	switch (cell_in_path->output)
	{
	case RIGHT:
		x_offset = 1;
		last_output = RIGHT;
		break;
	case LEFT:
		x_offset = -1;
		last_output = LEFT;
		break;
	case UP:
		y_offset = -1;
		last_output = UP;
		break;
	case DOWN:
		y_offset = 1;
		last_output = DOWN;
		break;
	default:
		break;
	}
	int newX = cell_in_path->x + x_offset;
	int newY = cell_in_path->y + y_offset;

	/* make sure cell is on the grid */
	if (WITHIN_GRID(newX, newY))
	{
		struct cell *next_cell = &grid[newX][newY];

		/* check to make sure the next cell isn't blocking */
		if (next_cell->pipe_index < 0)
		{
			next_cell->in_path = FALSE;
			return;
		}

		determine_cell_input(next_cell, last_output);

		if (next_cell->input == NOT_SET)
		{
			return;
		}

		else
		{
			cell_in_path->next_cell = next_cell;
			cell_in_path = &grid[cell_in_path->x + x_offset][cell_in_path->y + y_offset];
			determine_cell_output(cell_in_path);
		}
	}
}

static void fill_cell(struct cell current_cell, int i)
{
	switch (current_cell.input)
	{
	case LEFT:
		if (current_cell.output == UP)
		{
			FbHorizontalLine(STARTX(current_cell.x), ENDY(current_cell.y) - i, ENDX(current_cell.x), 0);
		}
		else if (current_cell.output == DOWN)
		{
			FbHorizontalLine(STARTX(current_cell.x), STARTY(current_cell.y) + i, ENDX(current_cell.x), 0);
		}

		FbVerticalLine(STARTX(current_cell.x) + i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		break;
	case RIGHT:
		if (current_cell.output == UP)
		{
			FbHorizontalLine(STARTX(current_cell.x), ENDY(current_cell.y) - i, ENDX(current_cell.x), 0);
		}
		else if (current_cell.output == DOWN)
		{
			FbHorizontalLine(STARTX(current_cell.x), STARTY(current_cell.y) + i, ENDX(current_cell.x), 0);
		}

		FbVerticalLine(ENDX(current_cell.x) - i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		break;
	case UP:
		if (current_cell.output == LEFT)
		{
			FbVerticalLine(ENDX(current_cell.x) - i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		}
		else if (current_cell.output == RIGHT)
		{
			FbVerticalLine(STARTX(current_cell.x) + i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		}

		FbHorizontalLine(STARTX(current_cell.x), STARTY(current_cell.y) + i, ENDX(current_cell.x), 0);
		break;
	case DOWN:
		if (current_cell.output == LEFT)
		{
			FbVerticalLine(ENDX(current_cell.x) - i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		}
		else if (current_cell.output == RIGHT)
		{
			FbVerticalLine(STARTX(current_cell.x) + i, STARTY(current_cell.y), 0, ENDY(current_cell.y));
		}
		FbHorizontalLine(STARTX(current_cell.x), ENDY(current_cell.y) - i, ENDX(current_cell.x), 0);
		break;
	default:
		break;
	}
}

/* TODO: implement flow animation */
static void advance_flow()
{
	int i;
	FbColor(BLUE);

	if (flow_path_cell == NULL)
	{
		head->input = LEFT;
		flow_path_cell = head;
	}

	if (flow_path_cell->next_cell == NULL)
	{
		return;
	}

	int count = 0;
	struct cell *current = head;

	while (current != NULL)
	{
		current->locked = TRUE;
		for (i = 0; i < CELL_SIZE; i++)
		{
			fill_cell(*current, i);
		}
		current = current->next_cell;
		count++;
	}

	flow_path_cell = flow_path_cell->next_cell;
}

static void draw_pipe(struct cell cell)
{
	/* Check to see if the cell is blocking */
	if (cell.pipe_index == BLOCKING_INDEX)
	{
		FbDrawObject(blocking_square_points, ARRAYSIZE(blocking_square_points), RED, STARTX(cell.x) + PIPE_OFFSET, STARTY(cell.y) + PIPE_OFFSET, PIPE_SCALE);
	}
	else
	{
		if (cell.locked)
		{
			FbColor(BLUE);
			int i = 0;
			for (i = 0; i < CELL_SIZE; i++)
			{
				fill_cell(cell, i);
			}
		}
		FbDrawObject(pipes[cell.pipe_index].drawing, pipes[cell.pipe_index].npoints, YELLOW, STARTX(cell.x) + PIPE_OFFSET, STARTY(cell.y) + PIPE_OFFSET, PIPE_SCALE);
#ifdef HACKSIM_DEBUG
		if (cell.in_path)
		{
			FbMove(STARTX(cell.x), STARTY(cell.y));
		}
#endif
	}
	FbColor(WHITE);
}

static void draw_hand()
{
	/* Check to see if the cell is blocking */
	FbDrawObject(pipes[hand].drawing, pipes[hand].npoints, YELLOW, STARTX(HANDX) + PIPE_OFFSET, STARTY(HANDY) + PIPE_OFFSET, PIPE_SCALE);
	FbColor(WHITE);
}

static void draw_timer()
{
	char p[4];
	FbMove(STARTX(TIMERX) + (CELL_SIZE / 5), STARTY(TIMERY) + (CELL_SIZE / 2));
	// itoa(p, hand, 10);
	itoa(p, game_tick, 10);
	FbWriteLine(p);
}

static void draw_box(int grid_x, int grid_y, int color)
{
	FbColor(color);
	FbHorizontalLine(STARTX(grid_x), STARTY(grid_y), ENDX(grid_x), STARTY(grid_y));
	FbHorizontalLine(STARTX(grid_x), ENDY(grid_y), ENDX(grid_x), ENDY(grid_y));
	FbVerticalLine(STARTX(grid_x), STARTY(grid_y), STARTX(grid_x), ENDY(grid_y));
	FbVerticalLine(ENDX(grid_x), STARTY(grid_y), ENDX(grid_x), ENDY(grid_y));
}

static void draw_arrow(int x, int y)
{
	FbDrawObject(arrow_points, ARRAYSIZE(arrow_points), GREEN, x + PIPE_OFFSET, y + PIPE_OFFSET, PIPE_SCALE);
}

static void draw_cell(int posX, int posY)
{
#if HACKSIM_DEBUG
	char p[4];
#endif

	/* paint the hand */
	if (posX == HANDX && posY == HANDY)
	{
		draw_box(posX, posY, BLUE);
		draw_hand();
		draw_timer();
#if HACKSIM_DEBUG
		FbMove(STARTX(posX) + (CELL_SIZE / 5), STARTY(posY) + (CELL_SIZE / 2));
		// itoa(p, hand, 10);
		itoa(p, cell_in_path->input, 10);
		FbWriteLine(p);
#endif
		return;
	}

	/* draw cell borders */
	draw_box(posX, posY, WHITE);

	if (grid[posX][posY].hidden == 1)
	{
		FbMove(STARTX(posX) + (CELL_SIZE / 2), STARTY(posY) + (CELL_SIZE / 2));
#if HACKSIM_DEBUG
		FbWriteLine("?");
#endif
	}
	else
	{
		draw_pipe(grid[posX][posY]);

#if HACKSIM_DEBUG
		FbMove(STARTX(posX) + (CELL_SIZE / 5), STARTY(posY) + (CELL_SIZE / 2));
		itoa(p, grid[posX][posY].pipe_index, 10);
		FbWriteLine(p);
#endif
	}

	if (posX == 0 && posY == source)
	{
		draw_arrow(STARTX(0) - 10, STARTY(posY));
	}

	if (posX == GRID_SIZE && posY == target)
	{
		draw_arrow(STARTX(GRID_SIZE), STARTY(posY));
	}
}

static void draw_grid()
{
	int x, y;

	for (x = 0; x <= GRID_SIZE; x++)
	{
		for (y = 0; y < GRID_SIZE; y++)
		{
			draw_cell(x, y);
		};
	};

	/* draw hand */
	FbColor(BLUE);
	draw_cell(HANDX, HANDY);
}

static void draw_cursor()
{
	draw_box(cursor_x_index, cursor_y_index, GREEN);
}

static void draw_screen()
{
	FbColor(WHITE);
	FbClear();

	if (game_tick < GAME_TIME_LIMIT)
	{
		advance_flow();
	}
	else
	{
		hacking_simulator_state = HACKINGSIMULATOR_FAIL;
	}

	draw_grid();
	draw_cursor();
	FbSwapBuffers();
}

static void hackingsimulator_run()
{
	check_buttons();
	advance_tick();
	fill_line = ((game_tick * FLOW_RATE) * GAME_TIME_LIMIT) / SCREEN_XDIM;
	setup_flow_path();
	do_flow();
	draw_screen();
}

static void hackingsimulator_exit()
{
	hacking_simulator_state = HACKINGSIMULATOR_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int hacking_simulator_cb(void)
{
	switch (hacking_simulator_state)
	{
	case HACKINGSIMULATOR_INIT:
		hackingsimulator_init();
		break;
	case HACKINGSIMULATOR_SPLASH_SCREEN:
		hackingsimulator_splash_screen();
		break;
	case HACKINGSIMULATOR_WIN_SCREEN:
		hackingsimulator_win_screen();
		break;
	case HACKINGSIMULATOR_RUN:
		hackingsimulator_run();
		break;
	case HACKINGSIMULATOR_FAIL:
		hackingsimulator_fail();
		break;
	case HACKINGSIMULATOR_EXIT:
		hackingsimulator_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
	start_gtk(&argc, &argv, hacking_simulator_cb, 30);
	return 0;
}
#endif
