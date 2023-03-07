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

#ifndef SYSTEM4_STRING_H
#define SYSTEM4_STRING_H

#include <stdbool.h>
#include <stdlib.h>

union vm_value;

struct string {
	int size;
	unsigned int ref : 24;
	unsigned int cow : 1;
	char text[];
};

extern struct string EMPTY_STRING;

void free_string(struct string *str);

// constructors
struct string *make_string(const char *str, size_t len);
struct string *string_alloc(unsigned int len);
struct string *string_realloc(struct string *s, unsigned int size);
struct string *string_ref(struct string *s);
struct string *string_dup(const struct string *in);
struct string *integer_to_string(int n);
int string_to_integer(struct string *s);
struct string *float_to_string(float f, int precision);
struct string *string_concatenate(const struct string *a, const struct string *b);
struct string *string_copy(const struct string *s, int index, int len);

// mutators
void string_append(struct string **a, const struct string *b);
void string_append_cstr(struct string **a, const char *b, size_t b_size);
void string_push_back(struct string **s, int c);
void string_pop_back(struct string **s);
void string_erase(struct string **s, int index);
void string_clear(struct string *s);

// queries
int string_find(const struct string *haystack, const struct string *needle);

// characters
int string_get_char(const struct string *str, int i);
void string_set_char(struct string **s, int i, unsigned int c);

// C strings
int int_to_cstr(char *buf, size_t size, int v, int figures, bool zero_pad, bool zenkaku);
int float_to_cstr(char *buf, size_t size, float v, int figures, bool zero_pad, int precision, bool zenkaku);
struct string *cstr_to_string(const char *str);

#endif
