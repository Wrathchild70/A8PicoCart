#include <math.h>
#include <ff.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "render3d.h"

/* Define window size */
#define H 192  // height of screen
#define W 80  // width of _game_ screen (in SplitScreen mode, map is drawn in the remaining space)
#define W2 W // width of screen

/* Define camera height from floor and distance to ceiling */
#define EyeHeight  6
#define DuckHeight 2.5
#define HeadMargin 1
#define KneeHeight 2 /* Maximum walkable obstacle height */
/* Define factors that affect the field of vision (never >= 180 degrees though) */
#define hfov (1.0 * 0.73f*H/W)
#define vfov (1.0 * .2f)

#define blur_width (W / 120)
#define blur_height (H / 90)

// Utility functions. Because C doesn't have templates,
// we use the slightly less safe preprocessor macros to
// implement these functions that work with multiple types.
#define min(a,b) (((a) < (b)) ? (a) : (b)) // min: Choose smaller of two values.
#define max(a,b) (((a) > (b)) ? (a) : (b)) // max: Choose bigger of two values.
#define abs(a) ((a) < 0 ? -(a) : (a))
#define clamp(a, mi,ma) min(max(a,mi),ma)  // clamp: Clamp value into set range.
#define sign(v) (((v) > 0) - ((v) < 0))    // sign: Return the sign of a value (-1, 0 or 1)
#define vxs(x0,y0, x1,y1) ((x0)*(y1) - (x1)*(y0)) // vxs: Vector cross product
// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0,a1,b0,b1) (min(a0,a1) <= max(b0,b1) && min(b0,b1) <= max(a0,a1))
// IntersectBox: Determine whether two 2D-boxes intersect.
#define IntersectBox(x0,y0, x1,y1, x2,y2, x3,y3) (Overlap(x0,x1,x2,x3) && Overlap(y0,y1,y2,y3))
// PointSide: Determine which side of a line the point is on. Return value: -1, 0 or 1.
#define PointSide(px,py, x0,y0, x1,y1) sign(vxs((x1)-(x0), (y1)-(y0), (px)-(x0), (py)-(y0)))
// Intersect: Calculate the point of intersection between two lines.
#define Intersect(x1,y1, x2,y2, x3,y3, x4,y4) ((struct xy) { \
    vxs(vxs(x1,y1, x2,y2), (x1)-(x2), vxs(x3,y3, x4,y4), (x3)-(x4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)), \
    vxs(vxs(x1,y1, x2,y2), (y1)-(y2), vxs(x3,y3, x4,y4), (y3)-(y4)) / vxs((x1)-(x2), (y1)-(y2), (x3)-(x4), (y3)-(y4)) })

// Some hard-coded limits.
#define MaxVertices 128 // maximum number of vertices in a map
#define MaxEdges 128    // maximum number of edges in a sector
#define MaxQueue 32     // maximum number of pending portal renders

/* Sectors: Floor and ceiling height; list of wall vertexes and neighbors */
static struct sector
{
	float floor, ceil;
	struct xy { float x, y; } *vertex; /* Each vertex has an x and y coordinate */
	unsigned short npoints;            /* How many vertexes there are */
	signed char *neighbors;            /* Each pair of vertexes may have a corresponding neighboring sector */
} *sectors = NULL;
static unsigned char NumSectors = 0;
static unsigned char NumVertices = 0;
static float MaxAngle = M_PI * 2;
static float TurnAngle = M_PI / 60.0f;

typedef enum _MYKEYS {
	KEY_ESCAPE = 0,
	KEY_W,
	KEY_S,
	KEY_A,
	KEY_D,
	KEY_Z,
	KEY_SPACE,
	KEY_TAB,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_BKSP,
	KEY_LAST
} MYKEYS;

#define MAP_FILE "test-map.txt"

static unsigned char *movement_key = &cart_d5xx[0xD0];
static unsigned char ground = 0, falling = 1, moving = 0, ducking = 0, map = 0;

#define BLACK 0
#define DARK_GREY 1
#define LIGHT_GREY 1
#define WHITE 2
#define DARK_RED 3
#define MID_RED 4
#define LIGHT_RED 5
#define DARK_GREEN 6
#define MID_GREEN 7
#define LIGHT_GREEN 8
#define MID_PURPLE 6
#define MID_BLUE 7
#define LIGHT_BLUE 8

/* Player: location */
static struct player
{
	struct xyz { float x, y, z; } where,      /* Current position */
		velocity;   /* Current motion vector */
	float angle, anglesin, anglecos, yaw;   /* Looking towards (and sin() and cos() thereof) */
	unsigned char sector;                   /* Which sector the player is currently in */
} player;

