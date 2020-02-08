
#ifdef __linux__

#include "../linux/linuxcompat.h"

#else

#include "colors.h"
#include "menu.h"
#include "buttons.h"

extern int timestamp;

/* TODO: I shouldn't have to declare these myself. */
#define size_t int
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcat(char *dest, const char *src);


#endif

#define INIT_APP_STATE 0
#define RENDER_SCREEN 1
#define CHECK_THE_BUTTONS 2
#define EXIT_APP 3

static void app_init(void);
static void render_screen(void);
static void check_the_buttons(void);
static void exit_app(void);

typedef void (*state_to_function_map_fn_type)(void);

static state_to_function_map_fn_type state_to_function_map[] = {
	app_init,
	render_screen,
	check_the_buttons,
	exit_app,
};

static struct point smiley[] =
#include "badge_monster_drawings/smileymon.h"

static int app_state = INIT_APP_STATE;

static int smiley_x, smiley_y;

#define SCREEN_XDIM 132
#define SCREEN_YDIM 132

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

static void render_screen(void)
{
	char buffer[10];
	FbClear();
	FbDrawObject(smiley, ARRAYSIZE(smiley), WHITE, smiley_x, smiley_y, 410);
	/* Display the time stamp for no particular reason */
	itoa(buffer, (volatile int) timestamp, 10);
	FbMove(10, 100);
	FbWriteLine(buffer);
	FbSwapBuffers();
	app_state = CHECK_THE_BUTTONS;
}

static void check_the_buttons(void)
{
	const int left_limit = SCREEN_XDIM / 3;
	const int right_limit = 2 * SCREEN_XDIM / 3;
	const int top_limit = SCREEN_YDIM / 3;
	const int bottom_limit = 2 * SCREEN_YDIM / 3;

	int something_changed = 0;

	if (UP_BTN_AND_CONSUME) {
		smiley_y -= 1;
		something_changed = 1;
	} else if (DOWN_BTN_AND_CONSUME) {
		smiley_y += 1;
		something_changed = 1;
	} else if (LEFT_BTN_AND_CONSUME) {
		smiley_x -= 1;
		something_changed = 1;
	} else if (RIGHT_BTN_AND_CONSUME) {
		smiley_x += 1;
		something_changed = 1;
	} else if (BUTTON_PRESSED_AND_CONSUME) {
		app_state = EXIT_APP;
	}
	if (smiley_x < left_limit)
		smiley_x = left_limit;
	if (smiley_x > right_limit)
		smiley_x = right_limit;
	if (smiley_y < top_limit)
		smiley_y = top_limit;
	if (smiley_y > bottom_limit)
		smiley_y = bottom_limit;
	if (something_changed && app_state == CHECK_THE_BUTTONS)
		app_state = RENDER_SCREEN;
        return;
}

static void exit_app(void)
{
	app_state = INIT_APP_STATE;
	returnToMenus();
}

static void app_init(void)
{
	FbInit();
	app_state = INIT_APP_STATE;
	smiley_x = SCREEN_XDIM / 2;
	smiley_y = SCREEN_XDIM / 2;
	app_state = RENDER_SCREEN;
}

int sample_app_cb(void)
{
	state_to_function_map[app_state]();
	return 0;
}

#ifdef __linux__

int main(int argc, char *argv[])
{
	start_gtk(&argc, &argv, sample_app_cb, 30);
	return 0;
}

#endif
