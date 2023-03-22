/* A Bison parser, made by GNU Bison 3.7.5.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30705

/* Bison version string.  */
#define YYBISON_VERSION "3.7.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 36 "codec/webvtt/CSSGrammar.y"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include "css_parser.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#line 84 "codec/webvtt/CSSGrammar.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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

#line 213 "codec/webvtt/CSSGrammar.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (yyscan_t scanner, vlc_css_parser_t *css_parser);

#endif /* !YY_YY_CODEC_WEBVTT_CSSGRAMMAR_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* TOKEN_EOF  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_LOWEST_PREC = 3,                /* LOWEST_PREC  */
  YYSYMBOL_UNIMPORTANT_TOK = 4,            /* UNIMPORTANT_TOK  */
  YYSYMBOL_WHITESPACE = 5,                 /* WHITESPACE  */
  YYSYMBOL_SGML_CD = 6,                    /* SGML_CD  */
  YYSYMBOL_INCLUDES = 7,                   /* INCLUDES  */
  YYSYMBOL_DASHMATCH = 8,                  /* DASHMATCH  */
  YYSYMBOL_BEGINSWITH = 9,                 /* BEGINSWITH  */
  YYSYMBOL_ENDSWITH = 10,                  /* ENDSWITH  */
  YYSYMBOL_CONTAINS = 11,                  /* CONTAINS  */
  YYSYMBOL_STRING = 12,                    /* STRING  */
  YYSYMBOL_IDENT = 13,                     /* IDENT  */
  YYSYMBOL_IDSEL = 14,                     /* IDSEL  */
  YYSYMBOL_HASH = 15,                      /* HASH  */
  YYSYMBOL_16_ = 16,                       /* ':'  */
  YYSYMBOL_17_ = 17,                       /* '.'  */
  YYSYMBOL_18_ = 18,                       /* '['  */
  YYSYMBOL_19_ = 19,                       /* '*'  */
  YYSYMBOL_20_ = 20,                       /* '|'  */
  YYSYMBOL_FONT_FACE_SYM = 21,             /* FONT_FACE_SYM  */
  YYSYMBOL_CHARSET_SYM = 22,               /* CHARSET_SYM  */
  YYSYMBOL_IMPORTANT_SYM = 23,             /* IMPORTANT_SYM  */
  YYSYMBOL_CDO = 24,                       /* CDO  */
  YYSYMBOL_CDC = 25,                       /* CDC  */
  YYSYMBOL_LENGTH = 26,                    /* LENGTH  */
  YYSYMBOL_ANGLE = 27,                     /* ANGLE  */
  YYSYMBOL_TIME = 28,                      /* TIME  */
  YYSYMBOL_FREQ = 29,                      /* FREQ  */
  YYSYMBOL_DIMEN = 30,                     /* DIMEN  */
  YYSYMBOL_PERCENTAGE = 31,                /* PERCENTAGE  */
  YYSYMBOL_NUMBER = 32,                    /* NUMBER  */
  YYSYMBOL_URI = 33,                       /* URI  */
  YYSYMBOL_FUNCTION = 34,                  /* FUNCTION  */
  YYSYMBOL_UNICODERANGE = 35,              /* UNICODERANGE  */
  YYSYMBOL_36_ = 36,                       /* '}'  */
  YYSYMBOL_37_ = 37,                       /* ';'  */
  YYSYMBOL_38_ = 38,                       /* '{'  */
  YYSYMBOL_39_ = 39,                       /* '+'  */
  YYSYMBOL_40_ = 40,                       /* '~'  */
  YYSYMBOL_41_ = 41,                       /* '>'  */
  YYSYMBOL_42_ = 42,                       /* '-'  */
  YYSYMBOL_43_ = 43,                       /* ','  */
  YYSYMBOL_44_ = 44,                       /* ']'  */
  YYSYMBOL_45_ = 45,                       /* '='  */
  YYSYMBOL_46_ = 46,                       /* ')'  */
  YYSYMBOL_47_ = 47,                       /* '/'  */
  YYSYMBOL_48_ = 48,                       /* '#'  */
  YYSYMBOL_49_ = 49,                       /* '%'  */
  YYSYMBOL_YYACCEPT = 50,                  /* $accept  */
  YYSYMBOL_stylesheet = 51,                /* stylesheet  */
  YYSYMBOL_maybe_space = 52,               /* maybe_space  */
  YYSYMBOL_maybe_sgml = 53,                /* maybe_sgml  */
  YYSYMBOL_maybe_charset = 54,             /* maybe_charset  */
  YYSYMBOL_closing_brace = 55,             /* closing_brace  */
  YYSYMBOL_charset = 56,                   /* charset  */
  YYSYMBOL_ignored_charset = 57,           /* ignored_charset  */
  YYSYMBOL_rule_list = 58,                 /* rule_list  */
  YYSYMBOL_valid_rule = 59,                /* valid_rule  */
  YYSYMBOL_rule = 60,                      /* rule  */
  YYSYMBOL_font_face = 61,                 /* font_face  */
  YYSYMBOL_combinator = 62,                /* combinator  */
  YYSYMBOL_maybe_unary_operator = 63,      /* maybe_unary_operator  */
  YYSYMBOL_unary_operator = 64,            /* unary_operator  */
  YYSYMBOL_ruleset = 65,                   /* ruleset  */
  YYSYMBOL_selector_list = 66,             /* selector_list  */
  YYSYMBOL_selector_with_trailing_whitespace = 67, /* selector_with_trailing_whitespace  */
  YYSYMBOL_selector = 68,                  /* selector  */
  YYSYMBOL_simple_selector = 69,           /* simple_selector  */
  YYSYMBOL_element_name = 70,              /* element_name  */
  YYSYMBOL_specifier_list = 71,            /* specifier_list  */
  YYSYMBOL_specifier = 72,                 /* specifier  */
  YYSYMBOL_class = 73,                     /* class  */
  YYSYMBOL_attr_name = 74,                 /* attr_name  */
  YYSYMBOL_attrib = 75,                    /* attrib  */
  YYSYMBOL_match = 76,                     /* match  */
  YYSYMBOL_ident_or_string = 77,           /* ident_or_string  */
  YYSYMBOL_pseudo = 78,                    /* pseudo  */
  YYSYMBOL_declaration_list = 79,          /* declaration_list  */
  YYSYMBOL_decl_list = 80,                 /* decl_list  */
  YYSYMBOL_declaration = 81,               /* declaration  */
  YYSYMBOL_property = 82,                  /* property  */
  YYSYMBOL_prio = 83,                      /* prio  */
  YYSYMBOL_expr = 84,                      /* expr  */
  YYSYMBOL_operator = 85,                  /* operator  */
  YYSYMBOL_term = 86,                      /* term  */
  YYSYMBOL_unary_term = 87,                /* unary_term  */
  YYSYMBOL_function = 88,                  /* function  */
  YYSYMBOL_invalid_rule = 89,              /* invalid_rule  */
  YYSYMBOL_invalid_block = 90,             /* invalid_block  */
  YYSYMBOL_invalid_block_list = 91         /* invalid_block_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;


/* Second part of user prologue.  */
#line 65 "codec/webvtt/CSSGrammar.y"

/* See bison pure calling */
int yylex(union YYSTYPE *, yyscan_t, vlc_css_parser_t *);

static int yyerror(yyscan_t scanner, vlc_css_parser_t *p, const char *msg)
{
    VLC_UNUSED(scanner);VLC_UNUSED(p);VLC_UNUSED(msg);
    return 1;
}


#line 339 "codec/webvtt/CSSGrammar.c"


