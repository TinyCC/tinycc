/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _MINGW_IP_TYPES_H
#define _MINGW_IP_TYPES_H

#include <_bsd_types.h>

#define h_addr h_addr_list[0]

struct hostent {
	char	*h_name;
	char	**h_aliases;
	short	h_addrtype;
	short	h_length;
	char	**h_addr_list;
};

struct netent {
	char	*n_name;
	char	**n_aliases;
	short	n_addrtype;
	u_long	n_net;
};

struct servent {
	char	*s_name;
	char	**s_aliases;
#ifdef _WIN64
	char	*s_proto;
	short	s_port;
#else
	short	s_port;
	char	*s_proto;
#endif
};

struct protoent {
	char	*p_name;
	char	**p_aliases;
	short	p_proto;
};

typedef struct hostent		HOSTENT;
typedef struct hostent		*PHOSTENT;
typedef struct hostent		*LPHOSTENT;

typedef struct servent		SERVENT;
typedef struct servent		*PSERVENT;
typedef struct servent		*LPSERVENT;

typedef struct protoent		PROTOENT;
typedef struct protoent		*PPROTOENT;
typedef struct protoent		*LPPROTOENT;


#include <inaddr.h>

struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct in_addr	sin_addr;
	char	sin_zero[8];
};

struct sockaddr {
	u_short	sa_family;
	char	sa_data[14];
};

struct sockproto {
	u_short	sp_family;
	u_short	sp_protocol;
};

struct linger {
	u_short	l_onoff;
	u_short	l_linger;
};

typedef struct sockaddr		SOCKADDR;
typedef struct sockaddr		*PSOCKADDR;
typedef struct sockaddr		*LPSOCKADDR;

typedef struct sockaddr_in	SOCKADDR_IN;
typedef struct sockaddr_in	*PSOCKADDR_IN;
typedef struct sockaddr_in	*LPSOCKADDR_IN;

typedef struct linger		LINGER;
typedef struct linger		*PLINGER;
typedef struct linger		*LPLINGER;


#include <_timeval.h>

typedef struct timeval		TIMEVAL;
typedef struct timeval		*PTIMEVAL;
typedef struct timeval		*LPTIMEVAL;

#endif	/* _MINGW_IP_TYPES_H */

