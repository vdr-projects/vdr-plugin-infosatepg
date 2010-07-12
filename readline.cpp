/*
 * readline.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include "readline.h"

#ifdef __FreeBSD__
#include <string.h>
#if __FreeBSD_version > 800000
#define HAVE_GETLINE
#endif
#else
#define HAVE_GETLINE
#endif

extern char *strcatrealloc(char *dest, const char *src);

// --- cReadLineInfosatepg ---------------------------------------------------

cReadLineInfosatepg::cReadLineInfosatepg(void)
{
  buffer = NULL;
}

cReadLineInfosatepg::~cReadLineInfosatepg()
{
  if (buffer) free(buffer);
}

char *cReadLineInfosatepg::Read(FILE *f,size_t *size)
{
  if (buffer) free(buffer);
   buffer=NULL;
  if ((!size) || (!f)) return NULL;
  bool ext=false;
  *size=0;
  char *tempbuffer=NULL;
  size_t tempsize=0;
  do
  {
    ext=false;
#ifndef HAVE_GETLINE
    size_t n;

    if (!tempbuffer)
    {
      if (!(tempbuffer = (char *)malloc(tempsize = 4096)))
        return NULL;
    }
    if (!fgets(tempbuffer, tempsize, f))
    {
      if (tempbuffer) free(tempbuffer);
      return buffer;
    }
    while ((n = strlen(tempbuffer)) >= tempsize - 1 &&
        tempbuffer[n - 1] != '\n')
    {
      if (!(tempbuffer = (char *)realloc(tempbuffer, tempsize * 2)))
        return NULL;
      tempsize *= 2;
      if (!fgets(tempbuffer + n, tempsize - n, f))
        break;
    }
#else
    int n = getline(&tempbuffer, &tempsize, f);
#endif
    if (n > 0)
    {
      if (tempbuffer[n-1] == '\n')
      {
        tempbuffer[--n] = 0;
        if (n > 0)
        {
          if (tempbuffer[n-1] == '\r')
             tempbuffer[--n] = 0;
        }
      }
      if (n>0)
      {
        if (tempbuffer[n-1] == '\\')
        {
          tempbuffer[--n]=0;
          ext=true;
        }
      }
      if (n>0) {
         buffer=strcatrealloc(buffer,tempbuffer);
         //tempbuffer[0]=0;
         *size+=n;
      } else {
         //tempbuffer[0]=0;
         ext=true;
      }
    }
  }
  while (ext==true);
  if (tempbuffer) free(tempbuffer);
  return buffer;
}
