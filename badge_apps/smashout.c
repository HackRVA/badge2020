/*********************************************

 Smashout game (like breakout) for HackRVA 2020 badge

 Author: Stephen M. Cameron <stephenmcameron@gmail.com>
 (c) 2019 Stephen M. Cameron

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
#include "xorshift.h"
#include "achievements.h"

/* TODO figure out where these should really come from */
#define SCREEN_XDIM 132
#define SCREEN_YDIM 132
#define PADDLE_HEIGHT 25
#define PADDLE_WIDTH 20
#define PADDLE_SPEED 7
#define BALL_STARTX (8 * SCREEN_XDIM / 2)
#define BALL_STARTY (8 * SCREEN_YDIM / 2)
#define BALL_START_VX (1 * 8)
#define BALL_START_VY (2 * 8)
#define BRICK_WIDTH 8
#define BRICK_HEIGHT 4
#define SPACE_ABOVE_BRICKS 20

static struct smashout_paddle {
	int x, y, w, vx;
} paddle, old_paddle;

static struct smashout_ball {
	int x, y, vx, vy;
} ball, oldball;

static struct smashout_brick {
	int x, y, alive; /* upper left corner of brick */
} brick[4 * 16];

static int score = 0;
static int balls = 0;
static int score_inc = 0;
static int balls_inc = 0;

/* Program states.  Initial state is SMASHOUT_GAME_INIT */
enum smashout_program_state_t {
	SMASHOUT_GAME_INIT,
	SMASHOUT_GAME_PLAY,
	SMASHOUT_GAME_,
	SMASHOUT_GAME_EXIT,
};

static int brick_color[4] = { YELLOW, GREEN, RED, BLUE };

static enum smashout_program_state_t smashout_program_state = SMASHOUT_GAME_INIT;

static void init_bricks(void)
{
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 16; j++) {
			brick[i * 16 + j].x = j * BRICK_WIDTH;
			brick[i * 16 + j].y = SPACE_ABOVE_BRICKS + i * BRICK_HEIGHT;
			brick[i * 16 + j].alive = 1;
		}
	}
}

static void smashout_game_init(void)
{
	FbInit();
	FbClear();
	init_bricks();
	paddle.x = SCREEN_XDIM / 2;
	paddle.y = SCREEN_YDIM - PADDLE_HEIGHT;
	paddle.w = PADDLE_WIDTH;
	paddle.vx = 0;
	old_paddle = paddle;
	ball.x = BALL_STARTX;
	ball.y = BALL_STARTY;
	ball.vx = BALL_START_VX;
	ball.vy = BALL_START_VY;
	oldball = ball;
	smashout_program_state = SMASHOUT_GAME_PLAY;
}

static void smashout_check_buttons()
{
	if (BUTTON_PRESSED_AND_CONSUME) {
		smashout_program_state = SMASHOUT_GAME_EXIT;
	} else if (LEFT_BTN_AND_CONSUME) {
		paddle.vx = -PADDLE_SPEED;
	} else if (RIGHT_BTN_AND_CONSUME) {
		paddle.vx = PADDLE_SPEED;
	}
}

static void smashout_draw_paddle()
{
	int dx = old_paddle.w / 2;
	FbColor(BLACK);
	FbHorizontalLine(old_paddle.x - dx, old_paddle.y, old_paddle.x + dx, old_paddle.y);
	FbColor(WHITE);
	FbHorizontalLine(paddle.x - dx, paddle.y, paddle.x + dx, paddle.y);
}

static void smashout_draw_brick(int row, int col)
{
	struct smashout_brick *b = &brick[row * 16 + col];
	if (b->alive)
		FbColor(brick_color[row]);
	else
		FbColor(BLACK);
	FbHorizontalLine(b->x, b->y, b->x + BRICK_WIDTH - 1, b->y);
	FbHorizontalLine(b->x, b->y + BRICK_HEIGHT - 1, b->x + BRICK_WIDTH - 1, b->y + BRICK_HEIGHT - 1);
	FbVerticalLine(b->x, b->y + 1, b->x, b->y + BRICK_HEIGHT - 1);
	FbVerticalLine(b->x + BRICK_WIDTH - 1, b->y + 1, b->x + BRICK_WIDTH - 1, b->y + BRICK_HEIGHT - 1);
}

static void smashout_draw_bricks()
{
	int i, j;
	int count = 0;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 16; j++) {
			smashout_draw_brick(i, j);
			if (brick[i * 16 + j].alive)
				count++;
		}

	if (count == 0) /* Start over when all the bricks are dead. */
		smashout_program_state = SMASHOUT_GAME_INIT;
}

