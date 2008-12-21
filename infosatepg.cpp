/*
 * infosatepg.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/channels.h>
#include <vdr/transfer.h>
#include <vdr/shutdown.h>
#include <vdr/eitscan.h>
#include <getopt.h>

#include "infosatepg.h"
#include "setup.h"
#include "process.h"

// --- cPluginInfosatepg
cPluginInfosatepg::cPluginInfosatepg(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    statusMonitor=NULL;
    global=new cGlobalInfosatepg;
}

cPluginInfosatepg::~cPluginInfosatepg()
{
    // Clean up after yourself!
    if (statusMonitor) delete statusMonitor;
    delete global;
}

const char *cPluginInfosatepg::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return "  -d DIR,   --dir=DIR      use DIR as infosat data directory\n"
           "                           (default: /tmp)\n";
}

bool cPluginInfosatepg::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    static struct option long_options[] =
    {
        { "dir",      required_argument, NULL, 'd'
        },
        { NULL }
    };

    int c;
    while ((c = getopt_long(argc, argv, "d:", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'd':
            if (global->SetDirectory(optarg))
            {
                isyslog("infosatepg: using directory '%s' for data",optarg);
            }
            else
            {
                fprintf(stderr,"infosatepg: can't access data directory: %s\n",
                        optarg);
                return false;
            }
            break;
        default:
            return false;
        }
    }
    return true;
}

bool cPluginInfosatepg::Initialize(void)
{
    // Initialize any background activities the plugin shall perform.
    return true;
}

bool cPluginInfosatepg::Start(void)
{
    // Start any background activities the plugin shall perform
    if (global->Load()==-1)
    {
        isyslog("infosatepg: failed to load plugin status");
    }
    statusMonitor = new cStatusInfosatepg(global);

#ifdef INFOSATEPG_DEBUG
    global->Infosatdata[2].Debug(global->Directory());
    cProcessInfosatepg process(2,global);
#endif
    return true;
}

void cPluginInfosatepg::Stop(void)
{
    // Stop any background activities the plugin is performing.
    if (global->Save()==-1)
    {
        isyslog("infosatepg: failed to save plugin status");
    }
}

void cPluginInfosatepg::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
    int numProcessed=0;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        if (global->Infosatdata[mac].isReady2Process())
        {
            isyslog("infosatepg: janitor found data to be processed: day=%i month=%i",
                    global->Infosatdata[mac].Day(),global->Infosatdata[mac].Month());
            cProcessInfosatepg process(mac,global);
        }
        if (global->Infosatdata[mac].wasProcessed())
        {
            numProcessed++;
        }
    }
    if (numProcessed==EPG_DAYS)
    {
        global->Lock(time(NULL));
    }
}

void cPluginInfosatepg::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    if ((!global->isWaitOk()) || (global->isSwitched()) || (global->isLocked())) return;

    cChannel *chan=Channels.GetByNumber(global->Channel);
    if (!chan) return;

    for (int i=0; i<cDevice::NumDevices(); i++)
    {
        cDevice *dev=cDevice::GetDevice(i);
        if (dev)
        {
            bool live=false;
            if (dev->IsTunedToTransponder(chan)) return; // device is already tuned to transponder -> ok
            if (!dev->ProvidesTransponder(chan)) continue; // device cannot provide transponder -> skip
            if (EITScanner.UsesDevice(dev)) continue; // EITScanner is updating EPG -> skip
            if (dev->Receiving()) continue; // device is recording -> skip

            if (dev->IsPrimaryDevice())
            {
                // just use primary ff-card if inactive
                if (!ShutdownHandler.IsUserInactive()) continue; // not idle -> skip
            }

            if (cDevice::ActualDevice()->CardIndex()==i)
            {
                // LIVE device without recording, just use if inactive
                if (!ShutdownHandler.IsUserInactive()) continue; // not idle -> skip
                live=true;
            }

            // ok -> use this device
            dsyslog("infosatepg: found free device %i",dev->DeviceNumber()+1);
            dev->SwitchChannel(chan,live);
            global->SetTimer();
            return;
        }
    }
}

cString cPluginInfosatepg::Active(void)
{
    // Returns a message string if shutdown should be postponed
    if (!global->Locked())
        return tr("Infosat plugin still working");
    return NULL;
}

time_t cPluginInfosatepg::WakeupTime(void)
{
    // Returns custom wakeup time for shutdown script
    if (!global->WakeupTime) return 0;
    time_t Now = time(NULL);
    time_t Time = cTimer::SetTime(Now,cTimer::TimeToInt(global->WakeupTime));
    if (Time <= Now)
       Time = cTimer::IncDay(Time,1);
    return Time;
}

cOsdObject *cPluginInfosatepg::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    return NULL;
}

cMenuSetupPage *cPluginInfosatepg::SetupMenu(void)
{
    // Returns the setup menu.
    return new cMenuSetupInfosatepg(global);
}

bool cPluginInfosatepg::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    if      (!strcasecmp(Name,"Channel")) global->Channel=atoi(Value);
    else if (!strcasecmp(Name,"Pid")) global->Pid=atoi(Value);
    else if (!strcasecmp(Name,"WaitTime")) global->WaitTime=atoi(Value);
    else if (!strcasecmp(Name,"EventTimeDiff")) global->EventTimeDiff=60*atoi(Value);
    else if (!strcasecmp(Name,"WakeupTime")) global->WakeupTime=atoi(Value);
    else if (!strncasecmp(Name,"Channel",7))
    {
        if (strlen(Name)<10) return false;
        tChannelID ChannelID=tChannelID::FromString(&Name[8]);
        if (ChannelID==tChannelID::InvalidID) return false;
        global->AddChannel(ChannelID,atoi(Value));
    }
    else return false;
    return true;
}

bool cPluginInfosatepg::Service(const char *Id, void *Data)
{
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginInfosatepg::SVDRPHelpPages(void)
{
    // Returns help text for SVDRP
    static const char *HelpPages[] =
    {
        "STATE\n"
        "    Return actual state of the plugin",
        NULL
    };
    return HelpPages;
}

cString cPluginInfosatepg::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    // Process SVDRP commands
    char *output=NULL;
    if (!strcasecmp(Command,"STATE"))
    {
        int day,month;
        asprintf(&output,"InfosatEPG state:\n");
        if (global->isLocked(&day,&month))
            asprintf(&output,"%s Locked: yes (%02i.%02i)",output,day,month);
        else
            asprintf(&output,"%s Locked: no",output);
        asprintf(&output,"%s Switched: %s\n",output,global->isSwitched() ? "yes" : "no");
        for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
        {
            asprintf(&output,"%s Day %i (%02i.%02i.): %3i%% %s\n",
                     output,mac,global->Infosatdata[mac].Day(),global->Infosatdata[mac].Month(),
                     global->Infosatdata[mac].ReceivedPercent(),
                     global->Infosatdata[mac].wasProcessed() ? "processed" : "");
        }
    }
    return output;
}

VDRPLUGINCREATOR(cPluginInfosatepg); // Don't touch this!
