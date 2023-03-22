/*
 * Copyright (C) 2002-2004 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: 057e230e7823400e3b022b1621aa8384a4e4671f $
 *
 * a parser for real's asm rules
 *
 * grammar for these rules:
 *

   rule_book  = { rule }
   rule       = ( '#' condition { ',' assignment } | [ assignment {',' assignment} ]) ';'
   assignment = id '=' const
   const      = ( number | string )
   condition  = comp_expr { ( '&&' | '||' ) comp_expr }
   comp_expr  = operand { ( '<' | '<=' | '==' | '>=' | '>' ) operand }
   operand    = ( '$' id | num | '(' condition ')' )

 */

#include "real.h"

#define ASMRP_SYM_NONE         0
#define ASMRP_SYM_EOF          1

#define ASMRP_SYM_NUM          2
#define ASMRP_SYM_ID           3
#define ASMRP_SYM_STRING       4

#define ASMRP_SYM_HASH         10
#define ASMRP_SYM_SEMICOLON    11
#define ASMRP_SYM_COMMA        12
#define ASMRP_SYM_EQUALS       13
#define ASMRP_SYM_AND          14
#define ASMRP_SYM_OR           15
#define ASMRP_SYM_LESS         16
#define ASMRP_SYM_LEQ          17
#define ASMRP_SYM_GEQ          18
#define ASMRP_SYM_GREATER      19
#define ASMRP_SYM_DOLLAR       20
#define ASMRP_SYM_LPAREN       21
#define ASMRP_SYM_RPAREN       22

#define ASMRP_MAX_ID         1024

#define ASMRP_MAX_SYMTAB       10

typedef struct {
  char *id;
  int   v;
} asmrp_sym_t;

typedef struct {

  /* public part */

  int         sym;
  int         num;
 
  char        str[ASMRP_MAX_ID];

  /* private part */

  char       *buf;
  int         pos;
  char        ch;

  asmrp_sym_t sym_tab[ASMRP_MAX_SYMTAB];
  int         sym_tab_num;

} asmrp_t;

static asmrp_t *asmrp_new (void ) {

  asmrp_t *p;

  p = malloc (sizeof (asmrp_t));

  p->sym_tab_num = 0;
  p->sym         = ASMRP_SYM_NONE;
  p->buf         = NULL;

  return p;
}

static void asmrp_dispose (asmrp_t *p) {

  int i;

  for (i=0; i<p->sym_tab_num; i++)
    free (p->sym_tab[i].id);

  free( p->buf );
  free( p );
}

static void asmrp_getch (asmrp_t *p) {
  p->ch = p->buf[p->pos];
  p->pos++;

  lprintf ("%c\n", p->ch);

}

static void asmrp_init (asmrp_t *p, const char *str) {

  p->buf = strdup (str);
  p->pos = 0;
 
  asmrp_getch (p);
}

static void asmrp_number (asmrp_t *p) {

  int num;

  num = 0;
  while ( (p->ch>='0') && (p->ch<='9') ) {

    num = num*10 + (p->ch - '0');

    asmrp_getch (p);
  }

  p->sym = ASMRP_SYM_NUM;
  p->num = num;
}

static void asmrp_string (asmrp_t *p) {

  int l;

  l = 0;

  while ( (p->ch!='"') && (p->ch>=32) ) {

    p->str[l] = p->ch;

    l++;
    asmrp_getch (p);
  }
  p->str[l]=0;
 
  if (p->ch=='"')
    asmrp_getch (p);
 
  p->sym = ASMRP_SYM_STRING;
}

static void asmrp_identifier (asmrp_t *p) {

  int l;

  l = 0;

  while ( ((p->ch>='A') && (p->ch<='z'))
      || ((p->ch>='0') && (p->ch<='9'))) {

    p->str[l] = p->ch;

    l++;
    asmrp_getch (p);
  }
  p->str[l]=0;
 
  p->sym = ASMRP_SYM_ID;
}

