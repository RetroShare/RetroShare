
#include "pqi/pqipacket.h"
#include "pqi/pqi.h"

#include <iostream>
#include <sstream>
#include <iomanip>

int checkMsg(PQItem *item);

int main()
{

	MsgItem *item = new MsgItem();

	item -> sendTime = 0x10203040;
	item -> msg = "asdfghjklwertyuisdfghjkasdfghewrtyefrghfghdfghjklghjkdfghjdfghjdfghjdfghjdghjsdghdfghjdfghdfghdfghjdfghdfghjdfghjfghjdfghjdfghjfghjfghjfghjdfghj";
	item -> msg += " asdfghjklwertyuisdfghjkasdfghewrtyefrghfghdfghjklghjkdfghjdfghjdfghjdfghjdghjsdghdfghjdfghdfghdfghjdfghdfghjdfghjfghjdfghjdfghjfghjfghjfghjdfghj";
	item -> msg += " asdfghjklwertyuisdfghjkasdfghewrtyefrghfghdfghjklghjkdfghjdfghjdfghjdfghjdghjsdghdfghjdfghdfghdfghjdfghdfghjdfghjfghjdfghjdfghjfghjfghjfghjdfghj";

	MsgFileItem mfi;
	mfi.name = "A";
	mfi.hash = "B";
	mfi.size = 0xFFFFFFFF;
	item -> files.push_back(mfi);

	mfi.name = "safjhlsafhlsa kfjdhlsa kjfh klsajhf lkjsahflkjhsafkljhsafkjhsafjkhsakfjhksjfhkla sjhf klsjhf kl";
	mfi.hash = "ia dfjsakfjhsalkfjhlsajkfhlhjksf ljksafhlkjsahflkjsahfl kjsahfl jkhsafl kjhsafkl jhsa fkljh ";
	mfi.size = 0xFFFFFFFF;
	item -> files.push_back(mfi);

	item -> title = "sadf";
	item -> header = "To you from me";


	/* fill it up with rubbish */
	checkMsg(item);

	ChatItem *item2 = new ChatItem();
	item2 -> msg = "A Short Message";
	checkMsg(item2);

	ChatItem *item3 = new ChatItem();
	item3 -> msg = "asdfjkhfjhl sajkfhjksahfkl jahs fjklhsakfj hsajk fh klajs hfklja hsfjkhsa kf hksjdfhksajhf klsajhfkl jhdsafkl jhsklfj hksfjh lksajf klsjhfkjsaf sah f;lksahfk; jshfl kjsahfl; kjhsa flkjhsal kfjh s;akfjhsakfjh ljksl kjsahf ;jksal;sajkfhowiher ;oi28y540832qy5h4rlkjqwhrp928y52hrl;kwajhr2098y54 25hujh 32u5h3 285y 319485yh 315jh3 1495y 13295y15ui1 h51o h51ou5A Short Message";
	checkMsg(item3);



	return 1;
}

int checkMsg(PQItem *item)
{
	int i, j;

	std::cerr << "Input MsgItem: " << std::endl;
	item -> print(std::cerr);

	/* stream it */
	std::cerr << "Streaming..." << std::endl;

	void *pkt = pqipkt_makepkt(item);

	if (!pkt)
	{
		std::cerr << "Pkt creation failed!" << std::endl;
		return 1;
	}

	/* print out packet */
	int size = pqipkt_rawlen(pkt);

	std::cerr << "Pkt Dump(" << size << ")" << std::endl;
	std::ostringstream out;
	for(i = 0; i < size;)
	{
		for(j = 0; (i + j < size) && (j < 8); j++)
		{
			unsigned char n = ((unsigned char *) pkt)[i+j];
			out << std::setw(2) << std::hex << (unsigned int) n << ":";
		}
		i += j;
		out << std::endl;
	}
	std::cerr << out.str();

	/* stream back */
	PQItem *i2 = pqipkt_create(pkt);

	std::cerr << "Output MsgItem: " << std::endl;
	i2 -> print(std::cerr);

	return 1;

}





