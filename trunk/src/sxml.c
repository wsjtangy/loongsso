/*****************************************************************************
 *
 * FILE:	sxml.c
 * DESCRIPTION:	Skimpy XML Parser/Grafter Library
 * DATE:	Wed, Sep  8 2004
 * UPDATED:	Fri, Aug 31 2007
 * AUTHOR:	Kouichi ABE (WALL) / ∞§…ÙπØ∞Ï
 * E-MAIL:	kouichi@MysticWALL.COM
 * URL:		http://www.MysticWALL.COM/
 * COPYRIGHT:	(c) 2004-2007 ∞§…ÙπØ∞Ï°øKouichi ABE (WALL), All rights reserved.
 * LICENSE:
 *
 *  Copyright (c) 2004-2007 Kouichi ABE (WALL) <kouichi@MysticWALL.COM>,
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *   SUCH DAMAGE.
 *
 * $Id: sxml.c,v 1.7 2007/08/31 09:57:44 kouichi Exp $
 *
 *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "sxml.h"

/******************************************************************************
 *
 *	Macros and structures definition
 *
 *****************************************************************************/
#ifndef	_BOOL_T
#define	_BOOL_T
typedef enum {
  false	= 0,
  true	= 1
} bool;
#endif	/* _BOOL_T */

#define	EOL	'\0'
#define	HTAB	'\011'
#define	LF	'\012'
#define	VTAB	'\013'
#define	NP	'\014'
#define	CR	'\015'
#define	SPC	'\040'

/******************************************************************************
 *
 *	Local functions declaration
 *
 *****************************************************************************/
static char *		strxsep(char **, const char *);
static int		whitespace(const char *);
static char *		mkstr(const char *, const char *);
static sxml_node_t *	create_new_node(sxml_node_t *);
static void		delete_node(sxml_node_t *);
static sxml_attr_t *	set_attribute(sxml_attr_t *, const char *,
				      const char *);
static int		set_element(sxml_element_t *, char *);
static void		delete_element(sxml_element_t *);
static int		parse(sxml_node_t *, const char *, size_t);
static int		mkval(sxml_node_t * node, const char * vp,
			      const char * p);

static void		print_node(sxml_node_t *, int, FILE *);

/******************************************************************************
 *
 *	Functions definition
 *
 *****************************************************************************/
static char *
strxsep(stringp, delim)
	register char **	stringp;
	register const char *	delim;
{
  register char *	s;
  register const char *	spanp;
  register int		c;
  register int		sc;
  char *		tok;

  if ((s = *stringp) == NULL) {
    return NULL;
  }
  for (tok = s; ; ) {
    c	  = *s++;
    spanp = delim;
    if (c == '"') {
      register char *	p;

      for (p = s; *s != '"'; s++) {
	if (*s == '\0') {
	  s = p;	/* reset */
	  break;
	}
      }
      if (*s == '"') {
	c = *s++;
      }
    }
    do {
      if ((sc = *spanp++) == c) {
	if (c == '\0') {
	  s = NULL;
	}
	else {
	  s[-1] = '\0';
	}
	*stringp = s;
	return tok;
      }
    } while (sc != '\0');
  }
  /* NOTREACHED */
}

static int
whitespace(s)
	const char *	s;
{
  int	c = 0;

  while (*s == SPC || *s == HTAB || *s == VTAB || *s == CR || *s == LF ||
	 *s == NP) {
    c++;
    s++;
  }

  return c;
}

static char *
mkstr(sp, ep)
	const char *	sp;
	const char *	ep;
{
  if (sp != NULL && ep != NULL) {
    size_t	len;

    len = ep - sp;
    if (len > 0) {
      char *	new;

      /* strip white space of string end */
      while (*(sp + len - 1) == CR   || *(sp + len - 1) == LF ||
	     *(sp + len - 1) == SPC  ||
	     *(sp + len - 1) == HTAB || *(sp + len - 1) == VTAB) {
	len--;
      }
      new = (char *)calloc(len + 1, sizeof(char)); 
      if (new != NULL) {
	memcpy(new, sp, len);
      }
      return new;
    }
  }

  return NULL;
}

