/*
 * readline.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __readline_h_
#define __readline_h_

#include <sys/types.h>

class cReadLineInfosatepg {
private:
  char *buffer;
public:
  cReadLineInfosatepg(void);
  ~cReadLineInfosatepg();
  char *Read(FILE *f,size_t *size);
  };

#endif
