#include <vdr/channels.h>
#include "global.h"

// --- cGlobalInfosatdata
cGlobalInfosatdata::cGlobalInfosatdata()
{
    file[0]=0;
    Init(NULL,0,0,0);
}

bool cGlobalInfosatdata::NeverSeen(int Day,int Month,int Packetcount)
{
    if ((day!=Day) || (month!=Month) || (pktcnt!=Packetcount))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void cGlobalInfosatdata::Init(char *File,int Day,int Month,int Packetcount)
{
    if (access(file,R_OK | W_OK)==0)
    {
        dsyslog("infosatepg: deleting old %s",file);
        unlink(file);
    }
    Processed=false;
    receivedall=false;
    missed=0;
    Unlocated=0;
    lastpkt=-1;
    receivedpercent=0;
    day=Day;
    month=Month;
    pktcnt=Packetcount;
    if (File) snprintf(file,sizeof(file),"%s",File);
    else file[0]=0;
    memset(bitfield,0,sizeof(bitfield));
}

int cGlobalInfosatdata::Save(int fd)
{
    if (fd==-1) return -1;
    ssize_t ret=write(fd,&receivedall,sizeof(receivedall));
    if (ret!=sizeof(receivedall)) return -1;
    ret=write(fd,&receivedpercent,sizeof(receivedpercent));
    if (ret!=sizeof(receivedpercent)) return -1;
    ret=write(fd,&Processed,sizeof(Processed));
    if (ret!=sizeof(Processed)) return -1;
    ret=write (fd,&day,sizeof (day));
    if (ret!=sizeof(day)) return -1;
    ret=write (fd,&month,sizeof (month));
    if (ret!=sizeof (month)) return -1;
    ret=write (fd,&pktcnt,sizeof (pktcnt));
    if (ret!=sizeof (pktcnt)) return -1;
    ret=write (fd,&missed,sizeof (missed));
    if (ret!=sizeof (missed)) return -1;
    ret=write (fd,&Unlocated,sizeof (Unlocated));
    if (ret!=sizeof (Unlocated)) return -1;
    ret=write (fd,&bitfield,sizeof (bitfield));
    if (ret!=sizeof (bitfield)) return -1;
    ret=write (fd,&file,sizeof (file));
    if (ret!=sizeof (file)) return -1;
    return ret;
}

int cGlobalInfosatdata::Load(int fd)
{
    if (fd==-1) return -1;
    ssize_t ret=read (fd,&receivedall,sizeof (receivedall));
    if (ret!=sizeof (receivedall)) return -1;
    ret=read (fd,&receivedpercent,sizeof(receivedpercent));
    if (ret!=sizeof (receivedpercent)) return -1;
    ret=read (fd,&Processed,sizeof (Processed));
    if (ret!=sizeof (Processed)) return -1;
    ret=read (fd,&day,sizeof (day));
    if (ret!=sizeof (day)) return -1;
    ret=read (fd,&month,sizeof (month));
    if (ret!=sizeof (month)) return -1;
    ret=read (fd,&pktcnt,sizeof (pktcnt));
    if (ret!=sizeof (pktcnt)) return -1;
    ret=read (fd,&missed,sizeof (missed)); 
    if (ret!=sizeof (missed)) return -1;
    ret=read (fd,&Unlocated,sizeof (Unlocated)); 
    if (ret!=sizeof (Unlocated)) return -1;
    ret=read (fd,&bitfield,sizeof (bitfield));
    if (ret!=sizeof (bitfield)) return -1;
    ret=read (fd,&file,sizeof (file));
    if (ret!=sizeof (file)) return -1;
    // check file
    if (file[0]!=0)
    {
        if (access(file,R_OK | W_OK)==-1)
        {
            // file doesnt exist -> receive it again
            esyslog("infosatepg: cannot access %s",file);
            Init(file,day,month,pktcnt);
        }
    }
    return ret;
}

#ifdef INFOSATEPG_DEBUG
void cGlobalInfosatdata::Debug(const char *Directory)
{
    char file[1024];
    snprintf (file,sizeof (file),"%s/infosatepg9999_999.dat",Directory);
    Init (file,99,99,999);
    ready=true;
}
#endif

void cGlobalInfosatdata::CheckMissed(int ActualPacket)
{
    if (receivedall) return; // count missed packets while receiving
    if ((ActualPacket!=(lastpkt+1)) && (lastpkt!=-1))
    {
        missed++;
    }
    lastpkt=ActualPacket;
}

bool cGlobalInfosatdata::CheckReceivedAll()
{
    int donecnt=0;
    receivedpercent=0;
    if (pktcnt==0) return false;
    bool retval=true;
    for (int i=0; i<pktcnt; i++)
    {
        if (GetBit(i)==true) donecnt++;
        else retval=false;
    }
    receivedpercent=((donecnt*100)/pktcnt);
    if (retval==true) receivedall=true;
    return retval;
}

// --- cGlobalInfosatepg -----------------------------------------------------
cGlobalInfosatepg::cGlobalInfosatepg()
{
    // set default values
    // set private member
    switched=false;
    directory=NULL;
    infosatchannels=NULL;
    this_day=-1;
    this_month=-1;
    numinfosatchannels=0;
    wakeuptime=-1;
    // set public member
    LastCurrentChannel=-1;
    Pid=1809;
    Srate = 22000;
    Frequency = 12604;
    Polarization ='h';
    channel=-1;
    MAC[0]=0x01;
    MAC[1]=0x00;
    MAC[2]=0x5e;
    MAC[3]=0x02;
    MAC[4]=0x02;
    WaitTime=10; // default 10 seconds
    SetDirectory ("/tmp");
    NoWakeup=false;
    NoDeferredShutdown=false;
    ActualMac=0;
    HideMainMenu=true;
}

cGlobalInfosatepg::~cGlobalInfosatepg()
{
    free((void *) directory);
    free(infosatchannels);
}

bool cGlobalInfosatepg::SetDirectory(const char *Directory)
{
    if (access (Directory,W_OK | R_OK | X_OK) ==-1)
    {
        // cannot access, maybe create?
        if (mkdir (Directory,0775) ==-1)
        {
            // even if the Directory exists, we cannot access
            return false;
        }
        if (access (Directory,W_OK | R_OK| X_OK) ==-1) return false;
    }
    free ((void *) directory);
    directory = Directory ? strdup (Directory) : NULL;
    return true;
}

bool cGlobalInfosatepg::CheckMAC(struct ethhdr *eth_hdr)
{
    if (!eth_hdr) return false;
    if (eth_hdr->h_dest[0]!=MAC[0]) return false;
    if (eth_hdr->h_dest[1]!=MAC[1]) return false;
    if (eth_hdr->h_dest[2]!=MAC[2]) return false;
    if (eth_hdr->h_dest[3]!=MAC[3]) return false;
    if (eth_hdr->h_dest[4]!=MAC[4]) return false;

    if (eth_hdr->h_dest[5]<EPG_FIRST_DAY_MAC) return false;
    if (eth_hdr->h_dest[5]>EPG_LAST_DAY_MAC) return false;

    return true;
}

void cGlobalInfosatepg::AddChannel(tChannelID ChannelID,int Usage,int Days)
{
    infosatchannels= (struct infosatchannels *) realloc (infosatchannels,
                     (numinfosatchannels+1) *sizeof (struct infosatchannels));
    if (!infosatchannels) return;
    infosatchannels[numinfosatchannels].ChannelID=ChannelID;
    infosatchannels[numinfosatchannels].Usage=Usage & 0xFFFF;
    if (Days<=0) Days=1;
    if (Days>EPG_DAYS) Days=EPG_DAYS;
    infosatchannels[numinfosatchannels].Days=Days;
    numinfosatchannels++;
}

void cGlobalInfosatepg::RemoveChannel(int Index)
{
    if ((Index<0) || (Index>numinfosatchannels)) return;
    int i;
    for (i=Index; i<(numinfosatchannels-1); i++)
    {
        infosatchannels[i].ChannelID=infosatchannels[i+1].ChannelID;
        infosatchannels[i].Usage=infosatchannels[i+1].Usage;
    }
    numinfosatchannels--;
}

tChannelID cGlobalInfosatepg::GetChannelID(int Index)
{
    if (numinfosatchannels==0) return tChannelID::InvalidID;
    if ((Index<0) || (Index>numinfosatchannels-1))
        return tChannelID::InvalidID;
    return infosatchannels[Index].ChannelID;
}

bool cGlobalInfosatepg::ChannelExists(tChannelID ChannelID,int *Index)
{
    if (numinfosatchannels==0) return false;
    for (int i=0; i<numinfosatchannels; i++)
    {
        if (infosatchannels[i].ChannelID==tChannelID::InvalidID) continue;
        if (infosatchannels[i].ChannelID==ChannelID)
        {
            if (Index) *Index=i;
            return true;
        }
    }
    return false;
}

bool cGlobalInfosatepg::SetChannelOptions(int Index,int Usage,int Days)
{
    if (numinfosatchannels==0) return false;
    if ((Index<0) || (Index>numinfosatchannels-1)) return false;
    bool ret=false;
    if ((infosatchannels[Index].Usage!=Usage) | (infosatchannels[Index].Days!=Days)) ret=true;
    infosatchannels[Index].Usage=Usage;
    infosatchannels[Index].Days=Days;
    return ret;
}

int cGlobalInfosatepg::GetChannelUsage(int Index)
{
    if (numinfosatchannels==0) return USE_NOTHING;
    if ((Index<0) || (Index>numinfosatchannels-1)) return USE_NOTHING;
    return infosatchannels[Index].Usage;
}

int cGlobalInfosatepg::GetChannelDays(int Index)
{
    if (numinfosatchannels==0) return 0;
    if ((Index<0) || (Index>numinfosatchannels-1)) return 0;
    return infosatchannels[Index].Days;
}

int cGlobalInfosatepg::Save()
{
    char file[1024];
    snprintf (file,sizeof (file),"%s/infosatepg.dat",directory);

    int f=open (file,O_RDWR|O_CREAT|O_TRUNC,0664);
    if (f==-1)
    {
        if (errno!=ENOSPC)
        {
            esyslog ("infosatepg: unable to create file '%s'", file);
        }
        return -1;
    }

    int ret;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        ret=Infosatdata[mac].Save (f);
        if (ret==-1) break;
    }

    if (ret!=-1)
    {
        ret=write (f,&this_day,sizeof(this_day));
        if (ret!=sizeof (this_day)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=write (f,&this_month,sizeof(this_month));
        if (ret!=sizeof (this_month)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=write (f,&wakeuptime,sizeof(wakeuptime));
        if (ret!=sizeof (wakeuptime)) ret=-1;
    }

    close (f);
    if (ret==-1) unlink (file);
    return ret;
}

int cGlobalInfosatepg::Load()
{
    char file[1024];
    snprintf (file,sizeof (file),"%s/infosatepg.dat",directory);

    int f=open (file,O_RDONLY);
    if (f==-1) return 0; // it's ok if the file doesn't exist

    int ret;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        ret=Infosatdata[mac].Load (f);
        if (ret==-1) break;
    }
    if (ret!=-1)
    {
        ret=read (f,&this_day,sizeof (this_day));
        if (ret!=sizeof (this_day)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=read (f,&this_month,sizeof (this_month));
        if (ret!=sizeof (this_month)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=read (f,&wakeuptime,sizeof (wakeuptime));
        if (ret!=sizeof (wakeuptime)) ret=-1;
    }

    close (f);
    if (ret==-1)
    {
        unlink (file); // but it's not ok if the read failed!
        ResetReceivedAll();
    }
    return ret;
}

void cGlobalInfosatepg::ResetReceivedAll(void)
{
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        Infosatdata[mac].ResetReceivedAll();
    }
    wakeuptime=-1;
}

void cGlobalInfosatepg::ResetProcessed (void)
{
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        Infosatdata[mac].Processed=false;
    }
    wakeuptime=-1;
}

bool cGlobalInfosatepg::ProcessedAll()
{
    int numProcessed=0;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        if (Infosatdata[mac].Processed) numProcessed++;
    }
    // all days processed
    if (numProcessed==EPG_DAYS) return true;
    return false;
}

