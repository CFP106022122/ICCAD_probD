#include "io.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <vector>
//#include "macro.h"

using namespace std;

void IoData::processDiearea(vector<pt> points)
{
    int min_x, min_y, max_x, max_y;
    Macro *m;
    if (points.size() == 2)
    {
        this->die_x = points[0].x;
        this->die_y = points[0].y;
        max_x = points[0].x;
        max_y = points[0].y;
        if (points[0].y > points[1].y)
        {
            this->die_y = points[1].y;
        }
        else
        {
            max_y = points[1].y;
        }
        if (points[0].x > points[1].x)
        {
            this->die_x = points[1].x;
        }
        else
        {
            max_x = points[1].x;
        }
        this->die_height = max_y - this->die_y;
        this->die_width = max_x - this->die_x;
    }
    else
    {
        min_x = 2147483647;
        min_y = 2147483647;
        max_x = -2147483647;
        max_y = -2147483647;

        int j;
        for (int i = 0; i < points.size(); i++)
        {
            if (points[i].x > max_x)
            {
                max_x = points[i].x;
            }
            else if (points[i].x < min_x)
            {
                min_x = points[i].x;
            }

            if (points[i].y > max_y)
            {
                max_y = points[i].y;
            }
            else if (points[i].y < min_y)
            {
                min_y = points[i].y;
                j = i;
            }
        }
        this->die_x = min_x;
        this->die_y = min_y;
        this->die_width = max_x - min_x;
        this->die_height = max_y - min_y;

        int left_right = -1;
        bool down_side = true;
        for (int i = 0; i < points.size(); i++, j++)
        {
            if ((points[j % points.size()].y != min_y) && (points[j % points.size()].y != max_y) && (points[(j - 1) % points.size()].y == points[j % points.size()].y))
            {
                //cout<<(j-1)%points.size()<<" "<<points[j%points.size()].y<<" "<<points[(j-1)%points.size()].y<<endl;
                if (down_side)
                {
                    m = new Macro(abs(points[(j - 1) % points.size()].x - points[j % points.size()].x),
                                  abs(points[(j) % points.size()].y - min_y),
                                  points[(j) % points.size()].x - this->die_x, 0, true, -1 * i);
                    this->macros.push_back(m);
                }
                else
                {
                    m = new Macro(abs(points[(j - 1) % points.size()].x - points[j % points.size()].x),
                                  abs(points[(j) % points.size()].y - max_y),
                                  points[(j) % points.size()].x - this->die_x, points[(j) % points.size()].y - this->die_y, true, -1 * i);
                    this->macros.push_back(m);
                }
                //(double w, double h, double _x, double _y, bool is_f, int i)
            }
            if ((points[j % points.size()].x == min_x) || (points[j % points.size()].x == max_x))
            {
                if (left_right == -1)
                {
                    left_right = (points[j % points.size()].x == min_x) ? 1 : 0;
                }
                if ((left_right == 1) != (points[j % points.size()].x == min_x))
                {
                    down_side = !down_side;
                    left_right = (left_right + 1) % 2;
                }
            }
        }
    }
}

void IoData::parseDef(ifstream &f)
{
    string linebuff, buff, buff1, buff2, buff3, buff4, buff5, buff6, buff7;
    int intbuff;
    bool isfixed;
    Macro *m;

    stringstream linestring;

    //--parse front part
    {
        //--version
        getline(f, linebuff);
        this->version = linebuff + "\n";
        //cout<<this->version;
        //--design
        getline(f, linebuff);
        this->design = linebuff + "\n";
        //cout<<this->design;
        //--dbu per micron
        getline(f, linebuff);
        this->dbu_per_micron_string = linebuff + "\n";
        linestring.str(linebuff);
        linestring >> buff >> buff >> buff >> intbuff;
        this->dbu_per_micron = intbuff;
        linestring.clear();
        //cout<<this->dbu_per_micron<<endl;

        //--die area
        vector<pt> points;
        getline(f, linebuff, ';');
        this->die_area_string = linebuff + ";" + "\n";
        //cout<<this->die_area_string;
        linestring.str(linebuff);
        linestring >> buff >> buff;
        while (!linestring.eof())
        {
            while (buff != "(")
            {
                linestring >> buff;
            }
            linestring >> buff1 >> buff2 >> buff3;
            buff = "";
            linestring >> buff;
            //cout<<buff1<<" "<<buff2<<endl;
            pt point;
            point.x = stoi(buff1);
            point.y = stoi(buff2);
            points.push_back(point);
        }
        processDiearea(points);
        linestring.clear();
    }

    //--parse macros
    getline(f, linebuff, ';');
    linestring.str(linebuff);
    linestring >> buff >> intbuff;
    this->num_macro = intbuff;
    linestring.clear();
    //cout<<this->num_macro<<endl;
    for (int i = 0; i < this->num_macro; i++)
    {
        getline(f, linebuff, ';');
        linestring.str(linebuff);
        linestring >> buff >> buff1 >> buff2;                  //                 - A1/m1 c5
        linestring >> buff >> buff3 >> buff >> buff4 >> buff5; //     + FIXED ( 0 20000 ) N ;
        //cout<<buff1<<" "<<buff2<<" "<<buff3<<" "<<buff4<<" "<<buff5<<" "<<endl;
        if (buff3 == "FIXED")
        {
            intbuff = _fixed;
            isfixed = true;
        }
        else if (buff3 == "PLACED")
        {
            intbuff = placed;
            isfixed = false;
        }
        else
        {
            cerr << "Undefined macro movable type: " << buff3;
        }

        //(string name, string shape, int type, double _x, double _y, bool is_f, int i)
        m = new Macro(buff1, buff2, intbuff, stod(buff4.c_str()) - this->die_x, stod(buff5.c_str()) - this->die_x, isfixed, i);
        this->macros.push_back(m);

        linestring.clear();
    }

    /*for(int i=0;i<this->macros.size();i++){
        cout<<"  **** "<<this->macros[i]->name()<<" "<<this->macros[i]->shape()<<" "<<this->macros[i]->type()<<" "<<this->macros[i]->x1()<<" "<<this->macros[i]->x2()<<"\n";
    }*/
    //--------------------------
    // def sample
    //
    // VERSION 5.7 ;
    // DESIGN sample_case ;
    // UNITS DISTANCE MICRONS 1000 ;
    //
    // DIEAREA ( 50000 0 ) ( 50000 20000 ) ( 0 20000 ) ( 0 300000 ) ( 200000 300000 )
    //         ( 200000 250000 ) ( 400000 250000 ) ( 400000 0 ) ;
    //
    // COMPONENTS 11 ;
    // - A1/m1 c5
    //     + FIXED ( 0 20000 ) N ;
    // - A2/m1 c4
    //     + FIXED ( 275000 100000 ) N ;
    // .
    // .
    // .
    // END COMPONENTS
    //
    //
    // END DESIGN
    //--------------------------
}

