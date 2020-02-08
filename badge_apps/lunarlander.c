/*********************************************

This is a lunar lander game for the HackRVA 2020 badge.

Author: Stephen M. Cameron <stephenmcameron@gmail.com>

**********************************************/
#ifdef __linux__
#include <stdio.h>
#include <sys/time.h> /* for gettimeofday */
#include <string.h> /* for memset */

#include "../linux/linuxcompat.h"
#include "../linux/bline.h"
#else
#include "colors.h"
#include "menu.h"
#include "buttons.h"
#include "fb.h"

/* TODO: I shouldn't have to declare these myself. */
#define size_t int
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcat(char *dest, const char *src);

#endif

#include "xorshift.h"
#include "build_bug_on.h"

#define SCREEN_XDIM 132
#define SCREEN_YDIM 132

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

#define TERRAIN_SEGMENT_WIDTH 10
#define DIMFACT 64

static int lander_time = 0;

/* Program states.  Initial state is LUNARLANDER_INIT */
enum lunarlander_state_t {
	LUNARLANDER_INIT,
	LUNARLANDER_RUN,
	LUNARLANDER_EXIT
};

static struct point lander_points[] = {
	{ 0, -10 }, /* body */
	{ 5, -8 },
	{ 5, 0 },
	{ 0, 3 },
	{ -5, 0 },
	{ -5, -8 },
	{ 0, -10 },
	{ -128, -128 },
	{ 0, 3 }, /* thruster */
	{ -3, 6 },
	{ 3, 6 },
	{ 0, 3 },
	{ -128, -128 },
	{ -5, 0 }, /* left leg */
	{ -7, 6 },
	{ -7, 8 },
	{ -128, -128 },
	{ -9, 8 }, /* left foot */
	{ -5, 8 },
	{ -128, -128 },
	{ 5, 0 }, /* right leg */
	{ 7, 6 },
	{ 7, 8 },
	{ -128, -128 },
	{ 9, 8 }, /* right foot */
	{ 5, 8 },
	{ -128, -128 },
	{ 0, -7 }, /* window */
	{ 3, -4 },
	{ 0, -1 },
	{ -3, -4 },
	{ 0, -7 },
};

static int terrain_y[1024] = { 0 };

struct lander_data {
	/* All values are 8x what is displayed. */
	int x, y;
	int vx, vy;
	int fuel;
} lander, oldlander;

static enum lunarlander_state_t lunarlander_state = LUNARLANDER_INIT;
static unsigned int xorshift_state = 0xa5a5a5a5;

static void init_terrain(int t[], int start, int stop)
{
	int mid, midy, n;

	mid = start + ((stop - start) / 2);

	if (mid == start || mid == stop)
		return;
	midy = (terrain_y[start] + terrain_y[stop]) / 2;

	/* Displace midy randomly here in proportion to (stop - start). */
	n = (xorshift(&xorshift_state) % 10000) - 5000;
	n = 2 * (n * (stop - start)) / 10000;
	midy += n;

	terrain_y[mid] = midy;
	init_terrain(terrain_y, start, mid);
	init_terrain(terrain_y, mid, stop);
	lander_time = 0;
}