static sxml_node_t *
create_new_node(parent)
	sxml_node_t *	parent;
{
  sxml_node_t *	new;

  new = (sxml_node_t *)calloc(1, sizeof(sxml_node_t));
  if (new != NULL) {
    new->parent	    = parent;
    new->child	    = NULL;	/* first child */
    new->last_child = NULL;	/* last child */
    new->next	    = NULL;	/* sibling */
    new->prev	    = NULL;	/* sibling */
    new->type	    = SXML_ELEMENT;
  }

  return new;
}

static void
delete_node(node)
	sxml_node_t *	node;
{
  register sxml_node_t *	np;
  register sxml_node_t *	np_next;

  for (np = node; np != NULL; np = np_next) {
    np_next = np->next;
    switch (np->type) {
      case SXML_ELEMENT:
	delete_element(&np->value.element);
	break;
      case SXML_CONTENT:
	free((char *)np->value.content);	np->value.content = NULL;
      default:
	break;
    }
    if (np->child != NULL)  { delete_node(np->child); }
    free(np); np = NULL;
  }
}

static sxml_attr_t *
set_attribute(ap, name, value)
	sxml_attr_t *	ap;
	const char *	name;
	const char *	value;
{
  sxml_attr_t *	new;

  new = (sxml_attr_t *)malloc(sizeof(sxml_attr_t));
  if (new != NULL) {
    new->name	= strdup(name);
    new->value	= strdup(value);
    new->next	= ap;
  }

  return new;
}

static int
set_element(e, s)
	sxml_element_t *	e;	/* element */
	char *			s;	/* element string */
{
  static const char	sep[] = " \t\r\n";
  char *		name;
  register char *	attr;

  name	   = strxsep(&s, sep);
  e->name  = strdup(name);
  e->attrs = NULL;

  for (attr = strxsep(&s, sep); attr != NULL; attr = strxsep(&s, sep)) {
    register char *	p;
    register char *	q;

    if (*attr == EOL) { continue; }
    p = strchr(attr, '=');
    if (p) {
      *p = EOL;
      p += 2;
      q	 = strchr(p, '"');
      if (q != NULL) {
	*q = EOL;
	e->attrs = set_attribute(e->attrs, attr, p);
      }
    }
  }

  return 0;
}

static void
delete_element(e)
	sxml_element_t *	e;
{
  free((char *)e->name);	e->name = NULL;
  if (e->attrs != NULL) {
    register sxml_attr_t *	ap;
    register sxml_attr_t *	ap_next;

    for (ap = e->attrs; ap != NULL; ap = ap_next) {
      ap_next = ap->next;
      free((char *)ap->name);	ap->name  = NULL;
      free((char *)ap->value);	ap->value = NULL;
      free((sxml_attr_t *)ap);	ap	  = NULL;
    }
  }
}

