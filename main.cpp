#include <iostream>
#include <limits>
#include <iomanip>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <windows.h>
#include "FuncInfo.hpp"
#include "invoker.hpp"

using namespace std;
using namespace std::chrono;

inline void pause()
{
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

vector<FuncInfo*> funcs;
enum class Cmds;
void createinvokerfile();
void printfuncs();

int main(int argc, char** argv)
{
    cout << "WARNING:\nThis program uses the system() API. In order to prevent security risks, please make sure of \n"
            "the following before using this program:\n\n1. You have a valid installation of a GNU compiler.\n2. You "
            "can invoke \"g++.exe\" from the command line directly.\n3. The windows \"copy\" command has not been "
            "compromised by malware.\n\nNeglecting these checks could cause this program to not function correctly, "
            "or in the worst\ncase scenario damage your system.\n\n\n\n\n";

    string pname = argv[0];

    if (pname[pname.find('.') - 1] == '_')
    {
        system("copy tempfiles\\selfrecomp_.exe selfrecomp.exe");
        STARTUPINFO si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        CreateProcess(0, "selfrecomp.exe", 0, 0, FALSE, CREATE_NEW_CONSOLE, 0, 0, &si, &pi);
        return 0;
    }

    cout << "Press enter to begin compiling user code...\n\n";
    pause();

    int errcode = system("g++.exe -std=c++14 -Wall -O2 -s -c userfuncs.cpp -o tempfiles/userfuncs.o");

    if (errcode > 0)
    {
        cout << "\n\nThere was an error during compilation.";
        pause();
        return 1;
    }

    cout << "\n\n...done.\n\n\nPress enter to generate invoker.hpp...\n\n";
    pause();

    createinvokerfile();

    cout << "...done.\n\n\nPress enter to recompile this program...\n\n";
    pause();

    system("g++.exe -std=c++14 -O2 -Wall -Wno-write-strings -Wno-sign-compare -s -c main.cpp -o tempfiles/main.o");
    system("g++.exe -std=c++14 -O2 -Wall -s -c invoker.cpp -o tempfiles/invoker.o");
    system("g++.exe -std=c++14 -O2 -Wall -s tempfiles/main.o tempfiles/invoker.o tempfiles/userfuncs.o -o tempfiles/selfrecomp_.exe");

    cout << "\n\n...done.";
    pause();

    STARTUPINFO si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    CreateProcess(0, "tempfiles/selfrecomp_.exe", 0, 0, FALSE, CREATE_NEW_CONSOLE, 0, 0, &si, &pi);
}



void createinvokerfile()
{
    ///First, delete any previous function information
    for (auto& f : funcs)
        delete f;

    funcs.clear();



    ///Then parse user code
    ifstream usercode("userfuncs.hpp");
    vector<string> userlines;
    string linebuf;

    while (usercode.good())
    {
        getline(usercode, linebuf);
        userlines.push_back(linebuf);
    }

    usercode.close();

    bool beginparse = false;

    for (string& line : userlines)
    {
        if (line == "//{{")
            beginparse = true;

        else if (line == "//}}")
            beginparse = false;

        else if (beginparse && line.length() > 0)
        {
            vector<string> argtypes;
            string funcname, buf;

            //Parse function name
            size_t curpos = line.find(' ');

            for (size_t i = curpos + 1; i < line.length(); i++)
            {
                if (line[i] != '(')
                    buf += line[i];

                else
                    break;
            }

            funcname = buf;
            buf = "";

            //parse param types and number of params
            curpos = line.find('(', curpos);

            if (line[curpos+1] == ')')
            {
                // No parameters case
                argtypes.shrink_to_fit();

                funcs.push_back(new FuncInfo{funcname, argtypes, 0});
            }

            else
            {
                // 1/more parameters case
                bool stopparse = false;

                for (size_t i = curpos + 1; i < line.length(); i++)
                {
                    if (line[i] == ')')
                        break;

                    //another param exists, begin parsing type again
                    if (line[i] == ',')
                    {
                        argtypes.push_back(buf);
                        buf = "";
                        stopparse = false;
                    }

                    //ignore space after a comma
                    else if (line[i-1] == ',' && line[i] == ' ')
                        continue;

                    //now iterating over param name (or possibly not), done parsing type
                    else if (line[i] == ' ')
                        stopparse = true;

                    else if (!stopparse)
                        buf += line[i];
                }

                argtypes.push_back(buf);

                if (argtypes.at(0) == "void")
                    argtypes.clear();

                argtypes.shrink_to_fit();

                funcs.push_back(new FuncInfo{funcname, argtypes, argtypes.size()});

                /**
                    PRECONCEIVED BUGS:
                    The parsing algo will break under the following cases (which i'm choosing to
                    ignore for now for simplicity):
                    -- the return type has a space in it (eg. pair<int, string>)
                    -- if there's an inital space in the parameter list (eg. void func( int) )
                    -- if there's two spaces after a comma (eg. void func(int,  int) )
                **/
            }
        }
    }


    ///Finally, generate invoker.hpp using parsed function information
    ofstream invoker("invoker.hpp", ios_base::trunc);

    invoker << "#pragma once\n"
               "#include <string>\n"
               "#include <sstream>\n"
               "#include \"userfuncs.hpp\"\n\n"
               "struct noreturn_t{};\n"
               "extern const noreturn_t noreturn;\n\n"
               "template <class T>\n"
               "T strast(const std::string& str) {\n"
               "    std::stringstream ss(str);\n"
               "    T t{};\n"
               "    ss >> t;\n"
               "    return t;\n"
               "}\n\n"
               "template <class... Args>\n"
               "decltype(auto) invoke(const std::string& fstr, Args... args) {\n"
               "    std::string agg[] {args...};\n\n";

    if (funcs.size() == 0)
        invoker << "    return noreturn;\n}\n";

    else
    {
        FuncInfo* curfunc = funcs.at(0);

        invoker << "    if (fstr == \"" << curfunc->funcname << "\")\n"
                << "        return " << curfunc->funcname << '(';

        for (int i = 0; i < curfunc->numargs; i++)
        {
            if (i == curfunc->numargs - 1)
                invoker << "strast<" << curfunc->argtypes.at(i) << ">(agg[" << i << "]));\n";

            else
                invoker << "strast<" << curfunc->argtypes.at(i) << ">(agg[" << i << "]), ";
        }

        invoker << ");\n\n";



        for (int i = 1; i < funcs.size(); i++)
        {
            curfunc = funcs.at(i);

            invoker << "    else if (fstr == \"" << curfunc->funcname << "\")\n"
                    << "        return " << curfunc->funcname << '(';

            for (int i = 0; i < curfunc->numargs; i++)
            {
                if (i == curfunc->numargs - 1)
                    invoker << "strast<" << curfunc->argtypes.at(i) << ">(agg[" << i << "])";

                else
                    invoker << "strast<" << curfunc->argtypes.at(i) << ">(agg[" << i << "]), ";
            }

            invoker << ");\n\n";
        }



        invoker << "    else\n        return noreturn;\n}\n";
    }

    invoker.close();
}



void printfuncs()
{
    cout << "Currently compiled functions:\n\n";

    for (auto& f : funcs)
    {
        cout << setw(23) << "Name:  " << f->funcname << endl
             << setw(23) << "Number of parameters:  " << f->numargs << endl
             << setw(23) << "Parameter list:  ";

        for (int i = 0; i < f->argtypes.size(); i++)
        {
            if (i + 1 == f->argtypes.size())
                cout << f->argtypes.at(i) << endl;

            else
                cout << f->argtypes.at(i) << ", ";
        }

        cout << "\n\n";
    }
}
































