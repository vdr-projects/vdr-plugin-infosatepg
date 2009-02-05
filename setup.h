/*
 * setup.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __setup_h_
#define __setup_h_

#include <vdr/plugin.h>
#include <vdr/menuitems.h>
#include "global.h"

class cMenuSetupInfosatepg : public cMenuSetupPage
{
private:
  cGlobalInfosatepg *global;
  //int newChannel;
  int newWaitTime;
  int newWakeupTime;
  int newEventTimeDiff;
  int newFrequency;
  char newPolarization;
  int newSrate;
  int newPid;
  int newNoWakeup;
  int chanCurrent;
protected:
  virtual void Store(void);
private:
  cOsdItem *NewTitle(const char *s);
  eOSState Edit(void);
public:
  cMenuSetupInfosatepg(cGlobalInfosatepg *Global);
  virtual eOSState ProcessKey(eKeys Key);
};

class cMenuSetupChannelMenu : public cMenuSetupPage
{
private:
  cGlobalInfosatepg *global;
  int newDays;
  int newChannelUse;
  int index;
  cChannel *channel;
protected:
  virtual void Store(void);
public:
  cMenuSetupChannelMenu(cGlobalInfosatepg *Global, int Index);
};

#endif
