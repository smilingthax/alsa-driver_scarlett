/*
 * PC-Speaker driver for Linux
 *
 * Volume conversion tables, sine-based amplification.
 * Copyright (C) 2001-2007  Stas Sergeev
 */

#ifndef __PCSP_TABS_H
#define __PCSP_TABS_H

extern const unsigned char pcsp_tabs[][256];
extern const unsigned int pcsp_max_gain;

#endif
