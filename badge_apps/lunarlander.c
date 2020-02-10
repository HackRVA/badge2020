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

static const struct point lander_points[] = {
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
#define FULL_FUEL (1 << 16)
#define HORIZONTAL_FUEL (256)
#define VERTICAL_FUEL (768)
	int alive;
} lander, oldlander;

#define NUM_LANDING_ZONES 5
struct fuel_tank {
	int x, y;
} fueltank[NUM_LANDING_ZONES];

#define MAXSPARKS 50
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

static void explosion(struct lander_data *lander)
{
	int i;

	for (i = 0; i < MAXSPARKS; i++) {
		spark[i].x = lander->x;
		spark[i].y = lander->y;
		spark[i].vx = (xorshift(&xorshift_state) & 0xff) - 128;
		spark[i].vy = (xorshift(&xorshift_state) & 0xff) - 128;
		spark[i].alive = 100;
	}
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

static void draw_fuel_gauge_ticks(void)
{
	int i;
	FbColor(WHITE);
	FbVerticalLine(127, 5, 127, 105);
	for (i = 0; i <= 10; i++)
		FbHorizontalLine(120, 5 + i * 10, 126, 5 + i * 10);
}

static void draw_fuel_gauge_marker(struct lander_data *lander, int color)
{
	int y1, y2, y3;

	y2 = 106 - ((100 * lander->fuel) >> 16);
	y1 = y2 - 5;
	y3 = y2 + 5;
	FbColor(color);
	FbVerticalLine(115, y1, 115, y3);
	FbLine(115, y1, 122, y2);
	FbLine(115, y3, 122, y2);
}

static void draw_fuel_gauge(struct lander_data *lander, int color)
{
	if (color != BLACK)
		draw_fuel_gauge_ticks();
	draw_fuel_gauge_marker(lander, color);
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

	/* Make sure we don't make accidental landing zones */
	if (midy == terrain_y[start])
		midy = midy + 1;
	if (midy == terrain_y[stop])
		midy = midy + 1;

	/* Displace midy randomly here in proportion to (stop - start). */
	n = (xorshift(&xorshift_state) % 10000) - 5000;
	n = 3 * (n * (stop - start)) / 10000;
	midy += n;

	terrain_y[mid] = midy;
	init_terrain(terrain_y, start, mid);
	init_terrain(terrain_y, mid, stop);
	lander_time = 0;
}

static void add_landing_zones(int t[], int start, int stop, int count)
{
	int d = (stop - start) / (count + 1);
	int i, x;

	x = d;

	/* Flatten out some terrain for a landing zone. */
	for (i = 0; i < count; i++) {
		t[x] = t[x + 1];
		t[x + 2] = t[x];
		fueltank[i].x = x;
		fueltank[i].y = 0;
		x += d;
	}
}

static void lunarlander_init(void)
{
	FbInit();
	FbClear();
	terrain_y[0] = -100;
	terrain_y[1023] = -100;
	init_terrain(terrain_y, 0, 1023);
	add_landing_zones(terrain_y, 0, 1023, NUM_LANDING_ZONES);
	lunarlander_state = LUNARLANDER_RUN;
	lander.x = 100 << 8;
	lander.y = (terrain_y[9] - 60) << 8;
	lander.vx = 0;
	lander.vy = 0;
	lander.fuel = FULL_FUEL;
	lander.alive = 1;
	oldlander = lander;
}

static void reduce_fuel(struct lander_data *lander, int amount)
{
	draw_fuel_gauge(lander, BLACK);
	lander->fuel -= amount;
	if (lander->fuel < 0)
		lander->fuel = 0;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		lunarlander_state = LUNARLANDER_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
		if (lander.fuel > 0) {
			lander.vx = lander.vx - (1 << 7);
			add_sparks(&lander, lander.vx + (5 << 8), lander.vy + 0, 5);
			reduce_fuel(&lander, HORIZONTAL_FUEL);
		}
	} else if (RIGHT_BTN_AND_CONSUME) {
		if (lander.fuel > 0) {
			lander.vx = lander.vx + (1 << 7);
			add_sparks(&lander, lander.vx - (5 << 8), lander.vy + 0, 5);
			reduce_fuel(&lander, HORIZONTAL_FUEL);
		}
	} else if (UP_BTN_AND_CONSUME) {
		if (lander.fuel > 0) {
			lander.vy = lander.vy - (1 << 7);
			add_sparks(&lander, lander.vx + 0, lander.vy + (5 << 8), 5);
			reduce_fuel(&lander, VERTICAL_FUEL);
		}
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void draw_lander(void)
{
	const int x = SCREEN_XDIM / 2;
	const int y = SCREEN_YDIM / 3;
	FbDrawObject(lander_points, ARRAYSIZE(lander_points), BLACK, x, y, 1024);
	if (lander.alive > 0)
		FbDrawObject(lander_points, ARRAYSIZE(lander_points), WHITE, x, y, 1024);
}

static void draw_terrain_segment(struct lander_data *lander, int i, int color)
{
	int x1, y1, x2, y2, sx1, sy1, sx2, sy2, j;
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
		if ((lander->y >> 8) >= y2 - 8) {
			if (lander->alive > 0 && y2 != y1) {
				/* Explode lander if not on level ground */
				FbColor(color);
				explosion(lander);
				lander->alive = -100;
			} else {
				/* Explode lander if it hits too hard. */
				if ((lander->vy > 256 || lander->vx > 256 || lander->vx < -256) && lander->alive > 0) {
					explosion(lander);
					lander->alive = -100;
				} else {
					if (lander->vy > 0)
						/* Allow lander to land */
						lander->vy = 0;
						for (j = 0; j < NUM_LANDING_ZONES; j++) {
							if (abs((lander->x >> 8) - fueltank[j].x * TERRAIN_SEGMENT_WIDTH) < 2 * TERRAIN_SEGMENT_WIDTH) {
								draw_fuel_gauge(lander, BLACK);
								lander->fuel = FULL_FUEL; /* refuel lander */
							}
						}
				}
			}
		} else {
			FbColor(color);
		}
	} else {
#if 0
		if (y2 == y1 && color != BLACK) /* Make landing zones green */
			FbColor(GREEN);
		else
#endif
			FbColor(color);
	}
	FbLine(sx1, sy1, sx2, sy2);
	/* Draw a flag by landing zones. */
	for (j = 0; j < NUM_LANDING_ZONES; j++) {
		if (i == fueltank[j].x) {
			int y2 = sy1 - 20;
			if (y2 < 0)
				y2 = 0;
			if (color != BLACK)
				FbColor(GREEN);
			FbLine(sx2, sy1, sx2, y2);
			if (y2 < 120 && sx2 < 120) {
				FbLine(sx2, y2, sx2 + 7, y2 + 3);
				FbLine(sx2 + 7, y2 + 3, sx2, y2 + 6);
			}
			break;
		}
	}
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
	if (stop > 1023)
		stop = 1023;
	FbColor(color);
	for (i = start; i < stop; i++)
		draw_terrain_segment(lander, i, color);
}

static void move_lander(void)
{
	if (lander.alive < 0) {
		lander.alive++;
		if (lander.alive == 0) {
			lunarlander_state = LUNARLANDER_INIT;
		}
		return;
	}
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
	draw_fuel_gauge(&lander, RED);
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