bool cGlobalInfosatepg::ReceivedAll(int *Day, int *Month)
{
    bool res=false;
    time_t Now=time (NULL);
    struct tm tm;
    if (!localtime_r (&Now,&tm)) return false;

    // First check if we have received all data
    int numReceived=0;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        if (Infosatdata[mac].ReceivedAll()) numReceived++;
    }
    // All days fully received
    if (numReceived==EPG_DAYS)
    {
        // First entry from today?
        if ((Infosatdata[EPG_FIRST_DAY_MAC].Day() ==tm.tm_mday) &&
                (Infosatdata[EPG_FIRST_DAY_MAC].Month() ==tm.tm_mon+1))
        {
            // Yes
            if ((this_day!=tm.tm_mday) || (this_month!=tm.tm_mon+1))
            {
                isyslog ("infosatepg: all data received");
                this_day=tm.tm_mday;
                this_month=tm.tm_mon+1;
            }
            res=true;
        }
        else
        {
            // New day, but new data is ready only after wakeup-time
            time_t Now = time(NULL);
            time_t Time;
            if (WakeupTime()==-1) Time=Now+301; // If no wakeup set, just wait
            else Time = WakeupTime();
            if (Now>=(Time-300))
            {
                // new day and new data should be available
                ResetReceivedAll();
                res=false;
            }
            else
            {
                // wait till we are after wakeuptime
                res=true;
            }
        }
    }

    if (Day) *Day=this_day;
    if (Month) *Month=this_month;

    return res;
}

