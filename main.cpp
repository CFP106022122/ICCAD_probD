#include "flow.h"
#include "graph.h"
#include "macro.h"
#include <iostream>
#include <random>
#include <vector>
#include "io.h"

using namespace std;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

double chip_width;// = 25.0;
double chip_height;// = 10.0;
int V;// = 7; // #macros;
double alpha;// = 1.0, 
double beta;// = 4.0;
double powerplan_width;// = 0.0, 
double min_spacing;// = 0.0;
vector<Macro *> macros;
IoData* shoatingMain(int argc, char* argv[]);

double hyper_parmeter = 0.1;
int rebuild_cnt = 0;

mt19937_64 rng;
std::uniform_real_distribution<double> unif(0.0, 1.0);
bool lucky(double ratio, int rebuild_cnt, double hyper_parameter)
{
	double x = unif(rng);
	double y = ratio / (ratio + 1.0) - (double)rebuild_cnt * hyper_parameter;
	return x < y;
}

void re_index(vector<Macro *> &macros)
{
	vector<Macro *> tmp{macros};
	for (int i = 0; i < tmp.size(); ++i)
	{
		macros[tmp[i]->id()] = tmp[i];
	}
}

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

void build_init_constraint_graph(Graph &Gh, Graph &Gv, int rebuild_cnt)
{
	add_st_nodes(Gh, Gv);
	for (int i = 0; i < V; ++i)
	{
		for (int j = i + 1; j < V; ++j)
		{
			double h_weight = (macros[i]->w() + macros[j]->w()) / 2 + ((lucky(beta / alpha, rebuild_cnt, hyper_parmeter)) ? (powerplan_width / 2) : (min_spacing / 2));
			double v_weight = (macros[i]->h() + macros[j]->h()) / 2 + ((lucky(beta / alpha, rebuild_cnt, hyper_parmeter)) ? (powerplan_width / 2) : (min_spacing / 2));
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

void build_Gc(Graph &G, DICNIC<double> &Gc, Graph &G_the_other_dir)
{
	vector<edge> zero_slack_edges = G.zero_slack_edges();
	for (auto &e : zero_slack_edges)
	{
		if (e.from == 0 || e.to == V + 1)
		{
			Gc.add_edge(e.from, e.to, DBL_MAX, true);
			// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << DBL_MAX << '\n';
		}
		else
		{
			G_the_other_dir.add_edge(e.from, e.to, e.weight);
			double Rvj = G.R[e.to];
			double Lvi = G.L[e.from];
			double test_vertical_longest_path = G_the_other_dir.longest_path(false);
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
			G_the_other_dir.remove_edge(e.from, e.to);
		}
	}
}

void adjustment_helper(Graph &G, DICNIC<double> &Gc, Graph &G_the_other_dir)
{
	Gc.min_cut(0, V + 1);
	for (auto &e : Gc.cut_e)
	{
		G_the_other_dir.add_edge(e.pre, e.v, (macros[e.pre]->h() + macros[e.v]->h()) / 2);
		G.remove_edge(e.pre, e.v);
	}
}

void adjustment(Graph &Gh, Graph &Gv)
{
	bool need_to_rebuild = false;
	double tmp;
	double longest_path_h = Gh.longest_path(true);
	double longest_path_v = Gv.longest_path(false);

	cout << "Before adjustment:\n";
	cout << "Horizontal constraint graph has longest path: " << longest_path_h << '\n';
	cout << "Vertical constraint graph has longest path: " << longest_path_v << "\n\n";

	while (longest_path_h > chip_width || longest_path_v > chip_height)
	{
		double prev_longest_path_h = longest_path_h, prev_longest_path_v = longest_path_v;
		DICNIC<double> Gc;
		Gc.init(V);
		if (longest_path_h > chip_width && longest_path_v <= chip_height)
		{
			build_Gc(Gh, Gc, Gv);
			adjustment_helper(Gh, Gc, Gv);
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
			if (need_to_rebuild && longest_path_h < prev_longest_path_h)
			{
				need_to_rebuild = false;
				chip_height = tmp;
			}
		}
		else if (longest_path_h <= chip_width && longest_path_v > chip_height)
		{
			build_Gc(Gv, Gc, Gh);
			adjustment_helper(Gv, Gc, Gh);
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
		}
		else
		{
			if (!need_to_rebuild)
			{
				tmp = chip_height;
				chip_height = longest_path_v;
				need_to_rebuild = true;
			}
			else
			{
				// need to rebuild constraint graph
				Gh.rebuild();
				Gv.rebuild();
				build_init_constraint_graph(Gh, Gv, ++rebuild_cnt);
				re_index(macros);
				adjustment(Gh, Gv);
				return;
			}
		}
	}
	cout << "After adjustment:\n";
	cout << "Horizontal constraint graph has longest path: " << longest_path_h << '\n';
	cout << "Vertical constraint graph has longest path: " << longest_path_v << '\n';
}

int main(int argc, char *argv[])
{
	rng.seed(87);
	IoData* iodata;
	cout<<"fuck 0 \n";
	iodata = shoatingMain(argc, argv);

	cout<<"fuck 1 \n";

	chip_width = iodata->die_width;// = 25.0;
	chip_height = iodata->die_height;// = 10.0;
	V = iodata->macros.size();// = 7; // #macros;
	alpha = iodata->weight_alpha;// = 1.0, 
	beta = iodata->weight_beta;// = 4.0 ;
	powerplan_width = iodata->powerplan_width_constraint;// = 0.0, 
	min_spacing = iodata->minimum_spacing;// = 0.0;
	macros = iodata->macros;

	Graph Gh(V), Gv(V);
	cout<<"fuck 2 \n";
	build_init_constraint_graph(Gh, Gv, rebuild_cnt);
	re_index(macros);
	adjustment(Gh, Gv);
	Gh.transitive_reduction();
	Gv.transitive_reduction();
	// Gh, Gv are ready.

	return 0;
}
