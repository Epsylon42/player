#include <boost/format.hpp>

#include <string>
#include <ostream>

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

// does not add a newline at the end
Log logPart(const std::string& formatString);
