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
*/
#ifndef __EPSON_WRAPPER_H__
#define __EPSON_WRAPPER_H__

int start_ecbt_engine(void);
int end_ecbt_engine(void);
int open_port_driver(P_CBTD_INFO);
int close_port_driver(P_CBTD_INFO);

int open_port_channel(P_CBTD_INFO, char);
int close_port_channel(P_CBTD_INFO, char);
int write_to_prt(P_CBTD_INFO, char, char*, int*);
int read_from_prt(P_CBTD_INFO, char, char*, int*);

#endif /* __EPSON_WRAPPER_H__ */
