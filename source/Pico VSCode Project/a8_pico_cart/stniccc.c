#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ff.h>
#include "cart_bus.h"
#include "cart_emu.h"
#include "stniccc.h"

#define SWIDTH 160
#define SHEIGHT 200

#define min(a,b) (((a) < (b)) ? (a) : (b)) // min: Choose smaller of two values.
#define max(a,b) (((a) > (b)) ? (a) : (b)) // max: Choose bigger of two values.

#define KEY_ESCAPE 32
#define KEY_SPACE 16

static uint8_t	*ScreenBuf;
static uint8_t	*BackBuf;
static uint8_t	*DummyBuf;
static int16_t	miny;
static int16_t	maxy;

static uint32_t	streampoi;
static uint32_t	pal[16];
static int32_t	linebuf[SHEIGHT][2];
static uint8_t	xpos_remap[256];
static uint8_t	palette_map[256];
static int16_t	frame;
static FATFS	FatFs;
static FIL		fil;

static uint8_t	cols_blue[4] = { 0x00, 0x86, 0x8A, 0x8E }; // Blue

static void __not_in_flash_func(LoadData)(void)
{
	if (f_mount(&FatFs, "", 1) == FR_OK) {
		if (f_open(&fil, "scene1.bin", FA_READ) == FR_OK) {
		}
	}

	for (int16_t i = 0; i < 256; i++)
	{
		xpos_remap[i] = (i * 5) / 8;
	}
}

static void __not_in_flash_func(SetupMap)(void)
{
	static uint8_t map[62][2] = {
		{ 3, 1 },
		{ 7, 1 },
		{ 18, 1 },
		{ 19, 1 },
		{ 25, 8 },
		{ 27, 1 },
		{ 28, 2 },
//		{ 28, 8 },
		{ 30, 1 },
		{ 38, 2 },
		{ 40, 2 },
		{ 46, 2 },
		{ 49, 2 },
		{ 51, 3 },
		{ 56, 2 },
		{ 59, 2 },
		{ 63, 2 },
		{ 66, 3 },
		{ 68, 2 },
		{ 72, 3 },
		{ 73, 4 },
		{ 75, 3 },
		{ 78, 3 },
		{ 82, 3 },
		{ 85, 4 },
		{ 89, 3 },
		{ 91, 3 },
		{ 94, 3 },
		{ 95, 2 },
		{ 101, 3 },
		{ 104, 4 },
		{ 105, 5 },
		{ 107, 4 },
		{ 111, 5 },
		{ 113, 4 },
		{ 113, 4 },
		{ 120, 5 },
		{ 123, 4 },
		{ 127, 4 },
		{ 132, 4 },
		{ 136, 5 },
//		{ 137, 6 },
		{ 137, 5 },
		{ 139, 5 },
		{ 142, 5 },
		{ 146, 5 },
		{ 152, 6 },
		{ 153, 6 },
		{ 156, 5 },
		{ 159, 5 },
		{ 161, 6 },
		{ 165, 6 },
		{ 168, 6 },
		{ 171, 7 },
		{ 175, 6 },
		{ 178, 6 },
		{ 187, 6 },
		{ 191, 6 },
		{ 194, 7 },
		{ 197, 7 },
		{ 199, 7 },
		{ 201, 7 },
		{ 210, 7 },
		{ 223, 7 }
	};
	for (int i = 0; i < 62; i++)
	{
		palette_map[map[i][0]] = map[i][1];
	}
}

static void __not_in_flash_func(SetColours)()
{
	cart_d5xx[0xD4] = cols_blue[1];
	cart_d5xx[0xD5] = cols_blue[2];
	cart_d5xx[0xD6] = cols_blue[3];
	cart_d5xx[0xD8] = cols_blue[0];
	// timer flag - changes to 'Z' when 1st pass has run
	cart_d5xx[0xDD] = 'T';
}

