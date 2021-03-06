/*
 * setup.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "setup.h"

// --- cMenuSetupInfosatepg
cMenuSetupInfosatepg::cMenuSetupInfosatepg (cGlobalInfosatepg *Global)
{
    global=Global;

    newFrequency = global->Frequency;
    newPolarization = global->Polarization;
    newSrate = global->Srate;
    newPid = global->Pid;
    newWaitTime = global->WaitTime;
    newNoWakeup=global->NoWakeup;
    newNoDeferredShutdown=global->NoDeferredShutdown;
    newHideMainMenu=global->HideMainMenu;

    Add (NewTitle (tr ("Scan parameter")));
    cString buffer = cString::sprintf("%s:\t%s",tr("Source"), "S19.2E"); // just for info
    Add (new cOsdItem (buffer,osUnknown,false));
    cOsdItem *firstItem = new cMenuEditIntItem (tr ("Frequency"), &newFrequency);
    Add (firstItem);
    Add (new cMenuEditChrItem (tr ("Polarization"), &newPolarization,"hlvr"));
    Add (new cMenuEditIntItem (tr ("Srate"), &newSrate));
    Add (new cMenuEditIntItem (tr ("Pid"), &newPid,1,8191));

    if (global->Channel()>0)
    {
        buffer = cString::sprintf("-> %s:\t%i",tr("Using channel"), global->Channel());
    }
    else
    {
        buffer = cString::sprintf("-> %s:\t%s",tr("Using channel"),tr("none"));
    }
    Add (new cOsdItem (buffer,osUnknown,false));

    Add (NewTitle (tr ("Event options")));
    Add (new cMenuEditIntItem (tr ("Wait time [s]"), &newWaitTime,MIN_WAITTIME,MAX_WAITTIME));

    Add (NewTitle (tr ("General options")));
    Add (new cMenuEditBoolItem(tr("Hide main menu"),&newHideMainMenu));
    Add (new cMenuEditBoolItem(tr("Prevent wakeup"),&newNoWakeup));
    Add (new cMenuEditBoolItem(tr("Prevent deferred shutdown"),&newNoDeferredShutdown));

    if (global->InfosatChannels())
    {
        Add (NewTitle (tr ("Infosat channels")),true);
        chanCurrent=Current() +1;
        SetCurrent (firstItem);

        for (int i=0; i<global->InfosatChannels(); i++)
        {
            cChannel *chan = Channels.GetByChannelID (global->GetChannelID (i));
            if (!chan) continue;
            int chanuse=global->GetChannelUsage(i);
            cString buffer = cString::sprintf ("%s:\t%s",chan->Name(),chanuse ? tr ("used") : "");
            Add (new cOsdItem (buffer));
        }
    }
}

cOsdItem *cMenuSetupInfosatepg::NewTitle (const char *s)
{
    cString buffer = cString::sprintf("---- %s ----", s);
    return new cOsdItem (buffer,osUnknown,false);
}

void cMenuSetupInfosatepg::Store (void)
{
    bool bResetReceivedAll=false;

    if ((global->Frequency!=newFrequency) || (global->Polarization!=newPolarization) ||
            (global->Srate!=newSrate) || (global->Pid!=newPid))
    {
        bResetReceivedAll=true;
    }

    SetupStore ("Frequency", global->Frequency = newFrequency);
    SetupStore ("Polarization", global->Polarization = newPolarization);
    SetupStore ("Srate", global->Srate = newSrate);
    SetupStore ("Pid", global->Pid = newPid);

    SetupStore ("WaitTime", global->WaitTime = newWaitTime);
    SetupStore ("NoWakeup",global->NoWakeup=newNoWakeup);
    SetupStore ("NoDeferredShutdown",global->NoDeferredShutdown=newNoDeferredShutdown);
    SetupStore ("HideMainMenu",global->HideMainMenu=newHideMainMenu);

    if (bResetReceivedAll)
    {
        if (global->FindReceiverChannel())
        {
            dsyslog ("infosatepg: receive files again");
            global->ResetReceivedAll();
        }
        else
        {
            esyslog("infosatepg: found no channel to receive, check setup");
        }
    }
}

eOSState cMenuSetupInfosatepg::Edit()
{
    if (HasSubMenu() || Count() ==0)
        return osUnknown;

    if (Current() >=chanCurrent)
    {
        int chanIndex=Current()-chanCurrent;
        if (chanIndex<global->InfosatChannels())
        {
            eOSState state=AddSubMenu (new cMenuSetupChannelMenu (global,chanIndex));
            cChannel *chan = Channels.GetByChannelID (global->GetChannelID (chanIndex));
            if (chan)
            {
                int chanuse=global->GetChannelUsage(chanIndex);
                cString buffer = cString::sprintf ("%s:\t%s",chan->Name(),chanuse ? tr ("used") : "");
                cOsdItem *osd=Get(Current());
                osd->SetText(buffer);
            }
            return state;
        }
        else
        {
            return osUnknown;
        }
    }
    else
        return osUnknown;
}

eOSState cMenuSetupInfosatepg::ProcessKey (eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey (Key);

    switch (state)
    {

    default:
        if (state==osUnknown)
        {
            switch (Key)
            {

            case kOk:
                state=Edit();
                if (state==osUnknown)
                {
                    Store();
                    state=osBack;
                }
                break;
            default:
                break;
            }
        }
    }
    return state;
}

cMenuSetupChannelMenu::cMenuSetupChannelMenu (cGlobalInfosatepg *Global, int Index)
{
    SetPlugin (cPluginManager::GetPlugin (PLUGIN_NAME_I18N));
    global=Global;
    index=Index;

    newChannelUse=global->GetChannelUsage(index);
    if (newChannelUse<0) newChannelUse=USE_NOTHING; // to be safe!
    newDays=global->GetChannelDays(index);
    if (newDays<=0) newDays=1;

    cChannel *channel = Channels.GetByChannelID (global->GetChannelID (index));
    if (!channel) return;

    SetHelp(tr("Button$Reset"),tr("Button$Copy"));

    cString buffer = cString::sprintf("---- %s ----", channel->Name());
    Add (new cOsdItem (buffer,osUnknown,false));

    Add(new cMenuEditIntItem(tr("Days in advance"),&newDays,1,EPG_DAYS));
    Add(new cMenuEditBitItem(tr("Short text"),(uint *) &newChannelUse,USE_SHORTTEXT));
    Add(new cMenuEditBitItem(tr("Long text"),(uint *) &newChannelUse,USE_LONGTEXT));
    if ((newChannelUse & USE_APPEND)!=USE_APPEND)
        Add(new cMenuEditBitItem(tr("Merge long texts"),(uint *) &newChannelUse,USE_MERGELONGTEXT));
    Add(new cMenuEditBitItem(tr("Extended EPG"),(uint *) &newChannelUse,USE_EXTEPG));
    Add(new cMenuEditBitItem(tr("Append non existing events"),(uint *) &newChannelUse,USE_APPEND));

}

void cMenuSetupChannelMenu::Store (void)
{
    bool bReprocess=false;

    cChannel *channel = Channels.GetByChannelID (global->GetChannelID (index));
    if (!channel) return;
    cString ChannelID = channel->GetChannelID().ToString();
    cString name = cString::sprintf("Channel-%s",*ChannelID);
    if (!*name) return;
    if (global->SetChannelOptions(index,newChannelUse,newDays)) bReprocess=true;
    int setupval=newChannelUse+(newDays<<16);
    SetupStore (name,setupval);

    if (bReprocess)
    {
        dsyslog ("infosatepg: reprocess files (later)");
        global->ResetProcessed();
    }
}

eOSState cMenuSetupChannelMenu::ProcessKey (eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey (Key);

    switch (state)
    {

    default:
        if (state==osUnknown)
        {
            switch (Key)
            {
            case kRed:
                if (Skins.Message(mtInfo,tr("Reset all channel settings?"))==kOk)
                {
                    newDays=1;
                    newChannelUse=USE_NOTHING;
                    int oldindex=index;
                    for (index=0; index<global->InfosatChannels(); index++) Store();
                    index=oldindex;
                    state=osBack;
                }
                else
                {
                    state=osContinue;
                }
                break;
            case kGreen:
                if (Skins.Message(mtInfo,tr("Copy settings to all channels?"))==kOk)
                {
                    int oldindex=index;
                    for (index=0; index<global->InfosatChannels(); index++) Store();
                    index=oldindex;

                }
                state=osContinue;
                break;

            case kOk:
                Store();
                state=osBack;
                break;

            default:
                break;
            }
        }
    }
    return state;
}
