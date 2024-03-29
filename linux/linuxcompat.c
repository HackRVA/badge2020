#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#define UNUSED __attribute__((unused))

#include "bline.h"
#include "linuxcompat.h"
#include "stacktrace.h"
#include "../include/build_bug_on.h"

static char *program_title;

#define FIFO_TO_BADGE "/tmp/fifo-to-badge"

/* Define only one of these as 1 */
#define USE_2016_BADGE_FONT 0
#define USE_2019_BADGE_FONT 1

#if USE_2016_BADGE_FONT
#define font_2_width 8
#define font_2_height 336
static const char font_2_bits[] = {
   0x04, 0x0a, 0x0a, 0x0a, 0x1f, 0x11, 0x11, 0x11, 0x07, 0x09, 0x09, 0x09,
   0x0f, 0x11, 0x11, 0x0f, 0x0e, 0x11, 0x01, 0x01, 0x01, 0x01, 0x11, 0x0e,
   0x07, 0x09, 0x11, 0x11, 0x11, 0x11, 0x09, 0x07, 0x1f, 0x01, 0x01, 0x01,
   0x0f, 0x01, 0x01, 0x1f, 0x1f, 0x01, 0x01, 0x01, 0x0f, 0x01, 0x01, 0x01,
   0x0e, 0x11, 0x01, 0x01, 0x19, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x11, 0x11,
   0x1f, 0x11, 0x11, 0x11, 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f,
   0x1f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x06, 0x11, 0x09, 0x05, 0x03,
   0x07, 0x09, 0x11, 0x11, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f,
   0x11, 0x1b, 0x15, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x13, 0x15,
   0x15, 0x19, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e,
   0x0f, 0x11, 0x11, 0x11, 0x0f, 0x01, 0x01, 0x01, 0x0e, 0x11, 0x11, 0x11,
   0x11, 0x11, 0x09, 0x16, 0x0f, 0x11, 0x11, 0x11, 0x0f, 0x09, 0x11, 0x11,
   0x0e, 0x11, 0x01, 0x01, 0x0e, 0x10, 0x11, 0x0e, 0x1f, 0x04, 0x04, 0x04,
   0x04, 0x04, 0x04, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e,
   0x11, 0x11, 0x11, 0x0a, 0x0a, 0x0a, 0x04, 0x04, 0x11, 0x11, 0x11, 0x11,
   0x15, 0x15, 0x15, 0x0a, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x0a, 0x11, 0x11,
   0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04, 0x1f, 0x10, 0x10, 0x08,
   0x04, 0x02, 0x01, 0x1f, 0x0e, 0x11, 0x15, 0x15, 0x15, 0x15, 0x11, 0x0e,
   0x06, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f, 0x0e, 0x11, 0x10, 0x08,
   0x04, 0x02, 0x01, 0x1f, 0x1f, 0x10, 0x08, 0x04, 0x08, 0x10, 0x11, 0x0e,
   0x10, 0x11, 0x11, 0x11, 0x1f, 0x10, 0x10, 0x10, 0x1f, 0x11, 0x01, 0x01,
   0x0e, 0x10, 0x11, 0x0e, 0x0e, 0x11, 0x01, 0x01, 0x0f, 0x11, 0x11, 0x0e,
   0x1f, 0x11, 0x10, 0x08, 0x04, 0x04, 0x04, 0x04, 0x0e, 0x11, 0x11, 0x0e,
   0x11, 0x11, 0x11, 0x0e, 0x0e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10, 0x10,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00,
   0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04,
   0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif

#if USE_2019_BADGE_FONT
#define font8x8_width 8
#define font8x8_height 1024
const unsigned char font8x8_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08,
   0x08, 0x00, 0x08, 0x00, 0x00, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00,
   0x14, 0x14, 0x3e, 0x14, 0x3e, 0x14, 0x14, 0x00, 0x08, 0x1c, 0x0a, 0x1c,
   0x28, 0x1c, 0x08, 0x00, 0x00, 0x04, 0x14, 0x08, 0x14, 0x10, 0x00, 0x00,
   0x04, 0x0a, 0x0a, 0x04, 0x0a, 0x0a, 0x14, 0x00, 0x00, 0x08, 0x08, 0x08,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x04, 0x04, 0x04, 0x04, 0x08, 0x00,
   0x00, 0x04, 0x08, 0x08, 0x08, 0x08, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0c,
   0x1e, 0x0c, 0x12, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00,
   0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x1c, 0x08,
   0x00, 0x10, 0x10, 0x08, 0x04, 0x02, 0x02, 0x00, 0x00, 0x08, 0x14, 0x14,
   0x14, 0x14, 0x08, 0x00, 0x00, 0x08, 0x0c, 0x08, 0x08, 0x08, 0x1c, 0x00,
   0x00, 0x0c, 0x12, 0x10, 0x0c, 0x02, 0x1e, 0x00, 0x00, 0x1e, 0x08, 0x0c,
   0x10, 0x12, 0x0c, 0x00, 0x00, 0x08, 0x0c, 0x0a, 0x1e, 0x08, 0x08, 0x00,
   0x00, 0x1e, 0x02, 0x0e, 0x10, 0x12, 0x0c, 0x00, 0x00, 0x0c, 0x02, 0x0e,
   0x12, 0x12, 0x0c, 0x00, 0x00, 0x1e, 0x10, 0x08, 0x08, 0x04, 0x04, 0x00,
   0x00, 0x0c, 0x12, 0x0c, 0x12, 0x12, 0x0c, 0x00, 0x00, 0x0c, 0x12, 0x12,
   0x1c, 0x10, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00,
   0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x08, 0x04, 0x00, 0x10, 0x08, 0x04,
   0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x00,
   0x00, 0x04, 0x08, 0x10, 0x10, 0x08, 0x04, 0x00, 0x00, 0x08, 0x14, 0x10,
   0x08, 0x00, 0x08, 0x00, 0x18, 0x24, 0x32, 0x2a, 0x2a, 0x12, 0x04, 0x18,
   0x00, 0x0c, 0x12, 0x12, 0x1e, 0x12, 0x12, 0x00, 0x00, 0x0e, 0x12, 0x0e,
   0x12, 0x12, 0x0e, 0x00, 0x00, 0x0c, 0x12, 0x02, 0x02, 0x12, 0x0c, 0x00,
   0x00, 0x0e, 0x12, 0x12, 0x12, 0x12, 0x0e, 0x00, 0x00, 0x1e, 0x02, 0x0e,
   0x02, 0x02, 0x1e, 0x00, 0x00, 0x1e, 0x02, 0x0e, 0x02, 0x02, 0x02, 0x00,
   0x00, 0x0c, 0x12, 0x02, 0x1a, 0x12, 0x0c, 0x00, 0x00, 0x12, 0x12, 0x1e,
   0x12, 0x12, 0x12, 0x00, 0x00, 0x1c, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00,
   0x00, 0x1c, 0x08, 0x08, 0x08, 0x0a, 0x04, 0x00, 0x00, 0x12, 0x0a, 0x06,
   0x0a, 0x0a, 0x12, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x1e, 0x00,
   0x00, 0x12, 0x1e, 0x1e, 0x12, 0x12, 0x12, 0x00, 0x00, 0x12, 0x16, 0x1e,
   0x1a, 0x1a, 0x12, 0x00, 0x00, 0x0c, 0x12, 0x12, 0x12, 0x12, 0x0c, 0x00,
   0x00, 0x0e, 0x12, 0x12, 0x0e, 0x02, 0x02, 0x00, 0x00, 0x0c, 0x12, 0x12,
   0x16, 0x1a, 0x0c, 0x10, 0x00, 0x0e, 0x12, 0x12, 0x0e, 0x12, 0x12, 0x00,
   0x00, 0x0c, 0x12, 0x04, 0x08, 0x12, 0x0c, 0x00, 0x00, 0x1c, 0x08, 0x08,
   0x08, 0x08, 0x08, 0x00, 0x00, 0x12, 0x12, 0x12, 0x12, 0x12, 0x0c, 0x00,
   0x00, 0x12, 0x12, 0x12, 0x12, 0x0c, 0x0c, 0x00, 0x00, 0x12, 0x12, 0x12,
   0x1e, 0x1e, 0x12, 0x00, 0x00, 0x12, 0x12, 0x0c, 0x0c, 0x12, 0x12, 0x00,
   0x00, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x00, 0x00, 0x1e, 0x10, 0x08,
   0x04, 0x02, 0x1e, 0x00, 0x00, 0x1c, 0x04, 0x04, 0x04, 0x04, 0x1c, 0x00,
   0x00, 0x02, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00, 0x00, 0x1c, 0x10, 0x10,
   0x10, 0x10, 0x1c, 0x00, 0x00, 0x08, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x08, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x12, 0x12, 0x1c, 0x00,
   0x00, 0x02, 0x02, 0x0e, 0x12, 0x12, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x18,
   0x04, 0x04, 0x18, 0x00, 0x00, 0x10, 0x10, 0x1c, 0x12, 0x12, 0x1c, 0x00,
   0x00, 0x00, 0x00, 0x0c, 0x1a, 0x06, 0x0c, 0x00, 0x00, 0x08, 0x14, 0x04,
   0x0e, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x12, 0x1c, 0x10, 0x0c,
   0x00, 0x02, 0x02, 0x0e, 0x12, 0x12, 0x12, 0x00, 0x00, 0x08, 0x00, 0x0c,
   0x08, 0x08, 0x1c, 0x00, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 0x14, 0x08,
   0x00, 0x02, 0x02, 0x12, 0x0e, 0x12, 0x12, 0x00, 0x00, 0x0c, 0x08, 0x08,
   0x08, 0x08, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x16, 0x2a, 0x2a, 0x2a, 0x00,
   0x00, 0x00, 0x00, 0x0e, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x0c,
   0x12, 0x12, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x12, 0x0e, 0x02, 0x02,
   0x00, 0x00, 0x00, 0x1c, 0x12, 0x1c, 0x10, 0x10, 0x00, 0x00, 0x00, 0x0a,
   0x16, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0c, 0x10, 0x0c, 0x00,
   0x00, 0x04, 0x04, 0x0e, 0x04, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00, 0x12,
   0x12, 0x12, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x08, 0x00,
   0x00, 0x00, 0x00, 0x22, 0x2a, 0x2a, 0x14, 0x00, 0x00, 0x00, 0x00, 0x12,
   0x0c, 0x0c, 0x12, 0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x1c, 0x12, 0x0c,
   0x00, 0x00, 0x00, 0x1e, 0x08, 0x04, 0x1e, 0x00, 0x18, 0x04, 0x08, 0x06,
   0x08, 0x04, 0x18, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00,
   0x06, 0x08, 0x04, 0x18, 0x04, 0x08, 0x06, 0x00, 0x00, 0x14, 0x0a, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00 };
