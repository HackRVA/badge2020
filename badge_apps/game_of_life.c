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

/* Program states.  Initial state is GAME_OF_LIFE_INIT */
enum game_of_life_state_t
{
	GAME_OF_LIFE_INIT,
	GAME_OF_LIFE_RUN,
	GAME_OF_LIFE_EXIT,
};

static enum game_of_life_state_t game_of_life_state = GAME_OF_LIFE_INIT;

static void game_of_life_init(void)
{
	FbInit();
	FbClear();
	game_of_life_state = GAME_OF_LIFE_RUN;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME)
	{
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
	returnToMenus();
}

int game_of_life_cb(void)
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
