#include "ChinesePostman.hpp"
#include <iostream>
#include <fstream>

#define DIVIDE_BY_BRIDGE_BRIDGELIST "bridges"
#define DIVIDE_BY_BRIDGE_COMPONENT "subgraph"
#define DIVIDE_BY_BRIDGE_EXTENSION ".edges"

int main(int argc, char ** argv){
	if(argc <= 2){
		std::cerr << "Usage: " << argv[0] << " FILENAME OUTPUT_DIR" << std::endl;
		return 1;
	}
	
	ChinesePostman::RouteNetwork rn;
	ChinesePostman::EdgeWeightType total_distance = ChinesePostman::read_from(argv[1], rn);
	if(total_distance == 0){
		std::cerr << "Error: When reading \"" << argv[1] << "\"" << std::endl;
		return 1;
	}
	
	// グラフ構造を表示
	std::cout << "[Graph Structure]" << std::endl;
	rn.print();
	std::cout << std::endl;
	
	// 次数2の頂点を除去
	rn.remove_trivial_vertices();
	
	// 橋を検出する
	ChinesePostman::BridgeDetector bd(rn);
	
	// 橋を出力する
	std::string fname_bridges(argv[2]);
	fname_bridges.append("/");
	fname_bridges.append(DIVIDE_BY_BRIDGE_BRIDGELIST);
	fname_bridges.append(DIVIDE_BY_BRIDGE_EXTENSION);
	std::ofstream ofs(fname_bridges.c_str(), std::ios::binary);
	if(!ofs){
		std::cerr << "Error: When opening \"" << fname_bridges << "\"" << std::endl;
		return 1;
	}
	
	for(std::set<ChinesePostman::Graph::edge_descriptor>::iterator ite = bd.result().begin(); ite != bd.result().end(); ++ite){
		ofs << boost::get(boost::edge_weight, rn, *ite) << " " << boost::get(boost::vertex_name, rn, boost::source(*ite, rn)) << " " << boost::get(boost::vertex_name, rn, boost::target(*ite, rn)) << std::endl;
		boost::remove_edge(*ite, rn);
	}
	ofs.close();
	std::cerr << "Wrote bridge list to \"" << fname_bridges << "\"" << std::endl;
	
	// 次数2の頂点を除去
	rn.remove_trivial_vertices();
	
	// 分割された各グラフを出力する
	ChinesePostman::RouteNetworkList graph_divisions;
	rn.connectedcomponents(graph_divisions);
	for(ChinesePostman::RouteNetworkList::iterator itg = graph_divisions.begin(); itg != graph_divisions.end(); ++itg){
		std::pair<ChinesePostman::Graph::vertex_iterator, ChinesePostman::Graph::vertex_iterator> vertex_range = boost::vertices(*itg);
		if(vertex_range.first == vertex_range.second){
			// 頂点が存在しない場合
			continue;
		}
		
		std::pair<ChinesePostman::Graph::edge_iterator, ChinesePostman::Graph::edge_iterator> edge_range = boost::edges(*itg);
		if(edge_range.first == edge_range.second){
			// 辺が存在しない場合
			continue;
		}
		
		std::string fname_component(argv[2]);
		fname_component.append("/");
		fname_component.append(DIVIDE_BY_BRIDGE_COMPONENT);
		fname_component.append("-");
		fname_component.append(boost::get(boost::vertex_name, *itg, *(vertex_range.first)));
		fname_component.append(DIVIDE_BY_BRIDGE_EXTENSION);
		
		std::ofstream ofsc(fname_component.c_str(), std::ios::binary);
		if(!ofsc){
			std::cerr << "Error: When opening \"" << fname_component << "\"" << std::endl;
			return 1;
		}
		
		itg->print_bridge_list(ofsc);
		ofsc.close();
		std::cerr << "Wrote graph component to \"" << fname_component << "\"" << std::endl;
	}
	
	return 0;
}
