// connections.c

#include "connections.h"

/*
 * Free list management for connections.
 */
static conn **freeconns;
static int freetotal;
static int freecurr;

//- not sure if we should do it this way.
void conn_init(void) {
    freetotal = 200;
    freecurr = 0;
    if ((freeconns = (conn **)malloc(sizeof(conn *) * freetotal)) == NULL) {
        fprintf(stderr, "malloc()\n");
    }
    return;
}

/*
 * Returns a connection from the freelist, if any. Should call this using
 * conn_from_freelist() for thread safety.
 */
conn *do_conn_from_freelist() {
    conn *c;

    if (freecurr > 0) {
        c = freeconns[--freecurr];
    } else {
        c = NULL;
    }

    return c;
}

/*
 * Adds a connection to the freelist. 0 = success. Should call this using
 * conn_add_to_freelist() for thread safety.
 */
bool do_conn_add_to_freelist(conn *c) {
    if (freecurr < freetotal) {
        freeconns[freecurr++] = c;
        return false;
    } else {
        /* try to enlarge free connections array */
        conn **new_freeconns = realloc(freeconns, sizeof(conn *) * freetotal * 2);
        if (new_freeconns) {
            freetotal *= 2;
            freeconns = new_freeconns;
            freeconns[freecurr++] = c;
            return false;
        }
    }
    return true;
}

// This function is called when a new connection is received.
conn *conn_new(const int sfd, enum conn_states init_state,
                const int event_flags,
                const int read_buffer_size, enum protocol prot,
                struct event_base *base) {
	
	conn *c = conn_from_freelist();

	if (NULL == c) {
		if (!(c = (conn *)malloc(sizeof(conn)))) {
			fprintf(stderr, "malloc()\n");
			return NULL;
		}
		c->rbuf = c->wbuf = 0;
		c->ilist = 0;
		c->suffixlist = 0;
		c->iov = 0;
		c->msglist = 0;
		c->hdrbuf = 0;
		
		c->rsize = read_buffer_size;
		c->wsize = DATA_BUFFER_SIZE;
		c->isize = ITEM_LIST_INITIAL;
		c->suffixsize = SUFFIX_LIST_INITIAL;
		c->iovsize = IOV_LIST_INITIAL;
		c->msgsize = MSG_LIST_INITIAL;
		c->hdrsize = 0;
		
		c->rbuf = (char *)malloc((size_t)c->rsize);
		c->wbuf = (char *)malloc((size_t)c->wsize);
		c->ilist = (item **)malloc(sizeof(item *) * c->isize);
		c->suffixlist = (char **)malloc(sizeof(char *) * c->suffixsize);
		c->iov = (struct iovec *)malloc(sizeof(struct iovec) * c->iovsize);
		c->msglist = (struct msghdr *)malloc(sizeof(struct msghdr) * c->msgsize);
		
		if (c->rbuf == 0 || c->wbuf == 0 || c->ilist == 0 || c->iov == 0 || c->msglist == 0 || c->suffixlist == 0) {
			if (c->rbuf != 0) free(c->rbuf);
			if (c->wbuf != 0) free(c->wbuf);
			if (c->ilist !=0) free(c->ilist);
			if (c->suffixlist != 0) free(c->suffixlist);
			if (c->iov != 0) free(c->iov);
			if (c->msglist != 0) free(c->msglist);
			free(c);
			fprintf(stderr, "malloc()\n");
			return NULL;
		}
		
		STATS_LOCK();
		stats.conn_structs++;
		STATS_UNLOCK();
	}

	/* unix socket mode doesn't need this, so zeroed out.  but why
		* is this done for every command?  presumably for UDP
		* mode.  */
	if (!settings.socketpath) {
		c->request_addr_size = sizeof(c->request_addr);
	} else {
		c->request_addr_size = 0;
	}

	c->sfd = sfd;
	c->protocol = prot;
	
	if (prot == ascii_prot) { c->handler = asc_event_handler; }
	else if (prot == ascii_udp_prot) { c->handler = udp_event_handler; }
	else if (prot == binary_prot) { c->handler = bin_event_handler; }
	else { c->handler = NULL; }
	
	c->extra = NULL;
	
	c->state = init_state;
	c->rlbytes = 0;
	c->rbytes = c->wbytes = 0;
	c->wcurr = c->wbuf;
	c->rcurr = c->rbuf;
	c->ritem = 0;
	c->icurr = c->ilist;
	c->suffixcurr = c->suffixlist;
	c->ileft = 0;
	c->suffixleft = 0;
	c->iovused = 0;
	c->msgcurr = 0;
	c->msgused = 0;
	
	c->write_and_go = init_state;
	c->write_and_free = 0;
	c->item = 0;
	c->bucket = -1;
	c->gen = 0;
	
	event_set(&c->event, sfd, event_flags, c->handler, (void *)c);
	c->noreply = false;
	event_base_set(base, &c->event);
	c->ev_flags = event_flags;
	
	if (event_add(&c->event, 0) == -1) {
		if (conn_add_to_freelist(c)) {
			conn_free(c);
		}
		perror("event_add");
		return NULL;
	}

	STATS_LOCK();
	stats.curr_conns++;
	stats.total_conns++;
	STATS_UNLOCK();
	
	return c;
}

static void conn_cleanup(conn *c) {
	assert(c != NULL);
	
	if (c->item) {
		item_remove(c->item);
		c->item = 0;
	}
	
	if (c->ileft != 0) {
		for (; c->ileft > 0; c->ileft--,c->icurr++) {
			item_remove(*(c->icurr));
		}
	}
	
	if (c->suffixleft != 0) {
		for (; c->suffixleft > 0; c->suffixleft--, c->suffixcurr++) {
			if(suffix_add_to_freelist(*(c->suffixcurr))) {
				free(*(c->suffixcurr));
			}
		}
	}
	
	if (c->write_and_free) {
		free(c->write_and_free);
		c->write_and_free = 0;
	}
}

/*
 * Frees a connection.
 */
void conn_free(conn *c) {
	if (c) {
		if (c->hdrbuf)
			free(c->hdrbuf);
		if (c->msglist)
			free(c->msglist);
		if (c->rbuf)
			free(c->rbuf);
		if (c->wbuf)
			free(c->wbuf);
		if (c->ilist)
			free(c->ilist);
		if (c->suffixlist)
			free(c->suffixlist);
		if (c->iov)
			free(c->iov);
				
		/* CJW-TODO: Need to actually clean up the extra data first, or we could introduce memory leaks */
		if (c->extra) 
			free(c->extra);
		free(c);
	}
}

static void conn_close(conn *c) {
	assert(c != NULL);
	
	/* delete the event, the socket and the conn */
	event_del(&c->event);
	
	if (settings.verbose > 1)
		fprintf(stderr, "<%d connection closed.\n", c->sfd);
	
	close(c->sfd);
	accept_new_conns(true);
	conn_cleanup(c);
	
	/* if the connection has big buffers, just free it */
	if (c->rsize > READ_BUFFER_HIGHWAT || conn_add_to_freelist(c)) {
		conn_free(c);
	}
	
	STATS_LOCK();
	stats.curr_conns--;
	STATS_UNLOCK();
	
	return;
}

