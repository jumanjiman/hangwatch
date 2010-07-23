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
   Updated by: Adam Stokes, astokes@redhat.com

   This program periodically polls /proc/loadavg and invokes the specified sysrq
   triggers if the load average exceeds a specified threshold.  It should work
   even when a cron job or script fails to fork, since no fork is required.

   BUGS: incomplete error checking for things that should never happen

   10.11.07 Adam Stokes <astokes@redhat.com>
   - added daemonize, code cleanup
   10.24.07 Adam Stokes <astokes@redhat.com>
   - signal handling
   3.10.10 Mauro Rappa <mrappa@redhat.com>
   - [PATCH] copy slabinfo/meminfo into log
   3.11.10 Adrien Kunysz <adk@redhat.com>
   - [PATCH] fd leak fix
   - [PATCH] cosmetic changes

*/

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#define DEFAULT_INTERVAL 1
#define DEFAULT_TRIGGERS "mpt"
#define DEFAULT_THRESHOLD 5.0

// Daemon info
#define DAEMON_NAME "hangwatch"
#define PID_FILE "/var/run/hangwatch.pid"

#define OPTSTRING "i:s:t:hnlm|help"

#define BUFSIZE 256


void usage(const char *progname)
{
    printf("hangwatch (C) 2006-2007 Red Hat, Inc.\n");
    printf("Distributed under version 2 of "\
           "the GNU General Public License.\n\n");
    printf("Usage: %s -h -t <#> -s <mpt> -i <#>\n", progname);
    printf("\tOptions:\n");
    printf("\t\t-h This help screen.\n");
    printf("\t\t-s \"mpt\" sysrq triggers.\n");
    printf("\t\t-t # Load threshold.\n");
    printf("\t\t-i # Time interval in min(s).\n");
    printf("\t\t-n # Run application in foreground.\n");
    printf("\t\t-l # Print Slabinfo.\n");
    printf("\t\t-m # Print Meminfo.\n");
    exit(1);
}

void signal_handler(int sig)
{
    switch(sig) {
    case SIGHUP:
        syslog(LOG_WARNING, "Received SIGHUP signal.");
        exit(0);
        break;
    case SIGTERM:
        syslog(LOG_WARNING, "Received SIGTERM signal.");
        exit(0);
        break;
    default:
        syslog(LOG_WARNING, "Unhandled signal");
        exit(0);
        break;
    }
}

static void procinfo(const char *filename)
{
    FILE*fp;
    char lyne[121];

    if((fp = fopen("/proc/meminfo","r")) == NULL) {
        printf("Cannot open file %s\n", filename);
    }
    while (fgets(lyne, sizeof(lyne) -1, fp)) {
        syslog(LOG_WARNING, "%s", lyne);
    }
    fclose(fp);
}

int main(int argc, char **argv)
{
    int daemonize = 1;
    int slab = 0;
    int mem = 0;
    int interval = DEFAULT_INTERVAL;
    char triggers[BUFSIZE] = DEFAULT_TRIGGERS;
    double threshold = DEFAULT_THRESHOLD;
    int c;
	
    // Setup signal handling
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
	
    while ( (c = getopt(argc, argv, OPTSTRING)) != -1) {
        switch (c) {
        case 'h':
            usage(argv[0]);
            break;
        case 'i':
            interval = atoi(optarg);
            if (interval < 1)
                usage(argv[0]);
            break;
        case 's':
            strncpy(triggers, optarg, BUFSIZE);
            break;
        case 't':
            threshold = atof(optarg);
            if (!(threshold > 0.0))
                usage(argv[0]);
            break;
        case 'l': 
            slab = 1;
            break;
        case 'm': 
            mem = 1;
            break;
        case 'n': 
            daemonize = 0;
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    syslog(LOG_INFO, "%s daemon started.", DAEMON_NAME);
	
    // our pid and sid 
    pid_t pid, sid;

    if(daemonize) {
        // Fork off parent
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        // exit parent if pid is good
        if(pid > 0) {
            exit(EXIT_SUCCESS);
        }

        // change file mode mask
        umask(0);

        // create sid for child
        sid = setsid();
        if(sid < 0) {
            exit(EXIT_FAILURE);
        }

        // close out fds
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    char procbuf[BUFSIZE];
    int sysrqfd, loadfd, rc;
    sysrqfd = open("/proc/sys/kernel/sysrq", O_RDWR);
    if (sysrqfd == -1) {
        perror("Can't open /proc/sys/kernel/sysrq");
        exit(errno);
    }
    rc = read(sysrqfd, procbuf, BUFSIZE);
    if (rc < 1) {
        perror("Can't read from /proc/sys/kernel/sysrq");
        close(sysrqfd);
        exit(errno);
    }
    if (procbuf[0] == '0') {
        printf("SysRq not enabled.  Enabling...\n");
        rc = write(sysrqfd, "1", 1);
        if (rc < 1) {
            perror("Can't write to /proc/sys/kernel/sysrq");
            close(sysrqfd);
            exit(errno);
        }
    }
    close(sysrqfd);
    sysrqfd = open("/proc/sysrq-trigger", O_WRONLY);
    if (sysrqfd == -1) {
        perror("Can't open /proc/sysrq-trigger");
        exit(errno);
    }
    loadfd = open("/proc/loadavg", O_RDONLY);
    if (loadfd == -1) {
        perror("Can't open /proc/loadavg");
        close(sysrqfd);
        exit(errno);
    }
    printf("Monitoring...\n");

    char *index;
    double load;
    size_t i;
    while(1) {
        rc = read(loadfd, procbuf, BUFSIZE);
        if (rc < 1) {
            perror("Can't read from /proc/loadavg");
            close(loadfd);
            close(sysrqfd);
            exit(errno);
        }
        lseek(loadfd, 0, SEEK_SET);
        index = strchr(procbuf, ' ');
        if (index == NULL) {
            printf("Unable to parse /proc/loadavg\n");
            close(loadfd);
            close(sysrqfd);
            exit(1);
        }
        *index = '\0';
        load = atof(procbuf);
        if (load > threshold) {
            for (i = 0; i < strlen(triggers); i++) {
                write(sysrqfd, triggers + i, 1);

                if (slab == 1) { procinfo("/proc/slabinfo"); }
                if (mem == 1) { procinfo("/proc/meminfo"); }

                sleep(1);
            }
            printf(" Load exceeded threshold.  Triggers activated.\n");
        }
        sleep(60 * interval);
    }
    exit(0);
}