#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   473

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  42
/* YYNRULES -- Number of rules.  */
#define YYNRULES  135
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  230

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   285


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    48,     2,    49,     2,     2,
       2,    46,    19,    39,    43,    42,    17,    47,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    16,    37,
       2,    45,    41,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    18,     2,    44,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    38,    20,    36,    40,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   176,   176,   180,   181,   184,   186,   187,   190,   192,
     198,   199,   203,   207,   210,   216,   221,   226,   228,   235,
     236,   240,   245,   246,   250,   255,   258,   264,   265,   266,
     270,   271,   275,   276,   280,   291,   296,   309,   316,   322,
     325,   329,   339,   348,   355,   359,   371,   377,   378,   382,
     385,   395,   402,   407,   415,   416,   417,   421,   428,   434,
     438,   451,   454,   457,   460,   463,   466,   472,   473,   477,
     481,   486,   495,   509,   519,   522,   527,   530,   533,   536,
     539,   545,   548,   552,   556,   559,   562,   570,   573,   579,
     588,   593,   602,   609,   614,   620,   626,   634,   640,   641,
     645,   650,   655,   659,   663,   670,   673,   676,   682,   683,
     687,   688,   690,   691,   692,   693,   694,   695,   696,   698,
     701,   707,   708,   709,   710,   711,   712,   716,   722,   727,
     732,   742,   760,   761,   765,   766
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "TOKEN_EOF", "error", "\"invalid token\"", "LOWEST_PREC",
  "UNIMPORTANT_TOK", "WHITESPACE", "SGML_CD", "INCLUDES", "DASHMATCH",
  "BEGINSWITH", "ENDSWITH", "CONTAINS", "STRING", "IDENT", "IDSEL", "HASH",
  "':'", "'.'", "'['", "'*'", "'|'", "FONT_FACE_SYM", "CHARSET_SYM",
  "IMPORTANT_SYM", "CDO", "CDC", "LENGTH", "ANGLE", "TIME", "FREQ",
  "DIMEN", "PERCENTAGE", "NUMBER", "URI", "FUNCTION", "UNICODERANGE",
  "'}'", "';'", "'{'", "'+'", "'~'", "'>'", "'-'", "','", "']'", "'='",
  "')'", "'/'", "'#'", "'%'", "$accept", "stylesheet", "maybe_space",
  "maybe_sgml", "maybe_charset", "closing_brace", "charset",
  "ignored_charset", "rule_list", "valid_rule", "rule", "font_face",
  "combinator", "maybe_unary_operator", "unary_operator", "ruleset",
  "selector_list", "selector_with_trailing_whitespace", "selector",
  "simple_selector", "element_name", "specifier_list", "specifier",
  "class", "attr_name", "attrib", "match", "ident_or_string", "pseudo",
  "declaration_list", "decl_list", "declaration", "property", "prio",
  "expr", "operator", "term", "unary_term", "function", "invalid_rule",
  "invalid_block", "invalid_block_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,    58,    46,    91,    42,
     124,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   125,    59,   123,    43,
     126,    62,    45,    44,    93,    61,    41,    47,    35,    37
};
#endif

#define YYPACT_NINF (-126)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-108)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -126,    10,    88,  -126,  -126,   290,  -126,  -126,    68,     6,
     135,  -126,    11,  -126,  -126,  -126,  -126,   328,    73,    75,
     -24,  -126,  -126,  -126,   368,     9,  -126,  -126,    95,  -126,
    -126,  -126,  -126,  -126,  -126,     1,   420,    84,  -126,   292,
      14,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,
      35,  -126,  -126,  -126,    -7,  -126,  -126,     3,   117,    30,
     171,   135,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,
    -126,   420,   179,  -126,  -126,    73,  -126,  -126,    79,  -126,
     141,  -126,  -126,  -126,  -126,  -126,   186,   248,    70,    70,
      70,  -126,  -126,  -126,   248,  -126,  -126,  -126,    54,  -126,
      70,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,   186,
     121,   119,  -126,  -126,    41,   388,   169,    24,    96,    42,
      -1,  -126,   291,    41,  -126,  -126,    90,    70,    70,  -126,
     173,    58,    66,  -126,   404,  -126,  -126,  -126,     0,  -126,
       2,  -126,  -126,  -126,  -126,    70,   204,  -126,   114,  -126,
     -24,    70,  -126,    70,   285,  -126,  -126,    12,  -126,    70,
     214,    70,    70,   417,  -126,  -126,  -126,  -126,  -126,  -126,
    -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,
     441,   245,  -126,  -126,  -126,  -126,    70,  -126,   245,    70,
      70,    70,    70,    70,    70,    70,    70,    70,    70,    70,
      70,   364,    70,    70,    70,  -126,  -126,  -126,  -126,  -126,
    -126,    33,   417,   201,    70,  -126,  -126,  -126,   325,    70,
      70,    70,    70,  -126,  -126,   -24,    70,  -126,  -126,    70
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     8,     1,     4,     0,     5,     9,     0,     0,
      17,    14,     0,    13,     3,     7,     6,     0,     0,     0,
       0,    47,    52,    53,     0,     0,     3,    48,     0,     3,
      22,    21,     5,    20,    19,     0,    40,     0,    39,    44,
       0,    49,    54,    55,    56,    23,    11,    10,   133,   134,
       0,    12,   131,    69,     0,     3,    57,     0,     0,     0,
       0,    18,    37,     3,     3,    41,    43,    38,     3,     3,
       3,     0,     0,    51,    50,     0,    70,     3,    31,     3,
       0,    26,    25,     3,     3,    16,     0,     0,    27,    28,
      29,    42,   132,   135,     0,     3,    33,    32,     0,    30,
      58,    62,    63,    64,    65,    66,    59,    61,     3,     0,
       0,    78,     3,     3,     0,     0,    74,     0,     0,     0,
       0,     3,     0,     0,    15,     3,     0,    97,    93,    34,
      79,    75,     0,     3,     0,    90,     3,    96,     0,    73,
       0,    68,    67,     3,    24,    84,    77,     3,     0,     3,
       0,    81,     3,    82,     0,    72,    71,     0,     3,    87,
       0,    86,    83,    95,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       0,     0,   100,   108,   119,    60,    85,     3,     0,   110,
     111,   116,   117,   123,   124,   125,   126,   112,   122,   121,
     114,     0,   115,   118,   120,     3,   109,   104,     3,     3,
       3,     0,     0,     0,    88,    91,   130,     3,     0,   113,
      98,   106,   105,    92,   101,   103,   129,   128,     3,   127
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -126,  -126,    -5,    85,  -126,   -72,  -126,  -126,  -126,  -126,
    -126,  -126,  -126,  -126,    52,  -126,  -126,  -126,   -74,   -10,
    -126,    80,   105,  -126,  -126,  -126,  -126,  -126,  -126,    23,
    -126,    28,  -126,     4,  -125,  -126,   -34,     8,  -126,  -126,
      29,   -17
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     2,    10,     6,    48,     7,    30,    17,    31,
      32,    33,    71,    98,   180,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    80,    43,   108,   143,    44,   114,
     115,   116,   117,   211,   181,   212,   182,   183,   184,    45,
      49,   213
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
       9,    50,    62,    92,     4,     4,    76,     4,     4,    19,
       3,     4,    18,   118,    12,    73,    79,     4,    14,   -46,
     119,    57,    56,    59,    60,   135,    65,    77,    22,    23,
      24,    25,    26,   -89,   223,     4,    75,    13,   188,    63,
     136,    46,   129,    66,    64,   139,   155,    67,   156,    52,
      78,   144,   -46,   -46,   -46,   -46,   185,   -46,    86,    87,
     -46,    91,    12,    88,    89,    90,   -80,   150,    83,   -89,
     -89,   -89,    94,    46,   100,     4,   218,    47,   109,   110,
       4,    68,    69,    70,     4,    66,   121,    82,    -3,    67,
     120,   146,    95,     4,   126,   149,    58,    66,   132,   134,
      -3,    67,   -80,   122,    93,    11,    12,   127,   128,    47,
       5,    12,    51,   148,   138,   160,   140,    61,    96,    72,
     145,    97,   -35,    68,    69,    70,     4,   -35,   151,   153,
      99,   154,   123,    -3,   -36,    68,    69,    70,   157,   -36,
      15,    16,   159,   131,   161,    74,   137,   162,   101,   102,
     103,   104,   105,   186,    81,    12,   125,    12,   124,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,    93,     4,    74,   224,    93,
      73,     0,   214,    84,   -45,   106,   107,   111,   206,    93,
       0,     4,   215,    22,    23,    24,    25,    26,     0,   112,
     219,  -102,   225,   220,   221,   222,   133,    12,    85,   113,
     147,    12,   226,  -102,  -102,  -102,  -102,   -45,   -45,   -45,
     -45,     0,   -45,   229,  -102,   -45,     0,  -102,  -102,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
    -102,   158,    12,  -102,  -102,   -99,   207,  -102,  -102,  -102,
    -102,   187,    12,     4,    93,     0,     0,  -107,  -107,  -107,
    -107,    21,    22,    23,    24,    25,    26,    27,   208,     0,
       0,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,
    -107,   -99,   -99,    12,  -107,   -94,   163,  -107,   209,     0,
       4,     8,   210,  -107,  -107,    -3,     4,   164,   165,   166,
     167,     0,    -3,   141,   142,     0,    22,    23,    24,    25,
      26,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   -94,   -94,   -94,    96,   227,   207,    97,    -2,    20,
       0,     0,     0,   178,   179,     0,     0,  -107,  -107,  -107,
    -107,    21,    22,    23,    24,    25,    26,    27,     0,    28,
      29,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,  -107,
    -107,     0,     0,    12,  -107,   216,     0,  -107,   209,     4,
       0,   228,   210,  -107,  -107,     0,   164,   165,   166,   167,
       0,    53,     0,     0,    54,     0,     0,     0,   -76,   130,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
       0,   112,    55,    96,    -3,   150,    97,     0,     0,    -3,
     217,   113,   178,   179,     0,     0,     0,    -3,     0,     0,
       0,     0,     0,     0,   -76,     0,    12,    -3,     0,   164,
     165,   166,   167,    21,    22,    23,    24,    25,    26,    27,
      -3,   152,    -3,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,     0,     0,     0,    96,     0,     0,    97,
       0,     0,     0,     0,     0,   178,   179,   168,   169,   170,
     171,   205,   173,   174
};

