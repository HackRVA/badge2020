/* This is a program to rescale and center "monster" pictures."
 * To use it:
 *
 * fixup_monster monsterfile.h > fixed_monsterfile.h
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int npoints = 0;

#define MAXPOINTS 10000
struct point {
	int x, y;
	float fx, fy;
} point[MAXPOINTS];

static void usage(void)
{
	fprintf(stderr, "usage: fixup_monster monsterfile.h > fixed_monsterfile.h\n");
	exit(1);
}

static void read_monster(FILE *f, struct point *p, int *npoints, int maxpoints)
{
	char *l, buffer[1000];
	int rc, x, y;

	do {
		l = fgets(buffer, sizeof(buffer), f);
		if (!l) {
			return;
		}
#define SPACES_OR_OPENBRACKET "%*[ 	{]"
#define SPACES_OR_COMMA "%*[ 	,]"
#define SPACES_OR_CLOSEBRACKET "%*[ 	]}"
		rc = sscanf(buffer, SPACES_OR_OPENBRACKET "%d" SPACES_OR_COMMA "%d" SPACES_OR_CLOSEBRACKET, &x, &y);
		if (rc != 2)
			continue;
		p[*npoints].x = x;
		p[*npoints].y = y;
		(*npoints)++;
		if (*npoints >= maxpoints)
			break;
	} while (1);
}

static void process_monster(struct point *p, int npoints)
{
	float minx, miny, maxx, maxy, width, height;
	float scale, xscale, yscale, xtrans, ytrans;
	float centerx, centery;
	int i;

	minx = 1000;
	miny = 1000;
	maxx = -1000;
	maxy = -1000;

	/* Find min and max extents of monster */
	for (i = 0; i < npoints; i++) {
		p[i].fx = p[i].x;
		p[i].fy = p[i].y;
		if (p[i].x == -128)
			continue;
		if (p[i].fx > maxx)
			maxx = p[i].fx;
		if (p[i].fy > maxy)
			maxy = p[i].fy;
		if (p[i].fx < minx)
			minx = p[i].fx;
		if (p[i].fy < miny)
			miny = p[i].fy;
	}

	/* Calculate scale and translation adjustments */
	width = maxx - minx;
	height = maxy - miny;
	xscale = 250.0 / width;
	yscale = 250.0 / height;
	scale = xscale > yscale ? yscale : xscale;
	centerx = (0.5 * width) + minx;
	centery = (0.5 * height) + miny;
	xtrans = -centerx;
	ytrans = -centery;

	/* Translate */
	for (i = 0; i < npoints; i++) {
		if (p[i].x == -128)
			continue;
		p[i].fx += xtrans;
		p[i].fy += ytrans;
	}

	/* Scale */
	for (i = 0; i < npoints; i++) {
		if (p[i].x == -128)
			continue;
		p[i].fx *= scale;
		p[i].fy *= scale;
	}
}

static void write_monster(struct point *p, int npoints)
{
	int i;
	printf("{\n");
	for (i = 0; i < npoints; i++)
		printf("  { %d, %d },\n", (int) p[i].fx, (int) p[i].fy);
	printf("};\n");
}

int main(int argc, char *argv[])
{
	npoints = 0;
	char *filename;
	FILE *f;

	if (argc < 2)
		usage();
	filename = argv[1];

	f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "%s: fopen %s\n", filename, strerror(errno));
		exit(1);
	}
	read_monster(f, point, &npoints, MAXPOINTS);
	fclose(f);
	process_monster(point, npoints);
	write_monster(point, npoints);
	exit(0);
}

