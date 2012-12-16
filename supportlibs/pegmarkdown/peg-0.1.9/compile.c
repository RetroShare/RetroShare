/* Copyright (c) 2007, 2012 by Ian Piumarta
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 * 
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 * 
 * Last edited: 2012-04-29 16:09:36 by piumarta on emilia
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "version.h"
#include "tree.h"

static int yyl(void)
{
  static int prev= 0;
  return ++prev;
}

static void charClassSet  (unsigned char bits[], int c)	{ bits[c >> 3] |=  (1 << (c & 7)); }
static void charClassClear(unsigned char bits[], int c)	{ bits[c >> 3] &= ~(1 << (c & 7)); }

typedef void (*setter)(unsigned char bits[], int c);

static inline int oigit(int c)	{ return '0' <= c && c <= '7'; }

static int cnext(unsigned char **ccp)
{
    unsigned char *cclass= *ccp;
    int c= *cclass++;
    if (c)
    {
	if ('\\' == c && *cclass)
	{
	    switch (c= *cclass++)
	    {
		case 'a':  c= '\a'; break;	/* bel */
		case 'b':  c= '\b'; break;	/* bs */
		case 'e':  c= '\e'; break;	/* esc */
		case 'f':  c= '\f'; break;	/* ff */
		case 'n':  c= '\n'; break;	/* nl */
		case 'r':  c= '\r'; break;	/* cr */
		case 't':  c= '\t'; break;	/* ht */
		case 'v':  c= '\v'; break;	/* vt */
		default:
		    if (oigit(c))
		    {
			c -= '0';
			if (oigit(*cclass)) c= (c << 3) + *cclass++ - '0';
			if (oigit(*cclass)) c= (c << 3) + *cclass++ - '0';
		    }
		    break;
	    }
	}
	*ccp= cclass;
    }
    return c;
}

static char *makeCharClass(unsigned char *cclass)
{
  unsigned char	 bits[32];
  setter	 set;
  int		 c, prev= -1;
  static char	 string[256];
  char		*ptr;

  if ('^' == *cclass)
    {
      memset(bits, 255, 32);
      set= charClassClear;
      ++cclass;
    }
  else
    {
      memset(bits, 0, 32);
      set= charClassSet;
    }

  while (*cclass)
    {
      if ('-' == *cclass && cclass[1] && prev >= 0)
	{
	  ++cclass;
	  for (c= cnext(&cclass);  prev <= c;  ++prev)
	    set(bits, prev);
	  prev= -1;
	}
      else
	{
	  c= cnext(&cclass);
	  set(bits, prev= c);
	}
    }

  ptr= string;
  for (c= 0;  c < 32;  ++c)
    ptr += sprintf(ptr, "\\%03o", bits[c]);

  return string;
}

static void begin(void)		{ fprintf(output, "\n  {"); }
static void end(void)		{ fprintf(output, "\n  }"); }
static void label(int n)	{ fprintf(output, "\n  l%d:;\t", n); }
static void jump(int n)		{ fprintf(output, "  goto l%d;", n); }
static void save(int n)		{ fprintf(output, "  int yypos%d= ctx->pos, yythunkpos%d= ctx->thunkpos;", n, n); }
static void restore(int n)	{ fprintf(output,     "  ctx->pos= yypos%d; ctx->thunkpos= yythunkpos%d;", n, n); }

