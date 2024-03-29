/*

This a got-a-collect-em-all style game that is meant to be played
with little PIC32 badges built by HackRVA for RVASec Conference
If the code seems a little strange, it's due to the environment this
code must run in.

--
Dustin Firebaugh <dafirebaugh@gmail.com>

*/

#ifdef __linux__
#include <string.h>

#include "../linux/linuxcompat.h"
#include <stdio.h>

#define DISABLE_INTERRUPTS do { disable_interrupts(); } while (0)
#define ENABLE_INTERRUPTS do { enable_interrupts(); } while (0)

#else
#include <string.h>
#include "colors.h"
#include "buttons.h"
#include "flash.h"
#include "ir.h"
#include "fb.h"

#define DISABLE_INTERRUPTS
#define ENABLE_INTERRUPTS

#endif

#include "dynmenu.h"
#include "badge_apps/badge_monsters.h"

#define INIT_APP_STATE 0
#define GAME_MENU 1
#define RENDER_SCREEN 2
#define RENDER_MONSTER 3
#define TRADE_MONSTERS 4
#define CHECK_THE_BUTTONS 5
#define EXIT_APP 6
#define ENABLE_ALL_MONSTERS 7

static void app_init(void);
static void game_menu(void);
static void render_screen(void);
static void render_monster(void);
static void trade_monsters(void);
static void check_the_buttons(void);
static void setup_main_menu(void);
static void setup_monster_menu(void);
static void exit_app(void);
#ifdef __linux__
static void enable_all_monsters(void);
#endif

typedef void (*state_to_function_map_fn_type)(void);

static state_to_function_map_fn_type state_to_function_map[] = {
    app_init,
    game_menu,
    render_screen,
    render_monster,
    trade_monsters,
    check_the_buttons,
    exit_app,
#ifdef __linux__
    enable_all_monsters,
#endif
};

#define TOTAL_BADGES 300
#define BADGE_ID G_sysData.badgeId
#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

/* These need to be protected from interrupts. */
#define QUEUE_SIZE 5
static int queue_in;
static int queue_out;
static int packet_queue[QUEUE_SIZE] = { 0 };

static int screen_changed = 0;
static int smiley_x, smiley_y;
static int current_monster;
static int nmonsters = 0;
static int nvendor_monsters = 0;
static int app_state = INIT_APP_STATE;
static int trading_monsters_enabled = 0;

static const struct point smiley_points[] =
#include "badge_monster_drawings/smileymon.h"
static const struct point freshmon_points[] =
#include "badge_monster_drawings/freshmon.h"
// static const struct point mcturtle_points[] =
// #include "badge_monster_drawings/mcturtle.h"
static const struct point boothole_points[] =
#include "badge_monster_drawings/boothole.h"
static const struct point ghostcat_points[] =
#include "badge_monster_drawings/ghostcat.h"
static const struct point bluekeep_points[] =
#include "badge_monster_drawings/bluekeep.h"
static const struct point smbghost_points[] =
#include "badge_monster_drawings/smbghost.h"
static const struct point goat_mon_points[] =
#include "badge_monster_drawings/goat_mon.h"
static const struct point hrvamon_points[] =
#include "badge_monster_drawings/hrvamon.h"
static const struct point octomon_points[] =
#include "badge_monster_drawings/octomon.h"
static const struct point zombieload_points[] =
#include "badge_monster_drawings/zombieload.h"
static const struct point spectre_points[] =
#include "badge_monster_drawings/spectre.h"
static const struct point heartbleed_points[] =
#include "badge_monster_drawings/heartbleed.h"
static const struct point stacksmasher_points[] =
#include "badge_monster_drawings/stacksmasher.h"
static const struct point worm_points[] =
#include "badge_monster_drawings/worm.h"
static const struct point godzilla_points[] =
#include "badge_monster_drawings/godzilla.h"
static const struct point eddie_points[] =
#include "badge_monster_drawings/eddie.h"
static const struct point gopher_points[] =
#include "badge_monster_drawings/gophermon.h"
static const struct point tux_points[] =
#include "badge_monster_drawings/tuxmon.h"
static const struct point octocat_points[] =
#include "badge_monster_drawings/octocat.h"
static const struct point sourdough_points[] =
#include "badge_monster_drawings/sourdough_bread.h"
static const struct point babyyoda_points[] =
#include "badge_monster_drawings/babyyoda.h"
static const struct point babyyoda1_points[] =
#include "badge_monster_drawings/babyyoda1.h"
static const struct point btc_points[] =
#include "badge_monster_drawings/btc.h"
static const struct point mythosaur_points[] =
#include "badge_monster_drawings/mythosaur.h"

