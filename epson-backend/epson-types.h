/*
 * Copyright (C) Seiko Epson Corporation 2016.
 *
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

#ifndef __EPSON_TYPES_H__
#define __EPSON_TYPES_H__

typedef void* HANDLE;

/*------------------------------- Define Basic Data Types ------------------------------*/
/*******************************************|********************************************/
typedef unsigned char  EPS_UINT8;       /* unsigned  8-bit  Min: 0          Max: 255        */
typedef unsigned short EPS_UINT16;      /* unsigned 16-bit  Min: 0          Max: 65535      */
typedef unsigned int   EPS_UINT32;      /* unsigned 32-bit  Min: 0          Max: 4294967295 */
typedef char           EPS_INT8;        /*   signed  8-bit  Min: -128       Max: 127        */
typedef short          EPS_INT16;       /*   signed 16-bit  Min: -32768     Max: 32767      */
typedef int            EPS_INT32;       /*   signed 32-bit  Min:-2147483648 Max: 2147483647 */
typedef float          EPS_FLOAT;       /*    float 32-bit  Min:3.4E-38     Max: 3.4E+38    */
typedef EPS_INT32      EPS_BOOL;        /* Boolean type                                     */
typedef EPS_INT32      EPS_ERR_CODE;    /* Error code for API's and routines                */
typedef int            EPS_SOCKET;      /* socket discripter                                */

/*------------------------------- Define Basic Data Types ------------------------------*/
/*******************************************|********************************************/
typedef unsigned char  ECB_UINT8;       /* unsigned  8-bit  Min: 0          Max: 255        */
typedef unsigned short ECB_UINT16;      /* unsigned 16-bit  Min: 0          Max: 65535      */
typedef unsigned int   ECB_UINT32;      /* unsigned 32-bit  Min: 0          Max: 4294967295 */
typedef char           ECB_INT8;        /*   signed  8-bit  Min: -128       Max: 127        */
typedef short          ECB_INT16;       /*   signed 16-bit  Min: -32768     Max: 32767      */
typedef int            ECB_INT32;       /*   signed 32-bit  Min:-2147483648 Max: 2147483647 */
typedef float          ECB_FLOAT;       /*    float 32-bit  Min:3.4E-38     Max: 3.4E+38    */
typedef ECB_INT32      ECB_BOOL;        /* Boolean type                                     */
typedef ECB_INT32      ECB_ERR_CODE;    /* Error code for API's and routines                */
typedef int            ECB_SOCKET;      /* socket discripter                                */