static void __not_in_flash_func(LoadData)(void)
{
	static FATFS	FatFs;
	static FIL		fil;
	if (f_mount(&FatFs, "", 1) == FR_OK)
	{
		cart_d5xx[0xDC] = '\0';
		cart_d5xx[0xDD] = '\0';
		if (f_open(&fil, MAP_FILE, FA_READ) == FR_OK)
		{
			char Buf[256], word[256], *ptr;
			struct xy vertex[MaxVertices];
			float x, y, angle, number, numbers[MaxEdges];
			int n, m, l = 0;
			while (f_gets(Buf, sizeof(Buf), &fil))
			{
				cart_d5xx[0xDC] = ++l;
				switch (sscanf(ptr = Buf, "%32s%n", word, &n) == 1 ? word[0] : '\0')
				{
				case 'v': // vertex
					cart_d5xx[0xD9] = (int)x;
					cart_d5xx[0xDA] = (int)y;
					cart_d5xx[0xDB] = NumVertices;
					for (sscanf(ptr += n, "%f%n", &y, &n); sscanf(ptr += n, "%f%n", &x, &n) == 1; )
					{
						if (NumVertices >= MaxVertices) {
//							fprintf(stderr, "ERROR: Too many vertices, limit is %u\n", MaxVertices);
							cart_d5xx[0xDD] = 'A';
//							exit(2);
							return;
						}
						vertex[NumVertices].x = x;
						vertex[NumVertices].y = y;
						NumVertices++;
						cart_d5xx[0xDB] = NumVertices;
					}
					if (l == 2)
					{
						cart_d5xx[0xDD] = 'A';
						return;
					}
					break;
				case 's': // sector
					sectors = realloc(sectors, ++NumSectors * sizeof(struct sector));
					struct sector* sect = &sectors[NumSectors - 1];
					sscanf(ptr += n, "%f%f%n", &sect->floor, &sect->ceil, &n);
					for (m = 0; sscanf(ptr += n, "%32s%n", word, &n) == 1 && word[0] != '#'; )
					{
						if (m >= MaxEdges)
						{
//							fprintf(stderr, "ERROR: Too many edges in sector %u. Limit is %u\n", NumSectors - 1, MaxEdges);
							cart_d5xx[0xDD] = 'B';
//							exit(2);
							return;
						}
						numbers[m++] = word[0] == 'x' ? -1 : strtof(word, 0);
					}
					sect->npoints = m /= 2;
					sect->neighbors = malloc((m) * sizeof(signed char));
					sect->vertex = malloc((m + 1) * sizeof(struct xy));
					for (n = 0; n<m; ++n) sect->neighbors[n] = numbers[m + n];
					for (n = 0; n<m; ++n)
					{
						int v = numbers[n];
						if (v >= NumVertices)
						{
//							fprintf(stderr, "ERROR: Invalid vertex number %d in sector %u; only have %u\n",
//								v, NumSectors - 1, NumVertices);
							cart_d5xx[0xD9] = (unsigned char)v;
							cart_d5xx[0xDA] = NumSectors - 1;
							cart_d5xx[0xDB] = NumVertices;
							cart_d5xx[0xDD] = 'C';
//							exit(2);
							return;
						}
						// TODO: bounds checking
						sect->vertex[n + 1].x = vertex[v].x;
						sect->vertex[n + 1].y = vertex[v].y;
					}
					sect->vertex[0] = sect->vertex[m];
					break;
				case 'p': // player
					sscanf(ptr += n, "%f %f %f %f", &x, &y, &angle, &number);
					player.where.x = x;
					player.where.y = y;
					player.where.z = sectors[player.sector].floor + EyeHeight;
					player.velocity.x = 0;
					player.velocity.y = 0;
					player.velocity.z = 0;
					player.angle = angle;
					player.anglesin = sinf(player.angle);
					player.anglecos = cosf(player.angle);
					player.yaw = 0;
					player.sector = number;
				}
			}
			f_close(&fil);
		}
	}
}

/*
static void __not_in_flash_func(UnloadData)(void)
{
	for (unsigned a = 0; a<NumSectors; ++a)
		free(sectors[a].vertex), free(sectors[a].neighbors);
	free(sectors);
	sectors = NULL;
	NumSectors = 0;
}
*/

static int __not_in_flash_func(IntersectLineSegments)(float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
	return IntersectBox(x0, y0, x1, y1, x2, y2, x3, y3)
		&& abs(PointSide(x2, y2, x0, y0, x1, y1) + PointSide(x3, y3, x0, y0, x1, y1)) != 2
		&& abs(PointSide(x0, y0, x2, y2, x3, y3) + PointSide(x1, y1, x2, y2, x3, y3)) != 2;
}

struct Scaler { int result, bop, fd, ca, cache; };

#define Scaler_Init(a,b,c,d,f) \
    { d + (b-1 - a) * (f-d) / (c-a), ((f<d) ^ (c<a)) ? -1 : 1, \
      abs(f-d), abs(c-a), (int)((b-1-a) * abs(f-d)) % abs(c-a) }

// Scaler_Next: Return (b++ - a) * (f-d) / (c-a) + d using the initial values passed to Scaler_Init().
static int __not_in_flash_func(Scaler_Next)(struct Scaler* i)
{
	for (i->cache += i->fd; i->cache >= i->ca; i->cache -= i->ca) i->result += i->bop;
	return i->result;
}

// Helper function for the antialiased line algorithm.
#define fpart(x) ((x) < 0 ? 1 - ((x) - floorf(x)) : (x) - floorf(x))
#define rfpart(x) (1 - fpart(x))

static void __not_in_flash_func(plot)(int x, int y, float opacity, uint8_t color)
{
	unsigned int mask = 0xF;
	if ((x & 1) == 0) color <<= 4;
	else mask = 0xF0;

	uint8_t *buffer = ScreenRAM + 0x10;
	buffer += y * 40;
	buffer += x / 2;

	*buffer = (*buffer & mask) | color;
}

static void __not_in_flash_func(line)(float x0, float y0, float x1, float y1, uint8_t color)
{
	// Xiaolin Wu's antialiased line algorithm from Wikipedia.
	int steep = fabsf(y1 - y0) > fabsf(x1 - x0);
	if (steep) { float tmp; tmp = x0; x0 = y0; y0 = tmp; tmp = x1; x1 = y1; y1 = tmp; }
	if (x0 > x1) { float tmp; tmp = x0; x0 = x1; x1 = tmp; tmp = y0; y0 = y1; y1 = tmp; }
	float dx = x1 - x0, dy = y1 - y0, gradient = dy / dx;
	// handle first endpoint
	int xend = (int)(x0 + 0.5f);
	int yend = y0 + gradient * (xend - x0);
	float xgap = rfpart(x0 + 0.5f);
	int xpxl1 = xend; // this will be used in the main loop
	int ypxl1 = (int)(yend);
	if (steep)
	{
		plot(ypxl1, xpxl1, rfpart(yend) * xgap, color);
		plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap, color);
	}
	else
	{
		plot(xpxl1, ypxl1, rfpart(yend) * xgap, color);
		plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap, color);
	}
	float intery = yend + gradient; // first y-intersection for the main loop
									// handle second endpoint
	xend = (int)(x1 + 0.5f);
	yend = y1 + gradient * (xend - x1);
	xgap = fpart(x1 + 0.5);
	int xpxl2 = xend; //this will be used in the main loop
	int ypxl2 = (int)(yend);
	if (steep)
	{
		plot(ypxl2, xpxl2, rfpart(yend) * xgap, color);
		plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap, color);
	}
	else
	{
		plot(xpxl2, ypxl2, rfpart(yend) * xgap, color);
		plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap, color);
	}
	// main loop
	for (int x = xpxl1 + 1; x < xpxl2; ++x, intery += gradient)
		if (steep)
		{
			plot((int)(intery), x, rfpart(intery), color);
			plot((int)(intery)+1, x, fpart(intery), color);
		}
		else
		{
			plot(x, (int)(intery), rfpart(intery), color);
			plot(x, (int)(intery)+1, fpart(intery), color);
		}
}

