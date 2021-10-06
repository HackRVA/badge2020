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

#define NKLINGONS 15
#define NCOMMANDERS 5
#define NROMULANS 10
#define NPLANETS 20
#define NBLACKHOLES 10
#define NSTARBASES 10
#define NSTARS 20
#define NTOTAL (NKLINGONS + NCOMMANDERS + NROMULANS + NPLANETS + NBLACKHOLES + NSTARBASES + NSTARS)

#define INITIAL_TORPEDOES 10
#define INITIAL_ENERGY 10000
#define INITIAL_DILITHIUM 100

#define ENEMY_SHIP 'E'
#define PLANET 'P'
#define BLACKHOLE 'B'
#define STARBASE 'S'
#define STAR '*'

static const char *object_type_name(char object_type)
{
	switch (object_type) {
	case ENEMY_SHIP:
		return "AN ENEMY\nSHIP";
		break;
	case PLANET:
		return "A PLANET";
		break;
	case BLACKHOLE:
		return "A BLACK HOLE";
		break;
	case STAR:
		return "A STAR";
		break;
	case STARBASE:
		return "A STARBASE";
		break;
	default:
		return "AN UNKNOWN\nOBJECT";
		break;
	}
}

/* Program states.  Initial state is MAZE_GAME_INIT */
enum st_program_state_t {
	ST_GAME_INIT = 0,
	ST_NEW_GAME,
	ST_CAPTAIN_MENU,
	ST_PROCESS_INPUT,
	ST_LRS,
	ST_SRS,
	ST_SET_COURSE,
	ST_AIM_WEAPONS,
	ST_CHOOSE_WEAPONS,
	ST_PHOTON_TORPEDOES,
	ST_PHASER_BEAMS,
	ST_PHASER_POWER,
	ST_PHASER_POWER_INPUT,
	ST_FIRE_PHASER,
	ST_FIRE_TORPEDO,
	ST_WARP,
	ST_WARP_INPUT,
	ST_SET_WEAPONS_BEARING,
	ST_GAME_LOST,
	ST_GAME_WON,
	ST_DRAW_MENU,
	ST_RENDER_SCREEN,
	ST_EXIT,
	ST_PLANETS,
	ST_SENSORS,
	ST_DAMAGE_REPORT,
	ST_STATUS_REPORT,
	ST_ALERT,
	ST_DOCK,
	ST_STANDARD_ORBIT,
	ST_TRANSPORTER,
	ST_MINE_DILITHIUM,
	ST_LOAD_DILITHIUM,
	ST_SELF_DESTRUCT_CONFIRM,
	ST_SELF_DESTRUCT,
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
	int x, y; /* 32 bits, 16 for sector, 16 for quadrant, high 3 bits of sector and hi 3 bits of quadrant */
	union type_specific_data tsd;
	char type;
};

static const char *ship_system[] = { "WARP", "IMPULSE", "SHIELDS", "LIFE SUPP", "PHASERS"};
#define NSHIP_SYSTEMS ARRAYSIZE(ship_system)

struct player_ship {
	int x, y; /* 32 bits, 16 for sector, 16 for quadrant */
	int heading, new_heading; /* in degrees */
	int weapons_aim, new_weapons_aim; /* in degrees */
	int energy;
	unsigned char torpedoes;
	int warp_factor, new_warp_factor; /* 0 to (1 << 16) */
#define TORPEDO_POWER (1 << 17)
	int phaser_power, new_phaser_power; /* 0 to (1 << 16) */
#define WARP10 (1 << 16)
#define WARP1 (WARP10 / 10)
	unsigned char shields;
	unsigned char shields_up;
	unsigned char dilithium_crystals;
	unsigned char damage[NSHIP_SYSTEMS];
	unsigned char docked;
	unsigned char standard_orbit;
	unsigned char away_team;
	unsigned char away_teams_crystals;
	unsigned char mined_dilithium;
};

static inline int warp_factor(int wf)
{
	return (wf / WARP1);
}

static inline int warp_factor_frac(int wf)
{
	return 10 * (wf % WARP1) / WARP1;
}

static inline int impulse_factor(int wf)
{
	return 10 * wf / WARP1;
}

struct game_state {
	struct game_object object[NTOTAL];
	struct player_ship player;
#define TICKS_PER_DAY 256  /* Each tick is about 5.6 minutes of game time */
#define START_DATE (2623)
	short stardate;
	short enddate;
	unsigned char srs_needs_update;
	unsigned char last_screen;
#define LRS_SCREEN 1
#define SRS_SCREEN 2
#define WARP_SCREEN 3
#define HEADING_SCREEN 4
#define SENSORS_SCREEN 5
#define DAMAGE_SCREEN 6
#define STATUS_SCREEN 7
#define PLANETS_SCREEN 8
#define CAPN_SCREEN 9
#define ALERT_SCREEN 10
#define AIMING_SCREEN 11
#define PHASER_POWER_SCREEN 12
#define UNKNOWN_SCREEN 255;
	int score;
#define KLINGON_POINTS 100
#define ROMULAN_POINTS 200
#define COMMANDER_POINTS 400
	unsigned char game_over;
} gs = { 0 };

static inline int coord_to_sector(int c)
{
	return (c >> 16);
}

static inline int coord_to_quadrant(int c)
{
	return (c >> 13) & 0x07;
}

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

static void alert_player(char *title, char *msg)
{
	FbClear();
	FbMove(2, 0);
	FbColor(WHITE);
	FbWriteLine(title);
	FbColor(GREEN);
	FbMove(2, 20);
	FbWriteString(msg);
	FbSwapBuffers();
	gs.last_screen = ALERT_SCREEN;
	st_program_state = ST_ALERT;
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
	st_program_state = ST_DRAW_MENU;
}

