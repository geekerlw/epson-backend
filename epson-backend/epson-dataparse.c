/*
* Copyright (C) Seiko Epson Corporation 2017.
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
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "epson-dataparse.h"

#define TIME_OUT 3
#define EPS_INK_NORMALIZE_LEVEL (5)

typedef struct INK_NODE {
	unsigned long id;
	int rest;
	struct INK_NODE *next;
} InkNode, *InkList;


static ECB_PRINTER_STS 	printer_status;
static ECB_PRINTER_ERR 	error_code;
static ECB_INT32   		ink_num;
static ECB_INT32   		colors[ECB_INK_NUM];
static ECB_INT32   		inklevel[ECB_INK_NUM];
static ECB_PAPER_COUNT  papercount;
static ECB_BOOL		showInkInfo;
static ECB_BOOL		showInkLow;
static InkList ink_list;

enum Inkbox_Id
{
	ECB_COLOR_BLACK,
	ECB_COLOR_CYAN,
	ECB_COLOR_MAGENTA,
	ECB_COLOR_YELLOW,
	ECB_COLOR_LIGHTCYAN,
	ECB_COLOR_LIGHTMAGENTA,
	ECB_COLOR_LIGHTYELLOW,
	ECB_COLOR_DARKYELLOW,
	ECB_COLOR_LIGHTBLACK,
	ECB_COLOR_RED,
	ECB_COLOR_VIOLET,
	ECB_COLOR_MATTE_BLACK,
	ECB_COLOR_LIGHTLIGHTBLACK,
	ECB_COLOR_PHOTO_BLACK,
	ECB_COLOR_CLEAR,
	ECB_COLOR_GRAY,
	ECB_COLOR_UNKNOWN,

	/* add ver5.0*/
	ECB_COLOR_BLACK2,
	ECB_COLOR_ORANGE,
	ECB_COLOR_GREEN,
	ECB_COLOR_WHITE,
	ECB_COLOR_CLEAN
};

enum Inkbox_Id inkbox_get_inkid(unsigned long id)
{
	enum Inkbox_Id color;

	switch (id) {
	case 0x1101:
	case 0x1140:
		color = ECB_COLOR_BLACK; break;
	case 0x3202:
		color = ECB_COLOR_CYAN; break;
	case 0x4304:
		color = ECB_COLOR_MAGENTA; break;
	case 0x5408:
		color = ECB_COLOR_YELLOW; break;
	case 0x6210:
		color = ECB_COLOR_LIGHTCYAN; break;
	case 0x7320:
		color = ECB_COLOR_LIGHTMAGENTA; break;
	case 0x9440:
		color = ECB_COLOR_DARKYELLOW; break;
	case 0xA140:
		color = ECB_COLOR_PHOTO_BLACK; break;
	case 0xB140:
	case 0xB101:
		color = ECB_COLOR_MATTE_BLACK; break;
	case 0xC140:
		color = ECB_COLOR_LIGHTBLACK; break;
	case 0x9020:
		/* 2004.01.21 Added Red */
		color = ECB_COLOR_RED; break;
	case 0xA040:
		/* 2004.01.21 Added Violet */
		color = ECB_COLOR_VIOLET; break;
	case 0xB080:
		/* 2004.01.21 Added Clear */
		color = ECB_COLOR_CLEAR; break;
	case 0xD010:
		color = ECB_COLOR_ORANGE; break;

	default:
		/* UNKNOWN COLOR */
		color = ECB_COLOR_UNKNOWN; break;
	}
	return	color;
}

static int hextoi(char hex)
{
	int value;
	switch (hex) {
	case '0':
		value = 0;
		break;
	case '1':
		value = 1;
		break;
	case '2':
		value = 2;
		break;
	case '3':
		value = 3;
		break;
	case '4':
		value = 4;
		break;
	case '5':
		value = 5;
		break;
	case '6':
		value = 6;
		break;
	case '7':
		value = 7;
		break;
	case '8':
		value = 8;
		break;
	case '9':
		value = 9;
		break;
	case 'A':
	case 'a':
		value = 10;
		break;
	case 'B':
	case 'b':
		value = 11;
		break;
	case 'C':
	case 'c':
		value = 12;
		break;
	case 'D':
	case 'd':
		value = 13;
		break;
	case 'E':
	case 'e':
		value = 14;
		break;
	case 'F':
	case 'f':
		value = 15;
		break;
	default:
		value = -1;
	}
	return value;
}


