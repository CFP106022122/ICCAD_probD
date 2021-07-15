#include "LP.h"
#include "flow.h"
#include "graph.h"
#include "io.h"
#include "macro.h"
#include <iostream>
#include <random>
#include <vector>

using namespace std;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

double chip_width;	// = 25.0;
double chip_height; // = 10.0;
int V;				// = 7; // #macros;
double alpha;		// = 1.0,
double beta;		// = 4.0;
double buffer_constraint;
double powerplan_width; // = 0.0,
double min_spacing;		// = 0.0;
Macro **macros;
vector<Macro *> og_macros;
IoData *shoatingMain(int argc, char *argv[]);
void output();

double hyper_parameter = 0.1;
int rebuild_cnt;

mt19937_64 rng;
std::uniform_real_distribution<double> unif(0.0, 1.0);

void rebuild_constraint_graph(Graph &Gh, Graph &Gv);
bool lucky(double ratio)
{
	double x = unif(rng);
	double y = max(ratio / (ratio + 1.0) - (double)rebuild_cnt * hyper_parameter, 0);
	return x < y;
}

double determine_edge_weight(Macro *m1, Macro *m2, bool is_horizontal)
{
	double w = (is_horizontal) ? (m1->w() + m2->w()) / 2 : (m1->h() + m2->h()) / 2;
	if (m1->name() != "null" && m2->name() != "null")
		w += (lucky(beta / alpha)) ? powerplan_width : min_spacing;
	return w;
}

void add_st_nodes(Graph &Gh, Graph &Gv, vector<Macro *> macros) // using og_macros here
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

void build_init_constraint_graph(Graph &Gh, Graph &Gv, vector<Macro *> macros) // using og_macros here
{
	add_st_nodes(Gh, Gv, macros);
	for (int i = 0; i < V; ++i)
	{
		for (int j = i + 1; j < V; ++j)
		{
			double h_weight = determine_edge_weight(macros[i], macros[j], true);
			double v_weight = determine_edge_weight(macros[i], macros[j], false);
			bool i_is_at_the_bottom = false, i_is_at_the_left = false;
			if (macros[i]->cx() < macros[j]->cx())
				i_is_at_the_left = true;
			if (macros[i]->cy() < macros[j]->cy())
				i_is_at_the_bottom = true;

			if (is_overlapped(*macros[i], *macros[j]))
			{
				if (x_dir_is_overlapped_less(*macros[i], *macros[j]))
				{
					(i_is_at_the_left) ? Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight)
									   : Gh.add_edge(macros[j]->id(), macros[i]->id(), h_weight);
				}
				else
				{
					(i_is_at_the_bottom) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
										 : Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
			}
			else if (projection_no_overlapped(*macros[i], *macros[j]))
			{
				double diff_x, diff_y;
				diff_x = (i_is_at_the_left) ? macros[j]->x1() - macros[i]->x2()
											: macros[i]->x1() - macros[j]->x2();
				diff_y = (i_is_at_the_bottom) ? macros[j]->y1() - macros[i]->y2()
											  : macros[i]->y1() - macros[j]->y2();
				if (diff_x < diff_y)
				{
					(i_is_at_the_bottom) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
										 : Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
				else
				{
					(i_is_at_the_left) ? Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight)
									   : Gh.add_edge(macros[j]->id(), macros[i]->id(), h_weight);
				}
			}
			else
			{
				if (x_dir_projection_no_overlapped(*macros[i], *macros[j]))
				{

					(i_is_at_the_left) ? Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight)
									   : Gh.add_edge(macros[j]->id(), macros[i]->id(), h_weight);
				}
				else if (y_dir_projection_no_overlapped(*macros[i], *macros[j]))
				{
					(i_is_at_the_bottom) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
										 : Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
				else
				{
					throw "error";
				}
			}
		}
	}
}

bool build_Gc(Graph &G, DICNIC<double> &Gc, Graph &G_the_other_dir, bool test_g_is_horizontal) // using macros
{
	vector<edge> &zero_slack_edges = G.zero_slack_edges();
	// G.show();
	int cnt = 0;
	for (auto &e : zero_slack_edges)
	{
		if (e.from != 0)
			cout << macros[e.from]->name() << ' ';
		else
			cout << "source ";
		if (e.to != V + 1)
			cout << macros[e.to]->name() << " ";
		else
			cout << "sink ";
		if (e.from == 0 || e.to == V + 1)
		{
			cout << "1\n";
			Gc.add_edge(e.from, e.to, DBL_MAX, true);
			++cnt;
			// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << DBL_MAX << '\n';
		}
		else
		{
			cout << "2";
			double w = determine_edge_weight(macros[e.from], macros[e.to], test_g_is_horizontal);
			G_the_other_dir.add_edge(e.from, e.to, w);
			double Rvj = G.R[e.to];
			double Lvi = G.L[e.from];
			double test_longest_path = G_the_other_dir.longest_path(test_g_is_horizontal);
			double boundry = (test_g_is_horizontal) ? chip_width : chip_height;
			if (test_longest_path > boundry)
			{
				Gc.add_edge(e.from, e.to, DBL_MAX, true);
				++cnt;
				cout << "a\n";
			}
			else
			{
				double avg = determine_edge_weight(macros[e.from], macros[e.to], test_g_is_horizontal);
				double coord_from = (test_g_is_horizontal) ? macros[e.from]->cx() : macros[e.from]->cy();
				double coord_to = (test_g_is_horizontal) ? macros[e.to]->cx() : macros[e.to]->cy();
				w = max(coord_from - Rvj + avg, 0) + max(Lvi + avg - coord_to, 0);
				Gc.add_edge(e.from, e.to, w, true);
				cout << "b " << w << endl;
				// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << w << '\n';
			}
			G_the_other_dir.remove_edge(e.from, e.to);
		}
	}
	return cnt == zero_slack_edges.size();
}

