#ifndef	_SXML_H
#define	_SXML_H

#include <stdio.h>
#include <stdarg.h>

/******************************************************************************
 *
 *	Macros and structures definition
 *
 *****************************************************************************/
/*
 * Numeric release version identifier:
 * MNNFFPPS: major minor fix patch status
 * The status nibble has one of the values 0 for development,
 * 1 to e for betas 1 to 14, and f for release.
 * The patch level is exactly that.
 */
#define	SXML_VERSION_NUMBER	0x10005000L
#define	SXML_VERSION		"SXML/1.0.5"
#define	SXML_VERSION_TEXT	SXML_VERSION " (2007/08/31)"
#define	SXML_VERSION_TEXT_LONG	"Skimpy XML Library 1.0.5, Fri, Aug 31 2007"

typedef enum {	/* XML node type */
  SXML_VERTEX,	/* root node */
  SXML_PROLOG,	/* XML prolog */
  SXML_ELEMENT,	/* XML element with attributes */
  SXML_CONTENT	/* content of element */
} sxml_type_t;

typedef struct _sxml_attr {	/* XML element attribute */
  const char *		name;
  const char *		value;
  struct _sxml_attr *	next;
} sxml_attr_t;

typedef struct {	/* XML element */
  const char *	name;	/* name of element */
  sxml_attr_t *	attrs;	/* attributes */
} sxml_element_t;

typedef union {				/* XML value union */
  sxml_element_t	element;	/* element */
  const char *		content;	/* content */
} sxml_value_t;

typedef struct _sxml_node {		/* XML node structure */
  struct _sxml_node *	parent;		/* parent node */
  struct _sxml_node *	child;		/* first child node */
  struct _sxml_node *	last_child;	/* last child node (role of sentry) */
  struct _sxml_node *	next;		/* next node under same parent */
  struct _sxml_node *	prev;		/* previous node under same parent */
  sxml_type_t		type;		/* node type */
  sxml_value_t		value;		/* node value */
} sxml_node_t;

/*****************************************************************************/

#define	sxml_get_type(x)		((x)->type)
#define	sxml_get_child(x)		((x)->child)
#define	sxml_get_next_sibling(x)	((x)->next)
#define	sxml_get_prev_sibling(x)	((x)->prev)
#define	sxml_get_element_name(x)	((x)->value.element.name)

/******************************************************************************
 *
 *	Global functions declaration
 *
 *****************************************************************************/
/* parse */
sxml_node_t *	sxml_parse_file(int fd);
void		sxml_delete_node(sxml_node_t * node);
sxml_node_t *	sxml_find_prolog(sxml_node_t * node, const char * name);
sxml_node_t *	sxml_find_element(sxml_node_t * node, const char * name,
				  const char * attr, const char * value);
const char *	sxml_get_attribute(sxml_node_t * node, const char * name);
const char *	sxml_get_content(sxml_node_t * node);
/* print */
void		sxml_print_tree(sxml_node_t * node, FILE * fout);
void		sxml_print_node(sxml_node_t * node, FILE * fout);
/* graft */
sxml_node_t *	sxml_new_vertex(void);
sxml_node_t *	sxml_new_prolog(sxml_node_t * parent, const char * name);
sxml_node_t *	sxml_new_element(sxml_node_t * parent, const char * name);
sxml_node_t *	sxml_set_content(sxml_node_t * element, const char * content);
int		sxml_set_attribute(sxml_node_t * node, const char * name,
				   const char * value);
int		sxml_set_fattribute(sxml_node_t * node, const char * name,
				    const char * fmt, ...);
sxml_node_t *	sxml_set_node(sxml_node_t * node, const char * name,
			      const char * content);
sxml_node_t *	sxml_set_fnode(sxml_node_t * node, const char * name,
			       const char * fmt, ...);

#endif	/* _SXML_H */
