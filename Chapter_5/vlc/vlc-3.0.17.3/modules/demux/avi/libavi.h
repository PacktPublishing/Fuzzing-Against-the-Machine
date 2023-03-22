/*****************************************************************************
 * libavi.h : LibAVI library
 ******************************************************************************
 * Copyright (C) 2001-2003 VLC authors and VideoLAN
 * $Id: 868d97bb92cdfb434ca55bc3388dfa8db5c63b6e $
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
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

/* biCompression / Others are FourCC */
#define BI_RGB              0x0000
#define BI_RLE8             0x0001
#define BI_RLE4             0x0002
#define BI_BITFIELDS        0x0003
#define BI_JPEG             0x0004
#define BI_PNG              0x0005
#define BI_CMYK             0x000B
#define BI_CMYKRLE8         0x000C
#define BI_CMYKRLE4         0x000D

/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX       0x00000010  /* Index at end of file? */
#define AVIF_MUSTUSEINDEX   0x00000020
#define AVIF_ISINTERLEAVED  0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800  /* Use CKType to find key frames? */
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED    0x00020000

/* Flags for index */
#define AVIIF_LIST          0x00000001L /* chunk is a 'LIST' */
#define AVIIF_KEYFRAME      0x00000010L /* this frame is a key frame.*/
#define AVIIF_NOTIME        0x00000100L /* this frame doesn't take any time */
#define AVIIF_COMPUSE       0x0FFF0000L /* these bits are for compressor use */

#define AVIIF_FIXKEYFRAME   0x00001000L /* invented; used to say that */
                                        /* the keyframe flag isn't a true flag */
                                        /* but have to be verified */

#define AVI_CHUNK_COMMON           \
    vlc_fourcc_t i_chunk_fourcc;   \
    uint64_t i_chunk_size;         \
    uint64_t i_chunk_pos;          \
    union  avi_chunk_u *p_next;    \
    union  avi_chunk_u *p_father;  \
    union  avi_chunk_u *p_first;

#define AVI_CHUNK( p_chk ) (avi_chunk_t*)(p_chk)

typedef struct idx1_entry_s
{
    vlc_fourcc_t i_fourcc;
    uint32_t i_flags;
    uint32_t i_pos;
    uint32_t i_length;

} idx1_entry_t;

typedef struct avi_chunk_common_s
{
    AVI_CHUNK_COMMON
} avi_chunk_common_t;

typedef struct avi_chunk_list_s
{
    AVI_CHUNK_COMMON
    vlc_fourcc_t i_type;
} avi_chunk_list_t;

typedef struct avi_chunk_idx1_s
{
    AVI_CHUNK_COMMON
    unsigned int i_entry_count;
    unsigned int i_entry_max;
    idx1_entry_t *entry;

} avi_chunk_idx1_t;

typedef struct avi_chunk_avih_s
{
    AVI_CHUNK_COMMON
    uint32_t i_microsecperframe;
    uint32_t i_maxbytespersec;
    uint32_t i_reserved1; /* dwPaddingGranularity;    pad to multiples of this
                             size; normally 2K */
    uint32_t i_flags;
    uint32_t i_totalframes;
    uint32_t i_initialframes;
    uint32_t i_streams;
    uint32_t i_suggestedbuffersize;
    uint32_t i_width;
    uint32_t i_height;
    uint32_t i_scale;
    uint32_t i_rate;
    uint32_t i_start;
    uint32_t i_length;
} avi_chunk_avih_t;

typedef struct avi_chunk_strh_s
{
    AVI_CHUNK_COMMON
    vlc_fourcc_t i_type;
    uint32_t i_handler;
    uint32_t i_flags;
    uint32_t i_reserved1;    /* wPriority wLanguage */
    uint32_t i_initialframes;
    uint32_t i_scale;
    uint32_t i_rate;
    uint32_t i_start;
    uint32_t i_length;       /* In units above... */
    uint32_t i_suggestedbuffersize;
    uint32_t i_quality;
    uint32_t i_samplesize;
} avi_chunk_strh_t;

