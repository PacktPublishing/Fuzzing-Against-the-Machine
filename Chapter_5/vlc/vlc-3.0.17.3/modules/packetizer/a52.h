/*****************************************************************************
 * a52.h
 *****************************************************************************
 * Copyright (C) 2001-2016 Laurent Aimar
 * $Id: c46a0f2bb9ed86f418c2d2d6044b7be999acd8a7 $
 *
 * Authors: Stéphane Borel <stef@via.ecp.fr>
 *          Christophe Massiot <massiot@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Thomas Guillem <thomas@gllm.fr>
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

#ifndef VLC_A52_H_
#define VLC_A52_H_

#include <vlc_bits.h>

/**
 * Minimum AC3 header size that vlc_a52_header_Parse needs.
 */
#define VLC_A52_HEADER_SIZE (8)

/**
 * AC3 header information.
 */
typedef struct
{
    bool b_eac3;

    unsigned int i_channels;
    unsigned int i_channels_conf;
    unsigned int i_chan_mode;
    unsigned int i_rate;
    unsigned int i_bitrate;

    unsigned int i_size;
    unsigned int i_samples;

    union {
        struct {
            enum {
                EAC3_STRMTYP_INDEPENDENT    = 0,
                EAC3_STRMTYP_DEPENDENT      = 1,
                EAC3_STRMTYP_AC3_CONVERT    = 2,
                EAC3_STRMTYP_RESERVED,
            } strmtyp;
            uint8_t i_substreamid;
        } eac3;
    };
    uint8_t i_blocks_per_sync_frame;
} vlc_a52_header_t;

/**
 * It parse AC3 sync info.
 *
 * cf. AC3 spec
 */
static inline int vlc_a52_header_ParseAc3( vlc_a52_header_t *p_header,
                                           const uint8_t *p_buf,
                                           const uint32_t *p_acmod,
                                           const unsigned *pi_fscod_samplerates )
{
    /* cf. Table 5.18 Frame Size Code Table */
    static const uint16_t ppi_frmsizcod_fscod_sizes[][3] = {
        /* 32K, 44.1K, 48K */
        { 96, 69, 64 },
        { 96, 70, 64 },
        { 120, 87, 80 },
        { 120, 88, 80 },
        { 144, 104, 96 },
        { 144, 105, 96 },
        { 168, 121, 112 },
        { 168, 122, 112 },
        { 192, 139, 128 },
        { 192, 140, 128 },
        { 240, 174, 160 },
        { 240, 175, 160 },
        { 288, 208, 192 },
        { 288, 209, 192 },
        { 336, 243, 224 },
        { 336, 244, 224 },
        { 384, 278, 256 },
        { 384, 279, 256 },
        { 480, 348, 320 },
        { 480, 349, 320 },
        { 576, 417, 384 },
        { 576, 418, 384 },
        { 672, 487, 448 },
        { 672, 488, 448 },
        { 768, 557, 512 },
        { 768, 558, 512 },
        { 960, 696, 640 },
        { 960, 697, 640 },
        { 1152, 835, 768 },
        { 1152, 836, 768 },
        { 1344, 975, 896 },
        { 1344, 976, 896 },
        { 1536, 1114, 1024 },
        { 1536, 1115, 1024 },
        { 1728, 1253, 1152 },
        { 1728, 1254, 1152 },
        { 1920, 1393, 1280 },
        { 1920, 1394, 1280 }
    };
    static const uint16_t pi_frmsizcod_bitrates[] = {
        32,  40,  48,  56,
        64,  80,  96, 112,
        128, 160, 192, 224,
        256, 320, 384, 448,
        512, 576, 640
    };

    bs_t s;
    bs_init( &s, (void*)p_buf, VLC_A52_HEADER_SIZE );
    bs_skip( &s, 32 );  /* start code + CRC */

    /* cf. 5.3.2 */
    const uint8_t i_fscod = bs_read( &s, 2 );
    if( i_fscod == 3 )
        return VLC_EGENERIC;
    const uint8_t i_frmsizcod = bs_read( &s, 6 );
    if( i_frmsizcod >= 38 )
        return VLC_EGENERIC;
    const uint8_t i_bsid = bs_read( &s, 5 );
    bs_skip( &s, 3 ); /* i_bsmod */
    const uint8_t i_acmod = bs_read( &s, 3 );
    if( ( i_acmod & 0x1 ) && ( i_acmod != 0x1 ) )
    {
        /* if 3 front channels */
        bs_skip( &s, 2 ); /* i_cmixlev */
    }
    if( i_acmod & 0x4 )
    {
        /* if a surround channel exists */
        bs_skip( &s, 2 ); /* i_surmixlev */
    }
    /* if in 2/0 mode */
    const uint8_t i_dsurmod = i_acmod == 0x2 ? bs_read( &s, 2 ) : 0;
    const uint8_t i_lfeon = bs_read( &s, 1 );

    p_header->i_channels_conf = p_acmod[i_acmod];
    p_header->i_chan_mode = 0;
    if( i_dsurmod == 2 )
        p_header->i_chan_mode |= AOUT_CHANMODE_DOLBYSTEREO;
    if( i_acmod == 0 )
        p_header->i_chan_mode |= AOUT_CHANMODE_DUALMONO;

    if( i_lfeon )
        p_header->i_channels_conf |= AOUT_CHAN_LFE;

    p_header->i_channels = popcount(p_header->i_channels_conf);

    const unsigned i_rate_shift = VLC_CLIP(i_bsid, 8, 11) - 8;
    p_header->i_bitrate = (pi_frmsizcod_bitrates[i_frmsizcod >> 1] * 1000)
                        >> i_rate_shift;
    p_header->i_rate = pi_fscod_samplerates[i_fscod] >> i_rate_shift;

    p_header->i_size = ppi_frmsizcod_fscod_sizes[i_frmsizcod][2 - i_fscod] * 2;
    p_header->i_blocks_per_sync_frame = 6;
    p_header->i_samples = p_header->i_blocks_per_sync_frame * 256;

    p_header->b_eac3 = false;
    return VLC_SUCCESS;
}

