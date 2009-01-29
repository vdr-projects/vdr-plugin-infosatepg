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
private:
  int duration;
  time_t startTime;
  char *title;
  char *shortText;
  char *description;
  char *announcement;
  char *country;
  char *genre;
  char *original;
  int category;
  int fsk;
  int year;
  int usage;
  int days;
  char *extepg;
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
  void SetCategory(int Category) { category=Category; }
  void SetFSK(int FSK) { fsk=FSK; }
  void SetAnnouncement(const char *Announcement);
  void SetCountry(const char *Country);
  void SetGenre(const char *Genre);
  void SetOriginal(const char *Original);
  const char *Description(void) const { return description; }
  const char *Title(void) const { return title; }
  const char *ShortText(void) const { return shortText; }
  const char *Announcement(void) const { return announcement; }
  const char *Genre(void) const { return genre; }
  const char *Country(void) const { return country; }
  const char *Original(void) const { return original; }
  int Year(void) const { return year; }
  int Duration(void) const { return duration; }
  int FSK(void) const { return fsk; }
  int Category(void) const { return category; }
  time_t StartTime(void) const { return startTime; }
  int Usage() { return usage; }
  int Days() { return days; }
  tEventID EventID(void) const { return eventID; }
  const char *ExtEPG(void);
};

// --- cProcessInfosatepg
class cProcessInfosatepg
{
private:
  cGlobalInfosatepg *global;
  bool AddInfosatEvent(cChannel *channel, cInfosatevent *iEvent);
  bool CheckOriginal(char *s,cInfosatevent *iEvent,cCharSetConv *conv);
  bool CheckAnnouncement(char *s,cInfosatevent *iEvent);
  bool ParseInfosatepg(FILE *f,int *firststarttime);
  cChannel *GetInfosatChannel(int frequency, int sid);
  u_long DoSum(u_long sum, const char *buf, int nBytes);
  cEvent *SearchEvent(cSchedule* Schedule, cInfosatevent *iEvent);
public:
  cProcessInfosatepg(int Mac, cGlobalInfosatepg *Global);
};

#endif
