/*****************************************************************************
 * vlc_common.h: common definitions
 * Collection of useful common types and macros definitions
 *****************************************************************************
 * Copyright (C) 1998-2011 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@via.ecp.fr>
 *          Vincent Seguin <seguin@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Rémi Denis-Courmont
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

/**
 * \file
 * This file is a collection of common definitions and types
 */

#ifndef VLC_COMMON_H
#define VLC_COMMON_H 1

/*****************************************************************************
 * Required system headers
 *****************************************************************************/
#include <stdarg.h>
#include <stdlib.h>

#include <cassert>
#include <ctype.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#define VLC_TS_INVALID INT64_C(0)
#define VOUT_MAX_PLANES 5

typedef struct video_format_t video_format_t;
typedef video_format_t video_frame_format_t;
typedef struct picture_t picture_t;
typedef struct picture_sys_t picture_sys_t;

/*****************************************************************************
  * Error values (shouldn't be exposed)
  *****************************************************************************/
#define VLC_SUCCESS (-0)  /**< No error */
#define VLC_EGENERIC (-1) /**< Unspecified error */
#define VLC_ENOMEM (-2)   /**< Not enough memory */
#define VLC_ETIMEOUT (-3) /**< Timeout */
#define VLC_ENOMOD (-4)   /**< Module not found */
#define VLC_ENOOBJ (-5)   /**< Object not found */
#define VLC_ENOVAR (-6)   /**< Variable not found */
#define VLC_EBADVAR (-7)  /**< Bad variable value */
#define VLC_ENOITEM (-8)  /**< Item not found */

typedef uint32_t vlc_fourcc_t;
typedef int64_t mtime_t;

#ifdef WORDS_BIGENDIAN
#define VLC_FOURCC(a, b, c, d) (((uint32_t) d) | (((uint32_t) c) << 8) | (((uint32_t) b) << 16) | (((uint32_t) a) << 24))
#define VLC_TWOCC(a, b) ((uint16_t)(b) | ((uint16_t)(a) << 8))

#else
#define VLC_FOURCC(a, b, c, d) (((uint32_t) a) | (((uint32_t) b) << 8) | (((uint32_t) c) << 16) | (((uint32_t) d) << 24))
#define VLC_TWOCC(a, b) ((uint16_t)(a) | ((uint16_t)(b) << 8))

#endif

typedef struct {
    unsigned num, den;
} vlc_rational_t;

/**
 * Translate a vlc_fourcc into its string representation. This function
 * assumes there is enough room in psz_fourcc to store 4 characters in.
 *
 * \param fcc a vlc_fourcc_t
 * \param psz_fourcc string to store string representation of vlc_fourcc in
 */
static inline void vlc_fourcc_to_char(vlc_fourcc_t fcc, char *psz_fourcc)
{
    memcpy(psz_fourcc, &fcc, 4);
}

/* __MAX and __MIN: self explanatory */
#ifndef __MAX
#define __MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __MIN
#define __MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static inline int64_t GCD(int64_t a, int64_t b)
{
    while (b) {
        int64_t c = a % b;
        a = b;
        b = c;
    }
    return a;
}

static int ffsll(long long x)
{
    for (unsigned i = 0; i < sizeof(x) * CHAR_BIT; i++)
        if ((x >> i) & 1) return i + 1;
    return 0;
}

#define GUID_FMT "0x%8.8x-0x%4.4x-0x%4.4x-0x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"
#define GUID_PRINT(guid)                                                                                                                   \
    (unsigned) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3],               \
            (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]


static int vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list args;
    int len;

    va_copy(args, ap);
    len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    char *str = (char *) malloc(len + 1);
    if (str != NULL) {
        int len2;

        va_copy(args, ap);
        len2 = vsprintf(str, fmt, args);
        assert(len2 == len);
        va_end(args);
    } else {
        len = -1;
#ifndef NDEBUG
        str = (char *) (intptr_t) 0x41414141; /* poison */
#endif
    }
    *strp = str;
    return len;
}

static int asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vasprintf(strp, fmt, ap);
    va_end(ap);
    return ret;
}


/* states: S_N: normal, S_I: comparing integral part, S_F: comparing
           fractionnal parts, S_Z: idem but with leading Zeroes only */
#define S_N 0x0
#define S_I 0x3
#define S_F 0x6
#define S_Z 0x9

/* result_type: CMP: return diff; LEN: compare using len_diff/diff */
#define CMP 2
#define LEN 3


/* Compare S1 and S2 as strings holding indices/version numbers,
   returning less than, equal to or greater than zero if S1 is less than,
   equal to or greater than S2 (for more info, see the texinfo doc).
*/

static inline int strverscmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    /* Symbol(s)    0       [1-9]   others
     Transition   (10) 0  (01) d  (00) x   */
    static const uint8_t next_state[] = {/* state    x    d    0  */
                                         /* S_N */ S_N, S_I, S_Z,
                                         /* S_I */ S_N, S_I, S_I,
                                         /* S_F */ S_N, S_F, S_F,
                                         /* S_Z */ S_N, S_F, S_Z};

    static const int8_t result_type[] = {/* state   x/x  x/d  x/0  d/x  d/d  d/0  0/x  0/d  0/0  */

                                         /* S_N */ CMP, CMP, CMP, CMP, LEN, CMP, CMP, CMP, CMP,
                                         /* S_I */ CMP, -1,  -1,  +1,  LEN, LEN, +1,  LEN, LEN,
                                         /* S_F */ CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
                                         /* S_Z */ CMP, +1,  +1,  -1,  CMP, CMP, -1,  CMP, CMP};

    if (p1 == p2) return 0;

    unsigned char c1 = *p1++;
    unsigned char c2 = *p2++;
    /* Hint: '0' is a digit too.  */
    int state = S_N + ((c1 == '0') + (isdigit(c1) != 0));

    int diff;
    while ((diff = c1 - c2) == 0) {
        if (c1 == '\0') return diff;

        state = next_state[state];
        c1 = *p1++;
        c2 = *p2++;
        state += (c1 == '0') + (isdigit(c1) != 0);
    }

    state = result_type[state * 3 + (((c2 == '0') + (isdigit(c2) != 0)))];

    switch (state) {
        case CMP:
            return diff;

        case LEN:
            while (isdigit(*p1++))
                if (!isdigit(*p2++)) return 1;

            return isdigit(*p2) ? -1 : diff;

        default:
            return state;
    }
}

static inline void *realloc_or_free(void *p, size_t sz)
{
    void *n = realloc(p, sz);
    if (!n) free(p);
    return n;
}

static inline unsigned(clz)(unsigned x)
{
#ifdef __GNUC__
    return __builtin_clz(x);
#else
    unsigned i = sizeof(x) * 8;

    while (x) {
        x >>= 1;
        i--;
    }
    return i;
#endif
}

#define clz8(x) (clz(x) - ((sizeof(unsigned) - sizeof(uint8_t)) * 8))
#define clz16(x) (clz(x) - ((sizeof(unsigned) - sizeof(uint16_t)) * 8))
/* XXX: this assumes that int is 32-bits or more */
#define clz32(x) (clz(x) - ((sizeof(unsigned) - sizeof(uint32_t)) * 8))

#endif /* !VLC_COMMON_H */
