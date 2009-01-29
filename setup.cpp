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

    newChannel = global->Channel;
    newPid = global->Pid;
    newWaitTime = global->WaitTime;
    newEventTimeDiff= (int) (global->EventTimeDiff/60);

    Add (NewTitle (tr ("Scan parameter")));
    cOsdItem *firstItem = new cMenuEditIntItem (tr ("Channel"), &newChannel,1,Channels.MaxNumber());
    Add (firstItem);
    Add (new cMenuEditIntItem (tr ("Pid"), &newPid,1,8191));
    Add (NewTitle (tr ("Event options")));
    Add (new cMenuEditIntItem (tr ("Wait time [s]"), &newWaitTime,MIN_WAITTIME,MAX_WAITTIME));
    Add (new cMenuEditIntItem (tr ("Time difference [min]"), &newEventTimeDiff,
                                 MIN_EVENTTIMEDIFF,MAX_EVENTTIMEDIFF));

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
    char *str;
    asprintf (&str,"---- %s ----", s);
    cOsdItem *tmp = new cOsdItem (str);
    tmp->SetSelectable (false);
    free (str);
    return tmp;
}

void cMenuSetupInfosatepg::Store (void)
{
    bool bReprocess=false;

    if (global->EventTimeDiff!= (60*newEventTimeDiff)) bReprocess=true;

    SetupStore ("Channel", global->Channel = newChannel);
    SetupStore ("Pid", global->Pid = newPid);
    SetupStore ("WaitTime", global->WaitTime = newWaitTime);
    SetupStore ("EventTimeDiff", newEventTimeDiff);
    global->EventTimeDiff = 60*newEventTimeDiff;

    if (bReprocess)
    {
        dsyslog ("infosatepg: reprocess files (later)");
        global->ResetProcessed();
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
            return AddSubMenu (new cMenuSetupChannelMenu (global,chanIndex));
        else
            return osUnknown;
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

    channel = Channels.GetByChannelID (global->GetChannelID (index));
    if (!channel) return;

    char *str;
    asprintf (&str,"---- %s ----", channel->Name());
    Add (new cOsdItem (str,osUnknown,false));
    free (str);

    Add(new cMenuEditIntItem(tr("Days in advance"),&newDays,1,EPG_DAYS));
    Add(new cMenuEditBitItem(tr("Short text"),(uint *) &newChannelUse,USE_SHORTTEXT));
    Add(new cMenuEditBitItem(tr("Long text"),(uint *) &newChannelUse,USE_LONGTEXT));
    Add(new cMenuEditBitItem(tr("Merge long texts"),(uint *) &newChannelUse,USE_MERGELONGTEXT));
    Add(new cMenuEditBitItem(tr("Extended EPG"),(uint *) &newChannelUse,USE_EXTEPG));
    Add(new cMenuEditBitItem(tr("Append non existing events"),(uint *) &newChannelUse,USE_APPEND));

}

void cMenuSetupChannelMenu::Store (void)
{
    bool bReprocess=false;

    if (!channel) return;
    cString ChannelID = channel->GetChannelID().ToString();
    char *name;
    asprintf (&name,"Channel-%s",*ChannelID);
    if (!name) return;
    if (global->SetChannelOptions(index,newChannelUse,newDays)) bReprocess=true;
    int setupval=newChannelUse+(newDays<<16);
    SetupStore (name,setupval);
    free (name);
    if (bReprocess)
    {
        dsyslog ("infosatepg: reprocess files (later)");
        global->ResetProcessed();
    }
}
