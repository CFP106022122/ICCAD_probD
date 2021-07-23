#include <iostream>
#include <vector>
#include <set>
#include "macro.h"
#include "graph.h"
#include "flow.h"
#include "LP.h"

using namespace std;
extern double chip_width, chip_height, buffer_constraint, powerplan_width;
# define ERROR() { fprintf(stderr, "Error\n"); exit(1); }

void Linear_Program(vector<Macro*>& macro, Graph& Gv, Graph& Gh){
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
    vector<pair<double, double>> solution;
    for(int i = 0; i < n; i++){
        solution.push_back(make_pair(model_solution[i + 1], model_solution[i + n + 3]));
    }
    
    // Show the x, y position for each mocro
    for(int i = 0; i < n; i++){
        printf("macro %d: x = %lf, y = %lf\n", i + 1, solution[i].first, solution[i].second);
    }

    // update macros' position by result from LP
    for(int i = 0; i < n; i++){
        macro[i]->updateXY(solution[i]);
    }

    // get total displacement
    double total_displacement = 0;
    total_displacement = get_objective(model);
    // printf("Total displacement = %lf\n", total_displacement);

    delete_lp(model);

    // To change this function to return vector<pair<double, double>>
    // You can modify the declaration of this function as 

    // vector<pair(double, double)> Linear_Program(vector<Macro*>& macro, Graph& Gv, Graph& Gh)
    // then add following code
    // ===========================
    // return solution;
    // ===========================
}

// ===============================
// Mind that in all DFS function, we use the area without concern the area exceed chip_boundary
// ===============================

void U_Horizontal_DFS(int target, int node, vector<edge>* v_edge_list, vector<Macro*>& macro, vector<int>& overlapped_macros, vector<bool>& visited){
    for(int i = 0; i < v_edge_list[node].size(); i++){
        if(!visited[v_edge_list[node][i].to]){
            visited[v_edge_list[node][i].to] = true;
            int current = v_edge_list[node][i].to - 1;
            if(macro[current]->y1() < macro[target]->y1() + macro[target]->h() + buffer_constraint + powerplan_width){
                if(!(macro[current]->x1() > macro[target]->x1() + macro[target]->w() + buffer_constraint + powerplan_width ||
                    macro[current]->x1() + macro[current]->w() < macro[target]->x1() - buffer_constraint - powerplan_width)){
                    overlapped_macros.push_back(current);
                }
                U_Horizontal_DFS(target, v_edge_list[node][i].to, v_edge_list, macro, overlapped_macros, visited);
            }
        }
    }
}

void B_Horizontal_DFS(int target, int node, vector<edge>* r_v_edge_list, vector<Macro*>& macro, vector<int>& overlapped_macros, vector<bool>& visited){
    for(int i = 0; i < r_v_edge_list[node].size(); i++){
        if(!visited[r_v_edge_list[node][i].from]){
            visited[r_v_edge_list[node][i].from] = true;
            int current = r_v_edge_list[node][i].from - 1;
            if(macro[current]->y1() + macro[current]->h() > macro[target]->y1() - buffer_constraint - powerplan_width){
                if(!(macro[current]->x1() > macro[target]->x1() + macro[target]->w() + buffer_constraint + powerplan_width ||
                    macro[current]->x1() + macro[current]->w() < macro[target]->x1() - buffer_constraint - powerplan_width)){
                    overlapped_macros.push_back(current);
                }
                B_Horizontal_DFS(target, r_v_edge_list[node][i].from, r_v_edge_list, macro, overlapped_macros, visited);
            }
        }
    }
}
void L_Horizontal_DFS(int target, int node, vector<edge>* r_h_edge_list, vector<Macro*>& macro, vector<int>& overlapped_macros, vector<bool>& visited){
    for(int i = 0; i < r_h_edge_list[node].size(); i++){
        if(!visited[r_h_edge_list[node][i].from]){
            visited[r_h_edge_list[node][i].from] = true;
            int current = r_h_edge_list[node][i].from - 1;
            if(macro[current]->x1() + macro[current]->w() > macro[target]->x1() - buffer_constraint - powerplan_width){
                if(!(macro[current]->y1() > macro[target]->y1() + macro[target]->h() + buffer_constraint + powerplan_width ||
                    macro[current]->y1() + macro[current]->h() < macro[target]->y1() - buffer_constraint - powerplan_width)){
                    overlapped_macros.push_back(current);
                }
                L_Horizontal_DFS(target, r_h_edge_list[node][i].from, r_h_edge_list, macro, overlapped_macros, visited);
            }
        }
    }
}
void R_Horizontal_DFS(int target, int node, vector<edge>* h_edge_list, vector<Macro*>& macro, vector<int>& overlapped_macros, vector<bool>& visited){
    for(int i = 0; i < h_edge_list[node].size(); i++){
        if(!visited[h_edge_list[node][i].to]){
            visited[h_edge_list[node][i].to] = true;
            int current = h_edge_list[node][i].to - 1;
            if(macro[current]->x1() < macro[target]->x1() + macro[target]->w() + buffer_constraint + powerplan_width){
                if(!(macro[current]->y1() > macro[target]->y1() + macro[target]->h() + buffer_constraint + powerplan_width ||
                    macro[current]->y1() + macro[current]->h() < macro[target]->y1() - buffer_constraint - powerplan_width)){
                    overlapped_macros.push_back(current);
                }
                R_Horizontal_DFS(target, h_edge_list[node][i].to, h_edge_list, macro, overlapped_macros, visited);
            }
        }
    }
}

