/*
 * setup.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "setup.h"

// --- cMenuSetupInfosatepg
cMenuSetupInfosatepg::cMenuSetupInfosatepg(cGlobalInfosatepg *Global)
{
    global=Global;

    newChannel = global->Channel;
    newPid = global->Pid;
    newWaitTime = global->WaitTime;
    newEventTimeDiff=(int) (global->EventTimeDiff/60);

    Add(NewTitle(tr("Scan parameter")));
    cOsdItem *firstItem = new cMenuEditIntItem( tr("Channel"), &newChannel,1,Channels.MaxNumber());
    Add(firstItem);
    Add(new cMenuEditIntItem( tr("Pid"), &newPid,1,8191));
    Add(NewTitle(tr("Event options")));
    Add(new cMenuEditIntItem( tr("Wait time [s]"), &newWaitTime,MIN_WAITTIME,MAX_WAITTIME));
    Add(new cMenuEditIntItem( tr("Time difference [min]"), &newEventTimeDiff,
                              MIN_EVENTTIMEDIFF,MAX_EVENTTIMEDIFF));

    if (global->InfosatChannels())
    {
        Add(NewTitle(tr("Infosat channels")),true);
        chanCurrent=Current()+1;
        SetCurrent(firstItem);

        for (int i=0; i<global->InfosatChannels(); i++)
        {
            cChannel *chan = Channels.GetByChannelID(global->GetChannelID(i));
            if (!chan) continue;
            Add(new cOsdItem(chan->Name()));
        }
    }
}

cOsdItem *cMenuSetupInfosatepg::NewTitle(const char *s)
{
    char *str;
    asprintf(&str,"---- %s ----", s);
    cOsdItem *tmp = new cOsdItem(str);
    tmp->SetSelectable(false);
    free(str);
    return tmp;
}

void cMenuSetupInfosatepg::Store(void)
{
    bool bReprocess=false;

    if (global->EventTimeDiff!=(60*newEventTimeDiff)) bReprocess=true;

    SetupStore("Channel", global->Channel = newChannel);
    SetupStore("Pid", global->Pid = newPid);
    SetupStore("WaitTime", global->WaitTime = newWaitTime);
    SetupStore("EventTimeDiff", newEventTimeDiff);
    global->EventTimeDiff = 60*newEventTimeDiff;

    if (bReprocess)
    {
        dsyslog("infosatepg: reprocess files (later)");
        global->ResetProcessed();
    }
}

eOSState cMenuSetupInfosatepg::Edit()
{
    if (HasSubMenu() || Count()==0)
        return osUnknown;

    if (Current()>=chanCurrent)
    {
        int chanIndex=Current()-chanCurrent;
        if (chanIndex<global->InfosatChannels())
            return AddSubMenu(new cMenuSetupChannelMenu(global,chanIndex));
        else
            return osUnknown;
    }
    else
        return osUnknown;
}

eOSState cMenuSetupInfosatepg::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

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

cMenuSetupChannelMenu::cMenuSetupChannelMenu(cGlobalInfosatepg *Global, int Index)
{
    SetPlugin(cPluginManager::GetPlugin(PLUGIN_NAME_I18N));
    global=Global;
    index=Index;

    ChannelUseText[0]=tr("no");
    ChannelUseText[1]=tr("short text");
    ChannelUseText[2]=tr("short/long text");
    ChannelUseText[3]=tr("short text/extEPG");
    ChannelUseText[4]=tr("intelligent");
    ChannelUseText[5]=tr("complete");

    newChannelUse=global->GetChannelUse(index);
    if (newChannelUse<0) newChannelUse=USE_NOTHING; // to be safe!

    channel = Channels.GetByChannelID(global->GetChannelID(index));
    if (!channel) return;

    char *str;
    asprintf(&str,"---- %s ----", channel->Name());
    Add(new cOsdItem(str,osUnknown,false));
    free(str);

    Add(new cMenuEditStraItem("Usage",&newChannelUse,
                              sizeof(ChannelUseText)/sizeof(const char *),ChannelUseText));

}

void cMenuSetupChannelMenu::Store(void)
{
    bool bReprocess=false;

    if (!channel) return;
    cString ChannelID = channel->GetChannelID().ToString();
    char *name;
    asprintf(&name,"Channel-%s",*ChannelID);
    if (!name) return;
    if (global->SetChannelUse(index,newChannelUse)) bReprocess=true;
    SetupStore(name,newChannelUse);
    free(name);
    if (bReprocess)
    {
        dsyslog("infosatepg: reprocess files (later)");
        global->ResetProcessed();
    }
}
