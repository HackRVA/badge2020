#include <stdlib.h>
//#include "badge16.h"
#include <plib.h>
#include "fb.h"
#include "S6B33.h"
#include "assetList.h"
#include "colors.h"
#include "build_bug_on.h"

#define uCHAR (unsigned char *)
struct framebuffer_t G_Fb;

/*
    memory map frame buffer for 16 bit S6B33 type LCD's

    Author: Paul Bruggeman
    paul@killercats.com
    1/2016

*/

/* the output buffer */
unsigned short LCDbuffer[FBSIZE] ;
unsigned char min_changed_x[LCD_YSIZE];
unsigned char max_changed_x[LCD_YSIZE];
#define IS_ROW_CHANGED(i) (min_changed_x[(i)] != 255)

void fb_mark_row_changed(int x, int y)
{
	if (x < min_changed_x[y])
		min_changed_x[y] = x;
	if (x > max_changed_x[y])
		max_changed_x[y] = x;
}

#define MARK_ROW_UNCHANGED(i) do { max_changed_x[(i)] = 0; min_changed_x[(i)] = 255; } while (0)

#define BUFFER( ADDR ) LCDbuffer[(ADDR)]

void FbInit() {
    G_Fb.buffer = LCDbuffer;
    G_Fb.pos.x = 0;
    G_Fb.pos.y = 0;
    G_Fb.font = FONT;
    G_Fb.fontHeight = 8;

    G_Fb.color = 255;
    G_Fb.BGcolor = 0;
    G_Fb.transMask = 0;
    G_Fb.transIndex = 0;
    G_Fb.changed = 0;
}

void FbMoveX(unsigned char x)
{
    G_Fb.pos.x = x;

    if (G_Fb.pos.x >= LCD_XSIZE) G_Fb.pos.x = LCD_XSIZE-1;
}

void FbMoveY(unsigned char y)
{
    G_Fb.pos.y = y;

    if (G_Fb.pos.y >= LCD_YSIZE) G_Fb.pos.y = LCD_YSIZE-1;
}

void FbMoveRelative(char x, char y)
{
    FbMove(G_Fb.pos.x + x, G_Fb.pos.y + y);
}

void FbMove(unsigned char x, unsigned char y)
{
    G_Fb.pos.x = x;
    G_Fb.pos.y = y;

    if (G_Fb.pos.x > LCD_XSIZE) G_Fb.pos.x = LCD_XSIZE-1;
    if (G_Fb.pos.y > LCD_YSIZE) G_Fb.pos.y = LCD_YSIZE-1;
}

void FbClear()
{
    unsigned short i;

    S6B33_rect(0, 0, LCD_XSIZE-1, LCD_YSIZE-1);

//    G_Fb.pos.x = 0;
//    G_Fb.pos.y = 0;
    G_Fb.changed = 0;

    for (i=0; i<(LCD_XSIZE * LCD_YSIZE); i++) {
	BUFFER(i) = G_Fb.BGcolor;
	S6B33_pixel(G_Fb.BGcolor);
    }
    memset(max_changed_x, 0, sizeof(max_changed_x));
    memset(min_changed_x, 255, sizeof(min_changed_x));
}

void FbTransparency(unsigned short transparencyMask)
{
    G_Fb.transMask = transparencyMask;
}

void FbColor(unsigned short color)
{
    G_Fb.color = color;
}

void FbBackgroundColor(unsigned short color)
{
    G_Fb.BGcolor = color;
}

void FbImage(unsigned char assetId, unsigned char seqNum)
{
    switch (assetList[assetId].type) {
	case PICTURE1BIT:
	    FbImage1bit(assetId, seqNum);
	    break;
	
	case PICTURE2BIT:
	    FbImage2bit(assetId, seqNum);
	    break;
	
	case PICTURE4BIT:
	    FbImage4bit(assetId, seqNum);
	    break;
	
	case PICTURE8BIT:
	    FbImage8bit(assetId, seqNum);
	    break;

	default: 
	    break;
    }
}

