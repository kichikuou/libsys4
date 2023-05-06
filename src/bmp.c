/* Copyright (C) 2021 kichikuou <KichikuouChrome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include "system4.h"
#include "system4/cg.h"
#include "system4/bmp.h"

#define STBI_NO_STDIO
#define STBI_ONLY_BMP
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool bmp_checkfmt(const uint8_t *data)
{
	return (data[0] == 'B' && data[1] == 'M');
}

static void bmp_init_metrics(int w, int h, int ch, struct cg_metrics *dst)
{
	dst->w = w;
	dst->h = h;
	dst->bpp = ch * 8;
	dst->has_pixel = true;
	dst->has_alpha = ch >= 4;
	dst->pixel_pitch = w * ch;
	dst->alpha_pitch = 1;
}

bool bmp_get_metrics(const uint8_t *data, size_t size, struct cg_metrics *dst)
{
	int width, height, channels;
	if (!stbi_info_from_memory(data, size, &width, &height, &channels))
		return false;
	bmp_init_metrics(width, height, channels, dst);
	return true;
}

void bmp_extract(const uint8_t *data, size_t size, struct cg *cg)
{
	int width, height, channels;
	uint8_t *pixels = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
	if (!pixels) {
		WARNING("cannot decode image: %s", stbi_failure_reason());
		return;
	}
	cg->type = ALCG_BMP;
	cg->pixels = pixels;
	bmp_init_metrics(width, height, channels, &cg->metrics);
}
