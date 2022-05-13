/*****************************************************************************
 * vlc_es.h: Elementary stream formats descriptions
 *****************************************************************************
 * Copyright (C) 1999-2012 VLC authors and VideoLAN
 * $Id: 3c8e04e1b15740166df2e0b2d9a651ffb2c5bc2f $
 *
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

#ifndef VLC_ES_H
#define VLC_ES_H 1

#include <vlc_common.h>
#include <vlc_fourcc.h>

/**
 * \file
 * This file defines the elementary streams format types
 */

/**
 * video palette data
 * \see video_format_t
 * \see subs_format_t
 */
#define VIDEO_PALETTE_COLORS_MAX 256

struct video_palette_t {
    int i_entries;                                /**< to keep the compatibility with libavcodec's palette */
    uint8_t palette[VIDEO_PALETTE_COLORS_MAX][4]; /**< 4-byte RGBA/YUVA palette */
};

/**
 * Picture orientation.
 */
typedef enum video_orientation_t {
    ORIENT_TOP_LEFT = 0, /**< Top line represents top, left column left. */
    ORIENT_TOP_RIGHT,    /**< Flipped horizontally */
    ORIENT_BOTTOM_LEFT,  /**< Flipped vertically */
    ORIENT_BOTTOM_RIGHT, /**< Rotated 180 degrees */
    ORIENT_LEFT_TOP,     /**< Transposed */
    ORIENT_LEFT_BOTTOM,  /**< Rotated 90 degrees clockwise */
    ORIENT_RIGHT_TOP,    /**< Rotated 90 degrees anti-clockwise */
    ORIENT_RIGHT_BOTTOM, /**< Anti-transposed */

    ORIENT_NORMAL = ORIENT_TOP_LEFT,
    ORIENT_TRANSPOSED = ORIENT_LEFT_TOP,
    ORIENT_ANTI_TRANSPOSED = ORIENT_RIGHT_BOTTOM,
    ORIENT_HFLIPPED = ORIENT_TOP_RIGHT,
    ORIENT_VFLIPPED = ORIENT_BOTTOM_LEFT,
    ORIENT_ROTATED_180 = ORIENT_BOTTOM_RIGHT,
    ORIENT_ROTATED_270 = ORIENT_LEFT_BOTTOM,
    ORIENT_ROTATED_90 = ORIENT_RIGHT_TOP,
} video_orientation_t;
/** Convert EXIF orientation to enum video_orientation_t */
#define ORIENT_FROM_EXIF(exif) ((0x57642310U >> (4 * ((exif) -1))) & 7)
/** Convert enum video_orientation_t to EXIF */
#define ORIENT_TO_EXIF(orient) ((0x76853421U >> (4 * (orient))) & 15)
/** If the orientation is natural or mirrored */
#define ORIENT_IS_MIRROR(orient) parity(orient)
/** If the orientation swaps dimensions */
#define ORIENT_IS_SWAP(orient) (((orient) &4) != 0)
/** Applies horizontal flip to an orientation */
#define ORIENT_HFLIP(orient) ((orient) ^ 1)
/** Applies vertical flip to an orientation */
#define ORIENT_VFLIP(orient) ((orient) ^ 2)
/** Applies horizontal flip to an orientation */
#define ORIENT_ROTATE_180(orient) ((orient) ^ 3)

typedef enum video_transform_t {
    TRANSFORM_IDENTITY = ORIENT_NORMAL,
    TRANSFORM_HFLIP = ORIENT_HFLIPPED,
    TRANSFORM_VFLIP = ORIENT_VFLIPPED,
    TRANSFORM_R180 = ORIENT_ROTATED_180,
    TRANSFORM_R270 = ORIENT_ROTATED_270,
    TRANSFORM_R90 = ORIENT_ROTATED_90,
    TRANSFORM_TRANSPOSE = ORIENT_TRANSPOSED,
    TRANSFORM_ANTI_TRANSPOSE = ORIENT_ANTI_TRANSPOSED
} video_transform_t;

/**
 * Video color primaries (a.k.a. chromacities)
 */
typedef enum video_color_primaries_t {
    COLOR_PRIMARIES_UNDEF,
    COLOR_PRIMARIES_BT601_525,
    COLOR_PRIMARIES_BT601_625,
    COLOR_PRIMARIES_BT709,
    COLOR_PRIMARIES_BT2020,
    COLOR_PRIMARIES_DCI_P3,
    COLOR_PRIMARIES_FCC1953,
#define COLOR_PRIMARIES_SRGB COLOR_PRIMARIES_BT709
#define COLOR_PRIMARIES_SMTPE_170 COLOR_PRIMARIES_BT601_525
#define COLOR_PRIMARIES_SMTPE_240 COLOR_PRIMARIES_BT601_525 /* Only differs from 1e10-4 in white Y */
#define COLOR_PRIMARIES_SMTPE_RP145 COLOR_PRIMARIES_BT601_525
#define COLOR_PRIMARIES_EBU_3213 COLOR_PRIMARIES_BT601_625
#define COLOR_PRIMARIES_BT470_BG COLOR_PRIMARIES_BT601_625
#define COLOR_PRIMARIES_BT470_M COLOR_PRIMARIES_FCC1953
#define COLOR_PRIMARIES_MAX COLOR_PRIMARIES_FCC1953
} video_color_primaries_t;