#endif

struct sysData_t G_sysData = { "TESTUSER00", 1 /* badge ID */ };
int IRpacketOutNext;
int IRpacketOutCurr;

volatile int timestamp = 0;

static GtkWidget *vbox, *window, *drawing_area;
#define SCALE_FACTOR 6
#define EXTRA_WIDTH 200
#define GTK_SCREEN_WIDTH (LCD_XSIZE * SCALE_FACTOR + EXTRA_WIDTH)
#define GTK_SCREEN_HEIGHT (LCD_YSIZE * SCALE_FACTOR)
static int real_screen_width = GTK_SCREEN_WIDTH;
static int real_screen_height = GTK_SCREEN_HEIGHT;
static GdkGC *gc = NULL;               /* our graphics context. */
static int screen_offset_x = 0;
static int screen_offset_y = 0;
static gint timer_tag;
#define NCOLORS 9 
#define FLARE_LED_COLOR (NCOLORS - 1)
GdkColor huex[NCOLORS];
static int (*badge_function)(void);
static int time_to_quit = 0;

static unsigned char current_color = BLUE;
static unsigned char current_background_color = BLACK;
static int serial_port_fd = -1;
static int g_callback_hz = 30;
static int g_timer = 0;

#define BUTTON 0
#define LEFT 1
#define RIGHT 2
#define UP 3
#define DOWN 4

