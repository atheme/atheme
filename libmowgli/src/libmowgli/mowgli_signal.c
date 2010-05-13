/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_signal.c: Safe signal handling.
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

#ifndef _WIN32

#include "mowgli.h"

static mowgli_signal_handler_t
mowgli_signal_install_handler_full(int signum, mowgli_signal_handler_t handler,
			    int *sigtoblock, size_t sigtoblocksize)
{
	struct sigaction action, old_action;
	size_t i;

	action.sa_handler = handler;
	action.sa_flags = SA_RESTART;

	sigemptyset(&action.sa_mask);

	for (i = 0; i < sigtoblocksize; i++)
		sigaddset(&action.sa_mask, sigtoblock[i]);

	if (sigaction(signum, &action, &old_action) == -1)
	{
		mowgli_log("Failed to install signal handler for signal %d", signum);
		return NULL;
	}

	return old_action.sa_handler;
}

/*
 * A version of signal(2) that works more reliably across different
 * platforms.
 *
 * It restarts interrupted system calls, does not reset the handler,
 * and blocks the same signal from within the handler.
 */
mowgli_signal_handler_t
mowgli_signal_install_handler(int signum, mowgli_signal_handler_t handler)
{
	return mowgli_signal_install_handler_full(signum, handler, NULL, 0);
}

#endif
