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

#include <stdio.h>
#include <stdlib.h>
#include "img_utils.h"

/* ***********************************************************************
 * FUNCTION: CheckImage()
 * DESCRIPTION: bit compare image data of 2 buffer
 * INPUTS:
 *          uint8_t *image : pointer to 1st image data
 *          uint8_t *imageref : pointer to 2nd image data
 *          int32_t pitch : pitch of the image
 *          int32_t height : height of the image
 *          int32_t width : width of the image
 *
 * OUTPUTS:
 *          return value PASS/FAIL : 1 is PASS and 0 is FAIL
 ************************************************************************/
int32_t checkImage(uint8_t *img1, uint8_t *img2, int32_t width, int32_t height, int32_t pitch)
{
  int32_t indx, indy, index, flag = 0;
  for (indy = 0; indy < height; indy++)
  {
    for (indx = 0; indx < width; indx++)
    {
      index = indy * pitch + indx;
      if (img1[index] != img2[index])
      {
        flag = 1;
        printf("(%d, %d): (%d, %d)\n", indy, indx, img1[index], img2[index]);
      }
    }
  }
  return(flag);
}

/* ***********************************************************************
 * FUNCTION: readPGM()
 * DESCRIPTION: read image data from ppm file
 * INPUTS:
 *          const char *filename : pointer to file name array
 *
 * OUTPUTS:
 *          return value PGMImage * : pointer to the structure PPMImage, contains
 *          details of images such as base pointer to data, resolution and pixel depth
 ************************************************************************/
PGMImage *readPGM(const char *filename)
{
  char buff[16];
  PGMImage *img;
  FILE *fp;
  int c, x, y, rgb_comp_color;
  //open PPM file for reading
  fp = fopen(filename, "rb");
  if (!fp)
  {
    fprintf(stderr, "Unable to open file '%s'\n", filename);
    exit(1);
  }

  //read image format
  if (!fgets(buff, sizeof(buff), fp))
  {
    perror(filename);
    exit(1);
  }

  //check the image format
  if (buff[0] != 'P' || buff[1] != '5')
  {
    fprintf(stderr, "Invalid image format (must be 'P5')\n");
    exit(1);
  }

  //check for comments
  c = getc(fp);
  while (c == '#')
  {
    while (getc(fp) != '\n')
    {
      ;
    }
    c = getc(fp);
  }

  ungetc(c, fp);
  //read image size information
  if (fscanf(fp, "%d %d", &x, &y) != 2)
  {
    fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
    exit(1);
  }

  //read rgb component
  if (fscanf(fp, "%d", &rgb_comp_color) != 1)
  {
    fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
    exit(1);
  }

  //check rgb component depth
  if (rgb_comp_color > (65535))
  {
    fprintf(stderr, "'%s' does not have 8/16-bits components\n", filename);
    exit(1);
  }

  //alloc memory form image
  img = (PGMImage *) malloc(sizeof(PGMImage));
  if (!img)
  {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(1);
  }
  img->x = x;
  img->y = y;

  img->compWidth = 8;
  while (fgetc(fp) != '\n')
  {
    ;
  }
  //memory allocation for pixel data
  img->data = (void *) malloc(img->x * img->y);

  if (!img->data)
  {
    fprintf(stderr, "Unable to allocate memory\n");
    exit(1);
  }

  //read pixel data from file
  if (fread(img->data, img->x, img->y, fp) != (unsigned)img->y)
  {
    fprintf(stderr, "Error loading image '%s'\n", filename);
    exit(1);
  }

  fclose(fp);
  return(img);
}

/* ***********************************************************************
 * FUNCTION: writePGM()
 * DESCRIPTION: write pgm data into a file.
 * INPUTS:
 *      const char *filename  : file name
 *      PGMImage *img       : pointer to image data to be written into a file
 *
 * OUTPUTS:
 *      NONE
 ************************************************************************/
void writePGM(const char *filename, PGMImage *img)
{
  FILE *fp;

  //open file for output
  fp = fopen(filename, "wb");

  if (!fp)
  {
    fprintf(stderr, "Unable to open file '%s'\n", filename);
    exit(1);
  }

  //write the header file
  //image format
  fprintf(fp, "P5\n");

  //comments
  fprintf(fp, "# %s\n", CREATOR);

  //image size
  fprintf(fp, "%d %d\n", img->x, img->y);

  // rgb component depth
  if (img->compWidth == 8)
  {
    fprintf(fp, "%d\n", 255);
    fwrite(img->data, img->x, img->y, fp);
  }
  fclose(fp);
}

