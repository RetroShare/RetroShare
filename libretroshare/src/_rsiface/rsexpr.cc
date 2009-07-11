#include "rsexpr.h"
#include "dbase/findex.h"
#include "rsexpr.h"
#include <algorithm>
#include <functional>


CompoundExpression::CompoundExpression( enum LogicalOperator op, Expression * exp1, Expression *exp2):
        Lexp(exp1),
        Rexp(exp2),
        Op(op)
{
}

bool CompoundExpression::eval (FileEntry *file)
{
    if (Lexp == NULL or Rexp == NULL) {
        return false;
    }
    switch (Op) {
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

CompoundExpression::~CompoundExpression()
{
    delete Lexp;
    delete Rexp;
}


StringExpression::StringExpression(enum StringOperator op, std::list<std::string> &t, bool ic) :
        Op(op),terms(t),
        IgnoreCase(ic)
{
}

/******************************************************************************************
eval functions of relational expressions.

******************************************************************************************/

//******** Releation expressions Constructors - Begin

DateExpression::DateExpression(enum RelOperator op, int v):
        RelExpression<int>(op,v,v)
{
}

DateExpression::DateExpression(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv)
{
}

SizeExpression::SizeExpression(enum RelOperator op, int v):
        RelExpression<int>(op,v,v)
{
}

SizeExpression::SizeExpression(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv)
{
}

SizeExpression::PopExpression(enum RelOperator op, int v):
        RelExpression<int>(op,v,v)
{
}

SizeExpression::PopExpression(enum RelOperator op, int lv, int hv):
        RelExpression<int>(op,lv,hv)
{
}
//******** Releation expressions Constructors - End

bool DateExpression::eval(FileEntry *file)
{
    return evalRel(file->modtime);
}

bool SizeExpression::eval(FileEntry *file)
{
    return evalRel(file->size);
}

bool PopExpression::eval(FileEntry *file)
{
    return evalRel(file->pop);
}

/******************************************************************************************
Code for evaluating string expressions

******************************************************************************************/

//******** String expressions Constructors - Begin
NameExpression::NameExpression(enum StringOperator op, std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic)
{
}

PathExpression::PathExpression(enum StringOperator op, std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic)
{
}

ExtExpression::ExtExpression(enum StringOperator op, std::list<std::string> &t, bool ic):
        StringExpression(op,t,ic)
{
}

HashExpression::HashExpression(enum StringOperator op, std::list<std::string> &t):
        StringExpression(op,t, true)
{
}
//******** String expressions Constructors - End


bool NameExpression::eval(FileEntry *file)
{
    return evalStr(file->name);
}

bool PathExpression::eval(FileEntry *file) {
    std::string path;
    /*Construct the path of this file*/
    DirEntry * curr = file->parent;
    while ( curr != NULL ) {
        path = curr->name+"/"+ path;
        curr = curr->parent;
    }
    return evalStr(path);
}

bool ExtExpression::eval(FileEntry *file) {
    std::string ext;
    /*Get the part of the string after the last instance of . in the filename */
    unsigned int index = file->name.find_last_of('.');
    if (index != std::string::npos) {
        ext = file->name.substr(index+1);
        if (ext != "" ) {
            return evalStr(ext);
        }
    }
    return false;
}

bool HashExpression::eval(FileEntry *file) {
    return evalStr(file->hash);
}

/*Check whether two strings are 'equal' to each other*/
static bool StrEquals(const std::string & str1, const std::string & str2,
                      bool IgnoreCase ) {
    if ( str1.size() != str2.size() ) {
        return false;
    } else if (IgnoreCase) {
        std::equal( str1.begin(), str1.end(),
                    str2.begin(), CompareCharIC() );
    }
    return std::equal( str1.begin(), str1.end(),
                       str2.begin());
}

/*Check whether one string contains the other*/
static bool StrContains( std::string & str1, std::string & str2,
                         bool IgnoreCase) {

    std::string::const_iterator iter ;
    if (IgnoreCase) {
        iter = std::search( str1.begin(), str1.end(),
                            str2.begin(), str2.end(), CompareCharIC() );
    } else {
        iter = std::search( str1.begin(), str1.end(),
                            str2.begin(), str2.end());
    }

    return ( iter != str1.end() );
}


bool StringExpression :: evalStr ( std::string &str ) {
    std::list<std::string>::iterator iter;
    switch (Op) {
    case ContainsAllStrings:
        for ( iter = terms.begin(); iter != terms.end(); iter++ ) {
            if ( StrContains (str, *iter, IgnoreCase) == false ) {
                return false;
            }
        }
        return true;
        break;
    case ContainsAnyStrings:
        for ( iter = terms.begin(); iter != terms.end(); iter++ ) {
            if ( StrContains (str,*iter, IgnoreCase) == true ) {
                return true;
            }
        }
        break;
    case EqualsString:
        for ( iter = terms.begin(); iter != terms.end(); iter++ ) {
            if ( StrEquals (str,*iter, IgnoreCase) == true ) {
                return true;
            }
        }
        break;
    default:
        return false;
    }
    return false;
}

template <class T>
RelExpression::RelExpression(enum RelOperator op, T lv, T hv):
        Op(op),
        LowerValue(lv),
        HigherValue(hv)
{
}

template <class T>
bool RelExpression<T>::evalRel(T val)
{
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

bool CompareCharIC::operator () ( char ch1 , char ch2 ) const
{
    return tolower( static_cast < unsigned char > (ch1) )
           == tolower( static_cast < unsigned char > (ch2) );
}


