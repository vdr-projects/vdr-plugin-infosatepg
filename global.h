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

#define MIN_WAITTIME 10
#define MAX_WAITTIME 120
#define MIN_EVENTTIMEDIFF 5
#define MAX_EVENTTIMEDIFF 10

class cGlobalInfosatdata
{
#define _GetByte( ptr, bitnum ) ( (((char*)ptr)+bitnum/8) )
#define _GetBit( ptr, bitnum ) ( (*_GetByte(ptr, bitnum) >> (bitnum%8)) & 1 )
#define _SetBit( ptr, bitnum, val ) ( val ? \
	(*_GetByte(ptr, bitnum) |= (1<<(bitnum%8))) : \
	(*_GetByte(ptr, bitnum) &= ~(1<<(bitnum%8))) )
private:
    bool        ready;
    bool        processed;
    u_char      day;
    u_char      month;
    u_short     pktcnt;
    u_char      bitfield[8192];
    char        file[1024];
public:
    cGlobalInfosatdata();
    bool isReady2Process()
    {
        return (ready && !processed);
    }
    bool wasProcessed()
    {
        return (ready && processed);
    }
    int  Day()
    {
        return day;
    }
    int  Month()
    {
        return month;
    }
    int  Packetcount()
    {
        return pktcnt;
    }
    void SetProcessed()
    {
        processed=true;
    }
    void ResetProcessed()
    {
        processed=false;
    }
    bool GetBit(int Bitnumber)
    {
        return _GetBit(bitfield,Bitnumber);
    }
    void SetBit(int Bitnumber,bool Value)
    {
        _SetBit(bitfield,Bitnumber,Value);
    }
    const char *GetFile() const
    {
        return (char *) &file;
    }

    bool NeverSeen(int Day, int Month, int Packetcount);
    void Init(char *File, int Day, int Month, int Packetcount);
    bool ReceivedAll();
    int ReceivedPercent();
    int Load(int fd);
    int Save(int fd);
#ifdef INFOSATEPG_DEBUG
    void Debug(const char *Directory);
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
    bool Switched;
    int this_day;
    int this_month;
public:
    cGlobalInfosatepg();
    ~cGlobalInfosatepg();
    int Channel;
    int Pid;
    int EventTimeDiff;
    int WaitTime;
    int WakeupTime; // 0100 = 01:00  1222 = 12:22

    const char *Directory()
    {
        return directory;
    }
    bool SetDirectory(const char *Directory);
    bool CheckMAC(struct ethhdr *eth_hdr);
    void SetTimer()
    {
        timer=time(NULL);
    }
    bool isWaitOk()
    {
        return (time(NULL)>(timer+(time_t) WaitTime));
    }
    void SetSwitched(bool Value)
    {
        Switched=Value;
    }
    bool isSwitched()
    {
        return Switched;
    }

public:
    cGlobalInfosatdata Infosatdata[EPG_DAYS+1];
    int Load();
    int Save();
    void Lock(time_t Now);
    bool isLocked(int *Day, int *Month);
    bool isLocked()
    {
        return isLocked(NULL,NULL);
    }

private:
    int numinfosatchannels;
    struct infosatchannels *infosatchannels;
public:
    void AddChannel(tChannelID ChannelID,int Usage);
    tChannelID GetChannelID(int Index);
    bool SetChannelUse(int Index,int Usage);
    void ResetProcessedFlags(void);
    int GetChannelUse(int Index);
    bool ChannelExists(tChannelID ChannelID,int *Index);
    int InfosatChannels()
    {
        return numinfosatchannels;
    }
};
#endif
