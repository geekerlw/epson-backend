/*
 * Copyright (C) Seiko Epson Corporation 2014.
 *
 *  This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * As a special exception, AVASYS CORPORATION gives permission to
 * link the code of this program with libraries which are covered by
 * the AVASYS Public License and distribute their linked
 * combinations.  You must obey the GNU General Public License in all
 * respects for all of the code used other than the libraries which
 * are covered by AVASYS Public License.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <cups/http.h>

#include "err.h"
#include "../daemon/epson-typdefs.h"

#define ECBLP_VERSION "* ecblp is a part of " PACKAGE_STRING

#define ECBLP_USAGE "usage: $ ecblp job-id user title copies options [file]"


void	list_devices (void);

/*
 * usage: printer-uri job-id user title copies options [file]
 */
int
main (int argc, char *argv[])
{
	char method[255];	/* Method in URI */
	char hostname[1024];	/* Hostname */
	char username[255];	/* Username info (not used) */
	char resource[1024];	/* Resource info (device and options) */
	char *options;		/* Pointer to options */
	int port;		/* Port number (not used) */
	FILE *fp;		/* Print file */
	int copies;		/* Number of copies to print */
	int fd;			/* Parallel device */
	int wbytes;		/* Number of bytes written */
	size_t nbytes;		/* Number of bytes read */
	size_t tbytes;		/* Total number of bytes written */
	char buffer[8192];	/* Output buffer */
	char *bufptr;		/* Pointer into buffer */
	int i;
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action; /* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */


	setbuf (stderr, NULL);

	if (argc == 1)
	{
		list_devices ();
		return 0;
	}
	else if (argc < 6 || argc > 7)
	{
		for ( i = 1; i < argc; i++ ) {
			if ( (0 == strncmp(argv[i], "-v", (strlen("-v")+1)) )
				|| (0 == strncmp(argv[i], "--version", (strlen("--version")+1)) )
				) {
				fprintf(stderr, "%s\n", ECBLP_VERSION);
				return 0;
			} else if ( (0 == strncmp(argv[i], "-u", (strlen("-u")+1)) )
				|| (0 == strncmp(argv[i], "--usage", (strlen("--usage")+1)) )
				) {
				fprintf(stderr, "%s\n", ECBLP_USAGE);
				return 0;
			} else if ( (0 == strncmp(argv[i], "-h", (strlen("-h")+1)) )
				|| (0 == strncmp(argv[i], "--help", (strlen("--help")+1)) )
				) {
				fprintf(stderr, "%s\n%s\n", ECBLP_VERSION, ECBLP_USAGE);
				return 0;
			}
		}
		err_msg (MSGTYPE_MESSAGE, ECBLP_USAGE);
		return 1;
	}

	if (argc == 7)
	{
		if ((fp = fopen (argv[6], "rb")) == NULL)
			err_system (argv[6]);

		copies = atoi (argv[4]);
	}
	else
	{
		fp = stdin;
		copies = 1;
	}

//	httpSeparate (argv[0], method, username, hostname, &port, resource);
	strcpy(resource, FIFO_PATH);


	if ((options = strchr (resource, '?')) != NULL)
		*options++ = '\0';

	do
	{
		if ((fd = open (resource, O_WRONLY | O_EXCL)) == -1)
		{
			if (errno == EBUSY)
			{
				err_msg (MSGTYPE_INFO, "EPSONBACKEND port busy; will retry in 30 seconds...\n");
				sleep (30);
			}
			else
			{
				err_fatal ("Unable to open EPSONBACKEND port device file \"%s\": %s\n",
					 resource, strerror (errno));
			}
		}
	}
	while (fd < 0);

	if (argc < 7)
	{
#ifdef HAVE_SIGSET /* Use System V signals over POSIX to avoid bugs */
		sigset (SIGTERM, SIG_IGN);
#elif defined(HAVE_SIGACTION)
		memset (&action, 0, sizeof(action));

		sigemptyset (&action.sa_mask);
		action.sa_handler = SIG_IGN;
		sigaction (SIGTERM, &action, NULL);
#else
		signal (SIGTERM, SIG_IGN);
#endif /* HAVE_SIGSET */
	}

	/*
	 * Finally, send the print file...
	 */

	while (copies > 0)
	{
		copies --;

		if (fp != stdin)
		{
			err_msg (MSGTYPE_MESSAGE, "PAGE: 1 1\n");
			rewind(fp);
		}

		tbytes = 0;
		while ((nbytes = fread (buffer, 1, sizeof (buffer), fp)) > 0)
		{
			/*
			 * Write the print data to the printer...
			 */

			tbytes += nbytes;
			bufptr = buffer;

			while (nbytes > 0)
			{
				if ((wbytes = write (fd, bufptr, nbytes)) < 0)
					if (errno == ENOTTY)
						wbytes = write (fd, bufptr, nbytes);

				if (wbytes < 0)
				{
					err_msg (MSGTYPE_ERROR, "Unable to send print file to printer.");
					break;
				}

				nbytes -= wbytes;
				bufptr += wbytes;
			}

			if (argc > 6)
				err_msg (MSGTYPE_INFO, "INFO: Sending print file, %lu bytes...\n",
					(unsigned long)tbytes);
		}
	}

	/*
	 * Close the socket connection and input file and return...
	 */

	close(fd);
	if (fp != stdin)
		fclose(fp);

	return (0);
}


/*
 * 'list_devices ()' - List all EPSONBACKEND devices.
 */

void
list_devices (void)
{
	int i;
	char device [255];

	for (i = 0;;i ++)
	{
		sprintf (device, "/var/run/ecblp%d", i);
		
		if (access (device, W_OK) != 0)
			break;
		
		printf ("direct ecblp:%s "
			"\"Epson Backend\" "
			"\"Epson Inkjet Printer #%d\"\n"
			, device, i + 1);
	}

	return;
}