void FbImage8bit(unsigned char assetId, unsigned char seqNum)
{
    unsigned char y, yEnd, x;
    unsigned char *pixdata, pixbyte, ci, *cmap, r, g, b;
    unsigned short pixel;

    /* clip to end of LCD buffer */
    yEnd = G_Fb.pos.y + assetList[assetId].y;
    if (yEnd > LCD_YSIZE) yEnd = LCD_YSIZE-1;

    for (y = G_Fb.pos.y; y < yEnd; y++) {
	pixdata = uCHAR(&(assetList[assetId].pixdata[ (y - G_Fb.pos.y) * assetList[assetId].x + seqNum * assetList[assetId].x * assetList[assetId].y]));

	for (x = 0; x < assetList[assetId].x; x++) {
		if ((x + G_Fb.pos.x) >= LCD_XSIZE) continue; /* clip x */

		pixbyte = *pixdata; /* 1 pixel per byte */

		ci = pixbyte;
		if (ci != G_Fb.transIndex) { /* transparent? */
		    cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		    r = cmap[0];
		    g = cmap[1];
		    b = cmap[2];

		    pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		    fb_mark_row_changed(x + G_Fb.pos.x, y);
		    if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		    else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
		}
		pixdata++;
	}
    }
    G_Fb.changed = 1;
}

void FbImage4bit(unsigned char assetId, unsigned char seqNum)
{
    unsigned char y, yEnd, x;
    unsigned char *pixdata, pixbyte, ci, *cmap, r, g, b;
    unsigned short pixel;

    /* clip to end of LCD buffer */
    yEnd = G_Fb.pos.y + assetList[assetId].y;
    if (yEnd >= LCD_YSIZE) yEnd = LCD_YSIZE-1;

    for (y = G_Fb.pos.y; y < yEnd; y++) {
	pixdata = uCHAR(&(assetList[assetId].pixdata[ (y - G_Fb.pos.y) * (assetList[assetId].x >> 1) + seqNum * (assetList[assetId].x >> 1) * assetList[assetId].y]));

	for (x = 0; x < (assetList[assetId].x); /* manual inc */ ) {
		pixbyte = *pixdata++; /* 2 pixels per byte */

		/* 1st pixel */
		if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

		ci = ((pixbyte >> 4) & 0xF);
		if (ci != G_Fb.transIndex) { /* transparent? */
		    cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		    r = cmap[0];
		    g = cmap[1];
		    b = cmap[2];

		    pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		    /* G_Fb.pos.x == offset into scan buffer */
		    fb_mark_row_changed(x + G_Fb.pos.x, y);
		    if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		    else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
		}
		x++;

		/* 2nd pixel */
		if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

		ci = pixbyte & 0xF;
		if (ci != G_Fb.transIndex) { /* transparent? */
		    cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		    r = cmap[0];
		    g = cmap[1];
		    b = cmap[2];

		    pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		    /* G_Fb.pos.x == offset into scan buffer */
		    fb_mark_row_changed(x + G_Fb.pos.x, y);
		    if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		    else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
		}
		x++;
	}
    }
    G_Fb.changed = 1;
}

