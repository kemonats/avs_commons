/*
 * AVSystem Commons Library
 *
 * Copyright (C) 2017 AVSystem <http://www.avsystem.com/>
 *
 * This code is free and open source software licensed under the MIT License.
 * See the LICENSE file for details.
 */

#ifndef AVS_COMMONS_UTILS_H
#define AVS_COMMONS_UTILS_H

#include <avsystem/commons/defs.h>

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Standard guarantees RAND_MAX to be at least 0x7fff so let's
 * use it as a base for random number generators.
 */
#define AVS_RAND_MAX 0x7fff

/**
 * Returns a pseudo-random integer from range [0, AVS_RAND_MAX]. It is
 * thread-safe.
 */
int avs_rand_r(unsigned int *seed);

/** Tests whether @p value is a power of two */
static inline bool avs_is_power_of_2(size_t value) {
    return value > 0 && !(value & (value - 1));
}

#ifdef	__cplusplus
}
#endif

#endif /* AVS_COMMONS_UTILS_H */