static void st_draw_menu(void)
{
	dynmenu_draw(&menu);
	gs.last_screen = UNKNOWN_SCREEN;
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
	gs.srs_needs_update = 0;
	gs.last_screen = 255;
}

static void st_captain_menu(void)
{
	clear_menu();
	strcpy(menu.title, "USS ENTERPRISE");
	strcpy(menu.title3, "CAPN'S ORDERS?");
	dynmenu_add_item(&menu, "LONG RNG SCAN", ST_LRS, 0);
	dynmenu_add_item(&menu, "SHORT RNG SCAN", ST_SRS, 0);
	dynmenu_add_item(&menu, "SET COURSE", ST_SET_COURSE, 0);
	dynmenu_add_item(&menu, "WEAPONS CTRL", ST_AIM_WEAPONS, 0);
	dynmenu_add_item(&menu, "WARP CTRL", ST_WARP, 0);
	dynmenu_add_item(&menu, "SHIELD CTRL", ST_NOT_IMPL, 0);
	dynmenu_add_item(&menu, "DAMAGE REPORT", ST_DAMAGE_REPORT, 0);
	dynmenu_add_item(&menu, "STATUS REPORT", ST_STATUS_REPORT, 0);
	dynmenu_add_item(&menu, "SENSORS", ST_SENSORS, 0);
	dynmenu_add_item(&menu, "PLANETS", ST_PLANETS, 0);
	dynmenu_add_item(&menu, "STANDARD ORBIT", ST_STANDARD_ORBIT, 0);
	dynmenu_add_item(&menu, "DOCKING CTRL", ST_DOCK, 0);
	dynmenu_add_item(&menu, "TRANSPORTER", ST_TRANSPORTER, 0);
	dynmenu_add_item(&menu, "MINE DILITHIUM", ST_MINE_DILITHIUM, 0);
	dynmenu_add_item(&menu, "LOAD DILITHIUM", ST_LOAD_DILITHIUM, 0);
	dynmenu_add_item(&menu, "SELF DESTRUCT", ST_SELF_DESTRUCT_CONFIRM, 0);
	dynmenu_add_item(&menu, "NEW GAME", ST_NEW_GAME, 0);
	dynmenu_add_item(&menu, "QUIT GAME", ST_EXIT, 0);
	menu.menu_active = 1;
	st_draw_menu();
	FbSwapBuffers();
	gs.last_screen = CAPN_SCREEN;
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
	return xorshift(&xorshift_state) & 0x0007ffff;
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
	gs.player.energy = INITIAL_ENERGY;
	gs.player.shields = 255;
	gs.player.shields_up = 0;
	gs.player.dilithium_crystals = INITIAL_DILITHIUM;
	gs.player.warp_factor = 0;
	gs.player.new_warp_factor = 0;
	gs.player.torpedoes = INITIAL_TORPEDOES;
	gs.player.phaser_power = 0;
	gs.player.new_phaser_power = 0;
	gs.player.docked = 0;
	gs.player.away_teams_crystals = 0;
	gs.player.mined_dilithium = 0;
#define ABOARD_SHIP 255
	gs.player.away_team = ABOARD_SHIP;

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
	gs.score = 0;
	gs.game_over = 0;
	st_program_state = ST_CAPTAIN_MENU;
}

static void st_render_screen(void)
{
	FbSwapBuffers();
	st_program_state = ST_PROCESS_INPUT;
}