static void __not_in_flash_func(DoLine)(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t t;

	if (y1 == y2)
		return;

	x1 = xpos_remap[x1];
	x2 = xpos_remap[x2];

	int32_t lineoff = 0;
	if (y2 < y1)
	{
		t = x1; x1 = x2; x2 = t;
		t = y1; y1 = y2; y2 = t;
		lineoff = 1;
	}

	int32_t dx = x2 - x1;
	int32_t dy = y2 - y1;
	int32_t xstep = (dx << 8) / dy;

	x1 <<= 8;

	miny = min(miny, y1);
	maxy = max(maxy, y2);

	for (int32_t y = y1; y < y2; y++)
	{
		linebuf[y][lineoff] = x1 >> 8;
		x1 += xstep;
	}
}

static void __not_in_flash_func(FillPoly)(uint8_t co)
{
	static uint8_t pal1[10] = { 0, 0, 1, 1, 1, 2, 2, 3, 0, 0 };
	static uint8_t pal2[10] = { 0, 1, 1, 2, 3, 2, 3, 3, 2, 3 };
	static uint8_t a8[4] = { 0x00, 0x55, 0xAA, 0xFF };

	uint32_t col1 = pal[co];

	int16_t r = (col1 >> 16) & 0xff;
	int16_t g = (col1 >> 8) & 0xff;
	int16_t b = (col1) & 0xff;
	int16_t br = (r * 30) / 100 + (g * 59) / 100 + (b * 11) / 100; // was (r + g + b) / 3;

	uint8_t coint = palette_map[br];
	if (br == 28 && r == 96)
	{
		if (frame < 92)
			coint = 1;
		else
			coint = 8;
	}
	else if (frame < 92 && br == 85) coint--;
	else if (br == 137 && b == 224) coint++;
	else if (frame < 1430 && br == 171) coint--;
	else if (frame >= 1430 && br == 94) coint = 9;
	else if (br != 0 && coint == 0)
	{
		coint = 7;
	}

	for (uint8_t y = miny; y < maxy; y++)
	{
		uint16_t x1 = linebuf[y][0];
		uint16_t x2 = linebuf[y][1];

		uint8_t *offset = BackBuf + ((uint16_t)y * (SWIDTH / 4)) + (x1 / 4);

		for (uint16_t x = x1; x < x2; x++)
		{
			uint8_t c = (y & 1)
				? ((x & 1) ? pal1[coint] : pal2[coint])
				: ((x & 1) ? pal2[coint] : pal1[coint]);
			uint8_t z = (0xC0 >> ((x & 3) << 1));
			uint8_t m = 0xFF ^ z;
			*offset = (*offset & m) | (a8[c & 3] & z);
			if (z == 3) offset++;
		}
	}
}

static uint8_t __not_in_flash_func(ReadByte)(void)
{
	uint8_t c;
	UINT rc;
	f_read(&fil, &c, 1, &rc);
	streampoi++;
	return c;
}

static uint16_t __not_in_flash_func(ReadShort)(void)
{
	uint8_t va1 = ReadByte();
	uint8_t va2 = ReadByte();
	uint16_t value = (va1 << 8) | (va2);

	return value;
}

