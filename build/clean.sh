#!/bin/bash
# activate globbing
shopt -s extglob


# remove the cmake generated definitions
rm -f ../include/sdsl/definitions.hpp

# removes all but the listed files in the directory
# and subdirectories
rm -rf !(build.sh|clean.sh|.gitignore|..|.)  