// BloomPostprocess adds some bloom to the 2D map image. It is merely a cosmetic device.
/*
static void __not_in_flash_func(BloomPostprocess)(void)
{
	float blur_kernel[blur_height * 2 + 1][blur_width * 2 + 1];
	for (int y = -blur_height; y <= blur_height; ++y)
	{
		for (int x = -blur_width; x <= blur_width; ++x)
		{
			float value = expf(-(x*x + y*y) / (2.f*(0.5f*max(blur_width, blur_height))));
			blur_kernel[y + blur_height][x + blur_width] = value * 0.3f;
			//printf("%10.3f", value);
		}
		//printf("\n");
	}

	static int pixels_original[W2*H];
	static struct pixel { float r, g, b, brightness; } img[W2*H];
	memcpy(pixels_original, surface->pixels, sizeof(pixels_original));
	int *pix = ((int*)surface->pixels);

	for (unsigned y = 0; y<H; ++y)
		for (unsigned x = 0; x<W2; ++x)
		{
			int original_pixel = pixels_original[y*W2 + x];
			float r = (original_pixel >> 16) & 0xFF;
			float g = (original_pixel >> 8) & 0xFF;
			float b = (original_pixel >> 0) & 0xFF;
			float wanted_br = original_pixel == 0xFFFFFF ? 1
				: original_pixel == 0x55FF55 ? 0.6
				: original_pixel == 0xFFAA55 ? 1
				: 0.1;
			float brightness = powf((r*0.299f + g*0.587f + b*0.114f) / 255.f, 12.f / 2.2f);
			brightness = (brightness*0.2f + wanted_br * 0.3f + max(max(r, g), b)*0.5f / 255.f);
			img[y*W2 + x] = (struct pixel) { r, g, b, brightness };
		}

#pragma omp parallel for schedule(static) collapse(2)
	for (unsigned y = 0; y<H; ++y)
#ifdef SplitScreen
		for (unsigned x = W; x<W2; ++x)
#else
		for (unsigned x = 0; x<W; ++x)
#endif
		{
			int ypmin = max(0, (int)y - blur_height), ypmax = min(H - 1, (int)y + blur_height);
			int xpmin = max(0, (int)x - blur_width), xpmax = min(W - 1, (int)x + blur_width);
			float rsum = img[y*W2 + x].r;
			float gsum = img[y*W2 + x].g;
			float bsum = img[y*W2 + x].b;
			for (int yp = ypmin; yp <= ypmax; ++yp)
				for (int xp = xpmin; xp <= xpmax; ++xp)
				{
					float r = img[yp*W2 + xp].r;
					float g = img[yp*W2 + xp].g;
					float b = img[yp*W2 + xp].b;
					float brightness = img[yp*W2 + xp].brightness;
					float value = brightness * blur_kernel[yp + blur_height - (int)y][xp + blur_width - (int)x];
					rsum += r * value;
					gsum += g * value;
					bsum += b * value;
				}
			int color = (((int)clamp(rsum, 0, 255)) << 16)
				+ (((int)clamp(gsum, 0, 255)) << 8)
				+ (((int)clamp(bsum, 0, 255)) << 0);
			pix[y*W2 + x] = color;
		}
}
*/

/* fillpolygon draws a filled polygon -- used only in the 2D map rendering. */
static void __not_in_flash_func(fillpolygon)(const struct sector* sect, uint8_t color)
{
	float square = min(W / 20.f / 0.8, H / 29.f), X = square*0.8, Y = square, X0 = (W - 18 * square*0.8) / 2, Y0 = (H - 28 * square) / 2;
	const struct xy* const vert = sect->vertex;
	// Find the minimum and maximum Y coordinates
	float miny = 9e9, maxy = -9e9;
	for (unsigned a = 0; a < sect->npoints; ++a)
	{
		miny = min(miny, 28 - vert[a].x);
		maxy = max(maxy, 28 - vert[a].x);
	}
	miny = Y0 + miny*Y; maxy = Y0 + maxy*Y;
	// Scan each line within this range
	for (int y = max(0, (int)(miny + 0.5)); y <= min(H - 1, (int)(maxy + 0.5)); ++y)
	{
		// Find all intersection points on this scanline
		float intersections[W2];
		unsigned num_intersections = 0;
		for (unsigned a = 0; a < sect->npoints && num_intersections < W; ++a)
		{
			float x0 = X0 + vert[a].y*X, x1 = X0 + vert[a + 1].y*X;
			float y0 = Y0 + (28 - vert[a].x)*Y, y1 = Y0 + (28 - vert[a + 1].x)*Y;
			if (IntersectBox(x0, y0, x1, y1, 0, y, W2 - 1, y))
			{
				struct xy point = Intersect(x0, y0, x1, y1, 0, y, W2 - 1, y);
				if (isnan(point.x) || isnan(point.y)) continue;
				// Insert it in intersections[] keeping it sorted.
				// Sorting complexity: n log n
				unsigned begin = 0, end = num_intersections, len = end - begin;
				while (len)
				{
					unsigned middle = begin + len / 2;
					if (intersections[middle] < point.x)
					{
						begin = middle++; len = len - len / 2 - 1;
					}
					else
						len /= 2;
				}
				for (unsigned n = num_intersections++; n > begin; --n)
					intersections[n] = intersections[n - 1];
				intersections[begin] = point.x;
			}
		}
		// Draw lines
		for (unsigned a = 0; a + 1 < num_intersections; a += 2)
			line(clamp(intersections[a], 0, W2 - 1), y,
				clamp(intersections[a + 1], 0, W2 - 1), y, color);
		//printf("line(%f,%d, %f,%d)\n", minx,y, maxx,y);
	}
}