static int
parse(vertex, text, size)
	sxml_node_t *	vertex;
	const char *	text;
	size_t		size;
{
  typedef enum {
    ST_INIT,
    ST_TAG_BEGIN,
    ST_TAG_END,
    ST_PROLOG_BEGIN,
    ST_PROLOG_END,
    ST_COMMENT_BEGIN,
    ST_COMMENT_S1,
    ST_COMMENT,
    ST_COMMENT_S2,
    ST_COMMENT_END,
    ST_CTEXT,
    ST_CDATA,
    ST_ELEMENT,
    ST_ATTRIBUTE,
    ST_VALUE
  } state_t;
  const char *	p     = text;
  const char *	vp    = NULL;	/* start position of value */
  state_t	state = ST_INIT;
  sxml_node_t *	node  = vertex;	/* current node */
  const char *	endp  = text + size;
  static bool	prolog = false;

  while (p < endp) {
    p += whitespace(p);
    switch (state) {
      case ST_INIT:
	if	(*p == '<') { state = ST_TAG_BEGIN; }
	else if (p == endp) { return  0; }
	else		    { return -1; }
	break;
      case ST_TAG_BEGIN:
	if	(*p == '?') { state = ST_PROLOG_BEGIN; }
	else if (*p == '!') { state = ST_COMMENT_BEGIN; }
	else if (*p == '/') { state = ST_TAG_END; }
	else		    { state = ST_ELEMENT; p--; }
	break;
      case ST_TAG_END:
	if (*p == '>')	{ state = ST_INIT;
			  if (node->parent != NULL) { node = node->parent; }
			}
	break;
      case ST_PROLOG_BEGIN:
	if (*p < 'a' || *p > 'z') { return -2; }
	else			  { state = ST_ELEMENT; p--; continue; }
	break;
      case ST_PROLOG_END:
	if (*p == '>')	{ state = ST_INIT; prolog = false; }
	else		{ return -3; }
	break;
      case ST_COMMENT_BEGIN:
	if (*p == '-')			 { state = ST_COMMENT_S1; }
	else if (*p >= 'A' || *p <= 'Z') { state = ST_CTEXT; }
	else				 { return -4; }
	break;
      case ST_COMMENT_S1:
	if (*p == '-')	{ state = ST_COMMENT; }
	else		{ return -5; }
	break;
      case ST_COMMENT:
	if (*p == '-') { state = ST_COMMENT_S2; }
	break;
      case ST_COMMENT_S2:
	if (*p == '-')	{ state = ST_COMMENT_END; }
	else		{ state = ST_COMMENT; }
	break;
      case ST_COMMENT_END:
	if (*p == '>')	{ state = ST_INIT; }
	else		{ state = ST_COMMENT; }
	break;
      case ST_CTEXT:
	if (*p == '>')	{ state = ST_INIT; }
	break;
      case ST_ELEMENT: {
	  state_t	nest1 = ST_ELEMENT;
	  const char *	sp    = p;	/* start position of element */
	  char *	s;		/* element with attributes */

	  while (p < endp) {
	    switch (nest1) {
	      case ST_ELEMENT:
		if	(*p == '"')  { nest1 = ST_ATTRIBUTE; }
		else if (*p == '>')  { state = ST_VALUE; vp = NULL; goto end; }
		else if (*p == '/')  { state = ST_TAG_END; goto end; }
		else if (*p == '?') {
		  if (prolog)	{  state = ST_PROLOG_END; goto end; }
		  else		{ prolog = true; sp = ++p; /* remove '?' */ }
		}
		break;
	      case ST_ATTRIBUTE:
		if (*p == '"') { nest1 = ST_ELEMENT; }
		break;
	      default:
		break;
	    }
	    p++;
	  }
	end:
	  s = mkstr(sp, p);
	  if (s != NULL) {
	    sxml_node_t *	n;	/* new node */

	    n = create_new_node(node);	if (n == NULL) { return -6; }
	    set_element(&n->value.element, s);
	    if (node->child != NULL) {
	      node->last_child->next = n;
	      n->prev		     = node->last_child;
	      node->last_child	     = n;
	    }
	    else {
	      node->last_child = node->child = n;
	    }
	    if (prolog) {	/* prolog has no children */
	      n->type = SXML_PROLOG;
	    }
	    else {
	      node = n;	/* set new node to current node */
	    }
	    free(s);	s = NULL;
	  }
	}
	break;
      case ST_VALUE:
	if (strncmp(p, "<![CDATA[", 9) == 0) { state = ST_CDATA; p += 8; }
	else if (*p == '<') {
	  if (mkval(node, vp, p) == 0)	{ state = ST_TAG_BEGIN; }
	  else				{ return -7; }
	}
	else if (vp == NULL) { vp = p; }
	break;
      case ST_CDATA:
	if (strncmp(p, "]]>", 3) == 0) {
	  if (mkval(node, vp, p) == 0)	{ state = ST_INIT; p += 2; }
	  else				{ return -8; }
	}
	else if (vp == NULL) { vp = p; }
	break;
      default:
	return -9;
    }
    p++;
  }

  return 0;
}

