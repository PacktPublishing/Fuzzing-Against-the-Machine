/* A Bison parser, made by GNU Bison 3.7.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_CODEC_WEBVTT_CSSGRAMMAR_H_INCLUDED
# define YY_YY_CODEC_WEBVTT_CSSGRAMMAR_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    TOKEN_EOF = 0,                 /* TOKEN_EOF  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    LOWEST_PREC = 258,             /* LOWEST_PREC  */
    UNIMPORTANT_TOK = 259,         /* UNIMPORTANT_TOK  */
    WHITESPACE = 260,              /* WHITESPACE  */
    SGML_CD = 261,                 /* SGML_CD  */
    INCLUDES = 262,                /* INCLUDES  */
    DASHMATCH = 263,               /* DASHMATCH  */
    BEGINSWITH = 264,              /* BEGINSWITH  */
    ENDSWITH = 265,                /* ENDSWITH  */
    CONTAINS = 266,                /* CONTAINS  */
    STRING = 267,                  /* STRING  */
    IDENT = 268,                   /* IDENT  */
    IDSEL = 269,                   /* IDSEL  */
    HASH = 270,                    /* HASH  */
    FONT_FACE_SYM = 271,           /* FONT_FACE_SYM  */
    CHARSET_SYM = 272,             /* CHARSET_SYM  */
    IMPORTANT_SYM = 273,           /* IMPORTANT_SYM  */
    CDO = 274,                     /* CDO  */
    CDC = 275,                     /* CDC  */
    LENGTH = 276,                  /* LENGTH  */
    ANGLE = 277,                   /* ANGLE  */
    TIME = 278,                    /* TIME  */
    FREQ = 279,                    /* FREQ  */
    DIMEN = 280,                   /* DIMEN  */
    PERCENTAGE = 281,              /* PERCENTAGE  */
    NUMBER = 282,                  /* NUMBER  */
    URI = 283,                     /* URI  */
    FUNCTION = 284,                /* FUNCTION  */
    UNICODERANGE = 285             /* UNICODERANGE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define TOKEN_EOF 0
#define YYerror 256
#define YYUNDEF 257
#define LOWEST_PREC 258
#define UNIMPORTANT_TOK 259
#define WHITESPACE 260
#define SGML_CD 261
#define INCLUDES 262
#define DASHMATCH 263
#define BEGINSWITH 264
#define ENDSWITH 265
#define CONTAINS 266
#define STRING 267
#define IDENT 268
#define IDSEL 269
#define HASH 270
#define FONT_FACE_SYM 271
#define CHARSET_SYM 272
#define IMPORTANT_SYM 273
#define CDO 274
#define CDC 275
#define LENGTH 276
#define ANGLE 277
#define TIME 278
#define FREQ 279
#define DIMEN 280
#define PERCENTAGE 281
#define NUMBER 282
#define URI 283
#define FUNCTION 284
#define UNICODERANGE 285

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 49 "codec/webvtt/CSSGrammar.y"

    bool boolean;
    char character;
    int integer;
    char *string;
    enum vlc_css_relation_e relation;

    vlc_css_term_t term;
    vlc_css_expr_t *expr;
    vlc_css_rule_t  *rule;
    vlc_css_declaration_t *declaration;
    vlc_css_declaration_t *declarationList;
    vlc_css_selector_t *selector;
    vlc_css_selector_t *selectorList;

#line 143 "codec/webvtt/CSSGrammar.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (yyscan_t scanner, vlc_css_parser_t *css_parser);

#endif /* !YY_YY_CODEC_WEBVTT_CSSGRAMMAR_H_INCLUDED  */
