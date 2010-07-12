/*
 * global.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __global_h_
#define __global_h_

#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <sys/types.h>
#include <ctype.h>

#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/device.h>

#define MIN_WAITTIME 10      // s
#define MAX_WAITTIME 120     // s

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
    int		lastpkt;
    int		missed;
    u_char      day;
    u_char      month;
    u_short     pktcnt;
    u_char      bitfield[8192];
    char        file[1024];
public:
    cGlobalInfosatdata();
    bool Processed;
    int	Unlocated; // unlocated events
    int Missed()
    {
        return missed;
    }
    bool ReceivedAll()
    {
        return receivedall;
    }
    void ResetReceivedAll()
    {
        Init(file,0,0,0);
    }
    bool CheckReceivedAll();
    void CheckMissed(int ActualPacket);
    void SetLastPkt(int ActualPacket)
    {
        lastpkt = ActualPacket;
    }
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
    // Usage field definition
    // Bit 0-15   USE_ flags
    // Bit 16-19  DAYS in advance
    // Bit 20-30  reserved for future used
    // Bit 31     always zero

#define USE_SHORTTEXT     1
#define USE_LONGTEXT      2
#define USE_EXTEPG        4
#define USE_MERGELONGTEXT 8
#define USE_APPEND        16

#define USE_NOTHING       0

    struct infosatchannels
    {
        tChannelID ChannelID;
        int Days;
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
    time_t wakeuptime;
    struct infosatchannels *infosatchannels;
    int channel;
public:
    cGlobalInfosatepg();
    ~cGlobalInfosatepg();
    cGlobalInfosatdata Infosatdata[EPG_DAYS+1];
    void SetWakeupTime(time_t Time)
    {
        if (Time==(time_t) -1) return;
        if ((Time>wakeuptime) && (wakeuptime!=(time_t) -1)) return; // already set
        wakeuptime=Time;
        isyslog("infosatepg: wakeup set to %s", ctime(&wakeuptime));
    }
    bool NoWakeup;
    bool NoDeferredShutdown;
    time_t WakeupTime()
    {
        return wakeuptime;
    }
    int LastCurrentChannel;
    int Channel()
    {
        return channel;
    }
    bool FindReceiverChannel();
    int Frequency;
    char Polarization;
    int Srate;
    int Pid;
    int WaitTime;
    const char *Directory()
    {
        return directory;
    }
    bool SetDirectory (const char *Directory);
    bool CheckMAC (ether_header *eth_hdr);
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

    bool ProcessedAll();
    void ResetProcessed (void);
    void ResetReceivedAll(void);
    bool ReceivedAll (int *Day, int *Month);
    bool ReceivedAll()
    {
        return ReceivedAll (NULL,NULL);
    }

    void AddChannel (tChannelID ChannelID,int Usage, int Days);
    void RemoveChannel(int Index);
    tChannelID GetChannelID (int Index);
    bool SetChannelOptions(int Index,int Usage,int Days);
    int GetChannelUsage(int Index);
    int GetChannelDays(int Index);
    bool ChannelExists (tChannelID ChannelID,int *Index);
    int InfosatChannels()
    {
        return numinfosatchannels;
    }
    int ActualMac;
    bool HideMainMenu;
};
#endif
