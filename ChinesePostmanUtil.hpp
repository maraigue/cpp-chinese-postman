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
	
	// �O���t�̘A�������̂���������o�����߂̒�`�B
	// boost::filtered_graph�i���̃O���t�̍\��������Ȃ��܂܁A�����O���t��
	// �擾���邽�߂̃N���X�j��p���Ă���B
	// http://www.boost.org/doc/libs/1_55_0/libs/graph/doc/filtered_graph.html
	// 
	// connectedcomponents���\�b�h�ŁAComponentMap�̃C���X�^���X�Ɂu���_���L�[�A
	// ���̒��_�����Ԗڂ̘A�������ɓ����Ă��邩��l�Ƃ���A�z�z��v���i�[�����̂ŁA
	// �����^���A���̂�����̔ԍ��݂̂ɑ�����ӁE���_�����o���B
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
	
	// ����O���t�����ɕʂ̃O���t��������Ƃ��i�����O���t�̒��o�Ȃǁj�A
	// ���̒��_�Ԃ̑Ή��֌W��\���A�z�z��
	typedef std::map<Graph::vertex_descriptor, Graph::vertex_descriptor> VertexMapping;
	
	// �u2���_�Ƃ��̍ŒZ�o�H�v���i�[���邽�߂̃N���X
	// �i���_�͖��O�Ŏw��j
	struct SubRoute{
		std::string v1, v2;
		EdgeWeightType weight;
		
		SubRoute(std::string vv1, std::string vv2, EdgeWeightType wweight) : v1(vv1), v2(vv2), weight(wweight) {}
	};
	
	// �u2���_�̑g�v���i�[���邽�߂̃N���X
	// �i���_��vertex_descriptor�Ŏw��j
	struct VirtualEdge{
		Graph::vertex_descriptor v1, v2;
		EdgeWeightType weight;
		
		VirtualEdge(Graph::vertex_descriptor vv1, Graph::vertex_descriptor vv2, EdgeWeightType wweight)
		: v1(vv1), v2(vv2), weight(wweight) {}
		
		inline bool operator==(const VirtualEdge & other) const{ return(v1 == other.v1 && v2 == other.v2 && weight == other.weight); }
		inline bool operator<(const VirtualEdge & other) const{ return(v1 < other.v1 || (v1 == other.v1 && (v2 < other.v2 || (v2 == other.v2 && weight < other.weight)))); }
	};
	
	// Graph�N���X�̃C���X�^���Xgraph�Ƀt�@�C���̓��e��ǂݍ��ށB
	// �e�s�́u����(�������Ɍ���) �n�_��1 �n�_��2�v�Ǝw�肷��B
	// �Ԃ�l�̓O���t�̋����̑��a�B�G���[�����������ꍇ��0��Ԃ��B
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
			// �s��ǂݍ���
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
			
			// ���_��ǉ��i�܂����݂��Ă��Ȃ��Ȃ�j
			// names2vertices�́u�w�����L�[�A���_��l�Ƃ���A�z�z��v
			for(size_t i = 0; i <= 1; ++i){
				it = names2vertices.find(s[i]);
				
				if(it == names2vertices.end()){
					vd[i] = boost::add_vertex(s[i], graph);
					names2vertices.insert(std::make_pair(s[i], vd[i]));
				}else{
					vd[i] = it->second;
				}
			}
			
			// �ӂ�ǉ�
			boost::add_edge(vd[0], vd[1], distance, graph);
			total_distance += distance;
		}
		
		return total_distance;
	}
} // namespace ChinesePostman

#endif // CHINESE_POSTMAN_UTIL_HPP_
