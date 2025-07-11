/* A Bison parser, made by GNU Bison 3.8.2.  */

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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_HOME_SYRIX_COMPILE_MINIC_EXPR_FRONTEND_FLEXBISON_AUTOGENERATED_MINICBISON_H_INCLUDED
# define YY_YY_HOME_SYRIX_COMPILE_MINIC_EXPR_FRONTEND_FLEXBISON_AUTOGENERATED_MINICBISON_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
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
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    T_DIGIT = 258,                 /* T_DIGIT  */
    T_ID = 259,                    /* T_ID  */
    T_INT = 260,                   /* T_INT  */
    T_RETURN = 261,                /* T_RETURN  */
    T_SEMICOLON = 262,             /* T_SEMICOLON  */
    T_L_PAREN = 263,               /* T_L_PAREN  */
    T_R_PAREN = 264,               /* T_R_PAREN  */
    T_L_BRACE = 265,               /* T_L_BRACE  */
    T_R_BRACE = 266,               /* T_R_BRACE  */
    T_COMMA = 267,                 /* T_COMMA  */
    T_ASSIGN = 268,                /* T_ASSIGN  */
    T_SUB = 269,                   /* T_SUB  */
    T_ADD = 270                    /* T_ADD  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 22 "/home/syrix/compile/minic-expr/frontend/flexbison/MiniC.y"

    class ast_node * node;

    struct digit_int_attr integer_num;
    struct digit_real_attr float_num;
    struct var_id_attr var_id;
    struct type_attr type;
    int op_class;

#line 89 "/home/syrix/compile/minic-expr/frontend/flexbison/autogenerated/MiniCBison.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_HOME_SYRIX_COMPILE_MINIC_EXPR_FRONTEND_FLEXBISON_AUTOGENERATED_MINICBISON_H_INCLUDED  */
