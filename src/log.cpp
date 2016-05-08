#include "log.hpp"

#include <fstream>

using namespace std;

ofstream fout("player.log");


Log log(const string& formatString)
{
    return Log(fout, formatString, true);
}

Log logPart(const string& formatString)
{
    return Log(fout, formatString, false);
}