static void Node_compile_c_ko(Node *node, int ko)
{
  assert(node);
  switch (node->type)
    {
    case Rule:
      fprintf(stderr, "\ninternal error #1 (%s)\n", node->rule.name);
      exit(1);
      break;

    case Dot:
      fprintf(output, "  if (!yymatchDot(ctx)) goto l%d;", ko);
      break;

    case Name:
      fprintf(output, "  if (!yy_%s(ctx)) goto l%d;", node->name.rule->rule.name, ko);
      if (node->name.variable)
	fprintf(output, "  yyDo(ctx, yySet, %d, 0);", node->name.variable->variable.offset);
      break;

    case Character:
    case String:
      {
	int len= strlen(node->string.value);
	if (1 == len)
	  {
	    if ('\'' == node->string.value[0])
	      fprintf(output, "  if (!yymatchChar(ctx, '\\'')) goto l%d;", ko);
	    else
	      fprintf(output, "  if (!yymatchChar(ctx, '%s')) goto l%d;", node->string.value, ko);
	  }
	else
	  if (2 == len && '\\' == node->string.value[0])
	    fprintf(output, "  if (!yymatchChar(ctx, '%s')) goto l%d;", node->string.value, ko);
	  else 
	    fprintf(output, "  if (!yymatchString(ctx, \"%s\")) goto l%d;", node->string.value, ko);
      }
      break;

    case Class:
      fprintf(output, "  if (!yymatchClass(ctx, (unsigned char *)\"%s\")) goto l%d;", makeCharClass(node->cclass.value), ko);
      break;

    case Action:
      fprintf(output, "  yyDo(ctx, yy%s, ctx->begin, ctx->end);", node->action.name);
      break;

    case Predicate:
      fprintf(output, "  yyText(ctx, ctx->begin, ctx->end);  if (!(%s)) goto l%d;", node->action.text, ko);
      break;

    case Alternate:
      {
	int ok= yyl();
	begin();
	save(ok);
	for (node= node->alternate.first;  node;  node= node->alternate.next)
	  if (node->alternate.next)
	    {
	      int next= yyl();
	      Node_compile_c_ko(node, next);
	      jump(ok);
	      label(next);
	      restore(ok);
	    }
	  else
	    Node_compile_c_ko(node, ko);
	end();
	label(ok);
      }
      break;

    case Sequence:
      for (node= node->sequence.first;  node;  node= node->sequence.next)
	Node_compile_c_ko(node, ko);
      break;

    case PeekFor:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ko);
	restore(ok);
	end();
      }
      break;

    case PeekNot:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ok);
	jump(ko);
	label(ok);
	restore(ok);
	end();
      }
      break;

    case Query:
      {
	int qko= yyl(), qok= yyl();
	begin();
	save(qko);
	Node_compile_c_ko(node->query.element, qko);
	jump(qok);
	label(qko);
	restore(qko);
	end();
	label(qok);
      }
      break;

    case Star:
      {
	int again= yyl(), out= yyl();
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->star.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    case Plus:
      {
	int again= yyl(), out= yyl();
	Node_compile_c_ko(node->plus.element, ko);
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->plus.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    default:
      fprintf(stderr, "\nNode_compile_c_ko: illegal node type %d\n", node->type);
      exit(1);
    }
}


static int countVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      ++count;
      node= node->variable.next;
    }
  return count;
}

static void defineVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      fprintf(output, "#define %s ctx->val[%d]\n", node->variable.name, --count);
      node->variable.offset= count;
      node= node->variable.next;
    }
  fprintf(output, "#define yy ctx->yy\n");
  fprintf(output, "#define yypos ctx->pos\n");
  fprintf(output, "#define yythunkpos ctx->thunkpos\n");
}

static void undefineVariables(Node *node)
{
  fprintf(output, "#undef yythunkpos\n");
  fprintf(output, "#undef yypos\n");
  fprintf(output, "#undef yy\n");
  while (node)
    {
      fprintf(output, "#undef %s\n", node->variable.name);
      node= node->variable.next;
    }
}


static void Rule_compile_c2(Node *node)
{
  assert(node);
  assert(Rule == node->type);

  if (!node->rule.expression)
    fprintf(stderr, "rule '%s' used but not defined\n", node->rule.name);
  else
    {
      int ko= yyl(), safe;

      if ((!(RuleUsed & node->rule.flags)) && (node != start))
	fprintf(stderr, "rule '%s' defined but not used\n", node->rule.name);

      safe= ((Query == node->rule.expression->type) || (Star == node->rule.expression->type));

      fprintf(output, "\nYY_RULE(int) yy_%s(yycontext *ctx)\n{", node->rule.name);
      if (!safe) save(0);
      if (node->rule.variables)
	fprintf(output, "  yyDo(ctx, yyPush, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  yyprintf((stderr, \"%%s\\n\", \"%s\"));", node->rule.name);
      Node_compile_c_ko(node->rule.expression, ko);
      fprintf(output, "\n  yyprintf((stderr, \"  ok   %%s @ %%s\\n\", \"%s\", ctx->buf+ctx->pos));", node->rule.name);
      if (node->rule.variables)
	fprintf(output, "  yyDo(ctx, yyPop, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  return 1;");
      if (!safe)
	{
	  label(ko);
	  restore(0);
	  fprintf(output, "\n  yyprintf((stderr, \"  fail %%s @ %%s\\n\", \"%s\", ctx->buf+ctx->pos));", node->rule.name);
	  fprintf(output, "\n  return 0;");
	}
      fprintf(output, "\n}");
    }

  if (node->rule.next)
    Rule_compile_c2(node->rule.next);
}

