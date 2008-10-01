// settings
#ifndef __SETTINGS_H
#define __SETTINGS_H

typedef struct {
	int port;
	int maxconns;
	bool verbose;
  bool daemonize;
} Settings;

void settings_init(Settings *ptr);

#endif
