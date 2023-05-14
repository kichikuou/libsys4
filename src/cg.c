/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>
#include "system4.h"
#include "system4/archive.h"
#include "system4/cg.h"
#include "system4/file.h"
#include "system4/ajp.h"
#include "system4/dcf.h"
#include "system4/pcf.h"
#include "system4/pms.h"
#include "system4/qnt.h"
#include "system4/webp.h"

#define STBI_NO_STDIO
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static uint8_t* zlib_compress(uint8_t *data, int data_len, int *out_len, int quality)
{
	unsigned long buflen = compressBound(data_len);
	uint8_t* buf = xmalloc(buflen);
	if (!buf)
		return NULL;
	if (compress2(buf, &buflen, data, data_len, quality) != Z_OK) {
		free(buf);
		return NULL;
	}
	*out_len = buflen;
	return buf;
}

#define STBIW_ZLIB_COMPRESS zlib_compress
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const char *cg_file_extensions[_ALCG_NR_FORMATS] = {
	[ALCG_UNKNOWN] = "",
	[ALCG_QNT]     = "qnt",
	[ALCG_AJP]     = "ajp",
	[ALCG_PNG]     = "png",
	[ALCG_PMS8]    = "pms",
	[ALCG_PMS16]   = "pms",
	[ALCG_WEBP]    = "webp",
	[ALCG_DCF]     = "dcf",
	[ALCG_JPEG]    = "jpg",
	[ALCG_PCF]     = "pcf",
	[ALCG_BMP]     = "bmp",
};

static bool png_checkfmt(const uint8_t *data)
{
	const uint8_t png_magic_bytes[] = {
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
	};
	return !memcmp(data, png_magic_bytes, 8);
}

static bool jpeg_checkfmt(const uint8_t *data)
{
	return data[0] == 0xff && data[1] == 0xd8;
}

static bool bmp_checkfmt(const uint8_t *data)
{
	return (data[0] == 'B' && data[1] == 'M');
}

static void stbi_init_metrics(int w, int h, int ch, struct cg_metrics *dst)
{
	dst->w = w;
	dst->h = h;
	dst->bpp = ch * 8;
	dst->has_pixel = true;
	dst->has_alpha = ch >= 4;
	dst->pixel_pitch = w * ch;
	dst->alpha_pitch = 1;
}

static bool stbi_get_metrics(const uint8_t *data, size_t size, struct cg_metrics *dst)
{
	int width, height, channels;
	if (!stbi_info_from_memory(data, size, &width, &height, &channels))
		return false;
	stbi_init_metrics(width, height, channels, dst);
	return true;
}

static void stbi_extract(enum cg_type type, const uint8_t *data, size_t size, struct cg *cg)
{
	int width, height, channels;
	uint8_t *pixels = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
	if (!pixels) {
		WARNING("cannot decode image: %s", stbi_failure_reason());
		return;
	}
	cg->type = type;
	cg->pixels = pixels;
	stbi_init_metrics(width, height, channels, &cg->metrics);
}

/*
 * Identify cg format
 *   data: pointer to compressed data
 *   return: cg type
 */
enum cg_type cg_check_format(uint8_t *data)
{
	if (qnt_checkfmt(data)) {
		return ALCG_QNT;
	} else if (ajp_checkfmt(data)) {
		return ALCG_AJP;
	} else if (png_checkfmt(data)) {
		return ALCG_PNG;
	} else if (webp_checkfmt(data)) {
		return ALCG_WEBP;
	} else if (dcf_checkfmt(data)) {
		return ALCG_DCF;
	} else if (pms8_checkfmt(data)) {
		return ALCG_PMS8;
	} else if (pms16_checkfmt(data)) {
		return ALCG_PMS16;
	} else if (jpeg_checkfmt(data)) {
		return ALCG_JPEG;
	} else if (pcf_checkfmt(data)) {
		return ALCG_PCF;
	} else if (bmp_checkfmt(data)) {
		return ALCG_BMP;
	}
	return ALCG_UNKNOWN;
}

