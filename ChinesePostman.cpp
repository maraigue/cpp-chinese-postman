#include <glpk.h>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <deque>
#include <vector>

//#define CHINESE_POSTMAN_DEBUG_DUMP // 途中の計算結果の詳細を表示したい場合
//#define CHINESE_POSTMAN_DEBUG_PROGRESS // 途中の計算がどの程度進んでいるか表示したい場合

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
	
	// 路線網を定義するクラス。
	class RouteNetwork : public Graph{
	public:
		RouteNetwork() : Graph(){
			// Do nothing
		}
		
		// このインスタンスにファイルの内容を読み込む。
		// 各行は「距離(正整数に限る) 地点名1 地点名2」と指定する。
		// 返り値はグラフの距離の総和。エラーが発生した場合は0を返す。
		EdgeWeightType read_from(const char * fname){
			this->clear();
			
			std::ifstream ifs(fname, std::ios::in | std::ios::binary);
			if(!ifs){
				std::cerr << "ERROR: Given file \"" << fname << "\" cannot be opened" << std::endl;
				return 0;
			}
			
			EdgeWeightType distance, total_distance = 0;
			std::map<std::string, vertex_descriptor> names2vertices;
			std::string line, s[2];
			vertex_descriptor vd[2];
			std::map<std::string, vertex_descriptor>::iterator it;
			
			while(!(ifs.eof())){
				// 行を読み込む
				std::getline(ifs, line);
 				if(line.length() == 0) continue;
				
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
						vd[i] = boost::add_vertex(s[i], *this);
						names2vertices.insert(std::make_pair(s[i], vd[i]));
					}else{
						vd[i] = it->second;
					}
				}
				
				// 辺を追加
				boost::add_edge(vd[0], vd[1], distance, *this);
				total_distance += distance;
			}
			
			return total_distance;
		}
		
		// ------------------------------------------------------------
		// ユーティリティ
		// ------------------------------------------------------------
		
		// 頂点と辺を与え、頂点から辺を辿った先の頂点を返す。
		// 与えられた頂点が辺のどちらの端点でもなかった場合は、null_vertexを返す。
		vertex_descriptor vertex_target(vertex_descriptor vertex, edge_descriptor edge) const{
			vertex_descriptor n1 = source(edge, *this);
			vertex_descriptor n2 = target(edge, *this);
			
			if(vertex == n1) return n2;
			if(vertex == n2) return n1;
			return null_vertex();
		}
		
		// 頂点を与え、その名前を返す。
		const std::string & vertexname(vertex_descriptor vertex) const{
			return boost::get(boost::vertex_name, *this, vertex);
		}
		
		// 辺を与え、その頂点の名前を返す。
		const std::string & vertexname1_fromedge(edge_descriptor edge) const{
			return boost::get(boost::vertex_name, *this, boost::source(edge, *this));
		}
		const std::string & vertexname2_fromedge(edge_descriptor edge) const{
			return boost::get(boost::vertex_name, *this, boost::target(edge, *this));
		}
		
		// 辺を与え、その距離を返す。
		EdgeWeightType edgeweight(edge_descriptor edge) const{
			return boost::get(boost::edge_weight, *this, edge);
		}
		
		// （経路決定の上で必要のない）次数2の頂点を除去し簡単化する。
		void remove_trivial_vertices(){
			std::pair<vertex_iterator, vertex_iterator> vertex_range = boost::vertices(*this);
			edge_descriptor links[2]; // 次数2の頂点から出ている辺
			vertex_descriptor sides[2]; // 上記の辺の行き先である頂点
			std::vector<vertex_descriptor> removed_vertices;
			size_t i;
			EdgeWeightType distance;
			
			for(vertex_iterator itv = vertex_range.first; itv != vertex_range.second; ++itv){
				if(out_degree(*itv, *this) == 2){
					std::pair<out_edge_iterator, out_edge_iterator> edge_range = boost::out_edges(*itv, *this);
					i = 0;
					distance = 0;
					for(out_edge_iterator ite = edge_range.first; ite != edge_range.second; ++ite, ++i){
						sides[i] = vertex_target(*itv, *ite);
						distance += boost::get(boost::edge_weight, *this, *ite);
						links[i] = *ite;
					}
					if(links[0] == links[1]) continue; // 頂点が1つ・辺が1つの輪
					boost::add_edge(sides[0], sides[1], distance, *this);
					boost::remove_edge(links[0], *this);
					boost::remove_edge(links[1], *this);
					removed_vertices.push_back(*itv);
				}
			}
			
			for(std::vector<vertex_descriptor>::iterator itv = removed_vertices.begin(); itv != removed_vertices.end(); ++itv){
				boost::remove_vertex(*itv, *this);
			}
		}
		
		// 連結成分に分割した結果を返す。
		// 結果は引数に格納される。
		void connectedcomponents(RouteNetworkList & division_result) const{
			size_t i;
			
			// 結果を格納するためのmap
			ComponentMap compomap;
			boost::associative_property_map<ComponentMap> prop_compomap(compomap);
			// 頂点に番号付けするためのmap
			// http://stackoverflow.com/questions/15432104/how-to-create-a-propertymap-for-a-boost-graph-using-lists-as-vertex-container
			ComponentMap indexmap;
			boost::associative_property_map<ComponentMap> prop_indexmap(indexmap);
			i = 0;
			std::pair<vertex_iterator, vertex_iterator> vertex_range = boost::vertices(*this);
			for(vertex_iterator itv = vertex_range.first; itv != vertex_range.second; ++itv){
				put(prop_indexmap, *itv, i);
				++i;
			}
			size_t componum = boost::connected_components(*this, prop_compomap, boost::vertex_index_map(prop_indexmap));
			
			// グループ分けされた結果を用いて、実際に分割されたグラフを作る
			division_result.clear();
			division_result.resize(componum, RouteNetwork());
			
			for(size_t gr = 0; gr < componum; ++gr){
				// 指定されたのグループ番号の頂点、
				// ならびに接する頂点が指定されたグループ番号である辺だけを
				// 取り出したグラフを得る
				GraphDivision part_graph(*this, EdgeInGroup(*this, compomap, gr), VertexInGroup(compomap, gr));
				
				// GraphDivision（boost::filtered_graph）のままだと
				// ワーシャル＝フロイド法が使えないっぽいので
				// RouteNetwork（boost::adjacency_list）に変換する。
				// relは rel[part_graphの頂点] = division_result[gr]の頂点 の対応付け。
				std::map<vertex_descriptor, vertex_descriptor> rel;
				division_result[gr] = RouteNetwork();
				
				std::pair<GraphDivision::vertex_iterator, GraphDivision::vertex_iterator> vertex_range_part = boost::vertices(part_graph);
				for(GraphDivision::vertex_iterator itv = vertex_range_part.first; itv != vertex_range_part.second; ++itv){
					rel[*itv] = boost::add_vertex(boost::get(boost::vertex_name, *this, *itv), division_result[gr]);
				}
				std::pair<GraphDivision::edge_iterator, GraphDivision::edge_iterator> edge_range_part = boost::edges(part_graph);
				for(GraphDivision::edge_iterator ite = edge_range_part.first; ite != edge_range_part.second; ++ite){
					boost::add_edge(rel[boost::source(*ite, *this)], rel[boost::target(*ite, *this)], boost::get(boost::edge_weight, *this, *ite), division_result[gr]);
				}
			}
		}
		
		// グラフの内容を出力する。
		void print() const{
			std::pair<vertex_iterator, vertex_iterator> vertex_range = boost::vertices(*this);
			
			std::cout << "Vertices:";
			if(vertex_range.first == vertex_range.second){
				std::cout << " (None)" << std::endl;
			}else{
				for(vertex_iterator itv = vertex_range.first; itv != vertex_range.second; ++itv){
					std::cout << " " << boost::get(boost::vertex_name, *this, *itv);
				}
				std::cout << std::endl;
			}
			
			std::pair<edge_iterator, edge_iterator> edge_range = boost::edges(*this);
			
			if(edge_range.first == edge_range.second){
				std::cout << "  (No edge contained)" << std::endl;
			}else{
				for(edge_iterator ite = edge_range.first; ite != edge_range.second; ++ite){
					std::cout << "  Edge: "  << boost::get(boost::vertex_name, *this, boost::source(*ite, *this)) << " - " << boost::get(boost::vertex_name, *this, boost::target(*ite, *this)) << " (Distance = " << boost::get(boost::edge_weight, *this, *ite) << ")" << std::endl;
				}
			}
		}
	};
	
	// 橋（その辺1本がなくなると連結でなくなるような辺）を検出する。
	// http://nupioca.hatenadiary.jp/entry/2013/11/03/200006
	class BridgeDetector{
		std::map<Graph::vertex_descriptor, size_t> pre_, low_;
		size_t count_;
		std::set<Graph::edge_descriptor> result_;
		const RouteNetwork & rn_;
		
		void detect_bridges_main(Graph::vertex_descriptor vertex, boost::optional<Graph::out_edge_iterator> ite_from_parent){
			// すでにpreにvertexが含まれている場合は何もしない
			if(pre_.find(vertex) != pre_.end()) return;
			
			// 新しい番号を割り当てる
			pre_[vertex] = count_;
			low_[vertex] = count_;
			++count_;
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			//if(count_ % 10 == 0) std::cerr << "[DEBUG]   Processing " << count_ << " vertices" << std::endl;
			std::cerr << "[DEBUG]   Processing station " << rn_.vertexname(vertex) << " (" << count_ << ")" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
			Graph::vertex_descriptor target;
			std::pair<Graph::out_edge_iterator, Graph::out_edge_iterator> edge_range = boost::out_edges(vertex, rn_);
			for(Graph::out_edge_iterator ite = edge_range.first; ite != edge_range.second; ++ite){
				if(ite_from_parent && *ite == *(ite_from_parent.get())) continue;
				
				target = rn_.vertex_target(vertex, *ite);
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
				std::cerr << "[DEBUG]     Searching: " << rn_.vertexname(vertex) << " -> " << rn_.vertexname(target) << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
				detect_bridges_main(target, boost::make_optional(ite));
				low_[vertex] = std::min(low_[vertex], low_[target]);
			}
			
			if(ite_from_parent && low_[vertex] == pre_[vertex]){
				result_.insert(*(ite_from_parent.get()));
			}
		}
		
	public:
		BridgeDetector(const RouteNetwork & rn) : count_(0), rn_(rn) {
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			std::cerr << "[DEBUG] Detecting Bridges ..." << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
			detect_bridges_main(*(boost::vertices(rn_).first), boost::optional<Graph::out_edge_iterator>());
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			std::cerr << "[DEBUG] Completed Detecting Bridges!" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
		}
		
		const std::set<Graph::edge_descriptor> & result() const{
			return result_;
		}
	};
	
	class Solver{
	public:
		struct SubRoute{
			std::string v1, v2;
			EdgeWeightType weight;
			
			SubRoute(std::string vv1, std::string vv2, EdgeWeightType wweight) : v1(vv1), v2(vv2), weight(wweight) {}
		};
		
	private:
		// 橋の検出用。
		// 橋（であるために2回通る必要のある辺）は p_bd->result() で得られる
		std::unique_ptr<BridgeDetector> p_bd;
		
		// 橋の一覧
		std::deque<SubRoute> brigdes_;
		// 橋以外で2回通る必要のある辺の一覧
		std::deque<SubRoute> doubled_edges_;
		
	public:
		int run(RouteNetwork & rn){
			brigdes_.clear();
			doubled_edges_.clear();
			
			// 橋を検出
			p_bd = std::unique_ptr<BridgeDetector>(new BridgeDetector(rn));
			
			// 橋を列挙し結果として格納＆除去、その後グラフ構造を表示
			for(std::set<Graph::edge_descriptor>::iterator ite = p_bd->result().begin(); ite != p_bd->result().end(); ++ite){
				brigdes_.push_back(
					SubRoute(
						boost::get(boost::vertex_name, rn, boost::source(*ite, rn)),
						boost::get(boost::vertex_name, rn, boost::target(*ite, rn)),
						boost::get(boost::edge_weight, rn, *ite)));
				boost::remove_edge(*ite, rn);
			}
			
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cout << std::endl;
			std::cout << "[Graph Structure Without Bridges]" << std::endl;
			rn.print();
			std::cout << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			
			// 次数2の頂点を除去してからグラフ構造を表示
			rn.remove_trivial_vertices();
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cout << std::endl;
			std::cout << "[Graph Structure Without Trivial Nodes]" << std::endl;
			rn.print();
			std::cout << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			
			// グラフを分割してからグラフ構造を表示
			RouteNetworkList graph_divisions;
			rn.connectedcomponents(graph_divisions);
#ifdef CHINESE_POSTMAN_DEBUG_DUMP
			std::cout << "[Graph Structure After Divided into Connected Components]" << std::endl;
			for(RouteNetworkList::iterator itg = graph_divisions.begin(); itg != graph_divisions.end(); ++itg){
				itg->print();
			}
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			
			// 各グラフにワーシャル＝フロイド法を適用し、その結果を表示する
			std::map<RouteNetworkList::iterator, DistanceMatrix> floyd_warshall_table;
			
#if defined(CHINESE_POSTMAN_DEBUG_PROGRESS)
			std::cerr << "[DEBUG] Calculating Shortest Paths..." << std::endl;
#elif defined(CHINESE_POSTMAN_DEBUG_DUMP)
			std::cout << "[Shortest Paths]" << std::endl;
#endif
			for(RouteNetworkList::iterator itg = graph_divisions.begin(); itg != graph_divisions.end(); ++itg){
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
				std::cerr << "[DEBUG]   Size: #vertices = " << boost::num_vertices(*itg) << ", #edges = " << boost::num_edges(*itg) << ", vertex[0] = " << rn.vertexname(*(boost::vertices(*itg).first)) << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
				boost::floyd_warshall_all_pairs_shortest_paths(*itg, floyd_warshall_table[itg]);

#ifdef CHINESE_POSTMAN_DEBUG_DUMP
				std::cout << "Graph (number of vertex(vertices): " << num_vertices(*itg) << "):" << std::endl;
				for(std::map<RouteNetworkList::iterator, DistanceMatrix>::iterator itr1 = floyd_warshall_table[itg].begin(); itr1 != floyd_warshall_table[itg].end(); ++itr1){
					std::cout << "Shortest paths from " << boost::get(boost::vertex_name, *itg, itr1->first) << ":" << std::endl;
					for(std::map<RouteNetworkList::iterator, DistanceMatrix>::iterator itr2 = itr1->second.begin(); itr2 != itr1->second.end(); ++itr2){
						std::cout << "    " << boost::get(boost::vertex_name, *itg, itr2->first) << ": " << itr2->second << std::endl;
					}
				}
#endif // CHINESE_POSTMAN_DEBUG_DUMP
			}
#if defined(CHINESE_POSTMAN_DEBUG_PROGRESS)
			std::cerr << "[DEBUG] Completed Calculating Shortest Paths!" << std::endl;
#elif defined(CHINESE_POSTMAN_DEBUG_DUMP)
			std::cout << std::endl;
#endif
			
			// 奇数次数の頂点だけ集めて、最小マッチングを求める
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			glp_term_out(GLP_ON);
#else
			glp_term_out(GLP_OFF);
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
			
			size_t temp_id, i, j;
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			std::cerr << "[DEBUG] Calculating Minimum Matching..." << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
			for(RouteNetworkList::iterator itg = graph_divisions.begin(); itg != graph_divisions.end(); ++itg){
				// 奇数次数の頂点を集める
				std::set<Graph::vertex_descriptor> odd_vertices;
				std::pair<Graph::vertex_iterator, Graph::vertex_iterator> vertex_range = boost::vertices(*itg);
				for(Graph::vertex_iterator itv = vertex_range.first; itv != vertex_range.second; ++itv){
					if(boost::out_degree(*itv, *itg) % 2 == 1){
						odd_vertices.insert(*itv);
					}
				}
				if(odd_vertices.size() % 2 == 1){
					// 奇数次の頂点が奇数個しかない（ありえない）
					std::cerr << "Unexpected Error: odd_vertices.size() == " << odd_vertices.size() << " (Expected an even number)" << std::endl;
					return 1;
				}
				if(odd_vertices.size() == 0){
					// 奇数次の頂点がない（グラフはすでにオイラーグラフ）
					continue;
				}
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
				std::cerr << "[DEBUG]   Size: #vertices = " << boost::num_vertices(*itg) << " (#odd_vertices = " << odd_vertices.size() << "), #edges = " << boost::num_edges(*itg) << ", vertex[0] = " << rn.vertexname(*(boost::vertices(*itg).first)) << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
				
				// ---------- 整数計画法で解く ----------
				glp_prob *mip = glp_create_prob();
				glp_set_obj_dir(mip, GLP_MIN);
				
				glp_add_rows(mip, odd_vertices.size());
				
				// 頂点（n個）に関する制約「最小マッチングにおいて必ず1回ずつ使う」
				temp_id = 1;
				for(std::set<Graph::vertex_descriptor>::iterator itv = odd_vertices.begin(); itv != odd_vertices.end(); ++itv){
					glp_set_row_bnds(mip, temp_id, GLP_FX, 1.0, 1.0);
					++temp_id;
				}
				
				// 頂点の組（n_C_2個）に関する制約「最小マッチングにおいて高々1回使う」
				size_t combinations = odd_vertices.size() * (odd_vertices.size() - 1) / 2;
				glp_add_cols(mip, combinations);
				std::vector<int> ia(combinations*2 + 1), ja(combinations*2 + 1);
				std::vector<double> ar(combinations*2 + 1);
				
				temp_id = 1;
				i = 1;
				double val_coef;
				
				for(std::set<Graph::vertex_descriptor>::iterator itv1 = odd_vertices.begin(); itv1 != odd_vertices.end(); ++itv1){
					std::set<Graph::vertex_descriptor>::iterator itv2 = itv1;
					++itv2;
					j = i + 1;
					for(; itv2 != odd_vertices.cend(); ++itv2){
						glp_set_col_kind(mip, temp_id, GLP_BV);
						
						val_coef = (double)(floyd_warshall_table[itg].at(*itv1).at(*itv2));
						glp_set_obj_coef(mip, temp_id, val_coef);
						
						ia[temp_id] = i; ja[temp_id] = temp_id; ar[temp_id] = 1.0;
						ia[temp_id+combinations] = j; ja[temp_id+combinations] = temp_id; ar[temp_id+combinations] = 1.0;
						
						// TODO:「itv1～itv2の最短経路が奇数次の点を途中で2つ以上通るならば、それらの任意の2つの組み合わせは結果に出てはならない」を式で表す
						
						++temp_id;
						++j;
					}
					++i;
				}
				glp_load_matrix(mip, combinations*2, &(ia[0]), &(ja[0]), &(ar[0]));
				
				glp_iocp parm;
				glp_init_iocp(&parm);
				parm.presolve = GLP_ON;
				
				// GLPKに解かせる
				int err = glp_intopt(mip, &parm);
				if(err != 0){
					std::cerr << "GLPK Error (Reason: " << err << ")" << std::endl;
					glp_delete_prob(mip); // TODO: RAIIにするためにラッパーを書く
					continue;
				}
				
				// GLPKに解かせた結果を得る
#if defined(CHINESE_POSTMAN_DEBUG_DUMP) || defined(CHINESE_POSTMAN_DEBUG_PROGRESS)
				double opt_result = 
#endif
				glp_mip_obj_val(mip);
				
#if defined(CHINESE_POSTMAN_DEBUG_DUMP) || defined(CHINESE_POSTMAN_DEBUG_PROGRESS)
				std::cout << "加算距離: " << opt_result << std::endl;
#endif
				
				double varpos;
				
				temp_id = 1;
				for(std::set<Graph::vertex_descriptor>::iterator itv1 = odd_vertices.begin(); itv1 != odd_vertices.end(); ++itv1){
					std::set<Graph::vertex_descriptor>::iterator itv2 = itv1;
					++itv2;
					for(; itv2 != odd_vertices.end(); ++itv2){
						varpos = glp_mip_col_val(mip, temp_id);
						// varposは本来は0か1だけだが、小数点で出ることを考慮して
						if(varpos > 0.5){
							doubled_edges_.push_back(
								SubRoute(
									boost::get(boost::vertex_name, rn, *itv1),
									boost::get(boost::vertex_name, rn, *itv2),
									floyd_warshall_table[itg].at(*itv1).at(*itv2)));
						}
						
						++temp_id;
					}
				}
				glp_delete_prob(mip);
			}
#ifdef CHINESE_POSTMAN_DEBUG_PROGRESS
			std::cerr << "[DEBUG] Completed Calculating Minimum Matching!" << std::endl;
#endif // CHINESE_POSTMAN_DEBUG_PROGRESS
			
			return 0;
		}
		
		Solver(){
			// Do nothing
		}
		
		Solver(RouteNetwork & rn){
			run(rn);
		}
		
		const std::deque<SubRoute> & bridges() const{
			return brigdes_;
		}
		const std::deque<SubRoute> & doubled_edges() const{
			return doubled_edges_;
		}
	};
} // namespace ChinesePostman

