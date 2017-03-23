prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${prefix}/lib
includedir=${prefix}/include

Name: @PROJECT_NAME@
Description: @PROJECT_DESCRIPTION@
Version: @LIBRARY_VERSION_FULL@
URL: @PROJECT_URL@
Libs: -L${libdir} -ldivsufsort -ldivsufsort64
Cflags: -I${includedir}
