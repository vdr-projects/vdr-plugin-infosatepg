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

// --- cReadLineInfosatepg ---------------------------------------------------

cReadLineInfosatepg::cReadLineInfosatepg(void)
{
  buffer = NULL;
}

cReadLineInfosatepg::~cReadLineInfosatepg()
{
  free(buffer);
}

char *cReadLineInfosatepg::Read(FILE *f,size_t *size)
{
  free(buffer); buffer=NULL;
  if ((!size) || (!f)) return NULL;
  bool ext=false;
  *size=0;
  do
  {
    char *tempbuffer=NULL;
    size_t tempsize=0;

    ext=false;
    int n = getline(&tempbuffer, &tempsize, f);
    if (n > 0)
    {
      if (tempbuffer[n-1] == '\n')
      {
        n--;
        tempbuffer[n] = 0;
        if (n > 0)
        {
          if (tempbuffer[n-1] == '\r')
            n--;
          tempbuffer[n] = 0;
        }
      }
      if (n>0)
      {
        if (tempbuffer[n-1] == '\\')
        {
          n--;
          tempbuffer[n]=0;
          ext=true;
        }
      }

      buffer=(char*) realloc(buffer,*size+(n+1));
      snprintf(&buffer[*size],n+1,tempbuffer);
      free(tempbuffer);
      *size+=n;
    }
  }
  while (ext==true);

  return buffer;
}
