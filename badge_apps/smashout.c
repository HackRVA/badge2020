/*********************************************

 Smashout game (like breakout) for HackRVA 2020 badge

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2019 Stephen M. Cameron

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

/* TODO: I shouldn't have to declare these myself. */
#define size_t int
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcat(char *dest, const char *src);

#endif

#include "build_bug_on.h"
#include "xorshift.h"
#include "achievements.h"

/* TODO figure out where these should really come from */
#define SCREEN_XDIM 132
#define SCREEN_YDIM 132

/* Program states.  Initial state is SMASHOUT_GAME_INIT */
enum smashout_program_state_t {
	SMASHOUT_GAME_INIT,
	SMASHOUT_GAME_PLAY,
	SMASHOUT_GAME_EXIT,
};

static enum smashout_program_state_t smashout_program_state = SMASHOUT_GAME_INIT;

static void smashout_game_init(void)
{
}

static void smashout_game_play()
{
}

static void smashout_game_exit()
{
}

int smashout_cb(void)
{
	switch (smashout_program_state) {
	case SMASHOUT_GAME_INIT:
		smashout_game_init();
		break;
	case SMASHOUT_GAME_PLAY:
		smashout_game_play();
		break;
	case SMASHOUT_GAME_EXIT:
		smashout_game_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, smashout_cb, 240);
        return 0;
}
#endif