void IoData::parseLef(ifstream &f)
{
    string linebuff, buff, buff1, buff2, buff3, buff4, buff5, buff6, buff7;
    int intbuff;
    bool isfixed;
    Macro *m;
    //cout<<"lef fuck" ;

    stringstream linestring;

    while (!f.eof())
    {
        getline(f, linebuff);
        if (linebuff == "END LIBRARY")
        {
            break;
        }
        linestring.str(linebuff);
        linestring >> buff >> buff1;
        linestring.clear();

        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> buff2 >> buff >> buff3;
        linestring.clear();
        //cout<<buff1<<" "<<buff2<<" "<<buff3<<endl;

        m = new Macro(buff1, stod(buff2), stod(buff3));
        this->macro_shapes.push_back(m);

        getline(f, linebuff);
        getline(f, linebuff);
    }

    for (int i = 0; i < this->macros.size(); i++)
    {
        if (this->macros[i]->type() == border)
        {
            continue;
        }
        //cout<<"why "<<i<<endl;

        for (int j = 0; j < this->macro_shapes.size(); j++)
        {
            if (this->macros[i]->shape() == this->macro_shapes[j]->name())
            {
                this->macros[i]->setWidthHeight(*(this->macro_shapes[j]));
                break;
            }
        }
    }
}

void IoData::output(string file)
{
    ofstream f(file.c_str());
    if (!f.good())
    {
        cerr << "Unable to output file";
    }

    f << this->version;
    f << this->design;
    f << this->dbu_per_micron_string;
    f << this->die_area_string;

    f << "\nCOMPONENTS " << this->num_macro << " ;\n";

    for (int i = 0; i < this->macros.size(); i++)
    {
        //f<<"  **** "<<this->macros[i].name()<<" "<<this->macros[i].w()<<" "<<this->macros[i].h()<<" "<<this->macros[i].x1()<<" "<<this->macros[i].x2()<<"\n";
        if (this->macros[i]->type() == border)
        {
            continue;
        }
        f << "   - " << this->macros[i]->name() << " " << this->macros[i]->shape() << " \n"
          << "      + ";
        if (this->macros[i]->is_fixed())
        {
            f << "FIXED ( ";
        }
        else
        {
            f << "PLACED ( ";
        }
        f << to_string(this->macros[i]->x1()) << " " << to_string(this->macros[i]->y1()) << " ) N ;\n";

        //f<<"   - "<<this->macros[i].shape()<<" "<<this->macros[i].w()<<" "<<this->macros[i].h()<<"\n";
    }

    f << "END COMPONENTS\n\n\nEND DESIGN\n\n\n";
    /*
    for(int i=0;i<this->macro_shapes.size();i++){
        f<<"   - "<<this->macro_shapes[i].name()<<" "<<this->macro_shapes[i].w()<<" "<<this->macro_shapes[i].h()<<"\n";
    }
*/
    f.close();
}

//--------------
//five argument in txt
void IoData::parseTxt(ifstream &f)
{
    string linebuff, buff, buff1, buff2, buff3, buff4, buff5, buff6, buff7;
    int intbuff;

    stringstream linestring;

    {
        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> intbuff;
        this->powerplan_width_constraint = intbuff;
        linestring.clear();

        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> intbuff;
        this->minimum_spacing = intbuff;
        linestring.clear();

        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> intbuff;
        this->buffer_constraint = intbuff;
        linestring.clear();

        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> intbuff;
        this->weight_alpha = intbuff;
        linestring.clear();

        getline(f, linebuff);
        linestring.str(linebuff);
        linestring >> buff >> intbuff;
        this->weight_beta = intbuff;
        linestring.clear();

        //cout<<this->powerplan_width_constraint<<" "<<this->minimum_spacing<<" "<<this->buffer_constraint<<" ";
        //cout<<this->weight_alpha<<" "<<this->weight_beta<<endl;
    }
}