/*******************************************************************************
 * libretroshare/src/retroshare: rsexpr.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Kashif Kaleem                                        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <string>
#include <list>
#include <stdint.h>

#include "util/rsprint.h"
#include "retroshare/rstypes.h"
#include "util/rstime.h"

/******************************************************************************************
Enumerations defining the Operators usable in the Boolean search expressions
******************************************************************************************/

namespace RsRegularExpression
{
enum LogicalOperator{
    AndOp=0,	/* exp AND exp */
    OrOp=1,	/* exp OR exp */
    XorOp=2		/* exp XOR exp */
};


/* Operators for String Queries */
enum StringOperator{
    ContainsAnyStrings = 0,	/* e.g. name contains any of 'conference' 'meeting' 'presentation' */
    ContainsAllStrings = 1,	/* same as above except that it contains ALL of the strings */
    EqualsString       = 2	/* exactly equal*/
};

/* Comparison operators ( >, <, >=, <=, == and InRange ) */
enum RelOperator{
    Equals        = 0,
    GreaterEquals = 1,
    Greater       = 2,
    SmallerEquals = 3,
    Smaller       = 4,
    InRange       = 5		/* lower limit <= value <= upper limit*/
};

/********************************************************************************************
 * Helper class for further serialisation
 ********************************************************************************************/

class StringExpression ;
class Expression ;

class LinearizedExpression
{
public:
    std::vector<uint8_t> _tokens ;
    std::vector<uint32_t> _ints ;
    std::vector<std::string> _strings ;

    typedef enum {	EXPR_DATE    = 0,
                    EXPR_POP     = 1,
                    EXPR_SIZE    = 2,
                    EXPR_HASH    = 3,
                    EXPR_NAME    = 4,
                    EXPR_PATH    = 5,
                    EXPR_EXT     = 6,
                    EXPR_COMP    = 7,
                    EXPR_SIZE_MB = 8 } token ;

    static Expression *toExpr(const LinearizedExpression& e) ;
	
	std::string GetStrings();
	
private:
    static Expression *toExpr(const LinearizedExpression& e,int&,int&,int&) ;
    static void readStringExpr(const LinearizedExpression& e,int& n_ints,int& n_strings,std::list<std::string>& strings,bool& b,StringOperator& op) ;
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

/*!
 * \brief The ExpFileEntry class
 * 		This is the base class the regular expressions can operate on. Derive it to fit your own needs!
 */
class ExpFileEntry
{
public:
    virtual const std::string& file_name()        const =0;
    virtual uint64_t           file_size()        const =0;
    virtual rstime_t             file_modtime()     const =0;
    virtual uint32_t           file_popularity()  const =0;
    virtual std::string        file_parent_path() const =0;
    virtual const RsFileHash&  file_hash()        const =0;
};

class Expression
{
public:
    virtual bool eval (const ExpFileEntry& file) = 0;
    virtual ~Expression() {};

    virtual void linearize(LinearizedExpression& e) const = 0 ;
	virtual std::string toStdString() const = 0 ;
};

class CompoundExpression : public Expression 
{
public:
    CompoundExpression( enum LogicalOperator op, Expression * exp1, Expression *exp2)
        : Lexp(exp1), Rexp(exp2), Op(op){ }

    bool eval (const ExpFileEntry& file) {
        if (Lexp == NULL or Rexp == NULL) {
            return false;
        }
        switch (Op){
        case AndOp:
            return Lexp->eval(file) && Rexp->eval(file);
        case OrOp:
            return Lexp->eval(file) || Rexp->eval(file);
        case XorOp:
            return Lexp->eval(file) ^ Rexp->eval(file);
        default:
            return false;
        }
    }
    virtual ~CompoundExpression(){
        delete Lexp;
        delete Rexp;
    }

	virtual std::string toStdString() const
	{
		switch(Op)
		{
		case AndOp: return "(" + Lexp->toStdString() + ") AND (" + Rexp->toStdString() +")" ;
		case  OrOp: return "(" + Lexp->toStdString() + ") OR (" + Rexp->toStdString() +")" ;
		case XorOp: return "(" + Lexp->toStdString() + ") XOR (" + Rexp->toStdString() +")" ;
		default:
			return "" ;
		}
	}

    virtual void linearize(LinearizedExpression& e) const ;
private:
    Expression *Lexp;
    Expression *Rexp;
    enum LogicalOperator Op;

};

class StringExpression: public Expression 
{
public:
    StringExpression(enum StringOperator op, const std::list<std::string> &t, bool ic): Op(op),terms(t), IgnoreCase(ic){}

    virtual void linearize(LinearizedExpression& e) const ;
	virtual std::string toStdStringWithParam(const std::string& varstr) const;
protected:
    bool evalStr(const std::string &str);

    enum StringOperator Op;
    std::list<std::string> terms;
    bool IgnoreCase;
};

template <class T>
class RelExpression: public Expression 
{
public:
    RelExpression(enum RelOperator op, T lv, T hv): Op(op), LowerValue(lv), HigherValue(hv) {}