void FbImage2bit(unsigned char assetId, unsigned char seqNum)
{
    unsigned char y, yEnd, x;
    unsigned char *pixdata, pixbyte, ci, *cmap, r, g, b;
    unsigned short pixel;

    /* clip to end of LCD buffer */
    yEnd = G_Fb.pos.y + assetList[assetId].y;
    if (yEnd > LCD_YSIZE) yEnd = LCD_YSIZE-1;

    for (y = G_Fb.pos.y; y < yEnd; y++) {
	 pixdata = uCHAR(&(assetList[assetId].pixdata[ (y - G_Fb.pos.y) * (assetList[assetId].x >> 2) + seqNum * (assetList[assetId].x >> 2) * assetList[assetId].y]));

	 for (x = 0; x < (assetList[assetId].x); /* manual inc */) {
	    pixbyte = *pixdata++; /* 4 pixels per byte */

		/* ----------- 1st pixel ----------- */
	    if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

	    ci = ((pixbyte >> 6) & 0x3);
	    if (ci != G_Fb.transIndex) { /* transparent? */
		cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		r = cmap[0];
		g = cmap[1];
		b = cmap[2];

		pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		/* G_Fb.pos.x == offset into scan buffer */
		fb_mark_row_changed(x + G_Fb.pos.x, y);
		if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
	    }
	    x++;

		/* ----------- 2nd pixel ----------- */
	    if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

	    ci = ((pixbyte >> 4) & 0x3);
	    if (ci != G_Fb.transIndex) { /* transparent? */
		cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		r = cmap[0];
		g = cmap[1];
		b = cmap[2];

		pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		/* G_Fb.pos.x == offset into scan buffer */
		fb_mark_row_changed(x + G_Fb.pos.x, y);
		if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
	    }
	    x++;

	    /* ----------- 3rd pixel ----------- */
	    if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

	    ci = ((pixbyte >> 2) & 0x3);
	    if (ci != G_Fb.transIndex) { /* transparent? */
		cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		r = cmap[0];
		g = cmap[1];
		b = cmap[2];

		pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		/* G_Fb.pos.x == offset into scan buffer */
		fb_mark_row_changed(x + G_Fb.pos.x, y);
		if (G_Fb.transMask > 0)
		if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
	    }
	    x++;

	    /* ----------- 4th pixel ----------- */
	    if ((x + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

	    ci = ((pixbyte) & 0x3);
	    if (ci != G_Fb.transIndex) { /* transparent? */
		cmap = uCHAR(&(assetList[assetId].data_cmap[ci * 3]));

		r = cmap[0];
		g = cmap[1];
		b = cmap[2];

		pixel = ((((r >> 3) & 0b11111) << 11 )
			  |  (((g >> 3) & 0b11111) <<  6 )
			  |  (((b >> 3) & 0b11111)       )) ;

		/* G_Fb.pos.x == offset into scan buffer */
		fb_mark_row_changed(x + G_Fb.pos.x, y);
		if (G_Fb.transMask > 0)
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
		else
			BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
	    }
	    x++;
	}
    }
    G_Fb.changed = 1;
}

void FbImage1bit(unsigned char assetId, unsigned char seqNum)
{
    unsigned char y, yEnd, x;
    unsigned char *pixdata, pixbyte, ci; // , *cmap, r, g, b;
    //unsigned short pixel;

    yEnd = G_Fb.pos.y + assetList[assetId].y;
    /* clip to end of LCD buffer */
    if (yEnd >= LCD_YSIZE) yEnd = LCD_YSIZE-1;

    for (y=G_Fb.pos.y; y < yEnd; y++) {
	pixdata = uCHAR(&(assetList[assetId].pixdata[ seqNum * (assetList[assetId].x >> 3) * assetList[assetId].y + (y - G_Fb.pos.y) * (assetList[assetId].x >> 3)]));

	for (x=0; x < (assetList[assetId].x); x += 8) {
	    unsigned char bit;

	    pixbyte = *pixdata++; 

	    for (bit=0; bit < 8; bit++) { /* 8 pixels per byte */
		if ((bit + G_Fb.pos.x) > (LCD_XSIZE-1)) continue; /* clip x */

		ci = ((pixbyte >> bit) & 0x1); /* ci = color index */
		if (ci != G_Fb.transIndex) { // transparent?
		fb_mark_row_changed(x + G_Fb.pos.x + bit, y);
		    if (ci == 0) {
			if (G_Fb.transMask > 0) 
			    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) = (BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) & (~G_Fb.transMask)) | (G_Fb.BGcolor & G_Fb.transMask);
			else
			    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) = G_Fb.BGcolor;
		    } else {
			if (G_Fb.transMask > 0) 
			    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) = (BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) & (~G_Fb.transMask)) | (G_Fb.color & G_Fb.transMask);
			else
			    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x + bit) = G_Fb.color;
		    }
		}
	    }
	}
    }
    G_Fb.changed = 1;
}

/* 
   FbTransparentIndex, also sometimes called key/chromakey color AKA bluescreen/greenscreen
   using this Index in an image means to use what is already in the scanline[] buffer 
   instead of this pixel
*/
void FbTransparentIndex(unsigned short color)
{
    G_Fb.transIndex = color;
}

#ifdef SMALLFONT
void FbCharacter(unsigned char charin)
{
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
    FbImage1bit(G_Fb.font, charin);

    /* advance x pos, but not y */
    // FbMove(G_Fb.pos.x + assetList[G_Fb.font].x, G_Fb.pos.y);
    G_Fb.changed = 1;
}
#else
void FbCharacter(unsigned char charin)
{
    if ((charin < 32) | (charin > 126)) charin = 32;

    charin -= 32;
    FbImage1bit(G_Fb.font, charin);

    /* advance x pos, but not y */
    // FbMove(G_Fb.pos.x + assetList[G_Fb.font].x, G_Fb.pos.y);
    G_Fb.changed = 1;
}
#endif

