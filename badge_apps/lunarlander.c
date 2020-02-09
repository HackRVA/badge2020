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

static struct lander_data {
	/* All values are 8x what is displayed. */
	int x, y;
	int vx, vy;
	int fuel;
} lander, oldlander;

#define MAXSPARKS 20
static struct spark_data {
	int x, y, vx, vy, alive;
} spark[MAXSPARKS] = { 0 };

static unsigned int xorshift_state = 0xa5a5a5a5;

static void add_spark(int x, int y, int vx, int vy)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive) {
			spark[i].x = x;
			spark[i].y = y;
			spark[i].vx = vx + (xorshift(&xorshift_state) & 0x03f) - 32;
			spark[i].vy = vy + (xorshift(&xorshift_state) & 0x03f) - 32;
			spark[i].alive = 2 + (xorshift(&xorshift_state) & 0x7);
			return;
		}
	}
}

static void add_sparks(struct lander_data *lander, int x, int y, int n)
{
	int i;

	for (i = 0; i < n; i++)
		add_spark(lander->x, lander->y, x, y);
}

static void move_sparks(void)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		spark[i].x += spark[i].vx;
		spark[i].y += spark[i].vy;
		if (spark[i].alive > 0)
			spark[i].alive--;
	}
}

static void draw_sparks(struct lander_data *lander, int color)
{
	int i, x1, y1, x2, y2;
	const int sx = SCREEN_XDIM / 2;
	const int sy = SCREEN_YDIM / 3;

	FbColor(color);
	for (i = 0; i < MAXSPARKS; i++) {
		if (!spark[i].alive)
			continue;
		x1 = ((spark[i].x - lander->x - spark[i].vx) >> 8) + sx;
		y1 = ((spark[i].y - lander->y - spark[i].vy) >> 8) + sy;
		x2 = ((spark[i].x - lander->x) >> 8) + sx;
		y2 = ((spark[i].y - lander->y) >> 8) + sy;
		if (x1 >= 0 && x1 <= 127 && y1 >= 0 && y1 <= 127 &&
			x2 >= 0 && x2 <= 127 && y2 >= 0 && y2 <= 127)
			FbLine(x1, y1, x2, y2);
	}
}

static enum lunarlander_state_t lunarlander_state = LUNARLANDER_INIT;

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
	terrain_y[0] = -100;
	terrain_y[1023] = -100;
	init_terrain(terrain_y, 0, 1023);
	lunarlander_state = LUNARLANDER_RUN;
	lander.x = 100 << 8;
	lander.y = (terrain_y[0] - 30) << 8;
	lander.vx = 0;
	lander.vy = 0;
	lander.fuel = 1000;
	oldlander = lander;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		lunarlander_state = LUNARLANDER_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
		lander.vx = lander.vx - (1 << 7);
		add_sparks(&lander, lander.vx + (5 << 8), lander.vy + 0, 5);
	} else if (RIGHT_BTN_AND_CONSUME) {
		lander.vx = lander.vx + (1 << 7);
		add_sparks(&lander, lander.vx - (5 << 8), lander.vy + 0, 5);
	} else if (UP_BTN_AND_CONSUME) {
		lander.vy = lander.vy - (1 << 7);
		add_sparks(&lander, lander.vx + 0, lander.vy + (5 << 8), 5);
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
	int left = (lander->x >> 8) - DIMFACT * SCREEN_XDIM / 2;
	int right = (lander->x >> 8) + DIMFACT * SCREEN_XDIM / 2;
	int top = (lander->y >> 8) - DIMFACT * SCREEN_YDIM / 3;
	int bottom = (lander->y >> 8) + DIMFACT * 2 * SCREEN_YDIM / 3;

	if (i < 0)
		return;
	if (i > 1022)
		return;
	x1 = i * TERRAIN_SEGMENT_WIDTH;
	if (x1 <= left || x1 >= right)
		return;
	sx1 = x1 - (lander->x >> 8) + SCREEN_XDIM / 2;
	if (sx1 < 0 || sx1 >= SCREEN_XDIM)
		return;
	x2 = (i + 1) * TERRAIN_SEGMENT_WIDTH;
	if (x2 <= left || x2 >= right)
		return;
	sx2 = x2 - (lander->x >> 8) + SCREEN_XDIM / 2;
	if (sx2 < 0 || sx2 >= SCREEN_XDIM)
		return;
	y1 = terrain_y[i];
	if (y1 < top || y1 >= bottom)
		return;
	sy1 = y1 - (lander->y >> 8) + SCREEN_YDIM / 3;
	if (sy1 < 0 || sy1 >= SCREEN_YDIM)
		return;
	y2 = terrain_y[i + 1];
	if (y2 < top || y2 >= bottom)
		return;
	sy2 = y2 - (lander->y >> 8) + SCREEN_YDIM / 3;
	if (sy2 < 0 || sy2 >= SCREEN_YDIM)
		return;
	if (x1 <= (lander->x >> 8) && x2 >= (lander->x >> 8) && color != BLACK) {
		if ((lander->y >> 8) >= y2 - 8)
			FbColor(RED);
		else
			FbColor(BLUE);
	} else {
		FbColor(color);
	}
	FbLine(sx1, sy1, sx2, sy2);
}

static void draw_terrain(struct lander_data *lander, int color)
{
	int start, stop, i;

	start = ((lander->x >> 8) - TERRAIN_SEGMENT_WIDTH * SCREEN_XDIM / 2) / TERRAIN_SEGMENT_WIDTH;
	if (start < 0)
		start = 0;
	if (start > 1023)
		return;
	stop = ((lander->x >> 8) + TERRAIN_SEGMENT_WIDTH * SCREEN_XDIM / 2) / TERRAIN_SEGMENT_WIDTH;

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
		lander.vy += 10;
	}
	lander.y += lander.vy;
	lander.x += lander.vx;
}

static void draw_instruments_color(struct lander_data *lander, int color)
{
	char buf1[10], buf2[10];

	itoa(buf1, (lander->x >> 8), 10);
	itoa(buf2, (lander->y >> 8), 10);
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
	draw_sparks(&lander, BLACK);
	move_lander();
	move_sparks();
	draw_terrain(&lander, WHITE); /* Draw terrain */
	draw_lander();
	draw_sparks(&lander, YELLOW);
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