/**
 * Video transfer functions
 */
typedef enum video_transfer_func_t {
    TRANSFER_FUNC_UNDEF,
    TRANSFER_FUNC_LINEAR,
    TRANSFER_FUNC_SRGB /*< Gamma 2.2 */,
    TRANSFER_FUNC_BT470_BG,
    TRANSFER_FUNC_BT470_M,
    TRANSFER_FUNC_BT709,
    TRANSFER_FUNC_SMPTE_ST2084,
    TRANSFER_FUNC_SMPTE_240,
    TRANSFER_FUNC_HLG,
#define TRANSFER_FUNC_BT2020 TRANSFER_FUNC_BT709
#define TRANSFER_FUNC_SMPTE_170 TRANSFER_FUNC_BT709
#define TRANSFER_FUNC_SMPTE_274 TRANSFER_FUNC_BT709
#define TRANSFER_FUNC_SMPTE_293 TRANSFER_FUNC_BT709
#define TRANSFER_FUNC_SMPTE_296 TRANSFER_FUNC_BT709
#define TRANSFER_FUNC_ARIB_B67 TRANSFER_FUNC_HLG
#define TRANSFER_FUNC_MAX TRANSFER_FUNC_HLG
} video_transfer_func_t;

/**
 * Video color space (i.e. YCbCr matrices)
 */
typedef enum video_color_space_t {
    COLOR_SPACE_UNDEF,
    COLOR_SPACE_BT601,
    COLOR_SPACE_BT709,
    COLOR_SPACE_BT2020,
#define COLOR_SPACE_SRGB COLOR_SPACE_BT709
#define COLOR_SPACE_SMPTE_170 COLOR_SPACE_BT601
#define COLOR_SPACE_SMPTE_240 COLOR_SPACE_SMPTE_170
#define COLOR_SPACE_MAX COLOR_SPACE_BT2020
} video_color_space_t;

/**
 * Video chroma location
 */
typedef enum video_chroma_location_t {
    CHROMA_LOCATION_UNDEF,
    CHROMA_LOCATION_LEFT,   /*< Most common in MPEG-2 Video, H.264/265 */
    CHROMA_LOCATION_CENTER, /*< Most common in MPEG-1 Video, JPEG */
    CHROMA_LOCATION_TOP_LEFT,
    CHROMA_LOCATION_TOP_CENTER,
    CHROMA_LOCATION_BOTTOM_LEFT,
    CHROMA_LOCATION_BOTTOM_CENTER,
#define CHROMA_LOCATION_MAX CHROMA_LOCATION_BOTTOM_CENTER
} video_chroma_location_t;

/**
 * video format description
 */
struct video_format_t {
    void *extra_info;
	void *decoder_p;
    vlc_fourcc_t i_chroma; /**< picture chroma */

    unsigned int i_width;          /**< picture width */
    unsigned int i_height;         /**< picture height */
    unsigned int i_x_offset;       /**< start offset of visible area */
    unsigned int i_y_offset;       /**< start offset of visible area */
    unsigned int i_visible_width;  /**< width of visible area */
    unsigned int i_visible_height; /**< height of visible area */

    unsigned int i_bits_per_pixel; /**< number of bits per pixel */

    unsigned int i_sar_num; /**< sample/pixel aspect ratio */
    unsigned int i_sar_den;

    video_orientation_t orientation;         /**< picture orientation */
    video_color_primaries_t primaries;       /**< color primaries */
    video_transfer_func_t transfer;          /**< transfer function */
    video_color_space_t space;               /**< YCbCr color space */
    bool b_color_range_full;                 /**< 0-255 instead of 16-235 */
    video_chroma_location_t chroma_location; /**< YCbCr chroma location */

    struct plane_t {
        /* Variables used for fast memcpy operations */
        int i_lines; /**< Number of lines, including margins */
        int i_pitch; /**< Number of bytes in a line, including margins */

        /** Size of a macropixel, defaults to 1 */
        int i_pixel_pitch;

        /* Variables used for pictures with margins */
        int i_visible_lines; /**< How many visible lines are there ? */
        int i_visible_pitch; /**< How many visible pixels are there ? */
    };

    struct plane_t plane[VOUT_MAX_PLANES];
    int i_planes;
};

/**
 * Initialize a video_format_t structure with chroma 'i_chroma'
 * \param p_src pointer to video_format_t structure
 * \param i_chroma chroma value to use
 */
static inline void video_format_Init(video_format_t *p_src, vlc_fourcc_t i_chroma)
{
    memset(p_src, 0, sizeof(video_format_t));
    p_src->i_chroma = i_chroma;
}

/**
 * It will fill up a video_format_t using the given arguments.
 * Note that the video_format_t must already be initialized.
 */
void video_format_Setup(video_format_t *, vlc_fourcc_t i_chroma, int i_width, int i_height, int i_visible_width, int i_visible_height,
                        int i_sar_num, int i_sar_den);

/**
 * It will copy the crop properties from a video_format_t to another.
 */
void video_format_CopyCrop(video_format_t *, const video_format_t *);

#endif
