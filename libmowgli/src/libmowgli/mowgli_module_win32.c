/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_module_win32.c: Loadable modules under Microsoft Windows.
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

#include "mowgli.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

mowgli_module_t mowgli_module_open(const char *path)
{
	HANDLE handle = LoadLibraryA(path);

	/* make sure we have something. make this an assertion so that 
	 * there is feedback if something happens. (poor programming practice).
	 */
	return_val_if_fail(handle != NULL, NULL);

	return (mowgli_module_t)handle;
}

void * mowgli_module_symbol(mowgli_module_t module, const char *symbol)
{
	void *handle;

	return_val_if_fail(module != NULL, NULL);

	handle = GetProcAddress((HANDLE)module, symbol);

	/* make sure we have something. make this an assertion so that 
	 * there is feedback if something happens. (poor programming practice).
	 */
	return_val_if_fail(handle != NULL, NULL);

	return handle;
}

void mowgli_module_close(mowgli_module_t module)
{
	return_if_fail(module != NULL);

	FreeLibrary((HANDLE)module);
}
