/*********************************************

 Basic star trek type game

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2021 Stephen M. Cameron

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

/* TODO: This should be more convenient. */
#ifndef NULL
#define NULL 0
#endif

#include "dynmenu.h"
#include "xorshift.h"
static unsigned int xorshift_state = 0xa5a5a5a5;

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define NKLINGONS 25
#define NCOMMANDERS 5
#define NROMULANS 10
#define NPLANETS 50
#define NBLACKHOLES 10
#define NSTARBASES 25
#define NSTARS 40
#define NTOTAL (NKLINGONS + NCOMMANDERS + NROMULANS + NPLANETS + NBLACKHOLES + NSTARBASES + NSTARS)

#define ENEMY_SHIP 'E'
#define PLANET 'P'
#define BLACKHOLE 'B'
#define STARBASE 'S'
#define STAR '*'

/* Program states.  Initial state is MAZE_GAME_INIT */
enum st_program_state_t {
	ST_GAME_INIT = 0,
	ST_NEW_GAME,
	ST_CAPTAIN_MENU,
	ST_PROCESS_INPUT,
	ST_LRS,
	ST_SRS,
	ST_SET_COURSE,
	ST_ENGAGE_WARP,
	ST_ENGAGE_IMPULSE,
	ST_SET_WEAPONS_BEARING,
	ST_FIRE_PHASERS,
	ST_FIRE_PHOTON_TORPEDO,
	ST_GAME_LOST,
	ST_GAME_WON,
	ST_DRAW_MENU,
	ST_RENDER_SCREEN,
	ST_EXIT,
	ST_PLANETS,
	ST_SENSORS,
	ST_DAMAGE_REPORT,
	ST_STATUS_REPORT,
	ST_NOT_IMPL,
};

static struct dynmenu menu;

static enum st_program_state_t st_program_state = ST_GAME_INIT;

struct enemy_ship {
	char hitpoints;
	char shiptype;
};

const char *planet_class = "MNOGRVI?";
static char *planet_class_name[] = {
		"HUMAN SUITABLE",
		"LOW GRAVITY",
		"MOSTLY WATER",
		"GAS GIANT",
		"ROCKY PLANETOID",
		"VENUS-LIKE",
		"ICEBALL/COMET",
};
struct planet {
	char flags;
#define PLANET_INHABITED	(1 << 0)
#define PLANET_KNOWN		(1 << 1)
#define PLANET_SCANNED		(1 << 2)
#define PLANET_HAS_DILITHIUM	(1 << 3)
#define PLANET_UNUSED_FLAG	(1 << 4);
#define PLANET_CLASS(x) (x >> 5) & 0x7
#define PLANET_CLASS_M 0x000 /* suitable for human life */
#define PLANET_CLASS_N 0x001 /* low gravity */
#define PLANET_CLASS_O 0x010 /* mostly water */
#define PLANET_CLASS_G 0x011 /* gas giant */
#define PLANET_CLASS_R 0x100 /* rocky planetoid */
#define PLANET_CLASS_V 0x101 /* venus like */
#define PLANET_CLASS_I 0x110 /* iceball/comet */
};

const char *star_class = "OBAFGKM";
struct star {
	/* Low three bits index into star_class for Morgan-keenan system. */
	/* bits 3 - 6 are 0 - 9 indicating temperature. */
	/* See https://en.wikipedia.org/wiki/Stellar_classification */
	char class;
};

union type_specific_data {
	struct enemy_ship ship;
	struct planet planet;
	struct star star;
};

struct game_object {
	int x, y;
	union type_specific_data tsd;
	char type;
};

static const char *ship_system[] = { "WARP", "IMPULSE", "SHIELDS", "LIFE SUPP", "PHASERS"};
#define NSHIP_SYSTEMS ARRAYSIZE(ship_system)

struct player_ship {
	int x, y;
	int heading, new_heading; /* in degrees */
	int energy;
	unsigned char shields;
	unsigned char shields_up;
	unsigned char dilithium_crystals;
	unsigned char damage[NSHIP_SYSTEMS];
};

struct game_state {
	struct game_object object[NTOTAL];
	struct player_ship player;
#define TICKS_PER_DAY 256  /* Each tick is about 5.6 minutes of game time */
#define START_DATE (2623)
	short stardate;
	short enddate;
} gs = { 0 };

