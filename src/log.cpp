#include "log.hpp"

#include <fstream>

using namespace std;

string ltToString(LogType type)
{
    switch (type)
    {
        case LogType::error:
            return "ERROR: ";

        case LogType::warning:
            return "WARNING: ";

        case LogType::debug:
            return "DEBUG: ";

        case LogType::info:
            return "INFO: ";
    }
}

ofstream fout("player.log");

Log log(const string& formatString)
{
    return Log(fout, formatString, true);
}

Log log(LogType type, const string& formatString)
{
    return Log(fout, ltToString(type) + formatString, true);
}

Log logPart(const string& formatString)
{
    return Log(fout, formatString, false);
}
