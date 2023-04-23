/* transport-factory.c
 *
 * A factory for transport layer instances.
 *
 * Dedicated to Yuzuyu Arielle Huizer.
 *
 * Copyright (C) 2016-2019  Ronald Huizer <rhuizer@hexpedition.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <string.h>
#include "transport-factory.h"
#include "transport-pty.h"
#include "transport-unix.h"

struct transport *
transport_factory_new(int type)
{
	struct transport *t = NULL;

	switch (type) {
	case TRANSPORT_PTY:
		t = transport_pty_new();
		break;
	case TRANSPORT_UNIX:
		t = transport_unix_new();
		break;
	}

	if (t != NULL)
		t->type = type;

	return t;
}

struct transport *
transport_factory_new_by_name(const char *name)
{
	if (!strcmp(name, "pty"))
		return transport_factory_new(TRANSPORT_PTY);

	if (!strcmp(name, "unix"))
		return transport_factory_new(TRANSPORT_UNIX);

	return NULL;
}
