#include <vlc_fixups.h>
#include <vlc_common.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#if !defined (HAVE_POSIX_MEMALIGN)
# include <malloc.h>
#endif

void* aligned_alloc(size_t align, size_t size)
{
    /* align must be a power of 2 */
    /* size must be a multiple of align */
    if ((align & (align - 1)) || (size & (align - 1)))
    {
        errno = EINVAL;
        return NULL;
    }

#ifdef HAVE_POSIX_MEMALIGN
    if (align < sizeof(void*)) /* POSIX does not allow small alignment */
        align = sizeof(void*);

    void* ptr;
    int err = posix_memalign(&ptr, align, size);
    if (err)
    {
        errno = err;
        ptr = NULL;
    }
    return ptr;

#elif defined(HAVE_MEMALIGN)
    return memalign(align, size);
#elif defined (_WIN32) && defined(__MINGW32__)
    return __mingw_aligned_malloc(size, align);
#elif defined (_WIN32) && defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    #warning unsupported aligned allocation!
        if (size > 0)
            errno = ENOMEM;
    return NULL;
#endif
}

/*****************************************************************************
 * reduce a fraction
 *   (adapted from libavcodec, author Michael Niedermayer <michaelni@gmx.at>)
 *****************************************************************************/
bool vlc_ureduce(unsigned* pi_dst_nom, unsigned* pi_dst_den,
    uint64_t i_nom, uint64_t i_den, uint64_t i_max)
{
    bool b_exact = 1;
    uint64_t i_gcd;

    if (i_den == 0)
    {
        *pi_dst_nom = 0;
        *pi_dst_den = 1;
        return 1;
    }

    i_gcd = GCD(i_nom, i_den);
    i_nom /= i_gcd;
    i_den /= i_gcd;

    if (i_max == 0) i_max = INT64_C(0xFFFFFFFF);

    if (i_nom > i_max || i_den > i_max)
    {
        uint64_t i_a0_num = 0, i_a0_den = 1, i_a1_num = 1, i_a1_den = 0;
        b_exact = 0;

        for (; ; )
        {
            uint64_t i_x = i_nom / i_den;
            uint64_t i_a2n = i_x * i_a1_num + i_a0_num;
            uint64_t i_a2d = i_x * i_a1_den + i_a0_den;

            if (i_a2n > i_max || i_a2d > i_max) break;

            i_nom %= i_den;

            i_a0_num = i_a1_num; i_a0_den = i_a1_den;
            i_a1_num = i_a2n; i_a1_den = i_a2d;
            if (i_nom == 0) break;
            i_x = i_nom; i_nom = i_den; i_den = i_x;
        }
        i_nom = i_a1_num;
        i_den = i_a1_den;
    }

    *pi_dst_nom = (unsigned int)i_nom;
    *pi_dst_den = (unsigned int)i_den;

    return b_exact;
}
