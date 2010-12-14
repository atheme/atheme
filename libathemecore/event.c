/*
 * atheme-services: A collection of minimalist IRC services   
 * event.c: Event subsystem.
 *
 * Copyright (c) 2005-2007 Atheme Project (http://www.atheme.org)           
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
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

#include "atheme.h"
#include "internal.h"

const char *last_event_ran = NULL;
struct ev_entry event_table[MAX_EVENTS];
static time_t event_time_min = -1;

static unsigned int event_add_real(const char *name, EVH *func, void *arg, time_t when, time_t frequency)
{
	unsigned int i;

	/* find the first inactive index */
	for (i = 0; i < MAX_EVENTS; i++)
	{
		if (!event_table[i].active)
		{
			event_table[i].func = func;
			event_table[i].name = name;
			event_table[i].arg = arg;
			event_table[i].when = CURRTIME + when;
			event_table[i].frequency = frequency;
			event_table[i].active = true;

			if (event_table[i].when < event_time_min && event_time_min != -1)
				event_time_min = event_table[i].when;

			slog(LG_DEBUG, "event_add_real(): added %s to event table", name);

			claro_state.event++;

			return i;
		}
	}

	/* failed to add it... */
	slog(LG_DEBUG, "event_add_real(): failed to add %s to event table", name);

	return -1;
}

/* add an event to the table to be continually ran */
unsigned int event_add(const char *name, EVH *func, void *arg, time_t when)
{
	return event_add_real(name, func, arg, when, when);
}

/* adds an event to the table to be ran only once */
unsigned int event_add_once(const char *name, EVH *func, void *arg, time_t when)
{
	return event_add_real(name, func, arg, when, 0);
}

/* delete an event from the table */
void event_delete(EVH *func, void *arg)
{
	int i = event_find(func, arg);

	if (i == -1)
		return;

	slog(LG_DEBUG, "event_delete(): removing \"%s\"", event_table[i].name);

	if (last_event_ran == event_table[i].name)
		last_event_ran = "<removed>";
	event_table[i].name = NULL;
	event_table[i].func = NULL;
	event_table[i].arg = NULL;
	event_table[i].active = false;

	claro_state.event--;
}

/* checks all pending events */
void event_run(void)
{
	unsigned int i;

	for (i = 0; i < MAX_EVENTS; i++)
	{
		if (event_table[i].active && (event_table[i].when <= CURRTIME))
		{
			/* now we call it */
			last_event_ran = event_table[i].name;
			event_table[i].func(event_table[i].arg);
			event_time_min = -1;

			/* event is scheduled more than once */
			if (event_table[i].frequency)
				event_table[i].when = CURRTIME + event_table[i].frequency;
			else
			{
				/* not nice but we need to play safe */
				last_event_ran = "<onceonly>";
				event_table[i].name = NULL;
				event_table[i].func = NULL;
				event_table[i].arg = NULL;
				event_table[i].active = false;

				claro_state.event--;
			}
		}
	}
}

/* returns the time the next event_run() should happen */
time_t event_next_time(void)
{
	unsigned int i;

	if (event_time_min == -1)
	{
		for (i = 0; i < MAX_EVENTS; i++)
		{
			if (event_table[i].active && ((event_table[i].when < event_time_min) || event_time_min == -1))
				event_time_min = event_table[i].when;
		}
	}

	return event_time_min;
}

/* initializes event system */
void event_init(void)
{
	last_event_ran = NULL;
	memset((void *)event_table, 0, sizeof(event_table));
}

/* finds an event in the table */
unsigned int event_find(EVH *func, void *arg)
{
	unsigned int i;

	for (i = 0; i < MAX_EVENTS; i++)
	{
		if ((event_table[i].func == func) && (event_table[i].arg == arg))
			return i;
	}

	return -1;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
