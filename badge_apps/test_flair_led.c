/*********************************************

Test flair LED

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
#endif

#include "build_bug_on.h"

/* Program states.  Initial state is TEST_FLAIR_INIT */
enum test_flair_state_t {
	TEST_FLAIR_INIT,
	TEST_FLAIR_RUN,
	TEST_FLAIR_EXIT,
};

static enum test_flair_state_t test_flair_state = TEST_FLAIR_INIT;

static void test_flair_init(void)
{
	FbInit();
	FbClear();
	FbColor(WHITE);
	FbMove(2, LCD_YSIZE / 2);
	FbWriteString("TESTING\nFLAIR\nLED");
	FbSwapBuffers();
	test_flair_state = TEST_FLAIR_RUN;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		test_flair_state = TEST_FLAIR_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
	} else if (RIGHT_BTN_AND_CONSUME) {
	} else if (UP_BTN_AND_CONSUME) {
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void update_led_color()
{
	static unsigned char r = 0;
	static unsigned char g = 0;
	static unsigned char b = 0;

	r = (r + 1) & 0x0ff;
	g = (g + 2) & 0x0ff;
	b = (g + 3) & 0x0ff;

	flareled(r, g, b);
}

static void test_flair_run()
{
	check_buttons();
	update_led_color();
}

static void test_flair_exit()
{
	test_flair_state = TEST_FLAIR_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename test_flair_cb() something else. */
int test_flair_cb(void)
{
	switch (test_flair_state) {
	case TEST_FLAIR_INIT:
		test_flair_init();
		break;
	case TEST_FLAIR_RUN:
		test_flair_run();
		break;
	case TEST_FLAIR_EXIT:
		test_flair_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, test_flair_cb, 30);
        return 0;
}
#endif
