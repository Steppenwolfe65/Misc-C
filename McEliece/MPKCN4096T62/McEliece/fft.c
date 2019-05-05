#include "fft.h"
#include "vec.h"

static void radix_conversions(uint64_t* in) 
{
	int32_t i;
	int32_t j;
	int32_t k;

	const uint64_t mask[5][2] =
	{
		{ 0x8888888888888888ULL, 0x4444444444444444ULL },
		{ 0xC0C0C0C0C0C0C0C0ULL, 0x3030303030303030ULL },
		{ 0xF000F000F000F000ULL, 0x0F000F000F000F00ULL },
		{ 0xFF000000FF000000ULL, 0x00FF000000FF0000ULL },
		{ 0xFFFF000000000000ULL, 0x0000FFFF00000000ULL }
	};

	const uint64_t s[5][MCELIECE_GFBITS] = 
	{
		{
			0xF3CFC030FC30F003ULL, 0x3FCF0F003C00C00CULL, 0x30033CC300C0C03CULL, 0xCCFF0F3C0F30F0C0ULL,
			0x0300C03FF303C3F0ULL, 0x3FFF3C0FF0CCCCC0ULL, 0xF3FFF0C00F3C3CC0ULL, 0x3003333FFFC3C000ULL,
			0x0FF30FFFC3FFF300ULL, 0xFFC0F300F0F0CC00ULL, 0xC0CFF3FCCC3CFC00ULL, 0xFC3C03F0F330C000ULL,
		},
		{
			0x000F00000000F00FULL, 0x00000F00F00000F0ULL, 0x0F00000F00000F00ULL, 0xF00F00F00F000000ULL,
			0x00F00000000000F0ULL, 0x0000000F00000000ULL, 0xF00000000F00F000ULL, 0x00F00F00000F0000ULL,
			0x0000F00000F00F00ULL, 0x000F00F00F00F000ULL, 0x00F00F0000000000ULL, 0x0000000000F00000ULL,
		},
		{
			0x0000FF00FF0000FFULL, 0x0000FF000000FF00ULL, 0xFF0000FF00FF0000ULL, 0xFFFF0000FF000000ULL,
			0x00FF00FF00FF0000ULL, 0x0000FFFFFF000000ULL, 0x00FFFF00FF000000ULL, 0xFFFFFF0000FF0000ULL,
			0xFFFF00FFFF00FF00ULL, 0x0000FF0000000000ULL, 0xFFFFFF00FF000000ULL, 0x00FF000000000000ULL,
		},
		{
			0x000000000000FFFFULL, 0x00000000FFFF0000ULL, 0x0000000000000000ULL, 0xFFFF000000000000ULL,
			0x00000000FFFF0000ULL, 0x0000FFFF00000000ULL, 0x0000000000000000ULL, 0x00000000FFFF0000ULL,
			0x0000FFFF00000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
		},
		{
			0x00000000FFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFFFFFF00000000ULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0xFFFFFFFF00000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
		}
	};

	for (i = 0; i <= 4; i++) 
	{
		for (j = 0; j < MCELIECE_GFBITS; j++)
		{
			for (k = 4; k >= i; k--)
			{
				in[j] ^= (in[j] & mask[k][0]) >> (1 << k);
				in[j] ^= (in[j] & mask[k][1]) >> (1 << k);
			}
		}

		/* scaling */
		vec_mul(in, in, s[i]);
	}
}

