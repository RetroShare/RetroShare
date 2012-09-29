#pragma once

#include <string>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <stdint.h>

class MonitoredTurtleRouter ;
class RsTurtle ;
class RsRawItem ;
class p3ServiceServer ;

class PeerNode
{
	public:
		PeerNode(const std::string& id,const std::list<std::string>& friends) ;
		~PeerNode() ;

		RsRawItem *outgoing() ;
		void incoming(RsRawItem *) ;

		const std::string& id() const { return _id ;}

		void tick() ;

		const RsTurtle *turtle_service() const ;
	private:
		p3ServiceServer *_service_server ;
		MonitoredTurtleRouter *_turtle ;
		std::string _id ;
};

template<class NODE_TYPE> class Graph
{
	public:
		typedef uint32_t NodeId ;

		void symmetric_connect(uint32_t n1,uint32_t n2) ;
		const std::set<NodeId>& neighbors(const NodeId& n) const { return _neighbors[n] ; }

		// number of nodes.
		//
		uint32_t n_nodes() const { return _nodes.size() ;}
		const NODE_TYPE& node(int i) const { return *_nodes[i] ; }
		NODE_TYPE& node(int i) { return *_nodes[i] ; }

	protected:

		std::vector<NODE_TYPE *> _nodes ;
		std::vector<std::set<uint32_t> > _neighbors ;
};

class Network: public Graph<PeerNode>
{
	public:
		// inits the graph as random. Returns true if connected, false otherwise.
		//
		bool initRandom(uint32_t n_nodes, float connexion_probability) ;

		// ticks all services of all nodes.
		//
		void tick() ;

		PeerNode& node_by_id(const std::string& node_id) ;

	private:
		std::map<std::string,uint32_t> _node_ids ;
};