static int button_pressed[5] = { 0 };

void FbColor(int color)
{
    current_color = (unsigned char) (color % 8);
}

void FbBackgroundColor(int color)
{
	current_background_color = color;
}

void FbInit(void)
{
}

static unsigned char screen_color[LCD_XSIZE][LCD_YSIZE];
static unsigned char live_screen_color[LCD_XSIZE][LCD_YSIZE];

void plot_point(int x, int y, void *context)
{
    unsigned char *screen_color = context;

    screen_color[x * LCD_YSIZE + y] = current_color;
}

void clear_point(int x, int y, void *context)
{
    unsigned char *screen_color = context;

    screen_color[x * LCD_YSIZE + y] = BLACK;
}

#define SWAPBUF_PERF_TICKS 30
static int swapbuf_timestamp[SWAPBUF_PERF_TICKS] = { 0 };
static int swapbuf_index = 0;

static void detect_high_swapbuffers_rate(void)
{
	int i;
	int total_time, start_time, end_time, frames, mspf;
	static int stacktrace_printed = 0;

	swapbuf_timestamp[swapbuf_index] = g_timer;
	swapbuf_index++;
	if (swapbuf_index >= SWAPBUF_PERF_TICKS)
		swapbuf_index = 0;
	start_time = 0;
	end_time = 0;
	frames = 0;
	for (i = 0; i < SWAPBUF_PERF_TICKS; i++) {
		if (swapbuf_timestamp[i] == 0)
			continue;
		frames++;
		if (start_time == 0)
			start_time = swapbuf_timestamp[i];
		if (end_time == 0)
			end_time = swapbuf_timestamp[i];
		if (swapbuf_timestamp[i] < start_time)
			start_time = swapbuf_timestamp[i];
		if (swapbuf_timestamp[i] > end_time)
			end_time = swapbuf_timestamp[i];
	}
	if (frames > 0) {
		total_time = end_time - start_time;
		if (total_time > 0) {
			mspf = (1000 * total_time) / g_callback_hz / frames;
			if (mspf > 0 && mspf < 100) {
				printf("Warning, high SwapBuffers() rate detected (%d milliseconds per frame)\n", mspf);
				printf("Performance on badge likely to be terrible.\n");
				if ((stacktrace_printed % 30) == 0) {
					stacktrace("High SwapBuffers Rate\n");
				}
				stacktrace_printed++;
			}
		}
	}
}


void FbSwapBuffers(void)
{
	memcpy(live_screen_color, screen_color, sizeof(live_screen_color));
	detect_high_swapbuffers_rate();
}

void FbPushBuffer(void)
{
	memcpy(live_screen_color, screen_color, sizeof(live_screen_color));
}

void FbPaintNewRows(void)
{
	memcpy(live_screen_color, screen_color, sizeof(live_screen_color));
}

void FbLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    bline(x1, y1, x2, y2, plot_point, screen_color);
}

void FbPoint(int x, int y)
{
	plot_point(x, y, screen_color);
}

void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, UNUSED unsigned char y2)
{
    unsigned char x;

    for (x = x1; x <= x2; x++)
        plot_point(x, y1, screen_color);
}

void FbVerticalLine(unsigned char x1, unsigned char y1, UNUSED unsigned char x2, unsigned char y2)
{
    unsigned char y;

    for (y = y1; y <= y2; y++)
        plot_point(x1, y, screen_color);
}

static unsigned char write_x = 0;
static unsigned char write_y = 0;

void FbFilledRectangle(unsigned char width, unsigned char height)
{
	int i, x, y;

	x = write_x;
	y = write_y;
	for (i = 0; i < height; i++) {
		FbHorizontalLine(x, y, x + width - 1, write_y);
		y++;
	}
	write_x += width;
	write_y += height;
}

void FbRectangle(unsigned char width, unsigned char height)
{
	int x, y;

	x = write_x;
	y = write_y;
	FbHorizontalLine(x, y, x + width - 1, y);
	FbHorizontalLine(x, y + height - 1, x + width - 1, y + height - 1);
	FbVerticalLine(x, y, x, y + height - 1);
	FbVerticalLine(x + width - 1, y, x + width - 1, y + height - 1);
	write_x += width;
	write_y += height;
}

void FbClear(void)
{
    memset(screen_color, current_background_color, LCD_XSIZE * LCD_YSIZE);
}

