#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "macro.h"
#include <algorithm>
#include <cfloat>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

extern double chip_width;
extern double chip_height;
extern vector<Macro *> macros;

struct edge
{
	int from, to;
	double weight;
	double slack;
	edge(int u, int v, double w) : from{u}, to{v}, weight{w} {}
};

class Graph
{
private:
	int MAX_N;
	int n;
	vector<edge> *g;
	vector<edge> *g_reversed;
	bool **adj_matrix;

public:
	Graph(int _n) : n{_n}
	{
		MAX_N = n + 5;
		g = new vector<edge>[MAX_N];
		g_reversed = new vector<edge>[MAX_N];
		L = new double[MAX_N];
		R = new double[MAX_N];
		visited = new bool[MAX_N];
		adj_matrix = new bool *[MAX_N];

		for (int i = 0; i < MAX_N; ++i)
			adj_matrix[i] = new bool[MAX_N];

		for (int i = 0; i < MAX_N; ++i)
			for (int j = 0; j < MAX_N; ++j)
				adj_matrix[i][j] = false;
	}
	void rebuild()
	{
		for (int i = 0; i < MAX_N; ++i)
		{
			g[i].clear();
			g_reversed[i].clear();
		}
		for (int i = 0; i < MAX_N; ++i)
			for (int j = 0; j < MAX_N; ++j)
				adj_matrix[i][j] = false;
	}
	// ~Graph()
	// {
	// 	delete g;
	// 	delete g_reversed;
	// 	delete L;
	// 	delete R;
	// 	delete visited;

	// 	for (int i = 0; i < MAX_N; ++i)
	// 	{
	// 		delete[] adj_matrix[i];
	// 	}
	// 	delete adj_matrix;
	// }

	void add_edge(int u, int v, double w)
	{
		g[u].push_back(edge(u, v, w));
		g_reversed[v].push_back(edge(u, v, w));
		adj_matrix[u][v] = true;
	}

	void remove_edge(int u, int v)
	{
		g[u].erase(remove_if(g[u].begin(), g[u].end(), [=](edge &e)
							 { return e.to == v; }),
				   g[u].end());
		g_reversed[v].erase(remove_if(g_reversed[v].begin(), g_reversed[v].end(), [=](edge &e)
									  { return e.from == u; }),
							g_reversed[v].end());
		adj_matrix[u][v] = false;
	}

	void dfs_for_topological_sort_helper(int u)
	{
		visited[u] = true;
		for (auto &e : g[u])
		{
			if (!visited[e.to])
			{
				dfs_for_topological_sort_helper(e.to);
			}
		}
		topological_order.push_back(u);
	}

	bool *visited;
	void dfs_for_topological_sort()
	{
		// memset(visited, false, sizeof(visited));
		for (int i = 0; i <= n + 1; ++i)
			visited[i] = false;
		for (int i = 0; i <= n; ++i)
			if (!visited[i])
				dfs_for_topological_sort_helper(i);
	}

	vector<int> topological_order;
	void topological_sort()
	{
		topological_order.clear();
		dfs_for_topological_sort();
		reverse(topological_order.begin(), topological_order.end());
	}

	vector<edge> _zero_slack_edges;
	vector<edge> &zero_slack_edges()
	{
		_zero_slack_edges.clear();
		for (int i = 0; i <= n; ++i)
			for (auto &e : g[i])
				if (R[e.to] - L[e.from] - e.weight == 0)
					_zero_slack_edges.push_back(e);
		return _zero_slack_edges;
	}

	double *L, *R; // as required in UCLA paper
	double longest_path(bool is_horizontal)
	{
		topological_sort();
		for (int i = 0; i <= n + 1; ++i)
		{
			L[i] = 0.0;
			R[i] = DBL_MAX;
		}
		// fill(L, L + n + 2, 0.0);
		// fill(R, R + n + 2, DBL_MAX);
		for (auto &u : topological_order)
		{
			for (auto &e : g[u])
			{
				// cout << u << ' ' << e.to << '\n';
				if (e.to == n + 1)
				{
					L[e.to] = max(L[e.to], L[u] + e.weight);
					continue;
				}
				L[e.to] = macros[e.to]->is_fixed() ? is_horizontal ? macros[e.to]->cx() : macros[e.to]->cy()
												   : max(L[e.to], L[u] + e.weight);
			}
		}
		R[n + 1] = max(L[n + 1], chip_width);
		reverse(topological_order.begin(), topological_order.end());
		for (auto &u : topological_order)
		{
			for (auto &e : g_reversed[u])
			{
				if (e.from == 0)
				{
					R[e.from] = min(R[e.from], R[u] - e.weight);
					continue;
				}
				R[e.from] = macros[e.from]->is_fixed() ? is_horizontal ? macros[e.from]->cx() : macros[e.from]->cy()
													   : min(R[e.from], R[u] - e.weight);
			}
		}
		return L[n + 1];
	}

	void dfs(int u, int v)
	{
		for (auto &e : g[v])
		{
			if (adj_matrix[u][e.to])
				remove_edge(u, e.to);
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
			if (i == 0)
				cout << "source 's neighbors:\n";
			else
				cout << "macro id" << macros[i]->id()<< "'s neighbors:\n";
			for (auto &e : g[i])
				cout << "\t" << ((e.to == n + 1) ? n + 1 : macros[e.to]->id()) << " with weight " << e.weight << endl;
			cout << endl;
		}
	}
	vector<edge>* get_edge_list(){
		return g;
	}
};

#endif