static void lunarlander_init(void)
{
	FbInit();
	FbClear();
	lunarlander_state = LUNARLANDER_RUN;
	lander.x = 100;
	lander.y = 0;
	lander.vx = 0;
	lander.vy = 0;
	lander.fuel = 1000;
	oldlander = lander;
	terrain_y[0] = -100;
	terrain_y[1023] = -100;
	init_terrain(terrain_y, 0, 1023);
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		lunarlander_state = LUNARLANDER_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
		lander.vx = lander.vx - 1;
	} else if (RIGHT_BTN_AND_CONSUME) {
		lander.vx = lander.vx + 1;
	} else if (UP_BTN_AND_CONSUME) {
		lander.vy = lander.vy - 3;
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void draw_lander(void)
{
	const int x = SCREEN_XDIM / 2;
	const int y = SCREEN_YDIM / 3;
	FbDrawObject(lander_points, ARRAYSIZE(lander_points), BLACK, x, y, 1024);
	FbDrawObject(lander_points, ARRAYSIZE(lander_points), WHITE, x, y, 1024);
}

static void draw_terrain_segment(struct lander_data *lander, int i, int color)
{
	int x1, y1, x2, y2, sx1, sy1, sx2, sy2;
	int left = lander->x - DIMFACT * SCREEN_XDIM / 2;
	int right = lander->x + DIMFACT * SCREEN_XDIM / 2;
	int top = lander->y - DIMFACT * SCREEN_YDIM / 3;
	int bottom = lander->y + DIMFACT * 2 * SCREEN_YDIM / 3;

	if (i < 0)
		return;
	if (i > 1022)
		return;
	x1 = i * TERRAIN_SEGMENT_WIDTH;
	if (x1 <= left || x1 >= right)
		return;
	sx1 = x1 - lander->x + SCREEN_XDIM / 2;
	if (sx1 < 0 || sx1 >= SCREEN_XDIM)
		return;
	x2 = (i + 1) * TERRAIN_SEGMENT_WIDTH;
	if (x2 <= left || x2 >= right)
		return;
	sx2 = x2 - lander->x + SCREEN_XDIM / 2;
	if (sx2 < 0 || sx2 >= SCREEN_XDIM)
		return;
	y1 = terrain_y[i];
	if (y1 < top || y1 >= bottom)
		return;
	sy1 = y1 - lander->y + SCREEN_YDIM / 3;
	if (sy1 < 0 || sy1 >= SCREEN_YDIM)
		return;
	y2 = terrain_y[i + 1];
	if (y2 < top || y2 >= bottom)
		return;
	sy2 = y2 - lander->y + SCREEN_YDIM / 3;
	if (sy2 < 0 || sy2 >= SCREEN_YDIM)
		return;
	if (x1 <= lander->x && x2 >= lander->x && color != BLACK)
		FbColor(RED);
	else
		FbColor(color);
	FbLine(sx1, sy1, sx2, sy2);
}

static void draw_terrain(struct lander_data *lander, int color)
{
	int start, stop, i;

	start = (lander->x - TERRAIN_SEGMENT_WIDTH * SCREEN_XDIM / 2) / TERRAIN_SEGMENT_WIDTH;
	if (start < 0)
		start = 0;
	if (start > 1023)
		return;
	stop = (lander->x + TERRAIN_SEGMENT_WIDTH * SCREEN_XDIM / 2) / TERRAIN_SEGMENT_WIDTH;

	FbColor(color);
	for (i = start; i < stop; i++)
		draw_terrain_segment(lander, i, color);
}

static void move_lander(void)
{
	oldlander = lander;
	lander_time++;
	if (lander_time > 10000)
		lander_time = 0;
	if ((lander_time & 0x08) == 0) {
		lander.vy += 1;
	}
	lander.y += lander.vy;
	lander.x += lander.vx;
}

static void draw_instruments_color(struct lander_data *lander, int color)
{
	char buf1[10], buf2[10];

	itoa(buf1, lander->x, 10);
	itoa(buf2, lander->y, 10);
	FbMove(10, 110);
	FbColor(color);
	FbWriteLine(buf1);
	FbMove(60, 110);
	FbWriteLine(buf2);

	itoa(buf1, lander->vx, 10);
	itoa(buf2, lander->vy, 10);
	FbMove(10, 120);
	FbColor(color);
	FbWriteLine(buf1);
	FbMove(60, 120);
	FbWriteLine(buf2);
}

static void draw_instruments(void)
{
	draw_instruments_color(&oldlander, BLACK);
	draw_instruments_color(&lander, WHITE);
}

static void draw_screen()
{
	FbColor(WHITE);
	draw_terrain(&lander, BLACK); /* Erase previously drawn terrain */
	move_lander();
	draw_terrain(&lander, WHITE); /* Draw terrain */
	draw_lander();
	draw_instruments();
	FbSwapBuffers();
}

static void lunarlander_run()
{
	check_buttons();
	draw_screen();
}

static void lunarlander_exit()
{
	lunarlander_state = LUNARLANDER_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int lunarlander_cb(void)
{
	switch (lunarlander_state) {
	case LUNARLANDER_INIT:
		lunarlander_init();
		break;
	case LUNARLANDER_RUN:
		lunarlander_run();
		break;
	case LUNARLANDER_EXIT:
		lunarlander_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, lunarlander_cb, 10);
        return 0;
}
#endif
