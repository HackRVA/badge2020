
This is a basic intro about how to add apps to the HackRVA badge, and how
to do so in such a way that most of your work of compiling, testing, and
debugging can be done on linux rather than on the badge which is quite a
bit more convenient.

There is a linux environment
that mimics much of the badge environment which allows you to develop badge
applications on linux without having to flash the code to the badge to check
every little thing. When you are reasonably sure that your code is working,
then you can flash it to the badge and check if it works as expected in the
real badge environment.

Step 0: Install the compilers on your linux system.

For the badge emulator, you'll need your system's native compiler.
On an ubuntu-based system, `sudo apt-get install build-essentials`. Other systems will differ. You
will also need some GTK libs, `sudo apt-get install libgtk2.0-dev` at
least (again, it will differ on systems not based on debian.)

To compile the badge firmware to run on the badge, you will need
another compiler, the Microchip XC32 compiler.
The commands to install it can be found in the docker file in
[`docker/badge-compiler/Dockerfile`](https://github.com/HackRVA/badge2020/blob/master/docker/badge-compiler/Dockerfile)

Step 1: Clone the linux badge environment:

```
	git clone git@github.com:HackRVA/badge2020.git
```

Step 2: Write your code:

```
	cd badge2020

	(hack on your code.  Add your app under the badge_apps directory)

	See badge_apps/sample_app.c for an example and badge_apps/badge-app-template.c
	for a template that you can copy and customize to make your badge app. There are
	instructions at the top of badge-app-template.c that explain how to do this, and
	most of the details explained below are already done for you in this file.

	Modify linux/Makefile to add your badge app following the other
	examples there.
```

There will be a few linux specific bits which you should surround by

```c
	#ifdef __linux__
	#endif
```

In particular, at the beginning you need the following two sections
enclosed in "#ifdef __linux__" sections:

At the beginning:

```c
	#ifdef __linux__
	#include "../linux/linuxcompat.h"
	#endif
```

And at the end:

```c
	#ifdef __linux__
	int main(int argc, char *argv[])
	{
		start_gtk(&argc, &argv, myapp_callback, 240);
		return 0;
	}
	#endif
```

Instead of `myapp_callback`, use whatever function name you used
for your app's callback entry point name.  The "240" in the above code
is the frequency in Hz at which your callback will be called. If your
app does very little calculation and spends most of its time waiting
for user input on the D-pad or button, then 30 might be a more
appropriate number.

Step 3: Write your app.  Check [BADGE-APP-HOWTO.md](https://github.com/HackRVA/badge2020/blob/master/BADGE-APP-HOWTO.md)
and `linux/linuxcompat.h` to see what functions you may call.  You may need
to include some header files to access some functions, and you may need to
include different header files on the badge
than on linux.

Use a pattern like this:

```c
	#ifdef __linux__
	#include <some_linux_header_file.h>
	#else
	#include <some_badge_header_file.h>
	#endif
```

There may be times when on linux, you need to call some function,
but on the badge you don't need such a function (or vice versa).

For such situations you can do something like this:

```c
	#ifdef __linux__
	#define LINUX_FUNCTION1 do { linux_function1(); } while (0)
	#else
	#define LINUX_FUNCTION1
	#endif
```

Then later in your code, you can say:

```c
	LINUX_FUNCTION1;
```

and on the badge, this will do nothing and generate no code, while on
linux, it will call linux_function1.

See [BADGE-APP-HOWTO.md](https://github.com/HackRVA/badge2020/blob/master/BADGE-APP-HOWTO.md) for
more information about the functions and variables your badge app might use.

Look also in include/fb.h and linux/linuxcompat.h as well as other badge apps
to get an idea of what functions you may call from your badge app to draw
or print messages on the screen, receive user input, etc. Unfortunately,
the header files and documentation for that type of thing is not currently
very good. Feel free to improve the situation.

Step 4: Build and run your app on linux:

```bash
you@yourmachine ~/github/badge2020/linux $ make bin/sample_app
cc -pthread -fsanitize=address -Wall --pedantic -g -pthread -isystem /usr/include/gtk-2.0 -isystem /usr/lib/x86_64-linux-gnu/gtk-2.0/include -isystem /usr/include/gio-unix-2.0/ -isystem /usr/include/cairo -isystem /usr/include/pango-1.0 -isystem /usr/include/atk-1.0 -isystem /usr/include/cairo -isystem /usr/include/pixman-1 -isystem /usr/include/libpng12 -isystem /usr/include/gdk-pixbuf-2.0 -isystem /usr/include/libpng12 -isystem /usr/include/pango-1.0 -isystem /usr/include/harfbuzz -isystem /usr/include/pango-1.0 -isystem /usr/include/glib-2.0 -isystem /usr/lib/x86_64-linux-gnu/glib-2.0/include -isystem /usr/include/freetype2 -c -I . linuxcompat.c -o linuxcompat.o
cc -pthread -fsanitize=address -Wall --pedantic -g -c bline.c -o bline.o
cc -pthread -fsanitize=address -Wall --pedantic -g -c ../src/achievements.c -I ../include -o achievements.o
cc -pthread -fsanitize=address -Wall --pedantic -g -pthread -isystem /usr/include/gtk-2.0 -isystem /usr/lib/x86_64-linux-gnu/gtk-2.0/include -isystem /usr/include/gio-unix-2.0/ -isystem /usr/include/cairo -isystem /usr/include/pango-1.0 -isystem /usr/include/atk-1.0 -isystem /usr/include/cairo -isystem /usr/include/pixman-1 -isystem /usr/include/libpng12 -isystem /usr/include/gdk-pixbuf-2.0 -isystem /usr/include/libpng12 -isystem /usr/include/pango-1.0 -isystem /usr/include/harfbuzz -isystem /usr/include/pango-1.0 -isystem /usr/include/glib-2.0 -isystem /usr/lib/x86_64-linux-gnu/glib-2.0/include -isystem /usr/include/freetype2 -c ../badge_apps/xorshift.c -o xorshift.o
cc -pthread -fsanitize=address -Wall --pedantic -g -pthread -isystem /usr/include/gtk-2.0 -isystem /usr/lib/x86_64-linux-gnu/gtk-2.0/include -isystem /usr/include/gio-unix-2.0/ -isystem /usr/include/cairo -isystem /usr/include/pango-1.0 -isystem /usr/include/atk-1.0 -isystem /usr/include/cairo -isystem /usr/include/pixman-1 -isystem /usr/include/libpng12 -isystem /usr/include/gdk-pixbuf-2.0 -isystem /usr/include/libpng12 -isystem /usr/include/pango-1.0 -isystem /usr/include/harfbuzz -isystem /usr/include/pango-1.0 -isystem /usr/include/glib-2.0 -isystem /usr/lib/x86_64-linux-gnu/glib-2.0/include -isystem /usr/include/freetype2 linuxcompat.o bline.o achievements.o xorshift.o -o bin/sample_app -I . -I ../include ../badge_apps/sample_app.c -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype -lgthread-2.0 -pthread -lglib-2.0
you@yourmachine ~/github/badge2020/linux $ bin/sample_app
````

![image of sample_app running](https://raw.githubusercontent.com/smcameron/hackrva-badge-boost/master/badgeboost.jpg)

The badge emulator library makes some attempt to detect if you're redrawing
the screen too frequently, and if it thinks you are, you may see this warning:

```
	Warning, high SwapBuffers() rate detected.
	Performance on badge likely to be terrible.
```

Step 4:  Get your app running on the badge.

3. Modify the Makefile to know about your files by adding them
   to the SRC_APPS_C variable.

4. Add a your header file into include/badge_apps/myapp.h (Substitute your appname for "myapp".)
In this header, put in a single function that is your app callback.  For instance:

```c
	#ifndef MYAPP_H__
	#define MYAPP_H__

	void myapp_callback(void);

	#endif
```

5. Modify src/menu.c:

```diff
diff --git a/src/menu.c b/src/menu.c
index 175d1d4..f0aad9a 100644
--- a/src/menu.c
+++ b/src/menu.c
@@ -15,6 +15,7 @@
 #include "badge_apps/conductor.h"
 #include "badge_apps/blinkenlights.h"
 #include "badge_apps/adc.h"
+#include "badge_apps/myapp.h"


 #define MAIN_MENU_BKG_COLOR GREY2
@@ -463,6 +464,7 @@ const struct menu_t games_m[] = {
    {"Blinkenlights", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)blinkenlights_cb}}, // Set other badges LED
    {"Conductor",     VERT_ITEM, FUNCTION, {(struct menu_t * )conductor_cb}}, // Tell other badges to play notes
    {"Sensors",       VERT_ITEM, FUNCTION, {(struct menu_t l* )adc_cb} },
+   {"MyApp",         VERT_ITEM, FUNCTION, {(struct menu_t * ) myapp} },
    {"Back",          VERT_ITEM|LAST_ITEM, BACK, {NULL} },
};
```

7. Maybe disable other apps.  If you're using the free compiler, and it complains
'...Compiler option (Optimize for size) ignored ...", then you may have to disable
other apps for your app to fit. If your app is too big, the badge will crash loop.
I don't know a way to check beforehand if your app is too big.

For example, to disable the maze app, comment out the #include of it's header file
and the entry in the games_m[] array, like so:

```diff
diff --git a/src/menu.c b/src/menu.c
index 15d2a89..30f9132 100644
--- a/src/menu.c
+++ b/src/menu.c
@@ -16,7 +16,7 @@
 #include "adc.h"
 #include "badge_apps/blinkenlights.h"
 #include "badge_apps/sample_app.h"
-#include "badge_apps/maze.h"
+// #include "badge_apps/maze.h"
 #include "badge_apps/sample_app.h"

 #define MAIN_MENU_BKG_COLOR GREY2
@@ -465,7 +465,7 @@ const struct menu_t games_m[] = {
    {"Blinkenlights", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)blinkenlights_cb}}, // Set other badges LED
    {"Conductor",     VERT_ITEM, FUNCTION, {(struct menu_t *)conductor_cb}}, // Tell other badges to play notes
    {"Sensors",       VERT_ITEM, FUNCTION, {(struct menu_t *)adc_cb} },
-   {"Maze",          VERT_ITEM, FUNCTION, {(struct menu_t *)maze_cb} },
+   // {"Maze",          VERT_ITEM, FUNCTION, {(struct menu_t *)maze_cb} },
    {"Sample App",    VERT_ITEM, FUNCTION, {(struct menu_t *)sample_app_cb} },
    {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}},
 };
```

8. Build it for the badge...

Type `make` in the top level directory of the `badge2020` project...

9. Flash the badge

Press the button on the badge as you simultaneously plug it into the USB port
of your computer.  A green LED should be flashing.

Next, run `sudo tools/bootloadit`. This will flash the firmware for the badge.

Then, unplug, and re-plug the badge into the USB port to reboot it.
At this point your program should be accessible via the main menu.