unsigned char char_to_index(unsigned char charin){
#if USE_2016_BADGE_FONT
    /*
        massaged from Jon's LCD code.
    */
    if (charin >= 'a' && charin <= 'z')
        charin -= 97;
    else {
        if (charin >= 'A' && charin <= 'Z')
                charin -= 65;
        else {
            if (charin >= '0' && charin <= '9')
                charin -= 22;
            else {
                switch (charin) {
                case '.':
                    charin = 36;
                    break;

                case ':':
                    charin = 37;
                    break;

                case '!':
                    charin = 38;
                    break;

                case '-':
                    charin = 39;
                    break;

                case '_':
                    charin = 40;
                    break;

                default:
                    charin = 41;
                }
            }
        }
    }
#endif
#if USE_2019_BADGE_FONT
    if ((charin < 32) | (charin > 126)) charin = 32;
    charin -= 32;
#endif
    return charin;
}

static void draw_character(unsigned char x, unsigned char y, unsigned char c)
{
	unsigned char index = char_to_index(c);
	unsigned char i, j, bits;
	unsigned char sx, sy;

	sy = y;
	for (i = 0; i < 8; i++) {
#if USE_2016_BADGE_FONT
		bits = font_2_bits[8 * index + i];
#endif
#if USE_2019_BADGE_FONT
		bits = font8x8_bits[8 * index + i];
#endif
		sx = x;
		for (j = 0; j < 8; j++) {
			if ((bits >> j) & 0x01) {
				plot_point(sx, sy, screen_color);
			} else {
				clear_point(sx, sy, screen_color);
			}
			sx++;
		}
		sy++;
	}
}

void FbMove(unsigned char x, unsigned char y)
{
	write_x = x;
	write_y = y;
}

void FbMoveRelative(char x, char y)
{
	write_x += x;
	write_y += y;
}

void FbMoveX(unsigned char x)
{
	if (x >= LCD_XSIZE)
		x = LCD_XSIZE - 1;
	write_x = x;
}

void FbMoveY(unsigned char y)
{
	if (y >= LCD_YSIZE)
		y = LCD_YSIZE - 1;
	write_y = y;
}

void FbWriteLine(char *s)
{
	int i;

	for (i = 0; s[i]; i++) {
		draw_character(write_x, write_y, s[i]);
		write_x += 8;
		if (write_x > LCD_XSIZE - 8) {
			write_x = 0;
			write_y += 8;
			if (write_y > LCD_YSIZE - 8)
				write_y = 0;
		}
	}
}

void FbWriteString(char *s)
{
	int i, ix;

	ix = write_x;
	for (i = 0; s[i]; i++) {
		if (s[i] == '\n') {
			write_x = ix;
			write_y += 8;
			continue;
		}
		draw_character(write_x, write_y, s[i]);
		write_x += 8;
		if (write_x > LCD_XSIZE - 8) {
			write_x = 0;
			write_y += 8;
			if (write_y > LCD_YSIZE - 8)
				write_y = 0;
		}
	}
}

void itoa(char *string, int value, UNUSED int base)
{
	sprintf(string, "%d", value);
}

int abs(int x)
{
    return x > 0 ? x : -x;
}

void returnToMenus(void)
{
	exit(0);
}

static void ir_packet_ignore(UNUSED struct IRpacket_t packet)
{
}

static void (*ir_packet_callback)(struct IRpacket_t) = ir_packet_ignore;


void register_ir_packet_callback(void (*callback)(struct IRpacket_t))
{
	ir_packet_callback = callback;
}

void unregister_ir_packet_callback(void)
{
	ir_packet_callback = ir_packet_ignore;
}

static pthread_cond_t packet_write_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t interrupt_mutex = PTHREAD_MUTEX_INITIALIZER;
#define IR_OUTPUT_QUEUE_SIZE 16
static uint32_t ir_output_queue[IR_OUTPUT_QUEUE_SIZE] = { 0 };
static int ir_output_queue_input = 0;
static int ir_output_queue_output = 0;
static int output_queue_has_new_packets = 0;

static int ir_output_queue_empty(void)
{
	return ir_output_queue_input == ir_output_queue_output;
}

static void ir_output_queue_enqueue(uint32_t v)
{
	if (((IRpacketOutNext+1) % MAXPACKETQUEUE) != IRpacketOutCurr) {
		ir_output_queue[ir_output_queue_input] = v;
		ir_output_queue_input = (ir_output_queue_input + 1) % IR_OUTPUT_QUEUE_SIZE;
		IRpacketOutNext = (IRpacketOutNext + 1) % MAXPACKETQUEUE;
	}
}

static uint32_t ir_output_queue_dequeue(void)
{
	uint32_t v;
	BUILD_ASSERT(MAXPACKETQUEUE == IR_OUTPUT_QUEUE_SIZE);

	if (ir_output_queue_empty())
		return (uint32_t) -1; /* should not happen */
	v = ir_output_queue[ir_output_queue_output];
	ir_output_queue_output = (ir_output_queue_output + 1) % IR_OUTPUT_QUEUE_SIZE;
	IRpacketOutCurr = (IRpacketOutCurr + 1) % MAXPACKETQUEUE;
	return v;
}

#define BADGE_IR_UDP_PORT 12345

