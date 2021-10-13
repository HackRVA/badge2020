#ifdef __linux__
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "../linux/linuxcompat.h"
#include "../linux/bline.h"
#else
#include "colors.h"
#include "buttons.h"
#include "fb.h"
#endif

#include <string.h>

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define SHIFT (8)

static enum cube_state_t {
	CUBE_INIT,
	CUBE_RUN,
	CUBE_EXIT,
} cube_state = CUBE_INIT;

#define TRIG_DIVISOR 256
static int ez = 60 << SHIFT;
static int cubescale = 8 << SHIFT;

static struct fixed_vec3 { /* fixed point vec3 */
	int x, y, z;
} cubept2[] = {
	{ 1, 1, 1 },
	{ -1, 1, 1 },
	{ -1, -1, 1 },
	{ 1, -1, 1 },
	{ 1, 1, -1 },
	{ -1, 1, -1 },
	{ -1, -1, -1 },
	{ 1, -1, -1 },
};

struct fixed_vec3 cubept3[ARRAYSIZE(cubept2)];
struct fixed_vec3 cubept[ARRAYSIZE(cubept2)];

static int cubetransx = 0;
static int cubetransy = 0;
static int cubetransz = 20 << SHIFT;

static int angle = 0;
static int angle2 = 0;
static int angle3 = 0;

char cube[] = { 0, 1, 2, 3, 0, 4, 5, 6, 7, 4,
		-1, 7, 3, -1, 2, 6, -1, 5, 1, -1};

static void cube_init(void)
{
	int i;
	FbInit();
	FbClear();
	FbSwapBuffers();
	cube_state = CUBE_RUN;
	for (i = 0; (size_t) i < ARRAYSIZE(cubept2); i++) {
		cubept2[i].x = cubept2[i].x << SHIFT;
		cubept2[i].y = cubept2[i].y << SHIFT;
		cubept2[i].z = cubept2[i].z << SHIFT;
	}
}

static void yrotate(struct fixed_vec3 *p1, struct fixed_vec3 *p2, int npoints, int angle)
{
	int i;
	int cosa, sina;

	cosa = cosine(angle);
	sina = sine(angle);

	for (i = 0; i < npoints; i++) {
		p2[i].x = (p1[i].x * cosa - p1[i].z * sina) / TRIG_DIVISOR;
		p2[i].z = (p1[i].x * sina + p1[i].z * cosa) / TRIG_DIVISOR;
		p2[i].y = p1[i].y;
	}
}

static void zrotate(struct fixed_vec3 *p1, struct fixed_vec3 *p2, int npoints, int angle)
{
	int i;
	int cosa, sina;

	cosa = cosine(angle);
	sina = sine(angle);


	for (i = 0; i < npoints; i++) {
		p2[i].x = (p1[i].x * cosa - p1[i].y * sina) / TRIG_DIVISOR;
		p2[i].y = (p1[i].x * sina + p1[i].y * cosa) / TRIG_DIVISOR;
		p2[i].z = p1[i].z;
	}
}

static void xrotate(struct fixed_vec3 *p1, struct fixed_vec3 *p2, int npoints, int angle)
{
	int i;
	int cosa, sina;

	cosa = cosine(angle);
	sina = sine(angle);

	for (i = 0; i < npoints; i++) {
		p2[i].y = (p1[i].y * cosa - p1[i].z * sina) / TRIG_DIVISOR;
		p2[i].z = (p1[i].y * sina + p1[i].z * cosa) / TRIG_DIVISOR;
		p2[i].x = p1[i].x;
	}
}

static void perspective(int x, int y, int z, int *dx, int *dy)
{
	const int centerx = (LCD_XSIZE << SHIFT) / 2;
	const int centery = (LCD_YSIZE << SHIFT) / 2;

	*dx = + (x * ez) / z;
	*dy = + (y * ez) / z;

	*dx += centerx;
	*dy += centery;
}

static void draw_cube(void)
{
	int i, n;
	int x, y, z, lx, ly, nx, ny;
	char x1, y1, x2, y2;

	n = cube[0];
	x = ((cubept[n].x * cubescale) >> SHIFT) + cubetransx;
	y = ((cubept[n].y * cubescale) >> SHIFT) + cubetransy;
	z = ((cubept[n].z * cubescale) >> SHIFT) + cubetransz;
	perspective(x, y, z, &lx, &ly);

	for (i = 1; (size_t) i < ARRAYSIZE(cube); i++) {
		n = cube[i];
		if (n == -1)
			continue;
		x = ((cubept[n].x * cubescale) >> SHIFT) + cubetransx;
		y = ((cubept[n].y * cubescale) >> SHIFT) + cubetransy;
		z = ((cubept[n].z * cubescale) >> SHIFT) + cubetransz;
		perspective(x, y, z, &nx, &ny);

		x1 = (char) (lx >> SHIFT);
		y1 = (char) (ly >> SHIFT);
		x2 = (char) (nx >> SHIFT);
		y2 = (char) (ny >> SHIFT);

		lx = nx;
		ly = ny;
		if (x1 < 0 || x2 < 0)
			continue;
		if (y1 < 0 || y2 < 0)
			continue;
		FbLine(x1, y1, x2, y2);
	}
}

static void docube(void)
{
	memcpy(&cubept, &cubept2, sizeof(cubept2));
	FbColor(WHITE);
	yrotate(cubept2, cubept, ARRAYSIZE(cubept), angle);
	zrotate(cubept, cubept3, ARRAYSIZE(cubept), angle2);
	xrotate(cubept3, cubept, ARRAYSIZE(cubept), angle2);
	angle2 += 1;
	if (angle2 > 127)
		angle2 = 0;
	angle += 1;
	if (angle > 127)
		angle = 0;
	angle3 += 1;
	if (angle3 > 127)
		angle3 = 0;
	FbColor(WHITE);
	draw_cube();
	FbPaintNewRows();
	FbColor(BLACK);
	draw_cube();
}

static void check_buttons(void)
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		cube_state = CUBE_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
	} else if (RIGHT_BTN_AND_CONSUME) {
	} else if (UP_BTN_AND_CONSUME) {
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void cube_run(void)
{
	check_buttons();
	docube();
}

int cube_cb(void)
{
	switch (cube_state) {
	case CUBE_INIT:
		cube_init();
		break;
	case CUBE_RUN:
		cube_run();
		break;
	case CUBE_EXIT:
		returnToMenus();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, cube_cb, 30);
        return 0;
}
#endif



