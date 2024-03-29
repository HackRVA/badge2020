/*********************************************
 Display a link to provide more information about
 the badge.
**********************************************/
#ifdef __linux__
#include <stdio.h>
#include <sys/time.h> /* for gettimeofday */
#include <string.h> /* for memset */

#include "../linux/linuxcompat.h"
#include "../linux/bline.h"
#else
#include <string.h>
#include "colors.h"
#include "menu.h"
#include "buttons.h"
#include "fb.h"
#endif

#include "build_bug_on.h"

/* Program states.  Initial state is ABOUT_BADGE_INIT */
enum about_badge_state_t {
	ABOUT_BADGE_INIT,
	ABOUT_BADGE_RUN,
	ABOUT_BADGE_EXIT,
};

static enum about_badge_state_t about_badge_state = ABOUT_BADGE_INIT;
static int screen_changed = 0;

static void about_badge_init(void)
{
	FbInit();
	about_badge_state = ABOUT_BADGE_RUN;
	screen_changed = 1;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		about_badge_state = ABOUT_BADGE_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
	} else if (RIGHT_BTN_AND_CONSUME) {
	} else if (UP_BTN_AND_CONSUME) {
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void draw_screen()
{
	if (!screen_changed)
		return;
	FbColor(WHITE);
	FbClear();
	FbMove(2, 2);
	FbWriteString("THIS BADGE WAS\nBUILT AND\nPROGRAMMED BY\nHACKRVA MEMBERS\n"
			"\nVISIT\n\nhttps://\nhackrva.github.\nio/badge2020/\n\nFOR MORE\nINFORMATION");
	FbSwapBuffers();
	screen_changed = 0;
}

static void about_badge_run()
{
	check_buttons();
	draw_screen();
}

static void about_badge_exit()
{
	about_badge_state = ABOUT_BADGE_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

int about_badge_cb(void)
{
	switch (about_badge_state) {
	case ABOUT_BADGE_INIT:
		about_badge_init();
		break;
	case ABOUT_BADGE_RUN:
		about_badge_run();
		break;
	case ABOUT_BADGE_EXIT:
		about_badge_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, about_badge_cb, 30);
        return 0;
}
#endif
