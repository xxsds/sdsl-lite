#ifndef INCLUDED_SDSL_TEST_COMMON
#define INCLUDED_SDSL_TEST_COMMON

#include "sdsl/io.hpp"
#include <string>

int init_2_arg_test(int argc,
                   char** argv,
                   std::string index_type,
                   std::string& test_file,
                   std::string& temp_dir,
                   std::string& temp_file) {
    if (argc < 3) {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " test_file tmp_dir\n\
        (1) Generates an index out of test_file; stores it in a temporary file.\n\
        (2) Performs tests.\n\
        (3) Deletes the temporary file." << std::endl;
        std::cout<<"Only "<<argc<<" parameters provided" << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    test_file    = argv[1];
    temp_dir     = argv[2];
    temp_file    = sdsl::tmp_file(temp_dir+"/"+index_type,index_type);
    return 0;
}

int init_3_arg_test(int argc,
                   char** argv,
                   std::string index_type,
                   std::string& test_file,
                   uint8_t& num_bytes,
                   std::string& temp_dir,
                   std::string& temp_file) {
    if (argc < 4) {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " test_file num_bytes tmp_dir\n\
        (1) Generates an index out of test_file interpreted as a sequence of\n\
            symbols each consisting of num_bytes bytes;\n\
            stores it in a temporary file.\n\
        (2) Performs tests.\n\
        (3) Deletes the temporary file." << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    test_file    = argv[1];
    num_bytes    = atoi(argv[2]);
    temp_dir     = argv[3];
    temp_file    = sdsl::tmp_file(temp_dir+"/"+index_type+"_"+argv[2],index_type);
    return 0;
}


#endif
