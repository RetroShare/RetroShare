//
// This is a first attempt at managing groups and group permissions
// in libretroshare.
//
// Rules:
// 	1 - Groups are handled GPG-wise, and not SSLId wise, but we need to provide functions
// 		for SSLIds since that's what peers are represented by.
//
// 	2 - A peer can be in different groups. That will be useful for e.g. file sharing.
//
// 	3 - Global permissions always prevail over permissions given at the group level. For 
// 	   instance for discovery between two peers that belong to groups G1 and G2:
//
//                             | Global discovery flag ON | Global disc flag OFF |
//         --------------------+--------------------------+----------------------+
//         Group disc(G1,G2)=1 |           Allow          |     Don't allow      |
//         Group disc(G1,G2)=0 |         Don't allow      |     Don't allow      |
//
//    4 - Binary group permissions are OR-ed between pairs of groups. If peers (A,B) can be matched to
//    	two different pairs of groups, the flags are combined with an OR.
//
// libretroshare side
// ==================
//   We need a matrix for binary group flags, i.e. flags for pair of groups, for:
//
//      Turtle traffic: 
//      	* does the turtle traffic can go from peers of a group to another
//      	* default: yes
//
//      Discovery:
//      	* does the turtle traffic can go from peers of a group to another
//      	* default: yes. Overriden by discovery status in config.
//
//      Public chat lobby advertisement:
//      	* public lobby coming from peers of a group go to peers of another group
//      	* default: yes
//
//      Relay connexions:
//       * allow relay connexions between people of group A and group B.
//
//      General Exchange Services:
//       * Allow data exchanged from one peers of one group to peers of another group
//
//   We need per-group permission flags for:
//
//      File download permission: 
//       * each shared directory belongs to a list of groups
//      	* each shared directory has a set of DL flags (browsable B,network wide N) for groups and for global.
//      	* files are listed with unix-like permissions style: 
//      			---- : file can't be transfered at all. 
//      			bn-- : file is browsable and turtle-able by friends of its groups only.
//      			b-b- : file is browsable by everybody
//      			--b- : file is browsable by everybody that does not belong to its groups (can be useful to isolate people in a group)
//      			-n-- : file is turtle-able by people that belong to its groups only.
//      			bnbn : file is always accessible in any ways. That's what file sharing is about. Should be the default ;-)
//
//      	We need to propose some pre-initialized settings to the user, so that he won't have to bother with flags, but we
//      	need to allow precisely tweaking them as well:
//      		Directory is 
//      			- Accessible for this group only (allow select a group)
//      			- Fully shared anonymously
//      			- [...]
//
//      	If the shared directory has no groups, than the first two flags are meaningless.
//  
// GUI side
// ========
// 	- we need a tab in config to manage groups and group flag permission matrix
// 	- when sharing a directory, the user can act on permissions: setting which groups a 
// 	  directory belongs to.
//
// Classes for groups. To be displatched in rstypes.h ?
//

#pragma once

typedef std::string SSLId ;
typedef std::string GPGId ;
typedef uint64_t    RsGroupId ;

class RsPeerGroupInfo
{
	public:
		RsGroupId group_id ;				// Id of the group, should be random and unique in the RS session.
		std::string group_name ;		// user-defined name of the group ("parents", "friends", etc)
		std::set<GPGId> elements ;		// people in the group. GPG-wise.
};

class GroupFlagsMatrix
{
	public:
		// flags

		static const uint32_t GROUP_MATRIX_FLAG_DISCOVERY    = 0x1 ;
		static const uint32_t GROUP_MATRIX_FLAG_TURTLE       = 0x2 ;
		static const uint32_t GROUP_MATRIX_FLAG_PUBLIC_LOBBY = 0x4 ;
		static const uint32_t GROUP_MATRIX_FLAG_RELAY        = 0x8 ;

		// returns/set all flags for a given pair of groups. Used by the 
		// GUI to draw/modify group flags.
		//
		uint32_t getGroupFlags(RsGroupId grp1,RsGroupId grp2) const ;	
		void setGroupFlags(uint32_t flags,RsGroupId grp1,RsGroupId grp2) const ;	

		// Returns a given flag for two people in particular. Used by services to 
		// know what to do or not do.
		//
		bool groupPermission(uint32_t group_flag,GPGId peer_1,GPGId peer2) const ;

	private:
		std::vector<uint32_t> _flags ;	// vector of size number_of_groups^2
		std::map<RsGroupId,int> _group_entries ; // index to read the flags vector.
};

// Interface class for Groups. Should go in libretroshare/stc/retroshare/rsgroups.h
//

class RsGroupManagement
{
	public:
		// Group handling

		// Returns false is the group does not exist. Otherwise, fills the GroupInfo struct.
		//
		virtual bool getGroupInfo(const RsGroupId& id,RsPeerGroupInfo& info) = 0 ;

		// Adds a friend to a group.
		virtual bool getGroupInfo(const RsGroupId& id,RsPeerGroupInfo& info) = 0 ;

		// Group permission handling
		//
		virtual bool allow_TwoPeers_TurtleTraffic     (const SSLId& p1,const SSLId& p2) = 0 ;
		virtual bool allow_TwoPeers_Discovery         (const SSLId& p1,const SSLId& p2) = 0 ;
		virtual bool allow_TwoPeers_LobbyAdvertisement(const SSLId& p1,const SSLId& p2) = 0 ;
		// [...]
};

class p3GroupManagement: public RsGroupManagement, public p3Config
{
	public:
		// [...]
		
	private:
		std::map<RsPeerGroupInfo> _groups ;
};

// This is the entry point for external interface.
//
extern RsGroupManagement *rsGroups ;

