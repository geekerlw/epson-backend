/*
* Copyright (C) Seiko Epson Corporation 2014.
*
* This file is part of the `ecbd' program.
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
* As a special exception, AVASYS CORPORATION gives permission
* to link the code of this program with the `cbt' library and
* distribute linked combinations including the two.  You must obey
* the GNU General Public License in all respects for all of the
* code used other then `cbt'.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <fcntl.h>
#include <Windows.h>
//#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
//#include <dirent.h>
#include <io.h>
#include "epson-daemon.h"

#define CONFIG_FILE_LENGTH_MAX 1024 /* Maximum size of configuration file */
#define KEY_WORD_MAX 20		/* Maximum size of attribute title */

#define DEVICEID_LENGTH 512
#define PATH_SIZE 256
#define PATH_MAX 4

#if 1
#define _IOC_NRBITS	8
#define _IOC_TYPEBITS	8

#ifndef _IOC_SIZEBITS
# define _IOC_SIZEBITS	14
#endif

#ifndef _IOC_DIRBITS
# define _IOC_DIRBITS	2
#endif

#define _IOC_NRMASK	((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK	((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK	((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK	((1 << _IOC_DIRBITS)-1)

#define _IOC_NRSHIFT	0
#define _IOC_TYPESHIFT	(_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT	(_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT	(_IOC_SIZESHIFT+_IOC_SIZEBITS)

#ifndef _IOC_READ
# define _IOC_READ	2U
#endif

#define _IOC(dir,type,nr,size) \
	(((dir)  << _IOC_DIRSHIFT) | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT) | \
	 ((size) << _IOC_SIZESHIFT))

#endif

static void get_parameter(char*, char*, char*, int*);
static void clean_text(char*);

static	char deviceid[DEVICEID_LENGTH];

#define DEVICEID_LENGTH 512

static int /* Return 0 when the searching printer is found or not 0. (*printer="ESCP") */
is_searching_printer(const char * usblp, const char * printer)
{
	char usbpath[PATH_MAX];
	memset(usbpath, 0x00, sizeof(usbpath));
	snprintf(usbpath, sizeof(usbpath), "%s/%s", "/dev/usb", usblp);

	int result = 1; /* not found */
	int fd = open(usbpath, O_RDWR);

	if (fd >= 0) {
		memset(deviceid, 0x00, DEVICEID_LENGTH);

		/* GET_DEVICE_ID = 1 */
		int iostatus = ioctl(fd, _IOC(_IOC_READ, 'P', 1, DEVICEID_LENGTH), deviceid);

		if (iostatus == 0) {
			if (strstr(deviceid + 2, printer)) {
				result = 0; /* found */
			}
		}

		close(fd);
	}

	return result;
}


static int
get_printer_devpath(const char * printer, char * path, int sizePath)
{

	/* todo: windows no linux dir api */
	/*
	struct dirent * lp = NULL;
	DIR * devpath = opendir("/dev/usb");
	int error = 1;

	if (devpath) {
		while ((lp = readdir(devpath)) != NULL) {
			if (lp->d_name[0] == '.') {
				continue;
			}

			if (strncmp(lp->d_name, "lp", 2) != 0) {
				continue;
			}

			if (is_searching_printer(lp->d_name, printer) == 0) {
				memset(path, 0x00, sizePath);
				snprintf(path, sizePath, "/dev/usb/%s", lp->d_name);
				error = 0;
				break;
			}
		}

		closedir(devpath);
	}

	return error;
	*/

	return 0;
}

// todo: 应该从这里获取到打印机的端口信息，libusb来做
int parameter_update(P_CBTD_INFO p_info)
{
	return get_printer_devpath("ESCP", p_info->devprt_path, sizeof(p_info->devprt_path));
}


/* get settings */
static void
get_parameter(char *buf, char *key, char *get_string, int *get_value)
{
	int len;
	char *tmp_buf;

	tmp_buf = strstr(buf, key);
	if (tmp_buf == NULL) return;

	tmp_buf += strlen(key);
	if (*tmp_buf != '=' || *(++tmp_buf) == '\0')
	{
		return;
	}

	len = strcspn(tmp_buf, " ");
	if (get_string != NULL && len < CONF_BUF_MAX)
	{
		strncpy(get_string, tmp_buf, len);
	}

	/* 5 = the greatest number of digits */
	if (get_value != NULL && len < 6)
	{
		char val_word[6];

		strncpy(val_word, tmp_buf, len);
		*get_value = atoi(val_word);
	}
	return;
}


/* fix data in buffer */
static void
clean_text(char *buf)
{
	char *new_buf = buf;
	int i, j;

	j = 0;
	for (i = 0; buf[i] != '\0'; i++)
	{
		if (isspace(buf[i]))
		{
			if (buf[i] == '\n')
				new_buf[j++] = ' ';
		}
		else
		{
			new_buf[j++] = buf[i];
		}
	}

	new_buf[j] = '\0';
	return;
}

/* get deviceid */
void
get_device_id(char* device_id)
{
	int skip = 0;

	if (deviceid[0] == 0x00 || deviceid[1] == 0x00) {
		skip = 2;
	}

	strcpy(device_id, deviceid + skip);

}