struct monster
{
    char name[20];
    int npoints;
    short status;
    int color;
    const struct point *drawing;
    char blurb[128];
};

struct monster monsters[] = {
    {"eddie", ARRAYSIZE(eddie_points), 0, YELLOW, eddie_points, "Sometimes\nprotective and\nEddie is the\nperfect guy\nevery girl\nwants to date"},
    {"godzilla", ARRAYSIZE(godzilla_points), 0, GREEN, godzilla_points, "a cross\nbetween a\ngorilla and\na whale"},
    {"worm", ARRAYSIZE(worm_points), 0, GREEN, worm_points, "An earthworm\nis a tube\nshaped,\nsegmented worm\nfound in the\nphylum\nAnnelida."},
    {"stacksmasher", ARRAYSIZE(stacksmasher_points), 0, CYAN, stacksmasher_points, "Stack smashing\nis a form of\nvulnerability\nwhere the\nstack of a\ncomputer\nprogram or OS\nis forced to\noverflow."},
    {"heartbleed", ARRAYSIZE(heartbleed_points), 0, RED, heartbleed_points, "Heartbleed is\na security bug\nin the OpenSSL\ncryptography\nlibrary"},
    {"spectre", ARRAYSIZE(spectre_points), 0, CYAN, spectre_points, "Spectre is a\nvulnerability\nthat affects\nmodern\nprocessors\nthat perform\nbranch\nprediction."},
    {"zombieload", ARRAYSIZE(zombieload_points), 0, WHITE, zombieload_points, "This one has\nthe delightful\nname of\nZombieLoad"},
    {"octomon", ARRAYSIZE(octomon_points), 0, MAGENTA, octomon_points, "a monster made\nwith 8 sides"},
    {"hrvamon", ARRAYSIZE(hrvamon_points), 0, WHITE, hrvamon_points, "Come check out\nHackRVA maker\nspace Thursday\nnights at 7.\n\n1600 Roseneath\nRoad Suite E\nRichmond, VA\n23230"},
    // {"mcturtle", ARRAYSIZE(mcturtle_points), 0, WHITE, mcturtle_points, "Turtles are reptiles with hard shells that protect them from predators"},
    {"GHOSTCAT", ARRAYSIZE(ghostcat_points), 0, YELLOW, ghostcat_points, "GHOSTCAT is a\nhigh-risk file\nread / include\nvulnerability\nin Tomcat"},
    {"BOOTHOLE", ARRAYSIZE(boothole_points), 0, GREEN, boothole_points, "BOOTHOLE is a\nvulnerability\nin GRUB2\nallowing\narbitrary code\nexecution\nduring secure\nboot"},
    {"BLUEKEEP", ARRAYSIZE(bluekeep_points), 0, BLUE, bluekeep_points, "BLUEKEEP is a\nremote code\nexecution\nvulnerability\nin Microsoft's\nRemote Desktop\nProtocol\nimplementation"},
    {"SMBGhost", ARRAYSIZE(smbghost_points), 0, BLUE, smbghost_points, "SMBGhost is a\nremote code\nexecution\nvulnerability\nin Microsoft's\nSMBv3"},
    {"goat_mon", ARRAYSIZE(goat_mon_points), 0, WHITE, goat_mon_points, "This goat is\nscared because\nthey are alone\nin the world"},
    {"freshmon", ARRAYSIZE(freshmon_points), 0, CYAN, freshmon_points, "a fusion of\nsmirkmon and\ngrinmon. This\nbadge monster\nlikes to troll\npeople really\nhard for fun"},
    {"smileymon", ARRAYSIZE(smiley_points), 0, YELLOW, smiley_points, "Have a nice\nday!"},
    {"gophermon", ARRAYSIZE(gopher_points), 0, CYAN, gopher_points, "Gophers are\nsmall, furry\nrodents that\nburrow tunnels\nthrough yards\nof North\nAmerica and\nCentral\nAmerica."},
    {"tuxmon", ARRAYSIZE(tux_points), 0, CYAN, tux_points, "Tux is a\npenguin\ncharacter and\nthe official\nbrand\ncharacter of\nthe Linux\nkernel."},
    {"octocat", ARRAYSIZE(octocat_points), 0, MAGENTA, octocat_points, "GitHub's mascot\nis an anthro-\npomorphized\n'octocat' with\nfive octopus-\nlike arms."},
    {"sourdoughmon", ARRAYSIZE(sourdough_points), 0, WHITE, sourdough_points, "Before I break \ndown and rye, \nI want you \nto know that \nI loaf you."},
    {"babyyodamon", ARRAYSIZE(babyyoda_points), 0, CYAN, babyyoda_points, "Baby Yoda. \nThat's it."},
    {"babyyodamon", ARRAYSIZE(babyyoda1_points), 0, CYAN, babyyoda1_points, "Baby Yoda. \nThat's it."},
    {"mythosaurmon", ARRAYSIZE(mythosaur_points), 0, RED, mythosaur_points, "You are \nMandalorian! \nYour ancestors \nrode the great \nMythosaur..."},
    {"bitcoinmon", ARRAYSIZE(btc_points), 0, YELLOW, btc_points, "a real bitcoin."},
};

