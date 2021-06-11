#include <iostream>
#include <vector>
#include <fstream>
#include "macro.h"

using namespace std;

struct pt{
    int x;
    int y;
};


class IoData{
private:
    
public:
    int chip_x1, chip_y1, chip_x2, chip_y2;
    int powerplan_width_constraint;
    int minimum_spacing;
    int buffer_constraint;
    int weight_alpha;
    int weight_beta;
    int dbu_per_micron;

    vector<Macro> macros;
    vector<Macro> macro_shapes;

    //other info that need to put to output
    string version;
    string design;
    string dbu_per_micron_string;
    string die_area_string;

    int num_macro;
    int die_x, die_y;
    int die_width, die_height;

    void parseDef(ifstream& f);

    void parseLef(ifstream& f);

    void parseTxt(ifstream& f);

    void output(string file);

    void processDiearea(vector<pt> points);

    IoData(){}

};

