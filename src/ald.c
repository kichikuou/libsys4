/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * Based on code from xsystem35
 * Copyright (C) 1997-1998 Masaki Chikama (Wren) <chikama@kasumi.ipl.mech.nagoya-u.ac.jp>
 *               1998-                           <masaki-c@is.aist-nara.ac.jp>
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "little_endian.h"
#include "system4.h"
#include "system4/ald.h"
#include "system4/file.h"

static bool ald_exists(struct archive *ar, int no);
static struct archive_data *ald_get(struct archive *ar, int no);
static struct archive_data *ald_get_by_name(struct archive *_ar, const char *name);
static bool ald_load_file(struct archive_data *data);
static struct archive_data *ald_copy_descriptor(struct archive_data *src);
static void ald_for_each(struct archive *_ar, void (*iter)(struct archive_data *data, void *user), void *user);
static void ald_free_data(struct archive_data *data);
static void ald_free(struct archive *ar);

struct archive_ops ald_archive_ops = {
	.exists = ald_exists,
	.get = ald_get,
	.get_by_name = ald_get_by_name,
	.load_file = ald_load_file,
	.release_file = NULL,
	.copy_descriptor = ald_copy_descriptor,
	.for_each = ald_for_each,
	.free_data = ald_free_data,
	.free = ald_free,
};

/* Get the size of a file in bytes. */
static long get_file_size(FILE *fp)
{
	fseek(fp, 0L, SEEK_END);
	return ftell(fp);
}

/* Check validity of file. */
static bool file_check(FILE *fp)
{
	uint8_t b[6];
	int mapsize, ptrsize;
	long filesize;

	// get filesize / 256
	filesize = (get_file_size(fp) + 255) >> 8;

	long fsize = get_file_size(fp);
	fseek(fp, fsize  & ~0xFF, SEEK_SET);
	fread(b, 1, 4, fp);

	// read top 6 bytes
	fseek(fp, 0L, SEEK_SET);
	fread(b, 1, 6, fp);

	// get ptrsize and mapsize
	ptrsize = LittleEndian_get3B(b, 0);
	mapsize = LittleEndian_get3B(b, 3) - ptrsize;

	// check that sizes are valid
	if (ptrsize < 0 || mapsize < 0)
		return false;
	if (ptrsize > filesize || mapsize > filesize)
		return false;
	return true;
}

/* Read the file map of an ALD file into a `struct ald_archive`. */
static void get_filemap(struct ald_archive *archive, FILE *fp)
{
	uint8_t b[6], *_b;
	int mapsize, ptrsize;

	// read top 6 bytes
	fseek(fp, 0L, SEEK_SET);
	fread(b, 1, 6, fp);

	// get ptrsize and mapsize
	ptrsize = LittleEndian_get3B(b, 0);
	mapsize = LittleEndian_get3B(b, 3) - ptrsize;

	// allocate read buffer
	_b = malloc(mapsize * 256);

	// read filemap
	fseek(fp, ptrsize * 256L, SEEK_SET);
	fread(_b, 256, mapsize, fp);

	// get max file number from mapdata
	archive->maxfile = (mapsize * 256) / 3;

	// map of disk
	archive->map_disk = malloc(archive->maxfile);
	archive->map_ptr = malloc(sizeof(short) * archive->maxfile);

	for (int i = 0; i < archive->maxfile; i++) {
		archive->map_disk[i] = _b[i * 3] - 1;
		archive->map_ptr[i] = LittleEndian_getW(_b, i * 3 + 1) - 1;
	}

	free(_b);
	return;
}

/* Read the pointer map of an ALD file into a `struct ald_archive`. */
static void get_ptrmap(struct ald_archive *archive, FILE *fp, int disk)
{
	uint8_t b[8], *_b;
	int ptrsize, filecnt;

	// read top 6 bytes
	fseek(fp, 0L, SEEK_SET);
	fread(b, 1, 6, fp);

	// get ptrmap size
	ptrsize = LittleEndian_get3B(b, 0);

	// estimate number of entries in ptrmap
	filecnt = (ptrsize * 256) / 3;

	// allocate read buffer
	_b = malloc(ptrsize * 256);

	// read pointers
	fseek(fp, 0L, SEEK_SET);
	fread(_b, 256, ptrsize, fp);

	// allocate pointers buffer
	archive->fileptr[disk] = calloc(filecnt, sizeof(int));

	// store pointers
	for (int i = 0; i < filecnt - 1; i++) {
		*(archive->fileptr[disk] + i) = (LittleEndian_get3B(_b, i * 3 + 3) * 256);
	}

	free(_b);
	return;
}

