#include "flow.h"
#include "graph.h"
#include "io.h"
#include "macro.h"
#include "LP.h"
#include "corner_stitch.h"
#include "corner_stitch/utils/update.h"
#include <iostream>
#include <random>
#include <vector>
#include <cmath>
#include <unistd.h>
#include <signal.h>

using namespace std;

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

double chip_width;		// = 25.0;
double chip_height;		// = 10.0;
int micron;
int V;					// = 7; // #macros;
double alpha;			// = 1.0,
double beta;			// = 4.0;
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
		w += min_spacing;
		// w += (lucky(beta / alpha)) ? powerplan_width : min_spacing;
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

bool build_Gc(Graph &G, DICNIC<double> *Gc, Graph &G_the_other_dir, bool test_g_is_horizontal) // using macros
{
	vector<edge> &zero_slack_edges = G.zero_slack_edges();
	// G.show();
	bool from_to_to;
	int cnt = 0;
	double w, Rvj, Lvi, test_longest_path, boundry, coord_from, coord_to, avg;
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
			Gc->add_edge(e.from, e.to, DBL_MAX, true);
			++cnt;
			// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << DBL_MAX << '\n';
		}
		else
		{
			cout << "2";
			w = determine_edge_weight(macros[e.from], macros[e.to], test_g_is_horizontal);
			if (test_g_is_horizontal) {
				if (macros[e.from]->cx() < macros[e.to]->cx()) {
					from_to_to = true;
				} else {
					from_to_to = false;
				}
			} else {
				if (macros[e.from]->cy() < macros[e.to]->cy()) {
					from_to_to = true;
				} else {
					from_to_to = false;
				}
			}
			if (from_to_to)
				G_the_other_dir.add_edge(e.from, e.to, w);
			else
				G_the_other_dir.add_edge(e.to, e.from, w);

			Rvj = G.R[e.to];
			Lvi = G.L[e.from];
			test_longest_path = G_the_other_dir.longest_path(test_g_is_horizontal);
			boundry = (test_g_is_horizontal) ? chip_width : chip_height;
			if (test_longest_path > boundry)
			{
				Gc->add_edge(e.from, e.to, DBL_MAX, true);
				++cnt;
				cout << "a\n";
			}
			else
			{
				avg = determine_edge_weight(macros[e.from], macros[e.to], test_g_is_horizontal);
				coord_from = (test_g_is_horizontal) ? macros[e.from]->cx() : macros[e.from]->cy();
				coord_to = (test_g_is_horizontal) ? macros[e.to]->cx() : macros[e.to]->cy();
				w = max(coord_from - Rvj + avg, 0) + max(Lvi + avg - coord_to, 0); //??
				Gc->add_edge(e.from, e.to, w, true);
				cout << "b " << w << endl;
				// cout << "adding edge from " << e.from << " to " << e.to << " with weight " << w << '\n';
			}
			if (from_to_to)
				G_the_other_dir.remove_edge(e.from, e.to);
			else
				G_the_other_dir.remove_edge(e.to, e.from);
				
		}
	}
	return cnt == zero_slack_edges.size();
}

