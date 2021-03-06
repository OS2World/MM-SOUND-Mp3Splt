/*
 * Mp3Splt -- Utility for mp3 splitting without decoding
 *
 * WIN32 Version
 *
 * Copyright (c) 2002-2004 M. Trotta - <matteo.trotta@lib.unimib.it>
 *
 * http://mp3splt.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Tested on: Linux 2.4.18 #4 Fri May 31 01:25:31 PDT 2002 i686
 */

#ifndef _MP3SPLT_CDDB_H
#define _MP3SPLT_CDDB_H

#define MAXCD 512
#define DISCIDLEN 8
#define BUFFERSIZE 8192
#define CDDBFILE "query.cddb"
#define PROXYCONFIG "mp3splt.ini"

#define FREEDB "freedb.org"
#define PORT1 80
#define PORT2 8880

#define FREEDBHTTP "http://www.freedb.org/~cddb/cddb.cgi?"

#define SEARCH "/freedb_search.php?words=%s&allfields=NO&fields=artist&fields=title&allcats=YES&grouping=none"
#define PROXYDLG "HTTP/1.0\nUserAgent: "NAME"/"VER"\n"
#define AUTH "Proxy-Authorization: Basic "
#define HELLO "CDDB HELLO nouser mp3splt.net "NAME" "VER"\n"

struct addr {
	short proxy;
	char hostname[256];
	int port;
	char *auth;
};

struct cd {
	char discid[DISCIDLEN+1];
	char category[20];
};

typedef struct {
	struct cd discs[MAXCD];
	int foundcd;
} cd_state;

char *encode3to4 (unsigned char *source, int srcoffset, int num, char *destination, int destoffset);

char *b64 (unsigned char *source, int len);

int checkstring (unsigned char *s);

int get_cue_splitpoints(unsigned char *file, splt_state *state);

int get_cddb_splitpoints (unsigned char *file, splt_state *state);

int find_cd (char *buf, int size, cd_state *state);

char *login (char *s);

struct addr useproxy(FILE *in, struct addr dest);

int search_freedb (splt_state *state, char *s);
// Only WIN32
#ifndef __OS2__
char *getpass (char *s);
#endif

#endif

