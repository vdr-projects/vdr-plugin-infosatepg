/*
 * process.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __process_h_
#define __process_h_

#include "global.h"

// --- cInfosatevent
class cInfosatevent
{
#define EVCONTENTMASK_MOVIEDRAMA               0x10
#define EVCONTENTMASK_NEWSCURRENTAFFAIRS       0x20
#define EVCONTENTMASK_SHOW                     0x30
#define EVCONTENTMASK_SPORTS                   0x40
#define EVCONTENTMASK_CHILDRENYOUTH            0x50
#define EVCONTENTMASK_MUSICBALLETDANCE         0x60
#define EVCONTENTMASK_ARTSCULTURE              0x70
#define EVCONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
#define EVCONTENTMASK_EDUCATIONALSCIENCE       0x90
#define EVCONTENTMASK_LEISUREHOBBIES           0xA0
#define EVCONTENTMASK_SPECIAL                  0xB0
#define EVCONTENTMASK_USERDEFINED              0xF0

private:
  int duration;
  time_t startTime;
  char *title;
  char *shorttext;
  char *description;
  char *announcement;
  char *country;
  char *genre;
  char *original;
  char *episode;
  char *category;
  char *extepg;
  char *addition;
  char *rating;
  int content;
  int fsk;
  int year;
  int usage;
  int days;
  tEventID eventID;
public:
  cInfosatevent();
  ~cInfosatevent();
  void SetTitle(const char *Title);
  void SetShortText(const char *ShortText);
  void SetDescription(const char *Description);
  void SetStartTime(time_t StartTime) { startTime=StartTime; }
  void SetDuration(int Duration) { duration=Duration; }
  void SetEventUsage(int Usage) { usage=Usage; }
  void SetEventDays(int Days) { days=Days; }
  void SetYear(int Year) { year=Year; }
  void SetEventID(tEventID EventID) { eventID=EventID; }
  void SetContentDescriptor(int Content) { content=Content; }
  void SetFSK(int FSK) { fsk=FSK; }
  void SetRating(const char *Rating);
  void SetAnnouncement(const char *Announcement);
  void SetCountry(const char *Country);
  void SetCategory(const char *Category);
  void SetCategoryByID(int i);
  void SetGenre(const char *Genre);
  void SetGenreByID(int i);
  void SetOriginal(const char *Original);
  void SetEpisode(const char *Episode);
  void SetAddition(const char *Addition);
  const char *Description(void) const { return description; }
  const char *Title(void) const { return title; }
  const char *ShortText(void) const { return shorttext; }
  const char *Announcement(void) const { return announcement; }
  const char *Category(void) const { return category; }
  const char *Genre(void) const { return genre; }
  const char *Country(void) const { return country; }
  const char *Original(void) const { return original; }
  const char *Episode(void) const { return episode; }
  const char *Addition(void) const { return addition; }
  const char *Rating(void) const { return rating; }
  int Content(void) const { return content; }
  int Year(void) const { return year; }
  int Duration(void) const { return duration; }
  int FSK(void) const { return fsk; }
  time_t StartTime(void) const { return startTime; }
  int Usage() { return usage; }
  int Days() { return days; }
  tEventID EventID(void) const { return eventID; }
  const char *ExtEPG(void);
};

// --- cProcessInfosatepg
class cProcessInfosatepg //: public cThread
{
private:
  cGlobalInfosatepg *global;
  bool AddInfosatEvent(cChannel *channel, cInfosatevent *iEvent);
  bool CheckOriginal_and_Episode(char **s,cInfosatevent *iEvent,cCharSetConv *conv);
  bool CheckAnnouncement(char *s,cInfosatevent *iEvent);
  bool ParseInfosatepg(FILE *f,int *firststarttime);
  cChannel *GetVDRChannel(int frequency, int sid);
  u_long DoSum(u_long sum, const char *buf, int nBytes);
  cEvent *SearchEvent(cSchedule* Schedule, cInfosatevent *iEvent);
public:
  //virtual void Action(); //(int Mac, cGlobalInfosatepg *Global);
  cProcessInfosatepg(int Mac, cGlobalInfosatepg *Global);
};

#endif
