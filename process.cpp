/*
 * process.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <stdio.h>
#include <vdr/skins.h>
#include <ctype.h>
#include "process.h"
#include "readline.h"

char *strcatrealloc(char *dest, const char *src)
{
    if (!src || !*src)
        return dest;

    size_t l = (dest ? strlen(dest) : 0) + strlen(src) + 1;
    if (dest)
    {
        dest = (char *)realloc(dest, l);
        strcat(dest, src);
    }
    else
    {
        dest = (char*)malloc(l);
        strcpy(dest, src);
    }
    return dest;
}

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
    extepg=NULL;
    episode=NULL;
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
    free(extepg);
    free(episode);
}

void cInfosatevent::SetOriginal(const char *Original)
{
    original = strcpyrealloc(original, Original);
    original = compactspace(original);
}

void cInfosatevent::SetGenre(const char *Genre)
{
    genre = strcpyrealloc(genre, Genre);
    genre = compactspace(genre);
}

void cInfosatevent::SetCountry(const char *Country)
{
    country = strcpyrealloc(country, Country);
    country = compactspace(country);
}

void cInfosatevent::SetAnnouncement(const char *Announcement)
{
    announcement = strcpyrealloc(announcement, Announcement);
    announcement = compactspace(announcement);
}

void cInfosatevent::SetTitle(const char *Title)
{
    title = strcpyrealloc(title, Title);
    title = compactspace(title);
}

void cInfosatevent::SetEpisode(const char *Episode)
{
    episode=strcpyrealloc(episode,Episode);
    episode=compactspace(episode);
}

void cInfosatevent::SetShortText(const char *ShortText)
{
    if (!ShortText) return;

    char *tmpText=strcpyrealloc(shortText,ShortText);
    if (!tmpText) return;
    tmpText=compactspace(tmpText);

    if (title)
    {
        if (!strcmp(title,tmpText))
        {
            free(tmpText);
            return; // ShortText same as title -> ignore
        }
    }
    shortText = tmpText;
}

void cInfosatevent::SetDescription(const char *Description)
{
    description = strcpyrealloc(description, Description);
    description = compactspace(description);
}

const char *cInfosatevent::ExtEPG(void)
{
    //  Returns Category:, Genre:, Year:, Country:, Originaltitle:, FSK: , Rating: [if available] ...
    char fmt[100];

    if (extepg)
    {
        free(extepg);
        extepg=NULL;
    }

    extepg=strcatrealloc(extepg,"\n\n");

    if (category!=-1)
    {
        sprintf(fmt,"Category: %i\n",category);
        extepg=strcatrealloc(extepg,fmt);
    }
    if (genre)
    {
        extepg=strcatrealloc(extepg,"Genre: ");
        extepg=strcatrealloc(extepg,genre);
        extepg=strcatrealloc(extepg,"\n");
    }
    if (year!=-1)
    {
        sprintf(fmt,"Year: %i\n",year);
        extepg=strcatrealloc(extepg,fmt);
    }
    if (country)
    {
        extepg=strcatrealloc(extepg,"Country: ");
        extepg=strcatrealloc(extepg,country);
        extepg=strcatrealloc(extepg,"\n");
    }
    if (original)
    {
        extepg=strcatrealloc(extepg,"Originaltitle: ");
        extepg=strcatrealloc(extepg,original);
        extepg=strcatrealloc(extepg,"\n");
    }
    if (fsk!=-1)
    {
        sprintf(fmt,"FSK: %i\n",fsk);
        extepg=strcatrealloc(extepg,fmt);
    }
    if (episode)
    {
        extepg=strcatrealloc(extepg,"Episode: ");
        extepg=strcatrealloc(extepg,episode);
        extepg=strcatrealloc(extepg,"\n");
    }
    if (announcement)
    {
        extepg=strcatrealloc(extepg,"Rating: ");
        extepg=strcatrealloc(extepg,announcement);
        extepg=strcatrealloc(extepg,"\n");
    }

    return (const char*) extepg;
}

// --- cProcessInfosatepg
cProcessInfosatepg::cProcessInfosatepg(int Mac, cGlobalInfosatepg *Global)
//void cProcessInfosatepg::Action() //int Mac, cGlobalInfosatepg *Global)
{
//    int Mac=0;
//    cGlobalInfosatepg *Global=NULL;

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
    if (iEvent->Usage()==USE_NOTHING) return true; // this should never happen!
    if (iEvent->StartTime()<time(NULL)) return true; // don't deal with old events
    // don't deal with events to far in the future
    if (iEvent->StartTime()>(time(NULL)+(iEvent->Days()*86400))) return true;

    cSchedulesLock SchedulesLock(true,2000); // to be safe ;)
    const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);
    if (!Schedules) return false; // No write lock -> try later!
    cSchedule* Schedule = (cSchedule *) Schedules->GetSchedule(channel,true);
    if (!Schedule) return true; // No schedule -> do nothing (is this ok?)

    time_t start=0;
    cEvent *Event=NULL;
    const cEvent *lastEvent=Schedule->Events()->Last();

    if ((lastEvent) && (iEvent->StartTime()<lastEvent->EndTime()))
    {
        // try to find, 1st with our own EventID
        Event = (cEvent *) Schedule->GetEvent(iEvent->EventID());
        // 2nd with StartTime +/- WaitTime
        if (!Event) Event= (cEvent *) SearchEvent(Schedule,iEvent);
        if (!Event) return true; // just bail out with ok

        start=iEvent->StartTime();
        dsyslog("infosatepg: changing event %s [%s]", iEvent->Title(),ctime(&start));

        // change existing event, prevent EIT EPG to update
        Event->SetTableID(0);
        Event->SetVersion(0xff); // is that ok?
        Event->SetSeen(); // meaning of this?
    }
    else
    {
        // we are beyond the last event, so just add (if we should)
        if ((iEvent->Usage() & USE_APPEND)!=USE_APPEND) return true;
        Event = new cEvent(iEvent->EventID());
        if (!Event) return true;
        Event->SetStartTime(iEvent->StartTime());
        Event->SetDuration(iEvent->Duration());
        Event->SetTitle(iEvent->Title());
        start=iEvent->StartTime();
        dsyslog("infosatepg: adding new event %s [%s]",iEvent->Title(),ctime(&start));
        Schedule->AddEvent(Event);
    }

    if ((iEvent->Usage() & USE_SHORTTEXT) == USE_SHORTTEXT)
    {
        Event->SetShortText(iEvent->ShortText());
    }

    if ((iEvent->Usage() & USE_LONGTEXT) == USE_LONGTEXT)
    {
        if ((iEvent->Usage() & USE_MERGELONGTEXT)==USE_MERGELONGTEXT)
        {
            // first check if we have a description
            const char *iDescr=iEvent->Description();
            if (iDescr)
            {
                const char *oDescr=Event->Description();
                if (oDescr)
                {
                    // there is an "old" description
                    if (strlen(iDescr)>strlen(oDescr))
                    {
                        // more text in infosat
                        Event->SetDescription(iEvent->Description());
                    }
                }
                else
                {
                    // event has no description, just use infosat one
                    Event->SetDescription(iEvent->Description());
                }
            }
        }
        else
        {
            // just do it the easy way
            Event->SetDescription(iEvent->Description());
        }
    }

    if ((iEvent->Usage() & USE_EXTEPG)==USE_EXTEPG)
    {
        const char *oDescr=Event->Description();
        const char *extEPG=iEvent->ExtEPG();

        if (extEPG)
        {
            // we have extended EPG info
            if (!oDescr)
            {
                // no old description -> just use extEPG
                Event->SetDescription(extEPG);
            }
            else
            {
                // add extEPG to description
                char *nDescr=strdup(oDescr);

                strreplace(nDescr,extEPG,"");

                nDescr=strcatrealloc(nDescr,extEPG);
                Event->SetDescription(nDescr);
                free(nDescr);
            }
        }
    }

    return true;
}

cChannel *cProcessInfosatepg::GetVDRChannel(int frequency, int sid)
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

    if (pEOT[-1]==')')
    {
        char *pTMP = strchr(&pOT[1],'(');
        if (pTMP)
        {
            pOT=pTMP;
        }
    }
    else
    {
        // just one brace
        if (isdigit(pOT[1])) return false;
    }

    *pOT=*pEOT=0;
    *pOT++;
    if (pOT[-1]==' ') pOT[-1]=0;
    // check some things
    if (!strcmp(pOT,"TM")) return false;
    // check for (number)
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
    time_t oldstart=-1;
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
                chan=GetVDRChannel(frequency,sid);
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
                        global->AddChannel(chan->GetChannelID(),USE_NOTHING,1);
                    }
                    else
                    {
                        if (global->GetChannelUsage(index)!=USE_NOTHING)
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
                            dsyslog("infosatepg: start on %02i.%02i.%04i %02i:%02i (%s)",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,
                                    tm.tm_hour,tm.tm_min,asctime(&tm));
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
                delete ievent; // delete old event
                ievent =NULL;
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
                oldstart=start;
                ievent->SetStartTime(start);
                ievent->SetTitle(conv->Convert(title));
                free(title);
                ievent->SetEventUsage(global->GetChannelUsage(index));
                ievent->SetEventDays(global->GetChannelDays(index));
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