static void *write_udp_packets_thread_fn(void *thread_info)
{
	unsigned short *p = thread_info;
	struct in_addr localhost_ip;
	int on;
	int rc, bcast;
	uint32_t netmask;
	struct ifaddrs *ifaddr, *a;
        struct sockaddr_in bcast_addr;
	int found_netmask = 0;
	unsigned short port_to_xmit_on = *p;

	free(p);

	if (serial_port_fd >= 0)
		goto skip_socket_stuff;

	/* Get localhost IP addr in byte form */
	rc = inet_aton("127.0.0.1", &localhost_ip);
	if (rc == 0) {
		fprintf(stderr, "inet_aton: invalid address 127.0.0.1");
		return NULL;
	}

	/* Make a UDP datagram socket */
	bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (bcast < 0) {
		fprintf(stderr, "socket: Failed to create UDP datagram socket: %s\n", strerror(errno));
		return NULL;
	}

	/* Set our socket up for broadcasting */
	on = 1;
	rc = setsockopt(bcast, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (const char *) &on, sizeof(on));
	if (rc < 0) {
		fprintf(stderr, "setsockopt(SO_REUSEADDR | SO_BROADCAST): %s\n", strerror(errno));
		close(bcast);
		return NULL;
	}

	/* Get the netmask for our localhost interface */
	rc = getifaddrs(&ifaddr);
	if (rc < 0) {
		fprintf(stderr, "getifaddrs() failed: %s\n", strerror(errno));
		close(bcast);
		return NULL;
	}
	found_netmask = 0;
	for (a = ifaddr; a; a = a->ifa_next) {
		struct in_addr s;
		if (a->ifa_addr == NULL)
			continue;
		if (a->ifa_addr->sa_family != AF_INET)
			continue;
		s = ((struct sockaddr_in *) a->ifa_addr)->sin_addr;
		bcast_addr = *(struct sockaddr_in *) a->ifa_addr;
		fprintf(stderr, "comparing ipaddr %08x to %08x\n", s.s_addr, localhost_ip.s_addr);
		if (s.s_addr == localhost_ip.s_addr) {
			fprintf(stderr, "Matched IP address %08x\n", (uint32_t) localhost_ip.s_addr);
			netmask = ((struct sockaddr_in *) a->ifa_netmask)->sin_addr.s_addr;
			found_netmask = 1;
			break;
		}
	}
	if (ifaddr)
		freeifaddrs(ifaddr);
	if (!found_netmask) {
		fprintf(stderr, "failed to get netmask.\n");
		close(bcast);
		return NULL;
	}

	/* Compute the broadcast address and set the port */
	bcast_addr.sin_addr.s_addr = ~netmask | localhost_ip.s_addr;
	bcast_addr.sin_port = htons(port_to_xmit_on);

skip_socket_stuff:
	/* Start sending packets */
	pthread_mutex_lock(&interrupt_mutex);
	do {
		uint32_t v;
		/* Wait for a packet to write to appear */
		rc = pthread_cond_wait(&packet_write_cond, &interrupt_mutex);
		if (rc != 0)
			fprintf(stderr, "pthread_cond_wait failed\n");
		if (!output_queue_has_new_packets)
			continue;
		do {
			v = ir_output_queue_dequeue();
			if (serial_port_fd < 0) { /* no serial port, send via udp */
				rc = sendto(bcast, &v, sizeof(v), 0, (struct sockaddr *) &bcast_addr, sizeof(bcast_addr));
				if (rc < 0)
					fprintf(stderr, "sendto failed: %s\n", strerror(errno));
			} else { /* send via serial port */
				int bytes_left, bytes_written;
				unsigned char *b = (unsigned char *) &v;

				bytes_left = sizeof(v);
				do {
					bytes_written = write(serial_port_fd, &b[4 - bytes_left], bytes_left);
					if (bytes_written < 0) {
						if (errno == EINTR)
							continue;
						fprintf(stderr, "write: %s\n", strerror(errno));
						goto out;
					}
					printf("Wrote %d bytes of %08x to serial port\n", bytes_written, v);
					bytes_left -= bytes_written;
				} while (bytes_left > 0);
			}
		} while (!ir_output_queue_empty());
	} while (1);
out:
	pthread_mutex_unlock(&interrupt_mutex);
	return NULL;
}

static void *read_udp_packets_thread_fn(void *thread_info)
{
	unsigned short *p = thread_info;
	int rc, bcast = -1;
	struct sockaddr_in bcast_addr;
	struct sockaddr remote_addr;
	socklen_t remote_addr_len;
	unsigned short port_to_recv_from = *p;

	free(p);

	if (serial_port_fd >= 0)
		goto skip_socket_stuff;

        /* Make ourselves a UDP datagram socket */
        bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (bcast < 0) {
                fprintf(stderr, "broadcast_lobby_info: socket() failed: %s\n", strerror(errno));
                return NULL;
        }

	/* Bind to any address on our port */
        bcast_addr.sin_family = AF_INET;
        bcast_addr.sin_addr.s_addr = INADDR_ANY;
        bcast_addr.sin_port = htons(port_to_recv_from);
        rc = bind(bcast, (struct sockaddr *) &bcast_addr, sizeof(bcast_addr));
        if (rc < 0) {
                fprintf(stderr, "bind() bcast_addr failed: %s\n", strerror(errno));
                return NULL;
        }

skip_socket_stuff:
	/* Start receiving packets */
        do {
		uint32_t v;
		struct IRpacket_t p;
		v = 0;
		if (serial_port_fd < 0) {
			remote_addr_len = sizeof(remote_addr);
			rc = recvfrom(bcast, &v, sizeof(v), 0, &remote_addr, &remote_addr_len);
		} else {
			int bytes_read, bytes_left;
			unsigned char *b = (unsigned char *) &v;

			bytes_left = sizeof(v);
			do {
				bytes_read = read(serial_port_fd, &b[4 - bytes_left], bytes_left);
				if (bytes_read < 0) {
					if (errno == EINTR)
						continue;
					fprintf(stderr, "read: %s\n", strerror(errno));
					enable_interrupts();
					goto out;
				}
				bytes_left -= bytes_read;
			} while (bytes_left > 0);
			rc = 0;
		}
		if (rc < 0) {
			fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
			continue;
		}
		/* fprintf(stderr, "Received broadcast lobby info: addr = %08x, port = %04x\n",
				ntohl(payload.ipaddr), ntohs(payload.port)); */
		p.v = v;
		disable_interrupts();
		ir_packet_callback(p);
		enable_interrupts();
	} while(1);
out:
	return NULL;
}

