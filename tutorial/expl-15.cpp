#include <iostream>
#include <utility>

#include <sdsl/construct.hpp>
#include <sdsl/rrr_vector.hpp>
#include <sdsl/wt_int.hpp>

using namespace std;
using namespace sdsl;

template <class value_type, class size_type>
ostream & operator<<(ostream & os, std::pair<value_type, size_type> const & p)
{
    return os << "(" << p.first << "," << p.second << ")";
}

int main()
{
    wt_int<rrr_vector<63>> wt;
    construct_im(wt, "6   1000 1 4 7 3   18 6 3", 'd');
    auto res = wt.range_search_2d(1, 5, 4, 18);
    for (auto point : res.second)
        cout << point << " ";
    cout << endl;
}