static void __not_in_flash_func(DrawMap)(void)
{
	static unsigned process = ~0u; ++process;

	// Render the 2D map on screen
	for (unsigned y = 0; y < H; ++y)
		line(0, y, W2 - 1, y, BLACK);
	float square = min(W / 20.f / 0.8, H / 29.f), X = square*0.8, Y = square, X0 = (W - 18 * square*0.8) / 2, Y0 = (H - 28 * square) / 2;
	for (float x = 0; x <= 18; ++x) line(X0 + x*X, Y0 + 0 * Y, X0 + x*X, Y0 + 28 * Y, DARK_GREY);
	for (float y = 0; y <= 28; ++y) line(X0 + 0 * X, Y0 + y*Y, X0 + 18 * X, Y0 + y*Y, DARK_GREY);

	fillpolygon(&sectors[player.sector], DARK_RED);

	for (unsigned c = 0; c<NumSectors; ++c)
	{
		unsigned a = c;
		if (a == player.sector && player.sector != (NumSectors - 1))
			a = NumSectors - 1;
		else if (a == NumSectors - 1)
			a = player.sector;

		const struct sector* const sect = &sectors[a];
		const struct xy* const vert = sect->vertex;
		for (unsigned b = 0; b < sect->npoints; ++b)
		{
			float x0 = 28 - vert[b].x, x1 = 28 - vert[b + 1].x;
			uint8_t vertcolor = a == player.sector ? LIGHT_GREEN
				: MID_GREEN;

			line(X0 + vert[b].y*X, Y0 + x0*Y,
				X0 + vert[b + 1].y*X, Y0 + x1*Y,
				(a == player.sector)
				? (sect->neighbors[b] >= 0 ? LIGHT_RED : WHITE)
				: (sect->neighbors[b] >= 0 ? MID_RED : LIGHT_GREY)
			);

			line(X0 + vert[b].y*X - 2, Y0 + x0*Y - 2, X0 + vert[b].y*X + 2, Y0 + x0*Y - 2, vertcolor);
			line(X0 + vert[b].y*X - 2, Y0 + x0*Y - 2, X0 + vert[b].y*X - 2, Y0 + x0*Y + 2, vertcolor);
			line(X0 + vert[b].y*X + 2, Y0 + x0*Y - 2, X0 + vert[b].y*X + 2, Y0 + x0*Y + 2, vertcolor);
			line(X0 + vert[b].y*X - 2, Y0 + x0*Y + 2, X0 + vert[b].y*X + 2, Y0 + x0*Y + 2, vertcolor);
		}
	}

	float c = player.anglesin, s = -player.anglecos;
	float px = player.where.y, tx = px + c*0.8f, qx0 = px + s*0.2f, qx1 = px - s*0.2f;
	float py = 28 - player.where.x, ty = py + s*0.8f, qy0 = py - c*0.2f, qy1 = py + c*0.2f;
	px = clamp(px, -.4f, 18.4f); tx = clamp(tx, -.4f, 18.4f); qx0 = clamp(qx0, -.4f, 18.4f); qx1 = clamp(qx1, -.4f, 18.4f);
	py = clamp(py, -.4f, 28.4f); ty = clamp(ty, -.4f, 28.4f); qy0 = clamp(qy0, -.4f, 28.4f); qy1 = clamp(qy1, -.4f, 28.4f);

	line(X0 + px*X, Y0 + py*Y, X0 + tx*X, Y0 + ty*Y, LIGHT_BLUE);
	line(X0 + qx0*X, Y0 + qy0*Y, X0 + qx1*X, Y0 + qy1*Y, LIGHT_BLUE);

	// BloomPostprocess();

	//static unsigned skip=0;
	//if(++skip >= 1) { skip=0; SDL_Flip(surface); }
}

static int __not_in_flash_func(vert_compare)(const struct xy* a, const struct xy* b)
{
	if (a->y != b->y) return (a->y - b->y) * 1e3;
	return (a->x - b->x) * 1e3;
}