void setup_ir_sensor(unsigned short port_to_recv_from)
{
	pthread_t thr;
	int rc;

	unsigned short *p = malloc(sizeof(*p));
	*p = port_to_recv_from;
	rc = pthread_create(&thr, NULL, read_udp_packets_thread_fn, p);
	if (rc < 0)
		fprintf(stderr, "Failed to create thread to read udp packets: %s\n", strerror(errno));
}

void setup_ir_transmitter(unsigned short port_to_transmit_from)
{
	pthread_t thr;
	int rc;

	unsigned short *p = malloc(sizeof(*p));
	*p = port_to_transmit_from;
	rc = pthread_create(&thr, NULL, write_udp_packets_thread_fn, p);
	if (rc < 0)
		fprintf(stderr, "Failed to create thread to write udp packets: %s\n", strerror(errno));
}

/* Opens a serial port and configures for 9600 baud, 8N1, global serial_port_fd is set */
static void setup_linux_ir_serial_port(char *serial_port)
{
	int fd;
	struct termios options;

	fd = open(serial_port, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		fprintf(stderr, "open: %s: %s\n", serial_port, strerror(errno));
		serial_port_fd = -1;
		return;
	}
	tcgetattr(fd, &options);
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	options.c_cflag |= (CLOCAL | CREAD);
	/* 8N1 */
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	/* Raw mode */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	tcsetattr(fd, TCSANOW, &options);

	serial_port_fd = fd;

	write(serial_port_fd, "ZsYnCxX#", 6);	/* write sync sequence. This is to enable the badge to successfully skip */
						/* a few indeterminate garbage bytes that show up on the USB serial when */
						/* first connected. Must be a multiple of 4 bytes long. */

	fprintf(stderr, "irxmit: Using serial port to simulate IR\n");
	return;
}

void setup_linux_ir_simulator(char *serial_port, unsigned short port_to_recv_from, unsigned short port_to_xmit_on)
{
	if (serial_port)
		setup_linux_ir_serial_port(serial_port);
	setup_ir_sensor(port_to_recv_from);
	setup_ir_transmitter(port_to_xmit_on);
}

void disable_interrupts(void)
{
	pthread_mutex_lock(&interrupt_mutex);
}

void enable_interrupts(void)
{
	pthread_mutex_unlock(&interrupt_mutex);
}

void IRqueueSend(union IRpacket_u packet)
{
	int rc;

	printf("Send packet : 0x%08x\n", packet.v);
	pthread_mutex_lock(&interrupt_mutex);
	ir_output_queue_enqueue(packet.v);
	output_queue_has_new_packets = 1;
	rc = pthread_cond_broadcast(&packet_write_cond);
	if (rc)
		fprintf(stderr, "pthread_cond_broadcast failed: %s\n", strerror(errno));
	pthread_mutex_unlock(&interrupt_mutex);
}

static void setup_window_geometry(GtkWidget *window)
{
	/* clamp window aspect ratio to constant */
	GdkGeometry geom;
	geom.min_aspect = (gdouble) GTK_SCREEN_WIDTH / (gdouble) GTK_SCREEN_HEIGHT;
	geom.max_aspect = geom.min_aspect;
	gtk_window_set_geometry_hints(GTK_WINDOW(window), NULL, &geom, GDK_HINT_ASPECT);
}

static gboolean delete_event(UNUSED GtkWidget *widget,
        UNUSED GdkEvent *event, UNUSED gpointer data)
{
	/* If you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */
	return FALSE;
}

static void destroy(UNUSED GtkWidget *widget, UNUSED gpointer data)
{
	gtk_main_quit();
}

static gint key_press_cb(UNUSED GtkWidget* widget, GdkEventKey* event, UNUSED gpointer data)
{
	switch (event->keyval) {

	case GDK_w:
	case GDK_KEY_Up:
		button_pressed[UP] = 1;
		break;
	case GDK_s:
	case GDK_KEY_Down:
		button_pressed[DOWN] = 1;
		break;
	case GDK_a:
	case GDK_KEY_Left:
		button_pressed[LEFT] = 1;
		break;
	case GDK_d:
	case GDK_KEY_Right:
		button_pressed[RIGHT] = 1;
		break;
	case GDK_space:
	case GDK_KEY_Return:
		button_pressed[BUTTON] = 1;
		break;
	case GDK_q:
	case GDK_KEY_Escape:
		time_to_quit = 1;
		break;
	}
	return TRUE;
}

static void draw_led_text(GtkWidget *widget, int x, int y)
{
#define LETTER_SPACING 12
	/* Literally draws L E D */
	/* Draw L */
	gdk_draw_line(widget->window, gc, x, y, x, y - 10);
	gdk_draw_line(widget->window, gc, x, y, x + 8, y);

	x += LETTER_SPACING;

	/* Draw E */
	gdk_draw_line(widget->window, gc, x, y, x, y - 10);
	gdk_draw_line(widget->window, gc, x, y, x + 8, y);
	gdk_draw_line(widget->window, gc, x, y - 5, x + 5, y - 5);
	gdk_draw_line(widget->window, gc, x, y - 10, x + 8, y - 10);

	x += LETTER_SPACING;

	/* Draw D */
	gdk_draw_line(widget->window, gc, x, y, x, y - 10);
	gdk_draw_line(widget->window, gc, x, y, x + 8, y);
	gdk_draw_line(widget->window, gc, x, y - 10, x + 8, y - 10);
	gdk_draw_line(widget->window, gc, x + 8, y - 10, x + 10, y - 5);
	gdk_draw_line(widget->window, gc, x + 8, y, x + 10, y - 5);
}