static int
mkval(node, vp, p)
	sxml_node_t *	node;
	const char *	vp;
	const char *	p;
{
  char *	s;

  s = mkstr(vp, p);
  if (s != NULL) {
    sxml_node_t *	n;	/* new node */
			     
    n = create_new_node(node);	if (n == NULL) { return -1; }
    n->type	     = SXML_CONTENT;
    n->value.content = s;
    node->child	     = node->last_child = n;
  }

  return 0;
}

/*****************************************************************************/

sxml_node_t *
sxml_parse_file(fd)
	int	fd;	/* XML file descriptor */
{
  sxml_node_t *	root = NULL;	/* root node */

  if (fd != -1) {
    struct stat	sbuf;

    if (fstat(fd, &sbuf) == 0) {
      void *	md;

      md = mmap(0, (size_t)sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, (off_t)0);
      if (md != MAP_FAILED) {
	root   = create_new_node(NULL); root->type = SXML_VERTEX;
	if (root != NULL) {
	  int	status;

	  status = parse(root, (const char *)md, (size_t)sbuf.st_size);
	  if (status != 0) {
#if	DEBUG
	    fprintf(stderr, "DEBUG[sxml_parse_file] status=%d\n", status);
#endif	/* DEBUG */
	    delete_node(root);
	    root = NULL;
	  }
	}
	munmap(md, (size_t)sbuf.st_size);
      }
    }
  }

  return root;
}

void
sxml_delete_node(node)
	sxml_node_t *	node;
{
  delete_node(node);
}

sxml_node_t *
sxml_find_prolog(node, name)
	sxml_node_t *	node;	/* start node to find element */
	const char *	name;	/* element name */
{
  register sxml_node_t *	np;

  for (np = node; np != NULL; np = np->next) {
    if (np->type == SXML_PROLOG) {
      if (name == NULL) { return np; }	/* first matched */
      if (strcmp(name, np->value.element.name) == 0) {
	return np;
      }
    }
    /* descend child */
    if (np->child != NULL) {
      sxml_node_t *	child;

      child = sxml_find_prolog(np->child, name);
      if (child != NULL) {
	return child;
      }
    }
  }

  return NULL;
}

sxml_node_t *
sxml_find_element(node, name, attr, value)
	sxml_node_t *	node;	/* start node to find element */
	const char *	name;	/* element name */
	const char *	attr;	/* attribute name */
	const char *	value;	/* attribute value */
{
  struct found_s {
    sxml_node_t *	name;
    sxml_node_t *	attr;
    sxml_node_t *	value;
  };
  register sxml_node_t *	np;

  for (np = node; np != NULL; np = np->next) {
    struct found_s	found = { NULL, NULL, NULL };

    if (np->type == SXML_ELEMENT) {
      register sxml_attr_t *	ap;

      if (name != NULL && strcmp(name, np->value.element.name) == 0) {
	found.name = np;
      }
      for (ap = np->value.element.attrs; ap != NULL; ap = ap->next) {
	if (attr != NULL && ap->name && strcmp(attr, ap->name) == 0) {
	  found.attr = np;
	}
	if (value != NULL && ap->value && strcmp(value, ap->value) == 0) {
	  found.value = np;
	}

	if (attr != NULL && value != NULL) {
	  if (found.attr != NULL && found.value != NULL) { break; }
	}
	else if (attr  != NULL) { if (found.attr  != NULL) { break; } }
	else if (value != NULL) { if (found.value != NULL) { break; } }
      }

      if (name != NULL && attr != NULL && value != NULL) {
	if (found.name != NULL && found.attr != NULL && found.value != NULL) {
	  return np;
	}
      }
      else if (name != NULL && attr != NULL) {
	if (found.name != NULL && found.attr != NULL) { return np; }
      }
      else if (name != NULL && value != NULL) {
	if (found.name != NULL && found.value != NULL) { return np; }
      }
      else if (attr != NULL && value != NULL) {
	if (found.attr != NULL && found.value != NULL) { return np; }
      }
      else if (name != NULL) {
	if (found.name != NULL) { return np; }
      }
      else if (attr != NULL) {
	if (found.attr != NULL) { return np; }
      }
      else if (value != NULL) {
	if (found.value != NULL) { return np; }
      }
    }

    /* descend child */
    if (np->child != NULL) {
      sxml_node_t *	child;

      child = sxml_find_element(np->child, name, attr, value);
      if (child != NULL) {
	return child;
      }
    }
  }

  return NULL;
}