typedef enum
{
  /* from lib-l2/epson-escpr-err.h: enum EPS_PRINTER_STATUS */
  EPS_PRNST_IDLE                                 ,    /* Idle (Enable Start Job)      */
  EPS_PRNST_PRINTING                             ,    /* Printing                     */
  /* EPS_PRNST_OFFLINE                           ,    Offline                      */
  EPS_PRNST_BUSY                                 ,    /* Busy (Disable Start Job)     */
  EPS_PRNST_CANCELLING                           ,    /* Cancellation processing      */
  EPS_PRNST_ERROR                                ,    /* Printer has an error         */

  /* from lib-l2/epson-escpr-err.h: enum EPS_PRINTER_ERROR */
  EPS_PRNERR_NOERROR                             ,
  EPS_PRNERR_GENERAL                             ,
  EPS_PRNERR_FATAL                               ,
  EPS_PRNERR_INTERFACE                           ,
  EPS_PRNERR_COVEROPEN                           ,
/*  EPS_PRNERR_LEVERPOSITION                       ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_PAPERJAM                            ,
  EPS_PRNERR_INKOUT                              ,
  EPS_PRNERR_PAPEROUT                            ,
/*  EPS_PRNERR_INITIALIZESETTING                   ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_UNKNOWN                             ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERCHANGE_UNCOMP                  ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERSIZE                           ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_RIBBONJAM                           ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_SIZE_TYPE_PATH                      ,
/*  EPS_PRNERR_PAPERTHICKLEVER                     ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERFEED                           ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_SIMMCOPY                            ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_SERVICEREQ                          ,    /* EPS_PRNERR_INKOVERFLOW1      */
/*  EPS_PRNERR_WAITTEAROFFRETURN                   ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_DOUBLEFEED                          ,
/*  EPS_PRNERR_HEADHOT                             ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERCUTMIS                         ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_HOLDLEVERRELEASE                    ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_NOT_CLEANING                        ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERCONFIG                         ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_PAPERSLANT                          ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_CLEANINGNUMOVER                     ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_INKCOVEROPEN                        ,
/*  EPS_PRNERR_LFP_INKCARTRIDGE                    ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_CUTTER                              ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_CUTTERJAM                           ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_INKCOLOR                            ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_CUTTERCOVEROPEN                     ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_LFP_INKLEVERRELEASE                 ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_LFP_NOMAINTENANCETANK1              ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_CARTRIDGECOMBINATION                ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_LFP_COMMAND                         ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_LEARCOVEROPEN                       ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_MULTICENSORGAIN                     ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_NOT_AUTOADJUST                      ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_FAILCLEANING                        ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_NOTRAY                              ,
  EPS_PRNERR_CARDLOADING                         ,
  EPS_PRNERR_CARTRIDGEOVERFLOW                   ,
/*  EPS_PRNERR_LFP_NOMAINTENANCETANK2              ,*/  /* Not supported by 2006 Model  */
/*  EPS_PRNERR_INKOVERFLOW2                        ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_BATTERYVOLTAGE                      ,
  EPS_PRNERR_BATTERYTEMPERATURE                  ,
  EPS_PRNERR_BATTERYEMPTY                        ,
  EPS_PRNERR_SHUTOFF                             ,
  EPS_PRNERR_NOT_INITIALFILL                     ,
  EPS_PRNERR_PRINTPACKEND                        ,
/*  EPS_PRNERR_ABNORMALHEAT                        ,*/  /* Not supported by 2006 Model  */
  EPS_PRNERR_SCANNEROPEN                         ,
  EPS_PRNERR_CDRGUIDEOPEN                        ,
  /* append for 2008 Model  */
  EPS_PRNERR_CDDVDCONFIG                         ,
  EPS_PRNERR_CDREXIST_MAINTE                     ,
  /* Status Error                                                                     */
  EPS_PRNERR_BUSY                                 ,
  EPS_PRNERR_FACTORY                              ,
  /* Communication Error                                                              */
  EPS_PRNERR_COMM                                 ,
  /* Ink Error                                                                        */
  EPS_PRNERR_CEMPTY                               ,
  EPS_PRNERR_CFAIL                                ,
  /* Printer Condition                                                                */
  EPS_PRNERR_TRAYCLOSE                            ,
  EPS_PRNERR_CDGUIDECLOSE                         ,   /* CDR guide close              */
/*    EPS_PRNERR_OVERHEAT                                  OVERHEAT is not an error     */
  EPS_PRNERR_JPG_LIMIT                            ,   /* Jpeg print data size limit   */
  EPS_PRNERR_DISABEL_CLEANING                     ,    /* can not start Head Cleaning  */
}PRINTER_STS;

typedef enum
{
  /* from lib-l2/epson-escpr-err.h: enum EPS_PRINTER_ERROR */
  ECB_PRNERR_NOERROR = 0                         ,
  ECB_PRNERR_GENERAL                             ,
  ECB_PRNERR_FATAL                               ,
  ECB_PRNERR_INTERFACE                           ,
  ECB_PRNERR_COVEROPEN                           ,
/*  ECB_PRNERR_LEVERPOSITION                       ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_PAPERJAM                            ,
  ECB_PRNERR_INKOUT                              ,
  ECB_PRNERR_PAPEROUT                            ,
/*  ECB_PRNERR_INITIALIZESETTING                   ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_UNKNOWN                             ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERCHANGE_UNCOMP                  ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERSIZE                           ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_RIBBONJAM                           ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_SIZE_TYPE_PATH                      ,
/*  ECB_PRNERR_PAPERTHICKLEVER                     ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERFEED                           ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_SIMMCOPY                            ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_SERVICEREQ                          ,    /* EPS_PRNERR_INKOVERFLOW1      */
/*  ECB_PRNERR_WAITTEAROFFRETURN                   ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_DOUBLEFEED                          ,
/*  ECB_PRNERR_HEADHOT                             ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERCUTMIS                         ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_HOLDLEVERRELEASE                    ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_NOT_CLEANING                        ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERCONFIG                         ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_PAPERSLANT                          ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_CLEANINGNUMOVER                     ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_INKCOVEROPEN                        ,
/*  ECB_PRNERR_LFP_INKCARTRIDGE                    ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_CUTTER                              ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_CUTTERJAM                           ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_INKCOLOR                            ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_CUTTERCOVEROPEN                     ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_LFP_INKLEVERRELEASE                 ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_LFP_NOMAINTENANCETANK1              ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_CARTRIDGECOMBINATION                ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_LFP_COMMAND                         ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_LEARCOVEROPEN                       ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_MULTICENSORGAIN                     ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_NOT_AUTOADJUST                      ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_FAILCLEANING                        ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_NOTRAY                              ,
  ECB_PRNERR_CARDLOADING                         ,
  ECB_PRNERR_CARTRIDGEOVERFLOW                   ,
/*  ECB_PRNERR_LFP_NOMAINTENANCETANK2              ,*/  /* Not supported by 2006 Model  */
/*  ECB_PRNERR_INKOVERFLOW2                        ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_BATTERYVOLTAGE                      ,
  ECB_PRNERR_BATTERYTEMPERATURE                  ,
  ECB_PRNERR_BATTERYEMPTY                        ,
  ECB_PRNERR_SHUTOFF                             ,
  ECB_PRNERR_NOT_INITIALFILL                     ,
  ECB_PRNERR_PRINTPACKEND                        ,
/*  ECB_PRNERR_ABNORMALHEAT                        ,*/  /* Not supported by 2006 Model  */
  ECB_PRNERR_SCANNEROPEN                         ,
  ECB_PRNERR_CDRGUIDEOPEN                        ,
  /* append for 2008 Model  */
  ECB_PRNERR_CDDVDCONFIG                         ,
  ECB_PRNERR_CDREXIST_MAINTE                     ,
  /* Status Error                                                                     */
  ECB_PRNERR_BUSY    = 100                        ,
  ECB_PRNERR_FACTORY                              ,
  /* Communication Error                                                              */
  ECB_PRNERR_COMM                                 ,
  /* Ink Error                                                                        */
  ECB_PRNERR_CEMPTY                               ,
  ECB_PRNERR_CFAIL                                ,
  /* Printer Condition                                                                */
  ECB_PRNERR_TRAYCLOSE                            ,
  ECB_PRNERR_CDGUIDECLOSE                         ,   /* CDR guide close              */
/*    ECB_PRNERR_OVERHEAT                                  OVERHEAT is not an error     */
  ECB_PRNERR_JPG_LIMIT                            ,   /* Jpeg print data size limit   */
  ECB_PRNERR_DISABEL_CLEANING                     ,    /* can not start Head Cleaning  */

}ECB_PRINTER_ERR;

