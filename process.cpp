/*
 * process.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <stdio.h>
#include <vdr/epg.h>
#include <vdr/tools.h>
#include <vdr/skins.h>
#include <ctype.h>
#include "process.h"
#include "readline.h"

// --- cInfosatevent
cInfosatevent::cInfosatevent()
{
    title = NULL;
    shortText = NULL;
    description = NULL;
    announcement=NULL;
    country=NULL;
    genre=NULL;
    original=NULL;
    year=-1;
    fsk=-1;
    category=-1;
    startTime = 0;
    duration = 0;
}

cInfosatevent::~cInfosatevent()
{
    free(title);
    free(shortText);
    free(description);
    free(announcement);
    free(country);
    free(genre);
    free(original);
}

void cInfosatevent::SetOriginal(const char *Original)
{
    original = strcpyrealloc(original, Original);
}

void cInfosatevent::SetGenre(const char *Genre)
{
    genre = strcpyrealloc(genre, Genre);
}

void cInfosatevent::SetCountry(const char *Country)
{
    country = strcpyrealloc(country, Country);
}

void cInfosatevent::SetAnnouncement(const char *Announcement)
{
    announcement = strcpyrealloc(announcement, Announcement);
}

void cInfosatevent::SetTitle(const char *Title)
{
    title = strcpyrealloc(title, Title);
}

void cInfosatevent::SetShortText(const char *ShortText)
{
    shortText = strcpyrealloc(shortText, ShortText);
}

void cInfosatevent::SetDescription(const char *Description)
{
    description = strcpyrealloc(description, Description);
}

const char *cInfosatevent::Description(const char *oldDescription)
{
    //  Add Category:, Genre:, Year:, Country:, Originaltitle:, FSK: , Rating: [if available] ...
    /*
      char fmt[100];
      char *descr;

      if (oldDescription) {
         descr=(char *) oldDescription;
         char *extEPG=strstr(descr,"\n\n");
         if (extEPG) *extEPG=0;
      } else {
         descr=description;
      }

      descr=strcatrealloc(descr,"\n\n");

      if (category!=-1) {
        descr=strcatrealloc(descr,"Category: ");
        descr=strcatrealloc(descr,Category());
        descr=strcatrealloc(descr,"\n");
      }
      if (genre) {
        descr=strcatrealloc(descr,"Genre: ");
        descr=strcatrealloc(descr,Genre());
        descr=strcatrealloc(descr,"\n");
      }
      if (year!=-1) {
        strncat(fmt,"Year: %i\n",9);
      }
      if (country) {
        descr=strcatrealloc(descr,"Country: ");
        descr=strcatrealloc(descr,Country());
        descr=strcatrealloc(descr,"\n");
      }
      if (original) {
        descr=strcatrealloc(descr,"Originaltitle: ");
        descr=strcatrealloc(descr,Original());
        descr=strcatrealloc(descr,"\n");
      }
      if (fsk!=-1) {
        strncat(fmt,"FSK: %i\n",8);
      }
      if (announcement) {
        descr=strcatrealloc(descr,"Rating: ");
        descr=strcatrealloc(descr,Announcement());
        descr=strcatrealloc(descr,"\n");
      }

    */
    return (const char*) description;
}

// --- cProcessInfosatepg
cProcessInfosatepg::cProcessInfosatepg(int Mac, cGlobalInfosatepg *Global)
{
    global=Global;

    FILE *f;
    const char *file = global->Infosatdata[Mac].GetFile();
    f=fopen(file,"r");
    if (f)
    {
        int firststarttime=-1;
        if (ParseInfosatepg(f,&firststarttime))
        {
            global->SetWakeupTime(firststarttime);
            global->Infosatdata[Mac].Processed=true;
        }
        else
        {
            esyslog("infosatepg: failed to get writelock while parsing %s",global->Infosatdata[Mac].GetFile());
        }
        fclose(f);
    }
}

cEvent *cProcessInfosatepg::SearchEvent(cSchedule* Schedule, cInfosatevent *iEvent)
{
    cEvent *f=NULL;
    int maxdiff=MAX_EVENTTIMEDIFF*60;

    for (cEvent *p = Schedule->Events()->First(); p; p = Schedule->Events()->Next(p))
    {
        if (!strcmp(p->Title(),iEvent->Title()))
        {
            // found event with same title
            int diff=abs((int) difftime(p->StartTime(),iEvent->StartTime()));
            if (diff<=global->EventTimeDiff)
            {
                if (diff<maxdiff)
                {
                    f=p;
                    maxdiff=diff;
                }
            }
        }
    }
    return f;
}

