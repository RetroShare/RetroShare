/*******************************************************************************
 * libretroshare/src/retroshare: rsexpr.cc                                     *
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

#include "retroshare/rsexpr.h"
#include "retroshare/rstypes.h"
#include <algorithm>
#include <functional>

/******************************************************************************************
eval functions of relational expressions. 

******************************************************************************************/

namespace RsRegularExpression
{
template<>
void RelExpression<int>::linearize(LinearizedExpression& e) const
{
    e._ints.push_back(Op) ;
    e._ints.push_back(LowerValue) ;
    e._ints.push_back(HigherValue) ;
}


bool DateExpression::eval(const ExpFileEntry& file)
{
    return evalRel(file.file_modtime());
}

bool SizeExpressionMB::eval(const ExpFileEntry& file)
{
    return evalRel((int)(file.file_size()/(uint64_t)(1024*1024)));
}

bool SizeExpression::eval(const ExpFileEntry& file)
{
    return evalRel(file.file_size());
}

bool PopExpression::eval(const ExpFileEntry& file)
{
    return evalRel(file.file_popularity());
}

/******************************************************************************************
Code for evaluating string expressions

******************************************************************************************/

bool NameExpression::eval(const ExpFileEntry& file)
{
    return evalStr(file.file_name());
}

bool PathExpression::eval(const ExpFileEntry& file)
{
    return evalStr(file.file_parent_path());
}

bool ExtExpression::eval(const ExpFileEntry& file)
{
    std::string ext;
    /*Get the part of the string after the last instance of . in the filename */
    size_t index = file.file_name().find_last_of('.');
    if (index != std::string::npos) {
        ext = file.file_name().substr(index+1);
        if (ext != "" ){
            return evalStr(ext);
        }
    }
    return false;
}

bool HashExpression::eval(const ExpFileEntry& file){
    return evalStr(file.file_hash().toStdString());
}

/*Check whether two strings are 'equal' to each other*/
static bool StrEquals(const std::string & str1, const std::string & str2, 
                      bool IgnoreCase ){
    if ( str1.size() != str2.size() ){
        return false;
    } else if (IgnoreCase) {
        std::equal( str1.begin(), str1.end(),
                    str2.begin(), CompareCharIC() );
    }
    return std::equal( str1.begin(), str1.end(),
                       str2.begin());
}

/*Check whether one string contains the other*/
static bool StrContains( const std::string & str1, const std::string & str2,
                         bool IgnoreCase){

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


std::string StringExpression::toStdStringWithParam(const std::string& varstr) const
{
	std::string strlist ;
	for (auto iter = terms.begin(); iter != terms.end(); ++iter )
		strlist += *iter + " ";

	if(!strlist.empty())
		strlist.resize(strlist.size()-1);	//strlist.pop_back();	// pops the last ",". c++11 is needed for pop_back()

	switch(Op)
	{
	case ContainsAllStrings:  return varstr + " CONTAINS ALL "+strlist ;
	case ContainsAnyStrings:  if(terms.size() == 1)
			return varstr + " CONTAINS "+strlist ;
		else
			return varstr + " CONTAINS ONE OF "+strlist ;
	case EqualsString:  	  if(terms.size() == 1)
			return varstr + " IS "+strlist ;
		else
			return varstr + " IS ONE OF "+strlist ;

	default:
		return "" ;
	}
}

bool StringExpression :: evalStr ( const std::string &str ){
    std::list<std::string>::iterator iter;
    switch (Op) {
    case ContainsAllStrings:
        for ( iter = terms.begin(); iter != terms.end(); ++iter ) {
            if ( StrContains (str, *iter, IgnoreCase) == false ){
                return false;
            }
        }
        return true;
        break;
    case ContainsAnyStrings:
        for ( iter = terms.begin(); iter != terms.end(); ++iter ) {
            if ( StrContains (str,*iter, IgnoreCase) == true ) {
                return true;
            }
        }
        break;
    case EqualsString:
        for ( iter = terms.begin(); iter != terms.end(); ++iter ) {
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

/*************************************************************************
 * linearization code
 *************************************************************************/

void CompoundExpression::linearize(LinearizedExpression& e) const
{
    e._tokens.push_back(LinearizedExpression::EXPR_COMP) ;
    e._ints.push_back(Op) ;

    Lexp->linearize(e) ;
    Rexp->linearize(e) ;
}

void StringExpression::linearize(LinearizedExpression& e) const
{
    e._ints.push_back(Op) ;
    e._ints.push_back(IgnoreCase) ;
    e._ints.push_back(terms.size()) ;

    for(std::list<std::string>::const_iterator it(terms.begin());it!=terms.end();++it)
        e._strings.push_back(*it) ;
}

Expression *LinearizedExpression::toExpr(const LinearizedExpression& e) 
{
    int i=0,j=0,k=0 ;
    return toExpr(e,i,j,k) ;
}

void LinearizedExpression::readStringExpr(const LinearizedExpression& e,int& n_ints,int& n_strings,std::list<std::string>& strings,bool& b,StringOperator& op) 
{
    op = static_cast<StringOperator>(e._ints[n_ints++]) ;
    b = e._ints[n_ints++] ;
    int n = e._ints[n_ints++] ;

    strings.clear() ;
    for(int i=0;i<n;++i)
        strings.push_back(e._strings[n_strings++]) ;
}

std::string LinearizedExpression::GetStrings()
{
	std::string str;
	for (std::vector<std::string>::const_iterator i = this->_strings.begin(); i != this->_strings.end(); ++i)
	{
		str += *i;
		str += " ";
	}
	return str;
}

Expression *LinearizedExpression::toExpr(const LinearizedExpression& e,int& n_tok,int& n_ints,int& n_strings) 
{
    LinearizedExpression::token tok = static_cast<LinearizedExpression::token>(e._tokens[n_tok++]) ;

    switch(tok)
    {
    case EXPR_DATE:  {
        RelOperator op = static_cast<RelOperator>(e._ints[n_ints++]) ;
        int lv = e._ints[n_ints++] ;
        int hv = e._ints[n_ints++] ;

        return new DateExpression(op,lv,hv) ;
    }

    case EXPR_POP:	  {
        RelOperator op = static_cast<RelOperator>(e._ints[n_ints++]) ;
        int lv = e._ints[n_ints++] ;
        int hv = e._ints[n_ints++] ;

        return new PopExpression(op,lv,hv) ;
    }
    case EXPR_SIZE:  {
        RelOperator op = static_cast<RelOperator>(e._ints[n_ints++]) ;
        int lv = e._ints[n_ints++] ;
        int hv = e._ints[n_ints++] ;

        return new SizeExpression(op,lv,hv) ;
    }
    case EXPR_HASH:	{
        std::list<std::string> strings ;
        StringOperator op ;
        bool b ;

        readStringExpr(e,n_ints,n_strings,strings,b,op) ;
        return new HashExpression(op,strings) ;
    }
    case EXPR_NAME:	{
        std::list<std::string> strings ;
        StringOperator op ;
        bool b ;

        readStringExpr(e,n_ints,n_strings,strings,b,op) ;

        return new NameExpression(op,strings,b) ;
    }
    case EXPR_PATH: {
        std::list<std::string> strings ;
        StringOperator op ;
        bool b ;

        readStringExpr(e,n_ints,n_strings,strings,b,op) ;

        return new ExtExpression(op,strings,b) ;
    }
    case EXPR_EXT: {
        std::list<std::string> strings ;
        StringOperator op ;
        bool b ;

        readStringExpr(e,n_ints,n_strings,strings,b,op) ;

        return new ExtExpression(op,strings,b) ;
    }
    case EXPR_COMP:	{
        LogicalOperator op = static_cast<LogicalOperator>(e._ints[n_ints++]) ;

        Expression *e1 = toExpr(e,n_tok,n_ints,n_strings) ;
        Expression *e2 = toExpr(e,n_tok,n_ints,n_strings) ;

        return new CompoundExpression(op,e1,e2) ;
    }
    case EXPR_SIZE_MB: {
        RelOperator op = static_cast<RelOperator>(e._ints[n_ints++]) ;
        int lv = e._ints[n_ints++] ;
        int hv = e._ints[n_ints++] ;

        return new SizeExpressionMB(op,lv,hv) ;
    }
    default:
        std::cerr << "No expression match the current value " << tok << std::endl ;
        return NULL ;
    }
}


}
