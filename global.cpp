#include <vdr/channels.h>
#include "global.h"

// --- cGlobalInfosatdata
cGlobalInfosatdata::cGlobalInfosatdata()
{
    file[0]=0;
    Init(NULL,0,0,0);
}

bool cGlobalInfosatdata::NeverSeen(int Day, int Month, int Packetcount)
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

void cGlobalInfosatdata::Init(char *File, int Day, int Month, int Packetcount)
{
    if (access(file,R_OK)==0)
    {
        dsyslog("infosatepg: deleting old %s",file);
        unlink(file);
    }
    ready=false;
    processed=false;
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
    ssize_t ret=write(fd,&ready,sizeof(ready));
    if (ret!=sizeof(ready)) return -1;
    ret=write(fd,&processed,sizeof(processed));
    if (ret!=sizeof(processed)) return -1;
    ret=write(fd,&day,sizeof(day));
    if (ret!=sizeof(day)) return -1;
    ret=write(fd,&month,sizeof(month));
    if (ret!=sizeof(month)) return -1;
    ret=write(fd,&pktcnt,sizeof(pktcnt));
    if (ret!=sizeof(pktcnt)) return -1;
    ret=write(fd,&bitfield,sizeof(bitfield));
    if (ret!=sizeof(bitfield)) return -1;
    ret=write(fd,&file,sizeof(file));
    if (ret!=sizeof(file)) return -1;
    return ret;
}

int cGlobalInfosatdata::Load(int fd)
{
    if (fd==-1) return -1;
    ssize_t ret=read(fd,&ready,sizeof(ready));
    if (ret!=sizeof(ready)) return -1;
    ret=read(fd,&processed,sizeof(processed));
    if (ret!=sizeof(processed)) return -1;
    ret=read(fd,&day,sizeof(day));
    if (ret!=sizeof(day)) return -1;
    ret=read(fd,&month,sizeof(month));
    if (ret!=sizeof(month)) return -1;
    ret=read(fd,&pktcnt,sizeof(pktcnt));
    if (ret!=sizeof(pktcnt)) return -1;
    ret=read(fd,&bitfield,sizeof(bitfield));
    if (ret!=sizeof(bitfield)) return -1;
    ret=read(fd,&file,sizeof(file));
    if (ret!=sizeof(file)) return -1;
    dsyslog("infosatepg: loaded file=*%s*",file);
    return ret;
}

#ifdef INFOSATEPG_DEBUG
void cGlobalInfosatdata::Debug(const char *Directory)
{
    char file[1024];
    snprintf(file,sizeof(file),"%s/infosatepg9999_999.dat",Directory);
    Init(file,99,99,999);
    ready=true;
}
#endif

int cGlobalInfosatdata::ReceivedPercent()
{
    if (pktcnt==0) return 0;
    int donecnt=0;
    for (int i=0; i<pktcnt; i++)
    {
        if (GetBit(i)==true) donecnt++;
    }
    return ((donecnt*100)/pktcnt);
}

bool cGlobalInfosatdata::ReceivedAll()
{
    if (pktcnt==0) return false;
    for (int i=0; i<pktcnt; i++)
    {
        if (GetBit(i)==false) return false;
    }
    ready=true;
    return true;
}

// --- cGlobalInfosatepg -----------------------------------------------------
cGlobalInfosatepg::cGlobalInfosatepg()
{
    // set default values
    Pid=1809;
    EventTimeDiff=480; // default 8 minutes
    Channel=1;
    MAC[0]=0x01;
    MAC[1]=0x00;
    MAC[2]=0x5e;
    MAC[3]=0x02;
    MAC[4]=0x02;
    WaitTime=10; // default 10 seconds
    Switched=false;
    directory=NULL;
    SetDirectory("/tmp");
    infosatchannels=NULL;
    numinfosatchannels=0;
    this_day=-1;
    this_month=-1;
}

cGlobalInfosatepg::~cGlobalInfosatepg()
{
    free((void *) directory);
    free(infosatchannels);
}

