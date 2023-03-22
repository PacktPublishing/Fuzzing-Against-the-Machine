/*****************************************************************************
 * omxil.c: Video decoder module making use of OpenMAX IL components.
 *****************************************************************************
 * Copyright (C) 2010 VLC authors and VideoLAN
 * $Id: 663a61acc9466a5b2f71f81fe2de560f3291a51c $
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "OMX_Core.h"

#include <vlc_common.h>

extern OMX_ERRORTYPE (*pf_init)(void);
extern OMX_ERRORTYPE (*pf_deinit)(void);
extern OMX_ERRORTYPE (*pf_get_handle)(OMX_HANDLETYPE *, OMX_STRING,
                                      OMX_PTR, OMX_CALLBACKTYPE *);
extern OMX_ERRORTYPE (*pf_free_handle)(OMX_HANDLETYPE);
extern OMX_ERRORTYPE (*pf_component_enum)(OMX_STRING, OMX_U32, OMX_U32);
extern OMX_ERRORTYPE (*pf_get_roles_of_component)(OMX_STRING, OMX_U32 *, OMX_U8 **);

/* Extra IOMX android functions. Can be NULL if we don't link with libiomx */
OMX_ERRORTYPE (*pf_enable_graphic_buffers)(OMX_HANDLETYPE, OMX_U32, OMX_BOOL);
OMX_ERRORTYPE (*pf_get_graphic_buffer_usage)(OMX_HANDLETYPE, OMX_U32, OMX_U32*);
OMX_ERRORTYPE (*pf_get_hal_format) (const char *, int *);

int InitOmxCore(vlc_object_t *p_this);
void DeinitOmxCore(void);

#define MAX_COMPONENTS_LIST_SIZE 32
int CreateComponentsList(vlc_object_t *p_this, const char *psz_role,
                         char ppsz_components[MAX_COMPONENTS_LIST_SIZE][OMX_MAX_STRINGNAME_SIZE]);

