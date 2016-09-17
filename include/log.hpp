#include <boost/format.hpp>

#include <string>
#include <ostream>
#include <chrono>
#include <ctime>

enum class LogType
{
    error,
    warning,
    debug,
    info
};

using LT = LogType;

class Log
{
    std::ostream& out;
    boost::basic_format<char> fmt;
    bool newLine;

    public:
    Log(std::ostream& out, const std::string& formatString, bool newLine) : 
        out(out),
        fmt(formatString),
        newLine(newLine) {}

    template< typename T >
        Log& operator% (T arg)
        {
            fmt % arg;
            return *this;
        }

    ~Log()
    {
        auto timestamp = std::chrono::system_clock().to_time_t(std::chrono::system_clock().now());
        std::string timestampStr = std::ctime(&timestamp);
        //
        // remove newline
        //FIXME: Might not work on windows/dos
        timestampStr.erase(timestampStr.size()-1, 1); 

        out << timestampStr << ": ";

        out << fmt;

        if (newLine)
        {
            out << std::endl;
        }
        else
        {
            out.flush();
        }
    }
};

Log log(const std::string& formatString = "");

Log log(LogType type, const std::string& formatString);

// does not add a newline at the end
Log logPart(const std::string& formatString);
