// connections.h
#ifndef __CONNECTIONS_H
#define __CONNECTIONS_H

enum conn_states {
	conn_listening,  /** the socket which listens for connections */
	conn_read,       /** reading in a command line */
	conn_write,      /** writing out a simple response */
	conn_nread,      /** reading in a fixed number of bytes */
	conn_swallow,    /** swallowing unnecessary bytes w/o storing */
	conn_closing,    /** closing this connection */
	conn_mwrite,     /** writing out many items sequentially */
	conn_bin_init,   /** Reinitializing a binary protocol connection */
	conn_negotiate,  /** Negotiating a protocol */
};


typedef struct {
    int    sfd;
    int    state;
    enum conn_states  state;
    struct event event;
    short  ev_flags;

    char   *rbuf;   /** buffer to read commands into */
    char   *rcurr;  /** but if we parsed some already, this is where we stopped */
    int    rsize;   /** total allocated size of rbuf */
    int    rbytes;  /** how much data, starting from rcur, do we have unparsed */

    char   *wbuf;
    char   *wcurr;
    int    wsize;
    int    wbytes;
    int    write_and_go; /** which state to go into after finishing current write */
    void   *write_and_free; /** free this memory after finishing writing */

    char   *ritem;  /** when we read in an item's value, it goes here */
    int    rlbytes;

    /* data for the nread state */

    /**
     * item is used to hold an item structure created after reading the command
     * line of set/add/replace commands, but before we finished reading the actual
     * data. The data is read into ITEM_data(item) to avoid extra copying.
     */

    void   *item;     /* for commands set/add/replace  */
    int    item_comm; /* which one is it: set/add/replace */

    /* data for the swallow state */
    int    sbytes;    /* how many bytes to swallow */

    /* data for the mwrite state */
    struct iovec *iov;
    int    iovsize;   /* number of elements allocated in iov[] */
    int    iovused;   /* number of elements used in iov[] */

    struct msghdr *msglist;
    int    msgsize;   /* number of elements allocated in msglist[] */
    int    msgused;   /* number of elements used in msglist[] */
    int    msgcurr;   /* element in msglist[] being transmitted now */
    int    msgbytes;  /* number of bytes in current msg */

   item   **ilist;   /* list of items to write out */
    int    isize;
    item   **icurr;
    int    ileft;

    char   **suffixlist;
    int    suffixsize;
    char   **suffixcurr;
    int    suffixleft;

<<<<<<< .mine
                int protocol;
=======
    enum protocol protocol;   /* which protocol this connection speaks */
>>>>>>> .r766

                /* CJW-TODO: Not sure what to do with this as it looks like UDP is used for both ascii and binary protocol */
    /* data for UDP clients */
    int    request_id; /* Incoming UDP request ID, if this is a UDP "connection" */
    struct sockaddr request_addr; /* Who sent the most recent request */
    socklen_t request_addr_size;
    unsigned char *hdrbuf; /* udp packet headers */
    int    hdrsize;   /* number of headers' worth of space is allocated */

    int    binary;    /* are we in binary mode */
    int    bucket;    /* bucket number for the next command, if running as
                        a managed instance. -1 (_not_ 0) means invalid. */
    int    gen;       /* generation requested for the bucket */
    bool   noreply;   /* True if the reply should not be sent. */

    /* Callback function for handling the protocol for this connection, and a void pointer to the datablock specific for the protocol. */
    void (*handler)(const int fd, const short which, void *arg);
    void *extra;
    conn   *next;     /* Used for generating a list of conn structures */
} conn;




void conn_init(void);
conn *do_conn_from_freelist(void);


#endif