static const yytype_int16 yycheck[] =
{
       5,    18,     1,    75,     5,     5,    13,     5,     5,    14,
       0,     5,     1,    87,    38,     1,    13,     5,    12,     5,
      94,    26,    13,    28,    29,     1,    36,    34,    14,    15,
      16,    17,    18,     0,     1,     5,     1,     8,   163,    38,
      16,     0,   114,     1,    43,    46,    46,     5,    46,    20,
      55,   123,    38,    39,    40,    41,    44,    43,    63,    64,
      46,    71,    38,    68,    69,    70,     0,     1,    38,    36,
      37,    38,    77,     0,    79,     5,   201,    36,    83,    84,
       5,    39,    40,    41,     5,     1,    32,    58,    46,     5,
      95,     1,    13,     5,   111,    37,     1,     1,   115,   116,
       5,     5,    36,   108,    75,    37,    38,   112,   113,    36,
      22,    38,    37,   130,   119,     1,   121,    32,    39,    39,
     125,    42,    38,    39,    40,    41,     5,    43,   133,   134,
      78,   136,   109,    38,    38,    39,    40,    41,   143,    43,
       5,     6,   147,   115,   149,    40,   117,   152,     7,     8,
       9,    10,    11,   158,    37,    38,    37,    38,    37,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   146,     5,    72,   212,   150,
       1,    -1,   187,    12,     5,    44,    45,     1,   180,   160,
      -1,     5,   188,    14,    15,    16,    17,    18,    -1,    13,
     205,     0,     1,   208,   209,   210,    37,    38,    37,    23,
      37,    38,   217,    12,    13,    14,    15,    38,    39,    40,
      41,    -1,    43,   228,    23,    46,    -1,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    37,    38,    42,    43,     0,     1,    46,    47,    48,
      49,    37,    38,     5,   225,    -1,    -1,    12,    13,    14,
      15,    13,    14,    15,    16,    17,    18,    19,    23,    -1,
      -1,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,     0,     1,    42,    43,    -1,
       5,     1,    47,    48,    49,     5,     5,    12,    13,    14,
      15,    -1,    12,    12,    13,    -1,    14,    15,    16,    17,
      18,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,     0,     1,    42,     0,     1,
      -1,    -1,    -1,    48,    49,    -1,    -1,    12,    13,    14,
      15,    13,    14,    15,    16,    17,    18,    19,    -1,    21,
      22,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    -1,    -1,    38,    39,     1,    -1,    42,    43,     5,
      -1,    46,    47,    48,    49,    -1,    12,    13,    14,    15,
      -1,    13,    -1,    -1,    16,    -1,    -1,    -1,     0,     1,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      -1,    13,    34,    39,     0,     1,    42,    -1,    -1,     5,
      46,    23,    48,    49,    -1,    -1,    -1,    13,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    -1,    38,    23,    -1,    12,
      13,    14,    15,    13,    14,    15,    16,    17,    18,    19,
      36,    37,    38,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    -1,    -1,    -1,    39,    -1,    -1,    42,
      -1,    -1,    -1,    -1,    -1,    48,    49,    26,    27,    28,
      29,    30,    31,    32
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    51,    52,     0,     5,    22,    54,    56,     1,    52,
      53,    37,    38,    90,    12,     5,     6,    58,     1,    52,
       1,    13,    14,    15,    16,    17,    18,    19,    21,    22,
      57,    59,    60,    61,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    75,    78,    89,     0,    36,    55,    90,
      91,    37,    90,    13,    16,    34,    13,    52,     1,    52,
      52,    53,     1,    38,    43,    69,     1,     5,    39,    40,
      41,    62,    71,     1,    72,     1,    13,    34,    52,    13,
      74,    37,    90,    38,    12,    37,    52,    52,    52,    52,
      52,    69,    55,    90,    52,    13,    39,    42,    63,    64,
      52,     7,     8,     9,    10,    11,    44,    45,    76,    52,
      52,     1,    13,    23,    79,    80,    81,    82,    68,    68,
      52,    32,    52,    79,    37,    37,    91,    52,    52,    55,
       1,    81,    91,    37,    91,     1,    16,    90,    52,    46,
      52,    12,    13,    77,    55,    52,     1,    37,    91,    37,
       1,    52,    37,    52,    52,    46,    46,    52,    37,    52,
       1,    52,    52,     1,    12,    13,    14,    15,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    48,    49,
      64,    84,    86,    87,    88,    44,    52,    37,    84,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    30,    87,     1,    23,    43,
      47,    83,    85,    91,    52,    83,     1,    46,    84,    52,
      52,    52,    52,     1,    86,     1,    52,     0,    46,    52
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    50,    51,    52,    52,    53,    53,    53,    54,    54,
      55,    55,    56,    56,    56,    57,    57,    58,    58,    59,
      59,    60,    60,    60,    61,    61,    61,    62,    62,    62,
      63,    63,    64,    64,    65,    66,    66,    66,    67,    68,
      68,    68,    68,    68,    69,    69,    69,    70,    70,    71,
      71,    71,    72,    72,    72,    72,    72,    73,    74,    75,
      75,    76,    76,    76,    76,    76,    76,    77,    77,    78,
      78,    78,    78,    78,    79,    79,    79,    79,    79,    79,
      79,    80,    80,    80,    80,    80,    80,    80,    80,    81,
      81,    81,    81,    81,    81,    81,    81,    82,    83,    83,
      84,    84,    84,    84,    84,    85,    85,    85,    86,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    87,    87,    87,    87,    87,    87,    88,    88,    88,
      88,    89,    90,    90,    91,    91
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     4,     0,     2,     0,     2,     2,     0,     1,
       1,     1,     5,     3,     3,     5,     3,     0,     3,     1,
       1,     1,     1,     1,     6,     3,     3,     2,     2,     2,
       1,     0,     1,     1,     5,     1,     4,     2,     2,     1,
       1,     2,     3,     2,     1,     2,     1,     1,     1,     1,
       2,     2,     1,     1,     1,     1,     1,     2,     2,     4,
       8,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       3,     7,     7,     6,     1,     2,     1,     3,     1,     2,
       2,     3,     3,     4,     3,     5,     4,     4,     6,     5,
       2,     6,     6,     2,     3,     4,     2,     2,     2,     0,
       1,     3,     2,     3,     2,     2,     2,     0,     1,     2,
       2,     2,     2,     3,     2,     2,     2,     2,     2,     1,
       2,     2,     2,     2,     2,     2,     2,     5,     4,     4,
       3,     2,     5,     3,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (scanner, css_parser, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, scanner, css_parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, yyscan_t scanner, vlc_css_parser_t *css_parser)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (scanner);
  YY_USE (css_parser);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, yyscan_t scanner, vlc_css_parser_t *css_parser)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, scanner, css_parser);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, yyscan_t scanner, vlc_css_parser_t *css_parser)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], scanner, css_parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner, css_parser); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, yyscan_t scanner, vlc_css_parser_t *css_parser)
{
  YY_USE (yyvaluep);
  YY_USE (scanner);
  YY_USE (css_parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_STRING: /* STRING  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1262 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_IDENT: /* IDENT  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1268 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_IDSEL: /* IDSEL  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1274 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_HASH: /* HASH  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1280 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_LENGTH: /* LENGTH  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1286 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_ANGLE: /* ANGLE  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1292 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_TIME: /* TIME  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1298 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_FREQ: /* FREQ  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1304 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_DIMEN: /* DIMEN  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1310 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_PERCENTAGE: /* PERCENTAGE  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1316 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_NUMBER: /* NUMBER  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1322 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_URI: /* URI  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1328 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_FUNCTION: /* FUNCTION  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1334 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_UNICODERANGE: /* UNICODERANGE  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1340 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_charset: /* charset  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1346 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_ignored_charset: /* ignored_charset  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1352 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_valid_rule: /* valid_rule  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1358 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_rule: /* rule  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1364 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_font_face: /* font_face  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1370 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_ruleset: /* ruleset  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1376 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_selector_list: /* selector_list  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selectorList)); }
#line 1382 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_selector_with_trailing_whitespace: /* selector_with_trailing_whitespace  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1388 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_selector: /* selector  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1394 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_simple_selector: /* simple_selector  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1400 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_element_name: /* element_name  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1406 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_specifier_list: /* specifier_list  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1412 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_specifier: /* specifier  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1418 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_class: /* class  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1424 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_attr_name: /* attr_name  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1430 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_attrib: /* attrib  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1436 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_ident_or_string: /* ident_or_string  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1442 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_pseudo: /* pseudo  */
#line 148 "codec/webvtt/CSSGrammar.y"
            { vlc_css_selectors_Delete(((*yyvaluep).selector)); }
#line 1448 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_declaration_list: /* declaration_list  */
#line 153 "codec/webvtt/CSSGrammar.y"
            { vlc_css_declarations_Delete(((*yyvaluep).declarationList)); }
#line 1454 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_decl_list: /* decl_list  */
#line 153 "codec/webvtt/CSSGrammar.y"
            { vlc_css_declarations_Delete(((*yyvaluep).declarationList)); }
#line 1460 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_declaration: /* declaration  */
#line 153 "codec/webvtt/CSSGrammar.y"
            { vlc_css_declarations_Delete(((*yyvaluep).declaration)); }
#line 1466 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_property: /* property  */
#line 171 "codec/webvtt/CSSGrammar.y"
            { free(((*yyvaluep).string)); }
#line 1472 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_expr: /* expr  */
#line 166 "codec/webvtt/CSSGrammar.y"
            { vlc_css_expression_Delete(((*yyvaluep).expr)); }
#line 1478 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_term: /* term  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1484 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_unary_term: /* unary_term  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1490 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_function: /* function  */
#line 119 "codec/webvtt/CSSGrammar.y"
            { vlc_css_term_Clean(((*yyvaluep).term)); }
#line 1496 "codec/webvtt/CSSGrammar.c"
        break;

    case YYSYMBOL_invalid_rule: /* invalid_rule  */
#line 134 "codec/webvtt/CSSGrammar.y"
            { vlc_css_rules_Delete(((*yyvaluep).rule)); }
#line 1502 "codec/webvtt/CSSGrammar.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (yyscan_t scanner, vlc_css_parser_t *css_parser)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, scanner, css_parser);
    }

  if (yychar <= TOKEN_EOF)
    {
      yychar = TOKEN_EOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 9: /* maybe_charset: charset  */
#line 192 "codec/webvtt/CSSGrammar.y"
            {
    vlc_css_rules_Delete((yyvsp[0].rule));
  }
#line 1778 "codec/webvtt/CSSGrammar.c"
    break;

  case 12: /* charset: CHARSET_SYM maybe_space STRING maybe_space ';'  */
#line 203 "codec/webvtt/CSSGrammar.y"
                                                 {
      free( (yyvsp[-2].string) );
      (yyval.rule) = 0;
  }
#line 1787 "codec/webvtt/CSSGrammar.c"
    break;

  case 13: /* charset: CHARSET_SYM error invalid_block  */
#line 207 "codec/webvtt/CSSGrammar.y"
                                    {
      (yyval.rule) = 0;
  }
#line 1795 "codec/webvtt/CSSGrammar.c"
    break;

  case 14: /* charset: CHARSET_SYM error ';'  */
#line 210 "codec/webvtt/CSSGrammar.y"
                          {
      (yyval.rule) = 0;
  }
#line 1803 "codec/webvtt/CSSGrammar.c"
    break;

  case 15: /* ignored_charset: CHARSET_SYM maybe_space STRING maybe_space ';'  */
#line 216 "codec/webvtt/CSSGrammar.y"
                                                   {
        // Ignore any @charset rule not at the beginning of the style sheet.
        free( (yyvsp[-2].string) );
        (yyval.rule) = 0;
    }
#line 1813 "codec/webvtt/CSSGrammar.c"
    break;

  case 16: /* ignored_charset: CHARSET_SYM maybe_space ';'  */
#line 221 "codec/webvtt/CSSGrammar.y"
                                  {
        (yyval.rule) = 0;
    }
#line 1821 "codec/webvtt/CSSGrammar.c"
    break;

  case 18: /* rule_list: rule_list rule maybe_sgml  */
#line 228 "codec/webvtt/CSSGrammar.y"
                             {
     if( (yyvsp[-1].rule) )
         vlc_css_parser_AddRule( css_parser, (yyvsp[-1].rule) );
 }
#line 1830 "codec/webvtt/CSSGrammar.c"
    break;

  case 21: /* rule: valid_rule  */
#line 240 "codec/webvtt/CSSGrammar.y"
               {
        (yyval.rule) = (yyvsp[0].rule);
        if((yyval.rule))
            (yyval.rule)->b_valid = true;
    }
#line 1840 "codec/webvtt/CSSGrammar.c"
    break;

  case 24: /* font_face: FONT_FACE_SYM maybe_space '{' maybe_space declaration_list closing_brace  */
#line 251 "codec/webvtt/CSSGrammar.y"
                                                   {
        vlc_css_declarations_Delete( (yyvsp[-1].declarationList) );
        (yyval.rule) = NULL;
    }
#line 1849 "codec/webvtt/CSSGrammar.c"
    break;

  case 25: /* font_face: FONT_FACE_SYM error invalid_block  */
#line 255 "codec/webvtt/CSSGrammar.y"
                                        {
        (yyval.rule) = NULL;
    }
#line 1857 "codec/webvtt/CSSGrammar.c"
    break;

  case 26: /* font_face: FONT_FACE_SYM error ';'  */
#line 258 "codec/webvtt/CSSGrammar.y"
                              {
        (yyval.rule) = NULL;
    }
#line 1865 "codec/webvtt/CSSGrammar.c"
    break;

  case 27: /* combinator: '+' maybe_space  */
#line 264 "codec/webvtt/CSSGrammar.y"
                    { (yyval.relation) = RELATION_DIRECTADJACENT; }
#line 1871 "codec/webvtt/CSSGrammar.c"
    break;

  case 28: /* combinator: '~' maybe_space  */
#line 265 "codec/webvtt/CSSGrammar.y"
                    { (yyval.relation) = RELATION_INDIRECTADJACENT; }
#line 1877 "codec/webvtt/CSSGrammar.c"
    break;

  case 29: /* combinator: '>' maybe_space  */
#line 266 "codec/webvtt/CSSGrammar.y"
                    { (yyval.relation) = RELATION_CHILD; }
#line 1883 "codec/webvtt/CSSGrammar.c"
    break;

  case 30: /* maybe_unary_operator: unary_operator  */
#line 270 "codec/webvtt/CSSGrammar.y"
                   { (yyval.integer) = (yyvsp[0].integer); }
#line 1889 "codec/webvtt/CSSGrammar.c"
    break;

  case 31: /* maybe_unary_operator: %empty  */
#line 271 "codec/webvtt/CSSGrammar.y"
      { (yyval.integer) = 1; }
#line 1895 "codec/webvtt/CSSGrammar.c"
    break;

  case 32: /* unary_operator: '-'  */
#line 275 "codec/webvtt/CSSGrammar.y"
        { (yyval.integer) = -1; }
#line 1901 "codec/webvtt/CSSGrammar.c"
    break;

  case 33: /* unary_operator: '+'  */
#line 276 "codec/webvtt/CSSGrammar.y"
        { (yyval.integer) = 1; }
#line 1907 "codec/webvtt/CSSGrammar.c"
    break;

  case 34: /* ruleset: selector_list '{' maybe_space declaration_list closing_brace  */
#line 280 "codec/webvtt/CSSGrammar.y"
                                                                 {
        (yyval.rule) = vlc_css_rule_New();
        if((yyval.rule))
        {
            (yyval.rule)->p_selectors = (yyvsp[-4].selectorList);
            (yyval.rule)->p_declarations = (yyvsp[-1].declarationList);
        }
    }
#line 1920 "codec/webvtt/CSSGrammar.c"
    break;

  case 35: /* selector_list: selector  */
#line 291 "codec/webvtt/CSSGrammar.y"
                                   {
        if ((yyvsp[0].selector)) {
            (yyval.selectorList) = (yyvsp[0].selector);
        }
    }
#line 1930 "codec/webvtt/CSSGrammar.c"
    break;

  case 36: /* selector_list: selector_list ',' maybe_space selector  */
#line 296 "codec/webvtt/CSSGrammar.y"
                                                                   {
        if ((yyvsp[-3].selectorList) && (yyvsp[0].selector) )
        {
            (yyval.selectorList) = (yyvsp[-3].selectorList);
            vlc_css_selector_Append( (yyval.selectorList), (yyvsp[0].selector) );
        }
        else
        {
            vlc_css_selectors_Delete( (yyvsp[-3].selectorList) );
            vlc_css_selectors_Delete( (yyvsp[0].selector) );
            (yyval.selectorList) = NULL;
        }
    }
#line 1948 "codec/webvtt/CSSGrammar.c"
    break;

  case 37: /* selector_list: selector_list error  */
#line 309 "codec/webvtt/CSSGrammar.y"
                        {
        vlc_css_selectors_Delete( (yyvsp[-1].selectorList) );
        (yyval.selectorList) = NULL;
    }
#line 1957 "codec/webvtt/CSSGrammar.c"
    break;

  case 38: /* selector_with_trailing_whitespace: selector WHITESPACE  */
#line 316 "codec/webvtt/CSSGrammar.y"
                        {
        (yyval.selector) = (yyvsp[-1].selector);
    }
#line 1965 "codec/webvtt/CSSGrammar.c"
    break;

  case 39: /* selector: simple_selector  */
#line 322 "codec/webvtt/CSSGrammar.y"
                    {
        (yyval.selector) = (yyvsp[0].selector);
    }
#line 1973 "codec/webvtt/CSSGrammar.c"
    break;

  case 40: /* selector: selector_with_trailing_whitespace  */
#line 326 "codec/webvtt/CSSGrammar.y"
    {
        (yyval.selector) = (yyvsp[0].selector);
    }
#line 1981 "codec/webvtt/CSSGrammar.c"
    break;

  case 41: /* selector: selector_with_trailing_whitespace simple_selector  */
#line 330 "codec/webvtt/CSSGrammar.y"
    {
        (yyval.selector) = (yyvsp[-1].selector);
        if ((yyval.selector))
        {
            vlc_css_selector_AddSpecifier( (yyval.selector), (yyvsp[0].selector) );
            (yyvsp[0].selector)->combinator = RELATION_DESCENDENT;
        }
        else (yyval.selector) = (yyvsp[0].selector);
    }
#line 1995 "codec/webvtt/CSSGrammar.c"
    break;

  case 42: /* selector: selector combinator simple_selector  */
#line 339 "codec/webvtt/CSSGrammar.y"
                                          {
        (yyval.selector) = (yyvsp[-2].selector);
        if ((yyval.selector))
        {
            vlc_css_selector_AddSpecifier( (yyval.selector), (yyvsp[0].selector) );
            (yyvsp[0].selector)->combinator = (yyvsp[-1].relation);
        }
        else (yyval.selector) = (yyvsp[0].selector);
    }
#line 2009 "codec/webvtt/CSSGrammar.c"
    break;

  case 43: /* selector: selector error  */
#line 348 "codec/webvtt/CSSGrammar.y"
                     {
        vlc_css_selectors_Delete( (yyvsp[-1].selector) );
        (yyval.selector) = NULL;
    }
#line 2018 "codec/webvtt/CSSGrammar.c"
    break;

  case 44: /* simple_selector: element_name  */
#line 355 "codec/webvtt/CSSGrammar.y"
                 {
        (yyval.selector) = vlc_css_selector_New( SELECTOR_SIMPLE, (yyvsp[0].string) );
        free( (yyvsp[0].string) );
    }
#line 2027 "codec/webvtt/CSSGrammar.c"
    break;

  case 45: /* simple_selector: element_name specifier_list  */
#line 359 "codec/webvtt/CSSGrammar.y"
                                  {
        (yyval.selector) = vlc_css_selector_New( SELECTOR_SIMPLE, (yyvsp[-1].string) );
        if( (yyval.selector) && (yyvsp[0].selector) )
        {
            vlc_css_selector_AddSpecifier( (yyval.selector), (yyvsp[0].selector) );
        }
        else
        {
            vlc_css_selectors_Delete( (yyvsp[0].selector) );
        }
        free( (yyvsp[-1].string) );
    }
#line 2044 "codec/webvtt/CSSGrammar.c"
    break;

  case 46: /* simple_selector: specifier_list  */
#line 371 "codec/webvtt/CSSGrammar.y"
                     {
        (yyval.selector) = (yyvsp[0].selector);
    }
#line 2052 "codec/webvtt/CSSGrammar.c"
    break;

  case 48: /* element_name: '*'  */
#line 378 "codec/webvtt/CSSGrammar.y"
          { (yyval.string) = strdup("*"); }
#line 2058 "codec/webvtt/CSSGrammar.c"
    break;

  case 49: /* specifier_list: specifier  */
#line 382 "codec/webvtt/CSSGrammar.y"
              {
        (yyval.selector) = (yyvsp[0].selector);
    }
#line 2066 "codec/webvtt/CSSGrammar.c"
    break;

  case 50: /* specifier_list: specifier_list specifier  */
#line 385 "codec/webvtt/CSSGrammar.y"
                               {
        if( (yyvsp[-1].selector) )
        {
            (yyval.selector) = (yyvsp[-1].selector);
            while( (yyvsp[-1].selector)->specifiers.p_first )
                (yyvsp[-1].selector) = (yyvsp[-1].selector)->specifiers.p_first;
            vlc_css_selector_AddSpecifier( (yyvsp[-1].selector), (yyvsp[0].selector) );
        }
        else (yyval.selector) = (yyvsp[0].selector);
    }
#line 2081 "codec/webvtt/CSSGrammar.c"
    break;

  case 51: /* specifier_list: specifier_list error  */
#line 395 "codec/webvtt/CSSGrammar.y"
                           {
        vlc_css_selectors_Delete( (yyvsp[-1].selector) );
        (yyval.selector) = NULL;
    }
#line 2090 "codec/webvtt/CSSGrammar.c"
    break;

  case 52: /* specifier: IDSEL  */
#line 402 "codec/webvtt/CSSGrammar.y"
          {
        (yyval.selector) = vlc_css_selector_New( SPECIFIER_ID, (yyvsp[0].string) );
        free( (yyvsp[0].string) );
    }
#line 2099 "codec/webvtt/CSSGrammar.c"
    break;

  case 53: /* specifier: HASH  */
#line 407 "codec/webvtt/CSSGrammar.y"
         {
        if ((yyvsp[0].string)[0] >= '0' && (yyvsp[0].string)[0] <= '9') {
            (yyval.selector) = NULL;
        } else {
            (yyval.selector) = vlc_css_selector_New( SPECIFIER_ID, (yyvsp[0].string) );
        }
        free( (yyvsp[0].string) );
    }
#line 2112 "codec/webvtt/CSSGrammar.c"
    break;

  case 57: /* class: '.' IDENT  */
#line 421 "codec/webvtt/CSSGrammar.y"
              {
        (yyval.selector) = vlc_css_selector_New( SPECIFIER_CLASS, (yyvsp[0].string) );
        free( (yyvsp[0].string) );
    }
#line 2121 "codec/webvtt/CSSGrammar.c"
    break;

  case 58: /* attr_name: IDENT maybe_space  */
#line 428 "codec/webvtt/CSSGrammar.y"
                      {
        (yyval.string) = (yyvsp[-1].string);
    }
#line 2129 "codec/webvtt/CSSGrammar.c"
    break;

  case 59: /* attrib: '[' maybe_space attr_name ']'  */
#line 434 "codec/webvtt/CSSGrammar.y"
                                  {
        (yyval.selector) = vlc_css_selector_New( SPECIFIER_ATTRIB, (yyvsp[-1].string) );
        free( (yyvsp[-1].string) );
    }
#line 2138 "codec/webvtt/CSSGrammar.c"
    break;

  case 60: /* attrib: '[' maybe_space attr_name match maybe_space ident_or_string maybe_space ']'  */
#line 438 "codec/webvtt/CSSGrammar.y"
                                                                                  {
        (yyval.selector) = vlc_css_selector_New( SPECIFIER_ATTRIB, (yyvsp[-5].string) );
        if( (yyval.selector) )
        {
            (yyval.selector)->match = (yyvsp[-4].integer);
            (yyval.selector)->p_matchsel = vlc_css_selector_New( SPECIFIER_ID, (yyvsp[-2].string) );
        }
        free( (yyvsp[-5].string) );
        free( (yyvsp[-2].string) );
    }
#line 2153 "codec/webvtt/CSSGrammar.c"
    break;

  case 61: /* match: '='  */
#line 451 "codec/webvtt/CSSGrammar.y"
        {
        (yyval.integer) = MATCH_EQUALS;
    }
#line 2161 "codec/webvtt/CSSGrammar.c"
    break;

  case 62: /* match: INCLUDES  */
#line 454 "codec/webvtt/CSSGrammar.y"
               {
        (yyval.integer) = MATCH_INCLUDES;
    }
#line 2169 "codec/webvtt/CSSGrammar.c"
    break;

  case 63: /* match: DASHMATCH  */
#line 457 "codec/webvtt/CSSGrammar.y"
                {
        (yyval.integer) = MATCH_DASHMATCH;
    }
#line 2177 "codec/webvtt/CSSGrammar.c"
    break;

  case 64: /* match: BEGINSWITH  */
#line 460 "codec/webvtt/CSSGrammar.y"
                 {
        (yyval.integer) = MATCH_BEGINSWITH;
    }
#line 2185 "codec/webvtt/CSSGrammar.c"
    break;

  case 65: /* match: ENDSWITH  */
#line 463 "codec/webvtt/CSSGrammar.y"
               {
        (yyval.integer) = MATCH_ENDSWITH;
    }
#line 2193 "codec/webvtt/CSSGrammar.c"
    break;

  case 66: /* match: CONTAINS  */
#line 466 "codec/webvtt/CSSGrammar.y"
               {
        (yyval.integer) = MATCH_CONTAINS;
    }
#line 2201 "codec/webvtt/CSSGrammar.c"
    break;

  case 69: /* pseudo: ':' IDENT  */
#line 477 "codec/webvtt/CSSGrammar.y"
              {
        (yyval.selector) = vlc_css_selector_New( SELECTOR_PSEUDOCLASS, (yyvsp[0].string) );
        free( (yyvsp[0].string) );
    }
#line 2210 "codec/webvtt/CSSGrammar.c"
    break;

  case 70: /* pseudo: ':' ':' IDENT  */
#line 481 "codec/webvtt/CSSGrammar.y"
                    {
        (yyval.selector) = vlc_css_selector_New( SELECTOR_PSEUDOELEMENT, (yyvsp[0].string) );
        free( (yyvsp[0].string) );
    }
#line 2219 "codec/webvtt/CSSGrammar.c"
    break;

  case 71: /* pseudo: ':' FUNCTION maybe_space maybe_unary_operator NUMBER maybe_space ')'  */
#line 486 "codec/webvtt/CSSGrammar.y"
                                                                           {
        if(*(yyvsp[-5].string) != 0)
            (yyvsp[-5].string)[strlen((yyvsp[-5].string)) - 1] = 0;
        (yyval.selector) = vlc_css_selector_New( SELECTOR_PSEUDOCLASS, (yyvsp[-5].string) );
        (yyvsp[-2].term).val *= (yyvsp[-3].integer);
        free( (yyvsp[-5].string) );
        vlc_css_term_Clean( (yyvsp[-2].term) );
    }
#line 2232 "codec/webvtt/CSSGrammar.c"
    break;

  case 72: /* pseudo: ':' ':' FUNCTION maybe_space selector maybe_space ')'  */
#line 495 "codec/webvtt/CSSGrammar.y"
                                                            {
        if(*(yyvsp[-4].string) != 0)
            (yyvsp[-4].string)[strlen((yyvsp[-4].string)) - 1] = 0;
        (yyval.selector) = vlc_css_selector_New( SELECTOR_PSEUDOELEMENT, (yyvsp[-4].string) );
        free( (yyvsp[-4].string) );
        if( (yyval.selector) && (yyvsp[-2].selector) )
        {
            vlc_css_selector_AddSpecifier( (yyval.selector), (yyvsp[-2].selector) );
            (yyvsp[-2].selector)->combinator = RELATION_SELF;
        }
        else
            vlc_css_selectors_Delete( (yyvsp[-2].selector) );
    }
#line 2250 "codec/webvtt/CSSGrammar.c"
    break;

  case 73: /* pseudo: ':' FUNCTION maybe_space IDENT maybe_space ')'  */
#line 509 "codec/webvtt/CSSGrammar.y"
                                                     {
        if(*(yyvsp[-4].string) != 0)
            (yyvsp[-4].string)[strlen((yyvsp[-4].string)) - 1] = 0;
        (yyval.selector) = vlc_css_selector_New( SELECTOR_PSEUDOCLASS, (yyvsp[-4].string) );
        free( (yyvsp[-4].string) );
        free( (yyvsp[-2].string) );
    }
#line 2262 "codec/webvtt/CSSGrammar.c"
    break;

  case 74: /* declaration_list: declaration  */
#line 519 "codec/webvtt/CSSGrammar.y"
                {
        (yyval.declarationList) = (yyvsp[0].declaration);
    }
#line 2270 "codec/webvtt/CSSGrammar.c"
    break;

  case 75: /* declaration_list: decl_list declaration  */
#line 522 "codec/webvtt/CSSGrammar.y"
                            {
        (yyval.declarationList) = (yyvsp[-1].declarationList);
        if( (yyval.declarationList) )
            vlc_css_declarations_Append( (yyval.declarationList), (yyvsp[0].declaration) );
    }
#line 2280 "codec/webvtt/CSSGrammar.c"
    break;

  case 76: /* declaration_list: decl_list  */
#line 527 "codec/webvtt/CSSGrammar.y"
                {
        (yyval.declarationList) = (yyvsp[0].declarationList);
    }
#line 2288 "codec/webvtt/CSSGrammar.c"
    break;

  case 77: /* declaration_list: error invalid_block_list error  */
#line 530 "codec/webvtt/CSSGrammar.y"
                                     {
        (yyval.declarationList) = NULL;
    }
#line 2296 "codec/webvtt/CSSGrammar.c"
    break;

  case 78: /* declaration_list: error  */
#line 533 "codec/webvtt/CSSGrammar.y"
            {
        (yyval.declarationList) = NULL;
    }
#line 2304 "codec/webvtt/CSSGrammar.c"
    break;

  case 79: /* declaration_list: decl_list error  */
#line 536 "codec/webvtt/CSSGrammar.y"
                      {
        (yyval.declarationList) = (yyvsp[-1].declarationList);
    }
#line 2312 "codec/webvtt/CSSGrammar.c"
    break;

  case 80: /* declaration_list: decl_list invalid_block_list  */
#line 539 "codec/webvtt/CSSGrammar.y"
                                   {
        (yyval.declarationList) = (yyvsp[-1].declarationList);
    }
#line 2320 "codec/webvtt/CSSGrammar.c"
    break;

  case 81: /* decl_list: declaration ';' maybe_space  */
#line 545 "codec/webvtt/CSSGrammar.y"
                                {
        (yyval.declarationList) = (yyvsp[-2].declaration);
    }
#line 2328 "codec/webvtt/CSSGrammar.c"
    break;

  case 82: /* decl_list: declaration invalid_block_list maybe_space  */
#line 548 "codec/webvtt/CSSGrammar.y"
                                                 {
        vlc_css_declarations_Delete( (yyvsp[-2].declaration) );
        (yyval.declarationList) = NULL;
    }
#line 2337 "codec/webvtt/CSSGrammar.c"
    break;

  case 83: /* decl_list: declaration invalid_block_list ';' maybe_space  */
#line 552 "codec/webvtt/CSSGrammar.y"
                                                     {
        vlc_css_declarations_Delete( (yyvsp[-3].declaration) );
        (yyval.declarationList) = NULL;
    }
#line 2346 "codec/webvtt/CSSGrammar.c"
    break;

  case 84: /* decl_list: error ';' maybe_space  */
#line 556 "codec/webvtt/CSSGrammar.y"
                            {
        (yyval.declarationList) = NULL;
    }
#line 2354 "codec/webvtt/CSSGrammar.c"
    break;

  case 85: /* decl_list: error invalid_block_list error ';' maybe_space  */
#line 559 "codec/webvtt/CSSGrammar.y"
                                                     {
        (yyval.declarationList) = NULL;
    }
#line 2362 "codec/webvtt/CSSGrammar.c"
    break;

  case 86: /* decl_list: decl_list declaration ';' maybe_space  */
#line 562 "codec/webvtt/CSSGrammar.y"
                                            {
        if( (yyvsp[-3].declarationList) )
        {
            (yyval.declarationList) = (yyvsp[-3].declarationList);
            vlc_css_declarations_Append( (yyval.declarationList), (yyvsp[-2].declaration) );
        }
        else (yyval.declarationList) = (yyvsp[-2].declaration);
    }
#line 2375 "codec/webvtt/CSSGrammar.c"
    break;

  case 87: /* decl_list: decl_list error ';' maybe_space  */
#line 570 "codec/webvtt/CSSGrammar.y"
                                      {
        (yyval.declarationList) = (yyvsp[-3].declarationList);
    }
#line 2383 "codec/webvtt/CSSGrammar.c"
    break;

  case 88: /* decl_list: decl_list error invalid_block_list error ';' maybe_space  */
#line 573 "codec/webvtt/CSSGrammar.y"
                                                               {
        (yyval.declarationList) = (yyvsp[-5].declarationList);
    }
#line 2391 "codec/webvtt/CSSGrammar.c"
    break;

  case 89: /* declaration: property ':' maybe_space expr prio  */
#line 579 "codec/webvtt/CSSGrammar.y"
                                       {
        (yyval.declaration) = vlc_css_declaration_New( (yyvsp[-4].string) );
        if( (yyval.declaration) )
            (yyval.declaration)->expr = (yyvsp[-1].expr);
        else
            vlc_css_expression_Delete( (yyvsp[-1].expr) );
        free( (yyvsp[-4].string) );
    }
#line 2404 "codec/webvtt/CSSGrammar.c"
    break;

  case 90: /* declaration: property error  */
#line 588 "codec/webvtt/CSSGrammar.y"
                   {
        free( (yyvsp[-1].string) );
        (yyval.declaration) = NULL;
    }
#line 2413 "codec/webvtt/CSSGrammar.c"
    break;

  case 91: /* declaration: property ':' maybe_space error expr prio  */
#line 593 "codec/webvtt/CSSGrammar.y"
                                             {
        free( (yyvsp[-5].string) );
        vlc_css_expression_Delete( (yyvsp[-1].expr) );
        /* The default movable type template has letter-spacing: .none;  Handle this by looking for
        error tokens at the start of an expr, recover the expr and then treat as an error, cleaning
        up and deleting the shifted expr.  */
        (yyval.declaration) = NULL;
    }
#line 2426 "codec/webvtt/CSSGrammar.c"
    break;

  case 92: /* declaration: property ':' maybe_space expr prio error  */
#line 602 "codec/webvtt/CSSGrammar.y"
                                             {
        free( (yyvsp[-5].string) );
        vlc_css_expression_Delete( (yyvsp[-2].expr) );
        /* When we encounter something like p {color: red !important fail;} we should drop the declaration */
        (yyval.declaration) = NULL;
    }
#line 2437 "codec/webvtt/CSSGrammar.c"
    break;

  case 93: /* declaration: IMPORTANT_SYM maybe_space  */
#line 609 "codec/webvtt/CSSGrammar.y"
                              {
        /* Handle this case: div { text-align: center; !important } Just reduce away the stray !important. */
        (yyval.declaration) = NULL;
    }
#line 2446 "codec/webvtt/CSSGrammar.c"
    break;

  case 94: /* declaration: property ':' maybe_space  */
#line 614 "codec/webvtt/CSSGrammar.y"
                             {
        free( (yyvsp[-2].string) );
        /* div { font-family: } Just reduce away this property with no value. */
        (yyval.declaration) = NULL;
    }
#line 2456 "codec/webvtt/CSSGrammar.c"
    break;

  case 95: /* declaration: property ':' maybe_space error  */
#line 620 "codec/webvtt/CSSGrammar.y"
                                   {
        free( (yyvsp[-3].string) );
        /* if we come across rules with invalid values like this case: p { weight: *; }, just discard the rule */
        (yyval.declaration) = NULL;
    }
#line 2466 "codec/webvtt/CSSGrammar.c"
    break;

  case 96: /* declaration: property invalid_block  */
#line 626 "codec/webvtt/CSSGrammar.y"
                           {
        /* if we come across: div { color{;color:maroon} }, ignore everything within curly brackets */
        free( (yyvsp[-1].string) );
        (yyval.declaration) = NULL;
    }
#line 2476 "codec/webvtt/CSSGrammar.c"
    break;

  case 97: /* property: IDENT maybe_space  */
#line 634 "codec/webvtt/CSSGrammar.y"
                      {
        (yyval.string) = (yyvsp[-1].string);
    }
#line 2484 "codec/webvtt/CSSGrammar.c"
    break;

  case 98: /* prio: IMPORTANT_SYM maybe_space  */
#line 640 "codec/webvtt/CSSGrammar.y"
                              { (yyval.boolean) = true; }
#line 2490 "codec/webvtt/CSSGrammar.c"
    break;

  case 99: /* prio: %empty  */
#line 641 "codec/webvtt/CSSGrammar.y"
                  { (yyval.boolean) = false; }
#line 2496 "codec/webvtt/CSSGrammar.c"
    break;

  case 100: /* expr: term  */
#line 645 "codec/webvtt/CSSGrammar.y"
         {
        (yyval.expr) = vlc_css_expression_New( (yyvsp[0].term) );
        if( !(yyval.expr) )
            vlc_css_term_Clean( (yyvsp[0].term) );
    }
#line 2506 "codec/webvtt/CSSGrammar.c"
    break;

  case 101: /* expr: expr operator term  */
#line 650 "codec/webvtt/CSSGrammar.y"
                         {
        (yyval.expr) = (yyvsp[-2].expr);
        if( !(yyvsp[-2].expr) || !vlc_css_expression_AddTerm((yyvsp[-2].expr), (yyvsp[-1].character), (yyvsp[0].term)) )
            vlc_css_term_Clean( (yyvsp[0].term) );
    }
#line 2516 "codec/webvtt/CSSGrammar.c"
    break;

  case 102: /* expr: expr invalid_block_list  */
#line 655 "codec/webvtt/CSSGrammar.y"
                              {
        vlc_css_expression_Delete( (yyvsp[-1].expr) );
        (yyval.expr) = NULL;
    }
#line 2525 "codec/webvtt/CSSGrammar.c"
    break;

  case 103: /* expr: expr invalid_block_list error  */
#line 659 "codec/webvtt/CSSGrammar.y"
                                    {
        vlc_css_expression_Delete( (yyvsp[-2].expr) );
        (yyval.expr) = NULL;
    }
#line 2534 "codec/webvtt/CSSGrammar.c"
    break;

  case 104: /* expr: expr error  */
#line 663 "codec/webvtt/CSSGrammar.y"
                 {
        vlc_css_expression_Delete( (yyvsp[-1].expr) );
        (yyval.expr) = NULL;
    }
#line 2543 "codec/webvtt/CSSGrammar.c"
    break;

  case 105: /* operator: '/' maybe_space  */
#line 670 "codec/webvtt/CSSGrammar.y"
                    {
        (yyval.character) = '/';
    }
#line 2551 "codec/webvtt/CSSGrammar.c"
    break;

  case 106: /* operator: ',' maybe_space  */
#line 673 "codec/webvtt/CSSGrammar.y"
                    {
        (yyval.character) = ',';
    }
#line 2559 "codec/webvtt/CSSGrammar.c"
    break;

  case 107: /* operator: %empty  */
#line 676 "codec/webvtt/CSSGrammar.y"
                {
        (yyval.character) = 0;
  }
#line 2567 "codec/webvtt/CSSGrammar.c"
    break;

  case 108: /* term: unary_term  */
#line 682 "codec/webvtt/CSSGrammar.y"
             { (yyval.term) = (yyvsp[0].term); }
#line 2573 "codec/webvtt/CSSGrammar.c"
    break;

  case 109: /* term: unary_operator unary_term  */
#line 683 "codec/webvtt/CSSGrammar.y"
                              {
      (yyval.term) = (yyvsp[0].term);
      (yyval.term).val *= (yyvsp[-1].integer);
  }
#line 2582 "codec/webvtt/CSSGrammar.c"
    break;

  case 110: /* term: STRING maybe_space  */
#line 687 "codec/webvtt/CSSGrammar.y"
                       { (yyval.term).type = TYPE_STRING; (yyval.term).psz = (yyvsp[-1].string); }
#line 2588 "codec/webvtt/CSSGrammar.c"
    break;

  case 111: /* term: IDENT maybe_space  */
#line 688 "codec/webvtt/CSSGrammar.y"
                      { (yyval.term).type = TYPE_IDENTIFIER; (yyval.term).psz = (yyvsp[-1].string); }
#line 2594 "codec/webvtt/CSSGrammar.c"
    break;

  case 112: /* term: DIMEN maybe_space  */
#line 690 "codec/webvtt/CSSGrammar.y"
                      { (yyval.term) = (yyvsp[-1].term); }
#line 2600 "codec/webvtt/CSSGrammar.c"
    break;

  case 113: /* term: unary_operator DIMEN maybe_space  */
#line 691 "codec/webvtt/CSSGrammar.y"
                                     { (yyval.term) = (yyvsp[-1].term); }
#line 2606 "codec/webvtt/CSSGrammar.c"
    break;

  case 114: /* term: URI maybe_space  */
#line 692 "codec/webvtt/CSSGrammar.y"
                    { (yyval.term).type = TYPE_URI; (yyval.term).psz = (yyvsp[-1].string); }
#line 2612 "codec/webvtt/CSSGrammar.c"
    break;

  case 115: /* term: UNICODERANGE maybe_space  */
#line 693 "codec/webvtt/CSSGrammar.y"
                             { (yyval.term).type = TYPE_UNICODERANGE; (yyval.term).psz = (yyvsp[-1].string); }
#line 2618 "codec/webvtt/CSSGrammar.c"
    break;

  case 116: /* term: IDSEL maybe_space  */
#line 694 "codec/webvtt/CSSGrammar.y"
                      { (yyval.term).type = TYPE_HEXCOLOR; (yyval.term).psz = (yyvsp[-1].string); }
#line 2624 "codec/webvtt/CSSGrammar.c"
    break;

  case 117: /* term: HASH maybe_space  */
#line 695 "codec/webvtt/CSSGrammar.y"
                     { (yyval.term).type = TYPE_HEXCOLOR; (yyval.term).psz = (yyvsp[-1].string); }
#line 2630 "codec/webvtt/CSSGrammar.c"
    break;

  case 118: /* term: '#' maybe_space  */
#line 696 "codec/webvtt/CSSGrammar.y"
                    { (yyval.term).type = TYPE_HEXCOLOR; (yyval.term).psz = NULL; }
#line 2636 "codec/webvtt/CSSGrammar.c"
    break;

  case 119: /* term: function  */
#line 698 "codec/webvtt/CSSGrammar.y"
             {
      (yyval.term) = (yyvsp[0].term);
  }
#line 2644 "codec/webvtt/CSSGrammar.c"
    break;

  case 120: /* term: '%' maybe_space  */
#line 701 "codec/webvtt/CSSGrammar.y"
                    { /* Handle width: %; */
      (yyval.term).type = TYPE_PERCENT; (yyval.term).val = 0;
  }
#line 2652 "codec/webvtt/CSSGrammar.c"
    break;

  case 127: /* function: FUNCTION maybe_space expr ')' maybe_space  */
#line 716 "codec/webvtt/CSSGrammar.y"
                                              {
        (yyval.term).type = TYPE_FUNCTION; (yyval.term).function = (yyvsp[-2].expr);
        (yyval.term).psz = (yyvsp[-4].string);
        if(*(yyval.term).psz != 0)
            (yyval.term).psz[strlen((yyval.term).psz) - 1] = 0;
    }
#line 2663 "codec/webvtt/CSSGrammar.c"
    break;

  case 128: /* function: FUNCTION maybe_space expr TOKEN_EOF  */
#line 722 "codec/webvtt/CSSGrammar.y"
                                        {
        (yyval.term).type = TYPE_FUNCTION; (yyval.term).function = (yyvsp[-1].expr); (yyval.term).psz = (yyvsp[-3].string);
        if(*(yyval.term).psz != 0)
            (yyval.term).psz[strlen((yyval.term).psz) - 1] = 0;
    }
#line 2673 "codec/webvtt/CSSGrammar.c"
    break;

  case 129: /* function: FUNCTION maybe_space ')' maybe_space  */
#line 727 "codec/webvtt/CSSGrammar.y"
                                         {
        (yyval.term).type = TYPE_FUNCTION; (yyval.term).function = NULL; (yyval.term).psz = (yyvsp[-3].string);
        if(*(yyval.term).psz != 0)
            (yyval.term).psz[strlen((yyval.term).psz) - 1] = 0;
    }
#line 2683 "codec/webvtt/CSSGrammar.c"
    break;

  case 130: /* function: FUNCTION maybe_space error  */
#line 732 "codec/webvtt/CSSGrammar.y"
                               {
        (yyval.term).type = TYPE_FUNCTION; (yyval.term).function = NULL; (yyval.term).psz = (yyvsp[-2].string);
        if(*(yyval.term).psz != 0)
            (yyval.term).psz[strlen((yyval.term).psz) - 1] = 0;
  }
#line 2693 "codec/webvtt/CSSGrammar.c"
    break;

  case 131: /* invalid_rule: error invalid_block  */
#line 742 "codec/webvtt/CSSGrammar.y"
                        {
        (yyval.rule) = NULL;
    }
#line 2701 "codec/webvtt/CSSGrammar.c"
    break;


#line 2705 "codec/webvtt/CSSGrammar.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (scanner, css_parser, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= TOKEN_EOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == TOKEN_EOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, scanner, css_parser);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, scanner, css_parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, css_parser, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner, css_parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, scanner, css_parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 769 "codec/webvtt/CSSGrammar.y"


#ifdef YYDEBUG
    int yydebug=1;
#else
    int yydebug=0;
#endif