typedef struct
{
    AVI_CHUNK_COMMON
    int i_cat;
    union
    {
        VLC_BITMAPINFOHEADER *p_bih;
        WAVEFORMATEX         *p_wf;
    } u;
} avi_chunk_strf_t;

typedef struct avi_chunk_strd_s
{
    AVI_CHUNK_COMMON
    uint8_t  *p_data;
} avi_chunk_strd_t;

typedef struct avi_chunk_vprp_s
{
    AVI_CHUNK_COMMON
    uint32_t i_video_format_token;
    uint32_t i_video_standard;
    uint32_t i_vertical_refresh;
    uint32_t i_h_total_in_t;
    uint32_t i_v_total_in_lines;
    uint32_t i_frame_aspect_ratio;
    uint32_t i_frame_width_in_pixels;
    uint32_t i_frame_height_in_pixels;
    uint32_t i_nb_fields_per_frame;
    struct
    {
        uint32_t i_compressed_bm_height;
        uint32_t i_compressed_bm_width;
        uint32_t i_valid_bm_height;
        uint32_t i_valid_bm_width;
        uint32_t i_valid_bm_x_offset;
        uint32_t i_valid_bm_y_offset;
        uint32_t i_video_x_offset_in_t;
        uint32_t i_video_y_valid_start_line;
    } field_info[2];

} avi_chunk_vprp_t;

typedef struct avi_chunk_dmlh_s
{
    AVI_CHUNK_COMMON
    uint32_t dwTotalFrames;
} avi_chunk_dmlh_t;

#define AVI_ZEROSIZED_CHUNK     0xFF
#define AVI_ZERO_FOURCC         0xFE

#define AVI_INDEX_OF_INDEXES    0x00
#define AVI_INDEX_OF_CHUNKS     0x01
#define AVI_INDEX_IS_DATA       0x80

#define AVI_INDEX_2FIELD        0x01
typedef struct
{
    uint32_t i_offset;
    uint32_t i_size;
} indx_std_entry_t;

typedef struct
{
    uint32_t i_offset;
    uint32_t i_size;
    uint32_t i_offsetfield2;
} indx_field_entry_t;

typedef struct
{
    uint64_t i_offset;
    uint32_t i_size;
    uint32_t i_duration;
} indx_super_entry_t;

typedef struct avi_chunk_indx_s
{
    AVI_CHUNK_COMMON
    int16_t  i_longsperentry;
    int8_t   i_indexsubtype;
    int8_t   i_indextype;
    uint32_t i_entriesinuse;
    vlc_fourcc_t i_id;

    uint64_t i_baseoffset;

    union
    {
        indx_std_entry_t    *std;
        indx_field_entry_t  *field;
        indx_super_entry_t  *super;
    } idx;
} avi_chunk_indx_t;

typedef struct avi_chunk_STRING_s
{
    AVI_CHUNK_COMMON
    char *p_type;
    char *p_str;
} avi_chunk_STRING_t;

typedef union avi_chunk_u
{
    avi_chunk_common_t  common;
    avi_chunk_list_t    list;
    avi_chunk_idx1_t    idx1;
    avi_chunk_avih_t    avih;
    avi_chunk_strh_t    strh;
    avi_chunk_strf_t    strf;
    avi_chunk_strd_t    strd;
    avi_chunk_vprp_t    vprp;
    avi_chunk_indx_t    indx;
    avi_chunk_STRING_t  strz;
} avi_chunk_t;

/****************************************************************************
 * Stream(input) access functions
 ****************************************************************************/
int     AVI_ChunkRead( stream_t *,
                       avi_chunk_t *p_chk,
                       avi_chunk_t *p_father );
void    AVI_ChunkClean( stream_t *, avi_chunk_t * );

int     AVI_ChunkCount_( avi_chunk_t *, vlc_fourcc_t, bool );
void   *AVI_ChunkFind_ ( avi_chunk_t *, vlc_fourcc_t, int, bool );

int     AVI_ChunkReadRoot( stream_t *, avi_chunk_t *p_root );
void    AVI_ChunkFreeRoot( stream_t *, avi_chunk_t *p_chk  );
int     AVI_ChunkFetchIndexes( stream_t *, avi_chunk_t *p_riff );