static char *header= "\
#include <stdio.h>\n\
#include <stdlib.h>\n\
#include <string.h>\n\
";

static char *preamble= "\
#ifndef YY_LOCAL\n\
#define YY_LOCAL(T)	static T\n\
#endif\n\
#ifndef YY_ACTION\n\
#define YY_ACTION(T)	static T\n\
#endif\n\
#ifndef YY_RULE\n\
#define YY_RULE(T)	static T\n\
#endif\n\
#ifndef YY_PARSE\n\
#define YY_PARSE(T)	T\n\
#endif\n\
#ifndef YYPARSE\n\
#define YYPARSE		yyparse\n\
#endif\n\
#ifndef YYPARSEFROM\n\
#define YYPARSEFROM	yyparsefrom\n\
#endif\n\
#ifndef YY_INPUT\n\
#define YY_INPUT(buf, result, max_size)			\\\n\
  {							\\\n\
    int yyc= getchar();					\\\n\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\\\n\
    yyprintf((stderr, \"<%c>\", yyc));			\\\n\
  }\n\
#endif\n\
#ifndef YY_BEGIN\n\
#define YY_BEGIN	( ctx->begin= ctx->pos, 1)\n\
#endif\n\
#ifndef YY_END\n\
#define YY_END		( ctx->end= ctx->pos, 1)\n\
#endif\n\
#ifdef YY_DEBUG\n\
# define yyprintf(args)	fprintf args\n\
#else\n\
# define yyprintf(args)\n\
#endif\n\
#ifndef YYSTYPE\n\
#define YYSTYPE	int\n\
#endif\n\
\n\
#ifndef YY_PART\n\
\n\
typedef struct _yycontext yycontext;\n\
typedef void (*yyaction)(yycontext *ctx, char *yytext, int yyleng);\n\
typedef struct _yythunk { int begin, end;  yyaction  action;  struct _yythunk *next; } yythunk;\n\
\n\
struct _yycontext {\n\
  char     *buf;\n\
  int       buflen;\n\
  int       pos;\n\
  int       limit;\n\
  char     *text;\n\
  int       textlen;\n\
  int       begin;\n\
  int       end;\n\
  int       textmax;\n\
  yythunk  *thunks;\n\
  int       thunkslen;\n\
  int       thunkpos;\n\
  YYSTYPE   yy;\n\
  YYSTYPE  *val;\n\
  YYSTYPE  *vals;\n\
  int       valslen;\n\
#ifdef YY_CTX_MEMBERS\n\
  YY_CTX_MEMBERS\n\
#endif\n\
};\n\
\n\
#ifdef YY_CTX_LOCAL\n\
#define YY_CTX_PARAM_	yycontext *yyctx,\n\
#define YY_CTX_PARAM	yycontext *yyctx\n\
#define YY_CTX_ARG_	yyctx,\n\
#define YY_CTX_ARG	yyctx\n\
#else\n\
#define YY_CTX_PARAM_\n\
#define YY_CTX_PARAM\n\
#define YY_CTX_ARG_\n\
#define YY_CTX_ARG\n\
yycontext yyctx0;\n\
yycontext *yyctx= &yyctx0;\n\
#endif\n\
\n\
YY_LOCAL(int) yyrefill(yycontext *ctx)\n\
{\n\
  int yyn;\n\
  while (ctx->buflen - ctx->pos < 512)\n\
    {\n\
      ctx->buflen *= 2;\n\
      ctx->buf= (char *)realloc(ctx->buf, ctx->buflen);\n\
    }\n\
  YY_INPUT((ctx->buf + ctx->pos), yyn, (ctx->buflen - ctx->pos));\n\
  if (!yyn) return 0;\n\
  ctx->limit += yyn;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchDot(yycontext *ctx)\n\
{\n\
  if (ctx->pos >= ctx->limit && !yyrefill(ctx)) return 0;\n\
  ++ctx->pos;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchChar(yycontext *ctx, int c)\n\
{\n\
  if (ctx->pos >= ctx->limit && !yyrefill(ctx)) return 0;\n\
  if ((unsigned char)ctx->buf[ctx->pos] == c)\n\
    {\n\
      ++ctx->pos;\n\
      yyprintf((stderr, \"  ok   yymatchChar(ctx, %c) @ %s\\n\", c, ctx->buf+ctx->pos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchChar(ctx, %c) @ %s\\n\", c, ctx->buf+ctx->pos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(int) yymatchString(yycontext *ctx, char *s)\n\
{\n\
  int yysav= ctx->pos;\n\
  while (*s)\n\
    {\n\
      if (ctx->pos >= ctx->limit && !yyrefill(ctx)) return 0;\n\
      if (ctx->buf[ctx->pos] != *s)\n\
        {\n\
          ctx->pos= yysav;\n\
          return 0;\n\
        }\n\
      ++s;\n\
      ++ctx->pos;\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchClass(yycontext *ctx, unsigned char *bits)\n\
{\n\
  int c;\n\
  if (ctx->pos >= ctx->limit && !yyrefill(ctx)) return 0;\n\
  c= (unsigned char)ctx->buf[ctx->pos];\n\
  if (bits[c >> 3] & (1 << (c & 7)))\n\
    {\n\
      ++ctx->pos;\n\
      yyprintf((stderr, \"  ok   yymatchClass @ %s\\n\", ctx->buf+ctx->pos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchClass @ %s\\n\", ctx->buf+ctx->pos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(void) yyDo(yycontext *ctx, yyaction action, int begin, int end)\n\
{\n\
  while (ctx->thunkpos >= ctx->thunkslen)\n\
    {\n\
      ctx->thunkslen *= 2;\n\
      ctx->thunks= (yythunk *)realloc(ctx->thunks, sizeof(yythunk) * ctx->thunkslen);\n\
    }\n\
  ctx->thunks[ctx->thunkpos].begin=  begin;\n\
  ctx->thunks[ctx->thunkpos].end=    end;\n\
  ctx->thunks[ctx->thunkpos].action= action;\n\
  ++ctx->thunkpos;\n\
}\n\
\n\
YY_LOCAL(int) yyText(yycontext *ctx, int begin, int end)\n\
{\n\
  int yyleng= end - begin;\n\
  if (yyleng <= 0)\n\
    yyleng= 0;\n\
  else\n\
    {\n\
      while (ctx->textlen < (yyleng + 1))\n\
	{\n\
	  ctx->textlen *= 2;\n\
	  ctx->text= (char *)realloc(ctx->text, ctx->textlen);\n\
	}\n\
      memcpy(ctx->text, ctx->buf + begin, yyleng);\n\
    }\n\
  ctx->text[yyleng]= '\\0';\n\
  return yyleng;\n\
}\n\
\n\
YY_LOCAL(void) yyDone(yycontext *ctx)\n\
{\n\
  int pos;\n\
  for (pos= 0;  pos < ctx->thunkpos;  ++pos)\n\
    {\n\
      yythunk *thunk= &ctx->thunks[pos];\n\
      int yyleng= thunk->end ? yyText(ctx, thunk->begin, thunk->end) : thunk->begin;\n\
      yyprintf((stderr, \"DO [%d] %p %s\\n\", pos, thunk->action, ctx->text));\n\
      thunk->action(ctx, ctx->text, yyleng);\n\
    }\n\
  ctx->thunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(void) yyCommit(yycontext *ctx)\n\
{\n\
  if ((ctx->limit -= ctx->pos))\n\
    {\n\
      memmove(ctx->buf, ctx->buf + ctx->pos, ctx->limit);\n\
    }\n\
  ctx->begin -= ctx->pos;\n\
  ctx->end -= ctx->pos;\n\
  ctx->pos= ctx->thunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(int) yyAccept(yycontext *ctx, int tp0)\n\
{\n\
  if (tp0)\n\
    {\n\
      fprintf(stderr, \"accept denied at %d\\n\", tp0);\n\
      return 0;\n\
    }\n\
  else\n\
    {\n\
      yyDone(ctx);\n\
      yyCommit(ctx);\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(void) yyPush(yycontext *ctx, char *text, int count)  { ctx->val += count; }\n\
YY_LOCAL(void) yyPop(yycontext *ctx, char *text, int count)   { ctx->val -= count; }\n\
YY_LOCAL(void) yySet(yycontext *ctx, char *text, int count)   { ctx->val[count]= ctx->yy; }\n\
\n\
#endif /* YY_PART */\n\
\n\
#define	YYACCEPT	yyAccept(ctx, yythunkpos0)\n\
\n\
";