int serInkLevelNromalize(int level) 
{
	int norm = 0;
	if (EPS_INK_NORMALIZE_LEVEL > 3 && (level >= 1) && (level <= 3)) {
		norm = 1;
	}
	else {
		norm = (level / EPS_INK_NORMALIZE_LEVEL) * EPS_INK_NORMALIZE_LEVEL;
		if (level % EPS_INK_NORMALIZE_LEVEL) {
			norm += EPS_INK_NORMALIZE_LEVEL;
		}
	}
	return norm;
}


static ECB_PRINTER_STS ReadStatuslogfile(InkList *, ECB_PRINTER_ERR* errorCode);
static void ink_list_delete(InkList);

static ECB_PRINTER_STS ReadStatuslogfile(InkList *list, ECB_PRINTER_ERR* errorCode)
{
	char *lpInk, *lpSts, *lpErr, *lpCC, *lpMC, *lpWC, *lpAC, *lpCB, *lpMB, *lpCS;
	char lpReply[1024], StsCode[3], ErrCode[3];
	char get_status_command[] = { 's', 't', 0x01, 0x00, 0x01 };
	int com_len, rep_len;

	clock_t start, stop;

	start = time(NULL);
RETRY:

	/* Initialization */
	StsCode[2] = 0;
	ErrCode[2] = 0;
	*list = NULL;


	com_len = sizeof(get_status_command);
	rep_len = 1024;
	/* Log file reading */

	if (sock_write(get_status_command, &com_len) < 0)
	{
		if (sock_reopen() < 0)
		{
			return ECB_DAEMON_DOWN;
		}
		return ECB_DAEMON_NO_REPLY;
	}

	sock_read(lpReply, &rep_len);

	if (rep_len == 0)
	{
		double diff;
		stop = time(NULL);
		diff = difftime(stop, start);
		if (diff < TIME_OUT) goto RETRY;

		return ECB_DAEMON_NO_REPLY;
	}

	/* Ink kind analysis */
	lpInk = strstr(lpReply, "INK:");

	if (lpInk) {
		char ic[7] = "0x";
		InkNode *prev = NULL;

		lpInk += 4;     /* skip "INK:" */
		while (*lpInk != ';') {
			InkNode *node;

			node = (InkNode *)calloc(1, sizeof(InkNode));
			if (!node) {
				return ECB_DAEMON_NO_REPLY;
			}

			memcpy(ic + 2, lpInk, 4);
			*(ic + 6) = '\0';
			node->id = strtol(ic, (char **)NULL, 16);

			if (prev) {
				prev->next = node;
				prev = prev->next;
			}
			else {
				prev = node;
				*list = prev;
			}
			lpInk += 4;
			if (*lpInk == ',') {
				lpInk++;
			}
			node = node->next;
		}
	}
	else {
		char snink[3];
		int nink, i;
		InkNode *prev = NULL;

		/* 2004.03.18           */
		/* INK:... not reply    */
		/* check "AI:CW:"       */
		lpInk = strstr(lpReply, "AI:CW:");
		if (!lpInk)
			return ECB_DAEMON_NO_REPLY;
		/* Found!               */
		lpInk += 6;     /* skip "AI:CW:" */

						/* get no. of ink */
		sprintf(snink, "%.2s", lpInk);
		nink = atoi(snink);

		lpInk += 2;

		/* setup ink list */
		for (i = 0; i < nink; i++) {
			InkNode *node;

			node = (InkNode *)calloc(1, sizeof(InkNode));
			if (!node) {
				return ECB_DAEMON_NO_REPLY;
			}

			switch (i) {
			case    0: /* black */
				node->id = 0x1101; break;
			case    1: /* cyan */
				node->id = 0x3202; break;
			case    2: /* yellow */
				node->id = 0x4304; break;
			case    3: /* magenta */
				node->id = 0x5408; break;
			case    4: /* light cyan */
				node->id = 0x6210; break;
			case    5: /* light magenta */
				node->id = 0x7320; break;
			case    6: /* dark yellow */
				node->id = 0x9440; break;
			case    7: /* light black */
				node->id = 0x1140; break;
			default:
				node->id = 0x0; break;
			}

			if (prev) {
				prev->next = node;
				prev = prev->next;
			}
			else {
				prev = node;
				*list = prev;
			}
			node = node->next;
		}
	}

	/* Ink residual quantity analysis */
	lpInk = strstr(lpReply, "IQ:");

	if (lpInk) {
		char ink[3];
		char *stopstring;
		InkNode *node = *list;

		lpInk += 3;
		memset(ink, 0, 3);


		int ink_no = 0;
		while (node)
		{
			strncpy(ink, lpInk, 2);
			node->rest = (int)strtol(ink, &stopstring, 16);

			colors[ink_no] = inkbox_get_inkid(node->id);
			inklevel[ink_no] = node->rest;
			ink_no++;

			node = node->next;
			lpInk += 2;
			if (*lpInk == ';') break;
		}
		ink_num = ink_no;
	}

	// lpReply = ST:03;ER:01;CS:11;IQ:69696969;INK:1101540843043202;CC:32000000;MC:04000000;WC:00000000;AC:00000000;CB:00000000;MB:00000000;

	/* Characteristic status code */
	lpCS = strstr(lpReply, "CS:");

	if (lpCS) {
		showInkInfo = hextoi(lpCS[3]);
		showInkLow = hextoi(lpCS[4]);
	}
	else {
		showInkInfo = 1; // 1: Show Ink Information
		showInkLow = 1;  // 1: Show Ink Low
	}


	/* Color Printed Number */
	lpCC = strstr(lpReply, "CC:");
	if (lpCC) {
		papercount.color = (hextoi(lpCC[3]) * 16 + hextoi(lpCC[4])) + (hextoi(lpCC[5]) * 16 + hextoi(lpCC[6])) * 256 + (hextoi(lpCC[7]) * 16 + hextoi(lpCC[8]))*(256 ^ 2) + (hextoi(lpCC[9]) * 16 + hextoi(lpCC[10]))*(256 ^ 3);
	}
	else {
		papercount.color = -1;
	}

	/* Monochrome Printed Number */
	lpMC = strstr(lpReply, "MC:");
	if (lpMC) {
		papercount.monochrome = (hextoi(lpMC[3]) * 16 + hextoi(lpMC[4])) + (hextoi(lpMC[5]) * 16 + hextoi(lpMC[6])) * 256 + (hextoi(lpMC[7]) * 16 + hextoi(lpMC[8]))*(256 ^ 2) + (hextoi(lpMC[9]) * 16 + hextoi(lpMC[10]))*(256 ^ 3);

		//	printf("mono=%d\n", papercount.monochrome);


	}
	else {
		papercount.monochrome = -1;
	}

	/* Blank Printed Number */
	lpWC = strstr(lpReply, "WC:");
	if (lpWC) {
		papercount.blank = (hextoi(lpWC[3]) * 16 + hextoi(lpWC[4])) + (hextoi(lpWC[5]) * 16 + hextoi(lpWC[6])) * 256 + (hextoi(lpWC[7]) * 16 + hextoi(lpWC[8]))*(256 ^ 2) + (hextoi(lpWC[9]) * 16 + hextoi(lpWC[10]))*(256 ^ 3);

	}
	else {
		papercount.blank = -1;
	}

	/* ADF Scanned Number */
	lpAC = strstr(lpReply, "AC:");
	if (lpAC) {
		papercount.adf = (hextoi(lpAC[3]) * 16 + hextoi(lpAC[4])) + (hextoi(lpAC[5]) * 16 + hextoi(lpAC[6])) * 256 + (hextoi(lpAC[7]) * 16 + hextoi(lpAC[8]))*(256 ^ 2) + (hextoi(lpAC[9]) * 16 + hextoi(lpAC[10]))*(256 ^ 3);

	}
	else {
		papercount.adf = -1;
	}

	/* Color Bordereless Printed Number */
	lpCB = strstr(lpReply, "CB:");
	if (lpCB) {
		papercount.color_borderless = (hextoi(lpCB[3]) * 16 + hextoi(lpCB[4])) + (hextoi(lpCB[5]) * 16 + hextoi(lpCB[6])) * 256 + (hextoi(lpCB[7]) * 16 + hextoi(lpCB[8]))*(256 ^ 2) + (hextoi(lpCB[9]) * 16 + hextoi(lpCB[10]))*(256 ^ 3);

	}
	else {
		papercount.color_borderless = -1;
	}

	/* Monochrome Bordereless Printed Number */
	lpMB = strstr(lpReply, "MB:");
	if (lpMB) {
		papercount.monochrome_borderless = (hextoi(lpMB[3]) * 16 + hextoi(lpMB[4])) + (hextoi(lpMB[5]) * 16 + hextoi(lpMB[6])) * 256 + (hextoi(lpMB[7]) * 16 + hextoi(lpMB[8]))*(256 ^ 2) + (hextoi(lpMB[9]) * 16 + hextoi(lpMB[10]))*(256 ^ 3);

	}
	else {
		papercount.monochrome_borderless = -1;
	}

	/* Status analysis */
	lpSts = strstr(lpReply, "ST:");
	if (lpSts) {
		strncpy(StsCode, lpSts + 3, 2);

		if (!strcmp(StsCode, "00")) {

			/* Error status analysis */
			lpErr = strstr(lpReply, "ER:");
#if DEBUG
			fprintf(stderr, "error code = %s\n", lpErr);
#endif

			if (lpErr) {
				strncpy(ErrCode, lpErr + 3, 2);
				*errorCode = ECB_PRNERR_GENERAL;
				//fprintf(stderr, "error code = %s\n", ErrCode);
				if (!strcmp(ErrCode, "00")) *errorCode = ECB_PRNERR_FATAL;
				if (!strcmp(ErrCode, "01")) *errorCode = ECB_PRNERR_INTERFACE;
				if (!strcmp(ErrCode, "02")) *errorCode = ECB_PRNERR_COVEROPEN;
				if (!strcmp(ErrCode, "04")) *errorCode = ECB_PRNERR_PAPERJAM;
				if (!strcmp(ErrCode, "05")) *errorCode = ECB_PRNERR_INKOUT;
				if (!strcmp(ErrCode, "06")) *errorCode = ECB_PRNERR_PAPEROUT;
				if (!strcmp(ErrCode, "0A")) *errorCode = ECB_PRNERR_SIZE_TYPE_PATH;
				if (!strcmp(ErrCode, "10")) *errorCode = ECB_PRNERR_SERVICEREQ;
				if (!strcmp(ErrCode, "12")) *errorCode = ECB_PRNERR_DOUBLEFEED;
				if (!strcmp(ErrCode, "1A")) *errorCode = ECB_PRNERR_INKCOVEROPEN;
				if (!strcmp(ErrCode, "29")) *errorCode = ECB_PRNERR_NOTRAY;
				if (!strcmp(ErrCode, "2A")) *errorCode = ECB_PRNERR_CARDLOADING;
				if (!strcmp(ErrCode, "2B")) *errorCode = ECB_PRNERR_CDDVDCONFIG;
				if (!strcmp(ErrCode, "2C")) *errorCode = ECB_PRNERR_CARTRIDGEOVERFLOW;
				if (!strcmp(ErrCode, "2F")) *errorCode = ECB_PRNERR_BATTERYVOLTAGE;
				if (!strcmp(ErrCode, "30")) *errorCode = ECB_PRNERR_BATTERYTEMPERATURE;
				if (!strcmp(ErrCode, "31")) *errorCode = ECB_PRNERR_BATTERYEMPTY;
				if (!strcmp(ErrCode, "32")) *errorCode = ECB_PRNERR_SHUTOFF;
				if (!strcmp(ErrCode, "33")) *errorCode = ECB_PRNERR_NOT_INITIALFILL;
				if (!strcmp(ErrCode, "34")) *errorCode = ECB_PRNERR_PRINTPACKEND;
				if (!strcmp(ErrCode, "37")) *errorCode = ECB_PRNERR_SCANNEROPEN;
				if (!strcmp(ErrCode, "38")) *errorCode = ECB_PRNERR_CDRGUIDEOPEN;
				if (!strcmp(ErrCode, "44")) *errorCode = ECB_PRNERR_CDRGUIDEOPEN;
				if (!strcmp(ErrCode, "45")) *errorCode = ECB_PRNERR_CDREXIST_MAINTE;
			}

			return ECB_PRNST_ERROR;
		}
#if DEBUG
		/* Debug */
		fprintf(stderr, "status code = %s\n", StsCode);
#endif

		*errorCode = ECB_PRNERR_NOERROR;

		if (!strcmp(StsCode, "01")) return ECB_PRNST_BUSY;
		if (!strcmp(StsCode, "02")) return ECB_PRNST_PRINTING;
		if (!strcmp(StsCode, "03")) return ECB_PRNST_BUSY;
		if (!strcmp(StsCode, "04")) return ECB_PRNST_IDLE;
		if (!strcmp(StsCode, "07")) return ECB_PRNST_BUSY;
		/* if(!strcmp(StsCode, "08")) return ECB_PRNERR_FACTORY; */
		if (!strcmp(StsCode, "0A")) return ECB_PRNST_BUSY;

		if (!strcmp(StsCode, "CN")) return ECB_DAEMON_PRINTER_RESET;
	}

	return ECB_DAEMON_NO_REPLY;
}