static int find_free_obj(void)
{
	int i;

	for (i = 0; i < NTOTAL; i++) {
		if (gs.object[i].type == 0)
			return i;
	}
	return -1;
}

static int add_object(unsigned char type, int x, int y)
{
	int i;

	i = find_free_obj();
	if (i < 0)
		return -1;
	gs.object[i].type = type;
	gs.object[i].x = x;
	gs.object[i].y = y;
	return i;
}

static void delete_object(int i)
{
	if (i < 0 || i >= NTOTAL)
		return;
	memset(&gs.object[i], 0, sizeof(gs.object[i]));
}

static void clear_menu(void)
{
	dynmenu_clear(&menu);
	dynmenu_set_colors(&menu, GREEN, WHITE);
}

static void setup_main_menu(void)
{
	clear_menu();
	strcpy(menu.title, "SPACE TRIPPER");
	dynmenu_add_item(&menu, "NEW GAME", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT", ST_EXIT, 0);
	menu.menu_active = 1;
}

static void st_draw_menu(void)
{
	dynmenu_draw(&menu);
	st_program_state = ST_RENDER_SCREEN;
}

static void st_game_init(void)
{
	FbInit();
	FbColor(WHITE);
	FbBackgroundColor(BLACK);
	FbClear();
	FbSwapBuffers();
	setup_main_menu();
	st_draw_menu();
	st_program_state = ST_RENDER_SCREEN;
}

static void st_captain_menu(void)
{
	clear_menu();
	strcpy(menu.title, "CAPN'S ORDERS?");
	dynmenu_add_item(&menu, "LRS", ST_LRS, 0);
	dynmenu_add_item(&menu, "SRS", ST_SRS, 0);
	dynmenu_add_item(&menu, "STAR CHART", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "SET COURSE", ST_SET_COURSE, 0);
	dynmenu_add_item(&menu, "IMPULSE", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "WARP", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "PHASERS", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "PHOTON TORPEDO", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "SHIELDS", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "DAMAGE REPORT", ST_DAMAGE_REPORT, 0);
	dynmenu_add_item(&menu, "STATUS REPORT", ST_STATUS_REPORT, 0);
	dynmenu_add_item(&menu, "SENSORS", ST_SENSORS, 0);
	dynmenu_add_item(&menu, "DOCK", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "STANDARD ORBIT", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "TRANSPORTER", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "MINE DILITHIUM", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "LOAD DILITHIUM", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "SELF DESTRUCT", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "PLANETS", ST_PLANETS, 0);
	dynmenu_add_item(&menu, "NEW GAME", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT GAME", ST_EXIT, 0);
	menu.menu_active = 1;
	st_draw_menu();
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_game_lost(void)
{
}

static void st_game_won(void)
{
}

static int random_coordinate(void)
{
	return xorshift(&xorshift_state) & 0x3f;
}

static void planet_customizer(int i)
{
	gs.object[i].tsd.planet.flags = xorshift(&xorshift_state) & 0xff;
	gs.object[i].tsd.planet.flags &= ~PLANET_KNOWN; /* initially unknown */
	gs.object[i].tsd.planet.flags &= ~PLANET_SCANNED; /* initially unscanned */
}

static void star_customizer(int i)
{
	gs.object[i].tsd.star.class = xorshift(&xorshift_state) & 0xff;
}

typedef void (*object_customizer)(int index);
static void add_simple_objects(int count, unsigned char type, object_customizer customize)
{
	int i, k;

	for (i = 0; i < count; i++) {
		k = add_object(type, random_coordinate(), random_coordinate());
		if (customize && k != -1)
			customize(k);
	}
}

static void add_enemy_ships(int count, unsigned char shiptype, unsigned char hitpoints)
{
	int i, k;

	for (i = 0; i < count; i++) {
		k = add_object(ENEMY_SHIP, random_coordinate(), random_coordinate());
		gs.object[k].tsd.ship.shiptype = shiptype;
		gs.object[k].tsd.ship.hitpoints = hitpoints;
	}
}

static void init_player()
{
	int i;

	gs.player.x = random_coordinate();
	gs.player.y = random_coordinate();
	gs.player.heading = 0;
	gs.player.new_heading = 0;
	gs.player.energy = 10000;
	gs.player.shields = 255;
	gs.player.shields_up = 0;
	gs.player.dilithium_crystals = 255;

	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++)
		gs.player.damage[i] = 0;
}

static void st_new_game(void)
{
	memset(&gs, 0, sizeof(gs));
	add_enemy_ships(NKLINGONS, 'K', 100);
	add_enemy_ships(NCOMMANDERS, 'C', 200);
	add_enemy_ships(NROMULANS, 'R', 250);
	add_simple_objects(NBLACKHOLES, BLACKHOLE, NULL);
	add_simple_objects(NSTARBASES, STARBASE, NULL);
	add_simple_objects(NSTARS, STAR, star_customizer);
	add_simple_objects(NPLANETS, PLANET, planet_customizer);
	init_player();
	gs.stardate = 0;
	gs.enddate = gs.stardate + 35 * TICKS_PER_DAY;
	st_program_state = ST_CAPTAIN_MENU;
}

static void st_render_screen(void)
{
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void button_pressed()
{
	if (!menu.menu_active) {
		menu.menu_active = 1;
	} else {
		st_program_state = menu.item[menu.current_item].next_state;
		menu.menu_active = 0;
		FbClear();
	}
}

static void do_something(void)
{
}

static void print_current_sector(void)
{
	char sx[5], sy[5];
	char msg[40];
	itoa(sx, gs.player.x >> 3, 10);
	itoa(sy, gs.player.y >> 3, 10);
	strcpy(msg, "SECTOR: (");
	strcat(msg, sx);
	strcat(msg, ",");
	strcat(msg, sy);
	strcat(msg, ")");
	FbWriteLine(msg);
}

static void print_current_quadrant(void)
{
	char sx[5], sy[5];
	char msg[40];
	itoa(sx, gs.player.x & 0x7, 10);
	itoa(sy, gs.player.y & 0x7, 10);
	strcpy(msg, "QUADRANT: (");
	strcat(msg, sx);
	strcat(msg, ",");
	strcat(msg, sy);
	strcat(msg, ")");
	FbWriteLine(msg);
}

static void print_current_heading(void)
{
	char msg[40];
	char heading[5];
	itoa(heading, gs.player.heading, 10);
	strcpy(msg, "BEARING: ");
	strcat(msg, heading);
	FbWriteLine(msg);
}

static void print_sector_quadrant_heading(void)
{
	FbColor(GREEN);
	FbMove(2, 10);
	print_current_sector();
	FbMove(2, 19);
	print_current_quadrant();
	FbMove(2, 28);
	print_current_heading();
}

static void single_digit_to_dec(int digit, char *num)
{
	if (digit < 0) {
		num[0] = '-';
		num[1] = -digit + '0';
		num[2] = '\0';
	} else {
		num[0] = digit + '0';
		num[1] = '\0';
	}
}

static void screen_header(char *title)
{

	FbClear();
	FbMove(2, 2);
	FbColor(WHITE);
	FbWriteLine(title);
}

static void st_lrs(void) /* long range scanner */
{
	int i, x, y, place;
	int sectorx, sectory, sx, sy;
	char scan[3][3][3];
	const int color[] = { WHITE, CYAN, YELLOW };
	char num[4];

	sectorx = gs.player.x >> 3;
	sectory = gs.player.y >> 3;
	memset(scan, 0, sizeof(scan));

	/* Count up nearby enemy ships, starbases and stars, and store counts in scan[][][] */
	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			place = 0;
			break;
		case STARBASE:
			place = 1;
			break;
		case STAR:
			place = 2;
			break;
		default:
			continue;
		}
		sx = gs.object[i].x >> 3;
		sy = gs.object[i].y >> 3;
		if (sx > sectorx + 1 || sx < sectorx - 1)
			continue;
		if (sy > sectory + 1 || sy < sectory - 1)
			continue;
		/* Calculate sector relative to player in range -1 - +1, then add 1 to get into range 0 - 2 */
		x = sx - sectorx + 1;
		y = sy - sectory + 1;
		scan[x][y][place]++;
	}

	screen_header("LONG RANGE SCAN");
	print_sector_quadrant_heading();

	FbColor(CYAN);
	for (x = -1; x < 2; x++) { /* Print out X coordinates */
		single_digit_to_dec(x + (gs.player.x >> 3), num);
		FbMove(35 + (x + 1) * 30, 40);
		FbWriteLine(num);
	}

	for (y = -1; y < 2; y++) { /* Print out Y coordinates */
		single_digit_to_dec(y + (gs.player.y >> 3), num);
		FbMove(5, (y + 1) * 10 + 50);
		FbWriteLine(num);
	}

	/* Print out scan[][][] */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			for (i = 0; i < 3; i++) {
				char digit[5];

				FbColor(color[i]);
				itoa(digit, scan[x][y][i], 10);
				FbMove(25 + x * 30 + i * 8, y * 10 + 50);
				FbWriteLine(digit);
			}
		}
		FbColor(GREEN);
		FbHorizontalLine(22, y * 10 + 49, 132 - 21, y * 10 + 49);
	}
	FbHorizontalLine(22, y * 10 + 49, 132 - 21, y * 10 + 49);
	for (i = 0; i < 4; i++)
		FbVerticalLine(22 + i * 30, 49, 22 + i * 30,  y * 10 + 49);

	FbMove(2, 89);
	FbColor(color[0]);
	FbWriteLine("# STARS");
	FbMove(2, 98);
	FbColor(color[1]);
	FbWriteLine("# STARBASES");
	FbMove(2, 107);
	FbColor(color[2]);
	FbWriteLine("# ENEMY SHIPS");
	
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_srs(void) /* short range scanner */
{
	int i, x, y;
	int sectorx, sectory, sx, sy, qx, qy;
	char scan[3][3][3];
	char c[2] = { '.', '\0' };
	int color;
	const int quadrant_width = 10;
	char num[4];
	const int left = 16;

	sectorx = gs.player.x >> 3;
	sectory = gs.player.y >> 3;
	memset(scan, 0, sizeof(scan));

	screen_header("SHORT RANGE SCAN");
	print_sector_quadrant_heading();

	/* Draw a grid */
	FbColor(BLUE);
	for (y = 0; y < 9; y++)
		FbHorizontalLine(left - 2, 45 + y * quadrant_width, left - 2 + 8 * quadrant_width, 45 + y * quadrant_width);
	for (x = 0; x < 9; x++)
		FbVerticalLine(left - 2 + x * quadrant_width, 45, left - 2 + x * quadrant_width, 45 + 8 * quadrant_width);

	/* Draw X coordinates */
	FbColor(CYAN);
	num[1] = '\0';
	for (x = 0; x < 8; x++) {
		num[0] = '0' + x;
		FbMove(left + x * quadrant_width, 37);
		FbWriteLine(num);
	}
	/* Draw Y coordinates */
	for (y = 0; y < 8; y++) {
		num[0] = '0' + y;
		FbMove(4, 45 + y * quadrant_width);
		FbWriteLine(num);
	}

	/* Find objects in this sector */
	for (i = 0; i < NTOTAL; i++) {

		if (gs.object[i].type == 0)
			continue;

		sx = gs.object[i].x >> 3;
		sy = gs.object[i].y >> 3;

		if (sx != sectorx || sy != sectory)
			continue;

		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			color = WHITE;
			c[0] = gs.object[i].tsd.ship.shiptype;
			break;
		case PLANET:
			color = CYAN;
			c[0] = 'O';
			gs.object[i].tsd.planet.flags |= PLANET_KNOWN;
			break;
		case STAR:
			color = YELLOW;
			c[0] = '*';
			break;
		case STARBASE:
			color = WHITE;
			c[0] = 'S';
			break;
		case BLACKHOLE:
			color = WHITE;
			c[0] = 'B';
			break;
		default:
			color = YELLOW;
			c[0] = '?';
			break;
		}
		qx = gs.object[i].x & 0x7;
		qy = gs.object[i].y & 0x7;
		FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
		FbColor(color);
		FbWriteLine(c);
	}

	/* Draw player */
	qx = gs.player.x & 0x7;
	qy = gs.player.y & 0x7;
	FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
	FbColor(WHITE);
	FbWriteLine("E");

	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void write_heading(char *msg, int heading, int color)
{
	char out[40];
	char b[5];

	itoa(b, heading, 10);
	strcpy(out, msg);
	strcat(out, b);
	FbColor(color);
	FbWriteLine(out);
}

