#include <fstream>
#include <iostream>
#include <string.h>
#include <vector>
//#include "macro.h"

#include "io.h"

using namespace std;

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

IoData *shoatingMain(int argc, char *argv[])
{
    ifstream fs;
    string arg[argc];
    //string output_filename;
    IoData *iodata = new IoData();

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