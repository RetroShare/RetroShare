/*
 * rs-core/src/dbase: rsexpr.cc
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


#include "dbase/findex.h"
#include "rsiface/rsexpr.h"
#include <algorithm>
#include <functional>

/******************************************************************************************
eval functions of relational expressions. 

******************************************************************************************/

template<>
void RelExpression<int>::linearize(LinearizedExpression& e) const
{
	e._ints.push_back(Op) ;
	e._ints.push_back(LowerValue) ;
	e._ints.push_back(HigherValue) ;
}


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

bool NameExpression::eval(FileEntry *file) 
{
	return evalStr(file->name);
}

bool PathExpression::eval(FileEntry *file){
	std::string path;
	/*Construct the path of this file*/
	DirEntry * curr = file->parent;
	while ( curr != NULL ){
		path = curr->name+"/"+ path;
		curr = curr->parent;
	}
	return evalStr(path);
}

bool ExtExpression::eval(FileEntry *file){
	std::string ext;
	/*Get the part of the string after the last instance of . in the filename */
	size_t index = file->name.find_last_of('.');
	if (index != std::string::npos) {
		ext = file->name.substr(index+1);
		if (ext != "" ){
			return evalStr(ext);	
		}
	}
	return false;
}

bool HashExpression::eval(FileEntry *file){
	return evalStr(file->hash);
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
static bool StrContains( std::string & str1, std::string & str2, 
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


bool StringExpression :: evalStr ( std::string &str ){
	std::list<std::string>::iterator iter;
	switch (Op) {
		case ContainsAllStrings:
			for ( iter = terms.begin(); iter != terms.end(); iter++ ) {	
				if ( StrContains (str, *iter, IgnoreCase) == false ){
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
							  
Expression *LinearizedExpression::toExpr(const LinearizedExpression& e,int& n_tok,int& n_ints,int& n_strings) 
{
	LinearizedExpression::token tok = static_cast<LinearizedExpression::token>(e._tokens[n_tok++]) ;

	switch(tok)
	{
		case EXPR_COMP:	{ 
									LogicalOperator op = static_cast<LogicalOperator>(e._ints[n_ints++]) ;

									Expression *e1 = toExpr(e,n_tok,n_ints,n_strings) ;
									Expression *e2 = toExpr(e,n_tok,n_ints,n_strings) ;

									return new CompoundExpression(op,e1,e2) ;
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
		case EXPR_DATE:  {
								  RelOperator op = static_cast<RelOperator>(e._ints[n_ints++]) ;
								  int lv = e._ints[n_ints++] ;
								  int hv = e._ints[n_ints++] ;

								  return new DateExpression(op,lv,hv) ;
							  }
		case EXPR_HASH:	{
									std::list<std::string> strings ;
									StringOperator op ;
									bool b ;

									readStringExpr(e,n_ints,n_strings,strings,b,op) ;
									return new HashExpression(op,strings) ;
								}
		case EXPR_EXT:
								{
									std::list<std::string> strings ;
									StringOperator op ;
									bool b ;

									readStringExpr(e,n_ints,n_strings,strings,b,op) ;

									return new ExtExpression(op,strings,b) ;
								}
		case EXPR_PATH:
								{
									std::list<std::string> strings ;
									StringOperator op ;
									bool b ;

									readStringExpr(e,n_ints,n_strings,strings,b,op) ;

									return new ExtExpression(op,strings,b) ;
								}
		case EXPR_NAME:	
								{
									std::list<std::string> strings ;
									StringOperator op ;
									bool b ;

									readStringExpr(e,n_ints,n_strings,strings,b,op) ;

									return new NameExpression(op,strings,b) ;
								}
		default:
								std::cerr << "No expression match the current value " << tok << std::endl ;
								return NULL ;
	}
}


