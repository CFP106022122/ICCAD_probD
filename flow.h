#ifndef _FLOW_H_
#define _FLOW_H_

#include <algorithm>
#include <cfloat>
#include <iostream>
#include <limits.h>
#include <queue>
#include <string.h>
#include <vector>
template <typename T>
struct DICNIC // O(V^2 E) -> O(VlogV E) // O(VE)
{
	int MAX_N;
	constexpr static const T INF = DBL_MAX;
	int n;			  /*點數*/
	int *level, *cur; /*層次、當前弧優化*/
	struct edge
	{
		int v, pre;
		T cap, flow, r;
		edge(int v, int pre, T cap) : v(v), pre(pre), cap(cap), flow(0), r(cap) {}
	};
	int *g;
	std::vector<edge> e;

	~DICNIC()
	{
		delete[] level;
		delete[] cur;
		delete[] g;
		delete[] vis;
	}

	void init(int _n)
	{
		MAX_N = _n + 5;
		level = new int[MAX_N];
		cur = new int[MAX_N];
		g = new int[MAX_N];
		vis = new bool[MAX_N];
		memset(g, -1, sizeof(int) * ((n = _n) + 1));
		e.clear();
	}
	void add_edge(int u, int v, T cap, bool directed = false)
	{
		e.push_back(edge(v, g[u], cap));
		g[u] = e.size() - 1;
		e.push_back(edge(u, g[v], directed ? 0 : cap));
		g[v] = e.size() - 1;
	}
	int bfs(int s, int t)
	{
		memset(level, 0, sizeof(int) * (n + 1));
		memcpy(cur, g, sizeof(int) * (n + 1));
		std::queue<int> q;
		q.push(s);
		level[s] = 1;
		while (q.size())
		{
			int u = q.front();
			q.pop();
			for (int i = g[u]; ~i; i = e[i].pre)
			{
				if (!level[e[i].v] && e[i].r)
				{
					level[e[i].v] = level[u] + 1;
					q.push(e[i].v);
					if (e[i].v == t)
						return 1;
				}
			}
		}
		return 0;
	}
	T dfs(int u, int t, T cur_flow = INF)
	{
		if (u == t || !cur_flow)
			return cur_flow;
		T df, tf = 0;
		for (int &i = cur[u]; ~i; i = e[i].pre)
		{
			if (level[e[i].v] == level[u] + 1 && e[i].r)
			{
				if (df = dfs(e[i].v, t, std::min(cur_flow, e[i].r)))
				{
					e[i].flow += df;
					e[i ^ 1].flow -= df;
					e[i].r -= df;
					e[i ^ 1].r += df;
					tf += df;
					if (!(cur_flow -= df))
						break;
				}
			}
		}
		if (!df)
			level[u] = 0;
		return tf;
	}
	T dinic(int s, int t, bool clean = true)
	{
		if (clean)
		{
			for (size_t i = 0; i < e.size(); ++i)
			{
				e[i].flow = 0;
				e[i].r = e[i].cap;
			}
		}
		T ans = 0;
		while (bfs(s, t))
			ans += dfs(s, t);
		return ans;
	}
	std::vector<edge> cut_e; //最小割邊集
	bool *vis;
	void dfs_cut(int u)
	{
		vis[u] = 1; //表示u屬於source的最小割集
		std::cout << u << '\n';
		for (int i = g[u]; ~i; i = e[i].pre)
		{
			if (e[i].flow < e[i].cap && !vis[e[i].v])
				dfs_cut(e[i].v);
		}
	}
	bool min_cut(int s, int t)
	{
		T ans = dinic(s, t);
		memset(vis, 0, sizeof(bool) * (n + 1));
		dfs_cut(s), cut_e.clear();
		for (int u = 0; u <= n; ++u)
		{
			if (vis[u])
			{
				for (int i = g[u]; ~i; i = e[i].pre)
				{
					if (!vis[e[i].v])
					{
						std::cout << "??\n";
						cut_e.push_back(edge(e[i].v, u, 5487.87));
					}
				}
			}
		}
		// std::cout << ans << "\n";
		return ans >= DBL_MAX;
	}
};

#endif