bool cProcessInfosatepg::AddInfosatEvent(cChannel *channel, cInfosatevent *iEvent)
{
    if ((!channel) || (!iEvent)) return true;
    if (iEvent->GetEventUse()==USE_NOTHING) return true; // this should never happen!
    if (iEvent->StartTime()<time(NULL)) return true; // don't deal with old events
    if (iEvent->StartTime()>(time(NULL)+86400)) return true; // don't deal with events to far in the future

    cSchedulesLock SchedulesLock(true,2000); // to be safe ;)
    const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
    if (!Schedules) return false; // No write lock -> try later!
    cSchedule* Schedule = (cSchedule *) Schedules->GetSchedule(channel,true);
    if (!Schedule) return true;

    time_t start;

    const cEvent *lastEvent=Schedule->Events()->Last();
    if ((lastEvent) && (iEvent->StartTime()<lastEvent->EndTime()))
    {
        // try to find, 1st with EventID
        cEvent *Event = (cEvent *) Schedule->GetEvent(iEvent->EventID());
        // 2nd with StartTime +/- WaitTime
        if (!Event) Event= (cEvent *) SearchEvent(Schedule,iEvent);
        if (!Event) return true; // just bail out with ok

        start=iEvent->StartTime();
        dsyslog("infosatepg: ievent %s [%s]", iEvent->Title(),ctime(&start));

        // change existing event
        Event->SetShortText(iEvent->ShortText());
        if (iEvent->GetEventUse()==USE_SHORTLONGTEXT)
        {
            Event->SetDescription(iEvent->Description(NULL));
        }
        if (iEvent->GetEventUse()==USE_SHORTTEXTEPG)
        {
            Event->SetDescription(iEvent->Description(Event->Description()));
        }
        if (iEvent->GetEventUse()==USE_INTELLIGENT)
        {
            if (!Event->Description() || (!iEvent->Description(NULL)))
            {
                Event->SetDescription(iEvent->Description(NULL));
            }
            else
            {
                if (strlen(iEvent->Description(NULL))>strlen(Event->Description()))
                {
                    Event->SetDescription(iEvent->Description(NULL));
                }
            }
        }
        Event->SetTableID(0);
        Event->SetSeen();
        start=Event->StartTime();
        dsyslog("infosatepg: changed event %s [%s]",Event->Title(),ctime(&start));
    }
    else
    {
        // we are beyond the last event, so just add (if we should)
        if (iEvent->GetEventUse()!=USE_ALL) return true;
        start=iEvent->StartTime();
        cEvent *Event = new cEvent(iEvent->EventID());
        if (!Event) return true;
        Event->SetTitle(iEvent->Title());
        Event->SetShortText(iEvent->ShortText());
        Event->SetDescription(iEvent->Description(NULL));
        Event->SetStartTime(iEvent->StartTime());
        Event->SetDuration(iEvent->Duration());
        Event->SetVersion(0);
        start=iEvent->StartTime();
        dsyslog("infosatepg: adding new event %s [%s]",iEvent->Title(),ctime(&start));
        Schedule->AddEvent(Event);
    }

    return true;
}

cChannel *cProcessInfosatepg::GetInfosatChannel(int frequency, int sid)
{
    cChannel *chan;
    int source = cSource::FromString("S19.2E"); // only from astra 19.2E

    for (int i=0; i<=Channels.MaxNumber(); i++)
    {
        chan = Channels.GetByNumber(i,0);
        if (chan)
        {
            if (chan->Source()!=source) continue;
            if (chan->Sid()!=sid) continue;

            if ((chan->Frequency()==frequency) || (chan->Frequency()+1==frequency) ||
                    (chan->Frequency()-1==frequency))
            {
                return chan;
            }
        }
    }
    return NULL;
}

bool cProcessInfosatepg::CheckOriginal(char *s,cInfosatevent *iEvent,cCharSetConv *conv)
{
    char *pOT,*pEOT;
    pOT=strchr(s,'(');
    if (!pOT) return false;
    pEOT=strrchr(pOT,')');
    if (!pEOT) return false;
    if (pEOT[1]!=0) return false;

    *pOT=*pEOT=0;
    *pOT++;
    if (pOT[-1]==' ') pOT[-1]=0;
    // check some things
    if (!strcmp(pOT,"TM")) return false;
    char *endp=NULL;
    long int ret =strtol(pOT,&endp,10);
    if ((ret!=0) && (*endp==0)) return false;
    // with original title
    iEvent->SetOriginal(conv->Convert(pOT));
    return true;
}

