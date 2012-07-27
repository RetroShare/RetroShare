
#include "gxp_service.h"

class db_wire
{
	/* external interface for accessing the info */

	/** Note this could get very busy with a large number of tweeters.
	 * need some other method of getting data
	 */

	getTweeters(std::list<std::string> &tweeterIds);
	{
		getGroups();
	}

	getTweetRange(GxpTimeStamp, GxpTimeStamp, std::list<std::string> &tweetIds);
	{
		getTimeRange();
	}

	getTweetRangeSource(GxpTimeStamp, GxpTimeStamp, std::string tweeterId, std::list<std::string> &tweetIds);
	{
		getGroupTimeRange();
	}

	getTweet(std::string id, TweetData &tweet);
	{
		StackLock();

		RsGxpItem *getMsg_locked(id);

		// translate message into TweetData.

	}

	// Default 
	getProfile(std::string id, TweetData &tweet);

	/* returns a search code, which is used to id later delivery */
	int searchTweets(GxpSearchCondition cond, std::list<std::string>);

	/* returns a search code, which is used to id later delivery */
	int fetchPendingSearchResults(int searchId, std::list<std::string>);

	int cancelSearch(int searchId, std::list<std::string>);

};


