#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace std;

class Graph {
private:
	constexpr static const int MAX_N = 105;
	int n;
	struct edge {
		int from, to;
		double weight;
		edge(int u, int v, double w): from{u}, to{v}, weight{w} { }
	};
	vector<edge> g[MAX_N];
	bool adj_matrix[MAX_N][MAX_N];
	bool visited[MAX_N];
public:
	Graph(int _n): n{_n} { 
		memset(adj_matrix, false, sizeof(adj_matrix));
	}
	
	void add_edge(int u, int v, double w) {
		g[u].push_back(edge(u, v, w));
		adj_matrix[u][v] = true;
	}	
	
	void remove_closure_edge(int u, int v) {
		g[u].erase(remove_if(g[u].begin(), g[u].end(), [=](edge& e) {
			return e.to == v;
		}), g[u].end()); 
	}
	
	void dfs(int u, int v) {
		visited[v] = true;
		for (auto& e: g[v]) {
			if (!visited[e.to]) {
				if (adj_matrix[u][e.to]) {
					remove_closure_edge(u, e.to);
				}
				dfs(u, e.to);
			}
		}

	}

	void transitive_reduction() {
		for (int i=0; i<=n; ++i) {
			for (auto& e:g[i]) {
				memset(visited, false, sizeof(visited));
				dfs(i, e.to);	
			}
		}
	}

	void show() {
		for (int i=0; i<=n; ++i) {
			cout << "macro " << i << "'s neighbors:\n";
			for (auto& e:g[i]) {
				cout << "\tmacro " << e.to << " with weight " << e.weight << endl;
			}
			cout << endl;
		}
	}

};
