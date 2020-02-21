#include "../include/build_bug_on.h"

/* Dimensions of the screen in pixels */
#define SCREEN_XDIM 132
#define SCREEN_YDIM 132

void FbInit(void); /* initialize the frame buffer */
void plot_point(int x, int y, void *context); /* Plot a point on the offscreen buffer */
void FbSwapBuffers(void); /* Push the data from offscreen buffer onscreen */
void FbPushBuffer(void) /* Push the data from offscreen buffer onscreen */;
void FbPaintNewRows(void); /* Same as FbPushBuffer but only paints new rows, potentially faster */

/* Draw a line */
void FbLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);

/* Draw a horizontal line (faster than FbLine) */
void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);

/* Draw a vertical line (faster than FbLine) */
void FbVerticalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);

/* Clear the frame buffer */
void FbClear(void);

/* Move the graphics "cursor" to x, y (e.g. for subsequent FbWriteLine()) */
void FbMove(unsigned char x, unsigned char y);

/* Write a string to the frame buffer */
void FbWriteLine(char *s);

/* Convert integer to ascii string */
void itoa(char *string, int value, int base);

/* Absolute value */
int abs(int x);

/* Yield back to the main loop */
void returnToMenus(void);

/* Set the current color */
void FbColor(int color);

#define BLUE    0
#define GREEN   1
#define RED     2
#define BLACK   3
#define WHITE   4
#define CYAN    5
#define YELLOW  6
#define MAGENTA 7

/* Play a note for a given duration.  This is synchronous. */
void setNote(int note, int duration);

/* Port numbers are in host byte order here. UDP is used if serial_port is NULL, otherwise serial port is used */
void setup_linux_ir_simulator(char *serial_port, unsigned short port_to_recv_from, unsigned short port_to_xmit_on);

void disable_interrupts(void);
void enable_interrupts(void);
void start_gtk(int *argc, char ***argv, int (*main_badge_function)(void), int callback_hz);

/* Functions for checking status of D-PAD and button (Use the macros below instead) */
int button_pressed_and_consume();
int up_btn_and_consume();
int down_btn_and_consume();
int left_btn_and_consume();
int right_btn_and_consume();

#define BUTTON_PRESSED_AND_CONSUME button_pressed_and_consume()
#define DOWN_BTN_AND_CONSUME down_btn_and_consume()
#define UP_BTN_AND_CONSUME up_btn_and_consume()
#define LEFT_BTN_AND_CONSUME left_btn_and_consume()
#define RIGHT_BTN_AND_CONSUME right_btn_and_consume()

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
	struct IRpacket_t p;
	unsigned int v;
};

extern struct sysData_t {
	char name[16];
	unsigned short badgeId;
	/* TODO: There is more stuff in the badge which we might want to expose if
	 * an app needs it.
	 */
} G_sysData;

void IRqueueSend(union IRpacket_u pkt);

/* This is tentative.  Not really sure how the IR stuff works.
   For now, I will assume I register a callback to receive 32-bit
   packets incoming from IR sensor. */
void register_ir_packet_callback(void (*callback)(struct IRpacket_t));
void unregister_ir_packet_callback(void);

/* Set the Flair LED color */
void flareled(unsigned char r, unsigned char g, unsigned char b);

#define IR_APP1 19
#define IR_APP2 20
#define IR_APP3 21
#define IR_APP4 22
#define IR_APP5 23
#define IR_APP6 24
#define IR_APP7 25

extern char username[10];
int flashWriteKeyValue(unsigned int valuekey, char *value, unsigned int valuelen);

struct point
{
    signed char x, y;
};

/* FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
 * (0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
 * so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
 * than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
 * polygons can be constructed via this program: https://github.com/smcameron/vectordraw
 * as well as allowing scaling.
 */
void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale);

/* This timestamp thing is just a counter that periodically increments. The linux one won't increment
 * at the same rate as the one on the badge though.
 */
extern volatile int timestamp;