bool cGlobalInfosatepg::SetDirectory(const char *Directory)
{
    if (access(Directory,W_OK | R_OK | X_OK)==-1)
    {
        // cannot access, maybe create?
        if (mkdir(Directory,0775)==-1)
        {
            // even if the Directory exists, we cannot access
            return false;
        }
        if (access(Directory,W_OK | R_OK| X_OK)==-1) return false;
    }
    free((void *) directory);
    directory = Directory ? strdup(Directory) : NULL;
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

void cGlobalInfosatepg::AddChannel(tChannelID ChannelID,int Usage)
{
    infosatchannels=(struct infosatchannels *) realloc(infosatchannels,
                    (numinfosatchannels+1)*sizeof(struct infosatchannels));
    if (!infosatchannels) return;
    infosatchannels[numinfosatchannels].ChannelID=ChannelID;
    infosatchannels[numinfosatchannels].Usage=Usage;
    numinfosatchannels++;
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

bool cGlobalInfosatepg::SetChannelUse(int Index,int Usage)
{
    if (numinfosatchannels==0) return false;
    if ((Index<0) || (Index>numinfosatchannels-1)) return false;
    bool ret=false;
    if (infosatchannels[Index].Usage!=Usage) ret=true;
    infosatchannels[Index].Usage=Usage;
    return ret;
}

int cGlobalInfosatepg::GetChannelUse(int Index)
{
    if (numinfosatchannels==0) return USE_NOTHING;
    if ((Index<0) || (Index>numinfosatchannels-1)) return USE_NOTHING;
    return infosatchannels[Index].Usage;
}

int cGlobalInfosatepg::Save()
{
    char file[1024];
    snprintf(file,sizeof(file),"%s/infosatepg.dat",directory);

    int f=open(file,O_RDWR|O_CREAT|O_TRUNC,0664);
    if (f==-1)
    {
        if (errno!=ENOSPC)
        {
            esyslog("infosatepg: unable to create file '%s'", file);
        }
        return -1;
    }

    int ret;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        ret=Infosatdata[mac].Save(f);
        if (ret==-1) break;
    }

    if (ret!=-1)
    {
        ret=write(f,&this_day,sizeof(this_day));
        if (ret!=sizeof(this_day)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=write(f,&this_month,sizeof(this_month));
        if (ret!=sizeof(this_month)) ret=-1;
    }

    close(f);
    if (ret==-1) unlink(file);
    return ret;
}

int cGlobalInfosatepg::Load()
{
    char file[1024];
    snprintf(file,sizeof(file),"%s/infosatepg.dat",directory);

    int f=open(file,O_RDONLY);
    if (f==-1) return 0; // it's ok if the file doesn't exist

    int ret;
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        ret=Infosatdata[mac].Load(f);
        if (ret==-1) break;
    }
    if (ret!=-1)
    {
        ret=read(f,&this_day,sizeof(this_day));
        if (ret!=sizeof(this_day)) ret=-1;
    }
    if (ret!=-1)
    {
        ret=read(f,&this_month,sizeof(this_month));
        if (ret!=sizeof(this_month)) ret=-1;
    }

    close(f);
    if (ret==-1)
    {
        unlink(file); // but it's not ok if the read failed!
        for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
        {
            Infosatdata[mac].Init(NULL,0,0,0);
        }

    }
    return ret;
}

void cGlobalInfosatepg::ResetProcessedFlags(void)
{
    dsyslog("infosatepg: reprocess files (later)");
    for (int mac=EPG_FIRST_DAY_MAC; mac<=EPG_LAST_DAY_MAC; mac++)
    {
        Infosatdata[mac].ResetProcessed();
    }
}

void cGlobalInfosatepg::Lock(time_t Now)
{
    struct tm tm;
    if (!localtime_r(&Now,&tm))
    {
        this_day=-1;
        this_month=-1;
    }
    else
    {
        if ((tm.tm_mday!=this_day) || ((tm.tm_mon+1)!=this_month))
            isyslog("infosatepg: all data processed");
        this_day=tm.tm_mday;
        this_month=tm.tm_mon+1;
    }
}

bool cGlobalInfosatepg::isLocked(int *Day, int *Month)
{
    if (Day) *Day=this_day;
    if (Month) *Month=this_month;
    time_t Now=time(NULL);
    struct tm tm;
    if (!localtime_r(&Now,&tm)) return false;
    if ((tm.tm_mday==this_day) && ((tm.tm_mon+1)==this_month)) return true;
    return false;
}
