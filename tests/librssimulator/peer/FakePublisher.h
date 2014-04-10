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