static int _ald_get(struct ald_archive *ar, int no, int *disk_out, int *dataptr_out)
{
	int disk, ptr, dataptr, dataptr2;

	// check that index is within range of file map
	if (no < 0 || no >= ar->maxfile)
		return 0;

	// look up data in file map
	disk = ar->map_disk[no];
	ptr  = ar->map_ptr[no];
	if (disk < 0 || ptr < 0)
		return 0;

	// no file registered
	if (ar->fileptr[disk] == NULL)
		return 0;

	// get pointer in file and size
	dataptr = *(ar->fileptr[disk] + ptr);
	dataptr2 = *(ar->fileptr[disk] + ptr + 1);
	if (!dataptr || !dataptr2)
		return 0;

	*dataptr_out = dataptr;
	*disk_out = disk;
	return dataptr2 - dataptr;
}

static bool ald_exists(struct archive *ar, int no)
{
	int disk, dataptr;
	return ar && !!_ald_get((struct ald_archive*)ar, no, &disk, &dataptr);
}

/* Get a descriptor for a file in an ALD archive. */
struct archive_data *ald_get_descriptor(struct archive *_ar, int no)
{
	if (!_ar)
		return NULL;

	struct ald_archive *ar = (struct ald_archive*)_ar;
	struct ald_archive_data *dfile = calloc(1, sizeof(struct ald_archive_data));

	if (!_ald_get(ar, no, &dfile->disk, &dfile->dataptr)) {
		free(dfile);
		return NULL;
	}

	// read header
	if (ar->ar.mmapped) {
		uint8_t *hdr  = ar->files[dfile->disk].data + dfile->dataptr;
		dfile->hdr_size = LittleEndian_getDW(hdr, 0);
		dfile->data.size = LittleEndian_getDW(hdr, 4);
		dfile->data.name = ar->conv((char*)hdr + 16);
	} else {
		uint8_t *hdr = xmalloc(16);
		FILE *fp = ar->files[dfile->disk].fp;

		// read header size, file size
		fseek(fp, dfile->dataptr, SEEK_SET);
		fread(hdr, 16, 1, fp);
		dfile->hdr_size = LittleEndian_getDW(hdr, 0);
		dfile->data.size = LittleEndian_getDW(hdr, 4);

		// read name
		dfile->data.name = xcalloc(dfile->hdr_size-16, 1);
		fread(dfile->data.name, dfile->hdr_size-16, 1, fp);

		free(hdr);
	}

	dfile->data.no = no;
	dfile->data.archive = &ar->ar;
	return &dfile->data;
}

/* Get a file from an ALD archive. */
struct archive_data *ald_get(struct archive *ar, int no)
{
	if (!ar)
		return NULL;

	struct archive_data *data = ald_get_descriptor(ar, no);
	if (!data)
		return NULL;

	if (!ald_load_file(data)) {
		ald_free_data(data);
		return NULL;
	}

	return data;
}

static struct archive_data *ald_get_by_name(struct archive *_ar, const char *name)
{
	struct ald_archive *ar = (struct ald_archive*)_ar;
	for (int i = 0; i < ar->maxfile; i++) {
		struct archive_data *data = ald_get_descriptor(_ar, i);
		if (!data)
			continue;
		if (!strcmp(data->name, name)) {
			ald_load_file(data);
			return data;
		}
		ald_free_data(data);
	}
	return NULL;
}

static bool ald_load_file(struct archive_data *data)
{
	struct ald_archive *ar = (struct ald_archive*)data->archive;
	struct ald_archive_data *dfile = (struct ald_archive_data*)data;
	if (data->archive->mmapped) {
		data->data = ar->files[dfile->disk].data + dfile->dataptr + dfile->hdr_size;
	} else {
		FILE *fp = ar->files[dfile->disk].fp;
		data->data = xmalloc(data->size);

		fseek(fp, dfile->dataptr + dfile->hdr_size, SEEK_SET);
		fread(data->data, data->size, 1, fp);
	}
	return data;
}

