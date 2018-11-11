/*
 * libretroshare/src/gxp: gxp_apps.h
 *
 * General Exchange Protocol interface for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#ifndef RS_ACADEE_H
#define RS_ACADEE_H

/*******
 * Stores a Bibliography of Academic Articles, with links to allow you to access the actual article.
 * The data fields, should contain enough information to 
 *  - extract full biblio (bibtex or ris formats).
 *  - read abstract.
 *  - review / rating of the article
 *  - references to similar papers, and bibliography.
 *  - keywords, and the like.
 * 
 * It will be possible to have multiple identical / similar / different descriptions of the same article.
 * The class will have to handle this: sorting and matching as best it can.
 *
 ****/

class gxp::Paper
{
	/* fields from ris */
	std::string reftype;
	std::string journal;
	std::string title;
	std::string issuetitle;
	uint32_t    volume;
	uint32_t    issue;
	std::string publisher;
	std::string serialnumber;
	std::string url;
	std::list<std::string> authors;
	rstime_t      date;
	uint32_t    startpage;
	uint32_t    endpage;	
	std::string language;

	std::string abstract;

	// KeyWords <- go into hashtags (parent class)
	//References & Similar Papers <- go into links (parent class)
};


class rsAcadee: public rsGmxp
{
	public:

	/* we want to access the  */




};


#endif /* RS_GXP_H */


