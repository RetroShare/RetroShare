/*
 * libretroshare/src/pqi: cleanupxpgp.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2008 by Sourashis Roy
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

#include "cleanupxpgp.h"
#include <iostream>
#include <string.h>  //strlen

/*
Method for cleaning up the certificate. This method removes any unnecessay white spaces and unnecessary
new line characters in the certificate. Also it makes sure that there are 64 characters per line in 
the certificate. This function removes white spaces and new line characters in the entire segment
-----BEGIN XPGP CERTIFICATE-----
<CERTIFICATE>
-----END XPGP CERTIFICATE-----  
We also take care of correcting cases like -----   BEGIN. Here extra empty spaces
have been introduced between ----- and BEGIN. Similarly for the 
end tag we take care of cases like -----   END  XPGP . Here extra empty spaces have been 
introduced and the actual tag should have been -----END XPGP
*/

std::string cleanUpCertificate(std::string badCertificate)
{
	/*
	Buffer for storing the cleaned certificate. In certain cases the 
	cleanCertificate can be larger than the badCertificate
	*/
	std::string cleanCertificate;
	//The entire certificate begin tag
        const char * beginCertTag="-----BEGIN";
	//The entire certificate end tag
        const char * endCertTag="-----END";
	//Tag containing dots. The common part of both start and end tags 
        const char * commonTag="-----";
	//Only BEGIN part of the begin tag
        const char * beginTag="BEGIN";
	//Only END part of the end tag
        const char * endTag="END";
	//The start index of the ----- part of the certificate begin tag
        size_t beginCertStartIdx1=0;
	//The start index of the BEGIN part of the certificate begin tag
        size_t beginCertStartIdx2=0;
	//The start index of the end part(-----) of the certificate begin tag. The begin tag ends with -----. Example -----BEGIN XPGP CERTIFICATE-----
        size_t beginCertEndIdx=0;
	//The start index of the ----- part of the certificate end tag
        size_t endCertStartIdx1=0;
	//The start index of the END part of the certificate end tag
        size_t endCertStartIdx2=0;
	//The start index of the end part(-----) of the certificate end tag. The begin tag ends with -----. Example -----BEGIN XPGP CERTIFICATE-----
        size_t endCertEndIdx=0;
	//The length of the bad certificate.
        size_t lengthOfCert=badCertificate.length();
	//The current index value in the bad certificate
        size_t currBadCertIdx=0;
	//Temporary index value
        size_t tmpIdx=0;
	//Boolean flag showing if the begin tag or the end tag has been found
	bool found=false;
	/*
	Calculating the value of the beginCertStartIdx1 and beginCertStartIdx2. Here we first locate the occurance of ----- and then 
	the location of BEGIN. Next we check if there are any non space or non new-line characters between their occureance. If there are any other
	characters between the two(----- and BEGIN), other than space and new line then it means that it is the certificate begin tag. 
	Here we take care of the fact that we may have introduced some spaces and newlines in the begin tag by mistake. This
	takes care of the spaces and newlines between ----- and BEGIN.
	*/

	while(found==false && (beginCertStartIdx1=badCertificate.find(commonTag,tmpIdx))!=std::string::npos)
	{
		beginCertStartIdx2=badCertificate.find(beginTag,beginCertStartIdx1+strlen(commonTag));	
		tmpIdx=beginCertStartIdx1+strlen(commonTag);	
		if(beginCertStartIdx2!=std::string::npos)
		{
			found=true;
                        for(size_t i=beginCertStartIdx1+strlen(commonTag);i<beginCertStartIdx2;i++)
			{
				if(badCertificate[i]!=' ' && badCertificate[i]!='\n' )
				{
					found=false;
					break;
				}
			}
		}
		else
		{
			break;
		}
		
	}
	/*
	begin tag not found
	*/
	if(!found)
	{
		std::cerr<<"Certificate corrupted beyond repair: No <------BEGIN > tag"<<std::endl;		
		return badCertificate;	
	}
	beginCertEndIdx=badCertificate.find(commonTag,beginCertStartIdx2);
	if(beginCertEndIdx==std::string::npos)
	{	
		std::cerr<<"Certificate corrupted beyond repair: No <------BEGIN > tag"<<std::endl;		
		return badCertificate;	
	}
	tmpIdx=beginCertEndIdx+strlen(commonTag);
	found=false;
	/*
	Calculating the value of the endCertStartIdx1 and endCertStartIdx2. Here we first locate the occurance of ----- and then 
	the location of END. Next we check if there are any non space or non new-line characters between their occureance. If there are any other
	characters between the two(----- and END), other than space and new line then it means that it is the certificate end tag. 
	Here we take care of the fact that we may have introduced some spaces and newlines in the end tag by mistake. This
	takes care of the spaces and newlines between ----- and END.
	*/
	while(found==false && (endCertStartIdx1=badCertificate.find(commonTag,tmpIdx))!=std::string::npos)
	{
		endCertStartIdx2=badCertificate.find(endTag,endCertStartIdx1+strlen(commonTag));	
		tmpIdx=endCertStartIdx1+strlen(commonTag);	
		if(endCertStartIdx2!=std::string::npos)
		{
			found=true;
                        for(size_t i=endCertStartIdx1+strlen(commonTag);i<endCertStartIdx2;i++)
			{
				if(badCertificate[i]!=' '&& badCertificate[i]!='\n')
				{
					found=false;
					break;
				}
			}
		}
		else
		{
			break;
		}
		
	}
	/*
	end tag not found
	*/
	if(!found)
	{
		std::cerr<<"Certificate corrupted beyond repair: No <------END > tag"<<std::endl;		
		return badCertificate;
	}	
	endCertEndIdx=badCertificate.find(commonTag,endCertStartIdx2);
	if(endCertEndIdx==std::string::npos || endCertEndIdx>=lengthOfCert)
	{	
		std::cerr<<"Certificate corrupted beyond repair: No <------END > tag"<<std::endl;		
		return badCertificate;
	}
	/*
	Copying the begin tag(-----BEGIN) to the clean certificate
	*/
	cleanCertificate += beginCertTag;
	currBadCertIdx=beginCertStartIdx2+strlen(beginTag);
	/*
	Copying the name of the tag e.g XPGP CERTIFICATE. At the same time remove any white spaces and new line
	characters.
	*/
	while(currBadCertIdx<beginCertEndIdx)
	{
		if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
		}
		else if(badCertificate[currBadCertIdx]==' ' && (badCertificate[currBadCertIdx-1]==' '|| badCertificate[currBadCertIdx-1]=='\n') )
		{
			currBadCertIdx++;
		}
		else
		{
			cleanCertificate += badCertificate[currBadCertIdx];
			currBadCertIdx++;
		}
	}	
	/*
	If the last character is a space we need to remove it.
	*/
	if(cleanCertificate.substr(cleanCertificate.length()-1, 1) == " ")
	{
		cleanCertificate.erase(cleanCertificate.length()-1);
	}
	/*
	Copying the end part of the certificate start tag(-----). 
	*/
	cleanCertificate += commonTag;
	cleanCertificate += "\n";
	currBadCertIdx=currBadCertIdx+strlen(commonTag);  
	/*
	Remove the white spaces between the end of the certificate begin tag and the actual
	start of the certificate.
	*/
	while(badCertificate[currBadCertIdx]=='\n'|| badCertificate[currBadCertIdx]==' ')
	{
		currBadCertIdx++;
        }

        //keep the first line : gnupg version
        cleanCertificate += badCertificate[currBadCertIdx];
        currBadCertIdx++;
        while(badCertificate[currBadCertIdx]!='\n')
        {
                cleanCertificate += badCertificate[currBadCertIdx];
                currBadCertIdx++;
        }
        cleanCertificate += badCertificate[currBadCertIdx];
        currBadCertIdx++;

	//Start of the actual certificate. Remove spaces in the certificate
	//and make sure there are 64 characters per line in the
	//new cleaned certificate
	int cntPerLine=0;
	while(currBadCertIdx<endCertStartIdx1)
	{
		if(cntPerLine==64)
		{
			cleanCertificate += "\n";
			cntPerLine=0;
			continue;
		}
		else if(badCertificate[currBadCertIdx]==' ')
		{
			currBadCertIdx++;
			continue;
		}
		else if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
			continue;
		}
		cleanCertificate += badCertificate[currBadCertIdx];
		cntPerLine++;
		currBadCertIdx++;

	}
    if(cleanCertificate.substr(cleanCertificate.length()-1,1)!="\n")
    {
        cleanCertificate += "\n";
//        std::cerr<<"zeeeee"<<std::endl;
    }
    else
    {
//        std::cerr<<"zooooo"<<std::endl;
    }
	/*
	Copying the begining part of the certificate end tag. Copying
	-----END part of the tag.
	*/
	cleanCertificate += endCertTag;
	currBadCertIdx=endCertStartIdx2+strlen(endTag);
 	/*
	Copying the name of the certificate e.g XPGP CERTIFICATE. The end tag also has the
	the name of the tag.
	*/
	while(currBadCertIdx<endCertEndIdx)
	{
		if(badCertificate[currBadCertIdx]=='\n')
		{
			currBadCertIdx++;
		}
		else if( badCertificate[currBadCertIdx]==' ' && (badCertificate[currBadCertIdx-1]==' '|| badCertificate[currBadCertIdx-1]=='\n'))
		{
			currBadCertIdx++;
		}
		else
		{
			cleanCertificate += badCertificate[currBadCertIdx];
			currBadCertIdx++;
			
		}
	}	
	/*
	If the last character is a space we need to remove it.
	*/
	if(cleanCertificate.substr(cleanCertificate.length()-1,1)==" ")
	{
		cleanCertificate.erase(cleanCertificate.length()-1);
	}
	/*
	Copying the end part(-----) of the end tag in the certificate. 
	*/	
	cleanCertificate += commonTag;
	cleanCertificate += "\n";

	return  cleanCertificate;
}

int findEndIdxOfCertStartTag(std::string badCertificate)
{
        size_t idxTag1=0;
        size_t tmpIdx=0;
        size_t idxTag2=0;
        const char * tag1="---";
        const char * tag2="---";
	bool found=false;
	while(found==false && (idxTag1=badCertificate.find(tag1,tmpIdx))!=std::string::npos)
	{
		idxTag2=badCertificate.find(tag2,idxTag1+strlen(tag1));	
	
		if(idxTag2!=std::string::npos)
		{
			found=true;
                        for(size_t i=idxTag1+strlen(tag1);i<idxTag2;i++)
			{
				if(badCertificate[i]!=' ')
				{
					found=false;
					break;
				}
			}
		}
		else
		{
			break;
		}
		
	}
	return 1;	

}