static void butterflies(uint64_t out[][MCELIECE_GFBITS], uint64_t* in) 
{
	uint64_t tmp[MCELIECE_GFBITS];
	uint64_t ctr;
	size_t i;
	size_t j;
	size_t k;
	int32_t s;
	int32_t b;

	const uint64_t consts[63][MCELIECE_GFBITS] = 
	{
		{
			0xF00F0FF0F00F0FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0x0FF00FF00FF00FF0ULL, 0xAA5555AAAA5555AAULL,
			0xF00F0FF0F00F0FF0ULL, 0x33CCCC33CC3333CCULL, 0xFFFF0000FFFF0000ULL, 0xCC33CC3333CC33CCULL,
			0x33CC33CC33CC33CCULL, 0x5A5A5A5A5A5A5A5AULL, 0xFF00FF00FF00FF00ULL, 0xF00F0FF0F00F0FF0ULL,
		},
		{
			0x3C3C3C3C3C3C3C3CULL, 0xF0F0F0F0F0F0F0F0ULL, 0x5555AAAA5555AAAAULL, 0xCC3333CCCC3333CCULL,
			0xC33CC33CC33CC33CULL, 0x55555555AAAAAAAAULL, 0x33333333CCCCCCCCULL, 0x00FF00FFFF00FF00ULL,
			0xF0F0F0F0F0F0F0F0ULL, 0x0000000000000000ULL, 0x0000FFFFFFFF0000ULL, 0xF0F00F0F0F0FF0F0ULL,
		},
		{
			0x3C3C3C3C3C3C3C3CULL, 0x0F0F0F0F0F0F0F0FULL, 0xAAAA5555AAAA5555ULL, 0xCC3333CCCC3333CCULL,
			0xC33CC33CC33CC33CULL, 0x55555555AAAAAAAAULL, 0x33333333CCCCCCCCULL, 0xFF00FF0000FF00FFULL,
			0x0F0F0F0F0F0F0F0FULL, 0x0000000000000000ULL, 0x0000FFFFFFFF0000ULL, 0xF0F00F0F0F0FF0F0ULL,
		},
		{
			0xAA55AA5555AA55AAULL, 0xCC33CC3333CC33CCULL, 0x33CCCC33CC3333CCULL, 0x55555555AAAAAAAAULL,
			0xFF0000FF00FFFF00ULL, 0x3CC33CC3C33CC33CULL, 0x5555AAAA5555AAAAULL, 0x0FF00FF00FF00FF0ULL,
			0xCCCC33333333CCCCULL, 0xF0F0F0F0F0F0F0F0ULL, 0x00FFFF0000FFFF00ULL, 0xC33CC33CC33CC33CULL,
		},
		{
			0x55AA55AAAA55AA55ULL, 0xCC33CC3333CC33CCULL, 0xCC3333CC33CCCC33ULL, 0x55555555AAAAAAAAULL,
			0xFF0000FF00FFFF00ULL, 0xC33CC33C3CC33CC3ULL, 0xAAAA5555AAAA5555ULL, 0xF00FF00FF00FF00FULL,
			0x3333CCCCCCCC3333ULL, 0x0F0F0F0F0F0F0F0FULL, 0xFF0000FFFF0000FFULL, 0xC33CC33CC33CC33CULL,
		},
		{
			0xAA55AA5555AA55AAULL, 0x33CC33CCCC33CC33ULL, 0xCC3333CC33CCCC33ULL, 0x55555555AAAAAAAAULL,
			0x00FFFF00FF0000FFULL, 0x3CC33CC3C33CC33CULL, 0x5555AAAA5555AAAAULL, 0x0FF00FF00FF00FF0ULL,
			0x3333CCCCCCCC3333ULL, 0xF0F0F0F0F0F0F0F0ULL, 0x00FFFF0000FFFF00ULL, 0xC33CC33CC33CC33CULL,
		},
		{
			0x55AA55AAAA55AA55ULL, 0x33CC33CCCC33CC33ULL, 0x33CCCC33CC3333CCULL, 0x55555555AAAAAAAAULL,
			0x00FFFF00FF0000FFULL, 0xC33CC33C3CC33CC3ULL, 0xAAAA5555AAAA5555ULL, 0xF00FF00FF00FF00FULL,
			0xCCCC33333333CCCCULL, 0x0F0F0F0F0F0F0F0FULL, 0xFF0000FFFF0000FFULL, 0xC33CC33CC33CC33CULL,
		},
		{
			0x6699669999669966ULL, 0x33CCCC33CC3333CCULL, 0xA5A5A5A55A5A5A5AULL, 0x3C3CC3C3C3C33C3CULL,
			0xF00FF00F0FF00FF0ULL, 0x55AA55AA55AA55AAULL, 0x3C3CC3C3C3C33C3CULL, 0x0F0F0F0FF0F0F0F0ULL,
			0x55AA55AA55AA55AAULL, 0x33CCCC33CC3333CCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x9966996666996699ULL, 0x33CCCC33CC3333CCULL, 0xA5A5A5A55A5A5A5AULL, 0x3C3CC3C3C3C33C3CULL,
			0x0FF00FF0F00FF00FULL, 0xAA55AA55AA55AA55ULL, 0x3C3CC3C3C3C33C3CULL, 0xF0F0F0F00F0F0F0FULL,
			0xAA55AA55AA55AA55ULL, 0xCC3333CC33CCCC33ULL, 0x0F0F0F0F0F0F0F0FULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x6699669999669966ULL, 0x33CCCC33CC3333CCULL, 0x5A5A5A5AA5A5A5A5ULL, 0xC3C33C3C3C3CC3C3ULL,
			0x0FF00FF0F00FF00FULL, 0xAA55AA55AA55AA55ULL, 0xC3C33C3C3C3CC3C3ULL, 0x0F0F0F0FF0F0F0F0ULL,
			0xAA55AA55AA55AA55ULL, 0x33CCCC33CC3333CCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x9966996666996699ULL, 0x33CCCC33CC3333CCULL, 0x5A5A5A5AA5A5A5A5ULL, 0xC3C33C3C3C3CC3C3ULL,
			0xF00FF00F0FF00FF0ULL, 0x55AA55AA55AA55AAULL, 0xC3C33C3C3C3CC3C3ULL, 0xF0F0F0F00F0F0F0FULL,
			0x55AA55AA55AA55AAULL, 0xCC3333CC33CCCC33ULL, 0x0F0F0F0F0F0F0F0FULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x6699669999669966ULL, 0xCC3333CC33CCCC33ULL, 0x5A5A5A5AA5A5A5A5ULL, 0x3C3CC3C3C3C33C3CULL,
			0x0FF00FF0F00FF00FULL, 0x55AA55AA55AA55AAULL, 0x3C3CC3C3C3C33C3CULL, 0x0F0F0F0FF0F0F0F0ULL,
			0x55AA55AA55AA55AAULL, 0x33CCCC33CC3333CCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x9966996666996699ULL, 0xCC3333CC33CCCC33ULL, 0x5A5A5A5AA5A5A5A5ULL, 0x3C3CC3C3C3C33C3CULL,
			0xF00FF00F0FF00FF0ULL, 0xAA55AA55AA55AA55ULL, 0x3C3CC3C3C3C33C3CULL, 0xF0F0F0F00F0F0F0FULL,
			0xAA55AA55AA55AA55ULL, 0xCC3333CC33CCCC33ULL, 0x0F0F0F0F0F0F0F0FULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x6699669999669966ULL, 0xCC3333CC33CCCC33ULL, 0xA5A5A5A55A5A5A5AULL, 0xC3C33C3C3C3CC3C3ULL,
			0xF00FF00F0FF00FF0ULL, 0xAA55AA55AA55AA55ULL, 0xC3C33C3C3C3CC3C3ULL, 0x0F0F0F0FF0F0F0F0ULL,
			0xAA55AA55AA55AA55ULL, 0x33CCCC33CC3333CCULL, 0xF0F0F0F0F0F0F0F0ULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x9966996666996699ULL, 0xCC3333CC33CCCC33ULL, 0xA5A5A5A55A5A5A5AULL, 0xC3C33C3C3C3CC3C3ULL,
			0x0FF00FF0F00FF00FULL, 0x55AA55AA55AA55AAULL, 0xC3C33C3C3C3CC3C3ULL, 0xF0F0F0F00F0F0F0FULL,
			0x55AA55AA55AA55AAULL, 0xCC3333CC33CCCC33ULL, 0x0F0F0F0F0F0F0F0FULL, 0xA55A5AA55AA5A55AULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x6996699669966996ULL, 0x00FFFF0000FFFF00ULL,
			0xFF00FF00FF00FF00ULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x6996699669966996ULL, 0x00FFFF0000FFFF00ULL,
			0x00FF00FF00FF00FFULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x6996699669966996ULL, 0xFF0000FFFF0000FFULL,
			0x00FF00FF00FF00FFULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x6996699669966996ULL, 0xFF0000FFFF0000FFULL,
			0xFF00FF00FF00FF00ULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x9669966996699669ULL, 0xFF0000FFFF0000FFULL,
			0x00FF00FF00FF00FFULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x9669966996699669ULL, 0xFF0000FFFF0000FFULL,
			0xFF00FF00FF00FF00ULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x9669966996699669ULL, 0x00FFFF0000FFFF00ULL,
			0xFF00FF00FF00FF00ULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x6996699669966996ULL, 0x9669966996699669ULL, 0x00FFFF0000FFFF00ULL,
			0x00FF00FF00FF00FFULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x9669966996699669ULL, 0x00FFFF0000FFFF00ULL,
			0xFF00FF00FF00FF00ULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x9669966996699669ULL, 0x00FFFF0000FFFF00ULL,
			0x00FF00FF00FF00FFULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x9669966996699669ULL, 0xFF0000FFFF0000FFULL,
			0x00FF00FF00FF00FFULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x9669966996699669ULL, 0xFF0000FFFF0000FFULL,
			0xFF00FF00FF00FF00ULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x6996699669966996ULL, 0xFF0000FFFF0000FFULL,
			0x00FF00FF00FF00FFULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x6996699669966996ULL, 0xFF0000FFFF0000FFULL,
			0xFF00FF00FF00FF00ULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x6996699669966996ULL, 0x00FFFF0000FFFF00ULL,
			0xFF00FF00FF00FF00ULL, 0x0FF00FF0F00FF00FULL, 0x0F0FF0F0F0F00F0FULL, 0xC33C3CC33CC3C33CULL,
			0xC33C3CC33CC3C33CULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x9669699696696996ULL, 0x9669966996699669ULL, 0x6996699669966996ULL, 0x00FFFF0000FFFF00ULL,
			0x00FF00FF00FF00FFULL, 0xF00FF00F0FF00FF0ULL, 0xF0F00F0F0F0FF0F0ULL, 0x3CC3C33CC33C3CC3ULL,
			0x3CC3C33CC33C3CC3ULL, 0xA55A5AA55AA5A55AULL, 0xC33C3CC33CC3C33CULL, 0x3CC3C33C3CC3C33CULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		},
		{
			0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
			0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFF00000000ULL, 0xFFFF0000FFFF0000ULL,
			0xFF00FF00FF00FF00ULL, 0xF0F0F0F0F0F0F0F0ULL, 0xCCCCCCCCCCCCCCCCULL, 0xAAAAAAAAAAAAAAAAULL,
		}
	};

	const uint8_t reversal[64] = 
	{
		0x00, 0x20, 0x10, 0x30, 0x08, 0x28, 0x18, 0x38,
		0x04, 0x24, 0x14, 0x34, 0x0C, 0x2C, 0x1C, 0x3C,
		0x02, 0x22, 0x12, 0x32, 0x0A, 0x2A, 0x1A, 0x3A,
		0x06, 0x26, 0x16, 0x36, 0x0E, 0x2E, 0x1E, 0x3E,
		0x01, 0x21, 0x11, 0x31, 0x09, 0x29, 0x19, 0x39,
		0x05, 0x25, 0x15, 0x35, 0x0D, 0x2D, 0x1D, 0x3D,
		0x03, 0x23, 0x13, 0x33, 0x0B, 0x2B, 0x1B, 0x3B,
		0x07, 0x27, 0x17, 0x37, 0x0F, 0x2F, 0x1F, 0x3F
	};

	/* broadcast */
	for (j = 0; j < 64; j++)
	{
		for (i = 0; i < MCELIECE_GFBITS; i++)
		{
			out[j][i] = (in[i] >> reversal[j]) & 1;
			out[j][i] = ~out[j][i] + 1;
		}
	}

	ctr = 0;

	/* butterflies */
	for (i = 0; i <= 5; i++) 
	{
		s = 1 << i;

		for (j = 0; j < 64; j += 2 * s)
		{
			for (k = j; k < j + s; k++) 
			{
				vec_mul(tmp, out[k + s], consts[ctr + (k - j)]);

				for (b = 0; b < MCELIECE_GFBITS; b++)
				{
					out[k][b] ^= tmp[b];
				}
				for (b = 0; b < MCELIECE_GFBITS; b++)
				{
					out[k + s][b] ^= out[k][b];
				}
			}
		}

		ctr += ((uint64_t)1 << i);
	}
}

void fft(uint64_t out[][MCELIECE_GFBITS], uint64_t* in)
{
	radix_conversions(in);
	butterflies(out, in);
}