#ifdef LOG
static void asmrp_print_sym (asmrp_t *p) {

  printf ("symbol: ");

  switch (p->sym) {

  case ASMRP_SYM_NONE:
    printf ("NONE\n");
    break;

  case ASMRP_SYM_EOF:
    printf ("EOF\n");
    break;

  case ASMRP_SYM_NUM:
    printf ("NUM %d\n", p->num);
    break;

  case ASMRP_SYM_ID:
    printf ("ID '%s'\n", p->str);
    break;

  case ASMRP_SYM_STRING:
    printf ("STRING \"%s\"\n", p->str);
    break;

  case ASMRP_SYM_HASH:
    printf ("#\n");
    break;

  case ASMRP_SYM_SEMICOLON:
    printf (";\n");
    break;
  case ASMRP_SYM_COMMA:
    printf (",\n");
    break;
  case ASMRP_SYM_EQUALS:
    printf ("==\n");
    break;
  case ASMRP_SYM_AND:
    printf ("&&\n");
    break;
  case ASMRP_SYM_OR:
    printf ("||\n");
    break;
  case ASMRP_SYM_LESS:
    printf ("<\n");
    break;
  case ASMRP_SYM_LEQ:
    printf ("<=\n");
    break;
  case ASMRP_SYM_GEQ:
    printf (">=\n");
    break;
  case ASMRP_SYM_GREATER:
    printf (">\n");
    break;
  case ASMRP_SYM_DOLLAR:
    printf ("$\n");
    break;
  case ASMRP_SYM_LPAREN:
    printf ("(\n");
    break;
  case ASMRP_SYM_RPAREN:
    printf (")\n");
    break;

  default:
    printf ("unknown symbol %d\n", p->sym);
  }
}
#endif

static void asmrp_get_sym (asmrp_t *p) {

  while (p->ch <= 32) {
    if (p->ch == 0) {
      p->sym = ASMRP_SYM_EOF;
      return;
    }

    asmrp_getch (p);
  }

  if (p->ch == '\\')
    asmrp_getch (p);

  switch (p->ch) {

  case '#':
    p->sym = ASMRP_SYM_HASH;
    asmrp_getch (p);
    break;
  case ';':
    p->sym = ASMRP_SYM_SEMICOLON;
    asmrp_getch (p);
    break;
  case ',':
    p->sym = ASMRP_SYM_COMMA;
    asmrp_getch (p);
    break;
  case '=':
    p->sym = ASMRP_SYM_EQUALS;
    asmrp_getch (p);
    if (p->ch=='=')
      asmrp_getch (p);
    break;
  case '&':
    p->sym = ASMRP_SYM_AND;
    asmrp_getch (p);
    if (p->ch=='&')
      asmrp_getch (p);
    break;
  case '|':
    p->sym = ASMRP_SYM_OR;
    asmrp_getch (p);
    if (p->ch=='|')
      asmrp_getch (p);
    break;
  case '<':
    p->sym = ASMRP_SYM_LESS;
    asmrp_getch (p);
    if (p->ch=='=') {
      p->sym = ASMRP_SYM_LEQ;
      asmrp_getch (p);
    }
    break;
  case '>':
    p->sym = ASMRP_SYM_GREATER;
    asmrp_getch (p);
    if (p->ch=='=') {
      p->sym = ASMRP_SYM_GEQ;
      asmrp_getch (p);
    }
    break;
  case '$':
    p->sym = ASMRP_SYM_DOLLAR;
    asmrp_getch (p);
    break;
  case '(':
    p->sym = ASMRP_SYM_LPAREN;
    asmrp_getch (p);
    break;
  case ')':
    p->sym = ASMRP_SYM_RPAREN;
    asmrp_getch (p);
    break;

  case '"':
    asmrp_getch (p);
    asmrp_string (p);
    break;

  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    asmrp_number (p);
    break;

  default:
    asmrp_identifier (p);
  }

#ifdef LOG
  asmrp_print_sym (p);
#endif

}

static int asmrp_find_id (asmrp_t *p, const char *s) {

  int i;

  for (i=0; i<p->sym_tab_num; i++) {
    if (!strcmp (s, p->sym_tab[i].id))
      return i;
  }

  return -1;
}

static int asmrp_set_id (asmrp_t *p, const char *s, int v) {

  int i;

  i = asmrp_find_id (p, s);

  if (i<0) {
    i = p->sym_tab_num;
    p->sym_tab_num++;
    p->sym_tab[i].id = strdup (s);

    lprintf ("new symbol '%s'\n", s);

  }

  p->sym_tab[i].v = v;
 
  lprintf ("symbol '%s' assigned %d\n", s, v);

  return i;
}

static int asmrp_condition (asmrp_t *p) ;

static int asmrp_operand (asmrp_t *p) {

  int i, ret;
 
  lprintf ("operand\n");

  ret = 0;

  switch (p->sym) {

  case ASMRP_SYM_DOLLAR:

    asmrp_get_sym (p);
 
    if (p->sym != ASMRP_SYM_ID) {
      printf ("error: identifier expected.\n");
      break;
    }

    i = asmrp_find_id (p, p->str);
    if (i<0) {
      lprintf ("error: unknown identifier %s\n", p->str);
    }
    ret = p->sym_tab[i].v;

    asmrp_get_sym (p);
    break;

  case ASMRP_SYM_NUM:
    ret = p->num;

    asmrp_get_sym (p);
    break;

  case ASMRP_SYM_LPAREN:
    asmrp_get_sym (p);

    ret = asmrp_condition (p);

    if (p->sym != ASMRP_SYM_RPAREN) {
      printf ("error: ) expected.\n");
      break;
    }

    asmrp_get_sym (p);
    break;

  default:
    lprintf ("syntax error, $ number or ( expected\n");
    break;
  }

  lprintf ("operand done, =%d\n", ret);
 
  return ret;
}

