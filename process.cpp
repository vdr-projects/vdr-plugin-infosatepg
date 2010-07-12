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
    title=NULL;
    shorttext=NULL;
    description=NULL;
    announcement=NULL;
    country=NULL;
    genre=NULL;
    original=NULL;
    extepg=NULL;
    episode=NULL;
    category=NULL;
    addition=NULL;
    rating=NULL;
    year=-1;
    fsk=-1;
    content=0;
    startTime = 0;
    duration = 0;
}

cInfosatevent::~cInfosatevent()
{
    free(title);
    free(shorttext);
    free(description);
    free(announcement);
    free(country);
    free(category);
    free(genre);
    free(original);
    free(extepg);
    free(episode);
    free(addition);
    free(rating);
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

void cInfosatevent::SetCategory(const char *Category)
{
    category = strcpyrealloc(category, Category);
    category = compactspace(category);
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

void cInfosatevent::SetAddition(const char *Addition)
{
    addition = strcpyrealloc(addition, Addition);
    addition = compactspace(addition);
}

void cInfosatevent::SetRating(const char *Rating)
{
    rating = strcpyrealloc(rating, Rating);
    rating = compactspace(rating);
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
    shorttext=strcpyrealloc(shorttext,ShortText);
    shorttext=compactspace(shorttext);
}

void cInfosatevent::SetDescription(const char *Description)
{
    description = strcpyrealloc(description, Description);
    description = compactspace(description);
}

void cInfosatevent::SetCategoryByID(int i)
{
    i>>=4;
    switch (i & 0xF0)
    {
    case EVCONTENTMASK_MOVIEDRAMA:
        SetCategory(tr("Content$Movie/Drama"));
        break;
    case EVCONTENTMASK_NEWSCURRENTAFFAIRS:
        SetCategory(tr("Content$News/Current Affairs"));
        break;
    case EVCONTENTMASK_SHOW:
        SetCategory(tr("Content$Show/Game Show"));
        break;
    case EVCONTENTMASK_SPORTS:
        SetCategory(tr("Content$Sports"));
        break;
    case EVCONTENTMASK_CHILDRENYOUTH:
        SetCategory(tr("Content$Children's/Youth Programmes"));
        break;
    case EVCONTENTMASK_MUSICBALLETDANCE:
        SetCategory(tr("Content$Music/Ballet/Dance"));
        break;
    case EVCONTENTMASK_ARTSCULTURE:
        SetCategory(tr("Content$Arts/Culture"));
        break;
    case EVCONTENTMASK_SOCIALPOLITICALECONOMICS:
        SetCategory(tr("Content$Social/Political/Economics"));
        break;
    case EVCONTENTMASK_EDUCATIONALSCIENCE:
        SetCategory(tr("Content$Education/Science/Factual"));
        break;
    case EVCONTENTMASK_LEISUREHOBBIES:
        SetCategory(tr("Content$Leisure/Hobbies"));
        break;
    case EVCONTENTMASK_SPECIAL:
        SetCategory(tr("Content$Special"));
        break;
    case EVCONTENTMASK_USERDEFINED:
        SetCategory(tr("Content$Userdefined"));
        break;
    }
}

void cInfosatevent::SetGenreByID(int i)
{
    i >>= 4;
    switch (i & 0xF0)
    {
    case EVCONTENTMASK_MOVIEDRAMA:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Movie/Drama"));
            break;
        case 0x01:
            SetGenre(tr("Content$Detective/Thriller"));
            break;
        case 0x02:
            SetGenre(tr("Content$Adventure/Western/War"));
            break;
        case 0x03:
            SetGenre(tr("Content$Science Fiction/Fantasy/Horror"));
            break;
        case 0x04:
            SetGenre(tr("Content$Comedy"));
            break;
        case 0x05:
            SetGenre(tr("Content$Soap/Melodrama/Folkloric"));
            break;
        case 0x06:
            SetGenre(tr("Content$Romance"));
            break;
        case 0x07:
            SetGenre(tr("Content$Serious/Classical/Religious/Historical Movie/Drama"));
            break;
        case 0x08:
            SetGenre(tr("Content$Adult Movie/Drama"));
            break;
        }
        break;

    case EVCONTENTMASK_NEWSCURRENTAFFAIRS:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$News/Current Affairs"));
            break;
        case 0x01:
            SetGenre(tr("Content$News/Weather Report"));
            break;
        case 0x02:
            SetGenre(tr("Content$News Magazine"));
            break;
        case 0x03:
            SetGenre(tr("Content$Documentary"));
            break;
        case 0x04:
            SetGenre(tr("Content$Discussion/Inverview/Debate"));
            break;
        }
        break;

    case EVCONTENTMASK_SHOW:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Show/Game Show"));
            break;
        case 0x01:
            SetGenre(tr("Content$Game Show/Quiz/Contest"));
            break;
        case 0x02:
            SetGenre(tr("Content$Variety Show"));
            break;
        case 0x03:
            SetGenre(tr("Content$Talk Show"));
            break;
        }
        break;

    case EVCONTENTMASK_SPORTS:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Sports"));
            break;
        case 0x01:
            SetGenre(tr("Content$Special Event"));
            break;
        case 0x02:
            SetGenre(tr("Content$Sport Magazine"));
            break;
        case 0x03:
            SetGenre(tr("Content$Football"));
            break;
        case 0x04:
            SetGenre(tr("Content$Tennis/Squash"));
            break;
        case 0x05:
            SetGenre(tr("Content$Team Sports"));
            break;
        case 0x06:
            SetGenre(tr("Content$Athletics"));
            break;
        case 0x07:
            SetGenre(tr("Content$Motor Sport"));
            break;
        case 0x08:
            SetGenre(tr("Content$Water Sport"));
            break;
        case 0x09:
            SetGenre(tr("Content$Winter Sports"));
            break;
        case 0x0A:
            SetGenre(tr("Content$Equestrian"));
            break;
        case 0x0B:
            SetGenre(tr("Content$Martial Sports"));
            break;
        }
        break;

    case EVCONTENTMASK_CHILDRENYOUTH:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Children's/Youth Programmes"));
            break;
        case 0x01:
            SetGenre(tr("Content$Pre-school Children's Programmes"));
            break;
        case 0x02:
            SetGenre(tr("Content$Entertainment Programmes for 6 to 14"));
            break;
        case 0x03:
            SetGenre(tr("Content$Entertainment Programmes for 10 to 16"));
            break;
        case 0x04:
            SetGenre(tr("Content$Informational/Educational/School Programme"));
            break;
        case 0x05:
            SetGenre(tr("Content$Cartoons/Puppets"));
            break;
        }
        break;

    case EVCONTENTMASK_MUSICBALLETDANCE:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Music/Ballet/Dance"));
            break;
        case 0x01:
            SetGenre(tr("Content$Rock/Pop"));
            break;
        case 0x02:
            SetGenre(tr("Content$Serious/Classical Music"));
            break;
        case 0x03:
            SetGenre(tr("Content$Folk/Tradional Music"));
            break;
        case 0x04:
            SetGenre(tr("Content$Jazz"));
            break;
        case 0x05:
            SetGenre(tr("Content$Musical/Opera"));
            break;
        case 0x06:
            SetGenre(tr("Content$Ballet"));
            break;
        }
        break;

    case EVCONTENTMASK_ARTSCULTURE:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Arts/Culture"));
            break;
        case 0x01:
            SetGenre(tr("Content$Performing Arts"));
            break;
        case 0x02:
            SetGenre(tr("Content$Fine Arts"));
            break;
        case 0x03:
            SetGenre(tr("Content$Religion"));
            break;
        case 0x04:
            SetGenre(tr("Content$Popular Culture/Traditional Arts"));
            break;
        case 0x05:
            SetGenre(tr("Content$Literature"));
            break;
        case 0x06:
            SetGenre(tr("Content$Film/Cinema"));
            break;
        case 0x07:
            SetGenre(tr("Content$Experimental Film/Video"));
            break;
        case 0x08:
            SetGenre(tr("Content$Broadcasting/Press"));
            break;
        case 0x09:
            SetGenre(tr("Content$New Media"));
            break;
        case 0x0A:
            SetGenre(tr("Content$Arts/Culture Magazines"));
            break;
        case 0x0B:
            SetGenre(tr("Content$Fashion"));
            break;
        }
        break;

    case EVCONTENTMASK_SOCIALPOLITICALECONOMICS:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Social/Political/Economics"));
            break;
        case 0x01:
            SetGenre(tr("Content$Magazines/Reports/Documentary"));
            break;
        case 0x02:
            SetGenre(tr("Content$Economics/Social Advisory"));
            break;
        case 0x03:
            SetGenre(tr("Content$Remarkable People"));
            break;
        }
        break;

    case EVCONTENTMASK_EDUCATIONALSCIENCE:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Education/Science/Factual"));
            break;
        case 0x01:
            SetGenre(tr("Content$Nature/Animals/Environment"));
            break;
        case 0x02:
            SetGenre(tr("Content$Technology/Natural Sciences"));
            break;
        case 0x03:
            SetGenre(tr("Content$Medicine/Physiology/Psychology"));
            break;
        case 0x04:
            SetGenre(tr("Content$Foreign Countries/Expeditions"));
            break;
        case 0x05:
            SetGenre(tr("Content$Social/Spiritual Sciences"));
            break;
        case 0x06:
            SetGenre(tr("Content$Further Education"));
            break;
        case 0x07:
            SetGenre(tr("Content$Languages"));
            break;
        }
        break;

    case EVCONTENTMASK_LEISUREHOBBIES:
        switch (i & 0x0F)
        {
        default:
        case 0x00:
            SetGenre(tr("Content$Leisure/Hobbies"));
            break;
        case 0x01:
            SetGenre(tr("Content$Tourism/Travel"));
            break;
        case 0x02:
            SetGenre(tr("Content$Handicraft"));
            break;
        case 0x03:
            SetGenre(tr("Content$Motoring"));
            break;
        case 0x04:
            SetGenre(tr("Content$Fitness & Health"));
            break;
        case 0x05:
            SetGenre(tr("Content$Cooking"));
            break;
        case 0x06:
            SetGenre(tr("Content$Advertisement/Shopping"));
            break;
        case 0x07:
            SetGenre(tr("Content$Gardening"));
            break;
        }
        break;

    case EVCONTENTMASK_SPECIAL:
        switch (i & 0x0F)
        {
        case 0x00:
            SetGenre(tr("Content$Original Language"));
            break;
        case 0x01:
            SetGenre(tr("Content$Black & White"));
            break;
        case 0x02:
            SetGenre(tr("Content$Unpublished"));
            break;
        case 0x03:
            SetGenre(tr("Content$Live Broadcast"));
            break;
        default:
            SetGenre(tr("Content$Special Characteristics"));
            break;
        }
        break;

    case EVCONTENTMASK_USERDEFINED:
        switch (i & 0x0F)
        {
        case 0x00:
            SetGenre(tr("Content$Drama")); // UK Freeview
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
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

    extepg=strcatrealloc(extepg,"\n");

    if (content)
    {
        SetCategoryByID(content);
        SetGenreByID(content);
    }

    if (category)
    {
        extepg=strcatrealloc(extepg,"Category: ");
        extepg=strcatrealloc(extepg,category);
        extepg=strcatrealloc(extepg,"\n");
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
    if (rating)
    {
        extepg=strcatrealloc(extepg,"Rating: ");
        extepg=strcatrealloc(extepg,rating);
        extepg=strcatrealloc(extepg,"\n");
    }


    if (announcement)
    {
        extepg=strcatrealloc(extepg,"Announcement: ");
        extepg=strcatrealloc(extepg,announcement);
        extepg=strcatrealloc(extepg,"\n");
    }
    if (addition)
    {
        extepg=strcatrealloc(extepg,"Addition: ");
        extepg=strcatrealloc(extepg,addition);
        extepg=strcatrealloc(extepg,"\n");
    }

    int len=strlen(extepg);
    if (len<=1)
    {
        free(extepg);
        extepg=NULL;
    }
    else
    {
        extepg[len-1]=0; // cut of last linefeed
    }
    return (const char*) extepg;
}

// --- cProcessInfosatepg
cProcessInfosatepg::cProcessInfosatepg()
        :cThread("infosatepg")
{
    mac=-1;
    global=NULL;
}

void cProcessInfosatepg::SetInfo(int Mac, cGlobalInfosatepg *Global)
{
    mac=Mac;
    global=Global;
}

void cProcessInfosatepg::Action()
{
    if ((mac<EPG_FIRST_DAY_MAC) || (mac>EPG_LAST_DAY_MAC)) return;

    FILE *f;
    const char *file = global->Infosatdata[mac].GetFile();
    f=fopen(file,"r");
    if (f)
    {
        time_t firststarttime=-1;
        if (ParseInfosatepg(f,&firststarttime))
        {
            if (firststarttime>time(NULL))
            {
                global->SetWakeupTime(firststarttime);
            }
            else
            {
                isyslog("infosatepg: wakeuptime from infosat is in the past");
                firststarttime=cTimer::SetTime(time(NULL),cTimer::TimeToInt(300));
                firststarttime+=79200; // add 20 hours
                global->SetWakeupTime(firststarttime);
            }
            global->Infosatdata[mac].Processed=true;
        }
        else
        {
            esyslog("infosatepg: failed to get writelock while parsing %s",file);
        }
        fclose(f);
    }
    else
    {
        if (access(file,R_OK)==-1)
        {
            // cannot open file -> receive it again
            esyslog("infosatepg: cannot access %s",file);
            global->Infosatdata[mac].ResetReceivedAll();
        }
    }
}

cEvent *cProcessInfosatepg::SearchEvent(cSchedule* Schedule, cInfosatevent *iEvent)
{
    cEvent *f=NULL;
    int maxdiff=INT_MAX;
    int eventTimeDiff=iEvent->Duration()/4;
    if (eventTimeDiff<600) eventTimeDiff=600;

    for (cEvent *p = Schedule->Events()->First(); p; p = Schedule->Events()->Next(p))
    {
        if (!strcmp(p->Title(),iEvent->Title()))
        {
            // found event with same title
            int diff=abs((int) difftime(p->StartTime(),iEvent->StartTime()));
            if (diff<=eventTimeDiff)
            {
                if (diff<=maxdiff)
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
    bool sAdd=false;
    if ((iEvent->Usage() & USE_APPEND)==USE_APPEND) sAdd=true;
    cSchedule* Schedule = (cSchedule *) Schedules->GetSchedule(channel,sAdd);
    if (!Schedule) return true; // No schedule -> do nothing (is this ok?)

    time_t start=0;
    cEvent *Event=NULL;
    const cEvent *lastEvent=Schedule->Events()->Last();

    if (iEvent->Duration()==0)
    {
        start=iEvent->StartTime();
        isyslog("infosatepg: event %s [%li (%s)] without duration", iEvent->Title(),start,ctime(&start));
    }

    if ((lastEvent) && (iEvent->StartTime()<lastEvent->EndTime()))
    {
        // try to find, 1st with StartTime
        Event = (cEvent *) Schedule->GetEvent(iEvent->EventID(),iEvent->StartTime());
        // 2nd with our own EventID
        if (!Event) Event = (cEvent *) Schedule->GetEvent(iEvent->EventID());
        // 3rd with StartTime +/- WaitTime
        if (!Event) Event= (cEvent *) SearchEvent(Schedule,iEvent);
        if (!Event)
        {
            start=iEvent->StartTime();
            dsyslog("infosatepg: failed to find event %s [%li (%s)]", iEvent->Title(),start,ctime(&start));
            global->Infosatdata[mac].Unlocated++;
            return true; // just bail out with ok
        }
        start=Event->StartTime();
        dsyslog("infosatepg: changing event %s [%li]", Event->Title(),start);

        // change existing event, prevent EIT EPG to update
        Event->SetTableID(0);
        Event->SetSeen(); // meaning of this?
    }
    else
    {
        // we are beyond the last event, just add (if we should)
        if ((iEvent->Usage() & USE_APPEND)!=USE_APPEND) return true;
        Event = new cEvent(iEvent->EventID());
        if (!Event) return true;
        Event->SetStartTime(iEvent->StartTime());
        Event->SetDuration(iEvent->Duration());
        Event->SetTitle(iEvent->Title());
        Event->SetVersion(0);
        start=iEvent->StartTime();
        dsyslog("infosatepg: adding new event %s (%lu) [%li]",iEvent->Title(),
                (u_long) iEvent->EventID(),start);
        Schedule->AddEvent(Event);
    }

    if ((iEvent->Usage() & USE_SHORTTEXT) == USE_SHORTTEXT)
    {
        if (!iEvent->ShortText())
        {
            // no short text
            if (iEvent->Original())
            {
                // use original if it exists
                iEvent->SetShortText(iEvent->Original());
            }
        }

        // if shorttext is the same as the title -> skip short text
        // this skips additional bug reporting in epg.c
        if (iEvent->ShortText() && iEvent->Title())
        {
            if (!strcmp(iEvent->ShortText(),iEvent->Title()))
            {
                iEvent->SetShortText(NULL);
            }
        }
        // if shorttext is the same as the description -> skip short text
        if (iEvent->ShortText() && Event->Description())
        {
            if (!strcmp(iEvent->ShortText(),Event->Description()))
            {
                iEvent->SetShortText(NULL);
            }
        }
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

bool cProcessInfosatepg::CheckOriginal_and_Episode(char **s,cInfosatevent *iEvent,cCharSetConv *conv)
{
    if (!strcmp(*s,"Blockbuster"))
    {
        iEvent->SetRating("Tipp");
        (*s)+=11;
        return false;
    }

    if (!strcmp(*s,"LIVE"))
    {
        iEvent->SetAnnouncement("LIVE");
        (*s)+=4;
        return false;
    }

    if (!strcmp(*s,"SAT.1-REIHE"))
    {
        // just ignore
        (*s)+=11;
        return false;
    }
    if (!strcmp(*s,"Sat.1 FilmFilm"))
    {
        // just ignore
        (*s)+=14;
        return false;
    }

    if (!strcmp(*s,"NIGHT ACTION"))
    {
        iEvent->SetAnnouncement("Action");
        (*s)+=12;
        return false;
    }
    if (!strcmp(*s,"FILME DER FILMEMACHER"))
    {
        iEvent->SetAnnouncement("Filme der Filmemacher");
        (*s)+=21;
        return false;
    }
    if (!strcmp(*s,"DIE BESTEN FILME ALLER ZEITEN"))
    {
        iEvent->SetRating("Tipp");
        (*s)+=29;
        return false;
    }

    if (!strncmp(*s,"Folge ",6))
    {
        (*s)+=6;
        char *episode=*s;
        // ok, overread Numbers and /
        while (isdigit(**s) | **s=='/') (*s)++;
        if (**s==0)
        {
            iEvent->SetEpisode(episode);
            return false;
        }
        (**s)=0;
        iEvent->SetEpisode(episode);
        (*s)++; // advance to subtitle
        if (**s=='-') (*s)++; // overread hyphen, if it exists
        while (**s==' ') (*s)++; // overread spaces
    }

    char *pOT,*pEOT;
    pOT=strchr(*s,'(');
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
    if (!strcmp(pOT,"Pilot"))
    {
        iEvent->SetAnnouncement("Pilot");
        return false;
    }
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
        iEvent->SetRating("Tipp");
    }
    else if ((strlen(s)>=9) && (!strncmp(s,"TAGESTIPP",9)))
    {
        iEvent->SetRating("Tipp");
    }
    else if ((strlen(s)>=10) && (!strncmp(s,"Tagestipp!",10)))
    {
        iEvent->SetRating("Tipp");
    }
    else if ((strlen(s)>=10) && (!strncmp(s,"Start der ",10)))
    {
        // just ignore this
    }
    else if ((strlen(s)>=15) && (!strncmp(s,"CARTOON NETWORK",15)))
    {
        // just ignore this
    }
    else if ((strlen(s)>=11) && (!strncmp(s,"SAT.1-SERIE",11)))
    {
        // just ignore this
    }

    else if ((strlen(s)>=16) && (!strncmp(s,"kabel eins Trick",16)))
    {
        // just ignore this
    }
    else if ((strlen(s)>=5) && (!strncmp(s,"JETIX",5)))
    {
        // just ignore this
    }
    else if ((strlen(s)>=11) && (!strncmp(s,"DISNEY TIME",11)))
    {
        // just ignore this
    }
    else ret=false;
    return ret;
}

bool cProcessInfosatepg::ParseInfosatepg(FILE *f,time_t *firststarttime)
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
// XXX don't default to assuming UTF-8 on FreeBSD (that's what the NULL does),
// its still often used without.
#if VDRVERSNUM < 10701 || defined(__FreeBSD__)
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
                int fsk;
                unsigned int content_descr;
                fields=sscanf(s,"%x %d",&content_descr,&fsk);
                if (fields==1) ievent->SetContentDescriptor(content_descr);
                if (fields==2)
                {
                    ievent->SetContentDescriptor(content_descr);
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
                            Skins.QueueMessage(mtInfo,
                                               tr("Infosat channellist available"));
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
                            dsyslog("infosatepg: start on %02i.%02i.%04i %02i:%02i (%s)",
                                    tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900,
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
#ifdef __FreeBSD__
            title = (char *)malloc(strlen(s));
            fields=0;
            if (title) fields=sscanf(s,"%d:%d %[^^]",&shour,&sminute,title);
#else
            fields=sscanf(s,"%d:%d %a[^^]",&shour,&sminute,&title);
#endif
            if (fields==3)
            {
                if (!ievent) ievent = new cInfosatevent;
                tm.tm_hour=shour;
                tm.tm_min=sminute;
                tm.tm_isdst=-1;
                time_t start=mktime(&tm);
                if ((oldstart!=(time_t) -1) && (difftime(start,oldstart)<0))
                {
                    // last ievent was yesterday
                    tm.tm_isdst=-1;
                    tm.tm_mday++;
                    start=mktime(&tm);
                }

                if (*firststarttime==(time_t) -1)
                {
                    *firststarttime=start+79200; // add 20 hours
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
                // check for special announcements
                pSA=strrchr(s,',');
                if (pSA)
                {
                    if (CheckAnnouncement(pSA+2,ievent))
                    {
                        // announcement was added to short description
                        *pSA=0;
                    }
                    CheckOriginal_and_Episode(&s,ievent,conv);
                    ievent->SetShortText(conv->Convert(s));
                }
                else
                {
                    if (!CheckAnnouncement(s,ievent))
                    {
                        CheckOriginal_and_Episode(&s,ievent,conv);
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
                // with additional info
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
                            if (year>1900) ievent->SetYear(year);
                            ievent->SetCountry(conv->Convert(pCY));
                        }
                        else
                        {
                            // just country
                            ievent->SetCountry(conv->Convert(pCY));
                        }
                    }
                }
                year=atoi(s);
                if (year>1900)
                {
                    ievent->SetYear(year);
                }
                else
                {
                    ievent->SetAddition(conv->Convert(s));
                }
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
            strreplace(s,-118,'\n'); // 0x8a
            ievent->SetDescription(conv->Convert(s));
            if (!AddInfosatEvent(chan,ievent)) abort=true;
            delete ievent;
            ievent=NULL;
            break;
        case 'Q':
            if (ignore) continue;
            if (!ievent) continue;
            if ((ievent->Duration()==0) && (ievent->StartTime()!=0))
            {
                int ehour,eminute;
                fields=sscanf(s,"%d:%d",&ehour,&eminute);
                if (fields==2)
                {
                    tm.tm_hour=ehour;
                    tm.tm_min=eminute;
                    tm.tm_isdst=-1;
                    time_t end=mktime(&tm);
                    if (difftime(end,ievent->StartTime())<0)
                    {
                        // last ievent was yesterday
                        tm.tm_isdst=-1;
                        tm.tm_mday++;
                        end=mktime(&tm);
                    }
                    double duration=difftime(end,ievent->StartTime());
                    if (duration>0)
                    {
                        ievent->SetDuration((int) (duration+60));
                    }
                }

            }
            if (!AddInfosatEvent(chan,ievent)) abort=true;
            delete ievent;
            ievent=NULL;
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
