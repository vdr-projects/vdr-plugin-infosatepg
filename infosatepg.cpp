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

#if __GNUC__ > 3
#define UNUSED(v) UNUSED_ ## v __attribute__((unused))
#else
#define UNUSED(x) x
#endif

extern char *strcatrealloc(char *dest, const char *src);

// --- cPluginInfosatepg
cPluginInfosatepg::cPluginInfosatepg(void)
{
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    statusMonitor=NULL;
    global=new cGlobalInfosatepg;
    process=new cProcessInfosatepg();
    pmac=EPG_FIRST_DAY_MAC;
}

cPluginInfosatepg::~cPluginInfosatepg()
{
    // Clean up after yourself!
    if (statusMonitor) delete statusMonitor;
    delete process;
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
    // Command line argument processing
    static struct option long_options[] =
    {
        { "dir",      required_argument, NULL, 'd'
        },
        { 0,0,0,0 }
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
    // Initialize plugin - check channellist
    tChannelID ChannelID;
    for (int i=0; i<global->InfosatChannels(); i++)
    {
        ChannelID=global->GetChannelID(i);
        if (!Channels.GetByChannelID(ChannelID))
        {
            dsyslog("infosatepg: remove %s",*ChannelID.ToString());
            global->RemoveChannel(i);
            // Removing entries from setup.conf is not possible!
        }
    }
    if (!global->FindReceiverChannel())
        esyslog("infosatepg: found no channel to receive, check setup");
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
    return true;
}

void cPluginInfosatepg::Stop(void)
{
    // Stop any background activities the plugin is performing.
    process->Stop();
    if (global->Save()==-1)
    {
        isyslog("infosatepg: failed to save plugin status");
    }
}

void cPluginInfosatepg::Housekeeping(void)
{
    // Perform any cleanup or other regular tasks.
}

void cPluginInfosatepg::MainThreadHook(void)
{
    // Perform actions in the context of the main program thread.
    if (!global->WaitOk()) return;

    if ((global->Infosatdata[pmac].ReceivedAll()) && (!global->ProcessedAll()))
    {
        if (!global->Infosatdata[pmac].Processed)
        {
            if (!process->Active())
            {
                isyslog ("infosatepg: found data to be processed: day=%i month=%i",
                         global->Infosatdata[pmac].Day(),global->Infosatdata[pmac].Month());

                process->SetInfo(pmac,global);
                process->Start();
            }
        }
        else
        {
            pmac++;
        }
        global->SetWaitTimer();
    }
    else
    {
        pmac=EPG_FIRST_DAY_MAC;
    }

    if ((global->Switched()) || (global->ReceivedAll()) || (global->Channel()==-1)) return;

    cChannel *chan=Channels.GetByNumber(global->Channel());
    if (!chan) return;

    if (ShutdownHandler.IsUserInactive())
    {
        // only switch primary device if we are not recording
        if (!cDevice::PrimaryDevice()->Receiving())
        {
            // first keep the current channel in "mind"
            if (global->LastCurrentChannel==-1) global->LastCurrentChannel=
                    cDevice::PrimaryDevice()->CurrentChannel();

            // we are idle -> try to use live device if we can
            if (cDevice::PrimaryDevice()->SwitchChannel(chan,true))
            {
                global->SetWaitTimer();
                return;
            }
        }
    }
    if (global->LastCurrentChannel!=-1)  global->LastCurrentChannel=-1;

    // Cannot use live device try another (if possible)

    for (int i=0; i<cDevice::NumDevices(); i++)
    {
        cDevice *dev=cDevice::GetDevice(i);
        if (dev)
        {
            if (!dev->ProvidesTransponder(chan)) continue; // device cannot provide transponder -> skip
            if (EITScanner.UsesDevice(dev)) continue; // EITScanner is updating EPG -> skip
            if (dev->Receiving()) continue; // device is recording -> skip
            if (dev->IsPrimaryDevice()) continue; // device is primary -> skip
            if (cDevice::ActualDevice()->CardIndex()==i) continue; // device is live viewing -> skip
            if (dev->IsTunedToTransponder(chan))
            {
                // we already have a device which is tuned (maybe switched manually?)
                // the filter will be added in status.cpp
                global->SetWaitTimer();
                return;
            }

            // ok -> use this device
            dsyslog("infosatepg: found free device %i",dev->DeviceNumber()+1);
            dev->SwitchChannel(chan,false);
            global->SetWaitTimer();
            return;
        }
    }

}

cString cPluginInfosatepg::Active(void)
{
    // Returns a message string if we are not ready

    if (global->NoDeferredShutdown) return NULL;

    // if we cannot receive, we shouldn't wait
    if (global->Channel()==-1) return NULL;

    if (!global->ProcessedAll())
        return tr("Infosat plugin still working");

    // we are done
    if (global->LastCurrentChannel!=-1)
    {
        // we switched from users last channel
        if (cDevice::PrimaryDevice()->CurrentChannel()==global->Channel())
        {
            // we are still on infosatepg channel
            cChannel *chan=Channels.GetByNumber(global->LastCurrentChannel);
            if (chan)
            {
                // switch back to users last viewed channel
                cDevice::PrimaryDevice()->SwitchChannel(chan,true);
                global->LastCurrentChannel=-1;
            }
        }
    }
    return NULL;
}

time_t cPluginInfosatepg::WakeupTime(void)
{
    // Returns custom wakeup time for shutdown script

    if (global->NoWakeup) return 0; // user option set -> don't wake up
    if (global->Channel()==-1) return 0; // we cannot receive, so we don't need to wake up
    if (global->WakeupTime()==-1)
    {
        // no wakeup? set wakeup to 03:00
        global->SetWakeupTime(cTimer::SetTime(time(NULL),cTimer::TimeToInt(300)));
    }
    time_t Now = time(NULL);
    time_t Time = global->WakeupTime();
    if (difftime(Time,Now)<0)
    {
        // wakeup time is in the past -> add a day
        Time = cTimer::IncDay(Time,1);
    }
    return Time;
}

const char *cPluginInfosatepg::MainMenuEntry(void)
{
    if (global->HideMainMenu) return NULL;
    double overall=0;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        overall+=global->Infosatdata[mac].ReceivedPercent()*0.9;
        if (global->Infosatdata[mac].Processed) overall+=10;
    }
    overall/=EPG_LAST_DAY_MAC;
    static char buffer[1024];
    snprintf(buffer,sizeof(buffer)-1,"%s (%0.f%%)", tr("Menu$infosatepg"), overall);
    return buffer;
}

