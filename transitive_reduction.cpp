#include "tr/index.h"
#include "graph.h"
#include "macro.h"
#include <iostream>

using namespace std;

void transitive_reduction(Graph& G, vector<Macro*>& macro){
	vertexNum = macro.size() + 2;
	edgeNum = 0;
	vector<edge>* edge_list = G.get_edge_list();
	for(int i = 0; i <= macro.size() + 1; i++){
		edgeNum += edge_list[i].size();
	}
	vis_cur = 0;
	cur = 0;
	zeroDegreeVertexCount = 0;
	readGraph(G);
	read_graph(G);
	init(3000); // suppose we have 1000 macros maxdegree = 2000?
	topology();
	index_construction();
	sort3();
	order3();
	vector<pair<int, int>> candidate_edges;
	candidate_edges = redunEdgeCount();
	for(int i = 0; i < candidate_edges.size(); i++){
		if(candidate_edges[i].first == 0){
			if(macro[candidate_edges[i].second - 1]->is_fixed()){
				continue;
			}
		}
		if(candidate_edges[i].second == vertexNum - 1){
			if(macro[candidate_edges[i].first - 1]->is_fixed()){
				continue;
			}
		}
		//cout << "remove (" << candidate_edges[i].first << ", " << candidate_edges[i].second << ")" << endl;
		G.remove_edge(candidate_edges[i].first, candidate_edges[i].second);
	}
	destroyGraph();
	destroyLabel();
	destroyIndex();
}