/*********************************************

This is a badge app template.

To use this file, do the following steps:

1. Copy the file to a different name, the name of your app.
   e.g. cp badge-app-template.c myapp.c

   (Don't call it "myapp" please.)

2. Copy include/badge-app-template.h to include/myapp.h

   (Don't call it "myapp" please.)

2. Edit the file.  Change myprogram_cb to some other name, e.g. "myapp_cb"
   wherever it appears in this file. (Think of a real name, don't actually
   use myapp_cb, please.)

3. Modify linux/Makefile to build your program, for example:

	diff --git a/linux/Makefile b/linux/Makefile
	index 83034ce..b1b75f8 100644
	--- a/linux/Makefile
	+++ b/linux/Makefile
	@@ -13,6 +13,7 @@ LASERTAG=lasertag
 	 SMASHOUT=smashout
 	 MAZE=maze
 	 USERNAME=username
	+MYAPP=myapp
 
 	-all:   ${LINUX_OBJS} bin/${MAZE} bin/${IRXMIT} bin/${LASERTAG} bin/badge_monsters bin/fixup_monster bin/username bin/sample_app
	+all:   ${LINUX_OBJS} bin/${MAZE} bin/${IRXMIT} bin/${LASERTAG} bin/badge_monsters bin/fixup_monster bin/username bin/sample_app bin/myapp

 	
	@@ -63,6 +64,9 @@ bin/${LASERTAG}:      ${BIN} ${APPDIR}/${LASERTAG}.c linuxcompat.o bline.o
 	 bin/${SMASHOUT}:       ${BIN} ${APPDIR}/${SMASHOUT}.c linuxcompat.o
        	$(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${SMASHOUT} -I . -I ${BADGE_INC} ${APPDIR}/${SMASHOUT}.c ${GTKLDFLAGS}
 	
	+bin/${MYAPP}:  ${BIN} ${APPDIR}/${MYAPP}.c linuxcompat.o
	+       $(CC) ${CFLAGS} ${GTKCFLAGS} ${LINUX_OBJS} -o bin/${MYAPP} -I . -I ${BADGE_INC} ${APPDIR}/${MYAPP}.c ${GTKLDFLAGS}
	+
 	 bin/${MAZE}:   ${BIN} ${APPDIR}/${MAZE}.c linuxcompat.o bline.o xorshift.o achievements.o\
                        ${APPDIR}/bones_points.h \
                        ${BADGE_INC}/build_bug_on.h \

4. Modify the main Makefile, for example:

	diff --git a/Makefile b/Makefile
	index 999e7ec..c16fc6c 100644
	--- a/Makefile
	+++ b/Makefile
	@@ -51,7 +51,7 @@ SRC_APPS_C = \
        	badge_apps/blinkenlights.c badge_apps/conductor.c \
        	badge_apps/lasertag.c badge_apps/QC.c ${IRXMIT} \
        	badge_apps/username.c badge_apps/badge_monsters.c \
	-       badge_apps/smashout.c
	+       badge_apps/smashout.c badge_apps/badge-app-template.c
 	
 	 SRC_USB_C = USB/usb_device.c  USB/usb_function_cdc.c USB/usb_descriptors.c

5. Modify src/menu.c to add your app, for example:

	diff --git a/src/menu.c b/src/menu.c
	index efefd24..b56cb44 100644
	--- a/src/menu.c
	+++ b/src/menu.c
	@@ -16,6 +16,7 @@
 	 #include "blinkenlights.h"
 	 #include "adc.h"
 	 #include "smashout.h"
	+#include "badge-app-template.h"
 	 #include "QC.h"
 	 #include "lasertag.h"
 	 #include "username.h"
	@@ -499,6 +500,7 @@ const struct menu_t games_m[] = {
    	 {"Conductor",     VERT_ITEM, FUNCTION, {(struct menu_t *)conductor_cb}}, // Tell other badges to play notes
    	 {"Sensors",       VERT_ITEM, FUNCTION, {(struct menu_t *)adc_cb} },
    	 {"Smash Out",     VERT_ITEM, FUNCTION, {(struct menu_t *)smashout_cb} },
	+   {"My App",      VERT_ITEM, FUNCTION, {(struct menu_t *)myapp_cb} },
    	 {"Laser Tag",     VERT_ITEM, FUNCTION, {(struct menu_t *)lasertag_cb} },
    	 {"Badge Monsters",VERT_ITEM, FUNCTION, {(struct menu_t *)badge_monsters_cb} },
 	 #ifdef INCLUDE_IRXMIT

6.  Build the linux program:

    cd linux
    make bin/myapp

    Or whatever you called it, (you didn't call it myapp, right?)

7.  Build the main program:

    cd ..
    make clean
    make

8.  Delete all these instruction comments from your copy of the file.

9.  Modify the program to make it do what you want.

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

/* Program states.  Initial state is MYPROGRAM_INIT */
enum myprogram_state_t {
	MYPROGRAM_INIT,
	MYPROGRAM_RUN,
	MYPROGRAM_EXIT,
};

static enum myprogram_state_t myprogram_state = MYPROGRAM_INIT;

static void myprogram_init(void)
{
	FbInit();
	FbClear();
	myprogram_state = MYPROGRAM_RUN;
}

static void check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		/* Pressing the button exits the program. You probably want to change this. */
		myprogram_state = MYPROGRAM_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
	} else if (RIGHT_BTN_AND_CONSUME) {
	} else if (UP_BTN_AND_CONSUME) {
	} else if (DOWN_BTN_AND_CONSUME) {
	}
}

static void draw_screen()
{
	FbColor(WHITE);
	FbMove(10, SCREEN_YDIM / 2);
	FbWriteLine("HOWDY!");
	FbSwapBuffers();
}

static void myprogram_run()
{
	check_buttons();
	draw_screen();
}

static void myprogram_exit()
{
	myprogram_state = MYPROGRAM_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename myprogram_cb() something else. */
int myprogram_cb(void)
{
	switch (myprogram_state) {
	case MYPROGRAM_INIT:
		myprogram_init();
		break;
	case MYPROGRAM_RUN:
		myprogram_run();
		break;
	case MYPROGRAM_EXIT:
		myprogram_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, myprogram_cb, 30);
        return 0;
}
#endif
