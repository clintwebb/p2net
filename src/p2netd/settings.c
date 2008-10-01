// settings.c

#include "p2net.h"
#include "settings.h"


void settings_init(Settings *ptr)
{
	assert(ptr != NULL);

	ptr->port = DEFAULT_PORT;
	ptr->maxconns = DEFAULT_MAXCONNS;
	ptr->verbose = false;
  ptr->daemonize = false;

}

