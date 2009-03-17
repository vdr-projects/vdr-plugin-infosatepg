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
    int n = getline(&tempbuffer, &tempsize, f);
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
