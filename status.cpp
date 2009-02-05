/*
 * status.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "status.h"

cStatusInfosatepg::cStatusInfosatepg(cGlobalInfosatepg *Global)
{
    global = Global;
    myFilter=new cFilterInfosatepg(global);
    myFilterDevice=NULL;
}

cStatusInfosatepg::~cStatusInfosatepg(void)
{
    if (myFilterDevice) myFilterDevice->Detach(myFilter);
    if (myFilter) delete myFilter;
}

void cStatusInfosatepg::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
    if (!ChannelNumber) return;
    if (!Device) return; // just to be safe

    bool bAddFilter=false;

    // just add filter if we aren't locked
    if (ChannelNumber==global->Channel())
    {
        cChannel *chan=Channels.GetByNumber(global->Channel());
        if (!chan) return;
        if (!Device->ProvidesTransponder(chan)) return; // ignore virtual devices
        if (Device==myFilterDevice) return; // already attached to this device
        if (!global->ReceivedAll()) bAddFilter=true;
    }

    if (bAddFilter)
    {
        if (myFilterDevice) return; // already attached to another device

        myFilterDevice = (cDevice *) Device;

        dsyslog("switching device %i to channel %i (infosatepg)",
                Device->DeviceNumber()+1,ChannelNumber);
        myFilterDevice->AttachFilter(myFilter);
        global->SetSwitched(true);
    }
    else
    {
        if (myFilterDevice)
        {
            if (Device==myFilterDevice)
            {
                dsyslog("infosatepg: detach filter");
                myFilterDevice->Detach(myFilter);
                myFilterDevice=NULL;
                global->SetWaitTimer();
                global->SetSwitched(false);
            }
        }
    }
}