bool cg_get_metrics_internal(uint8_t *buf, size_t buf_size, struct cg_metrics *dst)
{
	switch (cg_check_format(buf)) {
	case ALCG_QNT:
		qnt_get_metrics(buf, dst);
		break;
	case ALCG_AJP:
		WARNING("AJP GetMetrics not implemented");
		return false;
	case ALCG_PNG:
	case ALCG_JPEG:
	case ALCG_BMP:
		stbi_get_metrics(buf, buf_size, dst);
		break;
	case ALCG_WEBP:
		webp_get_metrics(buf, buf_size, dst);
		break;
	case ALCG_DCF:
		dcf_get_metrics(buf, buf_size, dst);
		break;
	case ALCG_PMS8:
	case ALCG_PMS16:
		pms_get_metrics(buf, dst);
		break;
	case ALCG_PCF:
		pcf_get_metrics(buf, buf_size, dst);
		break;
	default:
		WARNING("Unknown CG type");
		return false;
	}
	return true;
}

bool cg_get_metrics_data(struct archive_data *dfile, struct cg_metrics *dst)
{
	return cg_get_metrics_internal(dfile->data, dfile->size, dst);
}

bool cg_get_metrics(struct archive *ar, int no, struct cg_metrics *dst)
{
	struct archive_data *dfile;

	if (!(dfile = archive_get(ar, no)))
		return false;

	bool r = cg_get_metrics_data(dfile, dst);
	archive_free_data(dfile);
	return r;
}

/*
 * Free CG data
 *  cg: object to free
 */
void cg_free(struct cg *cg)
{
	if (!cg)
		return;
	free(cg->pixels);
	free(cg);
}

static struct cg *cg_load_internal(uint8_t *buf, size_t buf_size, struct archive *ar)
{
	struct cg *cg = xcalloc(1, sizeof(struct cg));

	enum cg_type type = cg_check_format(buf);
	switch (type) {
	case ALCG_QNT:
		qnt_extract(buf, cg);
		break;
	case ALCG_AJP:
		ajp_extract(buf, buf_size, cg);
		break;
	case ALCG_PNG:
	case ALCG_JPEG:
	case ALCG_BMP:
		stbi_extract(type, buf, buf_size, cg);
		break;
	case ALCG_WEBP:
		webp_extract(buf, buf_size, cg, ar);
		break;
	case ALCG_DCF:
		dcf_extract(buf, buf_size, cg, ar);
		break;
	case ALCG_PMS8:
	case ALCG_PMS16:
		pms_extract(buf, buf_size, cg);
		break;
	case ALCG_PCF:
		pcf_extract(buf, buf_size, cg);
		break;
	default:
		WARNING("Unknown CG type");
		break;
	}

	if (cg->pixels)
		return cg;
	free(cg);
	return NULL;
}

struct cg *cg_load_data(struct archive_data *dfile)
{
	return cg_load_internal(dfile->data, dfile->size, dfile->archive);
}

/*
 * Load cg data from file or cache
 *  no: file no ( >= 0)
 *  return: cg object(extracted)
*/
struct cg *cg_load(struct archive *ar, int no)
{
	struct cg *cg;
	struct archive_data *dfile;

	if (!(dfile = archive_get(ar, no))) {
		WARNING("Failed to load CG %d", no);
		return NULL;
	}

	cg = cg_load_data(dfile);
	archive_free_data(dfile);
	return cg;
}

struct cg *cg_load_file(const char *filename)
{
	size_t buf_size;
	uint8_t *buf = file_read(filename, &buf_size);
	struct cg *cg = cg_load_internal(buf, buf_size, NULL);
	free(buf);
	return cg;
}

struct cg *cg_load_buffer(uint8_t *buf, size_t buf_size)
{
	return cg_load_internal(buf, buf_size, NULL);
}

static void write_callback(void *context, void *data, int size)
{
	fwrite(data, size, 1, (FILE*)context);
}

int cg_write(struct cg *cg, enum cg_type type, FILE *f)
{
	switch (type) {
	case ALCG_QNT:
		return qnt_write(cg, f);
	case ALCG_PNG:
		return stbi_write_png_to_func(write_callback, f, cg->metrics.w, cg->metrics.h, 4, cg->pixels, cg->metrics.w * 4);
	case ALCG_WEBP:
		return webp_write(cg, f);
	default:
		WARNING("encoding not supported for CG type");
	}
	return 0;
}