static void draw_heading_indicator(const int cx, const int cy, const int degrees, const int length, const int color)
{
	int a, x, y;

	FbColor(color);
	a = (degrees * 128) / 360;
	x = cx + (cosine(a) * length) / 1024;
	y = cy + (-sine(a) * length) / 1024;
	FbLine(cx, cy, x, y);
}

static void st_set_course(void)
{
	int i, x, y, finished = 0;
	const int cx = (LCD_XSIZE >> 1);
	const int cy = (LCD_YSIZE >> 1) + 15;

	/* Erase old stuff */
	FbMove(2, 20);
	write_heading("NEW HEADING: ", gs.player.new_heading, BLACK);
	FbMove(2, 11);
	write_heading("CUR HEADING: ", gs.player.heading, BLACK);
	draw_heading_indicator(cx, cy, gs.player.heading, 150, BLACK);
	draw_heading_indicator(cx, cy, gs.player.new_heading, 150, BLACK);

    	if (BUTTON_PRESSED_AND_CONSUME) {
		gs.player.heading = gs.player.new_heading;
		st_program_state = ST_PROCESS_INPUT;
		finished = 1;
	} else if (UP_BTN_AND_CONSUME) {
		gs.player.new_heading += 10;
	} else if (DOWN_BTN_AND_CONSUME) {
		gs.player.new_heading -= 10;
	} else if (LEFT_BTN_AND_CONSUME) {
		gs.player.new_heading++;
	} else if (RIGHT_BTN_AND_CONSUME) {
		gs.player.new_heading--;
	}
	if (gs.player.new_heading < 0)
		gs.player.new_heading += 360;
	if (gs.player.new_heading >= 360)
		gs.player.new_heading -= 360;

	if (!finished) {
		FbColor(WHITE);
		FbMove(2, 2);
		FbWriteLine("SET HEADING");
		FbMove(2, 20);
		write_heading("NEW HEADING: ", gs.player.new_heading, WHITE);
	}
	FbMove(2, 11);
	write_heading("CUR HEADING: ", gs.player.heading, WHITE);

	draw_heading_indicator(cx, cy, gs.player.heading, 150, RED);
	draw_heading_indicator(cx, cy, gs.player.new_heading, 150, WHITE);
	/* Draw a circle of dots. */
	for (i = 0; i < 128; i += 4) {
		x = (-cosine(i) * 160) / 1024;
		y = (sine(i) * 160) / 1024;
		FbPoint(cx + x, cy + y);
	}
	FbSwapBuffers();
}