// Verify map for consistencies
static void __not_in_flash_func(VerifyMap)(void)
{
	//int phase=0;
Rescan:
	//DrawMap(); SDL_Delay(100);

	// Verify that the chain of vertexes forms a loop.
	for (unsigned a = 0; a<NumSectors; ++a)
	{
		const struct sector* const sect = &sectors[a];
		const struct xy* const vert = sect->vertex;

		if (vert[0].x != vert[sect->npoints].x
			|| vert[0].y != vert[sect->npoints].y)
		{
//			fprintf(stderr, "Internal error: Sector %u: Vertexes don't form a loop!\n", a);
		}
	}
	// Verify that for each edge that has a neighbor, the neighbor
	// has this same neighbor.
	for (unsigned a = 0; a<NumSectors; ++a)
	{
		const struct sector* sect = &sectors[a];
		const struct xy* const vert = sect->vertex;
		for (unsigned b = 0; b < sect->npoints; ++b)
		{
			if (sect->neighbors[b] >= (int)NumSectors)
			{
//				fprintf(stderr, "Sector %u: Contains neighbor %d (too large, number of sectors is %u)\n",
//					a, sect->neighbors[b], NumSectors);
			}
			struct xy point1 = vert[b], point2 = vert[b + 1];

			int found = 0;
			for (unsigned d = 0; d < NumSectors; ++d)
			{
				if (a == d) continue;
				const struct sector* const neigh = &sectors[d];
				for (unsigned c = 0; c < neigh->npoints; ++c)
				{
					if (neigh->vertex[c + 1].x == point1.x
						&& neigh->vertex[c + 1].y == point1.y
						&& neigh->vertex[c + 0].x == point2.x
						&& neigh->vertex[c + 0].y == point2.y)
					{
						if (neigh->neighbors[c] != (int)a)
						{
//							fprintf(stderr, "Sector %d: Neighbor behind line (%g,%g)-(%g,%g) should be %u, %d found instead. Fixing.\n",
//								d, point2.x, point2.y, point1.x, point1.y, a, neigh->neighbors[c]);
							neigh->neighbors[c] = a;
							goto Rescan;
						}
						if (sect->neighbors[b] != (int)d)
						{
//							fprintf(stderr, "Sector %u: Neighbor behind line (%g,%g)-(%g,%g) should be %u, %d found instead. Fixing.\n",
//								a, point1.x, point1.y, point2.x, point2.y, d, sect->neighbors[b]);
							sect->neighbors[b] = d;
							goto Rescan;
						}
						else
							++found;
					}
				}
			}
//			if (sect->neighbors[b] >= 0 && sect->neighbors[b] < (int)NumSectors && found != 1)
//				fprintf(stderr, "Sectors %u and its neighbor %d don't share line (%g,%g)-(%g,%g)\n",
//					a, sect->neighbors[b],
//					point1.x, point1.y, point2.x, point2.y);
		}
	}
	// Verify that the vertexes form a convex hull.
	for (unsigned a = 0; a<NumSectors; ++a)
	{
		struct sector* sect = &sectors[a];
		const struct xy* const vert = sect->vertex;
		for (unsigned b = 0; b < sect->npoints; ++b)
		{
			unsigned c = (b + 1) % sect->npoints, d = (b + 2) % sect->npoints;
			float x0 = vert[b].x, y0 = vert[b].y;
			float x1 = vert[c].x, y1 = vert[c].y;
			switch (PointSide(vert[d].x, vert[d].y, x0, y0, x1, y1))
			{
			case 0:
				continue;
				// Note: This used to be a problem for my engine, but is not anymore, so it is disabled.
				//       If you enable this change, you will not need the IntersectBox calls in some locations anymore.
				if (sect->neighbors[b] == sect->neighbors[c]) continue;
//				fprintf(stderr, "Sector %u: Edges %u-%u and %u-%u are parallel, but have different neighbors. This would pose problems for collision detection.\n",
//					a, b, c, c, d);
				break;
			case -1:
//				fprintf(stderr, "Sector %u: Edges %u-%u and %u-%u create a concave turn. This would be rendered wrong.\n",
//					a, b, c, c, d);
				break;
			default:
				// This edge is fine.
				continue;
			}
//			fprintf(stderr, "- Splitting sector, using (%g,%g) as anchor", vert[c].x, vert[c].y);

			// Insert an edge between (c) and (e),
			// where e is the nearest point to (c), under the following rules:
			// e cannot be c, c-1 or c+1
			// line (c)-(e) cannot intersect with any edge in this sector
			float nearest_dist = 1e29f;
			unsigned nearest_point = ~0u;
			for (unsigned n = (d + 1) % sect->npoints; n != b; n = (n + 1) % sect->npoints) // Don't go through b,c,d
			{
				float x2 = vert[n].x, y2 = vert[n].y;
				float distx = x2 - x1, disty = y2 - y1;
				float dist = distx*distx + disty*disty;
				if (dist >= nearest_dist) continue;

				if (PointSide(x2, y2, x0, y0, x1, y1) != 1) continue;

				int ok = 1;

				x1 += distx*1e-4f; x2 -= distx*1e-4f;
				y1 += disty*1e-4f; y2 -= disty*1e-4f;
				for (unsigned f = 0; f < sect->npoints; ++f)
					if (IntersectLineSegments(x1, y1, x2, y2,
						vert[f].x, vert[f].y, vert[f + 1].x, vert[f + 1].y))
					{
						ok = 0; break;
					}
				if (!ok) continue;

				// Check whether this split would resolve the original problem
				if (PointSide(x2, y2, vert[d].x, vert[d].y, x1, y1) == 1) dist += 1e6f;
				if (dist >= nearest_dist) continue;

				nearest_dist = dist;
				nearest_point = n;
			}
			if (nearest_point == ~0u)
			{
//				fprintf(stderr, " - ERROR: Could not find a vertex to pair with!\n");
				continue;
			}
			unsigned e = nearest_point;
//			fprintf(stderr, " and point %u - (%g,%g) as the far point.\n", e, vert[e].x, vert[e].y);
			// Now that we have a chain: a b c d e f g h
			// And we're supposed to split it at "c" and "e", the outcome should be two chains:
			// c d e         (c)
			// e f g h a b c (e)

			struct xy* vert1 = malloc(sect->npoints * sizeof(struct xy));
			struct xy* vert2 = malloc(sect->npoints * sizeof(struct xy));
			signed char* neigh1 = malloc(sect->npoints * sizeof(signed char));
			signed char* neigh2 = malloc(sect->npoints * sizeof(signed char));

			// Create chain 1: from c to e.
			unsigned chain1_length = 0;
			for (unsigned n = 0; n < sect->npoints; ++n)
			{
				unsigned m = (c + n) % sect->npoints;
				neigh1[chain1_length] = sect->neighbors[m];
				vert1[chain1_length].x = sect->vertex[m].x;
				vert1[chain1_length++].y = sect->vertex[m].y;
				if (m == e) {
					vert1[chain1_length].x = vert1[0].x;
					vert1[chain1_length].y = vert1[0].y;
					break;
				}
			}
			neigh1[chain1_length - 1] = NumSectors;
			// Create chain 2: from e to c.
			unsigned chain2_length = 0;
			for (unsigned n = 0; n < sect->npoints; ++n)
			{
				unsigned m = (e + n) % sect->npoints;
				neigh2[chain2_length] = sect->neighbors[m];
				vert2[chain2_length].x = sect->vertex[m].x;
				vert2[chain2_length++].y = sect->vertex[m].y;
				if (m == c)
				{
					vert2[chain2_length].x = vert2[0].x;
					vert2[chain2_length].y = vert2[0].y;
					break;
				}
			}
			neigh2[chain2_length - 1] = a;
			// Change sect into using chain1.
			free(sect->vertex);    sect->vertex = vert1;
			free(sect->neighbors); sect->neighbors = neigh1;
			sect->npoints = chain1_length;
			// Create another sector that uses chain2.
			sectors = realloc(sectors, ++NumSectors * sizeof(struct sector));
			sect = &sectors[a];
			struct sector* lastSect = &sectors[NumSectors - 1];
			lastSect->floor = sect->floor;
			lastSect->ceil = sect->ceil;
			lastSect->vertex = vert2;
			lastSect->npoints = chain2_length;
			lastSect->neighbors = neigh2;
			// The other sector may now have neighbors that think
			// their neighbor is still the old sector. Rescan to fix it.
			goto Rescan;
		}
	}

	//printf("PHASE 2\n"); SDL_Delay(500);
	//if(phase == 0) { phase = 1; goto Rescan; }
//	printf("%d sectors.\n", NumSectors);
}

