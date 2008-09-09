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

#include "estring.h"

estring es_init()
{
	estring temp_es;
	temp_es.data=malloc(sizeof(estring_data));
	(*temp_es.data).len=0;
	(*temp_es.data).ptr=(char*)malloc(1);
	(*(*temp_es.data).ptr)='\0';
	return(temp_es);
}

estring es_init_set(char* value)
{
	estring temp_es;
	temp_es=es_init();
	es_set(temp_es,value);
	return(temp_es);
}

char* es_get(estring temp_es)
{
	return((*temp_es.data).ptr);
}

char es_getchar(estring temp_es,int pos)
{
	return((char)*((*temp_es.data).ptr+pos-1));
}

int es_len(estring temp_es)
{
	return((*temp_es.data).len);
}

void es_free(estring temp_es)
{
	free((*temp_es.data).ptr);
	free(temp_es.data);
}

void es_set(estring temp_es,char* value)
{
	(*temp_es.data).len=strlen(value);
	free((*temp_es.data).ptr);
	
	(*temp_es.data).ptr=(char*)malloc((*temp_es.data).len+1);
	strcpy((*temp_es.data).ptr,value);
}

void es_setchar(estring temp_es,int pos,char value)
{
	*((*temp_es.data).ptr+pos-1)=value;
}

void es_append(estring temp_es,char* value)
{
	(*temp_es.data).len+=strlen(value);
	(*temp_es.data).ptr=(char*)realloc((*temp_es.data).ptr,(*temp_es.data).len+1);	
	strcat((*temp_es.data).ptr,value);
}

void es_appendchar(estring temp_es,char value)
{
	(*temp_es.data).len++;
	(*temp_es.data).ptr=(char*)realloc((*temp_es.data).ptr,(*temp_es.data).len+1);	
	*((*temp_es.data).ptr+(*temp_es.data).len-1)=value;
	*((*temp_es.data).ptr+(*temp_es.data).len)='\0';
}

void es_insert(estring temp_es,int pos,char* value)
{
	(*temp_es.data).len+=strlen(value);
	(*temp_es.data).ptr=(char*)realloc((*temp_es.data).ptr,(*temp_es.data).len+1);	
	memmove((*temp_es.data).ptr+pos-1+strlen(value),(*temp_es.data).ptr+pos-1,(*temp_es.data).len-pos-strlen(value)+1);
	memcpy((*temp_es.data).ptr+pos-1,value,strlen(value));
	*((*temp_es.data).ptr+(*temp_es.data).len)='\0';
}

void es_delete(estring temp_es,int pos, int len)
{
	memmove((*temp_es.data).ptr+pos-1,(*temp_es.data).ptr+pos-1+len,(*temp_es.data).len-pos-len+1);
	(*temp_es.data).len-=len;
	(*temp_es.data).ptr=(char*)realloc((*temp_es.data).ptr,(*temp_es.data).len+1);	
	*((*temp_es.data).ptr+(*temp_es.data).len)='\0';
}

void es_deletechar(estring temp_es,int pos)
{
	es_delete(temp_es,pos,1);
}

int es_toint(estring temp_es)
{
	return(atoi((*temp_es.data).ptr));
}

void es_fromint(estring temp_es,int value)
{
	char buffer[64];
	snprintf(buffer,sizeof(buffer),"%d",value);
	es_set(temp_es,buffer);
}

void es_fwriteline(FILE* stream,estring temp_es)
{
	fputs((*temp_es.data).ptr,stream);
	fputs("\n",stream);
}

void es_writeline(estring temp_es)
{
	es_fwriteline(stdout,temp_es);
}

void es_readline(estring temp_es)
{
	es_freadline(stdin,temp_es);
}

int es_freadline(FILE* stream,estring temp_es)
{
	char t;
	if(feof(stream))
		return(0);
	es_set(temp_es,"");
	while((t=(char)fgetc(stream))!='\n' && !feof(stream))
		es_appendchar(temp_es,t);	
	return(1);
}

int es_find(estring temp_es,int start,char* target)
{
	int i;
	for(i=start;i<=(*temp_es.data).len-strlen(target)+1;i++)
	{
		if(strncmp((*temp_es.data).ptr+i-1,target,strlen(target))==0)
			return(i);
	}
	return(0);
}

int es_replaceall(estring temp_es,char* target, char* replacewith)
{
	int i=0;
	int found=0;
	if(strcmp(target,replacewith)==0)
		return(0);
	while((i=es_find(temp_es,i+1,target)))
	{
		found++;
		es_delete(temp_es,i,strlen(target));
		es_insert(temp_es,i,replacewith);
		i--;
	}
	return(found);
	/*note: this function might be hanged if trying to replace for example
	* " " with " asd", this will be fixed in the future.
	*/
}

int es_removeall(estring temp_es,char* target)
{
	int i=0;
	int found=0;
	while((i=es_find(temp_es,i+1,target)))
	{
		found++;
		es_delete(temp_es,i,strlen(target));
		i--;
	}
	return(found);
}

void es_getsubestring(estring src, int start, int len, estring dst)
{
	(*dst.data).len=len;
	free((*dst.data).ptr);
	(*dst.data).ptr=(char*)malloc((*dst.data).len+1);
	strncpy((*dst.data).ptr,(*src.data).ptr+start-1,len);
	*((*dst.data).ptr+(*dst.data).len)='\0';
}

void es_getleft(estring src,int len,estring dst)
{
	es_getsubestring(src,1,len,dst);	
}

void es_getright(estring src,int len,estring dst)
{
	es_getsubestring(src,es_len(src)-len+1,len,dst);	
}