    virtual void linearize(LinearizedExpression& e) const ;
	virtual std::string toStdStringWithParam(const std::string& typestr) const;
protected:
    bool evalRel(T val);

    enum RelOperator Op;
    T LowerValue;
    T HigherValue;
};

template<> void RelExpression<int>::linearize(LinearizedExpression& e) const ;

template <class T>
bool RelExpression<T>::evalRel(T val) {
    switch (Op) {
    case Equals:
        return LowerValue == val;
    case GreaterEquals:
        return LowerValue >= val;
    case Greater:
        return LowerValue > val;
    case SmallerEquals:
        return LowerValue <= val;
    case Smaller:
        return LowerValue < val;
    case InRange:
        return (LowerValue <= val) && (val <= HigherValue);
    default:
        return false;
    }
}

template <class T>
std::string RelExpression<T>::toStdStringWithParam(const std::string& typestr) const
{
	std::string LowerValueStr = RsUtil::NumberToString(LowerValue) ;

    switch (Op) {
    case Equals:		 return typestr + " = " + LowerValueStr ;
    case GreaterEquals:  return typestr + " <= "+ LowerValueStr ;
    case Greater:		 return typestr + " < " + LowerValueStr ;
    case SmallerEquals:  return typestr + " >= "+ LowerValueStr ;
    case Smaller:		 return typestr + " > " + LowerValueStr ;
    case InRange:		 return LowerValueStr + " <= " + typestr + " <= " + RsUtil::NumberToString(HigherValue) ;
    default:
        return "";
    }
}

/******************************************************************************************
Binary Predicate for Case Insensitive search 

******************************************************************************************/
/*Binary predicate for case insensitive character comparison.*/
/*TODOS:
 *Factor locales in the comparison
 */
struct CompareCharIC :
        public std::binary_function< char , char , bool> {

    bool operator () ( char ch1 , char ch2 ) const {
        return tolower( static_cast < unsigned char > (ch1) )
                == tolower( static_cast < unsigned char > (ch2) );
    }

};

/******************************************************************************************
Some implementations of StringExpressions. 

******************************************************************************************/

class NameExpression: public StringExpression 
{
public:
    NameExpression(enum StringOperator op, const std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return StringExpression::toStdStringWithParam("NAME"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_NAME) ;
        StringExpression::linearize(e) ;
    }
};

class PathExpression: public StringExpression {
public:
    PathExpression(enum StringOperator op, const std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString()const { return StringExpression::toStdStringWithParam("PATH"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_PATH) ;
        StringExpression::linearize(e) ;
    }
};

class ExtExpression: public StringExpression {
public:
    ExtExpression(enum StringOperator op, const std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString()const { return StringExpression::toStdStringWithParam("EXTENSION"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_EXT) ;
        StringExpression::linearize(e) ;
    }
};

class HashExpression: public StringExpression {
public:
    HashExpression(enum StringOperator op, const std::list<std::string> &t):
        StringExpression(op,t, true) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return StringExpression::toStdStringWithParam("HASH"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_HASH) ;
        StringExpression::linearize(e) ;
    }
};

/******************************************************************************************
Some implementations of Relational Expressions.

******************************************************************************************/

class DateExpression: public RelExpression<int> 
{
public:
    DateExpression(enum RelOperator op, int v): RelExpression<int>(op,v,v){}
    DateExpression(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return RelExpression<int>::toStdStringWithParam("DATE"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_DATE) ;
        RelExpression<int>::linearize(e) ;
    }
};

class SizeExpression: public RelExpression<int> 
{
public:
    SizeExpression(enum RelOperator op, int v): RelExpression<int>(op,v,v){}
    SizeExpression(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return RelExpression<int>::toStdStringWithParam("SIZE"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_SIZE) ;
        RelExpression<int>::linearize(e) ;
    }
};

class SizeExpressionMB: public RelExpression<int> 
{
public:
    SizeExpressionMB(enum RelOperator op, int v): RelExpression<int>(op,v,v){}
    SizeExpressionMB(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv) {}
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return RelExpression<int>::toStdStringWithParam("SIZE"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_SIZE_MB) ;
        RelExpression<int>::linearize(e) ;
    }
};
class PopExpression: public RelExpression<int> 
{
public:
    PopExpression(enum RelOperator op, int v): RelExpression<int>(op,v,v){}
    PopExpression(enum RelOperator op, int lv, int hv): RelExpression<int>(op,lv,hv) {}
    PopExpression(const LinearizedExpression& e) ;
    bool eval(const ExpFileEntry& file);

	virtual std::string toStdString() const { return RelExpression<int>::toStdStringWithParam("POPULARITY"); }

    virtual void linearize(LinearizedExpression& e) const
    {
        e._tokens.push_back(LinearizedExpression::EXPR_POP) ;
        RelExpression<int>::linearize(e) ;
    }
};
}