static int drawing_area_expose(GtkWidget *widget, UNUSED GdkEvent *event, UNUSED gpointer p)
{
	/* Draw the screen */
	static int timer = 0;
	int x, y, w, h;

	timer++;
	g_timer++;
	w = (real_screen_width - EXTRA_WIDTH) / LCD_XSIZE;
	if (w < 1)
		w = 1;
	h = real_screen_height / LCD_YSIZE;
	if (h < 1)
		h = 1;

	for (y = 0; y < LCD_YSIZE; y++) {
		for (x = 0; x < LCD_XSIZE; x++) {
			unsigned char c = live_screen_color[x][y] % NCOLORS;
			gdk_gc_set_foreground(gc, &huex[c]);
			gdk_draw_rectangle(widget->window, gc, 1 /* filled */, x * w, y * h, w, h);
		}
	}

	/* Draw a vertical line demarcating the right edge of the screen */
	gdk_gc_set_foreground(gc, &huex[WHITE]);
	gdk_draw_line(widget->window, gc, LCD_XSIZE * w + 1, 0, LCD_XSIZE * w + 1, real_screen_height - 1);

	/* Draw simulated flare LED */
	x = LCD_XSIZE * w + EXTRA_WIDTH / 4;
	y = (LCD_YSIZE * h) / 2 - EXTRA_WIDTH / 4;
	draw_led_text(widget, LCD_XSIZE * w + EXTRA_WIDTH / 2 - 20, y - 10);
	gdk_gc_set_foreground(gc, &huex[FLARE_LED_COLOR]);
	gdk_draw_rectangle(widget->window, gc, 1 /* filled */, x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2);
	gdk_gc_set_foreground(gc, &huex[WHITE]);
	gdk_draw_rectangle(widget->window, gc, 0 /* not filled */, x, y, EXTRA_WIDTH / 2, EXTRA_WIDTH / 2);

	return 0;
}

static gint drawing_area_configure(GtkWidget *w, UNUSED GdkEventConfigure *event)
{
        GdkRectangle cliprect;

        /* first time through, gc is null, because gc can't be set without */
        /* a window, but, the window isn't there yet until it's shown, but */
        /* the show generates a configure... chicken and egg.  And we can't */
        /* proceed without gc != NULL...  but, it's ok, because 1st time thru */
        /* we already sort of know the drawing area/window size. */

        if (gc == NULL)
                return TRUE;

        real_screen_width =  w->allocation.width;
        real_screen_height =  w->allocation.height;

        gdk_gc_set_clip_origin(gc, 0, 0);
        cliprect.x = 0;
        cliprect.y = 0;
        cliprect.width = real_screen_width;
        cliprect.height = real_screen_height;
        gdk_gc_set_clip_rectangle(gc, &cliprect);
	return TRUE;
}

void flareled(unsigned char r, unsigned char g, unsigned char b)
{
	gdk_colormap_free_colors(gtk_widget_get_colormap(drawing_area), &huex[FLARE_LED_COLOR], 1);
	huex[FLARE_LED_COLOR].red = (unsigned short) r * 256;
	huex[FLARE_LED_COLOR].green = (unsigned short) g * 256;
	huex[FLARE_LED_COLOR].blue = (unsigned short) b * 256;
	gdk_colormap_alloc_color(gtk_widget_get_colormap(drawing_area), &huex[FLARE_LED_COLOR], FALSE, FALSE);
}

static void setup_gtk_colors(void)
{
	gdk_color_parse("white", &huex[WHITE]);
	gdk_color_parse("blue", &huex[BLUE]);
	gdk_color_parse("black", &huex[BLACK]);
	gdk_color_parse("green", &huex[GREEN]);
	gdk_color_parse("yellow", &huex[YELLOW]);
	gdk_color_parse("red", &huex[RED]);
	gdk_color_parse("cyan", &huex[CYAN]);
	gdk_color_parse("MAGENTA", &huex[MAGENTA]);
	gdk_color_parse("blue", &huex[FLARE_LED_COLOR]);
}

static void setup_gtk_window_and_drawing_area(GtkWidget **window, GtkWidget **vbox, GtkWidget **drawing_area)
{
	GdkRectangle cliprect;
	int i;
	char window_title[1024];

	*window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	setup_window_geometry(*window);
	gtk_container_set_border_width(GTK_CONTAINER(*window), 0);
	*vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_move(GTK_WINDOW(*window), screen_offset_x, screen_offset_y);
	*drawing_area = gtk_drawing_area_new();
        g_signal_connect(G_OBJECT(*window), "delete_event",
                G_CALLBACK(delete_event), NULL);
        g_signal_connect(G_OBJECT(*window), "destroy",
                G_CALLBACK(destroy), NULL);
        g_signal_connect(G_OBJECT(*window), "key_press_event",
                G_CALLBACK(key_press_cb), "window");
        g_signal_connect(G_OBJECT(*drawing_area), "expose_event",
                G_CALLBACK(drawing_area_expose), NULL);
        g_signal_connect(G_OBJECT(*drawing_area), "configure_event",
                G_CALLBACK(drawing_area_configure), NULL);
        gtk_container_add(GTK_CONTAINER(*window), *vbox);
        gtk_box_pack_start(GTK_BOX(*vbox), *drawing_area, TRUE /* expand */, TRUE /* fill */, 0);
        gtk_window_set_default_size(GTK_WINDOW(*window), real_screen_width, real_screen_height);
	snprintf(window_title, sizeof(window_title), "HackRVA Badge Emulator - %s", program_title);
	free(program_title);
	gtk_window_set_title(GTK_WINDOW(*window), window_title);

	for (i = 0; i < NCOLORS; i++)
		gdk_colormap_alloc_color(gtk_widget_get_colormap(*drawing_area), &huex[i], FALSE, FALSE);

        gtk_widget_modify_bg(*drawing_area, GTK_STATE_NORMAL, &huex[BLACK]);
        gtk_widget_show(*vbox);
        gtk_widget_show(*drawing_area);
	gtk_widget_show(*window);
	gc = gdk_gc_new(GTK_WIDGET(*drawing_area)->window);
	gdk_gc_set_foreground(gc, &huex[WHITE]);

	gdk_gc_set_clip_origin(gc, 0, 0);
	cliprect.x = 0;
	cliprect.y = 0;
	cliprect.width = real_screen_width;
	cliprect.height = real_screen_height;
	gdk_gc_set_clip_rectangle(gc, &cliprect);
}