cOsdObject *cPluginInfosatepg::MainMenuAction(void)
{
    // Perform the action when selected from the main VDR menu.
    return NULL;
}

cMenuSetupPage *cPluginInfosatepg::SetupMenu(void)
{
    // Return the setup menu.
    return new cMenuSetupInfosatepg(global);
}

bool cPluginInfosatepg::SetupParse(const char *Name, const char *Value)
{
    // Parse your own setup parameters and store their values.
    if      (!strcasecmp(Name,"Pid")) global->Pid=atoi(Value);
    else if (!strcasecmp(Name,"Frequency")) global->Frequency=atoi(Value);
    else if (!strcasecmp(Name,"Polarization")) global->Polarization=atoi(Value);
    else if (!strcasecmp(Name,"Srate")) global->Srate=atoi(Value);
    else if (!strcasecmp(Name,"NoWakeup")) global->NoWakeup=atoi(Value);
    else if (!strcasecmp(Name,"NoDeferredShutdown")) global->NoDeferredShutdown=atoi(Value);
    else if (!strcasecmp(Name,"HideMainMenu")) global->HideMainMenu=atoi(Value);
    else if (!strcasecmp(Name,"WaitTime")) global->WaitTime=atoi(Value);
    else if (!strncasecmp(Name,"Channel",7))
    {
        if (strlen(Name)<10) return false;
        tChannelID ChannelID=tChannelID::FromString(&Name[8]);
        if (ChannelID==tChannelID::InvalidID) return false;
        int val=atoi(Value);
        global->AddChannel(ChannelID,(val & 0xFFFF),(val & 0xF0000)>>16);
    }
    else return false;
    return true;
}

