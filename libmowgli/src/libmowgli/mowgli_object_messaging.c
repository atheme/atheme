/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_object_messaging.c: Object event notification and message passing.
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

void mowgli_object_class_message_handler_attach(mowgli_object_class_t *klass, mowgli_object_message_handler_t *sig)
{
	if (klass == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_object_class_exception);

	if (sig == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_signal_exception);

	mowgli_node_add(sig, mowgli_node_create(), &klass->message_handlers);
}

void mowgli_object_class_message_handler_detach(mowgli_object_class_t *klass, mowgli_object_message_handler_t *sig)
{
	mowgli_node_t *n;

	if (klass == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_object_class_exception);

	if (sig == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_signal_exception);

	n = mowgli_node_find(sig, &klass->message_handlers);
	mowgli_node_delete(n, &klass->message_handlers);
	mowgli_node_free(n);
}

void mowgli_object_message_handler_attach(mowgli_object_t *self, mowgli_object_message_handler_t *sig)
{
	if (self == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_object_exception);

	if (sig == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_signal_exception);

	mowgli_node_add(sig, mowgli_node_create(), &self->message_handlers);
}

void mowgli_object_message_handler_detach(mowgli_object_t *self, mowgli_object_message_handler_t *sig)
{
	mowgli_node_t *n;

	if (self == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_object_exception);

	if (sig == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_signal_exception);

	n = mowgli_node_find(sig, &self->message_handlers);
	mowgli_node_delete(n, &self->message_handlers);
	mowgli_node_free(n);
}

void mowgli_object_message_broadcast(mowgli_object_t *self, const char *name, ...)
{
	mowgli_argstack_t *stack;
	mowgli_object_message_handler_t *sig = NULL;
	mowgli_node_t *n;
	va_list va;

	if (self == NULL)
		mowgli_throw_exception(mowgli.object_messaging.invalid_object_exception);

	if (name == NULL)
		mowgli_throw_exception(mowgli.null_pointer_exception);

	/* try to find a signal to compile the argument stack from, we start with self::klass first. */
	MOWGLI_LIST_FOREACH(n, self->klass->message_handlers.head)
	{
		mowgli_object_message_handler_t *sig2 = (mowgli_object_message_handler_t *) n->data;

		if (!strcasecmp(sig2->name, name))
		{
			sig = sig2;
			break;
		}
	}

	if (sig == NULL)
	{
		MOWGLI_LIST_FOREACH(n, self->klass->message_handlers.head)
		{
			mowgli_object_message_handler_t *sig2 = (mowgli_object_message_handler_t *) n->data;

			if (!strcasecmp(sig2->name, name))
			{
				sig = sig2;
				break;
			}
		}
	}

	/* return if no signals found, else compile the argstack */
	if (sig == NULL)
		return;

	va_start(va, name);
	stack = mowgli_argstack_create_from_va_list(sig->descstr, va);
	va_end(va);

	MOWGLI_LIST_FOREACH(n, self->klass->message_handlers.head)
	{
		sig = (mowgli_object_message_handler_t *) n->data;

		if (!strcasecmp(sig->name, name) && sig->handler != NULL)
			sig->handler(self, sig, stack);
	}

	MOWGLI_LIST_FOREACH(n, self->message_handlers.head)
	{
		sig = (mowgli_object_message_handler_t *) n->data;

		if (!strcasecmp(sig->name, name) && sig->handler != NULL)
			sig->handler(self, sig, stack);
	}

	mowgli_object_unref(stack);
}