static char *footer= "\n\
\n\
#ifndef YY_PART\n\
\n\
typedef int (*yyrule)(yycontext *ctx);\n\
\n\
YY_PARSE(int) YYPARSEFROM(YY_CTX_PARAM_ yyrule yystart)\n\
{\n\
  int yyok;\n\
  if (!yyctx->buflen)\n\
    {\n\
      yyctx->buflen= 1024;\n\
      yyctx->buf= (char *)malloc(yyctx->buflen);\n\
      yyctx->textlen= 1024;\n\
      yyctx->text= (char *)malloc(yyctx->textlen);\n\
      yyctx->thunkslen= 32;\n\
      yyctx->thunks= (yythunk *)malloc(sizeof(yythunk) * yyctx->thunkslen);\n\
      yyctx->valslen= 32;\n\
      yyctx->vals= (YYSTYPE *)malloc(sizeof(YYSTYPE) * yyctx->valslen);\n\
      yyctx->begin= yyctx->end= yyctx->pos= yyctx->limit= yyctx->thunkpos= 0;\n\
    }\n\
  yyctx->begin= yyctx->end= yyctx->pos;\n\
  yyctx->thunkpos= 0;\n\
  yyctx->val= yyctx->vals;\n\
  yyok= yystart(yyctx);\n\
  if (yyok) yyDone(yyctx);\n\
  yyCommit(yyctx);\n\
  return yyok;\n\
}\n\
\n\
YY_PARSE(int) YYPARSE(YY_CTX_PARAM)\n\
{\n\
  return YYPARSEFROM(YY_CTX_ARG_ yy_%s);\n\
}\n\
\n\
#endif\n\
";

