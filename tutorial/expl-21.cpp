#include <iostream>

#include <sdsl/cst_sct3.hpp>
#include <sdsl/suffix_array_algorithm.hpp>
#include <sdsl/suffix_tree_algorithm.hpp>

using namespace sdsl;
using namespace std;

int main()
{
    cst_sct3<> cst;
    construct_im(cst, "umulmundumulmum", 1);
    cout << "inner nodes : " << cst.nodes() - cst.csa.size() << endl;
    auto v = cst.select_child(cst.child(cst.root(), 'u'), 1);
    auto d = cst.depth(v);
    cout << "v : " << d << "-[" << cst.lb(v) << "," << cst.rb(v) << "]" << endl;
    cout << "extract(cst, v) : " << extract(cst, v) << endl;
}