bool adjustment_helper(Graph &G, DICNIC<double> *Gc, Graph &G_the_other_dir, bool adjust_g_is_horizontal) //using macros
{
	cout<<"In adjustment_helper"<<endl;
	if (Gc->min_cut(0, V + 1))
	{
		return true;
	}
	cout <<"Cut size: " <<Gc->cut_e.size() << endl;
	for (auto &e : Gc->cut_e)
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
	// if (G_the_other_dir.hasCycle()) {
	// 	cout << "Adjustment induces cycle";
	// 	return true;
	// }
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
		DICNIC<double>* Gc = new DICNIC<double>;
		Gc->init(V + 2);
		if (longest_path_h > chip_width && longest_path_v <= chip_height)
		{
			if (build_Gc(Gh, Gc, Gv, false))
			{
				cout << "???\n";
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			if (adjustment_helper(Gh, Gc, Gv, true))
			{
				cout << "123213\n";
				chip_height = copy_of_chip_height;
				rebuild_constraint_graph(Gh, Gv);
				return;
			}
			longest_path_h = Gh.longest_path(true);
			longest_path_v = Gv.longest_path(false);
			if (longest_path_h == prev_longest_path_h)
			{
				cout << "456456\n";
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
		delete Gc;
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
	//Gh.transitive_reduction();
	//Gv.transitive_reduction();
	adjustment(Gh, Gv);
}

double displacement_evaluation(vector<Macro*>& macro, vector<Macro*>& native_macro){
	double displacement_cost = 0;
	for(int i = 0; i < native_macro.size(); i++){
		displacement_cost += abs(macro[i]->x1() - native_macro[i]->x1());
		displacement_cost += abs(macro[i]->y1() - native_macro[i]->y1());
	}
	return displacement_cost / micron;
}

double total_cost(double displace, double powerplan){
	return alpha * displace + beta * sqrt(powerplan);
}

bool **modified;
void perturb_strategy(double P, Graph& Gv_next, Graph& Gh_next, vector<Macro *>& macros_next){
	vector<edge>* h_edge_list = Gh_next.get_edge_list();
	vector<edge>* v_edge_list = Gv_next.get_edge_list();
	vector<edge>* r_h_edge_list = Gh_next.get_reverse_edge_list();
	vector<edge>* r_v_edge_list = Gv_next.get_reverse_edge_list();
	int from, to, w;
	int cnt = 0;
	for(int i=0;i<V;i++){
		for(int j=0;j<h_edge_list[i].size();j++){
			if(modified[i][h_edge_list[i][j].to]==true)
				continue;
			if(unif(rng)<=0.005){
				from = h_edge_list[i][j].from;
				to = h_edge_list[i][j].to;
				w = h_edge_list[i][j].weight;
				if(from==0 || from>V || to==0 || to>V)
					continue;
				Gh_next.remove_edge(from, to);
				if(macros_next[from]->cy()<=macros_next[to]->cy())
					Gv_next.add_edge(from, to, w);
				else
					Gv_next.add_edge(to, from, w);
				modified[from][to] = true;
				modified[to][from] = true;
				cnt++;
			}
		}

		for(int j=0;j<v_edge_list[i].size();j++){
			if(modified[i][v_edge_list[i][j].to]==true)
				continue;
			if(unif(rng)<=0.005){
				from = v_edge_list[i][j].from;
				to = v_edge_list[i][j].to;
				w = v_edge_list[i][j].weight;
				if(from==0 || from>V || to==0 || to>V)
					continue;
				Gv_next.remove_edge(from, to);
				if(macros_next[from]->cx()<=macros_next[to]->cx())
					Gh_next.add_edge(from, to, w);
				else
					Gh_next.add_edge(to, from, w);
				modified[from][to] = true;
				modified[to][from] = true;
				cnt++;
			}
		}
	}
}


	IoData *iodatas;
vector<Macro *> macros_best(V);
void sigalrm_handler(int sig)
{
    // This gets called when the timer runs out.  Try not to do too much here;
    // the recommended practice is to set a flag (of type sig_atomic_t), and have
    // code elsewhere check that flag (e.g. in the main loop of your program)
	cout<<"time's up!!!!!!!!!!!!"<<endl;
	iodatas->macros = macros_best;
	output();
	exit(sig);
}

int main(int argc, char *argv[])
{
	rng.seed(87);
	signal(SIGALRM, &sigalrm_handler);  // set a signal handler
	alarm(900);  // set an alarm for 15*60 seconds from now

	iodatas = shoatingMain(argc, argv);
	chip_width = (double)iodatas->die_width;						  // = 25.0;
	chip_height = (double)iodatas->die_height;					  // = 10.0;
	micron = iodatas->dbu_per_micron;
	V = iodatas->macros.size();									  // = 7; // #macros;
	alpha = (double)iodatas->weight_alpha;						  // = 1.0,
	beta = (double)iodatas->weight_beta;							  // = 4.0 ;
	buffer_constraint = (double)iodatas->buffer_constraint;
	powerplan_width = (double)iodatas->powerplan_width_constraint; // = 0.0,
	min_spacing = (double)iodatas->minimum_spacing;				  // = 0.0;
	vector<Macro*> native_macros;
	for(int i = 0; i < iodatas->macros.size(); i++){
		native_macros.push_back(new Macro(iodatas->macros[i]->w(), iodatas->macros[i]->h(),
											iodatas->macros[i]->x1(), iodatas->macros[i]->y1(),
											iodatas->macros[i]->is_fixed(), iodatas->macros[i]->id()));
	}
	og_macros = iodatas->macros;
	macros = new Macro *[V + 5];
	for (auto &m : og_macros)
		macros[m->id()] = m;
	Graph Gh(V), Gv(V);
	build_init_constraint_graph(Gh, Gv, og_macros);
	//Gh.transitive_reduction();
	//Gv.transitive_reduction();
	adjustment(Gh, Gv);
	
	// Gh, Gv are ready.
	// To change this function to return vector<pair<double, double>>
	// Please refer the annotation in bottom of LP.cpp
	// I also can modify macro[i].x, macro[i].y directly
	// If you want to do so, wellcom to contact me
	
	// ====================Important====================
	// Return values represent macros' "center" position
	// =================================================

	Linear_Program(og_macros, Gv, Gh);

	double displacement = displacement_evaluation(og_macros, native_macros);
	printf("Total displacement = %lf\n", displacement);

	// Create horizontal, vertical corner stitch data structure
	Plane* horizontal_plane = CreateTilePlane(), *vertical_plane = CreateTilePlane();

	// Calculate powerplan cost
	double powerplan_cost;
	powerplan_cost = cost_evaluation(og_macros, horizontal_plane, vertical_plane);

	// Vector stores invalid macros found by corner stitch data structure
	vector<int> invalid_macros;
	invalid_macros = invalid_check(og_macros, horizontal_plane);

	printf("Total cost = %lf\n", total_cost(displacement, powerplan_cost));

	// If exist any invalid macros, then we only try to legalize placement result in this round
	if(!invalid_macros.empty()){
		fix_invalid(og_macros, invalid_macros, Gh, Gv);
	}
	// If current placement result is legal, then we try to improve our result
	else{
		// improvement strategy :
		// 1. force macros near boundary to align boundary
		// 2. reduce powerplan cost in sparse region
		improve_strategy1(og_macros, native_macros, horizontal_plane, Gh, Gv);
		improve_strategy2(og_macros, horizontal_plane, Gh, Gv);
	}

	// Remove tile plane
	RemoveTilePlane(horizontal_plane);
	RemoveTilePlane(vertical_plane);

	// ======================
	// end of post processing
	// ======================

	// ==============
	// run next round
	// ==============
	
	adjustment(Gh, Gv);
	Linear_Program(og_macros, Gv, Gh);

	displacement = displacement_evaluation(og_macros, native_macros);
	printf("Total displacement = %lf\n", displacement);

	horizontal_plane = CreateTilePlane();
	vertical_plane = CreateTilePlane();

	// Calculate powerplan cost
	//double powerplan_cost;
	powerplan_cost = 0;
	powerplan_cost = cost_evaluation(og_macros, horizontal_plane, vertical_plane);

	// Vector stores invalid macros found by corner stitch data structure
	//vector<int> invalid_macros;
	invalid_macros.clear();
	invalid_macros = invalid_check(og_macros, horizontal_plane);

	double cost_now = total_cost(displacement, powerplan_cost);
	printf("Total cost = %lf\n", cost_now);

	//RemoveTilePlane(horizontal_plane);// if no SA un-comment these
	//RemoveTilePlane(vertical_plane);
//------------------------------------------------------------------------------  SA
	double T_cur, T_end, P, rate, cost_best, cost_next;
	int num_perturb_per_T;
	vector<Macro *> macros_next(V);
	macros_best.resize(V);
	Graph Gv_next(V), Gh_next(V), Gv_best(V), Gh_best(V);
	// args
	P = 0.8;//possibility of accepting worse solution at begining
	T_cur = -10000/log(P);//delta(avg) / ln(P)
	rate = 0.1;
	num_perturb_per_T = 2;
	T_end = 1;
	Gv_best.Copy(Gv);
	Gh_best.Copy(Gh);
	for(int j=0;j<V;j++){
		macros_next[j] = new Macro(*og_macros[j]);
		macros_best[j] = new Macro(*og_macros[j]);
	}
	cost_best = cost_now;
	modified = new bool*[V+5]; //modified
	for(int i=0;i<V+5;i++){
		modified[i] = new bool[V+5];
		for(int j=0;j<V+5;j++){
			modified[i][j] = false;
		}
	}
	while(T_cur>T_end){
		cout<<"Temp:"<<T_cur<<" begin"<<endl;
		for(int i=0;i<num_perturb_per_T;i++){
			//sNext = Perturb(sNow);
			Gv_next.rebuild();
			Gh_next.rebuild();
			Gv_next.Copy(Gv);
			Gh_next.Copy(Gh);
			for(int j=0;j<V;j++){
				delete macros_next[j];
				macros_next[j] = new Macro(*og_macros[j]);
			}

			//   perturb
			perturb_strategy(T_cur/T_end, Gv_next, Gh_next, macros_next);

			adjustment(Gh_next, Gv_next);
			Linear_Program(macros_next, Gv_next, Gh_next);
			//if (cost(sNext) < cost(sNow)){
			displacement = displacement_evaluation(macros_next, native_macros);
			RemoveTilePlane(horizontal_plane);
			RemoveTilePlane(vertical_plane);
			horizontal_plane = CreateTilePlane();
			vertical_plane = CreateTilePlane();
			powerplan_cost = 0;
			powerplan_cost = cost_evaluation(macros_next, horizontal_plane, vertical_plane);
			cost_next = total_cost(displacement, powerplan_cost);//unif(rng)*100;//
			cout<<"SA before copy, cost(next, now):"<<cost_next<<", "<<cost_now<<endl;
			if(cost_next<cost_now){
				//sNow = sNext;
				Gv.rebuild();
				Gh.rebuild();
				Gv.Copy(Gv_next);
				Gh.Copy(Gh_next);
				cost_now = cost_next;
				for(int k=0;k<V;k++){
					delete og_macros[k];
					og_macros[k] = new Macro(*macros_next[k]);
				}
				// if (cost(sNow) < cost(sBest))
				// 	sBest = sNow;
				if(cost_now<cost_best){
					Gv_best.rebuild();
					Gh_best.rebuild();
					Gv_best.Copy(Gv);
					Gh_best.Copy(Gh);
					cost_best = cost_now;
					for(int k=0;k<V;k++){
						delete macros_best[k];
						macros_best[k] = new Macro(*og_macros[k]);
					}
				}
			//}
			}
			//else if (Accept(T, cost(sNext)-cost(sNow)))
			else if(exp(-(cost_next-cost_now)/T_cur)>unif(rng)){
				//sNow = sNext;
				Gv.rebuild();
				Gh.rebuild();
				Gv.Copy(Gv_next);
				Gh.Copy(Gh_next);
				cost_now = cost_next;
				for(int k=0;k<V;k++){
					delete og_macros[k];
					og_macros[k] = new Macro(*macros_next[k]);
				}
			}
			cout<<"SA after copy, cur_T:"<<T_cur<<", costNow:"<<cost_now<<", costBest:"<<cost_best<<endl;
		}

		T_cur*=rate;
	}
	iodatas->macros = macros_best;
	//----------------------------------------------------------------- SA

	output();
	return 0;
}
