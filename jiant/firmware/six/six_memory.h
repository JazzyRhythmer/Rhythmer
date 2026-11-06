/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#ifdef SIX_HOST
#include <string.h>
#else
#include <stddef.h>
void *memset(void *,int,size_t);
void *memcpy(void *,const void *,size_t);
int memcmp(const void *,const void *,size_t);
#endif
