/*
 * filter.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __filter_h_
#define __filter_h_

#include <netinet/ip.h>
#include <netinet/udp.h>
#include <vdr/filter.h>

#include "global.h"

struct infosathdr {
   u_short   technisatId;
   u_char    tableId;
   u_char    tablesubId;
   u_char    day;
   u_char    month;
   u_char    res6;
   u_char    res7;
   u_short   pktnr;
   u_short   pktcnt;
};

// --- cFilterInfosatepg
class cFilterInfosatepg : public cFilter {
private:
  cGlobalInfosatepg *global;
  u_long do_sum(u_long sum, u_char *buf, int nBytes);
  u_short foldsum(u_long sum);
  u_short IPChecksum(ip *ipHeader);
  u_short UDPChecksum(ip *ipHeader, udphdr *udpHeader);
protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
public:
  cFilterInfosatepg(cGlobalInfosatepg *Global);
  };

#endif
