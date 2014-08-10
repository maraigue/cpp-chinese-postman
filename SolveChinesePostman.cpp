#include "ChinesePostman.hpp"
#include "masked_vector.hpp"
#include <iostream>
#include <algorithm>

//#define CHINESE_POSTMAN_DEBUG_DUMP // �r���̌v�Z���ʂ̏ڍׂ�\���������ꍇ
//#define CHINESE_POSTMAN_DEBUG_PROGRESS // �r���̌v�Z���ǂ̒��x�i��ł��邩�\���������ꍇ

template <class TYPE>
inline bool equal_pair(const TYPE & obj1_elem1, const TYPE & obj1_elem2, const TYPE & obj2_elem1, const TYPE & obj2_elem2){
	if(obj1_elem1 == obj2_elem1){
		return(obj1_elem2 == obj2_elem2);
	}else if(obj1_elem1 == obj2_elem2){
		return(obj1_elem2 == obj2_elem1);
	}else{
		return false;
	}
}

ChinesePostman::EdgeWeightType sum_of_distance(const std::deque<ChinesePostman::SubRoute> & route){
	ChinesePostman::EdgeWeightType result = 0;
	for(auto it = route.cbegin(); it != route.cend(); ++it){
		result += it->weight;
	}
	return result;
}

int main(int argc, char ** argv){
	if(argc < 2 || argc > 3){
		std::cerr << "Usage: " << argv[0] << " GRAPH_FILENAME (CUT_FILENAME)" << std::endl;
		return 1;
	}
	
	// ---------- �J�b�g����ӂ̈ꗗ
	ChinesePostman::RouteNetwork cut;
	if(argc == 3){
		ChinesePostman::EdgeWeightType cut_distance = ChinesePostman::read_from(argv[2], cut);
		if(cut_distance == 0){
			std::cerr << "Error: When reading \"" << argv[2] << "\"" << std::endl;
			return 1;
		}
	}
	
	// ---------- �O���t
	ChinesePostman::RouteNetwork rn;
	ChinesePostman::EdgeWeightType total_distance = ChinesePostman::read_from(argv[1], rn);
	if(total_distance == 0){
		std::cerr << "Error: When reading \"" << argv[1] << "\"" << std::endl;
		return 1;
	}
	
	// ---------- �u�J�b�g����ӂ̈ꗗ�v�ɂ���ӂ���������ƂƂ��ɁA�������ꂽ�ӂɐڂ��钸�_��񋓂���
	std::pair<ChinesePostman::Graph::edge_iterator, ChinesePostman::Graph::edge_iterator> edge_range = boost::edges(rn);
	std::vector<ChinesePostman::Graph::edge_descriptor> removed_edges_later_rn;
	
	// border_vertices��
	// �L�[�F���_
	// �l��first�F���Y���_��������̃O���t�̂����߂ɑ����Ă��邩�i��قǎw��j
	// �l��second�F���Y���_��masked_vertices_orig�̉��Ԗڂ̗v�f�ł��邩�i��قǎw��j
	std::map< ChinesePostman::Graph::vertex_descriptor, std::pair<size_t, size_t> > border_vertices;
	
	std::multiset<ChinesePostman::VirtualEdge> border_edges;
	
	
	std::pair<ChinesePostman::Graph::edge_iterator, ChinesePostman::Graph::edge_iterator> cutedge_range;
	
	for(ChinesePostman::Graph::edge_iterator ite = edge_range.first; ite != edge_range.second; ++ite){
		cutedge_range = boost::edges(cut);
		ChinesePostman::Graph::edge_iterator cutite;
		
		for(cutite = cutedge_range.first; cutite != cutedge_range.second; ++cutite){
			if(rn.edgeweight(*ite) == cut.edgeweight(*cutite) && equal_pair(
				rn.vertexname1_fromedge(*ite), rn.vertexname2_fromedge(*ite),
				cut.vertexname1_fromedge(*cutite), cut.vertexname2_fromedge(*cutite)
			)){
				break;
			}
		}
		if(cutite != cutedge_range.second){
			boost::remove_edge(*cutite, cut);
			removed_edges_later_rn.push_back(*ite);
			
			ChinesePostman::Graph::vertex_descriptor v1, v2;
			v1 = boost::source(*ite, rn);
			v2 = boost::target(*ite, rn);
			border_vertices.insert(std::make_pair(v1, std::make_pair(-1, -1)));
			border_vertices.insert(std::make_pair(v2, std::make_pair(-1, -1)));
			border_edges.insert(ChinesePostman::VirtualEdge(v1, v2, rn.edgeweight(*ite)));
		}
	}
	
	cutedge_range = boost::edges(cut);
	if(cutedge_range.first != cutedge_range.second){
		std::cerr << "ERROR: Cutting edge not found - " << cut.vertexname1_fromedge(*(cutedge_range.first)) << ", " << cut.vertexname2_fromedge(*(cutedge_range.first)) << std::endl;
		return 1;
	}
	
	for(std::vector<ChinesePostman::Graph::edge_descriptor>::iterator ite = removed_edges_later_rn.begin(); ite != removed_edges_later_rn.end(); ++ite){
		boost::remove_edge(*ite, rn);
	}
	
	// ---------- �A���v�f�ɕ���
	ChinesePostman::RouteNetworkList division_result;
	ChinesePostman::VertexMapping vmap; // vmap�́u���̃O���t��ł̒��_���L�[�A������̃O���t��ł̒��_��l�Ƃ���A�z�z��v
	rn.connectedcomponents(division_result, vmap);
	
	// ---------- �A���v�f�̂��ꂼ��ɂ��āA2��ʂ�ӂ����肷��
	// ���������̂Ƃ��A���E�̒��_�͊��ʂ邩������ʂ邩�ŏꍇ�킯����K�v������
	// ���Ȃ킿�A�i2^[���E�̒��_��]�j�ʂ�������K�v������
	
	std::vector< std::map< masked_vector<ChinesePostman::Graph::vertex_descriptor>::mask_type, std::deque<ChinesePostman::SubRoute> > > doubling_result(division_result.size());
	std::vector< masked_vector<ChinesePostman::Graph::vertex_descriptor> > masked_vertices_orig(division_result.size());
	std::vector< masked_vector<ChinesePostman::Graph::vertex_descriptor> > masked_vertices_sub(division_result.size());
	
	size_t graph_component_id;
	
	graph_component_id = 0;
	for(ChinesePostman::RouteNetworkList::iterator itg = division_result.begin(); itg != division_result.end(); ++itg){
		std::pair<ChinesePostman::Graph::vertex_iterator, ChinesePostman::Graph::vertex_iterator> vertex_range = boost::vertices(*itg);
		std::cerr << "Computing the graph of Stations[0] = \"" << (vertex_range.first == vertex_range.second ? std::string("<none>") : itg->vertexname(*vertex_range.first)) << "\", ";
		std::cerr << "Stations.size = " << boost::num_vertices(*itg) << ", ";
		std::cerr << "Edges.size = " << boost::num_edges(*itg) << std::endl;
		
		// border_vertices_subgraph��
		// ���̃O���t��ł̒��_���L�[�A������̃O���t�i*itg�j��ł̒��_��l�Ƃ���
		// �A�z�z��ŁA�������E��Ɉʒu���Ă�����̂̂ݏW�߂�����
		std::map<ChinesePostman::Graph::vertex_descriptor, ChinesePostman::Graph::vertex_descriptor> border_vertices_subgraph;
		
		for(ChinesePostman::Graph::vertex_iterator itv = vertex_range.first; itv != vertex_range.second; ++itv){
			std::map< ChinesePostman::Graph::vertex_descriptor, std::pair<size_t, size_t> >::iterator itvb = std::find_if(border_vertices.begin(), border_vertices.end(), [&](const std::pair< ChinesePostman::Graph::vertex_descriptor, std::pair<size_t, size_t> > & v){ return vmap.at(v.first) == *itv; });
			if(itvb != border_vertices.end()){
				itvb->second.first = graph_component_id;
				border_vertices_subgraph[itvb->first] = *itv;
			}
		}
		
		// �ԍ��t������masked_vertices_*�Ɋi�[����
		size_t count = 0;
		for(std::map<ChinesePostman::Graph::vertex_descriptor, ChinesePostman::Graph::vertex_descriptor>::iterator itv = border_vertices_subgraph.begin(); itv != border_vertices_subgraph.end(); ++itv){
			border_vertices[itv->first].second = count;
			++count;
			masked_vertices_orig[graph_component_id].push_back(itv->first);
			masked_vertices_sub[graph_component_id].push_back(itv->second);
		}
		
		// �t���C�h�����[�V����
		ChinesePostman::DistanceMatrix distance_table;
		boost::floyd_warshall_all_pairs_shortest_paths(*itg, distance_table);
		
		// ���ׂĂ�border_vertices_subgraph�u�̕����W���v�ɂ���
		// 2��ʂ�ׂ��ӂ����肷��
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
		glp_term_out(GLP_ON);
#else
		glp_term_out(GLP_OFF);
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
		
		do{
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cerr << "    Border nodes visited for even-number times (mask: " << masked_vertices_sub[graph_component_id].mask() << ", size: " << masked_vertices_sub[graph_component_id].size() << ")";
			std::cerr << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			
			masked_vertices_orig[graph_component_id].next();
			masked_vertices_sub[graph_component_id].next();
			
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cerr << "    Border nodes visited for even-number times (mask: " << masked_vertices_sub[graph_component_id].mask() << ", size: " << masked_vertices_sub[graph_component_id].size() << ")";
			for(size_t i = 0; i < masked_vertices_sub[graph_component_id].size(); ++i){
				if(masked_vertices_sub[graph_component_id].has(i)){
					std::cerr << " " << itg->vertexname(masked_vertices_sub[graph_component_id][i]);
				}
			}
			std::cerr << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			
			// �g�ݍ��킹�����߂�
			if(itg->find_doubled_edges(distance_table, doubling_result[graph_component_id][masked_vertices_sub[graph_component_id].mask()], masked_vertices_sub[graph_component_id])){
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
				std::cerr << "[[Computed!!]]" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			}else{
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
				std::cerr << "[[Skipped!]]" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
				doubling_result[graph_component_id].erase(masked_vertices_sub[graph_component_id].mask());
			}
		}while(!(masked_vertices_sub[graph_component_id].emptymask()));
		
		++graph_component_id;
	}
	
	// �J�b�g�p�̕ӂ�1��ʂ�/2��ʂ�Ƃ������ׂĂ̑g�ݍ��킹�ɂ���
	// �e�����̋����Ƒg�ݍ��킹�Ă���
	masked_vector<ChinesePostman::VirtualEdge> border_edge_subsets(border_edges);
	
	std::vector< masked_vector<ChinesePostman::Graph::vertex_descriptor>::mask_type > mask_compo(division_result.size());
	ChinesePostman::EdgeWeightType best_distance = 0;
	std::vector<const std::deque<ChinesePostman::SubRoute> *> temp_doubling_result(division_result.size()), best_doubling_result(division_result.size());
	masked_vector<ChinesePostman::VirtualEdge>::mask_type best_border_edge_subset_mask;
	
	boost::multiprecision::cpp_int percentage = 0, new_percentage;
	boost::multiprecision::cpp_int max_mask = 1;
	max_mask = (max_mask << border_edge_subsets.size()) - 1;
	if(max_mask == 0) max_mask = 1;
	
	do{
		new_percentage = border_edge_subsets.mask() * 100 / max_mask;
		if(new_percentage == 100 || new_percentage >= percentage + 10){
			percentage = new_percentage;
			std::cerr << "Border edge doubling: " << percentage << "%" << std::endl;
		}
		
		border_edge_subsets.next();
		
		// �J�b�g�p�̕ӂ�1��ʂ�/2��ʂ�Ƃ����g�ݍ��킹�iborder_edge_subsets�j�ɂ���
		// �Ή����鋫�E��̒��_��������ʂ邩���ʂ邩���肷��
		std::fill(mask_compo.begin(), mask_compo.end(), 0);
		std::pair<size_t, size_t> flag4vertex;
		ChinesePostman::EdgeWeightType cut_distance = 0;
		ChinesePostman::EdgeWeightType compo_distance = 0;
		
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
		std::cerr << "----------";
		std::cerr << "Doubling the following cut edges:";
#endif // CHINESE_POSTMAN_DEBUG_DUMP
		for(size_t i = 0; i < border_edge_subsets.size(); ++i){
			if(border_edge_subsets.has(i)){
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
				std::cerr << " " << rn.vertexname(border_edge_subsets[i].v1);
				std::cerr << "-" << rn.vertexname(border_edge_subsets[i].v2);
#endif // CHINESE_POSTMAN_DEBUG_DUMP
				cut_distance += border_edge_subsets[i].weight;
				
				flag4vertex = border_vertices[border_edge_subsets[i].v1];
				boost::multiprecision::bit_flip(mask_compo[flag4vertex.first], flag4vertex.second);
				flag4vertex = border_vertices[border_edge_subsets[i].v2];
				boost::multiprecision::bit_flip(mask_compo[flag4vertex.first], flag4vertex.second);
			}
		}
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
		std::cerr << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
		
		// ���E��̒��_��������ʂ邩���ʂ邩�̑g�ݍ��킹��
		// ���ۂɎ����\���m�F����
		ChinesePostman::RouteNetworkList::iterator itg;
		graph_component_id = 0;
		for(itg = division_result.begin(); itg != division_result.end(); ++itg){
			if(doubling_result[graph_component_id].find(mask_compo[graph_component_id]) == doubling_result[graph_component_id].end()) break;
			++graph_component_id;
		}
		if(itg != division_result.end()){
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cerr << "Not possible" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			continue;
		}
		
		// �i���ۂɉ\�Ȃ�j���������Z�������̂��擾����
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
		std::cerr << "Doubled edges optimization: " << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
		graph_component_id = 0;
		for(itg = division_result.begin(); itg != division_result.end(); ++itg){
			temp_doubling_result[graph_component_id] = &(doubling_result[graph_component_id][mask_compo[graph_component_id]]);
			
			for(std::deque<ChinesePostman::SubRoute>::const_iterator its = temp_doubling_result[graph_component_id]->cbegin(); its != temp_doubling_result[graph_component_id]->cend(); ++its){
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
				std::cerr << " " << its->v1;
				std::cerr << "-" << its->v2;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			}
			
			compo_distance += sum_of_distance(*(temp_doubling_result[graph_component_id]));
			
			++graph_component_id;
		}
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
		std::cerr << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
		
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
		std::cout << "Doubled edge weight in cuts: " << cut_distance << std::endl;
		std::cout << "Doubled edge weight in optimization: " << compo_distance << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
		
		if(best_distance == 0 || best_distance > cut_distance + compo_distance){
			best_distance = cut_distance + compo_distance;
			best_doubling_result.swap(temp_doubling_result);
			best_border_edge_subset_mask = border_edge_subsets.mask();
		}
	}while(!(border_edge_subsets.emptymask()));
	
	std::cout << "---------- Best Result ----------" << std::endl;
	std::cout << "# Total distance of all graph edges = " << total_distance << std::endl;
	std::cout << "# Total distance of doubled edges = " << best_distance << std::endl;
	std::cout << "# Total distance of traversed edges = " << total_distance + best_distance << std::endl;
	std::cout << "# Edges traversed twice in cuts" << std::endl;
	border_edge_subsets.set_mask(best_border_edge_subset_mask);
	for(size_t i = 0; i < border_edge_subsets.size(); ++i){
		if(border_edge_subsets.has(i)){
			std::cout << border_edge_subsets[i].weight << " ";
			std::cout << rn.vertexname(border_edge_subsets[i].v1) << " ";
			std::cout << rn.vertexname(border_edge_subsets[i].v2) << std::endl;
		}
	}
	
	for(graph_component_id = 0; graph_component_id < division_result.size(); ++graph_component_id){
		std::cout << "# Edges traversed twice in component " << (graph_component_id+1) << "" << std::endl;
		for(std::deque<ChinesePostman::SubRoute>::const_iterator its = best_doubling_result[graph_component_id]->cbegin(); its != best_doubling_result[graph_component_id]->cend(); ++its){
			std::cout << its->weight << " ";
			std::cout << its->v1 << " ";
			std::cout << its->v2 << std::endl;
		}
	}
	
	return 0;
}
