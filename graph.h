#include <algorithm>
#include <cfloat>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

extern double chip_width;
extern double chip_height;

class Graph
{
private:
	constexpr static const int MAX_N = 105;
	// constexpr static const double INF = DBL_MAX;
	int n;
	struct edge
	{
		int from, to;
		double weight;
		double slack;
		edge(int u, int v, double w) : from{u}, to{v}, weight{w} {}
	};
	vector<edge> g[MAX_N];
	vector<edge> g_reverserd[MAX_N];
	vector<int> topological_order;
	bool visited[MAX_N];
	bool adj_matrix[MAX_N][MAX_N];
	double L[MAX_N], R[MAX_N]; // as required in UCLA paper

public:
	Graph(int _n) : n{_n}
	{
		memset(adj_matrix, false, sizeof(adj_matrix));
	}

	void add_edge(int u, int v, double w)
	{
		g[u].push_back(edge(u, v, w));
		g_reverserd[v].push_back(edge(u, v, w));
		adj_matrix[u][v] = true;
	}

	void dfs_for_topological_sort_helper(int u)
	{
		visited[u] = true;
		for (auto &e : g[u])
			if (!visited[e.to])
				dfs_for_topological_sort_helper(e.to);
		topological_order.push_back(u);
	}

	void dfs_for_topological_sort()
	{
		memset(visited, false, sizeof(visited));
		for (int i = 0; i <= n; ++i)
			if (!visited[i])
				dfs_for_topological_sort_helper(i);
	}

	void topological_sort()
	{
		dfs_for_topological_sort();
		reverse(topological_order.begin(), topological_order.end());
	}

	double longest_path()
	{
		topological_sort();
		fill(L, L + n + 1, 0.0);
		fill(R, R + n + 1, DBL_MAX);
		for (auto &u : topological_order)
			for (auto &e : g[u])
				L[e.to] = max(L[e.to], L[u] + e.weight);
		R[n + 1] = max(L[n + 1], chip_width);
		reverse(topological_order.begin(), topological_order.end());
		for (auto &u : topological_order) // reverse topological order
			for (auto &e : g_reverserd[u])
				R[e.from] = min(R[e.from], R[u] - e.weight);
		return L[n + 1];
	}

	void remove_closure_edge(int u, int v)
	{
		g[u].erase(remove_if(g[u].begin(), g[u].end(), [=](edge &e)
							 { return e.to == v; }),
				   g[u].end());
	}

	void dfs(int u, int v)
	{
		for (auto &e : g[v])
		{
			if (adj_matrix[u][e.to])
			{
				remove_closure_edge(u, e.to);
			}
			dfs(u, e.to);
		}
	}

	void transitive_reduction()
	{
		for (int i = 0; i <= n; ++i)
			for (auto &e : g[i])
				dfs(i, e.to);
	}

	void show()
	{
		for (int i = 0; i <= n; ++i)
		{
			cout << "macro " << i << "'s neighbors:\n";
			for (auto &e : g[i])
			{
				cout << "\tmacro " << e.to << " with weight " << e.weight << endl;
			}
			cout << endl;
		}
	}
};
