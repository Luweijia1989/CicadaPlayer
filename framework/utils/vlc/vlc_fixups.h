#pragma once
#pragma once

#ifdef HAVE_CONFIG_H
# include <vlc_config_2.h>
#endif

#include <stdint.h>
#include <stdbool.h>

#ifndef HAVE_ALIGNED_ALLOC
void* aligned_alloc(size_t, size_t);
#endif

#if defined (_WIN32) && defined(__MINGW32__)
#define aligned_free(ptr)  __mingw_aligned_free(ptr)
#elif defined (_WIN32) && defined(_MSC_VER)
#define aligned_free(ptr)  _aligned_free(ptr)
#else
#define aligned_free(ptr)  free(ptr)
#endif

bool vlc_ureduce(unsigned*, unsigned*, uint64_t, uint64_t, uint64_t);