/* OpenVAS-Client
 * $Id$
 * Description: Writing and reading one or more openvas_certificates to / from 
 * a file that roughly uses freedesktop.org specifications (Glibs GKeyFile).
 *
 * Authors:
 * Felix Wolfsteller <felix.wolfsteller@intevation.de>
 *
 * Copyright:
 * Copyright (C) 2009 Greenbone Networks GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * or, at your option, any later version as published by the Free
 * Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENVAS_CERTIFICATE_FILE_H
#define _OPENVAS_CERTIFICATE_FILE_H

#include <includes.h>
#include <glib.h>
#include "openvas_i18n.h"

#include "context.h"
#include "error_dlg.h"
#include "openvas_certificates.h"

gboolean openvas_certificate_file_write(struct context* context, 
                                        char* filename);

GHashTable* openvas_certificate_file_read(char* filename);

#endif