static struct archive_data *ald_copy_descriptor(struct archive_data *_src)
{
	struct ald_archive_data *src = (struct ald_archive_data*)_src;
	struct ald_archive_data *dst = xmalloc(sizeof(struct ald_archive_data));
	_archive_copy_descriptor_ip(&dst->data, &src->data);
	dst->disk = src->disk;
	dst->dataptr = src->dataptr;
	dst->hdr_size = src->hdr_size;
	return &dst->data;
}

static void ald_for_each(struct archive *_ar, void (*iter)(struct archive_data *data, void *user), void *user)
{
	struct ald_archive *ar = (struct ald_archive*)_ar;
	for (int i = 0; i < ar->maxfile; i++) {
		struct archive_data *data = ald_get_descriptor(_ar, i);
		if (!data)
			continue;
		iter(data, user);
		ald_free_data(data);
	}
}

/* Free an ald_data strcture returned by `ald_get`. */
static void ald_free_data(struct archive_data *data)
{
	if (!data)
		return;
	if (!data->archive->mmapped && data->data)
		free(data->data);
	free(data->name);
	free(data);
}

/* Free an ald_archive structure returned by `ald_open`. */
static void ald_free(struct archive *_ar)
{
	if (!_ar)
		return;

	struct ald_archive *ar = (struct ald_archive*)_ar;

	// unmap mmap files/close file descriptors
	for (int i = 0; i < ALD_FILEMAX; i++) {
		if (ar->ar.mmapped && ar->files[i].data) {
			munmap(ar->files[i].data, ar->files[i].size);
		} else if (ar->files[i].fp) {
			fclose(ar->files[i].fp);
		}
		free(ar->files[i].name);
	}

	free(ar);
}

/* Open an ALD archive, optionally memory-mapping it. */
struct archive *ald_open_conv(char **files, int count, int flags, int *error, char*(*conv)(const char*))
{
	FILE *fp;
	long filesize;
	bool gotmap = false;
	struct ald_archive *ar = calloc(1, sizeof(struct ald_archive));
	ar->conv = conv;

#ifdef _WIN32
	flags &= ~ARCHIVE_MMAP;
#endif

	for (int i = 0; i < count; i++) {
		if (!files[i])
			continue;
		if (!(fp = file_open_utf8(files[i], "rb"))) {
			*error = ARCHIVE_FILE_ERROR;
			goto exit_err;
		}
		// check if it's a valid archive
		if (!file_check(fp)) {
			*error = ARCHIVE_BAD_ARCHIVE_ERROR;
			fclose(fp);
			goto exit_err;
		}
		// get file map, if we haven't already
		if (!gotmap) {
			get_filemap(ar, fp);
			gotmap = true;
		}
		// get pointer map
		get_ptrmap(ar, fp, i);
		// get file size for mmap
		filesize = get_file_size(fp);
		// copy filename
		ar->files[i].name = strdup(files[i]);
		// close
		fclose(fp);
		if (flags & ARCHIVE_MMAP) {
			int fd;
			if (0 > (fd = open(files[i], O_RDONLY))) {
				*error = ARCHIVE_FILE_ERROR;
				goto exit_err;
			}
			ar->files[i].data = mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
			close(fd);
			if (ar->files[i].data == MAP_FAILED) {
				*error = ARCHIVE_FILE_ERROR;
				goto exit_err;
			}
			ar->files[i].size = filesize;
		} else {
			// get a file descriptor for each file
			if (!(ar->files[i].fp = file_open_utf8(files[i], "rb"))) {
				*error = ARCHIVE_FILE_ERROR;
				goto exit_err;
			}
		}
	}
	int c = 0;
	for (int i = 0; i < ar->maxfile; i++) {
		if (ar->map_disk[i] < 0 || ar->map_ptr[i] < 0)
			continue;
		c++;
	}
	ar->ar.mmapped = flags & ARCHIVE_MMAP;
	ar->nr_files = count;
	ar->ar.ops = &ald_archive_ops;
	return &ar->ar;
exit_err:
	free(ar);
	return NULL;
}

struct archive *ald_open(char **files, int count, int flags, int *error)
{
	return ald_open_conv(files, count, flags, error, strdup);
}