struct monster vendor_monsters[] = {
    {"vgoatmon", ARRAYSIZE(smiley_points), 0, CYAN, goat_mon_points, "some nice words here"},
    {"vgoatmon", ARRAYSIZE(smiley_points), 0, CYAN, goat_mon_points, "some nice words here"},
    {"vsmileymon", ARRAYSIZE(smiley_points), 0, CYAN, smiley_points, "some nice words here"},
    {"vgoatmon", ARRAYSIZE(smiley_points), 0, CYAN, goat_mon_points, "some nice words here"}
};

struct dynmenu menu;
struct dynmenu_item menu_item[ARRAYSIZE(monsters) + ARRAYSIZE(vendor_monsters)];

int initial_mon;

#ifndef __linux__
static void (*old_callback)(struct IRpacket_t) = NULL;
static void register_ir_packet_callback(void (*callback)(struct IRpacket_t))
{
	/* This is pretty gross.  Ideally there should be some registration,
	 * unregistration functions provided by ir.[ch] and I shouldn't touch
	 * IRcallbacks[] directly myself, and all those hardcoded ir_app[1-7]()
	 * functions should disappear.
	 * Also, if an interrupt happens in the midst of the assignment we'll
	 * be in trouble.  I expect the assignment is probably atomic though.
	 */
	old_callback = IRcallbacks[BADGE_IR_GAME_ADDRESS].handler;
	IRcallbacks[BADGE_IR_GAME_ADDRESS].handler = callback;
}

static void unregister_ir_packet_callback(void)
{
	/* Gross. */
	IRcallbacks[BADGE_IR_GAME_ADDRESS].handler = old_callback;
}
#endif

enum menu_level_t {
    MAIN_MENU,
    MONSTER_MENU,
    DESCRIPTION
} menu_level;


static void enable_monster(int monster_id)
{
    if(monster_id < 0 || (size_t) monster_id > ARRAYSIZE(monsters) - 1)
        return;

    monsters[monster_id].status = 1;
    setNote(90, 4000);
    #ifdef __linux__
        printf("enabling monster: %d\n", monster_id);
    #endif
}

#ifdef __linux__
static void enable_all_monsters(void)
{
	int i;

	for (i = 0; i < nmonsters; i++)
		enable_monster(i);
	FbClear();
	FbMove(2, 2);
	FbWriteString("ALL MONSTERS\nENABLED!");
	app_state = RENDER_SCREEN;
}
#endif

static unsigned int build_packet(unsigned char cmd, unsigned char start,
            unsigned char address, unsigned short badge_id, unsigned short payload)
{
    return ((cmd & 0x01) << 31) |
        ((start & 0x01) << 30) |
        ((address & 0x01f) << 25) |
        ((badge_id & 0x1ff) << 16) |
        (payload);
}

static unsigned short get_payload(unsigned int packet)
{
	return (unsigned short) (packet & 0x0ffff);
}

static void process_packet(unsigned int packet)
{
    unsigned int payload;
    unsigned char opcode;

    if (packet == 32) /* Ignore spurious 32 that might come in. */
        return;

    payload = get_payload(packet);
    opcode = payload >> 12;

    if(opcode == OPCODE_XMIT_MONSTER){
        enable_monster(payload & 0x0ff);
    }
}

