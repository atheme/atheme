/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_signal.c: Safe signal handling.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

static mowgli_signal_handler_t
mowgli_signal_install_handler_full(int signum, signal_handler_t handler,
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
signal_handler_t
mowgli_signal_install_handler(int signum, signal_handler_t handler)
{
	return mowgli_signal_install_handler_full(signum, handler, NULL, 0);
}
