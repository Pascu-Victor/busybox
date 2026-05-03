/* Minimal rtnetlink-shaped ABI used by the WOS BusyBox ip backend. */
#ifndef WOS_IPROUTE_COMPAT_H
#define WOS_IPROUTE_COMPAT_H 1

#include <stdint.h>
#include <sys/socket.h>

typedef uint8_t __u8;
typedef uint32_t __u32;

#ifndef NETLINK_ROUTE
# define NETLINK_ROUTE 0
#endif

struct sockaddr_nl {
	sa_family_t nl_family;
	unsigned short nl_pad;
	uint32_t nl_pid;
	uint32_t nl_groups;
};

struct nlmsghdr {
	uint32_t nlmsg_len;
	uint16_t nlmsg_type;
	uint16_t nlmsg_flags;
	uint32_t nlmsg_seq;
	uint32_t nlmsg_pid;
};

struct nlmsgerr {
	int error;
	struct nlmsghdr msg;
};

struct rtattr {
	unsigned short rta_len;
	unsigned short rta_type;
};

struct rtgenmsg {
	unsigned char rtgen_family;
};

struct ifinfomsg {
	unsigned char ifi_family;
	unsigned char __ifi_pad;
	unsigned short ifi_type;
	int ifi_index;
	unsigned ifi_flags;
	unsigned ifi_change;
};

struct ifaddrmsg {
	unsigned char ifa_family;
	unsigned char ifa_prefixlen;
	unsigned char ifa_flags;
	unsigned char ifa_scope;
	int ifa_index;
};

struct ifa_cacheinfo {
	uint32_t ifa_prefered;
	uint32_t ifa_valid;
	uint32_t cstamp;
	uint32_t tstamp;
};

#define NLMSG_ALIGNTO 4
#define NLMSG_ALIGN(len) (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1))
#define NLMSG_HDRLEN ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len) + NLMSG_HDRLEN)
#define NLMSG_SPACE(len) NLMSG_ALIGN(NLMSG_LENGTH(len))
#define NLMSG_DATA(nlh) ((void *)(((char *)(nlh)) + NLMSG_LENGTH(0)))
#define NLMSG_NEXT(nlh, len) ((len) -= NLMSG_ALIGN((nlh)->nlmsg_len), (struct nlmsghdr *)(((char *)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#define NLMSG_OK(nlh, len) ((len) >= (int)sizeof(struct nlmsghdr) && (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && (int)(nlh)->nlmsg_len <= (len))

#define NLM_F_REQUEST 0x01
#define NLM_F_MULTI 0x02
#define NLM_F_ACK 0x04
#define NLM_F_ROOT 0x100
#define NLM_F_MATCH 0x200
#define NLM_F_REPLACE 0x100
#define NLM_F_EXCL 0x200
#define NLM_F_CREATE 0x400
#define NLM_F_APPEND 0x800

#define NLMSG_NOOP 0x1
#define NLMSG_ERROR 0x2
#define NLMSG_DONE 0x3

#define RTA_ALIGNTO 4
#define RTA_ALIGN(len) (((len) + RTA_ALIGNTO - 1) & ~(RTA_ALIGNTO - 1))
#define RTA_LENGTH(len) (RTA_ALIGN(sizeof(struct rtattr)) + (len))
#define RTA_SPACE(len) RTA_ALIGN(RTA_LENGTH(len))
#define RTA_DATA(rta) ((void *)(((char *)(rta)) + RTA_LENGTH(0)))
#define RTA_PAYLOAD(rta) ((int)((rta)->rta_len) - RTA_LENGTH(0))
#define RTA_NEXT(rta, attrlen) ((attrlen) -= RTA_ALIGN((rta)->rta_len), (struct rtattr *)(((char *)(rta)) + RTA_ALIGN((rta)->rta_len)))
#define RTA_OK(rta, len) ((len) >= (int)sizeof(struct rtattr) && (rta)->rta_len >= sizeof(struct rtattr) && (int)(rta)->rta_len <= (len))

#define RTM_NEWLINK 16
#define RTM_DELLINK 17
#define RTM_GETLINK 18
#define RTM_NEWADDR 20
#define RTM_DELADDR 21
#define RTM_GETADDR 22

#define IFLA_UNSPEC 0
#define IFLA_ADDRESS 1
#define IFLA_BROADCAST 2
#define IFLA_IFNAME 3
#define IFLA_MTU 4
#define IFLA_LINK 5
#define IFLA_QDISC 6
#define IFLA_OPERSTATE 16
#define IFLA_LINKMODE 17
#define IFLA_LINKINFO 18
#define IFLA_INFO_KIND 1
#define IFLA_INFO_DATA 2
#define IFLA_NET_NS_PID 19
#define IFLA_IFALIAS 20
#define IFLA_NUM_TX_QUEUES 31
#define IFLA_NUM_RX_QUEUES 32
#define IFLA_NET_NS_FD 28
#define IFLA_MASTER 10
#define IFLA_LINKMODE 17
#define IFLA_MAX 64

#define IFLA_RTA(r) ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ifinfomsg))))
#define IFLA_PAYLOAD(n) ((int)((n)->nlmsg_len) - NLMSG_LENGTH(sizeof(struct ifinfomsg)))

#define IFA_UNSPEC 0
#define IFA_ADDRESS 1
#define IFA_LOCAL 2
#define IFA_LABEL 3
#define IFA_BROADCAST 4
#define IFA_ANYCAST 5
#define IFA_CACHEINFO 6
#define IFA_FLAGS 8
#define IFA_MAX 8

#define IFA_F_SECONDARY 0x01
#define IFA_F_TENTATIVE 0x40
#define IFA_F_PERMANENT 0x80
#define IFA_F_DADFAILED 0x08
#define IFA_F_DEPRECATED 0x20
#define IFA_F_NOPREFIXROUTE 0x200

#define IFA_RTA(r) ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#define IFA_PAYLOAD(n) ((int)((n)->nlmsg_len) - NLMSG_LENGTH(sizeof(struct ifaddrmsg)))

#define RT_SCOPE_UNIVERSE 0
#define RT_SCOPE_LINK 253
#define RT_SCOPE_HOST 254
#define RT_SCOPE_NOWHERE 255

#define RTN_UNSPEC 0
#define RTN_UNICAST 1
#define RTN_LOCAL 2
#define RTN_BROADCAST 3
#define RTN_ANYCAST 4
#define RTN_MULTICAST 5
#define RTN_BLACKHOLE 6
#define RTN_UNREACHABLE 7
#define RTN_PROHIBIT 8
#define RTN_THROW 9
#define RTN_NAT 10
#define RTN_XRESOLVE 11

#endif
