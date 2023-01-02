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

#ifndef SYSTEM4_H
#define SYSTEM4_H

#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// fix windows.h symbol conflict
#undef ERROR

// TODO: should put in a separate header and guard with feature checks
#define const_pure __attribute__((const))
#define mem_alloc __attribute__((malloc))
#ifdef __cplusplus
#define _Noreturn [[ noreturn ]]
#endif
#define possibly_unused __attribute__((unused))
#define warn_unused __attribute__((warn_unused_result))
#define unlikely(x) __builtin_expect(x, 0)

#define ERROR(fmt, ...) \
	sys_error("*ERROR*(%s:%s:%d): " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define WARNING(fmt, ...) \
	sys_warning("*WARNING*(%s:%s:%d): " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define NOTICE(fmt, ...) \
	sys_message(fmt "\n", ##__VA_ARGS__)

extern bool sys_silent;
extern void (*sys_error_handler)(const char *msg);
_Noreturn void sys_verror(const char *fmt, va_list ap);
_Noreturn void sys_error(const char *fmt, ...);
void sys_vwarning(const char *fmt, va_list ap);
void sys_warning(const char *fmt, ...);
void sys_message(const char *fmt, ...);

_Noreturn void sys_exit(int code);

#define xmalloc(size) _xmalloc(size, __func__)
mem_alloc void *_xmalloc(size_t size, const char *func);

#define xcalloc(nmemb, size) _xcalloc(nmemb, size, __func__)
mem_alloc void *_xcalloc(size_t nmemb, size_t size, const char *func);

#define xrealloc(ptr, size) _xrealloc(ptr, size, __func__)
mem_alloc void *_xrealloc(void *ptr, size_t size, const char *func);

#define xstrdup(str) _xstrdup(str, __func__)
mem_alloc char *_xstrdup(const char *in, const char *func);

mem_alloc void *xrealloc_array(void *dst, size_t old_nmemb, size_t new_nmemb, size_t size);

#define max(a, b)				\
	({					\
		__typeof__ (a) _a = (a);	\
		__typeof__ (b) _b = (b);	\
		_a > _b ? _a : _b;		\
	})

#define min(a, b)				\
	({					\
		__typeof__ (a) _a = (a);	\
		__typeof__ (b) _b = (b);	\
		_a < _b ? _a : _b;		\
	})

#ifdef _WIN32
#define mmap(...) (ERROR("mmap not supported on Windows"), NULL)
#define munmap(...) ERROR("munmap not supported on Windows")
#define MAP_FAILED 0
#else
#include <sys/mman.h>
#endif

#endif /* SYSTEM4_H */