#define AVI_ChunkCount( p_chk, i_fourcc, b_list ) \
    AVI_ChunkCount_( AVI_CHUNK(p_chk), i_fourcc, b_list )
#define AVI_ChunkFind( p_chk, i_fourcc, i_number, b_list ) \
    AVI_ChunkFind_( AVI_CHUNK(p_chk), i_fourcc, i_number, b_list )

/* *** avi stuff *** */

#define AVIFOURCC_RIFF         VLC_FOURCC('R','I','F','F')
#define AVIFOURCC_ON2          VLC_FOURCC('O','N','2',' ')
#define AVIFOURCC_LIST         VLC_FOURCC('L','I','S','T')
#define AVIFOURCC_JUNK         VLC_FOURCC('J','U','N','K')
#define AVIFOURCC_AVI          VLC_FOURCC('A','V','I',' ')
#define AVIFOURCC_AVIX         VLC_FOURCC('A','V','I','X')
#define AVIFOURCC_ON2f         VLC_FOURCC('O','N','2','f')
#define AVIFOURCC_WAVE         VLC_FOURCC('W','A','V','E')
#define AVIFOURCC_INFO         VLC_FOURCC('I','N','F','O')

#define AVIFOURCC_avih         VLC_FOURCC('a','v','i','h')
#define AVIFOURCC_ON2h         VLC_FOURCC('O','N','2','h')
#define AVIFOURCC_hdrl         VLC_FOURCC('h','d','r','l')
#define AVIFOURCC_movi         VLC_FOURCC('m','o','v','i')
#define AVIFOURCC_idx1         VLC_FOURCC('i','d','x','1')

#define AVIFOURCC_strl         VLC_FOURCC('s','t','r','l')
#define AVIFOURCC_strh         VLC_FOURCC('s','t','r','h')
#define AVIFOURCC_strf         VLC_FOURCC('s','t','r','f')
#define AVIFOURCC_strd         VLC_FOURCC('s','t','r','d')
#define AVIFOURCC_strn         VLC_FOURCC('s','t','r','n')
#define AVIFOURCC_indx         VLC_FOURCC('i','n','d','x')
#define AVIFOURCC_vprp         VLC_FOURCC('v','p','r','p')
#define AVIFOURCC_dmlh         VLC_FOURCC('d','m','l','h')

#define AVIFOURCC_rec          VLC_FOURCC('r','e','c',' ')
#define AVIFOURCC_auds         VLC_FOURCC('a','u','d','s')
#define AVIFOURCC_vids         VLC_FOURCC('v','i','d','s')
#define AVIFOURCC_txts         VLC_FOURCC('t','x','t','s')
#define AVIFOURCC_mids         VLC_FOURCC('m','i','d','s')
#define AVIFOURCC_iavs         VLC_FOURCC('i','a','v','s')
#define AVIFOURCC_ivas         VLC_FOURCC('i','v','a','s')

