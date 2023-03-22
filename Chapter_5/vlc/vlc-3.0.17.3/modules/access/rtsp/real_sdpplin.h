/*
 * Copyright (C) 2002-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: 3818fcca70b3e85446e809af4c735f723ecb47bd $
 *
 * sdp/sdpplin parser.
 *
 */
 
#ifndef HAVE_SDPPLIN_H
#define HAVE_SDPPLIN_H

typedef struct {

  char *id;
  char *bandwidth;

  uint16_t stream_id;
  char *range;
  char *length;
  char *rtpmap;
  char *mimetype;
  int min_switch_overlap;
  int start_time;
  int end_one_rule_end_all;
  int avg_bit_rate;
  int max_bit_rate;
  int avg_packet_size;
  int max_packet_size;
  int end_time;
  int seek_greater_on_switch;
  int preroll;

  int duration;
  char *stream_name;
  int stream_name_size;
  char *mime_type;
  int mime_type_size;
  char *mlti_data;
  int mlti_data_size;
  int  rmff_flags_length;
  char *rmff_flags;
  int  asm_rule_book_length;
  char *asm_rule_book;

} sdpplin_stream_t;

typedef struct {

  int sdp_version, sdpplin_version;
  char *owner;
  char *session_name;
  char *session_info;
  char *uri;
  char *email;
  char *phone;
  char *connection;
  char *bandwidth;

  int flags;
  int is_real_data_type;
  uint16_t stream_count;
  char *title;
  char *author;
  char *copyright;
  char *keywords;
  int  asm_rule_book_length;
  char *asm_rule_book;
  char *abstract;
  char *range;
  int avg_bit_rate;
  int max_bit_rate;
  int avg_packet_size;
  int max_packet_size;
  int preroll;
  int duration;

  sdpplin_stream_t **stream;

} sdpplin_t;

sdpplin_t *sdpplin_parse(stream_t *p_access, char *data);

void sdpplin_free(sdpplin_t *description);

#endif
