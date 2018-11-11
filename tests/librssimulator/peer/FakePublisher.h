/*******************************************************************************
 * librssimulator/peer/: FakePublisher.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/
#pragma once

#include <list>

#include <serialiser/rsserial.h>
#include <pqi/pqiservice.h>

class FakePublisher: public pqiPublisher
{
	public:
		virtual bool sendItem(RsRawItem *item) 
		{
			_item_queue.push_back(item) ;
			return true;
		}

		RsRawItem *outgoing() 
		{
			if(_item_queue.empty())
                		return NULL ;

			RsRawItem *item = _item_queue.front() ;
				_item_queue.pop_front() ;
			return item ;
		}

		bool outgoingEmpty()
		{
			return _item_queue.empty();
		}

	private:
		std::list<RsRawItem*> _item_queue ;
};