/**
 * It parse E-AC3 sync info
 */
static inline int vlc_a52_header_ParseEac3( vlc_a52_header_t *p_header,
                                            const uint8_t *p_buf,
                                            const uint32_t *p_acmod,
                                            const unsigned *pi_fscod_samplerates )
{
    bs_t s;
    bs_init( &s, (void*)p_buf, VLC_A52_HEADER_SIZE );
    bs_skip( &s, 16 );  /* start code */
    p_header->eac3.strmtyp = bs_read( &s, 2 );      /* Stream Type */
    p_header->eac3.i_substreamid = bs_read( &s, 3 );/* Substream Identification */

    const uint16_t i_frmsiz = bs_read( &s, 11 );
    if( i_frmsiz < 2 )
        return VLC_EGENERIC;
    p_header->i_size = 2 * (i_frmsiz + 1 );

    const uint8_t i_fscod = bs_read( &s, 2 );
    if( i_fscod == 0x03 )
    {
        const uint8_t i_fscod2 = bs_read( &s, 2 );
        if( i_fscod2 == 0x03 )
            return VLC_EGENERIC;
        p_header->i_rate = pi_fscod_samplerates[i_fscod2] / 2;
        p_header->i_blocks_per_sync_frame = 6;
    }
    else
    {
        static const int pi_numblkscod [4] = { 1, 2, 3, 6 };

        p_header->i_rate = pi_fscod_samplerates[i_fscod];
        p_header->i_blocks_per_sync_frame = pi_numblkscod[bs_read( &s, 2 )];
    }

    const unsigned i_acmod = bs_read( &s, 3 );
    const unsigned i_lfeon = bs_read1( &s );

    p_header->i_channels_conf = p_acmod[i_acmod];
    p_header->i_chan_mode = 0;
    if( i_acmod == 0 )
        p_header->i_chan_mode |= AOUT_CHANMODE_DUALMONO;
    if( i_lfeon )
        p_header->i_channels_conf |= AOUT_CHAN_LFE;
    p_header->i_channels = popcount( p_header->i_channels_conf );
    p_header->i_bitrate = 8 * p_header->i_size * p_header->i_rate
                        / (p_header->i_blocks_per_sync_frame * 256);
    p_header->i_samples = p_header->i_blocks_per_sync_frame * 256;

    p_header->b_eac3 = true;
    return VLC_SUCCESS;
}

/**
 * It will parse the header AC3 frame and fill vlc_a52_header_t* if
 * it is valid or return VLC_EGENERIC.
 *
 * XXX It will only recognize big endian bitstream ie starting with 0x0b, 0x77
 */
static inline int vlc_a52_header_Parse( vlc_a52_header_t *p_header,
                                        const uint8_t *p_buffer, int i_buffer )
{
    static const uint32_t p_acmod[8] = {
        AOUT_CHANS_2_0,
        AOUT_CHAN_CENTER,
        AOUT_CHANS_2_0,
        AOUT_CHANS_3_0,
        AOUT_CHANS_FRONT | AOUT_CHAN_REARCENTER, /* 2F1R */
        AOUT_CHANS_FRONT | AOUT_CHANS_CENTER,    /* 3F1R */
        AOUT_CHANS_4_0,
        AOUT_CHANS_5_0,
    };
    static const unsigned pi_fscod_samplerates[] = {
        48000, 44100, 32000
    };

    if( i_buffer < VLC_A52_HEADER_SIZE )
        return VLC_EGENERIC;

    /* Check synword */
    if( p_buffer[0] != 0x0b || p_buffer[1] != 0x77 )
        return VLC_EGENERIC;

    /* Check bsid */
    const int bsid = p_buffer[5] >> 3;

    /* cf. Annex E 2.3.1.6 of AC3 spec */
    if( bsid <= 10 )
        return vlc_a52_header_ParseAc3( p_header, p_buffer,
                                        p_acmod, pi_fscod_samplerates );
    else if( bsid <= 16 )
        return vlc_a52_header_ParseEac3( p_header, p_buffer,
                                         p_acmod, pi_fscod_samplerates );
    else
        return VLC_EGENERIC;
}

#endif
