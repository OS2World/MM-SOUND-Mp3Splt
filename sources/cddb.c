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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include <conio.h>
#ifdef __OS2__
#include <netinet\in.h>
#include <sys\socket.h>
#include <netdb.h>
#define closesocket(x) soclose(x)
#define WSACleanup();
#define floorf(x) floor(x)
#else
#include <winsock.h>
#endif

#ifndef NO_OGG
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#include "mp3.h"
#include "ogg.h"
#include "splt.h"
#include "mp3splt.h"
#include "cddb.h"

static char alphabet [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Base64 Algorithm: Base64.java v. 1.3.6 by Robert Harder
 * Ported and optimized for C by Matteo Trotta
 */

char *encode3to4 (unsigned char *source, int srcoffset, int num, char *destination, int destoffset)
{

    int inbuff=(num>0?(source[srcoffset]<<16):0)|(num>1?(source[srcoffset+1]<<8):0)|(num>2?(source[srcoffset+2]):0);
    switch(num)
    {
	case 3:
	    destination[destoffset] = alphabet[(inbuff>>18)];
	    destination[destoffset+1] = alphabet[(inbuff>>12)&0x3f];
	    destination[destoffset+2] = alphabet[(inbuff>>6)&0x3f];
	    destination[destoffset+3] = alphabet[(inbuff)&0x3f];
	    return destination;

	case 2:
	    destination[destoffset] = alphabet[(inbuff>>18)];
	    destination[destoffset+1] = alphabet[(inbuff>>12)&0x3f];
	    destination[destoffset+2] = alphabet[(inbuff>>6)&0x3f];
	    destination[destoffset+3] = '=';
	    return destination;

	case 1:
	    destination[destoffset] = alphabet[(inbuff>>18)];
	    destination[destoffset+1] = alphabet[(inbuff>>12)&0x3f];
	    destination[destoffset+2] = '=';
	    destination[destoffset+3] = '=';
	    return destination;
	default:
	    return destination;
    }
}

char *b64 (unsigned char *source, int len)
{
	char *out;
	int d, e=0;
	d = ((len*4/3)+((len%3)>0?4:0));
	if ((out = malloc(d+1))==NULL) {
		perror("malloc");
		exit(1);
	}
	memset(out, 0x00, d+1);
	for(d=0;d<(len-2);d+=3,e+=4)
		out = encode3to4(source, d, 3, out, e);
	if(d<len)
		out = encode3to4(source, d, len-d, out, e);

	return out;
}
// End of Base64 Algorithm

// getpass() function only for Windows sources
#ifndef __OS2__
char *getpass (char *s)
{
  char *pass, c;
  int i=0;
  fputs(s, stdout);
  pass = malloc(100);
  do {
    c = getch();
    if (c!='\r') {
       if (c=='\b') {
	 if (i>0) {
	    printf ("\b \b");
	    i--;
	 }
       }
       else {
	 printf ("*");
	 pass[i++] = c;
       }
    }
    else break;
  } while (i<100);

   pass[i]='\0';

   printf("\n");

  return pass;
}
#endif

int checkstring (unsigned char *s)
{
	int i;
	for (i=0; i<strlen(s); i++)
		if ((isalnum(s[i])==0)&&(s[i]!=0x20)) {
			fprintf (stderr, " Error: '%c' is not allowed!\n", s[i]);
			return -1;
		}
	return 0;
}

char *get_cue_value(char *in, char *out, int maxlen)
{
	char *ptr_b, *ptr_e;
	ptr_b = strchr(in, '"');
	if (ptr_b==NULL)
		return NULL;
	ptr_e = strchr(ptr_b+1, '"');
	if (ptr_e==NULL)
		return NULL;
	*ptr_e='\0';
	strncpy(out, ptr_b, maxlen);
	cleanstring(out);
	return out;
}

int get_cue_splitpoints(unsigned char *file, splt_state *state)
{
	FILE *file_input;
	char line[512];
	char *ptr, *dot;
	int tracks = -1, check=1;

        ptr = dot = NULL;

	if (!(file_input=fopen(file, "r"))) {
		perror(file);
		exit(1);
	}

	fseek (file_input, 0, SEEK_SET);

	while (fgets(line, 512, file_input)!=NULL)
	{
		int type=0;

		if ((strstr(line, "TRACK")!=NULL)&&(strstr(line, "AUDIO")!=NULL))
			type=1;
		else if (strstr(line, "TITLE")!=NULL)
			type=2;
			else if (strstr(line, "PERFORMER")!=NULL)
				type=3;
				else if ((ptr=strstr(line, "INDEX 01"))!=NULL)
					type=4;

		switch (type)
		{
			case 0:	break;
			case 1:	if (tracks==-1) tracks = 0;
						if (check) tracks++;
						else return -1;
						check=0;
					  	break;
			case 2:	if (tracks==-1)
					{
						if (get_cue_value(line, state->id.album, 30)==NULL)
							return -1;
					}
					else
					{
						if (tracks > 0)
								if (get_cue_value(line, state->fn[tracks], 255)==NULL)
									return -1;
					}
					break;
			case 3:	if (tracks==-1)
					{
						if (get_cue_value(line, state->id.artist, 30)==NULL)
							return -1;
					}
					else
					{
						if (tracks>0)
							if (get_cue_value(line, state->performer[tracks], 127)==NULL)
								return -1;
					}
					break;
			case 4:	line[strlen(line)-1]='\0';
						ptr += 9;
                                                if ((dot = strchr(ptr, ':'))==NULL)
                                                	return -1;
						ptr[dot-ptr] = ptr[dot-ptr+3] = '.';
						cleanstring(ptr);
						if (tracks>0)
						{
							float seconds = c_seconds(ptr);
							if (seconds==-1)
								return -1;
							state->splitpoints[tracks-1] = floorf(seconds);
							seconds = (seconds - state->splitpoints[tracks-1])*4/3;
							state->splitpoints[tracks-1] += seconds;
							check=1;
						}
						break;
			default:	break;
		}
	}

	if (!check) tracks--;

	if (tracks>0) state->splitpoints[tracks] = (float) -1.f; // End of file

	fclose(file_input);

	return tracks;
}

int get_cddb_splitpoints (unsigned char *file, splt_state *state)
{

	FILE *file_input;
	char line[512];
	char prev[10];
	char *number, *c;
	int tracks = 0, i, j;

	if (!(file_input=fopen(file, "r"))) {
		perror(file);
		exit(1);
	}

	fseek (file_input, 0, SEEK_SET);
	do {
		if ((fgets(line, 512, file_input))==NULL)
			return -1;
		number = strstr(line, "Track frame offset");
	} while (number == NULL);

	memset(prev, 0x0, 10);

	do {
		if ((fgets(line, 512, file_input))==NULL)
			return -1;
		line[strlen(line)-1] = '\0';
		i = 0;
		while ((isdigit(line[i])==0) && (line[i]!='\0')) {
			i++;
			number = line + i;
		}
		if (number == (line + strlen(line))) break;
		else state->splitpoints[tracks++] = (float) atof (number);
	} while (tracks<MAXTRACKS);
	for (i=tracks-1; i>=0; i--) {
		state->splitpoints[i] -= state->splitpoints[0];
		state->splitpoints[i] /= 75;
	}
	state->splitpoints[tracks] = (float) -1.f; // End of file

	j=0;
	do {
		char temp[10];
		memset(temp, 0x0, 10);
		if ((fgets(line, 512, file_input))==NULL)
			return -1;
		line[strlen(line)-1] = '\0';
		if (strlen(line)>0)
			if (line[strlen(line)-1]=='\r') line[strlen(line)-1]='\0';
		if (j==0) {
			if (strstr(line, "DTITLE")==NULL) continue;
		}
		else
			if (strstr(line, "TTITLE")==NULL)
				continue;
		if ((number=strchr(line, '='))==NULL) return -1;
		if (j>0) {
			int len = number-line;
			if (len>10) len = 10;
			strncpy(temp, line, len);
			if ((c=strchr(number, '/'))!=NULL) number = c + 1;
			number = cleanstring (number);
		}
		if (strlen(++number)>255) number[255]='\0';
		if ((j>0)&&(strstr(number, "Data")!=NULL) && (strstr(number, "Track")!=NULL)) {
			state->splitpoints[j-1]=state->splitpoints[j];
			tracks -= 1;
		}
		else
		{
			if ((j>0)&&(strcmp(temp, prev)==0))
				strncat(state->fn[j-1], number, 255-strlen(state->fn[j-1]));
			else strncpy(state->fn[j++], number, 255);
		}
		strncpy(prev, temp, 10);
	} while (j<=tracks);

	while ((fgets(line, 512, file_input))!=NULL) {
		line[strlen(line)-1] = '\0';
		if (strlen(line)>0)
			if (line[strlen(line)-1]=='\r') line[strlen(line)-1]='\0';
		if (strstr(line, "EXTD")==NULL) continue;
		else {
			if ((number=strchr(line, '='))==NULL) break;
			else {
				if ((c=strstr(number, "YEAR"))!=NULL)
					strncpy(state->id.year, c+6, 4);
				if ((c=strstr(number, "ID3G"))!=NULL) {
					strncpy(line, c+6, 3);
					state->id.genre= (unsigned char) atoi(line);
				}
				break;
			}
		}
	}

	i=0;
	while ((state->fn[0][i]!='/') && (state->fn[0][i]!='\0')&&(i<30)) {
		state->id.artist[i] = state->fn[0][i];
		i++;
	}
	state->id.artist[i-1]='\0';
	cleanstring(state->id.artist);
	j=0;
	i += 2;
	do
		state->id.album[j++]=state->fn[0][i++];
	while ((state->fn[0][i]!='\0')&&(j<30));

	fclose(file_input);

	return tracks;
}

int find_cd (char *buf, int size, cd_state *state)
{
	char *c, *add;
	int i;
	if (size<30) return 0; // 30 is a safe value for preventing buffer exceed
	add = buf+size;
	buf = strstr(buf, "cat=");
	if (buf==NULL) return 0;
	else {
		if (state->foundcd==0) fprintf (stdout, "List of found cd:\n");
		do {
			i=0;
			buf += 4;
			if ((c=strchr(buf, '&'))==NULL)
					return -1;
			if (c==buf)	continue;
			memset(state->discs[state->foundcd].category, 0x00, 20);
			strncpy(state->discs[state->foundcd].category, buf, c - buf);
			buf=c+4;
			memset(state->discs[state->foundcd].discid, 0x00, 9);
			strncpy(state->discs[state->foundcd].discid, buf, DISCIDLEN);
			buf = buf + DISCIDLEN + 2;
			if ((c=strchr(buf, '<'))==NULL)
					return -1;
			else if (c == buf) {
				if ((buf=strchr(buf, '>'))==NULL)
						return -1;
				buf++;
				if ((c=strchr(buf, '<'))==NULL)
					return -1;
				i=-1;
			}
			if (i==-1) fprintf (stdout, "  |\\=>");
			fprintf (stdout, "%3d) ", state->foundcd);
			if (i==-1) fprintf (stdout, "Revision: ");
			while ((buf<c)&&(buf<add)) putchar(*buf++);
			if (((state->foundcd+1)%22)==0) {
				char junk[18];
				fprintf (stdout, "\n-- 'q' to select cd, Enter for more: ");
				fgets(junk, 16, stdin);
				if (junk[0]=='q') {
					state->foundcd++;
					return -2;
				}
			}
			else fprintf(stdout, "\n");
			state->foundcd++;
		} while ((buf<(add-30)) && (state->foundcd<MAXCD) && ((buf=strstr(buf, "cat="))!=NULL));
		return state->foundcd;
	}
}

char *login (char *s)
{
	char *pass, junk[130];
	printf ("Username: ");
	fgets(junk, 128, stdin);
	junk[strlen(junk)-1]='\0';
	pass = getpass("Password: ");
	sprintf (s, "%s:%s", junk, pass);
	memset (pass, 0x00, strlen(pass));
	free (pass);
	return s;
}

struct addr useproxy(FILE *in, struct addr dest)
{

	char line[270];
	char *ptr;

	dest.proxy=0;
	memset(dest.hostname, 0, 256);
	memset(line, 0, 270);

	if (in != NULL) {

		fseek(in, 0, SEEK_SET);

		if (fgets(line, 266, in)!=NULL) {
			if (strstr(line, "PROXYADDR")!=NULL) {
				line[strlen(line)-1]='\0';
				if ((ptr = strchr(line, '='))!=NULL) {
					ptr++;
					strncpy(dest.hostname, ptr, 255);
				}

				if (fgets(line, 266, in)!=NULL) {
					if (strstr(line, "PROXYPORT")!=NULL) {
						line[strlen(line)-1]='\0';
						if ((ptr = strchr(line, '='))!=NULL) {
							ptr++;
							dest.port = atoi (ptr);
							dest.proxy=1;
						}
						fprintf (stderr, "Using Proxy: %s on Port %d\n", dest.hostname, dest.port);
						if (fgets(line, 266, in)!=NULL) {
							if (strstr(line, "PROXYAUTH")!=NULL) {
								line[strlen(line)-1]='\0';
								if ((ptr = strchr(line, '='))!=NULL) {
									ptr++;
									if (ptr[0]=='1') {
										if (fgets(line, 266, in)!=NULL) {
											dest.auth = malloc(strlen(line)+1);
											if (dest.auth==NULL) {
												perror("malloc");
												exit(1);
											}
											memset(dest.auth, 0x0, strlen(line)+1);
											strncpy(dest.auth, line, strlen(line));
										}
										else {
											login(line);
											dest.auth = b64(line, strlen(line));
											memset(line, 0x00, strlen(line));
										}
									}
								}
							}
						}
						else dest.auth = NULL;
					}
				}
			}
		}
	}

	if (!dest.proxy) {
		strncpy(dest.hostname, FREEDB, 255);
		dest.port = PORT1;
	}

	return dest;
}

int search_freedb (splt_state *state, char *s)
{

	int i, tot=0;
	char buffer[BUFFERSIZE], message[1024], junk[255];
	char *c, *e=NULL;
	FILE *output = NULL;
	struct sockaddr_in host;
	struct hostent *h;
	struct addr dest;
	cd_state *cdstate;

#ifdef __OS2__
        int fd;
#else
	long winsockinit;
	WSADATA winsock;
	SOCKET fd;

	winsockinit = WSAStartup(0x0101,&winsock);
#endif

	memset (message, 0x00, 1024);

	// In Windows version file will be created in the same dir where program is
	if ((c=strstr(s, "MP3SPLT.EXE"))!=NULL) strncpy (message, s, (c-s));
	strncat(message, PROXYCONFIG, strlen(PROXYCONFIG));

	if (!(output=fopen(message, "r"))) {
		if (!(output=fopen(message, "w+"))) {
			fprintf(stderr, "\nWARNING Can't open config file ");
			perror(message);
		}
		else {
			fprintf (stderr, "Will you use a proxy? (y/n): ");
			fgets(junk, 100, stdin);
			if (junk[0]=='y') {
				fprintf (stderr, "Proxy Address: ");
				fgets(junk, 100, stdin);
				fprintf (output, "PROXYADDR=%s", junk);
				fprintf (stderr, "Proxy Port: ");
				fgets(junk, 100, stdin);
				fprintf (output, "PROXYPORT=%s", junk);
				fprintf (stderr, "Need authentication? (y/n): ");
				fgets(junk, 100, stdin);
				if (junk[0]=='y') {
					fprintf (output, "PROXYAUTH=1\n");
					fprintf (stderr, "Would you like to save password (insecure)? (y/n): ");
					fgets(junk, 100, stdin);
					if (junk[0]=='y') {
						login (message);
						e = b64(message, strlen(message));
						fprintf (output, "%s\n", e);
						memset(message, 0x00, strlen(message));
						memset(e, 0x00, strlen(e));
						free(e);
					}
				}
			}
		}
	}

	dest = useproxy(output, dest);

	if (output != NULL)
		fclose(output);

	do {
		printf ("\n\t____________________________________________________________]");
		printf ("\r Search: [");
		fgets(junk, 155, stdin);
		junk[strlen(junk)-1]='\0';
	} while ((strlen(junk)==0)||(checkstring(junk)!=0));

	i=0;
	while ((junk[i]!='\0')&&(i<155))
		if (junk[i]==' ') junk[i++]='+';
		else i++;

	printf ("\nConnecting to %s...\n", dest.hostname);
	if((h=gethostbyname(dest.hostname))==NULL) {
		perror(dest.hostname);
		exit(1);
	}

	memset(&host, 0x00, sizeof(host));
	host.sin_family=AF_INET;
	host.sin_addr.s_addr=((struct in_addr *) (h->h_addr)) ->s_addr;

	host.sin_port=htons(dest.port);

	if((fd=socket(AF_INET, SOCK_STREAM, 0))==-1) {
		perror("socket");
		exit(1);
	}
	if ((connect(fd, (void *)&host, sizeof(host)))==-1) {
		perror("connect");
		exit(1);
	}

	if (dest.proxy) {
		sprintf(message,"GET http://www.freedb.org"SEARCH" "PROXYDLG, junk);
		if (dest.auth!=NULL)
			sprintf (message, "%s"AUTH"%s\n", message, dest.auth);
		strncat(message, "\n", 1);
	}
	else sprintf(message,"GET "SEARCH"\n", junk);

	if((send(fd, message, strlen(message), 0))==-1) {
		perror("send");
		exit(1);
	}
	printf ("Host contacted. Waiting for answer...\n");

	memset(buffer, 0x00, BUFFERSIZE);

	if ((cdstate = (cd_state *) malloc (sizeof(cd_state)))==NULL) {
		perror("malloc");
		exit(1);
	}

	cdstate->foundcd = 0;

	do {
		tot=0;
		c = buffer;
		do {
			i = recv(fd, c, BUFFERSIZE-(c-buffer)-1, 0);
			if (i==-1) break;
			tot += i;
			buffer[tot]='\0';
			c += i;
		} while ((i>0)&&(tot<BUFFERSIZE-1)&&((e=strstr(buffer, "</html>"))==NULL));

		tot = find_cd(buffer, tot, cdstate);
		if (tot==-1) continue;
		if (tot==-2) break;
	} while ((i>0)&&(e==NULL)&&(cdstate->foundcd<MAXCD));

	closesocket(fd);

	if (cdstate->foundcd<=0) {
		if (dest.proxy) {
			if (strstr(buffer, "HTTP/1.0")!=NULL) {
				if ((c = strchr (buffer, '\n'))!=NULL)
					buffer[c-buffer]='\0';
				printf ("Proxy Reply: %s\n", buffer);
			}
		}
		if (cdstate->foundcd==0) return -1;
		if (cdstate->foundcd==-1) return -2;
	}
	if (cdstate->foundcd==MAXCD) fprintf (stderr, "\nMax cd number reached, this search may be too generic.\n");

	fprintf (stdout, "\n");

	do {
		i=0;
		printf ("Select cd #: ");
		fgets(message, 254, stdin);
		message[strlen(message)-1]='\0';
		tot=0;
		if (message[tot]=='\0') i=-1;
		while(message[tot]!='\0')
			if (isdigit(message[tot++])==0) {
				printf ("Please ");
				i=-1;
				break;
			}
		if (i!=-1) i = atoi (message);
	} while ((i>=cdstate->foundcd) || (i<0));
	state->id.genre = getgenre(cdstate->discs[i].category);
	if (dest.proxy) {
		sprintf(message, "GET "FREEDBHTTP"cmd=cddb+read+%s+%s&hello=nouser+mp3splt.net+"NAME"+"VER"&proto=5 "PROXYDLG, cdstate->discs[i].category, cdstate->discs[i].discid);
		if (dest.auth!=NULL) {
			sprintf (message, "%s"AUTH"%s\n", message, dest.auth);
			memset(dest.auth, 0x00, strlen(dest.auth));
			free(dest.auth);
		}
		strncat(message, "\n", 1);
	}
	else {
		sprintf(message, "CDDB READ %s %s\n", cdstate->discs[i].category, cdstate->discs[i].discid);
		host.sin_port=htons(PORT2);
	}

	printf ("\nContacting "FREEDB" to query selected cd...\n");

	if((fd=socket(AF_INET, SOCK_STREAM, 0))==-1) {
		perror("socket");
		exit(1);
	}
	if ((connect(fd, (void *)&host, sizeof(host)))==-1) {
		perror("connect");
		exit(1);
	}
	if (!dest.proxy) {
		i=recv(fd, buffer, BUFFERSIZE-1, 0);
		buffer[i]='\0';

		if (strncmp(buffer,"201",3)!=0)  return -4;
		if((send(fd, HELLO, strlen(HELLO), 0))==-1) {
			perror("send");
			exit(1);
		}
		i=recv(fd, buffer, BUFFERSIZE-1, 0);
		buffer[i]='\0';
		if (strncmp(buffer,"200",3)!=0)  return -4;
	}

	if((send(fd, message, strlen(message), 0))==-1) {
		perror("send");
		exit(1);
	}

	printf ("Host contacted. Waiting for answer...\n");

	memset(buffer, 0x00, BUFFERSIZE);
	c = buffer;
	tot=0;
	do {
		i = recv(fd, c, BUFFERSIZE-(c-buffer)-1, 0);
		if (i==-1) break;
		tot += i;
		buffer[tot]='\0';
		c += i;
	} while ((i>0)&&(tot<BUFFERSIZE-1)&&((e=strstr(buffer, "\n."))==NULL));

	if (!dest.proxy)
		if((send(fd, "quit\n", 5, 0))==-1) {
			perror("send");
			exit(1);
		}

	closesocket(fd);
	WSACleanup();

	if (tot==0) return -4;

	if (e!=NULL)
		buffer[e-buffer+1]='\0';

	if ((strstr(buffer, "database entry follows"))==NULL) {
		if ((c = strchr (buffer, '\n'))!=NULL)
			buffer[c-buffer]='\0';
		fprintf (stderr, "Invalid server answer: %s\n", buffer);
		return -5;
	}
	else {
		if ((c = strchr (buffer, '#'))==NULL)
			return -5;
		if (!(output=fopen(CDDBFILE, "w"))) {
			perror(CDDBFILE);
			exit(1);
		}
		fprintf (output, c);
		fclose(output);
	}

	printf ("OK, "CDDBFILE" has been written.\n");

	free(cdstate);

	return 0;

}
