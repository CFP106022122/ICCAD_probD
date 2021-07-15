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
extern Macro **macros;

struct edge
{
	int from, to;
	double weight;
	double slack; //refactor
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
	~Graph()
	{
		delete[] g;
		delete[] g_reversed;
		delete[] L;
		delete[] R;
		delete[] visited;

		for (int i = 0; i < MAX_N; ++i)
		{
			delete[] adj_matrix[i];
		}
		delete[] adj_matrix;
	}

	void add_edge(int u, int v, double w)
	{
		g[u].push_back(edge(u, v, w));
		g_reversed[v].push_back(edge(u, v, w));
		adj_matrix[u][v] = true;
	}

	void remove_edge(int u, int v)
	{
		for (int i = 0; i < g[u].size(); i++)
		{
			if (g[u][i].to == v)
			{
				g[u].erase(g[u].begin() + i);
				break;
			}
		}
		for (int i = 0; i < g_reversed[v].size(); i++)
		{
			if (g_reversed[v][i].from == u)
			{
				g_reversed[v].erase(g_reversed[v].begin() + i);
				break;
			}
		}
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
		for (auto &u : topological_order)
		{
			for (auto &e : g[u])
			{
				// cout << u << ' ' << e.to << '\n';
				// if (e.to == n + 1)
				// {
				// 	L[e.to] = max(L[e.to], L[u] + e.weight);
				// 	continue;
				// }
				// if (macros[e.to]->is_fixed())
				// {
				// 	L[e.to] = (is_horizontal) ? macros[e.to]->cx() : macros[e.to]->cy();
				// }
				// else
				// {
				L[e.to] = max(L[e.to], L[u] + e.weight);
				// }
				// L[e.to] = macros[e.to]->is_fixed() ? is_horizontal ? macros[e.to]->cx() : macros[e.to]->cy()
				// 								   : max(L[e.to], L[u] + e.weight);
			}
		}
		double chip_boundry;
		if (is_horizontal)
			chip_boundry = chip_width;
		else
			chip_boundry = chip_height;

		R[n + 1] = max(L[n + 1], chip_boundry);

		reverse(topological_order.begin(), topological_order.end());

		for (auto &u : topological_order)
		{
			for (auto &e : g_reversed[u])
			{
				// if (e.from == 0)
				// {
				// 	R[e.from] = min(R[e.from], R[u] - e.weight);
				// 	continue;
				// }
				// if (macros[e.from]->is_fixed())
				// {
				// 	R[e.from] = (is_horizontal) ? macros[e.from]->cx() : macros[e.from]->cy();
				// }
				// else
				// {
				R[e.from] = min(R[e.from], R[u] - e.weight);
				// }
				// R[e.from] = macros[e.from]->is_fixed() ? is_horizontal ? macros[e.from]->cx() : macros[e.from]->cy()
				// 									   : min(R[e.from], R[u] - e.weight);
			}
		}
		// for (fixed macro) L = R = x;
		return L[n + 1];
	}

	void dfs(int u, int v, vector<int> &delete_to_nodes)
	{
		if (u == 0)
		{ // u is source, can't remove edge(source, fixed macro)
			for (int i = 0; i < g[v].size(); i++)
			{
				if (adj_matrix[u][g[v][i].to] == true && !macros[g[v][i].to]->is_fixed())
				{
					// store node which will be deleted
					delete_to_nodes.push_back(g[v][i].to);
					adj_matrix[u][g[v][i].to] = false;
				}
				dfs(u, g[v][i].to, delete_to_nodes);
			}
		}
		else if (macros[u]->is_fixed())
		{ // u is a fixed macro, can't remove edge(fixed macro, sink)
			for (int i = 0; i < g[v].size(); i++)
			{
				if (adj_matrix[u][g[v][i].to] == true && g[v][i].to != n + 1)
				{
					// store node which will be deleted
					delete_to_nodes.push_back(g[v][i].to);
					adj_matrix[u][g[v][i].to] = false;
				}
				dfs(u, g[v][i].to, delete_to_nodes);
			}
		}
		else
		{ // normal case
			for (int i = 0; i < g[v].size(); i++)
			{
				if (adj_matrix[u][g[v][i].to] == true)
				{
					// store node which will be deleted
					delete_to_nodes.push_back(g[v][i].to);
					adj_matrix[u][g[v][i].to] = false;
				}
				dfs(u, g[v][i].to, delete_to_nodes);
			}
		}
	}

	void transitive_reduction()
	{
		for (int i = 0; i <= n; ++i)
		{
			// Create vector store deleted nodes
			vector<int> delete_to_nodes;
			for (int j = 0; j < g[i].size(); j++)
			{
				// Run DFS to find out nodes needed to remove
				dfs(g[i][j].from, g[i][j].to, delete_to_nodes);
			}
			// Remove stored nodes
			for (int j = 0; j < delete_to_nodes.size(); j++)
			{
				remove_edge(i, delete_to_nodes[j]);
			}
		}
	}

	void show()
	{
		for (int i = 0; i <= n; ++i)
		{
			if (i == 0)
				cout << "source 's neighbors:\n";
			else
				cout << "macro id" << macros[i]->id() << ' ' << macros[i]->name() << "(is_fixed: " << macros[i]->is_fixed() << ")'s neighbors:\n";
			for (auto &e : g[i])
				cout << "\t" << ((e.to == n + 1) ? "sink" : macros[e.to]->name()) << " with weight " << e.weight << endl;
			cout << endl;
		}
	}
	vector<edge> *get_edge_list()
	{
		return g;
	}
	vector<edge> *get_reverse_edge_list()
	{
		return g_reversed;
	}
};

#endif