/*
 * Copyright (c) 2016 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */

/* *****************************************************************************
 * FILE:  img_utils.h
 *
 * DESCRIPTION:
 *
 *    This file contains prototype definitions of functions related to
 *    image utils routines that are being used defined in img_utils.c
 *
 * ****************************************************************************/

#include "commonDef.h"
#include <stdint.h>
#include "tileManager.h"

int32_t checkImage(uint8_t *img1, uint8_t *img2, int32_t width, int32_t height, int32_t pitch);
PGMImage *readPGM(const char *filename);
void writePGM(const char *filename, PGMImage *img);

