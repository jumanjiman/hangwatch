/* hangwatch.c -- load monitor for triggering sysrq dumps during system hangs

Copyright (c) 2006 Red Hat, Inc. All rights reserved. This copyrighted material 
is made available to anyone wishing to use, modify, copy, or 
redistribute it subject to the terms and conditions of the GNU General 
Public License v.2.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

Author: Chris Snook, csnook@redhat.com

This program periodically polls /proc/loadavg and invokes the specified sysrq
triggers if the load average exceeds a specified threshold.  It should work
even when a cron job or script fails to fork, since no fork is required.

TODO: daemonize

BUGS: incomplete error checking for things that should never happen

*/

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_INTERVAL 1
#define DEFAULT_TRIGGERS "mpt"
#define DEFAULT_THRESHOLD 5.0

#define OPTSTRING "i:s:t:"

#define BUFSIZE 256


void usage()
{
	printf("hangwatch (C) 2006, Red Hat, Inc.\n");
	printf("Author: Chris Snook, csnook@redhat.com\n");
	printf("Distributed under version 2 of the GNU General Public License.\n\n");
	printf("Usage:\n");
	printf("hangwatch [-i minutes] [-s sysrq_triggers] [-t load_threshold]\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int interval = DEFAULT_INTERVAL;
	char triggers[BUFSIZE] = DEFAULT_TRIGGERS;
	double threshold = DEFAULT_THRESHOLD;
	char option;
	do
	{
		option = getopt(argc, argv, OPTSTRING);
		switch (option)
		{
			case 'i':
			{
				interval = atoi(optarg);
				if (interval < 1)
					usage();
				break;
			}
			case 's':
			{
				strncpy(triggers, optarg, BUFSIZE);
				break;
			}
			case 't':
			{
				threshold = atof(optarg);
				if (!(threshold > 0.0))
					usage();
				break;
			}
		}
	} while ((option != -1) && (option != ':') && (option != '?'));
	if (option != -1)
		usage();

	char procbuf[BUFSIZE];
	int sysrqfd, loadfd, rc;
	sysrqfd = open("/proc/sys/kernel/sysrq", O_RDWR);
	if (sysrqfd == -1)
	{
		perror("Can't open /proc/sys/kernel/sysrq");
		exit(errno);
	}
	rc = read(sysrqfd, procbuf, BUFSIZE);
	if (rc < 1)
	{
		perror("Can't read from /proc/sys/kernel/sysrq");
		close(sysrqfd);
		exit(errno);
	}
	if (procbuf[0] == '0')
	{
		printf("SysRq not enabled.  Enabling...\n");
		rc = write(sysrqfd, "1", 1);
		if (rc < 1)
		{
			perror("Can't write to /proc/sys/kernel/sysrq");
			close(sysrqfd);
			exit(errno);
		}
	}
	close(sysrqfd);
	sysrqfd = open("/proc/sysrq-trigger", O_WRONLY);
	if (sysrqfd == -1)
	{
		perror("Can't open /proc/sysrq-trigger");
		exit(errno);
	}
	loadfd = open("/proc/loadavg", O_RDONLY);
	if (loadfd == -1)
	{
		perror("Can't open /proc/loadavg");
		close(sysrqfd);
		exit(errno);
	}
	printf("Monitoring...\n");

	char *index;
	double load;
	int i;
	while(1)
	{
		rc = read(loadfd, procbuf, BUFSIZE);
		if (rc < 1)
		{
			perror("Can't read from /proc/loadavg");
			close(loadfd);
			close(sysrqfd);
			exit(errno);
		}
		lseek(loadfd, 0, SEEK_SET);
		index = strchr(procbuf, ' ');
		if (index == NULL)
		{
			printf("Unable to parse /proc/loadavg\n");
			close(loadfd);
			close(sysrqfd);
			exit(1);
		}
		*index = '\0';
		load = atof(procbuf);
		if (load > threshold)
		{
			for (i = 0; i < strlen(triggers); i++)
			{
				write(sysrqfd, triggers + i, 1);
				sleep(1);
			}
			printf("Load exceeded threshold.  Triggers activated.\n");
		}
		sleep(60 * interval);
	}
	return 0;
}