static void st_process_input(void)
{
    if (BUTTON_PRESSED_AND_CONSUME) {
        button_pressed();
    } else if (UP_BTN_AND_CONSUME) {
        if (menu.menu_active)
            dynmenu_change_current_selection(&menu, -1);
    } else if (DOWN_BTN_AND_CONSUME) {
        if (menu.menu_active)
            dynmenu_change_current_selection(&menu, 1);
    } else if (LEFT_BTN_AND_CONSUME) {
	do_something();
    } else if (RIGHT_BTN_AND_CONSUME) {
	do_something();
    } else {
        return;
    }

    if (st_program_state == ST_PROCESS_INPUT) {
        if (menu.menu_active)
            st_program_state = ST_DRAW_MENU;
    }
}

static void st_not_impl()
{
	menu.menu_active = 0;
	FbClear();
	FbColor(GREEN);
	FbMove(2, 40);
	FbWriteLine("SORRY, THAT");
	FbMove(2, 50);
	FbWriteLine("FUNCTION IS NOT");
	FbMove(2, 60);
	FbWriteLine("IMPLEMENTED");
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void strcatnum(char *s, int n)
{
	char num[10];
	itoa(num, n, 10);
	strcat(s, num);
}

static void strcat_sector_quadrant(char *msg, int x, int y)
{
	int sx, sy, qx, qy;

	sx = x >> 3;
	sy = y >> 3;
	qx = x & 0x07;
	qy = y & 0x07;
	strcat(msg, "S(");
	strcatnum(msg, sx);
	strcat(msg, ",");
	strcatnum(msg, sy);
	strcat(msg, ")Q(");
	strcatnum(msg, qx);
	strcat(msg, ",");
	strcatnum(msg, qy);
	strcat(msg, ")");
}

static void print_planets_report(int i, int n, int x, int y, int scanned)
{
	char msg[40];
	char cl[] = "CLASS  ";
	int c;

	strcpy(msg, "P");
	strcatnum(msg, n);
	strcat(msg, " ");
	strcat_sector_quadrant(msg, x, y);
	FbWriteLine(msg);
	FbMoveX(2);
	FbMoveRelative(0, 10);
	strcpy(msg, "");
	if (!scanned) {
		strcat(msg, " - NOT SCANNED");
	} else {
		if (PLANET_INHABITED & gs.object[i].tsd.planet.flags)
			strcat(msg, " I");
		if (PLANET_HAS_DILITHIUM & gs.object[i].tsd.planet.flags)
			strcat(msg, " DC ");
		c = PLANET_CLASS(gs.object[i].tsd.planet.flags);
		cl[6] = planet_class[c];
		strcat(msg, cl);
	}
	FbWriteLine(msg);
}

static void st_planets(void)
{
	int i, count, scanned;
	char cs[5];
	char msg[40];

	screen_header("PLANETS:");
	FbColor(GREEN);
	count = 0;

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case PLANET:
			if (gs.object[i].tsd.planet.flags & PLANET_KNOWN) {
				count++;
				scanned = gs.object[i].tsd.planet.flags & PLANET_SCANNED;
				FbMove(2, 20 * (count - 1) + 10);
				print_planets_report(i, count, gs.object[i].x, gs.object[i].y, scanned);
			}
			break;
		default:
			break;
		}
	}
	FbColor(WHITE);
	FbMove(2, (count) * 20 + 10);
	strcpy(msg, "TOTAL COUNT: ");
	itoa(cs, count, 10);
	strcat(msg, cs);
	FbWriteLine(msg);
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_sensors(void)
{
	int i, sx, sy, px, py;
	int candidate = -1;
	int mindist = 10000000;
	int dist;

	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case PLANET:
			sx = gs.object[i].x >> 3;
			sy = gs.object[i].y >> 3;
			px = gs.player.x >> 3;
			py = gs.player.y >> 3;
			if (sx != px || sy != py)
				break;
			sx = gs.player.x & 0x07;
			sy = gs.player.y & 0x07;
			px = gs.object[i].x & 0x07;
			py = gs.object[i].y & 0x07;
			sx = px - sx;
			sy = py - sy;
			dist = sx * sx + sy * sy;
			if (candidate == -1 || dist < mindist) {
				mindist = dist;
				candidate = i;
			}
			break;
		default:
			break;
		}
	}
	FbClear();
	FbMove(2, 2);
	FbColor(WHITE);
	if (candidate == -1) {
		FbWriteLine("NO NEARBY PLANETS");
		FbMoveX(2);
		FbMoveRelative(0, 10);
		FbWriteLine("TO SCAN");
	} else {
		char c, cl[] = "CLASS  ";
		char msg[40];
		char *flags = &gs.object[candidate].tsd.planet.flags;
		*flags |= PLANET_SCANNED;
		*flags |= PLANET_KNOWN;
		FbWriteLine("NEAREST PLANET:"); FbMoveX(2); FbMoveRelative(0, 10);
		strcpy(msg, "");
		strcat_sector_quadrant(msg, gs.object[candidate].x, gs.object[candidate].y);
		FbWriteLine(msg);
		FbMoveX(2); FbMoveRelative(0, 10);
		c = PLANET_CLASS(*flags);
		cl[6] = planet_class[(int) c];
		FbWriteLine(cl); FbMoveX(2); FbMoveRelative(0, 10);
		FbWriteLine(planet_class_name[(int) c]); FbMoveX(2); FbMoveRelative(0, 10);
		if (*flags & PLANET_INHABITED) {
			FbWriteLine("INHABITED"); FbMoveX(2); FbMoveRelative(0, 10);
		} else {
			FbWriteLine("UNINHABITED"); FbMoveX(2); FbMoveRelative(0, 10);
		}
		if (*flags & PLANET_HAS_DILITHIUM) {
			FbWriteLine("DILITHIUM FOUND"); FbMoveX(2); FbMoveRelative(0, 10);
		} else {
			FbWriteLine("NO DILITHIUM"); FbMoveX(2); FbMoveRelative(0, 10);
		}
		
	}
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_damage_report(void)
{
	int i, d;
	char ds[10];

	screen_header("DAMAGE REPORT");

	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) {
		FbColor(CYAN);
		FbMove(2, 18 + i * 9);
		FbWriteLine((char *) ship_system[i]);
		d = ((((256 - gs.player.damage[i]) * 1024) / 256) * 100) / 1024;
		if (d < 70)
			FbColor(YELLOW);
		else if (d < 40)
			FbColor(RED);
		else
			FbColor(GREEN);
		itoa(ds, d, 10);
		FbMove(LCD_XSIZE - 9 * 4, 18 + i * 9);
		FbWriteLine(ds);
	}
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void st_status_report(void)
{
	int es, sb, i, sd, frac; 
	char num[10];
	char msg[20];

	screen_header("STATUS REPORT");
	FbColor(CYAN);

	/* Compute and print current star date */
	FbMove(2, 2 * 9);
	FbWriteLine("STARDATE");
	sd = (gs.stardate / 256) + START_DATE;
	frac = ((gs.stardate % 256) * 100) / 256;
	itoa(num, sd, 10);
	msg[0] = '\0';
	strcat(msg, num);
	itoa(num, frac, 10);
	strcat(msg, ".");
	strcat(msg, num);
	FbMove(80, 2 * 9);
	FbWriteLine(msg);

	/* Compute and print time remaining */
	FbMove(2, 3 * 9);
	FbWriteLine("DAYS LEFT");
	sd = (gs.enddate - gs.stardate) / 256;
	msg[0] = '\0';
	itoa(num, sd, 10);
	strcat(msg, num); 
	frac = (((gs.enddate - gs.stardate) % 256) * 100) / 256;
	itoa(num, frac, 10);
	strcat(msg, ".");
	strcat(msg, num);
	FbMove(80, 3 * 9);
	FbWriteLine(msg);

	/* Count remaining starbases and enemy ships */
	es = 0;
	sb = 0;
	for (i = 0; i < NTOTAL; i++) {
		switch (gs.object[i].type) {
		case ENEMY_SHIP:
			es++;
			break;
		case STARBASE:
			sb++;
			break;
		default:
			break;
		}
	}
	FbMove(2, 4 * 9);
	FbWriteLine("STARBASES");
	itoa(num, sb, 10);
	FbMove(80, 4 * 9);
	FbWriteLine(num);
	FbMove(2, 5 * 9);
	FbWriteLine("ENEMY SHP");
	itoa(num, es, 10);
	FbMove(80, 5 * 9);
	FbWriteLine(num);

	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

int spacetripper_cb(void)
{
	switch (st_program_state) {
	case ST_GAME_INIT:
		st_game_init();
		break;
	case ST_NEW_GAME:
		st_new_game();
		break;
	case ST_CAPTAIN_MENU:
		st_captain_menu();
		break;
	case ST_EXIT:
		st_program_state = ST_GAME_INIT;
		returnToMenus();
		break;
	case ST_GAME_LOST:
		st_game_lost();
		break;
	case ST_GAME_WON:
		st_game_won();
		break;
	case ST_DRAW_MENU:
		st_draw_menu();
		break;
	case ST_RENDER_SCREEN:
		st_render_screen();
		break;
	case ST_PROCESS_INPUT:
		st_process_input();
		break;
	case ST_LRS:
		st_lrs();
		break;
	case ST_SRS:
		st_srs();
		break;
	case ST_SET_COURSE:
		st_set_course();
		break;
	case ST_NOT_IMPL:
		st_not_impl();
		break;
	case ST_PLANETS:
		st_planets();
		break;
	case ST_SENSORS:
		st_sensors();
		break;
	case ST_DAMAGE_REPORT:
		st_damage_report();
		break;
	case ST_STATUS_REPORT:
		st_status_report();
		gs.stardate++;
		break;
	default:
		st_program_state = ST_CAPTAIN_MENU;
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, spacetripper_cb, 240);
        return 0;
}
#endif
