/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli.h: Base header for libmowgli. Includes everything.
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

#ifndef __MOWGLI_STAND_H__
#define __MOWGLI_STAND_H__

#ifdef __cplusplus
# define MOWGLI_DECLS_START extern "C" {
# define MOWGLI_DECLS_END   }
#else
# define MOWGLI_DECLS_START
# define MOWGLI_DECLS_END
#endif

#ifdef MOWGLI_CORE
# include "win32_support.h"
# include "mowgli_config.h"
#endif

#include "mowgli_stdinc.h"

MOWGLI_DECLS_START

#include "mowgli_logger.h"
#include "mowgli_assert.h"
#include "mowgli_exception.h"
#include "mowgli_iterator.h"

#include "mowgli_alloc.h"
#include "mowgli_spinlock.h"
#include "mowgli_list.h"
#include "mowgli_object_class.h"
#include "mowgli_object.h"
#include "mowgli_allocation_policy.h"
#include "mowgli_dictionary.h"
#include "mowgli_patricia.h"
#include "mowgli_mempool.h"
#include "mowgli_module.h"
#include "mowgli_queue.h"
#include "mowgli_hash.h"
#include "mowgli_heap.h"
#include "mowgli_init.h"
#include "mowgli_bitvector.h"
#include "mowgli_hook.h"
#include "mowgli_signal.h"
#include "mowgli_error_backtrace.h"
#include "mowgli_random.h"
#include "mowgli_ioevent.h"
#include "mowgli_argstack.h"
#include "mowgli_object_messaging.h"
#include "mowgli_object_metadata.h"
#include "mowgli_global_storage.h"
#include "mowgli_string.h"
#include "mowgli_allocator.h"
#include "mowgli_formatter.h"

MOWGLI_DECLS_END

#endif

