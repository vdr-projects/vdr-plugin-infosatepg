/*
 * status.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#ifndef __status_h_
#define __status_h_

#include <vdr/status.h>

#include "filter.h"
#include "global.h"

// --- cStatusInfosatepg
class cStatusInfosatepg : public cStatus {
private:
  cFilterInfosatepg *myFilter;
  cDevice *myFilterDevice;
  cGlobalInfosatepg *global;
protected:
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
public:
  cStatusInfosatepg(cGlobalInfosatepg *Global);
  virtual ~cStatusInfosatepg(void); 
 };

#endif