void FbFilledRectangle(unsigned char width, unsigned char height)
{
    unsigned int y, x, endX, endY;

    endX = G_Fb.pos.x + width;
    if (endX >= LCD_XSIZE) endX = LCD_XSIZE;

    endY = G_Fb.pos.y + height;
    if (endY >= LCD_YSIZE) endY = LCD_YSIZE;

    for (y=G_Fb.pos.y; y < endY; y++) {
	for (x=G_Fb.pos.x; x < endX; x++) {
	    BUFFER(y * LCD_XSIZE + x) = G_Fb.color;
	}
    }
    FbMove(endX, endY);
    G_Fb.changed = 1;
}

void FbPoint(unsigned char x, unsigned char y)
{
    if (x >= LCD_XSIZE) x = LCD_XSIZE-1;
    if (y >= LCD_YSIZE) y = LCD_YSIZE-1;

    BUFFER(y * LCD_XSIZE + x) = G_Fb.color;
    fb_mark_row_changed(x, y);

    FbMove(x, y);
    G_Fb.changed = 1;
}

void FbHorizontalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    unsigned char x;

    FbMove(x1, y1);
    for (x=x1; x<x2; x++) FbPoint(x, y1);

    G_Fb.changed = 1;
}

void FbVerticalLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    unsigned char y;

    FbMove(x1, y1);
    for (y=y1; y<y2; y++) FbPoint(x1, y);
    G_Fb.changed = 1;
}

void FbLine1(unsigned char x1, unsigned char y1)
{
    FbLine(G_Fb.pos.x, G_Fb.pos.y, x1, y1);
    G_Fb.pos.x = x1;
    G_Fb.pos.y = y1;
}

void FbLine(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy)/2, e2;

    for(;;) {
	FbPoint(x0, y0); /* optimise this: join multiple y==y points into one segments */

	if (x0==x1 && y0==y1) break;

	e2 = err;
	if (e2 > -dx) { err -= dy; x0 += sx; }
	if (e2 < dy) { err += dx; y0 += sy; }
    }
    G_Fb.changed = 1;
}

void FbWriteLine(unsigned char *string)
{
    unsigned char j, x, y;

    //debug("FbWriteLine ");

    x = G_Fb.pos.x;
    y = G_Fb.pos.y;
    
    for(j=0; string[j] != 0; j++) {
	FbMove(x + j * assetList[G_Fb.font].x, y);
	FbCharacter(string[j]);
    }
    G_Fb.changed = 1;
}

void FbWriteString(unsigned char *string)
{
    unsigned char j, x, y;

    x = G_Fb.pos.x;
    y = G_Fb.pos.y;
    
    for(j=0; string[j] != 0; j++) {
	if (string[j] == '\n') {
		FbMoveRelative(0, 8);
		FbMoveX(x);
	} else {
		FbCharacter(string[j]);
		FbMoveRelative(8, 0);
	}
    }
    G_Fb.changed = 1;
}

void FbRectangle(unsigned char width, unsigned char height)
{
    unsigned char x, y;

    x = G_Fb.pos.x;
    y = G_Fb.pos.y;
    FbVerticalLine(x,	 y, x,	 y + height);
    FbVerticalLine(x + width, y, x + width, y + height);

    FbHorizontalLine(x, y,	  x + width, y	 );
    FbHorizontalLine(x, y + height, x + width, y + height);
    G_Fb.changed = 1;
}

/*
  doesn't currently swap buffers, just flushes ram->LCD
  could do double buffer with 4,2 or 1 bit per pixel
*/
void FbSwapBuffers()
{
    unsigned int i;

    //debug("FbWriteLine");

    if (G_Fb.changed == 0) return;

    S6B33_rect(0, 0, LCD_XSIZE-1, LCD_YSIZE-1);
    for (i=0; i<(LCD_XSIZE * LCD_YSIZE); i++) {
	S6B33_pixel(BUFFER(i));
	BUFFER(i) = G_Fb.BGcolor; /* clear buffer as we go */
    }
    G_Fb.changed = 0;

    G_Fb.pos.x = 0;
    G_Fb.pos.y = 0;
}

