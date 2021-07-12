#include <fstream>
#include <iostream>
#include <string.h>
#include <vector>
//#include "macro.h"

#include "io.h"

#include <limits>

typedef std::numeric_limits< double > dbl;

using namespace std;

IoData *iodata;

bool hasEnding(std::string fullString, std::string ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

bool hasBegining(std::string fullString, std::string begining)
{
    if (fullString.length() >= begining.length())
    {
        return (0 == fullString.compare(0, begining.length(), begining));
    }
    else
    {
        return false;
    }
}

void output(){
    ofstream f(iodata->output_filename.c_str());
    if (!f.good())
    {
        cerr << "Unable to output file";
    }

    f << iodata->version;
    f << iodata->design;
    f << iodata->dbu_per_micron_string;
    f << iodata->die_area_string;

    f << "\nCOMPONENTS " << iodata->num_macro << " ;\n";

    for (int i = 0; i < iodata->macros.size(); i++)
    {
        //f<<"  **** "<<iodata->macros[i].name()<<" "<<iodata->macros[i].w()<<" "<<iodata->macros[i].h()<<" "<<this->macros[i].x1()<<" "<<this->macros[i].x2()<<"\n";
        if (iodata->macros[i]->type() == border)
        {
            continue;
        }
        f << "   - " << iodata->macros[i]->name() << " " << iodata->macros[i]->shape() << " \n"
          << "      + ";
        if (iodata->macros[i]->is_fixed())
        {
            f << "FIXED ( ";
        }
        else
        {
            f << "PLACED ( ";
        }
        f.precision(dbl::max_digits10);
        //f << to_string(solution[i].first-iodata->macros[i]->w()/2) << " " << to_string(solution[i].second-iodata->macros[i]->h()/2) << " ) N ;\n";
        //f << to_string(iodata->macros[i]->x1()) << " " << to_string(iodata->macros[i]->y1()) << " ) N ;\n";
        f << iodata->macros[i]->x1() << " " << iodata->macros[i]->y1() << " ) N ;\n";

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



IoData *shoatingMain(int argc, char *argv[])
{
    ifstream fs;
    string arg[argc];
    //string output_filename;
    iodata = new IoData();

    for (int i = 0; i < argc; i++)
    {
        //cout<<argv[i]<<endl;
    }

    //get argv and call parse
    for (int i = 1; i < argc; i++)
    {
        // cout << argv[i] << endl;
        //cout<<"fuck 1 \n";
        arg[i] = argv[i];
        if (hasBegining(arg[i], "out"))
        {
            iodata->output_filename = arg[i];
            continue;
        }

        fs.open(argv[i]);
        if (!fs.good())
        {
            cerr << "Unable to open " << argv[i] << std::endl;
        }

        if (hasEnding(arg[i], "lef"))
        {
            iodata->parseLef(fs);
        }
        else if (hasEnding(arg[i], "def"))
        {
            iodata->parseDef(fs);
        }
        else if (hasEnding(arg[i], "txt"))
        {
            iodata->parseTxt(fs);
        }
        else
        {
            cerr << "File type unavailable: " << argv[i] << endl;
        }

        fs.close();
    }

    for (int i = 0; i < iodata->macros.size(); i++)
    {
        if (iodata->macros[i]->type() == border)
        {
            continue;
        }
        //cout<<"why "<<i<<endl;

        for (int j = 0; j < iodata->macro_shapes.size(); j++)
        {
            if (iodata->macros[i]->shape() == iodata->macro_shapes[j]->name())
            {
                iodata->macros[i]->setWidthHeight(*(iodata->macro_shapes[j]), iodata->dbu_per_micron);
                break;
            }
        }
    }

    iodata->powerplan_width_constraint *= iodata->dbu_per_micron;
    iodata->minimum_spacing *= iodata->dbu_per_micron;
    iodata->buffer_constraint *= iodata->dbu_per_micron;

    //iodata->output(output_filename);
    // cout << "tim end" << endl;

    return iodata;
}

//fs.close();
/*
        if (arg == "-lef" && i < argc){
            ifstream fs(string(argv[i]));
            if (!fs.good()){
                std::cerr << "Unable to open " << argv[i] << std::endl;
            }
        }*/