static void button_pressed()
{
	if (gs.game_over) {
		gs.game_over = 0;
		st_program_state = ST_GAME_INIT;
		return;
	}
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

	itoa(sx, coord_to_sector(gs.player.x), 10);
	itoa(sy, coord_to_sector(gs.player.y), 10);
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
	itoa(sx, coord_to_quadrant(gs.player.x), 10);
	itoa(sy, coord_to_quadrant(gs.player.y), 10);
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

static void print_speed(char *impulse, char *warp, int x, int y, int warp_f)
{
	char num[5];
	char msg[10];

	FbMove(x, y);
	if (warp_f < WARP1) {
		FbWriteLine(impulse);
		strcpy(msg, "0.");
		itoa(num, impulse_factor(warp_f), 10);
		strcat(msg, num);
	} else {
		FbWriteLine(warp);
		itoa(num, warp_factor(warp_f), 10);
		strcpy(msg, num);
		strcat(msg, ".");
		itoa(num, warp_factor_frac(warp_f), 10);
		strcat(msg, num);
	}
	FbMove(x, y + 9);
	FbWriteLine(msg);
}

static void print_power(int x, int y, int power)
{
	char num[5];
	char msg[10];

	FbMove(x, y);
	FbWriteLine("POWER");
	itoa(num, warp_factor(power), 10);
	strcpy(msg, num);
	strcat(msg, ".");
	itoa(num, warp_factor_frac(power), 10);
	strcat(msg, num);
	FbMove(x, y + 9);
	FbWriteLine(msg);
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

	sectorx = coord_to_sector(gs.player.x);
	sectory = coord_to_sector(gs.player.y);
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
		sx = coord_to_sector(gs.object[i].x);
		sy = coord_to_sector(gs.object[i].y);
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
	print_speed("IMP", "WRP", 95, 28, gs.player.warp_factor);

	FbColor(CYAN);
	for (x = -1; x < 2; x++) { /* Print out X coordinates */
		single_digit_to_dec(x + coord_to_sector(gs.player.x), num);
		FbMove(35 + (x + 1) * 30, 50);
		FbWriteLine(num);
	}

	for (y = -1; y < 2; y++) { /* Print out Y coordinates */
		single_digit_to_dec(y + coord_to_sector(gs.player.y), num);
		FbMove(5, (y + 1) * 10 + 60);
		FbWriteLine(num);
	}

	/* Print out scan[][][] */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			for (i = 0; i < 3; i++) {
				char digit[5];

				FbColor(color[i]);
				itoa(digit, scan[x][y][i], 10);
				FbMove(25 + x * 30 + i * 8, y * 10 + 60);
				FbWriteLine(digit);
			}
		}
		FbColor(GREEN);
		FbHorizontalLine(22, y * 10 + 59, 132 - 21, y * 10 + 59);
	}
	FbHorizontalLine(22, y * 10 + 59, 132 - 21, y * 10 + 59);
	for (i = 0; i < 4; i++)
		FbVerticalLine(22 + i * 30, 59, 22 + i * 30,  y * 10 + 59);

	FbMove(2, 98);
	FbColor(color[0]);
	FbWriteLine("# ENEMY SHIPS");
	FbMove(2, 107);
	FbColor(color[1]);
	FbWriteLine("# STARBASES");
	FbMove(2, 116);
	FbColor(color[2]);
	FbWriteLine("# STARS");
	
	FbSwapBuffers();
	gs.last_screen = LRS_SCREEN;
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

	sectorx = coord_to_sector(gs.player.x);
	sectory = coord_to_sector(gs.player.y);
	memset(scan, 0, sizeof(scan));

	screen_header("SHORT RANGE SCAN");
	print_sector_quadrant_heading();
	print_speed("IMP", "WRP", 95, 48, gs.player.warp_factor);

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

		sx = coord_to_sector(gs.object[i].x);
		sy = coord_to_sector(gs.object[i].y);

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
		qx = coord_to_quadrant(gs.object[i].x);
		qy = coord_to_quadrant(gs.object[i].y);
		FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
		FbColor(color);
		FbWriteLine(c);
	}

	/* Draw player */
	qx = coord_to_quadrant(gs.player.x);
	qy = coord_to_quadrant(gs.player.y);
	FbMove(left + qx * quadrant_width, 47 + qy * quadrant_width);
	FbColor(WHITE);
	FbWriteLine("E");

	FbSwapBuffers();
	gs.last_screen = SRS_SCREEN;
	gs.srs_needs_update = 0;
	st_program_state = ST_PROCESS_INPUT;
}

static void print_numeric_item_with_frac(char *item, int value, int frac)
{
	char num[10];

	FbWriteString(item);
	itoa(num, value, 10);
	FbWriteString(num);
	FbWriteString(".");
	itoa(num, frac, 10);
	FbWriteString(num);
	FbMoveX(2);
	FbMoveRelative(0, 9);
}

static void print_numeric_item(char *item, int value)
{
	char num[10];

	FbWriteString(item);
	itoa(num, value, 10);
	FbWriteString(num);
	FbMoveX(2);
	FbMoveRelative(0, 9);
}

