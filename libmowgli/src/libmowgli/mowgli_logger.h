/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_logger.h: Event and debugging message logging.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice is present in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MOWGLI_LOGGER_H__
#define __MOWGLI_LOGGER_H__

typedef void (*mowgli_log_cb_t)(const char *);

#ifdef __GNUC__
# define mowgli_log(...) mowgli_log_real(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#elif defined _MSC_VER
# if _MSC_VER <= 1200
   static __inline void mowgli_log(char *fmt, ...) { /* TODO/UNSUPPORTED */ }
# else
#  define mowgli_log(...) mowgli_log_real(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
# endif
#else
# define mowgli_log(...) mowgli_log_real(__FILE__, __LINE__, __func__, __VA_ARGS__)
#endif

extern void mowgli_log_real(const char *file, int line, const char *func, const char *buf, ...);

extern void mowgli_log_set_cb(mowgli_log_cb_t callback);

#endif
