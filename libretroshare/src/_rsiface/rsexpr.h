#ifndef RS_EXPRESSIONS_H
#define RS_EXPRESSIONS_H

/*
 * rs-core/src/rsiface: rsexpr.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Kashif Kaleem.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */


#include <string>
#include <list>

/******************************************************************************************
Enumerations defining the Operators usable in the Boolean search expressions
******************************************************************************************/


enum LogicalOperator {
    AndOp=0,	/* exp AND exp */
    OrOp,		/* exp OR exp */
    XorOp		/* exp XOR exp */
};


/*Operators for String Queries*/
enum StringOperator {
    ContainsAnyStrings = 0,	/* e.g. name contains any of 'conference' 'meeting' 'presentation' */
    ContainsAllStrings,		/* same as above except that it contains ALL of the strings */
    EqualsString			/* exactly equal*/
};

/*Relational operators ( >, <, >=, <=, == and InRange )*/
enum RelOperator {
    Equals = 0,
    GreaterEquals,
    Greater,
    SmallerEquals,
    Smaller,
    InRange		/* lower limit <= value <= upper limit*/
};

/******************************************************************************************
Boolean Search Expression
classes:

	Expression: 		The base class of all expression typest
	CompoundExpression: The expression which uses a logical operator to combine
							the results of two expressions
	StringExpression: 	An expression which uses some sort of string comparison.
	RelExpression: 		A Relational Expression where > < >= <= == make sense.
							e.g. size date etc

******************************************************************************************/

class FileEntry;

class Expression
{
public:
    virtual bool eval (FileEntry *file) = 0;
    virtual ~Expression();
};


class CompoundExpression : public Expression
{
public:
    CompoundExpression( enum LogicalOperator op, Expression * exp1, Expression *exp2);

    bool eval (FileEntry *file);
    virtual ~CompoundExpression();
private:
    Expression *Lexp;
    Expression *Rexp;
    enum LogicalOperator Op;

};

class StringExpression: public Expression
{
public:
    StringExpression(enum StringOperator op, std::list<std::string> &t, bool ic);
protected:
    bool evalStr(std::string &str);
private:
    enum StringOperator Op;
    std::list<std::string> terms;
    bool IgnoreCase;
};

template <class T>
class RelExpression: public Expression {
public:
    RelExpression(enum RelOperator op, T lv, T hv):
            Op(op), LowerValue(lv), HigherValue(hv) {}
protected:
    bool evalRel(T val);
private:
    enum RelOperator Op;
    T LowerValue;
    T HigherValue;
};



/******************************************************************************************
Binary Predicate for Case Insensitive search

******************************************************************************************/
/*Binary predicate for case insensitive character comparison.*/
/*TODOS:
 *Factor locales in the comparison
 */
struct CompareCharIC :
            public std::binary_function< char , char , bool>
{
    bool operator () ( char ch1 , char ch2 ) const;
};

/******************************************************************************************
Some implementations of StringExpressions.

******************************************************************************************/

class NameExpression: public StringExpression
{
public:
    NameExpression(enum StringOperator op, std::list<std::string> &t, bool ic);

    bool eval(FileEntry *file);
};

class PathExpression: public StringExpression
{
public:
    PathExpression(enum StringOperator op, std::list<std::string> &t, bool ic);

    bool eval(FileEntry *file);
};

class ExtExpression: public StringExpression {
public:
    ExtExpression(enum StringOperator op, std::list<std::string> &t, bool ic);

    bool eval(FileEntry *file);
};

class HashExpression: public StringExpression {
public:
    HashExpression(enum StringOperator op, std::list<std::string> &t);

    bool eval(FileEntry *file);
};

/******************************************************************************************
Some implementations of Relational Expressions.

******************************************************************************************/

class DateExpression: public RelExpression<int>
{
public:
    DateExpression(enum RelOperator op, int v);
    DateExpression(enum RelOperator op, int lv, int hv);

    bool eval(FileEntry *file);
};

class SizeExpression: public RelExpression<int>
{
public:
    SizeExpression(enum RelOperator op, int v);
    SizeExpression(enum RelOperator op, int lv, int hv);

    bool eval(FileEntry *file);
};

class PopExpression: public RelExpression<int> {
public:
    PopExpression(enum RelOperator op, int v);
    PopExpression(enum RelOperator op, int lv, int hv);

    bool eval(FileEntry *file);
};

#endif /* RS_EXPRESSIONS_H */
