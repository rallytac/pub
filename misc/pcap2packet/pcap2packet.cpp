// © 2020 Erik Rigtorp <erik@rigtorp.se>
// SPDX-License-Identifier: MIT
//
// Updates by Rally Tactical Systems:
//      - Added support for destination port and address

#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pcap/pcap.h>
#include <unistd.h>

#ifdef __MACH__
static void timespec_monodiff_rml(struct timespec *ts_out, const struct timespec *ts_in)
{
    static const long TIMING_GIGA = 1000000000;

    ts_out->tv_sec = ts_in->tv_sec - ts_out->tv_sec;
    ts_out->tv_nsec = ts_in->tv_nsec - ts_out->tv_nsec;
    if (ts_out->tv_sec < 0)
    {
        ts_out->tv_sec = 0;
        ts_out->tv_nsec = 0;
    }
    else if (ts_out->tv_nsec < 0)
    {
        if (ts_out->tv_sec == 0)
        {
            ts_out->tv_sec = 0;
            ts_out->tv_nsec = 0;
        }
        else
        {
            ts_out->tv_sec = ts_out->tv_sec - 1;
            ts_out->tv_nsec = ts_out->tv_nsec + TIMING_GIGA;
        }
    }
}

static int clock_nanosleep_abstime ( const struct timespec *req)
{
    struct timespec ts_delta;
    int retval = clock_gettime(CLOCK_MONOTONIC, &ts_delta);

    if (retval == 0)
    {
        timespec_monodiff_rml ( &ts_delta, req );
        retval = nanosleep ( &ts_delta, NULL );
    }

    return retval;
}
#endif

#define NANOSECONDS_PER_SECOND 1000000000L

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, ":")) != -1)
    {
        switch (opt)
        {
            default:
                goto usage;
        }
    }

    if (optind >= argc)
    {
    usage:
        std::cerr
                << "pcap2packet 1.0.0 © 2024 Rally Tactical Systems, Inc."
                "usage: pcap2packet pcap_file packet_file\n"
                << std::endl;
        
        return 1;
    }

    timespec deadline = {};
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1)
    {
        std::cerr << "clock_gettime: " << strerror(errno) << std::endl;
        return 1;
    }

    for (int i = 0; repeat == -1 || i < repeat; i++)
    {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *handle = pcap_open_offline_with_tstamp_precision(argv[optind], PCAP_TSTAMP_PRECISION_NANO, errbuf);

        if (handle == nullptr)
        {
            std::cerr << "pcap_open: " << errbuf << std::endl;
            return 1;
        }

        pcap_pkthdr header;
        const u_char *p;

        if (header.len != header.caplen)
        {
            std::cout << "h" << std::flush;
            continue;
        }

        auto eth = reinterpret_cast<const ether_header *>(p);

        // jump over and ignore vlan tags
        while (ntohs(eth->ether_type) == ETHERTYPE_VLAN)
        {
            p += 4;
            eth = reinterpret_cast<const ether_header *>(p);
        }

        if (ntohs(eth->ether_type) != ETHERTYPE_IP)
        {
            std::cout << "e" << std::flush;
            continue;
        }

        auto ip = reinterpret_cast<const struct ip *>(p + sizeof(ether_header));
        if (ip->ip_v != 4)
        {
            std::cout << "i" << std::flush;
            continue;
        }
        if (ip->ip_p != IPPROTO_UDP)
        {
            std::cout << "u" << std::flush;
            continue;
        }

            auto udp = reinterpret_cast<const udphdr *>(p + sizeof(ether_header) + ip->ip_hl * 4);

            #ifdef __GLIBC__
                ssize_t len = ntohs(udp->len) - 8;
            #else
                ssize_t len = ntohs(udp->uh_ulen) - 8;
            #endif

            if(srcPortFilter > 0)
            {
                if(udp->uh_sport != htons(srcPortFilter))
                {
                    pktsFiltered++;
                    bytesFiltered += len;
                    std::cout << "s" << std::flush;
                    continue;
                }
            }

            if(dstPortFilter > 0)
            {
                if(udp->uh_dport != htons(dstPortFilter))
                {
                    pktsFiltered++;
                    bytesFiltered += len;
                    std::cout << "d" << std::flush;
                    continue;
                }
            }

            if (interval != -1)
            {
                // Use constant packet rate
                deadline.tv_sec += interval / 1000L;
                deadline.tv_nsec += (interval * 1000000L) % NANOSECONDS_PER_SECOND;
            }
            else
            {
                // Next packet deadline = start + (packet ts - first packet ts) * speed
                int64_t delta = (header.ts.tv_sec - pcap_start.tv_sec) * NANOSECONDS_PER_SECOND +
                (header.ts.tv_usec - pcap_start.tv_nsec); // Note PCAP_TSTAMP_PRECISION_NANO
                if (speed != 1.0)
                {
                    delta *= speed;
                }

                deadline = start;
                deadline.tv_sec += delta / NANOSECONDS_PER_SECOND;
                deadline.tv_nsec += delta % NANOSECONDS_PER_SECOND;
            }

            // Normalize timespec
            if (deadline.tv_nsec > NANOSECONDS_PER_SECOND)
            {
                deadline.tv_sec++;
                deadline.tv_nsec -= NANOSECONDS_PER_SECOND;
            }

            timespec now = {};
            if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
            {
                std::cerr << "clock_gettime: " << strerror(errno) << std::endl;
                return 1;
            }

            if (deadline.tv_sec > now.tv_sec || (deadline.tv_sec == now.tv_sec && deadline.tv_nsec > now.tv_nsec))
            {
                if(
                    #ifdef __MACH__
                        clock_nanosleep_abstime(&deadline)
                    #else
                        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr)
                    #endif
                    == -1
                   )
                {
                    std::cerr << "clock_nanosleep: " << strerror(errno) << std::endl;
                    return 1;
                }
            }

            const u_char *d = &p[sizeof(ether_header) + ip->ip_hl * 4 + sizeof(udphdr)];

            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            #ifdef __GLIBC__
                if(dstPort != 0)
                {
                    addr.sin_port = ntohs(dstPort);
                }
                else
                {
                    addr.sin_port = udp->dest;    
                }
            #else
                if(dstPort != 0)
                {
                    addr.sin_port = ntohs(dstPort);
                }
                else
                {
                    addr.sin_port = udp->uh_dport;
                }
            #endif
            addr.sin_addr = {ip->ip_dst};
            if( dstAddr != nullptr )
            {
                inet_pton(AF_INET, dstAddr, &addr.sin_addr.s_addr);
            }

            auto n = sendto(fd, d, len, 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            if (n != len)
            {
                std::cerr << "sendto: " << strerror(errno) << std::endl;
                return 1;
            }

            //std::cout << "send " << std::endl;
            std::cout << "." << std::flush;
            
            pktsSent++;
            bytesSent += len;
        }

        pcap_close(handle);
    
    std::cout << std::endl;
    std::cout << "sent " << pktsSent << " packets (" << bytesSent << " bytes), "
              << "filtered " << pktsFiltered << " packets (" << bytesFiltered << " bytes)"
              << std::endl;

    return 0;
}