int main(int argc, char ** argv){
	if(argc <= 1){
		std::cerr << "Usage: " << argv[0] << " FILENAME" << std::endl;
		return 1;
	}
	
	ChinesePostman::RouteNetwork rn;
	ChinesePostman::EdgeWeightType total_distance = rn.read_from(argv[1]);
	if(total_distance == 0){
		std::cerr << "File-reading error!" << std::endl;
		return 1;
	}
	
	// グラフ構造を表示
	std::cout << "[Graph Structure]" << std::endl;
	rn.print();
	std::cout << std::endl;
	
	// 解く
	ChinesePostman::Solver slv(rn);
	
	// 結果を出力
	ChinesePostman::EdgeWeightType added_distance = 0;
	
	for(std::deque<ChinesePostman::Solver::SubRoute>::const_iterator it = slv.bridges().begin(); it != slv.bridges().end(); ++it){
		added_distance += it->weight;
		std::cout << "2回乗車する必要のある区間<Bridge>: " << it->v1 << " - " << it->v2 << " (距離: " << it->weight << ")" << std::endl;
	}
	
	for(std::deque<ChinesePostman::Solver::SubRoute>::const_iterator it = slv.doubled_edges().begin(); it != slv.doubled_edges().end(); ++it){
		added_distance += it->weight;
		std::cout << "2回乗車する必要のある区間<GLPK>:   " << it->v1 << " - " << it->v2 << " (距離: " << it->weight << ")" << std::endl;
	}
	std::cout << "路線総距離：　" << total_distance << std::endl;
	std::cout << "重複乗車距離：" << added_distance << std::endl;
	std::cout << "総乗車距離：　" << total_distance + added_distance << std::endl;
	
	return 0;
}