#define AVIFOURCC_IARL         VLC_FOURCC('I','A','R','L')
#define AVIFOURCC_IART         VLC_FOURCC('I','A','R','T')
#define AVIFOURCC_ICMS         VLC_FOURCC('I','C','M','S')
#define AVIFOURCC_ICMT         VLC_FOURCC('I','C','M','T')
#define AVIFOURCC_ICOP         VLC_FOURCC('I','C','O','P')
#define AVIFOURCC_ICRD         VLC_FOURCC('I','C','R','D')
#define AVIFOURCC_ICRP         VLC_FOURCC('I','C','R','P')
#define AVIFOURCC_IDIM         VLC_FOURCC('I','D','I','M')
#define AVIFOURCC_IDPI         VLC_FOURCC('I','D','P','I')
#define AVIFOURCC_IENG         VLC_FOURCC('I','E','N','G')
#define AVIFOURCC_IGNR         VLC_FOURCC('I','G','N','R')
#define AVIFOURCC_ISGN         VLC_FOURCC('I','S','G','N')
#define AVIFOURCC_IKEY         VLC_FOURCC('I','K','E','Y')
#define AVIFOURCC_ILGT         VLC_FOURCC('I','L','G','T')
#define AVIFOURCC_IMED         VLC_FOURCC('I','M','E','D')
#define AVIFOURCC_INAM         VLC_FOURCC('I','N','A','M')
#define AVIFOURCC_IPLT         VLC_FOURCC('I','P','L','T')
#define AVIFOURCC_IPRD         VLC_FOURCC('I','P','R','D')
#define AVIFOURCC_ISBJ         VLC_FOURCC('I','S','B','J')
#define AVIFOURCC_ISFT         VLC_FOURCC('I','S','F','T')
#define AVIFOURCC_ISHP         VLC_FOURCC('I','S','H','P')
#define AVIFOURCC_ISRC         VLC_FOURCC('I','S','R','C')
#define AVIFOURCC_ISRF         VLC_FOURCC('I','S','R','F')
#define AVIFOURCC_ITCH         VLC_FOURCC('I','T','C','H')
#define AVIFOURCC_ISMP         VLC_FOURCC('I','S','M','P')
#define AVIFOURCC_IDIT         VLC_FOURCC('I','D','I','T')
#define AVIFOURCC_ILNG         VLC_FOURCC('I','L','N','G')
#define AVIFOURCC_IRTD         VLC_FOURCC('I','R','T','D')
#define AVIFOURCC_IWEB         VLC_FOURCC('I','W','E','B')
#define AVIFOURCC_IPRT         VLC_FOURCC('I','P','R','T')
#define AVIFOURCC_IWRI         VLC_FOURCC('I','W','R','I')
#define AVIFOURCC_IPRO         VLC_FOURCC('I','P','R','O')
#define AVIFOURCC_ICNM         VLC_FOURCC('I','C','N','M')
#define AVIFOURCC_IPDS         VLC_FOURCC('I','P','D','S')
#define AVIFOURCC_IEDT         VLC_FOURCC('I','E','D','T')
#define AVIFOURCC_ICDS         VLC_FOURCC('I','C','D','S')
#define AVIFOURCC_IMUS         VLC_FOURCC('I','M','U','S')
#define AVIFOURCC_ISTD         VLC_FOURCC('I','S','T','D')
#define AVIFOURCC_IDST         VLC_FOURCC('I','D','S','T')
#define AVIFOURCC_ICNT         VLC_FOURCC('I','C','N','T')
#define AVIFOURCC_ISTR         VLC_FOURCC('I','S','T','R')
#define AVIFOURCC_IFRM         VLC_FOURCC('I','F','R','M')

#define AVIFOURCC_IAS1         VLC_FOURCC('I','A','S','1')
#define AVIFOURCC_IAS2         VLC_FOURCC('I','A','S','2')
#define AVIFOURCC_IAS3         VLC_FOURCC('I','A','S','3')
#define AVIFOURCC_IAS4         VLC_FOURCC('I','A','S','4')
#define AVIFOURCC_IAS5         VLC_FOURCC('I','A','S','5')
#define AVIFOURCC_IAS6         VLC_FOURCC('I','A','S','6')
#define AVIFOURCC_IAS7         VLC_FOURCC('I','A','S','7')
#define AVIFOURCC_IAS8         VLC_FOURCC('I','A','S','8')
#define AVIFOURCC_IAS9         VLC_FOURCC('I','A','S','9')

#define AVITWOCC_wb            VLC_TWOCC('w','b')
#define AVITWOCC_db            VLC_TWOCC('d','b')
#define AVITWOCC_dc            VLC_TWOCC('d','c')
#define AVITWOCC_pc            VLC_TWOCC('p','c')
#define AVITWOCC_AC            VLC_TWOCC('A','C')
#define AVITWOCC_tx            VLC_TWOCC('t','x')
#define AVITWOCC_sb            VLC_TWOCC('s','b')

/* *** codex stuff ***  */

/* DV */
#define FOURCC_dvsd         VLC_FOURCC('d','v','s','d')
#define FOURCC_dvhd         VLC_FOURCC('d','v','h','d')
#define FOURCC_dvsl         VLC_FOURCC('d','v','s','l')
#define FOURCC_dv25         VLC_FOURCC('d','v','2','5')
#define FOURCC_dv50         VLC_FOURCC('d','v','5','0')
