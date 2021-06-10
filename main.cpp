#include "flow.h"
#include "graph.h"
#include "macro.h"
#include <iostream>
#include <vector>

using namespace std;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

double chip_width = 25.0;
double chip_height = 10.0;

const int V = 7;
vector<Macro *> macros;

void add_st_nodes(Graph &Gh, Graph &Gv)
{
	// id 0 for source, V+1 for sink
	for (int i = 0; i < V; i++)
	{
		if (macros[i]->is_fixed())
		{
			Gh.add_edge(0, macros[i]->id(), macros[i]->cx() - 0);
			Gh.add_edge(macros[i]->id(), V + 1, chip_width - macros[i]->cx());
			Gv.add_edge(0, macros[i]->id(), macros[i]->cy() - 0);
			Gv.add_edge(macros[i]->id(), V + 1, chip_height - macros[i]->cy());
		}
		else
		{
			Gh.add_edge(0, macros[i]->id(), macros[i]->w() / 2);
			Gh.add_edge(macros[i]->id(), V + 1, macros[i]->w() / 2);
			Gv.add_edge(0, macros[i]->id(), macros[i]->h() / 2);
			Gv.add_edge(macros[i]->id(), V + 1, macros[i]->h() / 2);
		}
	}
}

void build_init_constraint_graph(Graph &Gh, Graph &Gv)
{
	add_st_nodes(Gh, Gv);
	for (int i = 0; i < V; ++i)
	{
		for (int j = i + 1; j < V; ++j)
		{
			double h_weight = (macros[i]->w() + macros[j]->w()) / 2;
			double v_weight = (macros[i]->h() + macros[j]->h()) / 2;
			if (is_overlapped(*macros[i], *macros[j]))
			{
				if (x_dir_is_overlapped_less(*macros[i], *macros[j]))
				{
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				}
				else
				{
					(macros[i]->cy() < macros[j]->cy()) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
														: Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
			}
			else if (projection_no_overlapped(*macros[i], *macros[j]))
			{
				if (macros[i]->cx() > macros[j]->cy())
					swap(*macros[i], *macros[j]);
				double diff_x, diff_y;
				bool i_is_at_the_bottom = false;
				diff_x = macros[j]->x1() - macros[i]->x2();
				if (macros[i]->cy() < macros[j]->cy())
				{
					diff_y = macros[j]->y1() - macros[i]->y2();
					i_is_at_the_bottom = true;
				}
				else
				{
					diff_y = macros[i]->y1() - macros[j]->y2();
				}
				if (diff_x < diff_y)
				{
					(i_is_at_the_bottom) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
										 : Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
				else
				{
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				}
			}
			else
			{
				if (x_dir_projection_no_overlapped(*macros[i], *macros[j]))
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				else if (y_dir_projection_no_overlapped(*macros[i], *macros[j]))
					Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight);
			}
		}
	}
}

void adjustment(Graph &Gh, Graph &Gv)
{
	double longest_path_h = Gh.longest_path(true);
	double longest_path_v = Gv.longest_path(false);
	while (longest_path_h > chip_width || longest_path_v > chip_height)
	{
		if (longest_path_h > chip_width && longest_path_v <= chip_height)
		{
			vector<edge> zero_slack_edges = Gh.zero_slack_edges();
			DICNIC<double> Gc;
			Gc.init(V);
			for (auto &e : zero_slack_edges)
			{
				if (e.from == 0 || e.to == V + 1)
				{
					Gc.add_edge(e.from, e.to, DBL_MAX, true);
					// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << DBL_MAX << '\n';
				}
				else
				{
					Gv.add_edge(e.from, e.to, e.weight);
					double Rvj = Gh.R[e.to];
					double Lvi = Gh.L[e.from];
					double test_vertical_longest_path = Gv.longest_path(false);
					if (test_vertical_longest_path > chip_height)
					{
						Gc.add_edge(e.from, e.to, DBL_MAX, true);
						// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << DBL_MAX << '\n';
					}
					else
					{
						double height_avg = (macros[e.from]->h() + macros[e.to]->h()) / 2;
						double w = max(macros[e.from]->cy() - Rvj + height_avg, 0) + max(Lvi + height_avg - macros[e.to]->cy(), 0);
						Gc.add_edge(e.from, e.to, w, true);
						// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << w << '\n';
					}
					Gv.remove_edge(e.from, e.to);
				}
			}
			Gc.min_cut(0, V + 1);
			for (auto &e : Gc.cut_e)
			{
				Gv.add_edge(e.pre, e.v, (macros[e.pre]->h() + macros[e.v]->h()) / 2);
				Gh.remove_edge(e.pre, e.v);
			}
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
		}
		else if (longest_path_h <= chip_width && longest_path_v > chip_height)
		{
		}
		else
		{
		}
	}
}

void re_index(vector<Macro *> &macros)
{
	vector<Macro *> tmp{macros};
	for (int i = 0; i < tmp.size(); ++i)
	{
		macros[tmp[i]->id()] = tmp[i];
	}
}

int main(int argc, char *argv[])
{
	Macro m1(4.0, 4.0, 0.0, 3.0, false, 1);
	Macro m2(4.0, 2.0, 3.8, 5.5, false, 2);
	Macro m3(4.0, 3.0, 3.8, 1.5, false, 3);
	Macro m4(6.0, 9.0, 7.6, 0.7, false, 4);
	Macro m5(6.0, 4.0, 13.4, 4.7, false, 5);
	Macro m6(6.0, 3.0, 16.0, 0.7, false, 6);
	Macro m7(6.0, 6.0, 19.0, 3.7, false, 7);
	macros.push_back(&m1);
	macros.push_back(&m2);
	macros.push_back(&m3);
	macros.push_back(&m4);
	macros.push_back(&m5);
	macros.push_back(&m6);
	macros.push_back(&m7);
	Graph Gh(V), Gv(V);
	build_init_constraint_graph(Gh, Gv);
	// Gh.transitive_reduction();
	// Gv.transitive_reduction();
	re_index(macros);
	adjustment(Gh, Gv);
	cout << "QQQQ\n";
	return 0;
}
