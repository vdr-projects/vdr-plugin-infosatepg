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
    bool bAddFilter=false;

    // just add filter if we aren't locked
    if ((ChannelNumber==global->Channel) && (!global->isLocked())) bAddFilter=true;

    if (bAddFilter)
    {

        if ((myFilterDevice) && (myFilter))
        {
            if (myFilterDevice==Device) return; // already attached -> bail out
            dsyslog("infosatepg: detach previously attached filter");
            myFilterDevice->Detach(myFilter);
        }

        myFilterDevice = Device->GetDevice(Device->DeviceNumber());
        if (!myFilterDevice) return;

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
                global->SetTimer();
                global->SetSwitched(false);
            }
        }
    }
}
