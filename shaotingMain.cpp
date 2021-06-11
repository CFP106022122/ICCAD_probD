#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
//#include "macro.h"
#include "io.h"

using namespace std;

bool hasEnding (std::string fullString, std::string ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool hasBegining (std::string fullString, std::string begining) {
    if (fullString.length() >= begining.length()) {
        return (0 == fullString.compare (0, begining.length(), begining));
    } else {
        return false;
    }
}


int main(int argc, char **argv){
    ifstream fs;
    string arg[argc];
    string output_filename;
    IoData* iodata = new IoData();

    //get argv and call parse
    for( int i = 1; i < argc; i++) {
        //cout<<argv[i]<<endl;
        arg[i] = argv[i];
        if(hasBegining(arg[i], "out")){
            output_filename = arg[i];
            continue;
        }

        fs.open(argv[i]);
        if (!fs.good()){
            cerr << "Unable to open " << argv[i] << std::endl;
        }
        
        if(hasEnding(arg[i], "lef")){
            iodata->parseLef(fs);
        }else if(hasEnding(arg[i], "def")){
            iodata->parseDef(fs);
        }else if(hasEnding(arg[i], "txt")){
            iodata->parseTxt(fs);
        }else{
            cerr<<"File type unavailable: "<<argv[i]<<endl;
        }

        fs.close();


       
    }

    iodata->output(output_filename);

    return 0;

}





 //fs.close();
        /*
        if (arg == "-lef" && i < argc){
            ifstream fs(string(argv[i]));
            if (!fs.good()){
                std::cerr << "Unable to open " << argv[i] << std::endl;
            }
        }*/