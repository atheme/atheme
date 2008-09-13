/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_hook.h: Hooks.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 * Copyright (c) 2007 Giacomo Lozito <james -at- develia.org>
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

#ifndef __MOWGLI_HOOK_H__
#define __MOWGLI_HOOK_H__

typedef void (*mowgli_hook_function_t)(void *hook_data, void *user_data);

typedef struct {
    mowgli_hook_function_t func;
    void *user_data;
    mowgli_node_t node;
} mowgli_hook_item_t;

typedef struct {
    const char *name;
    mowgli_list_t items;
} mowgli_hook_t;

extern void mowgli_hook_init(void);
extern void mowgli_hook_register(const char *name);
extern int  mowgli_hook_associate(const char *name, mowgli_hook_function_t func, void * user_data);
extern int  mowgli_hook_dissociate(const char *name, mowgli_hook_function_t func);
extern void mowgli_hook_call(const char *name, void * hook_data);

#endif