bool cGlobalInfosatepg::FindReceiverChannel()
{
    cChannel *chan;
    int source = cSource::FromString("S19.2E"); // only from astra 19.2E

    // Try to find a channel with the exact frequency
    for (int i=0; i<=Channels.MaxNumber(); i++)
    {
        chan = Channels.GetByNumber(i,0);
        if (chan)
        {
            if (chan->Source()!=source) continue;
            if (chan->Srate()!=Srate) continue;
            if (chan->Frequency()!=Frequency) continue;
            if (chan->Polarization()!=Polarization) continue;
            channel=i;
            return true;
        }
    }

    // None found? Then try to find a channel with frequency + 1
    for (int i=0; i<=Channels.MaxNumber(); i++)
    {
        chan = Channels.GetByNumber(i,0);
        if (chan)
        {
            if (chan->Source()!=source) continue;
            if (chan->Srate()!=Srate) continue;
            if (chan->Frequency()!=(Frequency+1)) continue;
            if (chan->Polarization()!=Polarization) continue;
            channel=i;
            return true;
        }
    }

    // Still none found? Then try to find a channel with frequency - 1
    for (int i=0; i<=Channels.MaxNumber(); i++)
    {
        chan = Channels.GetByNumber(i,0);
        if (chan)
        {
            if (chan->Source()!=source) continue;
            if (chan->Srate()!=Srate) continue;
            if (chan->Frequency()!=(Frequency-1)) continue;
            if (chan->Polarization()!=Polarization) continue;
            channel=i;
            return true;
        }
    }

    // No channel available
    channel=-1;
    return false;
}
