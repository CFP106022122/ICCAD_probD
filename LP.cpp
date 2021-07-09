#include <iostream>
#include <vector>
#include "macro.h"
#include "graph.h"
#include "flow.h"
#include "LP.h"

using namespace std;
extern double chip_width, chip_height;
# define ERROR() { fprintf(stderr, "Error\n"); exit(1); }

std::vector<std::pair<double, double>> Linear_Program(vector<Macro*>& macro, Graph& Gv, Graph& Gh){
    lprec *model;
    int n = macro.size();
    if ((model = make_lp(0, 2 * (n + 2) + 6 * n)) == NULL)
        // 0 constraints, 2 * (n + 2) + 6 * n variables
        ERROR();

    // set boundary for xs, xt, ys, yt
    set_upbo(model, 1, 0);
    set_lowbo(model, 1, 0);
    set_upbo(model, n + 2, chip_width);
    set_lowbo(model, n + 2, chip_width);
    set_upbo(model, n + 2 + 1, 0);
    set_lowbo(model, n + 2 + 1, 0);
    set_upbo(model, 2 * n + 4, chip_height);
    set_lowbo(model, 2 * n + 4, chip_height);

    // Set variables represent origin position of macro
    for(int i = 0; i < macro.size(); i++){
        //set "up boundary" for  macro's x origin position
        set_upbo(model, macro[i]->id() + 2 * n + 4, macro[i]->cx());
        //set "low boundary" for macro's x origin position
        set_lowbo(model, macro[i]->id() + 2 * n + 4, macro[i]->cx());
        //set "up boundary" for macro's y origin position
        set_upbo(model, macro[i]->id() + 3 * n + 4, macro[i]->cy());
        //set "low boundary" for macro's y origin position
        set_lowbo(model, macro[i]->id() + 3 * n + 4, macro[i]->cy());
    }


    for(int i = 0; i < macro.size(); i++){
        if(macro[i]->is_fixed() == true){
            //set "up boundary" for fixed macro's x position
            set_upbo(model, macro[i]->id() + 1, macro[i]->cx());
            //set "low boundary" for fixed macro's x position
            set_lowbo(model, macro[i]->id() + 1, macro[i]->cx());
            //set "up boundary" for fixed macro's y position
            set_upbo(model, macro[i]->id() + 3 + n, macro[i]->cy());
            //set "low boundary" for fixed macro's y position
            set_lowbo(model, macro[i]->id() + 3 + n, macro[i]->cy());
        }
        else{
            //set "up boundary" for macro's x position
            set_upbo(model, macro[i]->id() + 1, chip_width - macro[i]->w() / 2);
            //set "low boundary" for macro's x position
            set_lowbo(model, macro[i]->id() + 1, macro[i]->w() / 2);
            //set "up boundary" for macro's y position
            set_upbo(model, macro[i]->id() + 3 + n, chip_height - macro[i]->h() / 2);
            //set "low boundary" for macro's y position
            set_lowbo(model, macro[i]->id() + 3 + n, macro[i]->h() / 2);
        }
    }

    vector<edge>* edge_list = NULL;
    double sparserow[2];
    int colno[2];

    // add_vertical_constraints
    const vector<edge>* v_edge_list = Gv.get_edge_list();
    for(int i = 0; i <= n; i++){
        for (auto& edge:v_edge_list[i]) {
            // method1(faster when many zero terms)
            sparserow[0] = -1;
            sparserow[1] = 1;
            colno[0] = i + n + 3;
            colno[1] = edge.to + n + 3;
            add_constraintex(model, 2, sparserow, colno, GE, edge.weight);

            // method2
            // ==============================
            // double* row = new double[2 * (n + 2)]();
            // row[i + n + 3] = -1;
            // row[edge.to + n + 3] = 1;
            // add_constraint(model, row, GE, edge.weight / 2);
            // ==============================
		}
    }

    //add_horizontal_constraints
    const vector<edge>* h_edge_list = Gh.get_edge_list();
    for(int i = 0; i <= n; i++){
        for (auto& edge:h_edge_list[i]) {
            // method1(faster when many zero terms)
            sparserow[0] = -1;
            sparserow[1] = 1;
            colno[0] = i + 1;
            colno[1] = edge.to + 1;
            add_constraintex(model, 2, sparserow, colno, GE, edge.weight);

            // method2
            // ==============================
            // double* row = new double[2 * (n + 2)](0);
            // row[i + 1] = -1;
            // row[edge.to + 1] = 1;
            // add_constraint(model, row, GE, edge.weight / 2);
            // ==============================
		}
    }

    // Introduce dxa1 ~ dxan, dxb1 ~ dxbn, dya1 ~ dyan, dyb1 ~ dybn
    
    // Add constraint xi' - xi + dxa1 >=  0 and -xi' + xi + dxb1 >=  0
    // By this way, must one of dxa1 or dxb1 is zero,
    // and the the other will be |xi' - xi|
    // Then we add all of these terms to get total displacement

    double sparserow_3[3];
    int colno_3[3];
    for(int i = 0; i < n; i++){
        // xi' - xi + dxai >= 0
        sparserow_3[0] = 1;
        sparserow_3[1] = -1;
        sparserow_3[2] = 1;
        colno_3[0] = i + 2;
        colno_3[1] = i + n * 2 + 5;
        colno_3[2] = i + n * 4 + 5;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }
    for(int i = 0; i < n; i++){
        // -xi' + xi + dxbi >= 0
        sparserow_3[0] = -1;
        sparserow_3[1] = 1;
        sparserow_3[2] = 1;
        colno_3[0] = i + 2;
        colno_3[1] = i + n * 2 + 5;
        colno_3[2] = i + n * 5 + 5;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }
    for(int i = 0; i < n; i++){
        // yi' - yi + dyai >= 0
        sparserow_3[0] = 1;
        sparserow_3[1] = -1;
        sparserow_3[2] = 1;
        colno_3[0] = i + n + 4;
        colno_3[1] = i + n * 3 + 5;
        colno_3[2] = i + n * 6 + 5;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }
    for(int i = 0; i < n; i++){
        // -yi' + yi + dyai >= 0
        sparserow_3[0] = -1;
        sparserow_3[1] = 1;
        sparserow_3[2] = 1;
        colno_3[0] = i + n + 4;
        colno_3[1] = i + n * 3 + 5;
        colno_3[2] = i + n * 7 + 5;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }    

    // Set objective function
    // Minimize summation of dxa1 ~ dxan, dxb1 ~ dxbn, dya1 ~ dyan, dyb1 ~ dybn

    double* obj_fn_sparse = new double[n * 4];
    int* obj_fn_colno = new int[n * 4];
    for(int i = 0; i < 4 * n; i++){
        obj_fn_sparse[i] = 1;
        obj_fn_colno[i] = i + n * 4 + 5;
    }
    set_obj_fnex(model, 4 * n, obj_fn_sparse, obj_fn_colno);

    // Show the LP model
    // =================
    // print_lp(model);
    // =================

    // Solve LP model
    solve(model);
    // Show result of LP
    // =================
    // printf("%d",model);
    // print_objective(model);
    // print_solution(model, 1);
    // =================

    // Read "all" variables in LP model
    double model_solution[2 * (n + 2) + 6 * n];
    get_variables(model, model_solution);

    // Read useful varibles, i.e. macros' x, y position
    std::vector<std::pair<double, double>> solution;
    for(int i = 0; i < n; i++){
        solution.push_back(make_pair(model_solution[i + 1], model_solution[i + n + 3]));
    }
    
    // Show the x, y position for each mocro
    for(int i = 0; i < n; i++){
        printf("macro %d: x = %lf, y = %lf\n", i + 1, solution[i].first, solution[i].second);
    }

    delete_lp(model);

    // To change this function to return vector<pair<double, double>>
    // You can modify the declaration of this function as 

    // vector<pair(double, double)> Linear_Program(vector<Macro*>& macro, Graph& Gv, Graph& Gh)
    // then add following code
    // ===========================
    return solution;
    // ===========================
}