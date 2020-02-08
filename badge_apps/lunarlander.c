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

#include "build_bug_on.h"

#define SCREEN_XDIM 132
#define SCREEN_YDIM 132

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

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

struct lander_data {
	/* All values are 8x what is displayed. */
	int x, y;
	int vx, vy;
	int fuel;
} lander, oldlander;

static enum lunarlander_state_t lunarlander_state = LUNARLANDER_INIT;

static void lunarlander_init(void)
{
	FbInit();
	FbClear();
	lunarlander_state = LUNARLANDER_RUN;
	lander.x = 30 * 8;
	lander.y = 30 * 8;
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
	} else if (RIGHT_BTN_AND_CONSUME) {
	} else if (UP_BTN_AND_CONSUME) {
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

static void move_lander(void)
{
}

static void draw_screen()
{
	FbColor(WHITE);
	draw_lander();
	move_lander();
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
        start_gtk(&argc, &argv, lunarlander_cb, 30);
        return 0;
}
#endif
