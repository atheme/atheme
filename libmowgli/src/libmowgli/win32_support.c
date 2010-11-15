/*
 * libmowgli: A collection of useful routines for programming.
 * win32_support.c: Support functions and values for Win32 platform.
 *
 * Copyright (c) 2009 SystemInPlace, Inc.
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

#ifdef _MSC_VER
# define EPOCH_TIME_IN_MICROSECS	11644473600000000Ui64
#else
# define EPOCH_TIME_IN_MICROSECS	11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	ULARGE_INTEGER tmpres = { 0 };
	static mowgli_boolean_t tz_init_done = FALSE;

	if (tv != NULL)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres.u.HighPart = ft.dwHighDateTime;
		tmpres.u.LowPart  = ft.dwLowDateTime;

		tmpres.QuadPart /= 10;
		tmpres.QuadPart -= EPOCH_TIME_IN_MICROSECS;
		tv->tv_sec = (long) (tmpres.QuadPart / 1000000UL);
		tv->tv_usec = (long) (tmpres.QuadPart % 1000000UL);
	}

	if (tz != NULL)
	{
		if (!tz_init_done)
		{
			_tzset();
			tz_init_done = TRUE;
		}

		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}
