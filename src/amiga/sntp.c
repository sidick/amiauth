/* sntp.c — AmigaOS SNTP transport (bsdsocket). The Amiga glue for
 * clock_sntp_sync: a single UDP exchange with an NTP server, feeding the
 * portable packet/offset helpers in clock.c. Linked only into the Amiga build.
 *
 * Note: the Amiga clock reads local time, but SNTP compares the server's UTC
 * against it, so the computed offset removes the whole local/DST difference and
 * yields true UTC — which is why SNTP is the reliable (DST-proof) time source. */
#ifdef __amigaos__

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/socket.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <string.h>
#include <time.h>

#include "clock.h"

#define NTP_PORT          123
#define SNTP_TIMEOUT_SECS 5

struct Library *SocketBase = NULL;

int clock_sntp_sync(clock_ctx *c, const char *server)
{
    uint8_t req[NTP_PACKET_SIZE], resp[NTP_PACKET_SIZE];
    struct sockaddr_in addr;
    struct timeval tv;
    uint64_t t1, t4, orig, t2, t3;
    unsigned long ip;
    int sock = -1;
    long rc = -1;

    if (!c || !server) return -1;

    SocketBase = OpenLibrary((STRPTR)"bsdsocket.library", 4);
    if (!SocketBase) return -1;              /* no TCP/IP stack -> offline */

    do {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(NTP_PORT);

        /* Accept a dotted-quad directly; otherwise resolve the hostname. */
        ip = inet_addr((STRPTR)server);
        if (ip != (unsigned long)-1) {
            addr.sin_addr.s_addr = ip;
        } else {
            struct hostent *he = gethostbyname((STRPTR)server);
            if (!he || !he->h_addr_list[0]) break;
            memcpy(&addr.sin_addr, he->h_addr_list[0], (size_t)he->h_length);
        }

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) break;

        tv.tv_sec  = SNTP_TIMEOUT_SECS;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

        t1 = (uint64_t)time(NULL);           /* local send time */
        clock_ntp_build_request(req, t1);
        if (sendto(sock, (char *)req, NTP_PACKET_SIZE, 0,
                   (struct sockaddr *)&addr, sizeof(addr)) != NTP_PACKET_SIZE)
            break;

        if (recvfrom(sock, (char *)resp, NTP_PACKET_SIZE, 0, NULL, NULL)
                != NTP_PACKET_SIZE)
            break;
        t4 = (uint64_t)time(NULL);           /* local receive time */

        if (clock_ntp_parse_response(resp, &orig, &t2, &t3) != 0) break;
        if (orig != t1) break;               /* must echo our request (anti-spoof) */

        clock_apply_offset(c, t1, t2, t3, t4);
        rc = 0;
    } while (0);

    if (sock >= 0) CloseSocket(sock);
    CloseLibrary(SocketBase);
    SocketBase = NULL;
    return (int)rc;
}

#endif /* __amigaos__ */
