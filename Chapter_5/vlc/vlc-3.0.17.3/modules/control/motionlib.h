/*****************************************************************************
 * motion.h: laptop built-in motion sensors
 *****************************************************************************
 * Copyright (C) 2012 the VideoLAN team
 * $Id: 6292b698297076ebeeb1e932026d85141779a87a $
 *
 * Author: Pierre Ynard
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef MOTION_H
#define MOTION_H

typedef struct motion_sensors_t motion_sensors_t;

motion_sensors_t *motion_create( vlc_object_t *obj );
void motion_destroy( motion_sensors_t *motion );
int motion_get_angle( motion_sensors_t *motion );

#endif

