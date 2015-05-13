/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifndef _INC_WS2IPDEF
#define _INC_WS2IPDEF

#include <in6addr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ipv6_mreq {
  struct in6_addr ipv6mr_multiaddr;
  unsigned int ipv6mr_interface;
} IPV6_MREQ;

struct sockaddr_in6_old {
  short sin6_family;
  u_short sin6_port;
  u_long sin6_flowinfo;
  struct in6_addr sin6_addr;
};

typedef union sockaddr_gen {
  struct sockaddr Address;
  struct sockaddr_in AddressIn;
  struct sockaddr_in6_old AddressIn6;
} sockaddr_gen;

struct sockaddr_in6 {
  short sin6_family;
  u_short sin6_port;
  u_long sin6_flowinfo;
  struct in6_addr sin6_addr;
  __C89_NAMELESS union {
    u_long sin6_scope_id;
    SCOPE_ID sin6_scope_struct;
  };
};

typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct sockaddr_in6 *PSOCKADDR_IN6;
typedef struct sockaddr_in6 *LPSOCKADDR_IN6;

typedef struct _INTERFACE_INFO {
  u_long iiFlags;
  sockaddr_gen iiAddress;
  sockaddr_gen iiBroadcastAddress;
  sockaddr_gen iiNetmask;
} INTERFACE_INFO,*LPINTERFACE_INFO;

typedef enum _MULTICAST_MODE_TYPE {
  MCAST_INCLUDE   = 0,
  MCAST_EXCLUDE
} MULTICAST_MODE_TYPE;

typedef struct _sockaddr_in6_pair {
  PSOCKADDR_IN6 SourceAddress;
  PSOCKADDR_IN6 DestinationAddress;
} SOCKADDR_IN6_PAIR, *PSOCKADDR_IN6_PAIR;

typedef union _SOCKADDR_INET {
  SOCKADDR_IN    Ipv4;
  SOCKADDR_IN6   Ipv6;
  ADDRESS_FAMILY si_family;
} SOCKADDR_INET, *PSOCKADDR_INET;

typedef struct group_filter {
  ULONG               gf_interface;
  SOCKADDR_STORAGE    gf_group;
  MULTICAST_MODE_TYPE gf_fmode;
  ULONG               gf_numsrc;
  SOCKADDR_STORAGE    gf_slist[1];
} GROUP_FILTER, *PGROUP_FILTER;

typedef struct group_req {
  ULONG            gr_interface;
  SOCKADDR_STORAGE gr_group;
} GROUP_REQ, *PGROUP_REQ;

typedef struct group_source_req {
  ULONG            gsr_interface;
  SOCKADDR_STORAGE gsr_group;
  SOCKADDR_STORAGE gsr_source;
} GROUP_SOURCE_REQ, *PGROUP_SOURCE_REQ;

#ifdef __cplusplus
}
#endif

#endif /*_INC_WS2IPDEF*/
