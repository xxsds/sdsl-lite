#include <iostream>

#include <sdsl/csa_bitcompressed.hpp>
#include <sdsl/suffix_array_algorithm.hpp>

using namespace std;
using namespace sdsl;

int main()
{
    csa_bitcompressed<> csa;
    construct_im(csa, "abracadabra", 1);
    cout << "csa.size(): " << csa.size() << endl;
    cout << "csa.sigma : " << csa.sigma << endl;
    cout << csa << endl;
    cout << extract(csa, 0, csa.size() - 1) << endl;
}
