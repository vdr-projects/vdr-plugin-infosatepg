/*
 * infosatepg.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __infosatepg_h_
#define __infosatepg_h_

#include "global.h"
#include "status.h"
#include "process.h"

static const char *VERSION        = "0.0.8";
static const char *DESCRIPTION    = trNOOP("Read EPG info from infosat");

class cPluginInfosatepg : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cGlobalInfosatepg *global;
  cStatusInfosatepg *statusMonitor;
  cProcessInfosatepg *process;
  int pmac;
public:
  cPluginInfosatepg(void);
  virtual ~cPluginInfosatepg();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void);
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

#endif
