/*
 * filter.cpp: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <netinet/if_ether.h>
#include <unistd.h>

#include "filter.h"

#undef WRITE_RAW

#if __GNUC__ > 3
#define UNUSED(v) UNUSED_ ## v __attribute__((unused))
#else
#define UNUSED(x) x
#endif

cFilterInfosatepg::cFilterInfosatepg(cGlobalInfosatepg *Global)
{
    global = Global;
    Set(global->Pid,0,0);
}

u_long cFilterInfosatepg::do_sum(u_long sum, u_char *buf, int nBytes)
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

u_short cFilterInfosatepg::foldsum(u_long sum)
{
    while (sum>>16)
        sum = (sum >> 16) + (sum & 0xFFFF);

    return ((u_short) ~sum);
}

u_short cFilterInfosatepg::IPChecksum(iphdr *ipHeader)
{
    return foldsum(do_sum(0, (u_char*) ipHeader, sizeof(iphdr)));

}   /* IpChecksum() */


u_short cFilterInfosatepg::UDPChecksum(iphdr *ipHeader, udphdr *udpHeader)
{
    u_long sum = 0;

    // Ip-Pseudo-Header
    sum = do_sum(sum, (u_char*)(&ipHeader->saddr), sizeof(ipHeader->saddr));
    sum = do_sum(sum, (u_char*)(&ipHeader->daddr), sizeof(ipHeader->daddr));
    sum += udpHeader->len;
    sum += ipHeader->protocol<<8;

    sum = do_sum(sum, (u_char*)udpHeader, ntohs(udpHeader->len));

    return foldsum(sum);
}

void cFilterInfosatepg::Process(u_short UNUSED(Pid), u_char UNUSED(Tid), const u_char *Data, int Length)
{
#define SECT_IP_HDR_START  12
#define SECT_UDP_HDR_START 32
#define SECT_IS_HDR_START  40
#define SECT_IS_DATA_START 52

    if (Data[0]!=0x3E) return;

    struct ethhdr eth_hdr;
    memset(&eth_hdr,0,sizeof(struct ethhdr));

    eth_hdr.h_dest[0]=Data[11];
    eth_hdr.h_dest[1]=Data[10];
    eth_hdr.h_dest[2]=Data[9];
    eth_hdr.h_dest[3]=Data[8];
    eth_hdr.h_dest[4]=Data[4];
    eth_hdr.h_dest[5]=Data[3];

    // check mac and range
    if (!global->CheckMAC(&eth_hdr)) return;

    int mac = eth_hdr.h_dest[5];
    global->ActualMac=mac;

    struct iphdr *ip_hdr = (iphdr *) &Data[SECT_IP_HDR_START];
    struct udphdr *udp_hdr = (udphdr *) &Data[SECT_UDP_HDR_START];

    // Only IPv4
    if (ip_hdr->version!=4) return;

    // Check IP checksum
    if (IPChecksum(ip_hdr)!=0)
    {
        esyslog("infosatepg: ip checksum error");
        return;
    }

    // Only UDP
    if (ip_hdr->protocol!=17) return;

    // Check UDP checksum
    if (UDPChecksum(ip_hdr,udp_hdr)!=0)
    {
        esyslog("infosatepg: udp checksum error");
        return;
    }

    struct infosathdr *ishdr = (struct infosathdr*) &Data[SECT_IS_HDR_START];

    if (ntohs(ishdr->technisatId)!=0x0001) return;

    int pktnr = ntohs(ishdr->pktnr);
    int pktcnt = ntohs(ishdr->pktcnt);

    const u_char *infosatdata = &Data[SECT_IS_DATA_START];
    int len = Length - SECT_IS_DATA_START-4;

    char file[1024];
    snprintf(file,sizeof(file),"%s/infosatepg%02i%02i_%03i.dat",global->Directory(),ishdr->day,ishdr->month,
             pktcnt);

    if (global->Infosatdata[mac].NeverSeen(ishdr->day,ishdr->month,pktcnt))
    {
        // never seen such a packet -> init structure
        global->Infosatdata[mac].Init(file,ishdr->day,ishdr->month,pktcnt);
    }

    // Check if we missed a packet
    global->Infosatdata[mac].CheckMissed(pktnr);

    // Check if we already have this packet
    if (global->Infosatdata[mac].GetBit(pktnr))
    {
        global->Infosatdata[mac].SetLastPkt(pktnr);
        return;
    }


#ifdef VDRDEBUG
    dsyslog("infosatepg: mac=%02x-%02x-%02x-%02x-%02x-%02x",eth_hdr.h_dest[0],eth_hdr.h_dest[1],
            eth_hdr.h_dest[2],eth_hdr.h_dest[3],eth_hdr.h_dest[4],eth_hdr.h_dest[5] );

    dsyslog("infosatepg: tid=%04i tbl=%04i stbl=%04i day=%02i month=%02i pktnr=%03i pktcnt=%03i len=%i",
            ntohs(ishdr->technisatId),ishdr->tableId,ishdr->tablesubId,ishdr->day,
            ishdr->month, pktnr, pktcnt, len);

    dsyslog("infosatepg: save to %s", file);
#endif

    int f=open(file,O_RDWR|O_CREAT,0664);
    if (f==-1)
    {
        if (errno!=ENOSPC)
        {
            esyslog("infosatepg: unable to create file '%s'", file);
        }
        return;
    }
    off_t offset = (off_t) (pktnr*1400);
    if (lseek(f,offset,SEEK_SET)!=(off_t) -1)
    {
#ifdef VDRDEBUG
        dsyslog("infosatepg: writing to %li",offset);
#endif
        if (write(f,infosatdata,len)==len)
        {
            // set bit in Infosatdata bitfield
            global->Infosatdata[mac].SetBit(pktnr,true);
        }
    }
    close(f);

#ifdef WRITE_RAW
    sprintf(file,"%s/%03i.dat",dir,pktnr);
    f=open(file,O_RDWR|O_CREAT,0664);
    if (f==-1) return;
    write(f,Data,Length);
    close(f);
#endif

    // check if we have all packets
    if (global->Infosatdata[mac].CheckReceivedAll())
    {
        // we have all packets
        isyslog("infosatepg: day=%02i month=%02i fully received", ishdr->day,ishdr->month);
    }

}