typedef enum
{
  STS_NO_ERROR,
  STS_DAEMON_DOWN = 100,         /* add sk -Mon Mar 12 2001 */
  STS_NO_REPLY,
  STS_PRINTER_RESET,
  STS_DEFAULT
}DAEMON_STS;

typedef enum
{
	/* from lib-l2/epson-escpr-err.h: enum EPS_PRINTER_STATUS */
	ECB_PRNST_IDLE = 0,    /* Idle (Enable Start Job)      */
	ECB_PRNST_BUSY,    /* Busy (Disable Start Job)     */
	ECB_PRNST_PRINTING,    /* Printing                     */
	ECB_PRNST_CANCELLING,    /* Cancellation processing      */
	ECB_PRNST_ERROR,    /* Printer has an error         */

	ECB_DAEMON_NO_ERROR = 0,
	ECB_DAEMON_DOWN = 100,         /* add sk -Mon Mar 12 2001 */
	ECB_DAEMON_NO_REPLY,
	ECB_DAEMON_PRINTER_RESET,
	ECB_DAEMON_DEFAULT

}ECB_PRINTER_STS;

#define ECB_INK_NUM		20
#define INKLOW           5
typedef struct _tagECB_PAPER_COUNT_ {
	EPS_INT32	color;
	EPS_INT32	monochrome;
	EPS_INT32	blank;
	EPS_INT32	adf;
	EPS_INT32	color_borderless;
	EPS_INT32	monochrome_borderless;
} ECB_PAPER_COUNT;

typedef struct _tagECB_STATUS_ {
	EPS_INT32		printerStatus;
	EPS_INT32		errorCode;
	EPS_INT32		ink_num;
	EPS_INT32		colors[ECB_INK_NUM];
	EPS_INT32		inklevel[ECB_INK_NUM];
	EPS_INT32		inkstatus[ECB_INK_NUM];
	ECB_PAPER_COUNT paper_count;
	EPS_BOOL		showInkInfo;
	EPS_BOOL		showInkLow;
} ECB_STATUS;


/*** Ink Cartridge Status                                                           */
/*** -------------------------------------------------------------------------------*/
#define ECB_INK_ST_NORMAL                        (0)
#define ECB_INK_ST_LOW                           (1)
#define ECB_INK_ST_END                           (2)
#define ECB_INK_ST_NOTPRESENT                    -1
#define ECB_INK_ST_FAIL                          -2
#define ECB_INK_ST_NOTAVAIL                      -3
#define ECB_INK_ST_NOREAD                        -4


#define ECB_NAME_BUFFSIZE  64

#endif /* __EPSON_TYPES_H__ */        