extern int getRotate(void);

/* Copies LCDbuffer to screen one row at at time and only if that row has changed.
 * If your app only changes small parts of the screen at a time, this can be faster.
 */
FbPaintNewRows(void)
{
	unsigned int i, j;
	int rotated = getRotate();

	if (G_Fb.changed == 0)
		return;

	for (i =0; i < LCD_YSIZE; i++) {
		/* Skip painting rows that have not changed */
		if (!IS_ROW_CHANGED(i))
			continue;
		/* Copy changed rows to screen and to old[] buffer */
		if (!rotated)
			S6B33_rect(i, min_changed_x[i], 1, max_changed_x[i] - min_changed_x[i] + 1);
		else
			S6B33_rect(min_changed_x[i], i, max_changed_x[i] - min_changed_x[i] + 1, 1);
		for (j = min_changed_x[i]; j <= max_changed_x[i]; j++)
			S6B33_pixel(LCDbuffer[i * LCD_YSIZE + j]);
		MARK_ROW_UNCHANGED(i);
	}
	G_Fb.changed = 0;
	G_Fb.pos.x = 0;
	G_Fb.pos.y = 0;
}

// Move buffer to screen without clearing the buffer
// - Useful for making incremental changes to a consistent scene
void FbPushBuffer()
{
    unsigned int i;

    //debug("FbWriteLine");

    if (G_Fb.changed == 0) return;

    S6B33_rect(0, 0, LCD_XSIZE-1, LCD_YSIZE-1);
    for (i=0; i<(LCD_XSIZE * LCD_YSIZE); i++) {
        S6B33_pixel(BUFFER(i));
	//BUFFER(i) = G_Fb.BGcolor; /* clear buffer as we go */
    }
    G_Fb.changed = 0;

//    G_Fb.pos.x = 0;
//    G_Fb.pos.y = 0;
}

//doesn't work
void FbPushRegion(unsigned int x, unsigned int y, 
        unsigned int width,unsigned int height)
{
    unsigned int i, j;

    //debug("FbWriteLine");

    if (G_Fb.changed == 0) return;

    S6B33_rect(x, y, width, height);
    for(i=y-1;i < (y+height-1);i++) // for each row
    {
        for(j=(LCD_XSIZE * i) + x; 
                j < ((LCD_XSIZE * i) + x + width); 
                j++)
            S6B33_pixel(BUFFER(j));
        
    }
    G_Fb.changed = 0;

    G_Fb.pos.x = 0;
    G_Fb.pos.y = 0;
}

void FbDrawVectors(short points[][2],
                   unsigned char n_points,
                   short center_x,
                   short center_y,
                   unsigned char connect_last_to_first)
{
    unsigned char n = 0;
    short x0, y0, x1, y1;
    for(n=0; n < (n_points-1); n++)
    {
        x0 = points[n][0] + center_x;
        y0 = points[n][1] + center_y;
        x1 = points[n+1][0] + center_x;
        y1 = points[n+1][1] + center_y;
        // Don't bother with the line if
        // either point is out of bounds
        if(!(
                (x0 < 1) || (x0 > 131)
                || (x1 < 1) || (x1 > 131)
                || (y0 < 1) || (y0 > 131)
                || (y1 < 1) || (y1 > 131)
        ))
            FbLine((unsigned char)x0, (unsigned char)y0,
                   (unsigned char)x1, (unsigned char)y1);
    }

    if(connect_last_to_first){
        x0 = points[n_points-1][0] + center_x;
        y0 = points[n_points-1][1] + center_y;
        x1 = points[0][0] + center_x;
        y1 = points[0][1] + center_y;

        if(!(
                (x0 < 0) || (x0 > 132)
                || (x1 < 0) || (x1 > 132)
                || (y0 < 0) || (y0 > 132)
                || (y1 < 0) || (y1 > 132)
        ))
            FbLine((unsigned char)x0, (unsigned char)y0,
                   (unsigned char)x1, (unsigned char)y1);
    }
}

void FbPolygonFromPoints(short points[][2],
                         unsigned char n_points,
                         short center_x,
                         short center_y)
{

    FbDrawVectors(points, n_points, center_x, center_y, 1);
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