static int asmrp_comp_expression (asmrp_t *p) {

  int a;

  lprintf ("comp_expression\n");

  a = asmrp_operand (p);

  while ( (p->sym == ASMRP_SYM_LESS)
      || (p->sym == ASMRP_SYM_LEQ)
      || (p->sym == ASMRP_SYM_EQUALS)
      || (p->sym == ASMRP_SYM_GEQ)
      || (p->sym == ASMRP_SYM_GREATER) ) {
    int op = p->sym;
    int b;

    asmrp_get_sym (p);

    b = asmrp_operand (p);

    switch (op) {
    case ASMRP_SYM_LESS:
      a = a<b;
      break;
    case ASMRP_SYM_LEQ:
      a = a<=b;
      break;
    case ASMRP_SYM_EQUALS:
      a = a==b;
      break;
    case ASMRP_SYM_GEQ:
      a = a>=b;
      break;
    case ASMRP_SYM_GREATER:
      a = a>b;
      break;
    }

  }

  lprintf ("comp_expression done = %d\n", a);

  return a;
}

static int asmrp_condition (asmrp_t *p) {
 
  int a;

  lprintf ("condition\n");

  a = asmrp_comp_expression (p);

  while ( (p->sym == ASMRP_SYM_AND) || (p->sym == ASMRP_SYM_OR) ) {
    int op, b;

    op = p->sym;

    asmrp_get_sym (p);

    b = asmrp_comp_expression (p);

    switch (op) {
    case ASMRP_SYM_AND:
      a = a & b;
      break;
    case ASMRP_SYM_OR:
      a = a | b;
      break;
    }
  }

  lprintf ("condition done = %d\n", a);

  return a;
}

static void asmrp_assignment (asmrp_t *p) {

  lprintf ("assignment\n");

  if (p->sym == ASMRP_SYM_COMMA || p->sym == ASMRP_SYM_SEMICOLON) {
    lprintf ("empty assignment\n");
    return;
  }
 
  if (p->sym != ASMRP_SYM_ID) {
    printf ("error: identifier expected\n");
    return;
  }
  asmrp_get_sym (p);

  if (p->sym != ASMRP_SYM_EQUALS) {
    printf ("error: = expected\n");
    return;
  }
  asmrp_get_sym (p);

  if ( (p->sym != ASMRP_SYM_NUM) && (p->sym != ASMRP_SYM_STRING)
       && (p->sym != ASMRP_SYM_ID)) {
    printf ("error: number or string expected\n");
    return;
  }
  asmrp_get_sym (p);

  lprintf ("assignment done\n");
}

static int asmrp_rule (asmrp_t *p) {
 
  int ret;

  lprintf ("rule\n");

  ret = 1;
 
  if (p->sym == ASMRP_SYM_HASH) {

    asmrp_get_sym (p);
    ret = asmrp_condition (p);

    while (p->sym == ASMRP_SYM_COMMA) {
 
      asmrp_get_sym (p);
 
      asmrp_assignment (p);
    }

  } else if (p->sym != ASMRP_SYM_SEMICOLON) {

    asmrp_assignment (p);

    while (p->sym == ASMRP_SYM_COMMA) {

      asmrp_get_sym (p);
      asmrp_assignment (p);
    }
  }

  lprintf ("rule done = %d\n", ret);

  if (p->sym != ASMRP_SYM_SEMICOLON) {
    printf ("semicolon expected.\n");
    return ret;
  }

  asmrp_get_sym (p);

  return ret;
}

static int asmrp_eval (asmrp_t *p, int *matches, int matchsize) {

  int rule_num, num_matches;

  lprintf ("eval\n");

  asmrp_get_sym (p);

  rule_num = 0; num_matches = 0;
  while (p->sym != ASMRP_SYM_EOF && num_matches < matchsize - 1) {

    if (asmrp_rule (p)) {
      lprintf ("rule #%d is true\n", rule_num);

      matches[num_matches] = rule_num;
      num_matches++;
    }

    rule_num++;
  }

  matches[num_matches] = -1;
  return num_matches;
}

int asmrp_match (const char *rules, int bandwidth, int *matches, int matchsize) {

  asmrp_t *p;
  int      num_matches;

  p = asmrp_new ();

  asmrp_init (p, rules);

  asmrp_set_id (p, "Bandwidth", bandwidth);
  asmrp_set_id (p, "OldPNMPlayer", 0);

  num_matches = asmrp_eval (p, matches, matchsize);

  asmrp_dispose (p);

  return num_matches;
}