/* vline: Draw a vertical line on screen, with a different color pixel in top & bottom */
static void __not_in_flash_func(vline)(int x, int y1, int y2, uint8_t top, uint8_t middle, uint8_t bottom)
{
	y1 = clamp(y1, 0, H - 1);
	y2 = clamp(y2, 0, H - 1);
	if (y2 == y1)
		plot(x, y1, 0.f, middle);
	else if (y2 > y1)
	{
		plot(x, y1, 0.f, top);
		for (int y = y1 + 1; y<y2; ++y) plot(x, y, 0.f, middle);
		plot(x, y2, 0.f, bottom);
	}
}

/* Moves the player by (dx,dy) in the map, and also updates
* their anglesin/anglecos/sector properties properly.
*/
static void __not_in_flash_func(MovePlayer)(float dx, float dy)
{
	float px = player.where.x, py = player.where.y;
	/* Check if this movement crosses one of this sector's edges
	* that have a neighboring sector on the other side.
	* Because the edge vertices of each sector are defined in
	* clockwise order, PointSide will always return -1 for a point
	* that is outside the sector and 0 or 1 for a point that is inside.
	*/
	const struct sector* const sect = &sectors[player.sector];
	for (int s = 0; s < sect->npoints; ++s)
		if (sect->neighbors[s] >= 0
			&& IntersectBox(px, py, px + dx, py + dy,
				sect->vertex[s + 0].x, sect->vertex[s + 0].y,
				sect->vertex[s + 1].x, sect->vertex[s + 1].y)
			&& PointSide(px + dx, py + dy,
				sect->vertex[s + 0].x, sect->vertex[s + 0].y,
				sect->vertex[s + 1].x, sect->vertex[s + 1].y) < 0)
		{
			player.sector = sect->neighbors[s];
//			printf("Player is now in sector %d\n", player.sector);
			break;
		}

	player.where.x += dx;
	player.where.y += dy;
	player.anglesin = sinf(player.angle);
	player.anglecos = cosf(player.angle);
}