void Rule_compile_c_header(void)
{
  fprintf(output, "/* A recursive-descent parser generated by peg %d.%d.%d */\n", PEG_MAJOR, PEG_MINOR, PEG_LEVEL);
  fprintf(output, "\n");
  fprintf(output, "%s", header);
  fprintf(output, "#define YYRULECOUNT %d\n", ruleCount);
}

int consumesInput(Node *node)
{
  if (!node) return 0;

  switch (node->type)
    {
    case Rule:
      {
	int result= 0;
	if (RuleReached & node->rule.flags)
	  fprintf(stderr, "possible infinite left recursion in rule '%s'\n", node->rule.name);
	else
	  {
	    node->rule.flags |= RuleReached;
	    result= consumesInput(node->rule.expression);
	    node->rule.flags &= ~RuleReached;
	  }
	return result;
      }
      break;

    case Dot:		return 1;
    case Name:		return consumesInput(node->name.rule);
    case Character:
    case String:	return strlen(node->string.value) > 0;
    case Class:		return 1;
    case Action:	return 0;
    case Predicate:	return 0;

    case Alternate:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (!consumesInput(n))
	    return 0;
      }
      return 1;

    case Sequence:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (consumesInput(n))
	    return 1;
      }
      return 0;

    case PeekFor:	return 0;
    case PeekNot:	return 0;
    case Query:		return 0;
    case Star:		return 0;
    case Plus:		return consumesInput(node->plus.element);

    default:
      fprintf(stderr, "\nconsumesInput: illegal node type %d\n", node->type);
      exit(1);
    }
  return 0;
}


void Rule_compile_c(Node *node)
{
  Node *n;

  for (n= rules;  n;  n= n->rule.next)
    consumesInput(n);

  fprintf(output, "%s", preamble);
  for (n= node;  n;  n= n->rule.next)
    fprintf(output, "YY_RULE(int) yy_%s(yycontext *ctx); /* %d */\n", n->rule.name, n->rule.id);
  fprintf(output, "\n");
  for (n= actions;  n;  n= n->action.list)
    {
      fprintf(output, "YY_ACTION(void) yy%s(yycontext *ctx, char *yytext, int yyleng)\n{\n", n->action.name);
      defineVariables(n->action.rule->rule.variables);
      fprintf(output, "  yyprintf((stderr, \"do yy%s\\n\"));\n", n->action.name);
      fprintf(output, "  %s;\n", n->action.text);
      undefineVariables(n->action.rule->rule.variables);
      fprintf(output, "}\n");
    }
  Rule_compile_c2(node);
  fprintf(output, footer, start->rule.name);
}