static void connect_daemon()
{
	/* Renewal of ink residual quantity */
	printer_status = ReadStatuslogfile(&ink_list, &error_code);
	ink_list_delete(ink_list);
	return;
}


static void
ink_list_delete(InkList list)
{
	InkNode *node;

	while (list) {
		node = list->next;
		free(list);
		list = node;
	}
	return;
}

/* -----------------------------------------------------------------------------*/
/*             Definition of API Functions                                      */
/*                                                                              */
/* -----------------------------------------------------------------------------*/


ECB_PRINTER_STS Get_Status(ECB_STATUS* status)
{

	int i;
	printer_status = ECB_DAEMON_NO_ERROR;
	ink_num = 0;
	for (i = 0; i< ECB_INK_NUM; i++) {
		colors[i] = 0;
		inklevel[i] = 0;
	}

	ECB_PRINTER_STS ret = ECB_DAEMON_NO_ERROR;

	connect_daemon();

	status->printerStatus = printer_status;
	status->errorCode = error_code;

	if (printer_status == ECB_DAEMON_NO_REPLY) return ECB_DAEMON_NO_REPLY;

	status->ink_num = ink_num;

	for (i = 0; i< ink_num; i++) {
		status->colors[i] = colors[i];
		status->inklevel[i] = inklevel[i];

		switch (status->inklevel[i]) {
		case 'w':
		case 'r':
			status->inkstatus[i] = ECB_INK_ST_FAIL;
			break;
		case 'n':
			status->inkstatus[i] = ECB_INK_ST_NOTPRESENT;
			break;
		case 'i':
			status->inkstatus[i] = ECB_INK_ST_NOREAD;
			break;
		case 'g':
			status->inkstatus[i] = ECB_INK_ST_NOTAVAIL;
			break;
		default:
			if (status->inklevel[i] > 100 || status->inklevel[i] < 0)
			{
				status->inkstatus[i] = ECB_INK_ST_FAIL;
			}
			else {

				status->inklevel[i] = serInkLevelNromalize(status->inklevel[i]);

				if (status->inklevel[i] == 0) {
					if (ECB_PRNERR_INKOUT == status->errorCode) {
						status->inkstatus[i] = ECB_INK_ST_END;
					}
				}
				else if (status->inklevel[i] >= 1 && status->inklevel[i] >= INKLOW) {
					status->inkstatus[i] = ECB_INK_ST_LOW;
				}
				else {
					status->inkstatus[i] = ECB_INK_ST_NORMAL;
				}

			}
		}
	}

	status->paper_count.color = papercount.color;
	status->paper_count.monochrome = papercount.monochrome;
	status->paper_count.blank = papercount.blank;
	status->paper_count.adf = papercount.adf;
	status->paper_count.color_borderless = papercount.color_borderless;
	status->paper_count.monochrome_borderless = papercount.monochrome_borderless;

	status->showInkInfo = showInkInfo;
	status->showInkLow = showInkLow;

	return ret;
}