static void write_heading(char *msg, int heading, int color)
{
	FbColor(color);
	print_numeric_item(msg, heading);
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

static void show_energy_and_torps(void)
{
	FbMove(2, 20);
	print_numeric_item("ENERGY:", gs.player.energy);
	print_numeric_item("TORP:", gs.player.torpedoes);
}

static void st_choose_angle(char *new_head, char *cur_head, char *set_head,
		int *heading, int *new_heading, int which_screen, void (*show_extra_data)(void))
{
	int i, x, y, finished = 0;
	const int cx = (LCD_XSIZE >> 1);
	const int cy = (LCD_YSIZE >> 1) + 15;

	/* Erase old stuff */
	FbMove(2, 20);
	write_heading(new_head, *new_heading, BLACK);
	FbMove(2, 11);
	write_heading(cur_head, *heading, BLACK);
	if (show_extra_data)
		show_extra_data();
	draw_heading_indicator(cx, cy, *heading, 150, BLACK);
	draw_heading_indicator(cx, cy, *new_heading, 150, BLACK);

	if (BUTTON_PRESSED_AND_CONSUME) {
		*heading = *new_heading;
		st_program_state = ST_PROCESS_INPUT;
		finished = 1;
	} else if (UP_BTN_AND_CONSUME) {
		*new_heading += 10;
	} else if (DOWN_BTN_AND_CONSUME) {
		*new_heading -= 10;
	} else if (LEFT_BTN_AND_CONSUME) {
		(*new_heading)++;
	} else if (RIGHT_BTN_AND_CONSUME) {
		(*new_heading)--;
	}
	if (*new_heading < 0)
		*new_heading += 360;
	if (*new_heading >= 360)
		*new_heading -= 360;

	if (!finished) {
		FbColor(WHITE);
		FbMove(2, 2);
		FbWriteLine(set_head);
		FbMove(2, 20);
		write_heading(new_head, *new_heading, WHITE);
	}
	FbMove(2, 11);
	write_heading(cur_head, *heading, WHITE);

	draw_heading_indicator(cx, cy, *heading, 150, RED);
	draw_heading_indicator(cx, cy, *new_heading, 150, WHITE);
	/* Draw a circle of dots. */
	for (i = 0; i < 128; i += 4) {
		x = (-cosine(i) * 160) / 1024;
		y = (sine(i) * 160) / 1024;
		FbPoint(cx + x, cy + y);
	}
	if (show_extra_data)
		show_extra_data();
	FbSwapBuffers();
	gs.last_screen = which_screen;
	if (finished && which_screen == AIMING_SCREEN) {
		st_program_state = ST_CHOOSE_WEAPONS;
	}
}

static void st_choose_weapons(void)
{
	clear_menu();	
	strcpy(menu.title, "CHOOSE WEAPON");
	dynmenu_add_item(&menu, "PHOTON TORPS", ST_PHOTON_TORPEDOES, 0);
	dynmenu_add_item(&menu, "PHASER BEAMS", ST_PHASER_BEAMS, 0);
	dynmenu_add_item(&menu, "CANCEL WEAPONS", ST_CAPTAIN_MENU, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
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
	gs.last_screen = UNKNOWN_SCREEN;
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

	sx = coord_to_sector(x);
	sy = coord_to_sector(y);
	qx = coord_to_quadrant(x);
	qy = coord_to_quadrant(y);
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
	FbMove(2, (count) * 20 + 1);
	print_numeric_item("TOTAL COUNT: ", count);
	FbSwapBuffers();
	gs.last_screen = PLANETS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static int player_is_next_to(unsigned char object_type)
{
	int i, sx, sy, qx, qy;

	/* See if there is an object of the specified type nearby */
	sx = coord_to_sector(gs.player.x);
	sy = coord_to_sector(gs.player.y);
	qx = coord_to_quadrant(gs.player.x);
	qy = coord_to_quadrant(gs.player.y);
	for (i = 0; i < NTOTAL; i++) {
		struct game_object *o = &gs.object[i];
		if (o->type != object_type)
			continue;
		if (sx != coord_to_sector(o->x))
			continue;
		if (sy != coord_to_sector(o->y))
			continue;
		if (abs(qx - coord_to_quadrant(o->x) > 1))
			continue;
		if (abs(qy - coord_to_quadrant(o->y) > 1))
			continue;
		return i + 1; /* Add 1 so that we never return 0 here and result can be used as boolean */
	}
	return 0;
}

static int object_is_next_to_player(int object)
{
	if (coord_to_sector(gs.player.x) != coord_to_sector(gs.object[object].x))
		return 0;
	if (coord_to_sector(gs.player.y) != coord_to_sector(gs.object[object].y))
		return 0;
	if (abs(coord_to_quadrant(gs.player.x) - coord_to_quadrant(gs.object[object].x)) > 1)
		return 0;
	if (abs(coord_to_quadrant(gs.player.y) - coord_to_quadrant(gs.object[object].y)) > 1)
		return 0;
	return 1;
}

static void st_dock(void)
{
	/* If player docked, undock player */
	if (gs.player.docked) {
		gs.player.docked = 0;
		alert_player("DOCKING CONTROL", "CAPTAIN THE\nSHIP HAS BEEN\nUNDOCKED FROM\nSTARBASE");
		return;
	}

	/* See if there is a starbase nearby */
	if (player_is_next_to(STARBASE))
		alert_player("DOCKING CONTROL", "CAPTAIN THE\nSHIP HAS BEEN\nDOCKED WITH\nSTARBASE\n\nSUPPLIES\nREPLENISHING");
	else
		alert_player("DOCKING CONTROL", "CAPTAIN THERE\nARE NO NEARBY\nSTARBASES WITH\nWHICH TO DOCK");

	/* Ship's supplies get replenished in move_player() */
}

static void st_standard_orbit(void)
{
	if (gs.player.docked) {
		alert_player("NAVIGATION", "CAPTAIN THE\nSHIP CANNOT\nENTER STANDARD\nORBIT WHILE\nDOCKED WITH\nTHE STARBASE");
		return;
	}

	if (gs.player.standard_orbit) {
		gs.player.standard_orbit = 0;
		alert_player("NAVIGATION", "CAPTAIN WE\nHAVE LEFT\nSTANDARD ORBIT\nAND ARE IN\nOPEN SPACE");
		return;
	}

	if (gs.player.warp_factor > 0) {
		alert_player("NAVIGATION", "CAPTAIN\n\nWE MUST\nCOME OUT OF\nWARP BEFORE\nENTERING\nSTANDARD ORBIT");
		return;
	}

	if (player_is_next_to(PLANET)) {
		gs.player.standard_orbit = 1;
		gs.player.warp_factor = 0; /* belt and suspenders */
		alert_player("NAVIGATION", "CAPTAIN WE\nHAVE ENTERED\nSTANDARD ORBIT\nAROUND THE\nPLANET");
	} else {
		alert_player("NAVIGATION", "CAPTAIN THERE\nIS NO PLANET\nCLOSE ENOUGH\nTO ENTER\nSTANDARD ORBIT");
	}
}

static void st_transporter(void)
{
	int planet;

	if (!gs.player.standard_orbit) {
		alert_player("TRANSPORTER", "CAPTAIN\n\nWE ARE NOT\nCURRENTLY IN\nORBIT AROUND\nA SUITABLE\nPLANET");
		return;
	}

	if (gs.player.away_team != ABOARD_SHIP &&
		object_is_next_to_player(gs.player.away_team)) {
		gs.player.away_team = ABOARD_SHIP;
		if (gs.player.away_teams_crystals > 0) {
			int dilith_crystals;

			alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nABOARD FROM\n"
							"PLANET SURFACE\nWITH DILITHIUM\nCRYSTALS");
			dilith_crystals = gs.player.mined_dilithium + gs.player.away_teams_crystals;
			if (dilith_crystals > 255)
				dilith_crystals = 255;
			gs.player.mined_dilithium = dilith_crystals;
			gs.player.away_teams_crystals = 0;
		} else {
			alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nABOARD FROM\nPLANET SURFACE");
		}
		return;
	}

	planet = player_is_next_to(PLANET);
	if (!planet) {
		/* This is a bug. We should never be in standard orbit while not next to a planet. */
		alert_player("TRANSPORTER", "CAPTAIN\n\nTHE PLANET HAS\nMYSTERIOUSLY\nDISAPPEARED!");
		return;
	}
	/* Remember on which planet we dropped off the away team. */
	gs.player.away_team = planet - 1; /* player_is_next_to() added 1 so value can be used as boolean, so we subtract here. */
	alert_player("TRANSPORTER", "CAPTAIN\n\nAWAY TEAM\nHAS BEAMED\nDOWN TO THE\nPLANET SURFACE");
}

static void st_mine_dilithium(void)
{
	struct game_object *planet;

	if (gs.player.away_team == ABOARD_SHIP) {
		alert_player("COMMS", "CAPTAIN\n\nTHE AWAY TEAM\nIS STILL ABOARD\nTHE SHIP");
		return;
	}
	planet = &gs.object[gs.player.away_team];
	if (planet->tsd.planet.flags & PLANET_HAS_DILITHIUM) {
		alert_player("COMMS", "CAPTAIN\n\nTHE AWAY\nTEAM REPORTS\nTHEY HAVE\n"
				"FOUND\nDILITHIUM\nCRYSTALS");
		gs.player.away_teams_crystals += 5 + (xorshift(&xorshift_state) & 0x07);
		if (gs.player.away_teams_crystals > 50)
			gs.player.away_teams_crystals = 50;
		return;
	}
	alert_player("COMMS", "CAPTAIN\n\nTHE AWAY\nTEAM REPORTS\nTHEY HAVE\n"
			"FOUND NO\nDILITHIUM\nCRYSTALS");
}

static void st_load_dilithium(void)
{
	int n, leftover = 0;

	if (gs.player.mined_dilithium == 0) {
		alert_player("ENGINEERING", "CAPTAIN\n\nWE HAVEN'T GOT\nANY EXTRA\nDILITHIUM\nCRYSTALS\nTO LOAD INTO\nTHE WARP CHAMBER");
		return;
	}
	if (gs.player.dilithium_crystals == 255) {
		alert_player("ENGINEERING", "CAPTAIN\n\nTHE WARP DRIVE\nIS AT FULL\nCAPACITY ALREADY");
		return;
	}
	n = gs.player.dilithium_crystals + gs.player.mined_dilithium;
	if (n > 255) {
		n = 255;
		leftover = gs.player.dilithium_crystals + gs.player.mined_dilithium - 255;
	}
	gs.player.dilithium_crystals = n;
	gs.player.mined_dilithium = leftover;
	alert_player("ENGINEERING", "CAPTAIN\n\nWE HAVE LOADED\nUP THE MINED\n"
				"DILITHIUM\nCRYSTALS\nINTO THE WARP\nCHAMBER\n"
				"HOPEFULLY THE\nQUALITY IS NOT\nTOO POOR");
}

static void st_self_destruct_confirm(void)
{
	clear_menu();
	strcpy(menu.title, "SELF DESTRUCT?");
	dynmenu_add_item(&menu, "NO DO NOT DESTRUCT", ST_CAPTAIN_MENU, 0);
	dynmenu_add_item(&menu, "YES SELF DESTRUCT", ST_SELF_DESTRUCT, 0);
	menu.menu_active = 1;
	st_program_state = ST_DRAW_MENU;
}

static void adjust_score_on_kill(int target)
{
	switch (gs.object[target].tsd.ship.shiptype) {
	case 'K':
		gs.score += KLINGON_POINTS;
		break;
	case 'C':
		gs.score += COMMANDER_POINTS;
		break;
	case 'R':
		gs.score += ROMULAN_POINTS;
		break;
	default:
		break;
	}
}

static void st_self_destruct(void)
{
	int i;
	char msg[60];
	char num[10];

	/* Kill any enemy ships in the sector. */
	for (i = 0; i < NTOTAL; i++) {
		struct game_object *o = &gs.object[i];
		if (o->type != ENEMY_SHIP)
			continue;
		if (coord_to_sector(o->x) != coord_to_sector(gs.player.x))
			continue;
		if (coord_to_sector(o->y) != coord_to_sector(gs.player.y))
			continue;
		adjust_score_on_kill(i);
		delete_object(i);
	}
	strcpy(msg, "YOUR FINAL\nSCORE WAS: ");
	itoa(num, gs.score, 10);
	strcat(msg, num);
	alert_player("GAME OVER", msg);
	gs.game_over = 1;
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
			sx = coord_to_sector(gs.object[i].x);
			sy = coord_to_sector(gs.object[i].y);
			px = coord_to_sector(gs.player.x);
			py = coord_to_sector(gs.player.y);
			if (sx != px || sy != py)
				break;
			sx = coord_to_quadrant(gs.player.x);
			sy = coord_to_quadrant(gs.player.y);
			px = coord_to_quadrant(gs.object[i].x);
			py = coord_to_quadrant(gs.object[i].y);
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
	gs.last_screen = SENSORS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void show_torps_energy_and_dilith(void)
{
	FbMoveRelative(0, 9);
	FbMoveX(2);
	print_numeric_item("ENERGY:", gs.player.energy);
	print_numeric_item("TORPEDOES:", gs.player.torpedoes);
	print_numeric_item("DILITH:", gs.player.dilithium_crystals);
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
	show_torps_energy_and_dilith();
	FbColor(WHITE);
	if (gs.player.docked)
		FbWriteString("CURRENTLY\nDOCKED AT\nSTARBASE");
	if (gs.player.standard_orbit)
		FbWriteString("CURRENTLY\nIN STANDARD\nORBIT\n");
	if (gs.player.away_team != ABOARD_SHIP)
		FbWriteString("AWAY TEAM OUT");
	FbSwapBuffers();
	gs.last_screen = DAMAGE_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

static void st_status_report(void)
{
	int es, sb, i, sd, frac;

	screen_header("STATUS REPORT");
	FbColor(CYAN);

	/* Compute and print current star date */
	FbMove(2, 2 * 9);
	sd = (gs.stardate / 256) + START_DATE;
	frac = ((gs.stardate % 256) * 100) / 256;
	print_numeric_item_with_frac("STARDATE:", sd, frac);

	/* Compute and print time remaining */
	sd = (gs.enddate - gs.stardate) / 256;
	frac = (((gs.enddate - gs.stardate) % 256) * 100) / 256;
	print_numeric_item_with_frac("DAYS LEFT:", sd, frac);

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
	print_numeric_item("STARBASES", sb);
	print_numeric_item("ENEMY SHP", es);
	show_torps_energy_and_dilith();
	FbColor(YELLOW);
	print_numeric_item("SCORE:", gs.score);
	FbColor(WHITE);
	if (gs.player.docked)
		FbWriteString("CURRENTLY\nDOCKED AT\nSTARBASE");
	if (gs.player.standard_orbit)
		FbWriteString("CURRENTLY\nIN STANDARD\nORBIT\n");
	if (gs.player.away_team != ABOARD_SHIP)
		FbWriteString("AWAY TEAM OUT");

	FbSwapBuffers();
	gs.last_screen = STATUS_SCREEN;
	st_program_state = ST_PROCESS_INPUT;
}

/* returns true if collision */
static int player_collision_detection(int *nx, int *ny)
{
	int i, sx, sy, qx, qy, osx, osy, oqx, oqy;
	unsigned char neutral_zone;
	char msg[40];

	/* Keep the player in bounds */
	neutral_zone = 0;
	if (*nx < 0) {
		neutral_zone = 1;
		*nx = 0;
	}
	if (*nx > 0x0007ffff) {
		neutral_zone = 1;
		*nx = 0x0007ffff;
	}
	if (*ny < 0) {
		neutral_zone = 1;
		*ny = 0;
	}
	if (*ny > 0x0007ffff) {
		neutral_zone = 1;
		*ny = 0x0007ffff;
	}
	if (neutral_zone) {
		gs.player.warp_factor = 0;
		gs.srs_needs_update = 1;
		alert_player("STARFLEET MSG",
			"PERMISSION TO\nENTER NEUTRAL\nZONE DENIED\n\n"
			"SHUT DOWN\nWARP DRIVE\n\n-- STARFLEET");
		return 0;
	}

	sx = coord_to_sector(*nx);
	sy = coord_to_sector(*ny);
	qx = coord_to_quadrant(*nx);
	qy = coord_to_quadrant(*ny);

	for (i = 0; i < NTOTAL; i++) {
		if (gs.object[i].type == 0)
			continue;
		osx = coord_to_sector(gs.object[i].x);
		if (osx != sx)
			continue;
		osy = coord_to_sector(gs.object[i].y);
		if (osy != sy)
			continue;
		oqx = coord_to_quadrant(gs.object[i].x);
		if (oqx != qx)
			continue;
		oqy = coord_to_quadrant(gs.object[i].y);
		if (oqy != qy)
			continue;
		const char *object = object_type_name(gs.object[i].type);
		strcpy(msg, "WE HAVE\nENCOUNTERED\n");
		strcat(msg, object);
		alert_player("WARP SHUTDOWN", msg);
		gs.player.warp_factor = 0;
		return 1;
	}
	return 0;
}

static void replenish_int_supply(int *supply, int limit, int increment)
{
	if (*supply < limit) {
		*supply += increment;
		if (*supply > limit)
			*supply = limit;
	}
}

static void replenish_char_supply(unsigned char *supply, unsigned int limit, unsigned char increment)
{
	int v;

	if (*supply < limit) {
		v = *supply + increment;
		if (v > (int) limit)
			v = limit;
		*supply = (unsigned char) v;;
	}
}

static void replenish_supplies_and_repair_ship(void)
{
	int i, n;

	replenish_char_supply(&gs.player.torpedoes, INITIAL_TORPEDOES, 1);
	replenish_int_supply(&gs.player.energy, INITIAL_ENERGY, INITIAL_ENERGY / 10);
	replenish_char_supply(&gs.player.dilithium_crystals, INITIAL_DILITHIUM, INITIAL_DILITHIUM / 10);
	for (i = 0; (size_t) i < NSHIP_SYSTEMS; i++) { /* Repair damaged systems */
		if (gs.player.damage[i] > 0) {
			n = gs.player.damage[i] - 25;
			if (n < 0)
				n = 0; 
			gs.player.damage[i] = n;
		}
	}
}

static void move_player(void)
{
	int dx, dy, b, nx, ny;

	if (gs.player.docked) {
		gs.player.warp_factor = 0;
		replenish_supplies_and_repair_ship();
		return;
	}
	/* Move player */
	if (gs.player.warp_factor > 0) {
		b = (gs.player.heading * 128) / 360;
		if (b < 0)
			b += 128;
		if (b > 128)
			b -= 128;
		dx = (gs.player.warp_factor * cosine(b)) / 1024;
		dy = (-gs.player.warp_factor * sine(b)) / 1024;
		nx = gs.player.x + dx;
		ny = gs.player.y + dy;

		if (coord_to_sector(gs.player.x) != coord_to_sector(nx) ||
			coord_to_sector(gs.player.y) != coord_to_sector(ny) ||
			coord_to_quadrant(gs.player.x) != coord_to_quadrant(nx) ||
			coord_to_quadrant(gs.player.y) != coord_to_quadrant(ny)) {
			gs.srs_needs_update = 1;
		}
		if (!player_collision_detection(&nx, &ny)) {
			gs.player.x = nx;
			gs.player.y = ny;
		}
	}
}

static void move_objects(void)
{
	move_player();
}

static void draw_speed_gauge_ticks(void)
{
	int i;
	char num[3];

	FbColor(WHITE);
	FbVerticalLine(127, 5, 127, 105);
	num[2] = '\0';
	for (i = 0; i <= 10; i++) {
		FbHorizontalLine(120, 5 + i * 10, 126, 5 + i * 10);
		if ((i % 5) == 0) {
			FbMove(90, 5 + i * 10);
			if (i == 0)
				num[0] = '1';
			else
				num[0] = ' ';
			num[1] = '0' + ((10 - i) % 10);
			FbWriteLine(num);
		}
	}
}

static void draw_speed_gauge_marker(int color, int speed)
{
	int y1, y2, y3;

	y2 = 105 - ((100 * speed) >> 16);
	y1 = y2 - 5;
	y3 = y2 + 5;
	FbColor(color);
	FbVerticalLine(115, y1, 115, y3);
	FbLine(115, y1, 122, y2);
	FbLine(115, y3, 122, y2);
}

#define SPEED_GAUGE 0
#define PHASER_POWER_GAUGE 1 
static void draw_speed_gauge(int gauge_type, int color, int color2, int speed, int new_speed)
{
	draw_speed_gauge_ticks();
	draw_speed_gauge_marker(color, speed);
	draw_speed_gauge_marker(color2, new_speed);
	FbColor(GREEN);
	switch (gauge_type) {
	case SPEED_GAUGE:
		print_speed("CURR IMPULSE", "CURR WARP", 2, 30, speed);
		print_speed("NEW IMPULSE", "NEW WARP", 2, 60, new_speed);
		break;
	case PHASER_POWER_GAUGE:
		print_power(2, 30, speed);
		break;
	default:
		break;
	}
}

static void st_warp()
{
	if (gs.player.docked) {
		alert_player("WARP CONTROL", "CAPTAIN\n\n"
			"WE CANNOT\nENGAGE\nPROPULSION\nWHILE DOCKED\n"
			"WITH THE\nSTARBASE");
		return;
	}

	if (gs.player.standard_orbit) {
		alert_player("NAVIGATION", "CAPTAIN\nWE MUST LEAVE\nSTANDARD ORBIT\nBEFORE\nENGAGING THE\nWARP DRIVE");
		return;
	}

	screen_header("WARP");
	draw_speed_gauge(SPEED_GAUGE, GREEN, RED, gs.player.warp_factor, gs.player.new_warp_factor);

	FbSwapBuffers();
	gs.last_screen = WARP_SCREEN;
	st_program_state = ST_WARP_INPUT;
}

static void st_phaser_power()
{
	screen_header("PHASER PWR");
	draw_speed_gauge(PHASER_POWER_GAUGE, GREEN, RED, gs.player.phaser_power, gs.player.new_phaser_power);

	FbMove(2, 110);
	print_numeric_item("AVAILABLE\nENERGY: ", gs.player.energy);

	FbSwapBuffers();
	gs.last_screen = PHASER_POWER_SCREEN;
	st_program_state = ST_PHASER_POWER_INPUT;
}

static void st_warp_input(void)
{
	int old = gs.player.new_warp_factor;

	if (BUTTON_PRESSED_AND_CONSUME) {
		gs.player.warp_factor = gs.player.new_warp_factor;
		st_warp();
		st_program_state = ST_PROCESS_INPUT;
		return;
	} else if (UP_BTN_AND_CONSUME) {
		gs.player.new_warp_factor += WARP1;
	} else if (DOWN_BTN_AND_CONSUME) {
		gs.player.new_warp_factor -= WARP1;
	} else if (LEFT_BTN_AND_CONSUME) {
		gs.player.new_warp_factor -= WARP1 / 10;
	} else if (RIGHT_BTN_AND_CONSUME) {
		gs.player.new_warp_factor += WARP1 / 10;
	}
	if (gs.player.new_warp_factor < 0)
		gs.player.new_warp_factor = 0;
	if (gs.player.new_warp_factor > WARP10)
		gs.player.new_warp_factor = WARP10;

	if (old != gs.player.new_warp_factor)
		st_program_state = ST_WARP;
}

static void st_phaser_power_input(void)
{
	int old = gs.player.new_phaser_power;

	if (BUTTON_PRESSED_AND_CONSUME) {
		gs.player.phaser_power = gs.player.new_phaser_power;
		st_phaser_power();
		st_program_state = ST_FIRE_PHASER;
		return;
	} else if (UP_BTN_AND_CONSUME) {
		gs.player.new_phaser_power += WARP1;
	} else if (DOWN_BTN_AND_CONSUME) {
		gs.player.new_phaser_power -= WARP1;
	} else if (LEFT_BTN_AND_CONSUME) {
		gs.player.new_phaser_power -= WARP1 / 10;
	} else if (RIGHT_BTN_AND_CONSUME) {
		gs.player.new_phaser_power += WARP1 / 10;
	}
	if (gs.player.new_phaser_power < 0)
		gs.player.new_phaser_power = 0;
	if (gs.player.new_phaser_power > WARP10)
		gs.player.new_phaser_power = WARP10;

	if (old != gs.player.new_phaser_power)
		st_program_state = ST_PHASER_POWER;
}

static int check_weapon_collision(char *weapon_name, int i, int x, int y)
{
	int tx, ty; 
	char msg[40];

	tx = gs.object[i].x;
	ty = gs.object[i].y;

	/* Can't use pythagorean theorem because square of distance across 1 quadrant
	 * will overflow an int.
	 */
	if (coord_to_sector(tx) != coord_to_sector(x))
		return 0;
	if (coord_to_sector(ty) != coord_to_sector(y))
		return 0;
	if (coord_to_quadrant(tx) != coord_to_quadrant(x))
		return 0;
	if (coord_to_quadrant(ty) != coord_to_quadrant(y))
		return 0;
	strcpy(msg, weapon_name);
	strcat(msg, " HITS\n");
	strcat(msg, object_type_name(gs.object[i].type));
	strcat(msg, "!");
	alert_player(weapon_name, msg);
	
	return 1;
}

static void do_weapon_damage(int target, int power)
{
	int new_hp, scaled_power, damage;

	if (gs.object[target].type != ENEMY_SHIP)
		return;

	scaled_power = (power >> 11) & 0x0ff;
	damage = 127 + (xorshift(&xorshift_state) & 0x0ff) / 2;
	damage = (damage * scaled_power) / 256;
	new_hp = gs.object[target].tsd.ship.hitpoints;
	new_hp -= damage;
	if (new_hp <= 0) {
		new_hp = 0;
		adjust_score_on_kill(target);
		delete_object(target);
		alert_player("WEAPONS", "ENEMY SHIP\nDESTROYED!");
		return;
	}
	gs.object[target].tsd.ship.hitpoints = new_hp;
	return;
}

static void st_fire_weapon(char *weapon_name, int weapon_power)
{
	int i, j, x, y, a, dx, dy;

	x = gs.player.x;
	y = gs.player.y;

	FbClear();
	FbMove(2, 60);
	FbColor(WHITE);

	a = (gs.player.weapons_aim * 128) / 360;
	dx = (cosine(a) * 100) / 1024;
	dy = (-sine(a) * 100) / 1024;
	dx = dx * (WARP1 / 1000);
	dy = dy * (WARP1 / 1000);

	for (i = 0; i < 20; i++) { /* FIXME, is this too much looping? */
		for (j = 0; j < NTOTAL; j++) {
			switch (gs.object[j].type) {
			case ENEMY_SHIP:
				if (check_weapon_collision(weapon_name, j, x, y)) {
					do_weapon_damage(j, weapon_power);
					return;
				}
				break;
			default:
				continue;
			}
			x += dx;
			y += dy;
		}
	}
	FbSwapBuffers();
	alert_player(weapon_name, "MISSED!");
}

static void st_alert(void)
{
	if (BUTTON_PRESSED_AND_CONSUME)
		st_program_state = ST_CAPTAIN_MENU;
}

static void st_photon_torpedoes(void)
{
	if (gs.player.torpedoes > 0) {
		st_fire_weapon("TORPEDO", TORPEDO_POWER);
		gs.player.torpedoes--;
	} else {
		alert_player("WEAPONS", "NO PHOTON\nTORPEDOES\nREMAIN, SIR!");
	}
}

static void st_phaser_beams(void)
{
	st_program_state = ST_PHASER_POWER;
}

int spacetripper_cb(void)
{
	static int ticks = 0;

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
		st_choose_angle("NEW HEADING: ", "CUR HEADING", "SET HEADING: ",
				&gs.player.heading, &gs.player.new_heading, HEADING_SCREEN, NULL);
		break;
	case ST_AIM_WEAPONS:
		st_choose_angle("AIM WEAPONS: ", "CUR AIM", "SET AIM: ",
				&gs.player.weapons_aim, &gs.player.new_weapons_aim, AIMING_SCREEN,
				show_energy_and_torps);
		break;
	case ST_CHOOSE_WEAPONS:
		st_choose_weapons();
		break;
	case ST_WARP:
		st_warp();
		break;
	case ST_WARP_INPUT:
		st_warp_input();
		break;
	case ST_NOT_IMPL:
		st_not_impl();
		break;
	case ST_PLANETS:
		st_planets();
		break;
	case ST_DOCK:
		st_dock();
		break;
	case ST_TRANSPORTER:
		st_transporter();
		break;
	case ST_MINE_DILITHIUM:
		st_mine_dilithium();
		break;
	case ST_LOAD_DILITHIUM:
		st_load_dilithium();
		break;
	case ST_SELF_DESTRUCT_CONFIRM:
		st_self_destruct_confirm();
		break;
	case ST_SELF_DESTRUCT:
		st_self_destruct();
		break;
	case ST_STANDARD_ORBIT:
		st_standard_orbit();
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
	case ST_ALERT:
		st_alert();
		break;
	case ST_PHOTON_TORPEDOES:
		st_photon_torpedoes();
		break;
	case ST_PHASER_BEAMS:
		st_phaser_beams();
		break;
	case ST_PHASER_POWER:
		st_phaser_power();
		break;
	case ST_PHASER_POWER_INPUT:
		st_phaser_power_input();
		break;
	case ST_FIRE_PHASER:
		st_fire_weapon("PHASER", gs.player.phaser_power);
		break;
	case ST_FIRE_TORPEDO:
		st_fire_weapon("TORPEDO", TORPEDO_POWER);
		break;
	default:
		st_program_state = ST_CAPTAIN_MENU;
		break;
	}
	ticks++;
	if ((ticks % 60) == 0) { /* Every 2 seconds */
		move_objects();
		if (gs.last_screen == SRS_SCREEN && gs.srs_needs_update)
			st_program_state = ST_SRS;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
	start_gtk(&argc, &argv, spacetripper_cb, 30);
	return 0;
}
#endif
