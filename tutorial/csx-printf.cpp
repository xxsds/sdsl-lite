#include <iostream>
#include <regex>
#include <sstream>

#include <sdsl/csa_bitcompressed.hpp>
#include <sdsl/cst_sct3.hpp>

using namespace sdsl;
using namespace std;

std::string format("%3I%3S %3s %3P %3p %3L %3B  %T");
std::string header("  i SA ISA PSI  LF LCP BWT  TEXT");
static const std::streamsize BUF_SIZE = 4096;
char line[BUF_SIZE];

typedef csa_bitcompressed<int_alphabet<>> csa_int_t;
typedef cst_sct3<> cst_byte_t;
typedef cst_sct3<csa_int_t> cst_int_t;

void print_usage(char const * command)
{
    cout << "\
A pretty printer for suffix array/tree members.\n\
Transforms each input line into a CST and outputs\n\
formatted suffix array/tree members.\n\
Usage: " << command
         << " X \"[FORMAT]\" \"[HEADER]\" \"[SENTINEL]\"\n\
X       : Input is interpreted dependent on X.\n\
         X=1: byte sequence.\n\
         X=d: sequence of decimal numbers.\n\
FORMAT  : Format string. Default=`"
         << format << "`.\n\
HEADER  : Header string. Default=`"
         << header << "`.\n\
SENTINEL: Sentinel character. \n\
\n\
Each line of the output will be formatted according to the format string.\
All content, except tokens which start with `%` will be copied. Tokens\
which start with `%` will be replaced as follows (let w be a positive\
number. setw(w) is used to format single numbers):\
\n\
Token      |  Replacement | Comment\n\
-----------------------------------------------------------------------\n\
 %[w]I     | Row index i.                           |                  \n\
 %[w]S     | SA[i]                                  |                  \n\
 %[w]s     | ISA[i]                                 |                  \n\
 %[w]P     | PSI[i]                                 |                  \n\
 %[w]p     | LF[i]                                  |                  \n\
 %[w]L     | LCP[i]                                 | only for CSTs    \n\
 %[w]B     | BWT[i]                                 |                  \n\
 %[w[:W]]T | Print min(idx.size(),w) chars of each  |                  \n\
           | suffix, each char formatted by setw(W).|                  \n\
 %%        | %                                      |                  \n";
}

int main(int argc, char * argv[])
{
    if (argc < 2 or !('1' == argv[1][0] or 'd' == argv[1][0]))
    {
        print_usage(argv[0]);
        return 1;
    }
    if (argc > 2)
    {
        format = argv[2];
    }
    if (argc > 3)
    {
        header = argv[3];
    }
    while (cin.getline(line, BUF_SIZE))
    {
        cout << header << endl;
        if ('1' == argv[1][0])
        {
            cst_byte_t cst;
            construct_im(cst, (char const *)line, 1);
            stringstream ss;
            csXprintf(ss, format, cst, ((argc > 4) ? argv[4][0] : '$'));
            std::string line(ss.str());
            cout << std::regex_replace(line, std::regex(R"(\$)"), "\\$") << endl;
        }
        else if ('d' == argv[1][0])
        {
            cst_int_t cst;
            construct_im(cst, (char const *)line, 'd');
            csXprintf(cout, format, cst, ((argc > 4) ? argv[4][0] : '0'));
        }
        cout << endl;
    }
}
