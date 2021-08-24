#include <iostream>
#include <pthread.h>
#include <vector>
#include <set>
#include "macro.h"
#include "graph.h"
#include "flow.h"
#include "LP.h"

using namespace std;
extern double chip_width, chip_height, buffer_constraint, powerplan_width;
# define ERROR() { fprintf(stderr, "Error\n"); exit(1); }

class thread_param{
    private:
        vector<Macro*>* macro;
        vector<edge>* edge_list;
        bool is_horizontal;
        double* solution;
    public:
        thread_param(vector<Macro*>& macros, vector<edge>* edge_list, bool is_horizontal){
            this->macro = &macros;
            this->edge_list = edge_list;
            this->is_horizontal = is_horizontal;
            this->solution = new double[macros.size()];
        }
        vector<Macro*>* get_macros(){
            return this->macro;
        }
        vector<edge>* get_edge_list(){
            return this->edge_list;
        }
        bool get_is_horizontal(){
            return this->is_horizontal;
        }
        double* get_solution_ptr(){
            return this->solution;
        }
};

void* pthread_Linear_Program(void* _param){
	// read pthread parameters
	thread_param* param = (thread_param *) _param;
    vector<Macro*>* macro = param->get_macros();
    vector<edge>* edge_list = param->get_edge_list();
    bool is_horizontal = param->get_is_horizontal();
    double* solution = param->get_solution_ptr();

    lprec* model;
    int n = (*macro).size();

    if((model = make_lp(0, 4 * n + 2)) == NULL){
        ERROR();
    }

    if(is_horizontal){
        set_upbo(model, 1, 0);
        set_lowbo(model, 1, 0);
        set_upbo(model, n + 2, chip_width);
        set_lowbo(model, n + 2, chip_width);

        for(int i = 0; i < (*macro).size(); i++){
            //set "up boundary" for  macro's x origin position
            set_upbo(model, (*macro)[i]->id() + n + 2, (*macro)[i]->cx());
            //set "low boundary" for macro's x origin position
            set_lowbo(model, (*macro)[i]->id() + n + 2, (*macro)[i]->cx());
        }

        for(int i = 0; i < (*macro).size(); i++){
            if((*macro)[i]->is_fixed() == true){
                //set "up boundary" for fixed macro's x position
                set_upbo(model, (*macro)[i]->id() + 1, (*macro)[i]->cx());
                //set "low boundary" for fixed macro's x position
                set_lowbo(model, (*macro)[i]->id() + 1, (*macro)[i]->cx());
            }
            else{
                //set "up boundary" for macro's x position
                set_upbo(model, (*macro)[i]->id() + 1, chip_width - (*macro)[i]->w() / 2);
                //set "low boundary" for macro's x position
                set_lowbo(model, (*macro)[i]->id() + 1, (*macro)[i]->w() / 2);
            }
        }
    }
    else{
        set_upbo(model, 1, 0);
        set_lowbo(model, 1, 0);
        set_upbo(model, n + 2, chip_height);
        set_lowbo(model, n + 2, chip_height);

        for(int i = 0; i < (*macro).size(); i++){
            //set "up boundary" for  macro's y origin position
            set_upbo(model, (*macro)[i]->id() + n + 2, (*macro)[i]->cy());
            //set "low boundary" for macro's y origin position
            set_lowbo(model, (*macro)[i]->id() + n + 2, (*macro)[i]->cy());
        }

        for(int i = 0; i < (*macro).size(); i++){
            if((*macro)[i]->is_fixed() == true){
                //set "up boundary" for fixed macro's y position
                set_upbo(model, (*macro)[i]->id() + 1, (*macro)[i]->cy());
                //set "low boundary" for fixed macro's y position
                set_lowbo(model, (*macro)[i]->id() + 1, (*macro)[i]->cy());
            }
            else{
                //set "up boundary" for macro's y position
                set_upbo(model, (*macro)[i]->id() + 1, chip_height - (*macro)[i]->h() / 2);
                //set "low boundary" for macro's y position
                set_lowbo(model, (*macro)[i]->id() + 1, (*macro)[i]->h() / 2);
            }
        }
    }

    double sparserow[2];
    int colno[2];
    //add_horizontal(vertical)_constraints
    for(int i = 0; i <= n; i++){
        for (auto& edge:edge_list[i]) {
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

    double sparserow_3[3];
    int colno_3[3];
    for(int i = 0; i < n; i++){
        //  xi' - xi + dxai >= 0
        // (yi' - yi + dyai >= 0)
        sparserow_3[0] = 1;
        sparserow_3[1] = -1;
        sparserow_3[2] = 1;
        colno_3[0] = i + 2;
        colno_3[1] = i + n + 3;
        colno_3[2] = i + 2 * n + 3;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }
    for(int i = 0; i < n; i++){
        //  -xi' + xi + dxbi >= 0
        // (-yi' + yi + dybi >= 0)
        sparserow_3[0] = -1;
        sparserow_3[1] = 1;
        sparserow_3[2] = 1;
        colno_3[0] = i + 2;
        colno_3[1] = i + n + 3;
        colno_3[2] = i + 3 * n + 3;
        add_constraintex(model, 3, sparserow_3, colno_3, GE, 0);
    }

    // // Set objective function
    // // Minimize summation of dxa1 ~ dxan, dxb1 ~ dxbn(dya1 ~ dyan, dyb1 ~ dybn)
    double* obj_fn_sparse = new double[2 * n];
    int* obj_fn_colno = new int[2 * n];
    for(int i = 0; i < 2 * n; i++){
        obj_fn_sparse[i] = 1;
        obj_fn_colno[i] = i + 2 * n + 3;
    }
    set_obj_fnex(model, 2 * n, obj_fn_sparse, obj_fn_colno);

    solve(model);

    // Read "all" variables in LP model
    double model_solution[4 * n + 2];
    get_variables(model, model_solution);

    // Read useful varibles, i.e. macros' x, y position
    for(int i = 0; i < n; i++){
        solution[i] = model_solution[i + 1];
    }
    
    delete_lp(model);

    return NULL;
}


void Linear_Program(vector<Macro*>& macro, Graph& Gv, Graph& Gh){

    thread_param* horizontal_params = new thread_param(macro, Gh.get_edge_list(), true);
    thread_param* vertical_params = new thread_param(macro, Gv.get_edge_list(), false);

    pthread_t horizontal_thread, vertical_thread;

    pthread_create(&horizontal_thread, NULL, pthread_Linear_Program, horizontal_params);
    pthread_create(&vertical_thread, NULL, pthread_Linear_Program, vertical_params);

    pthread_join(horizontal_thread, NULL);
    pthread_join(vertical_thread, NULL);

    double* x_solution = horizontal_params->get_solution_ptr();
    double* y_solution = vertical_params->get_solution_ptr();

    for(int i = 0; i < macro.size(); i++){
        macro[i]->updateXY(make_pair(x_solution[i], y_solution[i]));
    }
}