static void send_ir_packet(unsigned int packet)
{
	union IRpacket_u p;

	p.v = packet;
	p.v = packet | (1 << 30); /* Set command bit on outgoing packets */

	IRqueueSend(p);
}

static void check_for_incoming_packets(void)
{
    unsigned int new_packet;
    int next_queue_out;
    DISABLE_INTERRUPTS;
    while (queue_out != queue_in) {
        next_queue_out = (queue_out + 1) % QUEUE_SIZE;
        new_packet = packet_queue[queue_out];
        queue_out = next_queue_out;
        ENABLE_INTERRUPTS;
        process_packet(new_packet);
        DISABLE_INTERRUPTS;
    }
    ENABLE_INTERRUPTS;
}

static void draw_menu(void)
{
    int i;

    dynmenu_draw(&menu);
    if(menu_level != MONSTER_MENU){
        int nunlocked = 0;
        char available_monsters[3];
        char unlocked_monsters[3];
        for(i = 0; i < nmonsters; i++)
        {
            if(monsters[i].status == 1)
            {
                nunlocked++;
            }
        }

        for(i = 0; i < nvendor_monsters; i++)
        {
            if(vendor_monsters[i].status == 1)
            {
                nunlocked++;
            }
        }
        itoa(available_monsters, nmonsters + nvendor_monsters, 10);
        itoa(unlocked_monsters, nunlocked, 10);

        FbMove(1,25);
        FbWriteLine("Collected: ");
        FbWriteLine(unlocked_monsters);
        FbWriteLine("/");
        FbWriteLine(available_monsters);
    }
    app_state = RENDER_SCREEN;
}

static void change_menu_level(enum menu_level_t level)
{
    menu_level = level;
    static int which_item = -1;

    if (which_item == -1)
	which_item = initial_mon;

    if (strcmp(menu.title, "Monsters") == 0)
	which_item = menu.current_item;

    switch(level){
        case MAIN_MENU:
            setup_main_menu();
            screen_changed = 1;
            break;
        case MONSTER_MENU:
            setup_monster_menu();
            if (menu.max_items > which_item)
		    menu.current_item = which_item; /* Stay on the same monster */
            current_monster = menu.item[menu.current_item].cookie;
            screen_changed = 1;
            break;
        case DESCRIPTION:
            screen_changed = 1;
            return;
    }
}

static void show_message(char *message)
{
    #ifdef __linux__
        printf("%s\n", message);
    #endif

    FbClear();
    FbColor(WHITE);

    FbMove(8, 5);
    FbWriteString(message);
    FbMove(5, 120);
    FbWriteLine("<Back");

    change_menu_level(DESCRIPTION);
    app_state = RENDER_SCREEN;
    screen_changed = 1;
}

static void render_monster(void)
{
    int npoints, color;
    const struct point *drawing = current_monster > 100 ? vendor_monsters[current_monster - 100].drawing : monsters[current_monster].drawing;
    char *name;

    if(current_monster > 100)
    {
        name = vendor_monsters[current_monster-100].name;
        npoints = vendor_monsters[current_monster-100].npoints;
        color = vendor_monsters[current_monster-100].color;
    }
    else
    {
        name = monsters[current_monster].name;
        npoints = monsters[current_monster].npoints;
        color = monsters[current_monster].color;
    }

    FbClear();
    if(current_monster == initial_mon)
    {
        FbMove(0,10);
        FbWriteLine("--starting-mon--");
    }
    FbMove(0,0);
    FbWriteLine(name);
    FbDrawObject(drawing, npoints, color, smiley_x, smiley_y, 410);

    FbColor(WHITE);
    FbMove(43,120);
    FbWriteLine("|down|");
    FbColor(GREEN);

    FbMove(5, 120);
    FbWriteLine("<Back");


    FbMove(90,120);
    FbWriteLine("desc>");

    FbSwapBuffers();
    screen_changed = 1;
    app_state = RENDER_SCREEN;
}