static void __not_in_flash_func(drawStniccc)()
{
	uint8_t flags = ReadByte();

	int16_t doclear = 1;
	int16_t haspal = 0;
	int16_t isindexed = 0;
	int16_t isempty = 0;

	if (flags & 1)
		doclear = 0;
	if (flags & 2)
		haspal = 1;
	if (flags & 4)
		isindexed = 1;
	if (flags & 8)
		isempty = 1;

	memset(BackBuf, 0, (SWIDTH / 4) * SHEIGHT);

	if (haspal)
	{
		uint16_t palmask = ReadShort();
		for (int16_t i = 0; i < 16; i++)
		{
			if (((palmask >> (15 - i)) & 1) == 1)
			{
				uint16_t col = ReadShort();
				int16_t r = (col >> 8) & 7;
				int16_t g = (col >> 4) & 7;
				int16_t b = (col) & 7;
				uint32_t col2 = (r << 21) | (g << 13) | (b << 5);
				pal[i] = col2;
			}
		}
	}

	if (isindexed)
	{
		uint8_t numframeverts = ReadByte();
		uint8_t vertxs[256];
		uint8_t vertys[256];

		for (int16_t i = 0; i < numframeverts; i++)
		{
			vertxs[i] = ReadByte();
			vertys[i] = ReadByte();
		}

		while (1)
		{
			uint8_t polytype = ReadByte();
			uint8_t co = polytype >> 4;
			uint8_t vertcount = polytype & 0x0f;

			if (polytype == 0xff)
				break;
			if (polytype == 0xfe)
			{
				UINT rc;
				uint32_t newpoi = (streampoi + 0xffff) & 0xffff0000;
				f_read(&fil, DummyBuf, newpoi - streampoi, &rc);
				streampoi = newpoi;
				break;
			}
			if (polytype == 0xfd)
			{
				streampoi = 0;
				frame = 0;
				f_close(&fil);
				f_open(&fil, "scene1.bin", FA_READ);
				cart_d5xx[0xDD] = 'Z';
				break;
			}

			int16_t id1 = 0;
			int16_t id3 = 0;

			miny = SHEIGHT;
			maxy = 0;

			for (int16_t i = 0; i < vertcount; i++)
			{
				int16_t id2 = ReadByte();
				if (i > 0)
				{
					DoLine(vertxs[id1], vertys[id1], vertxs[id2], vertys[id2]);
				}
				else
				{
					id3 = id2;
				}

				id1 = id2;
			}

			DoLine(vertxs[id1], vertys[id1], vertxs[id3], vertys[id3]);

			FillPoly(co);
		}
	}
	else
	{
		while (1)
		{
			uint8_t polytype = ReadByte();
			uint8_t co = polytype >> 4;
			uint8_t vertcount = polytype & 0x0f;

			if (polytype == 0xff)
				break;
			if (polytype == 0xfe)
			{
				UINT rc;
				uint32_t newpoi = (streampoi + 0xffff) & 0xffff0000;
				f_read(&fil, DummyBuf, newpoi - streampoi, &rc);
				streampoi = newpoi;
				break;
			}
			if (polytype == 0xfd)
			{
				streampoi = 0;
				frame = 0;
				f_close(&fil);
				f_open(&fil, "scene1.bin", FA_READ);
				cart_d5xx[0xDD] = 'Z';
				break;
			}

			int16_t x1 = 0;
			int16_t y1 = 0;
			int16_t x3 = 0;
			int16_t y3 = 0;

			miny = SHEIGHT;
			maxy = 0;

			for (int16_t i = 0; i < vertcount; i++)
			{
				int16_t x2 = ReadByte();
				int16_t y2 = ReadByte();
				if (i > 0)
				{
					DoLine(x1, y1, x2, y2);
				}
				else
				{
					x3 = x2;
					y3 = y2;
				}

				x1 = x2;
				y1 = y2;
			}

			DoLine(x1, y1, x3, y3);

			FillPoly(co);
		}
	}
}

void __not_in_flash_func(initStniccc)()
{
	ScreenBuf = ScreenRAM + 0x10; // helps align to 4K boundary
	BackBuf = &cart_ram[0x4000];
	DummyBuf = &cart_ram[0x6000];

	frame = 0;
	streampoi = 0;

	memset(pal, 0, sizeof(pal));
	memset(palette_map, 0, sizeof(palette_map));
	memset(linebuf, 0, sizeof(linebuf));

	LoadData();
	SetupMap();
	SetColours();
}

void __not_in_flash_func(refreshStniccc)(void) {
	drawStniccc();
	memcpy(ScreenBuf, BackBuf, (SWIDTH / 4) * SHEIGHT);
	frame++;
}