bool cProcessInfosatepg::CheckAnnouncement(char *s,cInfosatevent *iEvent)
{
    bool ret=true;
    if ((strlen(s)==16) && (!strcmp(s,"Erstausstrahlung")))
    {
        iEvent->SetAnnouncement("Erstausstrahlung");
    }
    else if ((strlen(s)==20) && (!strcmp(s,"Deutschland-Premiere")))
    {
        iEvent->SetAnnouncement("Deutschland-Premiere");
    }
    else if ((strlen(s)>=16) && (!strncmp(s,"Free-TV-Premiere",16)))
    {
        iEvent->SetAnnouncement("Free-TV-Premiere!");
    }
    else if ((strlen(s)>=9) && (!strncmp(s,"Highlight",9)))
    {
        iEvent->SetAnnouncement("Tipp");
    }
    else ret=false;
    return ret;
}

bool cProcessInfosatepg::ParseInfosatepg(FILE *f,int *firststarttime)
{
    char *s,tag;
    int fields,index;
    size_t size;
    time_t oldstart;
    bool ignore=true;
    bool abort=false;
    struct tm tm;
    int ieventnr=1;
    cChannel *chan=NULL;
    cInfosatevent *ievent=NULL;
#if VDRVERSNUM < 10701
    cCharSetConv *conv = new cCharSetConv("ISO-8859-1",cCharSetConv::SystemCharacterTable() ?
                                          cCharSetConv::SystemCharacterTable() : "UTF-8");
#else
    cCharSetConv *conv = new cCharSetConv("ISO-8859-1",NULL);
#endif

    cReadLineInfosatepg ReadLine;
    while ((s=ReadLine.Read(f,&size))!=NULL)
    {
        if (size<3) continue;
        if (isdigit(s[0]))
        {
            if (ievent)
            {
                int category,fsk;
                fields=sscanf(s,"%d %d",&category,&fsk);
                if (fields==1) ievent->SetCategory(category);
                if (fields==2)
                {
                    ievent->SetCategory(category);
                    ievent->SetFSK(fsk);
                }
            }
            continue;
        }

        s++;
        tag=*s;
        s+=2;
        switch (tag)
        {
        case 'P':
            // contains program (schedule)
            ignore=true;
            int frequency,sid,day,month,year,hour,minute;
            fields=sscanf(s,"%*[^(](%*[^,],%d,%d) %d.%d.%d %*s %*s %d:%d",
                          &frequency,&sid,&day,&month,&year,&hour,&minute);
            if (fields==7)
            {
                // got all fields
                chan=GetInfosatChannel(frequency,sid);
                if (chan)
                {
                    if (!global->ChannelExists(chan->GetChannelID(),&index))
                    {
                        if (!global->InfosatChannels())
                        {
                            // Send message
                            Skins.QueueMessage(mtInfo,tr("Infosat channellist available"));
                        }
                        // Channel is not in global list->add
                        global->AddChannel(chan->GetChannelID(),USE_NOTHING);
                    }
                    else
                    {
                        if (global->GetChannelUse(index)!=USE_NOTHING)
                        {
                            memset(&tm,0,sizeof(tm));
                            tm.tm_sec=0;
                            tm.tm_min=minute;
                            tm.tm_hour=hour;
                            tm.tm_mday=day;
                            tm.tm_mon=month-1;
                            tm.tm_year=year-1900;
                            tm.tm_isdst=-1;
                            oldstart=mktime(&tm);
                            dsyslog("infosatepg: using '%s'",s);
                            dsyslog("infosatepg: start on %02i.%02i.%04i %02i:%02i",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,
                                    tm.tm_hour,tm.tm_min);
                            ignore=false;
                            ieventnr=1;
                        }
                    }
                }
            }
            break;
        case 'E':
            // contains event / title
            if (ignore) continue;
            if (ievent)
            {
                // There was an event without long description -> add
                if (!AddInfosatEvent(chan,ievent)) abort=true;
                oldstart=ievent->StartTime();
                delete ievent; // delete old event
                ievent =NULL;
            }
            else
            {
                oldstart=(time_t) -1;
            }
            int shour,sminute;
            char *title;
            fields=sscanf(s,"%d:%d %a[^^]",&shour,&sminute,&title);
            if (fields==3)
            {
                if (!ievent) ievent = new cInfosatevent;
                tm.tm_hour=shour;
                tm.tm_min=sminute;

                if (*firststarttime==-1)
                {
                    shour-=2;
                    if (shour<0) shour=24+shour;
                    *firststarttime=(shour*100)+sminute;
                }

                tm.tm_isdst=-1;
                time_t start=mktime(&tm);
                if ((oldstart!=(time_t) -1) && (difftime(start,oldstart)<0))
                {
                    // last ievent was yesterday
                    tm.tm_isdst=-1;
                    tm.tm_mday++;
                    start=mktime(&tm);
                }
                ievent->SetStartTime(start);
                ievent->SetTitle(conv->Convert(title));
                free(title);
                ievent->SetEventUse(global->GetChannelUse(index));
                ievent->SetEventID(DoSum(ieventnr++,ievent->Title(),strlen(ievent->Title())));
            }
            else
            {
                dsyslog("infosatepg: ERROR sscanf failed, just %i fields [%s]", fields,s);
            }
            break;

        case 'S':
            if (ignore) continue;
            if (!ievent) continue;
            // contains:
            // short description (,maybe special announcement)<8A>
            // Genre (,maybe Country and Year), Duration (sometimes missing)
            char *p8A,*pSA;
            p8A=strchr(s,0x8A);
            if (p8A)
            {
                *p8A=0;
                // check for special announcements:
                // Erstausstrahlung
                // Deutschland-Premiere
                // Free-TV-Premiere
                // Highlight
                pSA=strrchr(s,',');
                if (pSA)
                {
                    if (CheckAnnouncement(pSA+2,ievent))
                    {
                        // announcement was added to short description
                        *pSA=0;
                    }
                    CheckOriginal(s,ievent,conv);
                    ievent->SetShortText(conv->Convert(s));
                }
                else
                {
                    if (!CheckAnnouncement(s,ievent))
                    {
                        CheckOriginal(s,ievent,conv);
                        ievent->SetShortText(conv->Convert(s));
                    }
                }
                s=++p8A;
            }
            char *pDU;
            pDU=strrchr(s,',');
            if (pDU)
            {
                *pDU=0;
                // with genre
                if (CheckAnnouncement(s,ievent))
                {
                    char *tp8A;
                    tp8A=strchr(s,0x8A);
                    if (tp8A) s=++tp8A;
                }
                char *pCY;
                pCY=strchr(s,',');
                if (pCY)
                {
                    // with country or year
                    *pCY=0;
                    pCY+=2;
                    if (isdigit(*pCY))
                    {
                        // just year (checked)
                        ievent->SetYear(atoi(pCY));
                    }
                    else
                    {
                        char *pSP;
                        pSP=strrchr(pCY,' ');
                        if (pSP)
                        {
                            *pSP=0;
                            pSP++;
                            // country and year (check!!)
                            int year = atoi(pSP);
                            if (year!=0) ievent->SetYear(year);
                            ievent->SetCountry(conv->Convert(pCY));
                        }
                        else
                        {
                            // just country
                            ievent->SetCountry(conv->Convert(pCY));
                        }
                    }
                }
                ievent->SetGenre(conv->Convert(s));
                s=++pDU;
            }
            int len;
            len=strlen(s);
            if ((s[len-1]=='.') && (s[len-2]=='n') && (s[len-3]=='i') && (s[len-4]=='M'))
            {
                // Duration
                ievent->SetDuration(60*atoi(s));
            }
            break;

        case 'L':
            // contains description
            if (ignore) continue;
            if (!ievent) continue;
            strreplace(s,0x8A,'\n');
            ievent->SetDescription(conv->Convert(s));
            if (!AddInfosatEvent(chan,ievent)) abort=true;
            oldstart=ievent->StartTime();
            delete ievent;
            ievent=NULL;
            break;
        case 'Q':
            if (ignore) continue;
            if (ievent)
            {
                if (!AddInfosatEvent(chan,ievent)) abort=true;
                delete ievent;
                ievent=NULL;
            }
        default:
            continue;
        }

        if (abort) break;

    }
    delete conv;
    return (!abort);
}

u_long cProcessInfosatepg::DoSum(u_long sum, const char *buf, int nBytes)
{
    int nleft=nBytes;
    u_short *w = (u_short*)buf;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        u_short answer = 0;
        *(u_char*)(&answer) = *(u_char*)w;
        sum += answer;
    }
    return sum;
}
