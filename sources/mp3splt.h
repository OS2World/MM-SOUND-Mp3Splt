/*
 * Mp3Splt -- Utliity for mp3/ogg splitting without decoding
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
 * Tested on: GNU/Linux 2.4.20 #2 Mon Mar 17 22:02:15 PST 2003 i686
 */

#ifndef _MP3SPLT_H
#define _MP3SPLT_H

#define NAME "Mp3Splt"
#define VER "2.1"
#define YEAR "(2004/Sep/28)"
#define AUTHOR "Matteo Trotta"
#define EMAIL "<matteo.trotta@lib.unimib.it>"
#define WEBSITE "http://mp3splt.sf.net"

#define SEP "_"
#define EOF_STRING "EOF"

#define DEFAULT_GAP 30

typedef struct {
  struct id3 id;
  float splitpoints[2*MAXTRACKS+1];
  unsigned char fn[MAXTRACKS+1][256];
  unsigned char performer[MAXTRACKS+1][128];
  char format[OUTNUM+1][MAXOLEN];
  mp3_state *mstate;
#ifndef NO_OGG
  ogg_state *ostate;
#endif
} splt_state;

splt_state *splt_state_new (splt_state *state);

void splt_state_free (splt_state *state);

#endif

