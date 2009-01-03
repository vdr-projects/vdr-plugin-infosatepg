/*
 * global.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __global_h_
#define __global_h_

#include <netinet/if_ether.h>
#include <sys/types.h>

#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/device.h>

#define MIN_WAITTIME 10
#define MAX_WAITTIME 120
#define MIN_EVENTTIMEDIFF 5
#define MAX_EVENTTIMEDIFF 10

class cGlobalInfosatdata
{
#define _GetByte(ptr, bitnum) ((((char*)ptr)+bitnum/8))
#define _GetBit(ptr, bitnum) ((*_GetByte(ptr, bitnum) >> (bitnum%8)) & 1)
#define _SetBit(ptr, bitnum, val) (val ? \
	(*_GetByte(ptr, bitnum) |= (1<<(bitnum%8))) : \
	(*_GetByte(ptr, bitnum) &= ~(1<<(bitnum%8))))
private:
    bool        receivedall;
    int         receivedpercent;
    u_char      day;
    u_char      month;
    u_short     pktcnt;
    u_char      bitfield[8192];
    char        file[1024];
public:
    cGlobalInfosatdata();
    bool Processed;
    bool ReceivedAll()
    {
        return receivedall;
    }
    void ResetReceivedAll()
    {
        Init(file,0,0,0);
    }
    bool CheckReceivedAll();
    int ReceivedPercent()
    {
        return receivedpercent;
    }
    int  Day()
    {
        return day;
    }
    int  Month()
    {
        return month;
    }
    bool GetBit (int Bitnumber)
    {
        return _GetBit (bitfield,Bitnumber);
    }
    void SetBit (int Bitnumber,bool Value)
    {
        _SetBit (bitfield,Bitnumber,Value);
    }
    const char *GetFile() const // used in process.cpp
    {
        return (char *) &file;
    }
    bool NeverSeen (int Day, int Month, int Packetcount);
    void Init (char *File, int Day, int Month, int Packetcount);
    int Load (int fd);
    int Save (int fd);
#ifdef INFOSATEPG_DEBUG
    void Debug (const char *Directory);
#endif
};

class cGlobalInfosatepg
{
#define USE_NOTHING       0


#define USE_SHORTTEXT     1
#define USE_SHORTLONGTEXT 2
#define USE_SHORTTEXTEPG  3
#define USE_INTELLIGENT   4
#define USE_ALL           5

    /*
    #define USE_NOTHING       0
    #define USE_SHORTTEXT     1
    #define USE_LONGTEXT      2
    #define USE_EXTEPG        4
    #define
    */

    struct infosatchannels
    {
        tChannelID ChannelID;
        int Usage;
    };

#define EPG_FIRST_DAY_MAC 1
#define EPG_LAST_DAY_MAC  7
#define EPG_DAYS          7

private:
    const char *directory;
    u_char MAC[5];
    time_t timer;
    bool switched;
    int this_day;
    int this_month;
    int numinfosatchannels;
    int wakeuptime;
    struct infosatchannels *infosatchannels;
    void ResetReceivedAll(void);
public:
    cGlobalInfosatepg();
    ~cGlobalInfosatepg();
    cGlobalInfosatdata Infosatdata[EPG_DAYS+1];
    cDevice *dev;
    void SetWakeupTime(int Time)
    {
        if (Time==-1) return;
        if (wakeuptime!=-1) return; // already set
        int hour,minute;
        hour=(int) (wakeuptime/100);
        minute=wakeuptime-(hour*100);
        isyslog("infosatepg: wakeup set to %02i:%02i", hour,minute);
        wakeuptime=Time;
    }
    int WakeupTime()
    {
        return wakeuptime;
    }
    int LastCurrentChannel;
    int Channel;
    int Pid;
    int EventTimeDiff;
    int WaitTime;
    const char *Directory()
    {
        return directory;
    }
    bool SetDirectory (const char *Directory);
    bool CheckMAC (struct ethhdr *eth_hdr);
    void SetWaitTimer()
    {
        timer=time (NULL);
    }
    bool WaitOk()
    {
        return (time (NULL) > (timer+ (time_t) WaitTime));
    }
    void SetSwitched (bool Value)
    {
        switched=Value;
    }
    bool Switched()
    {
        return switched;
    }
    int Load();
    int Save();
    bool ProcessedAll;
    bool ReceivedAll (int *Day, int *Month);
    bool ReceivedAll()
    {
        return ReceivedAll (NULL,NULL);
    }
    void AddChannel (tChannelID ChannelID,int Usage);
    void RemoveChannel(int Index);
    tChannelID GetChannelID (int Index);
    bool SetChannelUse (int Index,int Usage);
    void ResetProcessed (void);
    int GetChannelUse (int Index);
    bool ChannelExists (tChannelID ChannelID,int *Index);
    int InfosatChannels()
    {
        return numinfosatchannels;
    }
};
#endif
