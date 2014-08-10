#ifndef CHINESE_POSTMAN_UTIL_HPP_
#define CHINESE_POSTMAN_UTIL_HPP_

#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace ChinesePostman{
	typedef int EdgeWeightType;
	typedef boost::adjacency_list<boost::vecS, boost::setS, boost::undirectedS, boost::property<boost::vertex_name_t, std::string>, boost::property<boost::edge_weight_t, EdgeWeightType> > Graph;
	
	typedef std::map<Graph::vertex_descriptor, Graph::vertices_size_type> ComponentMap;
	typedef std::map<Graph::vertex_descriptor, std::map<Graph::vertex_descriptor, EdgeWeightType> > DistanceMatrix;
	
	// グラフの連結成分のうち一つを取り出すための定義。
	// boost::filtered_graph（元のグラフの構造を崩さないまま、部分グラフを
	// 取得するためのクラス）を用いている。
	// http://www.boost.org/doc/libs/1_55_0/libs/graph/doc/filtered_graph.html
	// 
	// connectedcomponentsメソッドで、ComponentMapのインスタンスに「頂点をキー、
	// その頂点が何番目の連結成分に入っているかを値とする連想配列」が格納されるので、
	// それを与え、そのうち一つの番号のみに属する辺・頂点を取り出す。
	class EdgeInGroup{
	private:
		const Graph & source_graph_;
		const ComponentMap & map_;
		Graph::vertices_size_type group_id_;
	public:
		EdgeInGroup(const Graph & source_graph, const ComponentMap & map, Graph::vertices_size_type group_id) : source_graph_(source_graph), map_(map), group_id_(group_id) {}
		bool operator()(Graph::edge_descriptor e) const {
			ComponentMap::const_iterator it = map_.find(boost::source(e, source_graph_));
			return it != map_.end() && it->second == group_id_;
		}
	};
	
	class VertexInGroup{
	private:
		const ComponentMap & map_;
		Graph::vertices_size_type group_id_;
	public:
		VertexInGroup(const ComponentMap & map, Graph::vertices_size_type group_id) : map_(map), group_id_(group_id) {}
		bool operator()(Graph::vertex_descriptor v) const {
			ComponentMap::const_iterator it = map_.find(v);
			return it != map_.end() && it->second == group_id_;
		}
	};
	
	class RouteNetwork;
	typedef boost::filtered_graph<RouteNetwork, EdgeInGroup, VertexInGroup> GraphDivision;
	typedef std::vector<RouteNetwork> RouteNetworkList;
	typedef RouteNetworkList::iterator RouteNetworkListIterator;
	typedef RouteNetworkList::const_iterator RouteNetworkListConstIterator;
	
	// あるグラフを元に別のグラフを作ったとき（部分グラフの抽出など）、
	// その頂点間の対応関係を表す連想配列
	typedef std::map<Graph::vertex_descriptor, Graph::vertex_descriptor> VertexMapping;
	
	// 「2頂点とその最短経路」を格納するためのクラス
	// （頂点は名前で指定）
	struct SubRoute{
		std::string v1, v2;
		EdgeWeightType weight;
		
		SubRoute(std::string vv1, std::string vv2, EdgeWeightType wweight) : v1(vv1), v2(vv2), weight(wweight) {}
	};
	
	// 「2頂点の組」を格納するためのクラス
	// （頂点はvertex_descriptorで指定）
	struct VirtualEdge{
		Graph::vertex_descriptor v1, v2;
		EdgeWeightType weight;
		
		VirtualEdge(Graph::vertex_descriptor vv1, Graph::vertex_descriptor vv2, EdgeWeightType wweight)
		: v1(vv1), v2(vv2), weight(wweight) {}
		
		inline bool operator==(const VirtualEdge & other) const{ return(v1 == other.v1 && v2 == other.v2 && weight == other.weight); }
		inline bool operator<(const VirtualEdge & other) const{ return(v1 < other.v1 || (v1 == other.v1 && (v2 < other.v2 || (v2 == other.v2 && weight < other.weight)))); }
	};
	
	// Graphクラスのインスタンスgraphにファイルの内容を読み込む。
	// 各行は「距離(正整数に限る) 地点名1 地点名2」と指定する。
	// 返り値はグラフの距離の総和。エラーが発生した場合は0を返す。
	EdgeWeightType read_from(const char * fname, Graph & graph){
		graph.clear();
		
		std::ifstream ifs(fname, std::ios::in | std::ios::binary);
		if(!ifs){
			std::cerr << "ERROR: Given file \"" << fname << "\" cannot be opened" << std::endl;
			return 0;
		}
		
		EdgeWeightType distance, total_distance = 0;
		std::map<std::string, Graph::vertex_descriptor> names2vertices;
		std::string line, s[2];
		Graph::vertex_descriptor vd[2];
		std::map<std::string, Graph::vertex_descriptor>::iterator it;
		
		while(!(ifs.eof())){
			// 行を読み込む
			std::getline(ifs, line);
			if(line.length() == 0 || line[0] == '#' || line[0] == '\r') continue;
			
			std::stringstream sst(line);
			sst.seekg(0, std::ios_base::beg);
			sst >> distance;
			if(distance <= 0){
				std::cerr << "ERROR: Distance less than zero or invalid distance found" << std::endl;
				return 0;
			}
			sst >> s[0];
			sst >> s[1];
			if(s[0].empty() || s[1].empty()){
				std::cerr << "ERROR: Station name invalid" << std::endl;
				return 0;
			}
			
			// 頂点を追加（まだ存在していないなら）
			// names2verticesは「駅名をキー、頂点を値とする連想配列」
			for(size_t i = 0; i <= 1; ++i){
				it = names2vertices.find(s[i]);
				
				if(it == names2vertices.end()){
					vd[i] = boost::add_vertex(s[i], graph);
					names2vertices.insert(std::make_pair(s[i], vd[i]));
				}else{
					vd[i] = it->second;
				}
			}
			
			// 辺を追加
			boost::add_edge(vd[0], vd[1], distance, graph);
			total_distance += distance;
		}
		
		return total_distance;
	}
} // namespace ChinesePostman

#endif // CHINESE_POSTMAN_UTIL_HPP_