const char *
sxml_get_attribute(node, name)
	sxml_node_t *	node;	/* prolog/element node */
	const char *	name;	/* name of attribute */
{
  if (node->type == SXML_ELEMENT || node->type == SXML_PROLOG) {
    if (node->value.element.attrs != NULL) {
      register sxml_attr_t *	ap;

      for (ap = node->value.element.attrs; ap != NULL; ap = ap->next) {
	if (ap->name != NULL && strcmp(name, ap->name) == 0) {
	  return ap->value;
	}
      }
    }
  }

  return NULL;
}

const char *
sxml_get_content(node)
	sxml_node_t *	node;
{
  return (node != NULL && node->type == SXML_CONTENT) ?
	  node->value.content : NULL;
}

/***************************************************************************** 
 *
 *****************************************************************************/
void
sxml_print_tree(node, fout)
	sxml_node_t *	node;
	register FILE *	fout;
{
  if (fout != NULL) {
    print_node(node, node->type == SXML_VERTEX ? -1 : 0, fout);
    fflush(fout);
  }
}

static void
print_node(node, depth, fout)
	sxml_node_t *	node;
	int		depth;
	register FILE *	fout;
{
  register sxml_node_t *	np;

  for (np = node; np; np = np->next) {
    switch (np->type) {
      case SXML_PROLOG: {
	  register sxml_attr_t *	ap;

	  fprintf(fout, "<?%s", np->value.element.name);
	  for (ap = np->value.element.attrs; ap != NULL; ap = ap->next) {
	    fprintf(fout, " %s=\"%s\"", ap->name, ap->value);
	  }
	  fputs("?>\n", fout);
	}
	break;
      case SXML_ELEMENT: {
	  register int			i;
	  register sxml_attr_t *	ap;

          for (i = 0; i < depth; i++) {
	    fputs("  ", fout);
	  }
	  fprintf(fout, "<%s", np->value.element.name);
	  for (ap = np->value.element.attrs; ap != NULL; ap = ap->next) {
	    fprintf(fout, " %s=\"%s\"", ap->name, ap->value);
	  }
	  fputs(">", fout);
	  if (np->child && np->child->type == SXML_ELEMENT) {
	    fputs("\n", fout);
	  }
	}
	break;
      case SXML_CONTENT:
	fprintf(fout, "%s", np->value.content);
      default:
	break;
    }
    if (np->child)  { print_node(np->child, depth + 1, fout); }
    if (np->type == SXML_ELEMENT) {
      if (np->child && np->child->type == SXML_ELEMENT) {
	register int	i;

	for (i = 0; i < depth; i++) {
	  fputs("  ", fout);
	}
      }
      fprintf(fout, "</%s>\n", np->value.element.name);
    }
  }
}

void
sxml_print_node(node, fout)
	sxml_node_t *	node;
	register FILE *	fout;
{
  if (node != NULL) {
    switch (node->type) {
      case SXML_ELEMENT: {
  	  register sxml_attr_t *	ap;

  	  fprintf(fout, "<%s", node->value.element.name);
  	  for (ap = node->value.element.attrs; ap != NULL; ap = ap->next) {
  	    fprintf(fout, " %s=\"%s\"", ap->name, ap->value);
  	  }
  	  fprintf(fout, ">\n");
   	}
    	break;
      case SXML_CONTENT:
      	fprintf(fout, "%s\n", node->value.content);
      default:
  	  break;
    }
    fflush(fout);
  }
}

/***************************************************************************** 
 *
 *****************************************************************************/