static void __not_in_flash_func(DrawScreen)(void)
{
	struct item { short sectorno, sx1, sx2; } queue[MaxQueue], *head = queue, *tail = queue;
	short ytop[W] = { 0 }, ybottom[W], *renderedsectors;
	renderedsectors = malloc(NumSectors * sizeof(short));

	for (unsigned x = 0; x<W; ++x) ybottom[x] = H - 1;
	for (unsigned n = 0; n<NumSectors; ++n) renderedsectors[n] = 0;

	/*Begin whole-screen rendering from where the player is. */
	*head = (struct item) { player.sector, 0, W - 1 };
	if (++head == queue + MaxQueue) head = queue;

	while (head != tail)
	{
		/* Pick a sector & slice from queue to draw */
		const struct item now = *tail;
		if (++tail == queue + MaxQueue) tail = queue;

		if (renderedsectors[now.sectorno] & 0x21) continue; // Odd = still rendering, 0x20 = give up
		++renderedsectors[now.sectorno];
		/* Render each wall of this sector that is facing towards player. */
		const struct sector* const sect = &sectors[now.sectorno];

		/* This loop can be used to illustrate currently rendering window. Should be disabled otherwise. */
		//for(unsigned x=now.sx1; x<=now.sx2; ++x)
		//    vline(x, ytop[x], ybottom[x], 0x003300, 0x00AA00, 0x003300);

		for (int s = 0; s < sect->npoints; ++s)
		{
			/* Acquire the x,y coordinates of the two vertexes forming the edge of the sector */
			/* Transform the vertices into the player's view */
			float vx1 = sect->vertex[s + 0].x - player.where.x, vy1 = sect->vertex[s + 0].y - player.where.y;
			float vx2 = sect->vertex[s + 1].x - player.where.x, vy2 = sect->vertex[s + 1].y - player.where.y;
			/* Rotate them around the player's view */
			float pcos = player.anglecos, psin = player.anglesin;
			float tx1 = vx1 * psin - vy1 * pcos, tz1 = vx1 * pcos + vy1 * psin;
			float tx2 = vx2 * psin - vy2 * pcos, tz2 = vx2 * pcos + vy2 * psin;
			/* Is the wall at least partially in front of the player? */
			if (tz1 <= 0 && tz2 <= 0) continue;
			/* If it's partially behind the player, clip it against player's view frustrum */
			if (tz1 <= 0 || tz2 <= 0)
			{
				float nearz = 1e-4f, farz = 5, nearside = 1e-5f, farside = 20.f;
				// Find an intersetion between the wall and the approximate edges of player's view
				struct xy i1 = Intersect(tx1, tz1, tx2, tz2, -nearside, nearz, -farside, farz);
				struct xy i2 = Intersect(tx1, tz1, tx2, tz2, nearside, nearz, farside, farz);
				if (tz1 < nearz) { if (i1.y > 0) { tx1 = i1.x; tz1 = i1.y; } else { tx1 = i2.x; tz1 = i2.y; } }
				if (tz2 < nearz) { if (i1.y > 0) { tx2 = i1.x; tz2 = i1.y; } else { tx2 = i2.x; tz2 = i2.y; } }
			}

			// if(tz1 <= 0.07f) { tx1 = (0.07f-tz1) * (tx2-tx1) / (tz2-tz1) + tx1; tz1 = 0.07f; }
			// if(tz2 <= 0.07f) { tx2 = (0.07f-tz2) * (tx1-tx2) / (tz1-tz2) + tx2; tz2 = 0.07f; }
			/* Do perspective transformation */
			float xscale1 = (W*hfov) / (tz1), yscale1 = (H*vfov) / (tz1); int x1 = W / 2 + (int)(-tx1 * xscale1);
			float xscale2 = (W*hfov) / (tz2), yscale2 = (H*vfov) / (tz2); int x2 = W / 2 + (int)(-tx2 * xscale2);
			if (x1 >= x2) continue;
			if (x2 < now.sx1 || x1 > now.sx2) continue;
			/* Acquire and transform the floor and ceiling heights */
			float yceil = sect->ceil - player.where.z;
			float yfloor = sect->floor - player.where.z;
#define Yaw(y,z) (y + z*player.yaw)
			int y1a = H / 2 + (int)(-Yaw(yceil, tz1) * yscale1), y1b = H / 2 + (int)(-Yaw(yfloor, tz1) * yscale1);
			int y2a = H / 2 + (int)(-Yaw(yceil, tz2) * yscale2), y2b = H / 2 + (int)(-Yaw(yfloor, tz2) * yscale2);
			/* Check the edge type. neighbor=-1 means wall, other=boundary between two sectors. */
			int neighbor = sect->neighbors[s];
			float nyceil = 0, nyfloor = 0;

			if (neighbor >= 0)
			{
				/* Something is showing through this wall (portal). */
				/* Perspective-transform the floor and ceiling coordinates of the neighboring sector. */
				nyceil = sectors[neighbor].ceil - player.where.z;
				nyfloor = sectors[neighbor].floor - player.where.z;
			}
			int ny1a = H / 2 + (int)(-Yaw(nyceil, tz1) * yscale1), ny1b = H / 2 + (int)(-Yaw(nyfloor, tz1) * yscale1);
			int ny2a = H / 2 + (int)(-Yaw(nyceil, tz2) * yscale2), ny2b = H / 2 + (int)(-Yaw(nyfloor, tz2) * yscale2);

			/* Render the wall. */
			int beginx = max(x1, now.sx1), endx = min(x2, now.sx2);

			struct Scaler ya_int = Scaler_Init(x1, beginx, x2, y1a, y2a);
			struct Scaler yb_int = Scaler_Init(x1, beginx, x2, y1b, y2b);
			struct Scaler nya_int = Scaler_Init(x1, beginx, x2, ny1a, ny2a);
			struct Scaler nyb_int = Scaler_Init(x1, beginx, x2, ny1b, ny2b);

			for (int x = beginx; x <= endx; ++x)
			{
				// Affine calculation of txtx would be:
				//    alpha : (x-x1) / (x2-x1)
				//    u0    : 0;
				//    u1    : 1023;
				//    txtx  : u0 + u1 * alpha;
				// Perspective-corrected calculation of txtx (from Wikipedia):
				//    txtx  : ((1-alpha) * (u0 / z0) + alpha * (u1 / z1))
				//          / (((1-alpha) / z0) + alpha/z1);
				// Unrolled for only one division with Maxima:
				//    txtx  : u0*z1*(x2-x) + u1*z0*(x-x1)
				//          / ((x2-x)*z1 + (x-x1)*z0);
				/* Acquire the Y coordinates for our floor & ceiling for this X coordinate */
				int ya = Scaler_Next(&ya_int);
				int yb = Scaler_Next(&yb_int);
				/* Clamp the ya & yb */
				int cya = clamp(ya, ytop[x], ybottom[x]);
				int cyb = clamp(yb, ytop[x], ybottom[x]);

				// Our perspective calculation produces these two:
				//     screenX = W/2 + -mapX              * (W*hfov) / mapZ
				//     screenY = H/2 + -(mapY + mapZ*yaw) * (H*vfov) / mapZ
				// To translate these coordinates back into mapX, mapY and mapZ...
				//
				// Solving for Z, when we know Y (ceiling height):
				//     screenY - H/2  = -(mapY + mapZ*yaw) * (H*vfov) / mapZ
				//     (screenY - H/2) / (H*vfov) = -(mapY + mapZ*yaw) / mapZ
				//     (H/2 - screenY) / (H*vfov) = (mapY + mapZ*yaw) / mapZ
				//     mapZ = mapY / ((H/2 - screenY) / (H*vfov) - yaw)
				//     mapZ = mapY*H*vfov / (H/2 - screenY - yaw*H*vfov)
				// Solving for X, when we know Z
				//     mapX = mapZ*(W/2 - screenX) / (W*hfov)
				//
				// This calculation is used for visibility tracking
				//   (the visibility cones in the map)
				// and for floor & ceiling texture mapping.
				//
#define CeilingFloorScreenCoordinatesToMapCoordinates(mapY, screenX,screenY, X,Z) \
                    do { Z = (mapY)*H*vfov / ((H/2 - (screenY)) - player.yaw*H*vfov); \
                         X = (Z) * (W/2 - (screenX)) / (W*hfov); \
                         RelativeMapCoordinatesToAbsoluteOnes(X,Z); } while(0)
				//
#define RelativeMapCoordinatesToAbsoluteOnes(X,Z) \
                    do { float rtx = (Z) * pcos + (X) * psin; \
                         float rtz = (Z) * psin - (X) * pcos; \
                         X = rtx + player.where.x; Z = rtz + player.where.y; \
                    } while(0)

				/* Render ceiling: everything above this sector's ceiling height. */
				uint8_t black = BLACK;
				uint8_t ceiling = DARK_RED;
//				vline(x, ytop[x], cya - 1, al_map_rgb(0x11, 0x11, 0x11), al_map_rgb(0x22, 0x22, 0x22), al_map_rgb(0x11, 0x11, 0x11));
				vline(x, ytop[x], cya - 1, black, ceiling, ceiling);
				/* Render floor: everything below this sector's floor height. */
				uint8_t bottom = MID_BLUE;
//				vline(x, cyb + 1, ybottom[x], LIGHT_BLUE, MID_BLUE, LIGHT_BLUE);
				vline(x, cyb + 1, ybottom[x], black, bottom, bottom);

				/* Is there another sector behind this edge? */
				if (neighbor >= 0)
				{
					/* Same for _their_ floor and ceiling */
					int nya = Scaler_Next(&nya_int);
					int nyb = Scaler_Next(&nyb_int);
					/* Clamp ya2 & yb2 */
					int cnya = clamp(nya, ytop[x], ybottom[x]);
					int cnyb = clamp(nyb, ytop[x], ybottom[x]);
					/* If our ceiling is higher than their ceiling, render upper wall */
					uint8_t r1 = LIGHT_GREY, r2 = MID_PURPLE;
					uint8_t black = BLACK;
					vline(x, cya, cnya - 1, black, x == x1 || x == x2 ? black : r1, black);
					ytop[x] = clamp(max(cya, cnya), ytop[x], H - 1);

					// If our floor is lower than their floor, render bottom wall
					vline(x, cnyb + 1, cyb, black, x == x1 || x == x2 ? black : r2, black);
					ybottom[x] = clamp(min(cyb, cnyb), 0, ybottom[x]);

					/* These vlines can be used to illustrate the frame being rendered. */
					/* They should be disabled otherwise. */
					//vline(x, ytop[x],ybottom[x], 0x330000,0xAA0000,0x330000);
					//vline(x, cya,cyb, 0x330000,0xAA0000,0x330000);
				}
				else
				{
					/* There's no neighbor. Render wall. */
					uint8_t r = LIGHT_GREY;
					uint8_t black = BLACK;
					vline(x, cya, cyb, black, x == x1 || x == x2 ? black : r, black);
				}
			}
			/* Schedule the other sector for rendering within the window formed by this wall. */
			if (neighbor >= 0 && endx >= beginx && (head + MaxQueue + 1 - tail) % MaxQueue)
			{
				*head = (struct item) { neighbor, beginx, endx };
				if (++head == queue + MaxQueue) head = queue;
				//if(tail-- == queue) tail = queue+MaxQueue-1;
				//*tail = (struct item) { neighbor, beginx, endx };
			}
		}
		++renderedsectors[now.sectorno];
	}
}

