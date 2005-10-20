/*
 * Copyright (c) 2005 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines that interface the event system.
 * This code is based on ircd-ratbox's event.c with modifications.
 *
 * $Id: event.c 3047 2005-10-20 04:37:13Z nenolod $
 */

#include <org.atheme.claro.base>

const char *last_event_ran = NULL;
struct ev_entry event_table[MAX_EVENTS];
static time_t event_time_min = -1;

/* add an event to the table to be continually ran */
uint32_t event_add(const char *name, EVH *func, void *arg, time_t when)
{
	uint32_t i;

	/* find the first inactive index */
	for (i = 0; i < MAX_EVENTS; i++)
	{
		if (!event_table[i].active)
		{
			event_table[i].func = func;
			event_table[i].name = name;
			event_table[i].arg = arg;
			event_table[i].when = CURRTIME + when;
			event_table[i].frequency = when;
			event_table[i].active = TRUE;

			if ((event_table[i].when < event_time_min) || (event_time_min == -1))
				event_time_min = event_table[i].when;

			clog(LG_DEBUG, "event_add(): \"%s\"", name);

			claro_state.event++;

			return i;
		}
	}

	/* failed to add it... */
	clog(LG_DEBUG, "event_add(): failed to add \"%s\" to event table", name);

	return -1;
}

/* adds an event to the table to be ran only once */
uint32_t event_add_once(const char *name, EVH *func, void *arg, time_t when)
{
	uint32_t i;

	/* find the first inactive index */
	for (i = 0; i < MAX_EVENTS; i++)
	{
		if (!event_table[i].active)
		{
			event_table[i].func = func;
			event_table[i].name = name;
			event_table[i].arg = arg;
			event_table[i].when = CURRTIME + when;
			event_table[i].frequency = 0;
			event_table[i].active = TRUE;

			if ((event_table[i].when < event_time_min) || (event_time_min == -1))
				event_time_min = event_table[i].when;

			clog(LG_DEBUG, "event_add_once(): \"%s\"", name);

			claro_state.event++;

			return i;
		}
	}

	/* failed to add it... */
	clog(LG_DEBUG, "event_add(): failed to add \"%s\" to event table", name);

	return -1;
}

/* delete an event from the table */
void event_delete(EVH *func, void *arg)
{
	int32_t i = event_find(func, arg);

	if (i == -1)
		return;

	clog(LG_DEBUG, "event_delete(): removing \"%s\"", event_table[i].name);

	event_table[i].name = NULL;
	event_table[i].func = NULL;
	event_table[i].arg = NULL;
	event_table[i].active = FALSE;

	claro_state.event--;
}

/* checks all pending events */
void event_run(void)
{
	uint32_t i;

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
				event_table[i].name = NULL;
				event_table[i].func = NULL;
				event_table[i].arg = NULL;
				event_table[i].active = FALSE;

				claro_state.event--;
			}
		}
	}
}

/* returns the time the next event_run() should happen */
time_t event_next_time(void)
{
	uint32_t i;

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
uint32_t event_find(EVH *func, void *arg)
{
	uint32_t i;

	for (i = 0; i < MAX_EVENTS; i++)
	{
		if ((event_table[i].func == func) && (event_table[i].arg == arg))
			return i;
	}

	return -1;
}