bool cPluginInfosatepg::Service(const char *UNUSED(Id), void *UNUSED(Data))
{
    // Handle custom service requests from other plugins
    return false;
}

const char **cPluginInfosatepg::SVDRPHelpPages(void)
{
    // Returns help text for SVDRP
    static const char *HelpPages[] =
    {
        "STAT\n"
        "    Return actual state of the plugin",
        "RESR\n"
        "    Reset received all",
        "REPR\n"
        "    Reprocess again",
        "SAVE\n",
        "    Save state of plugin",
        NULL
    };
    return HelpPages;
}

cString cPluginInfosatepg::SVDRPCommand(const char *Command, const char *UNUSED(Option),
                                        int &UNUSED(ReplyCode))
{
    // Process SVDRP commands
    cString output;
    if (!strcasecmp(Command,"RESR"))
    {
        global->ResetReceivedAll();
        pmac=EPG_FIRST_DAY_MAC;

        output="Restarted receiver\n";
    }
    if (!strcasecmp(Command,"REPR"))
    {
        global->ResetProcessed();
        pmac=EPG_FIRST_DAY_MAC;

        output="Reprocess files\n";
    }
    if (!strcasecmp(Command,"SAVE"))
    {
        global->Save();
        output="State saved\n";
    }

    if (!strcasecmp(Command,"STAT"))
    {
        int day,month;
        cString head=cString::sprintf("InfosatEPG state:\n Switched: %s",global->Switched() ? "yes" : "no");

        cString lcc;
        if (global->LastCurrentChannel!=-1)
        {
            lcc=cString::sprintf(" Switchback to: %i\n", global->LastCurrentChannel);
        }
        else
        {
            lcc=" Switchback to: unset\n";
        }

        cString rall;
        if (global->ReceivedAll(&day,&month))
            rall=cString::sprintf(" Received all: yes (%02i.%02i.)",day,month);
        else
            rall=" Received all: no";

        cString pall;
        pall=cString::sprintf(" Processed all: %s\n Prevent shutdown until ready: %s\n",
                              global->ProcessedAll() ? "yes" : "no",
                              global->NoDeferredShutdown ? "no" : "yes");

        cString wut;
        if (global->WakeupTime()!=-1)
        {
            time_t wakeup = global->WakeupTime();
            wut=cString::sprintf(" WakeupTime: %s %s",ctime(&wakeup),global->NoWakeup ? "(blocked)" : "");
        }
        else
        {
            wut="%s WakeupTime: unset\n";
        }

        cString head2;
        head2="\n" \
              "      |        | missed  |            |            | unlocated\n" \
              " Day  | Date   | Packets | Received % | Processed  | Events\n" \
              "------+--------+---------+------------+------------+----------\n";

        cString mstr;
        char *fmstr=NULL;
        for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
        {
            mstr=cString::sprintf("%c %i   | %02i.%02i. |   %3i   |    %3i     |    %s     |  %3i\n",
                                  (global->ActualMac==mac) ? '*' : ' ',
                                  mac,global->Infosatdata[mac].Day(),
                                  global->Infosatdata[mac].Month(),
                                  global->Infosatdata[mac].Missed(),
                                  global->Infosatdata[mac].ReceivedPercent(),
                                  global->Infosatdata[mac].Processed ? "yes" : " no",
                                  global->Infosatdata[mac].Unlocated);
            fmstr=strcatrealloc(fmstr,mstr);
        }
        output=cString::sprintf("%s%s%s%s%s%s%s",*head,*lcc,*rall,*pall,*wut,*head2,fmstr);
    }

    return output;
}

VDRPLUGINCREATOR(cPluginInfosatepg); // Don't touch this!
