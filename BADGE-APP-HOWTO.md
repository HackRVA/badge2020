
BADGE APP HOWTO
---------------

This assumes you've got your system set up and are able to compile
the badge firmware, and that now you're ready to start working on your
badge app. For instructions on setting up your linux system for badge
app development, see [linux/README.md](https://github.com/HackRVA/badge2020/tree/master/linux)

It is also assumed that you are an OK C programmer, though even beginner
C programmers are encouraged to give it a try.

Overview
--------

The badge uses a simple, cooperative multitasking system with no memory
protection. Each badge app must provide a callback function (that you write),
and when the badge app is active, that function will be repeatedly called.

This function can do whatever you want it to, but it must return
within a short time as part of the "cooperative multitasking". If it
doesn't return in a timely manner, certain other functions of the badge
may not work correctly (e.g. the clock might not advance correctly, LEDs
might not blink, USB serial traffic may not work, etc.)

When the badge app is ready to exit, it should call returnToMenus().

A typical badge app callback function might look something like this:


```c
static enum my_badge_app_state_t {
	MY_BADGE_APP_INIT = 0,
	MY_BADGE_APP_POLL_INPUT,
	MY_BADGE_APP_DRAW_SCREEN,
	MY_BADGE_APP_EXIT,
	/* ... whatever other states your badge app might have go here ... */
} my_badge_app_state = MY_BADGE_APP_INIT;

int my_badge_app_cb(void)
{
	switch (my_badge_app_state) {
	case MY_BADGE_APP_INIT:
		init_badge_app();	/* you write this function */
		break;
	MY_BADGE_APP_POLL_INPUT:
		poll_input();		/* you write this function */
		break;
	MY_BADGE_APP_DRAW_SCREEN:
		draw_screen();		/* you write this function */
		break;
	MY_BADGE_APP_EXIT:
		returnToMenus();
		break;

	/* ... Etc ... */

	default:
		break;
	}
	return 0;
}
```

Badge App Template
------------------

To get started quickly, a badge app template file is provided which you can copy
and modify to get started making your badge app quickly without having to type in
a bunch of boilerplate code before anything even begins to work.

* [badge_apps/badge-app-template.c](https://github.com/HackRVA/badge2020/blob/master/badge_apps/badge-app-template.c)

It is extensively commented to help you know how it's meant to be used.

Drawing on the Screen
---------------------

The screen is 132x132 pixels and the interface for drawing on the screen
is mostly defined in [include/fb.h](https://github.com/HackRVA/badge2020/blob/master/include/fb.h).
(BTW, use `LCD_XSIZE` and `LCD_YSIZE` rather than hard coding 132.)  Note also that there is
a 2 pixel border of invisible pixels around the edge of the screen, so the visible pixels are
between 2 and 129 inclusive.

Here is a (probably incomplete) list of functions you may call for drawing on the screen:

Initializing and Clearing the Framebuffer
-----------------------------------------

```
	void FbInit(void);
		You must call this before doing anything else with the framebuffer

	void FbClear(void);
		Clear the framebuffer with the background color
```

Updating the Screen
-------------------

```
	void FbSwapBuffers();
		Copy the framebuffer contents to the actual screen. This makes whatever
		changes you've made to the framebuffer visible on the badge screen. This
		call is quite slow. You might get a 10-15Hz refresh rate using this, so
		if your app needs to update the screen quickly, see FbPaintNewRows() which
		might help.

	void FbPaintNewRows();
		Similar to FbSwapBuffers(), but potentially faster.  All the badge drawing
		operations track which rows of the framebuffer they have modified. FbPaintNewRows()
		only copies the rows modified since the last time the screen was updated, so
		if you are only modifying a few rows (e.g. as would be the case for rendering
		the ball in a breakout style arcade game), then FbPaintNewRows() can be much
		faster than FbSwapBuffers().  If you are modifying a lot of rows, FbSwapBuffers()
		may be faster.

	void FbPushBuffer(void);
		Similar to FbSwapBuffers(), but it does not clear the physical screen first.
		This can be useful for making incremental changes to a consistent scene.
		(NOTE: the linux emulator does not currently implement this correctly.)
```

Colors
------

There are 8 colors.

Actually there are more than 8 colors, but the above colors are those known by the
linux badge emulator that also work on the actual badge.  If you need more than
those colors, see
[include/colors.h](https://github.com/HackRVA/badge2020/blob/master/include/colors.h)

```

	BLUE
	GREEN
	RED
	BLACK
	WHITE
	CYAN
	YELLOW
	MAGENTA

	void FbColor(int color);
		Sets the current foreground color, which subsequent drawing functions will then use.

	void FbBackgroundColor(int color);
		Sets the current background color, which subsequent drawing functions and FbClear()
		will then use.
```

Drawing Lines and Points and other things
-----------------------------------------

```
	void plot_point(int x, int y, void *context);
		plots a point on the framebuffer at (x, y)

	void FbLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x2, y2)

	void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x2, y1) (y2 is unused)  Faster than FbLine().

	void FbVerticalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
		draws a line from (x1, y1) to (x1, y2) (x2 is unused)  Faster than FbLine().

	struct point
	{
		signed char x, y;
	};

	void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale);
		FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
		(0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
		so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
		than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
		polygons can be constructed via this program: https://github.com/smcameron/vectordraw
		as well as allowing scaling.
```

The following are also available, however they are not implemented in
the linux badge emulator. See [include/fb.h](https://github.com/HackRVA/badge2020/blob/master/include/fb.h).

```
	void FbSprite(unsigned char picId, unsigned char imageNo);
	void FbCharacter(unsigned char charin);
	void FbFilledRectangle(unsigned char width, unsigned char height);
	void FbPrintChar(unsigned char charin, unsigned char x, unsigned char y);
	void FbRectangle(unsigned char width, unsigned char height);
	void FbImage(unsigned char assetId, unsigned char seqNum);
	void FbImage8bit(unsigned char assetId, unsigned char seqNum);
	void FbImage4bit(unsigned char assetId, unsigned char seqNum);
	void FbImage2bit(unsigned char assetId, unsigned char seqNum);
	void FbImage1bit(unsigned char assetId, unsigned char seqNum

```

Moving the "Cursor"
-------------------

```
	void FbMove(unsigned char x, unsigned char y);
		Move the cursor to (x, y)

	void FbMoveRelative(char x, char y);
		Move the cursor relative to its current position, you can think of it as doing:

			cursor.x += x;
			cursor.y += y;

	void FbMoveX(unsigned char x);
		Sets the x position of the cursor

	void FbMoveY(unsigned char y);
		Sets the y position of the cursor
```

Writing Strings to the Framebuffer
----------------------------------

```
	All strings are assumed to be NULL terminated.

	void FbWriteLine(char *s);
		Writes the string s to the framebuffer. The cursor is positioned
		just to the right of the last character of the string. Newlines are
		not handled specially.

	void FbWriteString(char *s, unsigned char length);
		Writes a string with the specified length to the framebuffer.
		Newlines are handled.

	void FbWriteZString(char *s);
		Writes a string with the specified length to the framebuffer.
		Newlines are handled. Same as FbWriteString(), but a length
		parameter is not required.
```

Buttons and Directional-Pad Inputs
----------------------------------

```
	Several macros are defined for handling the buttons and the Directional-pad.

	BUTTON_PRESSED_AND_CONSUME
	DOWN_BTN_AND_CONSUME
	UP_BTN_AND_CONSUME
	LEFT_BTN_AND_CONSUME
	RIGHT_BTN_AND_CONSUME

	Each of these macros returns true when the corresponding input is made
	by the user.

	Here's an example of typical usage:

	if (BUTTON_PRESSED_AND_CONSUME) {
		generate_response_to_button_press(); /* you write this function. */
	} else if (DOWN_BTN_AND_CONSUME) {
		generate_response_to_dpad(DOWN); /* you write this function. */
	} if (DOWN_BTN_AND_CONSUME) {
		generate_response_to_dpad(UP);
	} else if (LEFT_BTN_AND_CONSUME) {
		generate_response_to_dpad(LEFT);
	} if (RIGHT_BTN_AND_CONSUME) {
		generate_response_to_dpad(RIGHT);
	}
```

Infrared Transmitter and Receiver
---------------------------------

```
		TODO: This documentation is not very good.

		Look in badge_monsters.c and lasertag.c badge apps to see examples of the IR
		system being used. The badge_monsters.c one is simpler, the lasertag.c one is
		more complicated.

		/* Emulate some stuff from ir.c */
		extern int IRpacketOutNext;
		extern int IRpacketOutCurr;
		#define MAXPACKETQUEUE 16
		#define IR_OUTPUT_QUEUE_FULL (((IRpacketOutNext+1) % MAXPACKETQUEUE) == IRpacketOutCurr)

		struct IRpacket_t {
			/* Note: this is different than what the badge code actually uses.
			 * The badge code uses 32-bits worth of bitfields.  Bitfields are
			 * not portable, nor are they clear, and *which* particular bits
			 * any bitfields land in are completely up to the compiler.
			 * Relying on the compiler using any particular bits is a programming
			 * error, so don't actually use this. Instead always
			 * use the "v" element of the IRpacket_u union.
			 */
			unsigned int v;
		};

		union IRpacket_u {
			struct IRpacket_t p;		/* <---- DON'T USE THIS */
			unsigned int v;			/* <---- USE THIS */
		};


		void IRqueueSend(union IRpacket_u pkt);
			Queues an IR packet for transmission.

		/* This is tentative.  Not really sure how the IR stuff works.
		   For now, I will assume I register a callback to receive 32-bit
		   packets incoming from IR sensor. */
		void register_ir_packet_callback(void (*callback)(struct IRpacket_t));
		void unregister_ir_packet_callback(void);

		#define IR_APP1 19
		#define IR_APP2 20
		#define IR_APP3 21
		#define IR_APP4 22
		#define IR_APP5 23
		#define IR_APP6 24
		#define IR_APP7 25
```

Badge ID and User Name:
-----------------------

```
	Each badge has a unique ID which is a 16-bit unsigned short burned into the firmware
	at the time the badge is firmware is flashed, and a user assignable name (using the
	username badge app).

		extern struct sysData_t {
			char name[16];
			unsigned short badgeId;
			/* TODO: There is more stuff in the badge which we might want to expose if
			 * an app needs it.
			 */
		} G_sysData;
```

Audio
-----

```

	Sound is very rudimentary:

	void setNote(int frequency, int duration);
		Plays a sound at specified frequency (in Hz) for duration (milliseconds).
		It does not return until duration has elapsed, so you cannot do anything
		else while the note is playing.

	TODO:
		There appear to be more audio related functions in include/assets.h and
		src/assets.h, but I don't know about them, and the linux badge emulator does
		not implement ANY of the audio functionality, not even setNote().
```

Setting the Flair LED Color
---------------------------

```
	void flareled(unsigned char r, unsigned char g, unsigned char b);
		Sets the color of the flair RGB LED to the given red, green and blue values
```

Flash Memory Access
-------------------

```
	int flashWriteKeyValue(unsigned int valuekey, char *value, unsigned int valuelen);
		Writes a string to flash memory with the given key.
```

Exiting the Badge App:
----------------------

```
	void returnToMenus(void);
		"Exits" the badge app and returns to the main menu. (In the linux
		badge emulator, this actually calls exit(3).)
```

Miscellaneous Functions:
------------------------

```

	void itoa(char *string, int value, int base);
		Converts the integer value into an ascii string in the given base. The string
		must have enough memory to hold the converted value.  (Note: the linux badge
		emulator currently ignores base, and assumes you meant base 10.)

	char *strcpy(char *dest, const char *src);
		Copies NULL terminated string src to dest. There must be enough room in dest.

	char *strcat(char *dest, const char *src);
		Concatenates NULL terminated string src onto the end of NULL terminated string dest.
		There must be enough room in dest.

	int abs(int x);
		Returns the absolute value of integer x.

	short sine(int a);
	short cosine(int a);
		Returns 1024 * the sine and 1024 * the cosine of angle a, where a is in units
		of 128th's of a circle. Typical usage is as follows:

		int angle_in_degress = 90; /* degrees */
		int speed = 20;

		int vx = (speed * cosine((128 * angle_in_degrees) / 360)) / 1024;
		int vy = (speed * -sine((128 * angle_in_degrees) / 360)) / 1024;

	extern volatile int timestamp;
		There is a volatile int "timestamp" which periodically increments
		which a badge_app can sample. NOTE: the linux badge emulator's timestamp
		will not increment at the same rate as the real badge's timestamp.
```

Pseudorandom numbers
--------------------

Some badge apps may have a need for pseudorandom numbers. In the badge_apps directory,
you'll find xorshift.h and xorshift.c which you can use.

```
	unsigned int xorshift(unsigned int *state);
		Returns a pseudo random number derived from seed, *state, and scrambles
		*state to prepare for the next call.  The state variable should be initialized
		to a random seed which should not be zero, and should contain a fair number of
		1 bits and a fair number of 0 bits.

		Typical usage:

		unsigned int my_state = 0xa5a5a5a5 ^ timestamp; // For example.

		Then you can do something like this:

		int random_number_between_0_and_1000(void)
		{
		   return (xorshift(&my_state) % 1000);
		}

```

For more information about how this pseudorandom number generator is implemented,
see https://en.wikipedia.org/wiki/Xorshift#Example_implementation.

Menus
-----

There is a library for badge app menus. The interface for this library is defined in
[include/dynmenu.h](https://github.com/HackRVA/badge2020/blob/master/include/dynmenu.h)
and the implementation is in [src/dynmenu.c](https://github.com/HackRVA/badge2020/blob/master/src/dynmenu.c).
The types and functions declared in dynmenu.h and the way they are meant to be used are are
fairly well documented within [the header file](https://github.com/HackRVA/badge2020/blob/master/include/dynmenu.h).

There is also *another*, different menu system defined in
[include/badge_menu.h](https://github.com/HackRVA/badge2020/blob/master/include/badge_menu.h),
which is used for the badge main menu, and it can also be used by badge apps.
The API for this is a bit more complicated, while at the same time more
limiting, at least in some ways, as it does not (I think) allow for dynamically
changing the menu elements. That is, if you need to have a menu item like "Take X",
where X is replaced at runtime with some other not known at compile time,
then include/badge_menu.h won't help you (badge_apps/maze.c has menu items like this).

Achievements
------------

If you want to add "achievements" to a badge app (i.e. a game) which are persistently stored in
flash memory, a library is provided to help with this. This simple interface is documented in
[include/achievements.h](https://github.com/HackRVA/badge2020/blob/master/include/achievements.h).

You'll need to add any achievements specific to your badge app to the enumerated type defined
there.