class Mini_macro{
private:
    double _x, _y; // left bottom x, y
    double _w, _h;   // width, height
    string _name; // for debug, delete later
public:
    Mini_macro(int x, int y, int w, int h, string name){
        this->_x = x;
        this->_y = y;
        this->_w = w;
        this->_h = h;
        this->_name = name;
    }
    double x() const { return _x; }
    double y() const { return _y; }
    double w() const { return _w; }
    double h() const { return _h; }
    string name() const { return _name; } // for debug, delete later
    bool operator< (const Mini_macro& mini_macro) const{
        if(this->x() != mini_macro.x())
            return this->x() < mini_macro.x();
        else
            return this->y() < mini_macro.y();
    }
};

// return vector<int>
void Linear_Check_Sol(vector<Macro*>& macro, Graph& Gh, Graph& Gv){
    int n = macro.size();
    vector<edge>* h_edge_list = Gh.get_edge_list();   // Horizontal constraint graph
    vector<edge>* v_edge_list = Gv.get_edge_list();   // Vertical constraint graph
    vector<edge>* r_h_edge_list = Gh.get_reverse_edge_list(); // Reverse Horizontal constraint graph
    vector<edge>* r_v_edge_list = Gv.get_reverse_edge_list(); // Reverse vertical constraint graph
    vector<bool> visited(n + 2);
    vector<int> illegal_macros;
    for(int i = 0; i < n; i++){
        double Up_bound = macro[i]->y2() + buffer_constraint + powerplan_width, 
                Bottom_bound = macro[i]->y1() - buffer_constraint - powerplan_width,
                Left_bound = macro[i]->x1() - buffer_constraint - powerplan_width,
                Right_bound = macro[i]->x2() + buffer_constraint + powerplan_width;
        double region_width = Right_bound - Left_bound, region_height = Up_bound - Bottom_bound;
        if(Up_bound > chip_height) Up_bound = chip_height;
        if(Bottom_bound < 0) Bottom_bound = 0;
        if(Left_bound < 0) Left_bound = 0;
        if(Right_bound > chip_width) Right_bound = chip_width;
        vector<int> U_overlapped_macros;
        vector<int> B_overlapped_macros;
        vector<int> L_overlapped_macros;
        vector<int> R_overlapped_macros;
        // Calculate overlapped macros in U, B, L, R direction

        fill(visited.begin(), visited.end(), false);
        visited[0] = true;
        visited[visited.size() - 1] = true;
        U_Horizontal_DFS(i, macro[i]->id(), v_edge_list, macro, U_overlapped_macros, visited);
        fill(visited.begin(), visited.end(), false);
        visited[0] = true;
        visited[visited.size() - 1] = true;
        B_Horizontal_DFS(i, macro[i]->id(), r_v_edge_list, macro, B_overlapped_macros, visited);
        fill(visited.begin(), visited.end(), false);
        visited[0] = true;
        visited[visited.size() - 1] = true;
        L_Horizontal_DFS(i, macro[i]->id(), r_h_edge_list, macro, L_overlapped_macros, visited);
        fill(visited.begin(), visited.end(), false);
        visited[0] = true;
        visited[visited.size() - 1] = true;
        R_Horizontal_DFS(i, macro[i]->id(), h_edge_list, macro, R_overlapped_macros, visited);
        
        // add elements in overlapped_macors into a std::map
        set<Mini_macro> overlapped_macros;
        // macros overlapped at upper subregion
        for(int j = 0; j < U_overlapped_macros.size(); j++){
            //set x, y, w, h by information of macro[j]
            double x = macro[U_overlapped_macros[j]]->x1(), y = macro[U_overlapped_macros[j]]->y1();
            double w = macro[U_overlapped_macros[j]]->w(), h = macro[U_overlapped_macros[j]]->h();
            // up edge of macro higher than up boundary
            if(macro[U_overlapped_macros[j]]->y2() > Up_bound){
                h = Up_bound - macro[U_overlapped_macros[j]]->y1();
            }
            // left edge of macro smaller than left boundary
            if(macro[U_overlapped_macros[j]]->x1() < Left_bound){
                x = Left_bound;
                w = macro[U_overlapped_macros[j]]->x2() - Left_bound;
            }
            // right edge of macro exceed right boundary
            if(macro[U_overlapped_macros[j]]->x2() > Right_bound){
                w = Right_bound - macro[U_overlapped_macros[j]]->x1();
            }
            overlapped_macros.insert(Mini_macro(x, y, w, h, macro[U_overlapped_macros[j]]->name()));
        }
        // macros overlapped at bottom subregion
        for(int j = 0; j < B_overlapped_macros.size(); j++){
            //set x, y, w, h by information of macro[j]
            double x = macro[B_overlapped_macros[j]]->x1(), y = macro[B_overlapped_macros[j]]->y1();
            double w = macro[B_overlapped_macros[j]]->w(), h = macro[B_overlapped_macros[j]]->h();
            // bottom edge of macro lower than bottom boundary
            if(macro[B_overlapped_macros[j]]->y1() < Bottom_bound){
                y = Bottom_bound;
                h = macro[B_overlapped_macros[j]]->y2() - Bottom_bound;
            }
            // left edge of macro smaller than left boundary
            if(macro[B_overlapped_macros[j]]->x1() < Left_bound){
                x = Left_bound;
                w = macro[B_overlapped_macros[j]]->x2() - Left_bound;
            }
            // right edge of macro exceed right boundary
            if(macro[B_overlapped_macros[j]]->x2() > Right_bound){
                w = Right_bound - macro[B_overlapped_macros[j]]->x1();
            }            
            overlapped_macros.insert(Mini_macro(x, y, w, h, macro[B_overlapped_macros[j]]->name()));
        }
        // macros overlapped at left subregion
        for(int j = 0; j < L_overlapped_macros.size(); j++){
            //set x, y, w, h by information of macro[j]
            double x = macro[L_overlapped_macros[j]]->x1(), y = macro[L_overlapped_macros[j]]->y1();
            double w = macro[L_overlapped_macros[j]]->w(), h = macro[L_overlapped_macros[j]]->h();
            // up edge of macro higher than up boundary
            if(macro[L_overlapped_macros[j]]->y2() > Up_bound){
                h = Up_bound - macro[L_overlapped_macros[j]]->y1();         
            }
            // bottom edge of macro lower than bottom boundary
            if(macro[L_overlapped_macros[j]]->y1() < Bottom_bound){
                y = Bottom_bound;
                h = macro[L_overlapped_macros[j]]->y2() - Bottom_bound;
            }
            // left edge of macro smaller than left boundary
            if(macro[L_overlapped_macros[j]]->x1() < Left_bound){
                x = Left_bound;
                w = macro[L_overlapped_macros[j]]->x2() - Left_bound;
            }   
            overlapped_macros.insert(Mini_macro(x, y, w, h, macro[L_overlapped_macros[j]]->name()));
        }
        // macros overlapped at right subregion
        for(int j = 0; j < R_overlapped_macros.size(); j++){
            //set x, y, w, h by information of macro[j]
            double x = macro[R_overlapped_macros[j]]->x1(), y = macro[R_overlapped_macros[j]]->y1();
            double w = macro[R_overlapped_macros[j]]->w(), h = macro[R_overlapped_macros[j]]->h();
            // up edge of macro higher than up boundary
            if(macro[R_overlapped_macros[j]]->y2() > Up_bound){
                h = Up_bound - macro[R_overlapped_macros[j]]->y1();
            }
            // bottom edge of macro lower than bottom boundary
            if(macro[R_overlapped_macros[j]]->y1() < Bottom_bound){
                y = Bottom_bound;
                h = macro[R_overlapped_macros[j]]->y2() - Bottom_bound;
            }
            // right edge of macro exceed right boundary
            if(macro[R_overlapped_macros[j]]->x2() > Right_bound){
                w = Right_bound - macro[R_overlapped_macros[j]]->x1();
            }              
            overlapped_macros.insert(Mini_macro(x, y, w, h, macro[R_overlapped_macros[j]]->name()));
        }

        lprec *model;
        int over_num = overlapped_macros.size();
        if ((model = make_lp(0, (over_num + 1) * 2)) == NULL)
            // 0 constraints, (n + 1) * 2 variables
            ERROR();

        // Stop when find any solution
        set_break_at_first(model, TRUE);

        //set pi, qi to be binary variables
        for(int j = 3; j <= (over_num + 1) * 2; j++)
            set_binary(model, j, TRUE);

        set_upbo(model, 1, Right_bound);    // Wx's upper bound
        set_lowbo(model, 1, Left_bound);    // Wx's lower bound
        set_upbo(model, 2, Up_bound);       // Wy's upper bound
        set_lowbo(model, 2, Bottom_bound);  // Wy's lower bound

        int k = 3;
        double sparserow[3];
        int colno[3];
        for(auto mini_macros : overlapped_macros) {
            // pi = 0, qi = 0; Wx + W * pi + W * qi >= xi + wi
            sparserow[0] = 1;
            sparserow[1] = region_width;
            sparserow[2] = region_width;
            colno[0] = 1;
            colno[1] = k;
            colno[2] = k + 1;
            add_constraintex(model, 3, sparserow, colno, GE, mini_macros.x() + mini_macros.w());
            // pi = 0, qi = 1; Wy + H * pi - H * qi >= yi + hi - H
            sparserow[0] = 1;
            sparserow[1] = region_height;
            sparserow[2] = -region_height;
            colno[0] = 2;
            colno[1] = k;
            colno[2] = k + 1;
            add_constraintex(model, 3, sparserow, colno, GE, mini_macros.y() + mini_macros.h() - region_height);
            // pi = 1, qi = 0; Wx + W * pi - W * qi <= xi - powerplan_width + W
            sparserow[0] = 1;
            sparserow[1] = region_width;
            sparserow[2] = -region_width;
            colno[0] = 1;
            colno[1] = k;
            colno[2] = k + 1;
            add_constraintex(model, 3, sparserow, colno, LE, mini_macros.x() - powerplan_width + region_width);
            // pi = 1, qi = 1; Wy + H * pi + H * qi <= yi - powerplan_width + H
            sparserow[0] = 1;
            sparserow[1] = region_height;
            sparserow[2] = region_height;
            colno[0] = 2;
            colno[1] = k;
            colno[2] = k + 1;
            add_constraintex(model, 3, sparserow, colno, LE, mini_macros.y() - powerplan_width + region_height * 2);
            k += 2;
        }

        // Set objective function
        double* obj_fn_sparse = new double[2];
        int* obj_fn_colno = new int[2];
        obj_fn_sparse[0] = 1;
        obj_fn_sparse[1] = 1;
        obj_fn_colno[0] = 1;
        obj_fn_colno[1] = 2;
        set_obj_fnex(model, 2, obj_fn_sparse, obj_fn_colno);

        // Solve model
        int solution;
        solution = solve(model);
        if(solution == 0){
            cout << "==========" << macro[i]->name() << "==========" << endl;
            cout << "==================================" << endl;
            cout << "Successfully find optimal solution" << endl;
            cout << "==================================" << endl;
        }
        else if(solution == 1){
            cout << "==========" << macro[i]->name() << "==========" << endl;
            cout << "=====================================" << endl;
            cout << "Successfully find suboptimal solution" << endl;
            cout << "=====================================" << endl;            
        }
        else{
            cout << "======" << macro[i]->name() << "======" << endl;
            cout << "=========================" << endl;
            cout << "Failure find any solution" << endl;
            cout << "=========================" << endl;
            illegal_macros.push_back(i);
        }
        // Delete model
        delete_lp(model);
    }

    cout << "Illegal macros: " << endl;
    for(auto illegal_macro : illegal_macros){
        cout << macro[illegal_macro]->name() << endl;
    }
}