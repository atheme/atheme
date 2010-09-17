/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_assert.h: Assertions.
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

#ifndef __MOWGLI_ASSERT_H__
#define __MOWGLI_ASSERT_H__

extern void mowgli_soft_assert_log(const char *asrt, const char *file, int line, const char *function);

#ifdef __GNUC__

/*
 * Performs a soft assertion. If the assertion fails, we log it.
 */
#define soft_assert(x)								        \
	if (!(x)) { 								        \
                mowgli_soft_assert_log(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
	}

/*
 * Same as soft_assert, but returns if an assertion fails.
 */
#define return_if_fail(x)							        \
	if (!(x)) { 								        \
                mowgli_soft_assert_log(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
		return;								        \
	}

/*
 * Same as soft_assert, but returns a given value if an assertion fails.
 */
#define return_val_if_fail(x, y)						        \
	if (!(x)) { 								        \
                mowgli_soft_assert_log(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
		return (y);							        \
	}

/*
 * Same as soft_assert, but returns NULL if the value is NULL.
 */
#define return_if_null(x)							        \
	if (x == NULL) { 							        \
                mowgli_soft_assert_log(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
		return (NULL);							        \
	}

#else

/*
 * Performs a soft assertion. If the assertion fails, we log it.
 */
#define soft_assert(x)								        \
	if (!(x)) { 								        \
		mowgli_soft_assert_log(#x, __FILE__, __LINE__, __FUNCTION__);	        \
	}

/*
 * Same as soft_assert, but returns if an assertion fails.
 */
#define return_if_fail(x)							        \
	if (!(x)) { 								        \
		mowgli_soft_assert_log(#x, __FILE__, __LINE__, __FUNCTION__);	        \
		return;								        \
	}

/*
 * Same as soft_assert, but returns a given value if an assertion fails.
 */
#define return_val_if_fail(x, y)						        \
	if (!(x)) { 								        \
		mowgli_soft_assert_log(#x, __FILE__, __LINE__, __FUNCTION__);	        \
		return (y);							        \
	}

/*
 * Same as soft_assert, but returns NULL if the value is NULL.
 */
#define return_if_null(x)							        \
	if (x == NULL) { 							        \
		mowgli_soft_assert_log(#x, __FILE__, __LINE__, __FUNCTION__);	        \
		return (NULL);							        \
	}

#endif

#endif