void __not_in_flash_func(initRender3D)()
{
	LoadData();
	//VerifyMap();
}

void __not_in_flash_func(refreshRender3D)(void) {
	// act on keyboard input
	if (movement_key[KEY_SPACE] && ground)
	{
		player.velocity.z += 0.5;
		falling = 1;
	}
	ducking = movement_key[KEY_Z];
	map = movement_key[KEY_TAB];

	if (movement_key[KEY_RIGHT])
	{
		player.angle += TurnAngle;
		if (player.angle > MaxAngle) player.angle -= MaxAngle;
	}

	if (movement_key[KEY_LEFT])
	{
		player.angle -= TurnAngle;
		if (player.angle < 0.f) player.angle += MaxAngle;
	}

	if (movement_key[KEY_UP]) player.yaw += 0.25f;
	if (movement_key[KEY_DOWN]) player.yaw -= 0.25f;
	player.yaw = clamp(player.yaw, -3.f, 3.f);

	float move_vec[2] = { 0.f, 0.f };
	if (movement_key[KEY_W]) { move_vec[0] += player.anglecos*0.2f; move_vec[1] += player.anglesin*0.2f; }
	if (movement_key[KEY_S]) { move_vec[0] -= player.anglecos*0.2f; move_vec[1] -= player.anglesin*0.2f; }
	if (movement_key[KEY_A]) { move_vec[0] += player.anglesin*0.2f; move_vec[1] -= player.anglecos*0.2f; }
	if (movement_key[KEY_D]) { move_vec[0] -= player.anglesin*0.2f; move_vec[1] += player.anglecos*0.2f; }
	uint8_t pushing = movement_key[KEY_W] || movement_key[KEY_S] || movement_key[KEY_A] || movement_key[KEY_D]
		|| movement_key[KEY_LEFT] || movement_key[KEY_RIGHT] || movement_key[KEY_UP] || movement_key[KEY_DOWN];
	float acceleration = pushing ? 0.4 : 0.2;

	player.velocity.x = player.velocity.x * (1 - acceleration) + move_vec[0] * acceleration;
	player.velocity.y = player.velocity.y * (1 - acceleration) + move_vec[1] * acceleration;

	//fprintf(fp, "%g %g %d %d %d %d %d %g %g\n", player.velocity.x, player.velocity.y, pushing, ducking, falling, moving, ground, player.angle, yaw); fflush(fp);
	//fscanf(fp, "%g %g %d %d %d %d %d %g %g\n", &player.velocity.x, &player.velocity.y, &pushing,&ducking,&falling,&moving,&ground, &player.angle, &yaw); MovePlayer(0,0);

	if (pushing) moving = 1;

	if (map) DrawMap();
	else DrawScreen();

	/* Vertical collision detection */
	float eyeheight = ducking ? DuckHeight : EyeHeight;
	ground = (falling == 0);
	if (falling)
	{
		player.velocity.z -= 0.05f; /* Add gravity */
		float nextz = player.where.z + player.velocity.z;
		if (player.velocity.z < 0 && nextz  < sectors[player.sector].floor + eyeheight)
		{
			/* Fix to ground */
			player.where.z = sectors[player.sector].floor + eyeheight;
			player.velocity.z = 0;
			falling = 0;
			ground = 1;
		}
		else if (player.velocity.z > 0 && nextz > sectors[player.sector].ceil)
		{
			/* Prevent jumping above ceiling */
			player.velocity.z = 0;
			falling = 1;
		}
		if (falling)
		{
			player.where.z += player.velocity.z;
			moving = 1;
		}
	}
	/* Horizontal collision detection */
	if (moving)
	{
		float px = player.where.x, py = player.where.y;
		float dx = player.velocity.x, dy = player.velocity.y;

		const struct sector* const sect = &sectors[player.sector];
		/* Check if the player is about to cross one of the sector's edges */
		for (int s = 0; s < sect->npoints; ++s)
			if (IntersectBox(px, py, px + dx, py + dy,
				sect->vertex[s + 0].x, sect->vertex[s + 0].y,
				sect->vertex[s + 1].x, sect->vertex[s + 1].y)
				&& PointSide(px + dx, py + dy,
					sect->vertex[s + 0].x, sect->vertex[s + 0].y,
					sect->vertex[s + 1].x, sect->vertex[s + 1].y) < 0)
			{
				float hole_low = 9e9, hole_high = -9e9;
				if (sect->neighbors[s] >= 0)
				{
					/* Check where the hole is. */
					hole_low = max(sect->floor, sectors[sect->neighbors[s]].floor);
					hole_high = min(sect->ceil, sectors[sect->neighbors[s]].ceil);
				}
				/* Check whether we're bumping into a wall. */
				if (hole_high < player.where.z + HeadMargin
					|| hole_low  > player.where.z - eyeheight + KneeHeight)
				{
					/* Bumps into a wall! Slide along the wall. */
					/* This formula is from Wikipedia article "vector projection". */
					float xd = sect->vertex[s + 1].x - sect->vertex[s + 0].x;
					float yd = sect->vertex[s + 1].y - sect->vertex[s + 0].y;
					player.velocity.x = xd * (dx*xd + dy*yd) / (xd*xd + yd*yd);
					player.velocity.y = yd * (dx*xd + dy*yd) / (xd*xd + yd*yd);
					moving = 0;
				}
			}
		MovePlayer(player.velocity.x, player.velocity.y);
		falling = 1;
	}
}