static void smashout_draw_ball()
{
	int x, y;

	x = oldball.x / 8;
	y = oldball.y / 8;

	FbColor(BLACK);
	FbHorizontalLine(x - 1, y - 1, x + 1, y - 1);
	FbHorizontalLine(x - 1, y + 1, x + 1, y + 1);
	FbVerticalLine(x - 1, y - 1, x - 1, y + 1);
	FbVerticalLine(x + 1, y - 1, x + 1, y + 1);

	x = ball.x / 8;
	y = ball.y / 8;
	FbColor(WHITE);
	FbHorizontalLine(x - 1, y - 1, x + 1, y - 1);
	FbHorizontalLine(x - 1, y + 1, x + 1, y + 1);
	FbVerticalLine(x - 1, y - 1, x - 1, y + 1);
	FbVerticalLine(x + 1, y - 1, x + 1, y + 1);
}

static void smashout_move_paddle()
{
	old_paddle = paddle;
	paddle.x += paddle.vx;
	if (paddle.vx > 0)
		paddle.vx -= 1;
	if (paddle.vx < 0)
		paddle.vx += 1;
	if (paddle.x < PADDLE_WIDTH / 2) {
		paddle.x = PADDLE_WIDTH / 2;
		paddle.vx = 0;
	}
	if (paddle.x > SCREEN_XDIM - PADDLE_WIDTH / 2) {
		paddle.x = SCREEN_XDIM - PADDLE_WIDTH / 2;
		paddle.vx = 0;
	}
}

static void smashout_move_ball()
{
	oldball = ball;
	ball.x = ball.x + ball.vx;
	ball.y = ball.y + ball.vy;
	if (ball.x > 8 * SCREEN_XDIM - 2 * 8) {
		ball.x = 8 * SCREEN_XDIM - 2 * 8;
		if (ball.vx > 0)
			ball.vx = -ball.vx;
	}
	if (ball.y > 8 * SCREEN_YDIM - 2 * 8) {
		ball.x = BALL_STARTX;
		ball.y = BALL_STARTY;
		ball.vx = BALL_START_VX;
		ball.vy = BALL_START_VY;
		balls_inc++;
	}
	if (ball.x < 8) {
		ball.x = 8;
		if (ball.vx < 0)
			ball.vx = -ball.vx;
	}
	if (ball.y < 8) {
		ball.y = 8;
		if (ball.vy < 0)
			ball.vy = -ball.vy;
	}
	if (ball.y >= (paddle.y - 2) * 8 && ball.y <= (paddle.y * 8)) {
		if (ball.x > (paddle.x - PADDLE_WIDTH / 2 - 1) * 8 &&
			ball.x < (paddle.x + PADDLE_WIDTH / 2 + 1) * 8) {
			/* Ball has hit paddle, bounce. */
			ball.vy = - ball.vy;
			/* Impart some paddle energy to ball. */
			ball.vx += paddle.vx * 4;
			if (ball.vx > 80)
				ball.vx = 80;
			if (ball.vx < -80)
				ball.vx = -80;
		}
	}
	if (ball.y > 8 * SPACE_ABOVE_BRICKS && ball.y < 8 * (SPACE_ABOVE_BRICKS + 4 * BRICK_HEIGHT)) {
		/* Figure out which brick we are intersecting */
		int col = ball.x / (BRICK_WIDTH * 8);
		int row = (ball.y - 8 * SPACE_ABOVE_BRICKS) / (8 * BRICK_HEIGHT);
		if (col >= 0 && col <= 15 && row >= 0 && row <= 3) {
			struct smashout_brick *b = &brick[row * 16 + col];
			if (b->alive) {
				b->alive = 0;
				score_inc++;
				ball.vy = -ball.vy;
			}
		}
	}
}

static void draw_score_and_balls(int color)
{
	char s[20], b[20];
	FbColor(color);
	itoa(s, score, 10);
	itoa(b, balls, 10);
	FbMove(10, SCREEN_YDIM - 10);
	FbWriteLine(s);
	FbMove(70, SCREEN_YDIM - 10);
	FbWriteLine(b);
}

static void smashout_draw_screen()
{
	smashout_draw_paddle();
	smashout_draw_ball();
	smashout_draw_bricks();
	draw_score_and_balls(BLACK);
	score += score_inc;
	balls += balls_inc;
	score_inc = 0;
	balls_inc = 0;
	draw_score_and_balls(WHITE);
	FbPaintNewRows();
}

static void smashout_game_play()
{
	smashout_check_buttons();
	smashout_move_paddle();
	smashout_move_ball();
	smashout_draw_screen();
}

static void smashout_game_exit()
{
	smashout_program_state = SMASHOUT_GAME_INIT;
	returnToMenus();
}

int smashout_cb(void)
{
	switch (smashout_program_state) {
	case SMASHOUT_GAME_INIT:
		smashout_game_init();
		break;
	case SMASHOUT_GAME_PLAY:
		smashout_game_play();
		break;
	case SMASHOUT_GAME_EXIT:
		smashout_game_exit();
		break;
	default:
		break;
	}
	return 0;
}

#ifdef __linux__
int main(int argc, char *argv[])
{
        start_gtk(&argc, &argv, smashout_cb, 30);
        return 0;
}
#endif
