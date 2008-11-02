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
	unsigned int index = file->name.find_last_of('.');
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
