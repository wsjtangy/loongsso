/*
*This program is free software; you can redistribute it and/or modify
*it under the terms of the GNU Lesser General Public License as published by
*the Free Software Foundation; either version 2.1 of the License, or
*(at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU Lesser General Public License for more details.
*
*You should have received a copy of the GNU Lesser General Public License
*along with this program; if not, write to the Free Software
*Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ESTRING_H
#define ESTRING_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct estring_data_struct
{
	char* ptr;
	int len;
};
typedef struct estring_data_struct estring_data;

struct estring_struct
{
	estring_data* data;	
};
typedef struct estring_struct estring;

estring es_init();
estring es_init_set(char* value);
char* es_get(estring temp_es);
char es_getchar(estring temp_es,int pos);
int es_len(estring temp_es);
void es_free(estring temp_es);
void es_set(estring temp_es,char* value);
void es_setchar(estring temp_es,int pos,char value);
void es_append(estring temp_es,char* value);
void es_appendchar(estring temp_es,char value);
void es_insert(estring temp_es,int pos,char* value);
void es_delete(estring temp_es,int pos, int len);
void es_deletechar(estring temp_es,int pos);
int es_toint(estring temp_es);
void es_fromint(estring temp_es,int value);
void es_fwriteline(FILE* stream,estring temp_es);
void es_writeline(estring temp_es);
int es_freadline(FILE* stream,estring temp_es);
void es_readline(estring temp_es);
int es_find(estring temp_es,int start,char* target);
int es_replaceall(estring temp_es,char* target, char* replacewith);
int es_removeall(estring temp_es,char* target);
void es_getsubestring(estring src, int start, int len, estring dst);
void es_getleft(estring src,int len,estring dst);
void es_getright(estring src,int len,estring dst);
#endif