static void trade_monsters(void)
{
	static int counter = 0;

	counter++;
	if ((counter % 10000) == 0) { /* transmit our monster IR packet */
		send_ir_packet(build_packet(1,1,BADGE_IR_GAME_ADDRESS, BADGE_IR_BROADCAST_ID,
			(OPCODE_XMIT_MONSTER << 12) | (initial_mon & 0x01ff)));
	    setNote(50, 100);
	}
	if (!trading_monsters_enabled) {
		FbClear();
		FbMove(10, 60);
		FbWriteLine("TRADING");
		FbMove(10, 70);
		FbWriteLine("MONSTERS!");
		screen_changed = 1;
		trading_monsters_enabled = 1;
	}
	app_state = RENDER_SCREEN;
}

void render_screen_save_monsters(void) {
    static unsigned char current_index = 0;
    int npoints, color;
    if(current_index == ARRAYSIZE(monsters)) {
        current_index = 0;
    }
    else {
        current_index++;
    }

    const struct point *drawing = monsters[current_index].drawing;
    npoints = monsters[current_index].npoints;
    color = monsters[current_index].color;
    FbClear();
    FbColor(BLACK);
    FbDrawObject(drawing, npoints, color, 64, 64, 410);
}

static void render_screen(void)
{
    app_state = CHECK_THE_BUTTONS;
    if (!screen_changed)
        return;
    FbPushBuffer();
    screen_changed = 0;
}

#ifdef __linux__
static void print_menu_info(void)
{
    /* int next_state = menu.item[menu.current_item].next_state
       system("clear"); */
    printf("current item: %d\nmenu level: %d\ncurrent monster: %d\n menu item: %s\nn-menu-items: %d\ncookie_monster: %d\n",
    menu.current_item, menu_level, current_monster, menu.item[menu.current_item].text, menu.nitems, menu.item[menu.current_item].cookie);
}
#endif

static void check_the_buttons(void)
{
    int something_changed = 0;

    /* If we are trading monsters, stop trading monsters on a button press */
    if (trading_monsters_enabled) {
		if (UP_BTN_AND_CONSUME ||
			DOWN_BTN_AND_CONSUME ||
			LEFT_BTN_AND_CONSUME ||
			RIGHT_BTN_AND_CONSUME ||
			BUTTON_PRESSED_AND_CONSUME) {
			trading_monsters_enabled = 0;
			app_state = GAME_MENU;
			return;
		}
    }

    switch(menu_level){
        case MAIN_MENU:
            if (UP_BTN_AND_CONSUME)
            {
                dynmenu_change_current_selection(&menu, -1);
		screen_changed = 1;
                something_changed = 1;
            }
            else if (DOWN_BTN_AND_CONSUME)
            {
                dynmenu_change_current_selection(&menu, 1);
		screen_changed = 1;
                something_changed = 1;
            }
            else if (LEFT_BTN_AND_CONSUME)
            {
            }
            else if (RIGHT_BTN_AND_CONSUME)
            {
            }
            else if (BUTTON_PRESSED_AND_CONSUME)
            {
                switch(menu.current_item){
                    case 0:
                        change_menu_level(MONSTER_MENU);
                        current_monster = menu.item[menu.current_item].cookie;
                        break;
                    case 1:
                        app_state = TRADE_MONSTERS;
                        break;
                    case 2:
                        app_state = EXIT_APP;
                        break;
#ifdef __linux__
		    case 3:
			app_state = ENABLE_ALL_MONSTERS;
			break;
#endif
                }
            }

            break;
        case MONSTER_MENU:
            if (UP_BTN_AND_CONSUME)
            {
                dynmenu_change_current_selection(&menu, -1);
		screen_changed = 1;
                current_monster = menu.item[menu.current_item].cookie;
                render_monster();
            }
            else if (DOWN_BTN_AND_CONSUME)
            {
                dynmenu_change_current_selection(&menu, 1);
		screen_changed = 1;
                current_monster = menu.item[menu.current_item].cookie;
                render_monster();
            }
            else if (LEFT_BTN_AND_CONSUME)
            {
                change_menu_level(MAIN_MENU);
                something_changed = 1;
            }
            else if (RIGHT_BTN_AND_CONSUME)
            {
                if(current_monster >= 100)
                {
                    show_message(vendor_monsters[current_monster-100].blurb);
                }
                else
                {
                    show_message(monsters[current_monster].blurb);
                }
            }
            else if (BUTTON_PRESSED_AND_CONSUME)
            {
                #ifdef __linux__
                    print_menu_info();
                #endif
                if(current_monster >= 100)
                {
                    show_message(vendor_monsters[current_monster-100].blurb);
                }
                else
                {
                    show_message(monsters[current_monster].blurb);
                }
            }
            break;
        case DESCRIPTION:
            if (UP_BTN_AND_CONSUME)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (DOWN_BTN_AND_CONSUME)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (LEFT_BTN_AND_CONSUME)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (RIGHT_BTN_AND_CONSUME)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED_AND_CONSUME)
            {
                change_menu_level(MONSTER_MENU);
            }
            break;
        default:
            break;
        }

    if (trading_monsters_enabled && !something_changed) {
	app_state = TRADE_MONSTERS;
	return;
    }

    if (something_changed && app_state == CHECK_THE_BUTTONS)
        app_state = GAME_MENU;
    return;
}