static gint advance_game(__attribute__((unused)) gpointer data)
{
	if (time_to_quit)
		exit(0);
	badge_function();
	gdk_threads_enter();
	gtk_widget_queue_draw(drawing_area);
	gdk_threads_leave();
	timestamp++;
	return TRUE;
}

void start_gtk(int *argc, char ***argv, int (*main_badge_function)(void), int callback_hz)
{
	program_title = strdup((*argv)[0]);
	gtk_set_locale();
	gtk_init(argc, argv);
	setup_gtk_colors();
	setup_gtk_window_and_drawing_area(&window, &vbox, &drawing_area);
	badge_function = main_badge_function;
	flareled(0, 0, 0);
	g_callback_hz = callback_hz;
	timer_tag = g_timeout_add(1000 / callback_hz, advance_game, NULL);

#if 0
	/* Apparently (some versions of?) portaudio calls g_thread_init(). */
	/* It may only be called once, and subsequent calls abort, so */
	/* only call it if the thread system is not already initialized. */
	if (!g_thread_supported ())
		g_thread_init(NULL);
#endif
	gdk_threads_init();
	gtk_main();
}

static int generic_button_pressed(int which_button)
{
	if (button_pressed[which_button]) {
		button_pressed[which_button] = 0;
		return 1;
	}
	return 0;
}

int button_pressed_and_consume()
{
	return generic_button_pressed(BUTTON);
}

int up_btn_and_consume()
{
	return generic_button_pressed(UP);
}

int down_btn_and_consume()
{
	return generic_button_pressed(DOWN);
}

int left_btn_and_consume()
{
	return generic_button_pressed(LEFT);
}

int right_btn_and_consume()
{
	return generic_button_pressed(RIGHT);
}

void setNote(__attribute__((unused)) int note, __attribute__((unused)) int duration)
{
}

char username[10] = "TESTUSER\0\0";

int flashWriteKeyValue(UNUSED unsigned int valuekey, UNUSED char *value, UNUSED unsigned int valuelen)
{
	return 0;
}

/* FbDrawObject() draws an object at x, y.  The coordinates of drawing[] should be centered at
 * (0, 0).  The coordinates in drawing[] are multiplied by scale, then divided by 1024 (via a shift)
 * so for 1:1 size, use scale of 1024.  Smaller values will scale the object down. This is different
 * than FbPolygonFromPoints() or FbDrawVectors() in that drawing[] contains signed chars, and the
 * polygons can be constructed via this program: https://github.com/smcameron/vectordraw
 * as well as allowing scaling.
 */
void FbDrawObject(const struct point drawing[], int npoints, int color, int x, int y, int scale)
{
	int i;
	int xcenter = x;
	int ycenter = y;

	FbColor(color);
	for (i = 0; i < npoints - 1;) {
		if (drawing[i].x == -128) {
			i++;
			continue;
		}
		if (drawing[i + 1].x == -128) {
			i+=2;
			continue;
		}
		FbLine(xcenter + ((drawing[i].x * scale) >> 10), ycenter + ((drawing[i].y * scale) >> 10),
			xcenter + ((drawing[i + 1].x * scale) >> 10), ycenter + ((drawing[i + 1].y * scale) >> 10));
		i++;
	}
}

/* 128 sine values * 256 */
static int sine_array[] = {
	0, 12, 25, 37, 49, 62, 74, 86, 97, 109, 120, 131, 142, 152, 162, 171, 181, 189, 197, 205, 212,
	219, 225, 231, 236, 241, 244, 248, 251, 253, 254, 255, 256, 255, 254, 253, 251, 248, 244, 241,
	236, 231, 225, 219, 212, 205, 197, 189, 181, 171, 162, 152, 142, 131, 120, 109, 97, 86, 74, 62,
	49, 37, 25, 12, 0, -12, -25, -37, -49, -62, -74, -86, -97, -109, -120, -131, -142, -152, -162,
	-171, -181, -189, -197, -205, -212, -219, -225, -231, -236, -241, -244, -248, -251, -253, -254,
	-255, -256, -255, -254, -253, -251, -248, -244, -241, -236, -231, -225, -219, -212, -205, -197,
	-189, -181, -171, -162, -152, -142, -131, -120, -109, -97, -86, -74, -62, -49, -37, -25, -12,
};

short sine(int a)
{
	return sine_array[a];
}

short cosine(int a)
{
	return sine_array[(a + 32) & 127];
}
