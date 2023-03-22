/*****************************************************************************
 * error.c: DOS and Winsock error messages formatting
 *****************************************************************************
 * Copyright © 2006-2013 Rémi Denis-Courmont
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <winerror.h>

#include <vlc_common.h>

#ifndef WSA_QOS_EUNKNOWNPSOBJ
# define WSA_QOS_EUNKNOWNPSOBJ 11024L
#endif

typedef struct
{
    int code;
    const char *msg;
} wsaerrmsg_t;

static const wsaerrmsg_t wsaerrmsg[] =
{
    { WSAEINTR, "Interrupted function call" },
    { WSAEBADF, "File handle is not valid" },
    { WSAEACCES, "Access denied" },
    { WSAEFAULT, "Invalid memory address" },
    { WSAEINVAL, "Invalid argument" },
    { WSAEMFILE, "Too many open sockets" },
    { WSAEWOULDBLOCK, "Resource temporarily unavailable" },
    { WSAEINPROGRESS, "Operation now in progress" },
    { WSAEALREADY, "Operation already in progress" },
    { WSAENOTSOCK, "Non-socket handle specified" },
    { WSAEDESTADDRREQ, "Missing destination address" },
    { WSAEMSGSIZE, "Message too long" },
    { WSAEPROTOTYPE, "Protocol wrong type for socket", },
    { WSAENOPROTOOPT, "Option not supported by protocol" },
    { WSAEPROTONOSUPPORT, "Protocol not supported" },
    { WSAESOCKTNOSUPPORT, "Socket type not supported" },
    { WSAEOPNOTSUPP, "Operation not supported" },
    { WSAEPFNOSUPPORT, "Protocol family not supported" },
    { WSAEAFNOSUPPORT, "Address family not supported by protocol family" },
    { WSAEADDRINUSE, "Address already in use" },
    { WSAEADDRNOTAVAIL, "Cannot assign requested address" },
    { WSAENETDOWN, "Network is down" },
    { WSAENETUNREACH, "Network unreachable" },
    { WSAENETRESET, "Network dropped connection on reset" },
    { WSAECONNABORTED, "Software caused connection abort" },
    { WSAECONNRESET, "Connection reset by peer" },
    { WSAENOBUFS, "No buffer space available (not enough memory)" },
    { WSAEISCONN, "Socket is already connected" },
    { WSAENOTCONN, "Socket is not connected" },
    { WSAESHUTDOWN, "Cannot send after socket shutdown" },
    { WSAETOOMANYREFS, "Too many references" },
    { WSAETIMEDOUT, "Connection timed out" },
    { WSAECONNREFUSED, "Connection refused by peer" },
    { WSAELOOP, "Cannot translate name" },
    { WSAENAMETOOLONG, "Name too long" },
    { WSAEHOSTDOWN, "Remote host is down" },
    { WSAEHOSTUNREACH, "No route to host (unreachable)" },
    { WSAENOTEMPTY, "Directory not empty" },
    { WSAEPROCLIM, "Too many processes" },
    { WSAEUSERS, "User quota exceeded" },
    { WSAEDQUOT, "Disk quota exceeded" },
    { WSAESTALE, "Stale file handle reference" },
    { WSAEREMOTE, "Item is remote", },
    { WSASYSNOTREADY, "Network subsystem is unavailable (network stack not ready)" },
    { WSAVERNOTSUPPORTED, "Winsock.dll version out of range (network stack version not supported" },
    { WSANOTINITIALISED, "Network not initialized" },
    { WSAEDISCON, "Graceful shutdown in progress" },
    { WSAENOMORE, "No more results" },
    { WSAECANCELLED, "Call has been cancelled" },
    { WSAEINVALIDPROCTABLE, "Procedure call table is invalid" },
    { WSAEINVALIDPROVIDER, "Service provider is invalid" },
    { WSAEPROVIDERFAILEDINIT, "Service provider failed to initialize" },
    { WSASYSCALLFAILURE, "System call failure" },
    { WSASERVICE_NOT_FOUND, "Service not found" },
    { WSATYPE_NOT_FOUND, "Class type not found" },
    { WSA_E_NO_MORE, "No more results" },
    { WSA_E_CANCELLED, "Call was cancelled" },
    { WSAEREFUSED, "Database query was refused" },
    { WSAHOST_NOT_FOUND, "Host not found" },
    { WSATRY_AGAIN, "Nonauthoritative host not found (temporary hostname error)" },
    { WSANO_RECOVERY, "Non-recoverable hostname error" },
    { WSANO_DATA, "Valid name, no data record of requested type" },
    { WSA_QOS_RECEIVERS, "QOS receivers" },
    { WSA_QOS_SENDERS, "QOS senders" },
    { WSA_QOS_NO_SENDERS, "No QOS senders" },
    { WSA_QOS_NO_RECEIVERS, "QOS no receivers" },
    { WSA_QOS_REQUEST_CONFIRMED, "QOS request confirmed" },
    { WSA_QOS_ADMISSION_FAILURE, "QOS admission error" },
    { WSA_QOS_POLICY_FAILURE, "QOS policy failure" },
    { WSA_QOS_BAD_STYLE, "QOS bad style" },
    { WSA_QOS_BAD_OBJECT, "QOS bad object" },
    { WSA_QOS_TRAFFIC_CTRL_ERROR, "QOS traffic control error" },
    { WSA_QOS_GENERIC_ERROR, "QOS generic error" },
    { WSA_QOS_ESERVICETYPE, "QOS service type error" },
    { WSA_QOS_EFLOWSPEC, "QOS flowspec error" },
    { WSA_QOS_EPROVSPECBUF, "Invalid QOS provider buffer" },
    { WSA_QOS_EFILTERSTYLE, "Invalid QOS filter style" },
    { WSA_QOS_EFILTERTYPE, "Invalid QOS filter type" },
    { WSA_QOS_EFILTERCOUNT, "Incorrect QOS filter count" },
    { WSA_QOS_EOBJLENGTH, "Invalid QOS object length" },
    { WSA_QOS_EFLOWCOUNT, "Incorrect QOS flow count" },
    { WSA_QOS_EUNKNOWNPSOBJ, "Unrecognized QOS object" },
    { WSA_QOS_EPOLICYOBJ, "Invalid QOS policy object" },
    { WSA_QOS_EFLOWDESC, "Invalid QOS flow descriptor" },
    { WSA_QOS_EPSFLOWSPEC, "Invalid QOS provider-specific flowspec" },
    { WSA_QOS_EPSFILTERSPEC, "Invalid QOS provider-specific filterspec" },
    { WSA_QOS_ESDMODEOBJ, "Invalid QOS shape discard mode object" },
    { WSA_QOS_ESHAPERATEOBJ, "Invalid QOS shaping rate object" },
    { WSA_QOS_RESERVED_PETYPE, "Reserved policy QOS element type" },
    { 0, NULL }
    /* Winsock2 error codes are missing, they "never" occur */
};

const char *vlc_strerror_c(int errnum)
{
    /* C run-time errors */
    if ((unsigned)errnum < (unsigned)_sys_nerr)
        return _sys_errlist[errnum];

    /* Windows socket errors */
    for (const wsaerrmsg_t *e = wsaerrmsg; e->msg != NULL; e++)
        if (e->code == errnum)
            return e->msg;

    return "Unknown error";
}

const char *vlc_strerror(int errnum)
{
    return /*vlc_gettext*/(vlc_strerror_c(errnum));
}