sxml_node_t *
sxml_new_vertex(void)
{
  sxml_node_t *	new;

  new = create_new_node(NULL);
  if (new != NULL) {
    new->type = SXML_VERTEX;
  }

  return new;
}

sxml_node_t *
sxml_new_prolog(parent, name)
	sxml_node_t *	parent;
	const char *	name;	/* name of prolog */
{
  sxml_node_t *	new;	/* node of new element */

  new = create_new_node(parent);
  if (new != NULL) {
    new->value.element.name	= strdup(name);
    new->value.element.attrs	= NULL;
    new->type			= SXML_PROLOG;

    if (parent->child != NULL) {	/* parent has children */
      new->next			= parent->child;
      parent->child->prev	= new;
      parent->child		= new;
    }
    else {	/* I'm fist child of the parent */
      parent->last_child = parent->child = new;
    }
  }

  return new;
}

sxml_node_t *
sxml_new_element(parent, name)
	sxml_node_t *	parent;
	const char *	name;	/* name of element */
{
  sxml_node_t *	new;	/* node of new element */

  new = create_new_node(parent);
  if (new != NULL) {
    new->value.element.name	= strdup(name);
    new->value.element.attrs	= NULL;

    if (parent->child != NULL) {	/* parent has children */
      parent->last_child->next	= new;
      new->prev			= parent->last_child;
      parent->last_child	= new;
    }
    else {	/* I'm fist child of the parent */
      parent->last_child = parent->child = new;
    }
  }

  return new;
}

sxml_node_t *
sxml_set_content(element, content)
	sxml_node_t *	element;
	const char *	content;	/* content of element */
{
  sxml_node_t *	new;	/* content */

  new = create_new_node(element);
  if (new != NULL) {
    new->type		= SXML_CONTENT;
    new->value.content	= strdup(content);
    element->child	= element->last_child = new;
  }

  return new;
}

int
sxml_set_attribute(node, name, value)
	sxml_node_t *	node;
	const char *	name;
	const char *	value;
{
  if (node->type == SXML_ELEMENT || node->type == SXML_PROLOG) {
    sxml_attr_t *	ap;

    ap = set_attribute(node->value.element.attrs, name, value);
    if (ap != NULL) {
      node->value.element.attrs = ap;
      return 0;
    }
  }

  return -1;
}

int
sxml_set_fattribute(
	sxml_node_t *	node,
	const char *	name,
	const char *	fmt,
	...)
{
  sxml_attr_t *	ap = NULL;

  if (node->type == SXML_ELEMENT || node->type == SXML_PROLOG) {
    char *	val;

#define	MAGIC_SIZE	(4*1024)	/* XXX: groundless value */
    val = calloc(MAGIC_SIZE, sizeof(char));
    if (val != NULL) {
      va_list	vp;

      va_start(vp, fmt);
      vsnprintf(val, MAGIC_SIZE, fmt, vp);
      va_end(vp);

      ap = set_attribute(node->value.element.attrs, name, val);
      if (ap != NULL) {
       	node->value.element.attrs = ap;
      }
      free(val);
    }
  }

  return (ap != NULL ? 0 : -1);
}

sxml_node_t *
sxml_set_node(parent, name, content)
	sxml_node_t *	parent;
	const char *	name;		/* name of element */
	const char *	content;	/* content of element */
{
  sxml_node_t *	new;

  new = sxml_new_element(parent, name);
  if (new != NULL) {
    sxml_set_content(new, content);
  }

  return new;
}

sxml_node_t *
sxml_set_fnode(
	sxml_node_t *	parent,
	const char *	name,	/* name of element */
	const char *	fmt,	/* format for content of element */
	...)
{
  sxml_node_t *	new = NULL;
  char *	val;

  val = calloc(MAGIC_SIZE, sizeof(char));
  if (val != NULL) {
    va_list	vp;

    va_start(vp, fmt);
    vsnprintf(val, MAGIC_SIZE, fmt, vp);
    va_end(vp);

    new = sxml_new_element(parent, name);
    if (new != NULL) {
      sxml_set_content(new, val);
    }
    free(val);
  }

  return new;
}
