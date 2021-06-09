#include <iostream>
#include <vector>
#include "macro.h"
#include "graph.h"

using namespace std;

double chip_width = 25.0;
double chip_height = 10.0;

const int V = 7;
vector<Macro*> macros;

void build_init_constraint_graph(Graph& Gh, Graph& Gv) {
	for (int i=0; i<V; ++i) {
		for (int j=i+1; j<V; ++j) {
			double h_weight = (macros[i]->w()+ macros[j]->w())/2;
			double v_weight = (macros[i]->h()+ macros[j]->h())/2;
			if (is_overlapped(*macros[i], *macros[j])) {
				if (x_dir_is_overlapped_less(*macros[i], *macros[j])) { 
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				} else { 
					(macros[i]->cy() < macros[j]->cy()) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
					: Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				}
			} else if (projection_no_overlapped(*macros[i], *macros[j])) {
				if (macros[i]->cx() > macros[j]->cy())
					swap(*macros[i], *macros[j]);
				double diff_x, diff_y;
				bool i_is_at_the_bottom = false;
				diff_x = macros[j]->x1() - macros[i]->x2();
				if (macros[i]->cy() < macros[j]->cy()) {
					diff_y = macros[j]->y1() - macros[i]->y2();
					i_is_at_the_bottom = true;
				} else { 
					diff_y = macros[i]->y1() - macros[j]->y2();
				}
				if (diff_x < diff_y) {
					(i_is_at_the_bottom) ? Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight)
					: Gv.add_edge(macros[j]->id(), macros[i]->id(), v_weight);
				} else {
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				}
			} else {
				if (x_dir_projection_no_overlapped(*macros[i], *macros[j]))
					Gh.add_edge(macros[i]->id(), macros[j]->id(), h_weight);
				else if (y_dir_projection_no_overlapped(*macros[i], *macros[j]))
					Gv.add_edge(macros[i]->id(), macros[j]->id(), v_weight);
			}
		}
	}
}


int main(int argc, char* argv[]) {
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
	cout << "In horizontal constraint graph:\n";
	Gh.show();
	cout << "\nIn vertical constraint graph:\n";
	Gv.show();

	return 0;
}