static void setup_monster_menu(void)
{
    int i;
    dynmenu_clear(&menu);
    menu.menu_active = 0;
    strcpy(menu.title, "Monsters");

    for(i = 0; i < nmonsters; i++){
        if(monsters[i].status)
            dynmenu_add_item(&menu, monsters[i].name, RENDER_MONSTER, i);
    }
    if (current_monster < menu.max_items)
        menu.current_item = current_monster;
    else
        current_monster = menu.current_item;

    #if 0
    for(i = 0; i < nvendor_monsters; i++){
        if(vendor_monsters[i].status)
            menu_add_item(vendor_monsters[i].name, RENDER_MONSTER, i+100);
    }
    #endif

    screen_changed = 1;
    render_monster();
}

static void setup_main_menu(void)
{
    dynmenu_clear(&menu);
    strcpy(menu.title, "Badge Monsters");
    dynmenu_add_item(&menu, "Monsters", RENDER_SCREEN, 0);
    dynmenu_add_item(&menu, "Trade Monsters", TRADE_MONSTERS, 1);
    dynmenu_add_item(&menu, "EXIT", EXIT_APP, 2);
#ifdef __linux__
    /* For testing purposes allow enabling all monsters on linux */
    dynmenu_add_item(&menu, "Test Monsters", ENABLE_ALL_MONSTERS, 3);
#endif
    screen_changed = 1;
}

static void game_menu(void)
{
    draw_menu();
    check_for_incoming_packets();
    app_state = RENDER_SCREEN;
}

static void load_from_flash(void){
    /*
    load from flash should load a list of monsterIDs that have been enabled
    for each monsterID, it should run enable_monster() function
    */
}

static void save_to_flash(void){
    /*
    save to flash should save some form of a list of monsterIDs that have been unlocked
    */
}

static void exit_app(void)
{
    app_state = INIT_APP_STATE;
    save_to_flash();
    unregister_ir_packet_callback();
    returnToMenus();
}

static void ir_packet_callback(struct IRpacket_t packet)
{
	/* Interrupts will be already disabled when this is called. */
	int next_queue_in;

	next_queue_in = (queue_in + 1) % QUEUE_SIZE;
	if (next_queue_in == queue_out) /* queue is full, drop packet */
		return;
	memcpy(&packet_queue[queue_in], &packet, sizeof(packet_queue[0]));
	queue_in = next_queue_in;
}

static void app_init(void)
{
    FbInit();
    app_state = INIT_APP_STATE;
    register_ir_packet_callback(ir_packet_callback);
    dynmenu_init(&menu, menu_item, ARRAYSIZE(menu_item));

    change_menu_level(MAIN_MENU);
    app_state = GAME_MENU;
    screen_changed = 1;
    smiley_x = LCD_XSIZE / 2;
    smiley_y = LCD_XSIZE / 2;
    nmonsters = ARRAYSIZE(monsters);
    nvendor_monsters = ARRAYSIZE(vendor_monsters);
    initial_mon = BADGE_ID % nmonsters;
    current_monster = initial_mon;
    enable_monster(initial_mon);
    load_from_flash();
}

int badge_monsters_cb(void)
{
    state_to_function_map[app_state]();
    return 0;
}

#ifdef __linux__

int main(int argc, char *argv[])
{
    char *serial_port = NULL;

#define IRXMIT_UDP_PORT 12345
#define LASERTAG_UDP_PORT 12346

	if (argc >= 2)
		serial_port = argv[1];
	setup_linux_ir_simulator(serial_port, IRXMIT_UDP_PORT, LASERTAG_UDP_PORT);
    start_gtk(&argc, &argv, badge_monsters_cb, 30);
    return 0;
}

#endif