bool adjustment_helper(Graph &G, DICNIC<double> &Gc, Graph &G_the_other_dir, bool adjust_g_is_horizontal) //using macros
{
	if (Gc.min_cut(0, V + 1))
	{
		return true;
	}
	for (auto &e : Gc.cut_e)
	{
		cout << e.pre << ' ' << e.v << endl;
		double w = determine_edge_weight(macros[e.pre], macros[e.v], !adjust_g_is_horizontal);
		if (adjust_g_is_horizontal)
		{
			if (macros[e.pre]->cy() < macros[e.v]->cy())
			{
				G_the_other_dir.add_edge(e.pre, e.v, w);
			}
			else
			{
				G_the_other_dir.add_edge(e.v, e.pre, w);
			}
		}
		else
		{
			if (macros[e.pre]->cx() < macros[e.v]->cx())
			{
				G_the_other_dir.add_edge(e.pre, e.v, w);
			}
			else
			{
				G_the_other_dir.add_edge(e.v, e.pre, w);
			}
		}
		G.remove_edge(e.pre, e.v);
	}
	return false;
}

void adjustment(Graph &Gh, Graph &Gv)
{
	bool has_changed_chip_height = false;
	double tmp;
	double longest_path_h = Gh.longest_path(true);
	double longest_path_v = Gv.longest_path(false);
	double copy_of_chip_height = chip_height; // for safety
	cout << "Before adjustment:\n";
	cout << "Horizontal constraint graph has longest path: " << longest_path_h << '\n';
	cout << "Vertical constraint graph has longest path: " << longest_path_v << "\n\n";

	while (longest_path_h > chip_width || longest_path_v > chip_height)
	{
		cout << "Now:\n";
		cout << "Horizontal constraint graph has longest path: " << longest_path_h << '\n';
		cout << "Vertical constraint graph has longest path: " << longest_path_v << "\n\n";

		double prev_longest_path_h = longest_path_h, prev_longest_path_v = longest_path_v;
		DICNIC<double> Gc;
		Gc.init(V + 2);
		if (longest_path_h > chip_width && longest_path_v <= chip_height)
		{
			if (build_Gc(Gh, Gc, Gv, false))
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			if (adjustment_helper(Gh, Gc, Gv, true))
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
			if (longest_path_h >= prev_longest_path_h)
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			if (has_changed_chip_height)
			{
				has_changed_chip_height = false;
				chip_height = tmp;
			}
		}
		else if (longest_path_h <= chip_width && longest_path_v > chip_height)
		{
			if (build_Gc(Gv, Gc, Gh, true))
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			if (adjustment_helper(Gv, Gc, Gh, false))
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
			if (longest_path_v >= prev_longest_path_v)
			{
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
		}
		else
		{
			if (!has_changed_chip_height)
			{
				tmp = chip_height;
				chip_height = longest_path_v;
				has_changed_chip_height = true;
			}
		}
	}
	cout << "After adjustment:\n";
	cout << "Horizontal constraint graph has longest path: " << longest_path_h << " less than chip_width: " << chip_width << '\n';
	cout << "Vertical constraint graph has longest path: " << longest_path_v << " less than chip_height: " << copy_of_chip_height << '\n';
	chip_height = copy_of_chip_height;
}

void rebuild_constraint_graph(Graph &Gh, Graph &Gv)
{
	if (++rebuild_cnt > 10)
	{
		cout << "End of the world\n";
		return;
	}
	// cout << rebuild_cnt << "th time rebuild\n";
	Gh.rebuild();
	Gv.rebuild();
	build_init_constraint_graph(Gh, Gv, og_macros);
	Gh.transitive_reduction();
	Gv.transitive_reduction();
	adjustment(Gh, Gv);
}

int main(int argc, char *argv[])
{
	rng.seed(87);

	IoData *iodata;
	iodata = shoatingMain(argc, argv);
	chip_width = (double)iodata->die_width;	  // = 25.0;
	chip_height = (double)iodata->die_height; // = 10.0;
	V = iodata->macros.size();				  // = 7; // #macros;
	alpha = (double)iodata->weight_alpha;	  // = 1.0,
	beta = (double)iodata->weight_beta;		  // = 4.0 ;
	buffer_constraint = (double)iodata->buffer_constraint;
	powerplan_width = (double)iodata->powerplan_width_constraint; // = 0.0,
	min_spacing = (double)iodata->minimum_spacing;				  // = 0.0;

	og_macros = iodata->macros;
	macros = new Macro *[V + 5];
	for (auto &m : og_macros)
		macros[m->id()] = m;
	Graph Gh(V), Gv(V);
	build_init_constraint_graph(Gh, Gv, og_macros);
	Gh.transitive_reduction();
	Gv.transitive_reduction();
	adjustment(Gh, Gv);
	// Gh, Gv are ready.
	//Gh.show();

	// To change this function to return vector<pair<double, double>>
	// Please refer the annotation in bottom of LP.cpp
	// I also can modify macro[i].x, macro[i].y directly
	// If you want to do so, wellcom to contact me

	// ====================Important====================
	// Return values represent macros' "center" position
	// =================================================
	Linear_Program(og_macros, Gv, Gh);

	Linear_Check_Sol(og_macros, Gh, Gv);

	output();
	return 0;
}
