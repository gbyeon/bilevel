if(CPLEXLIB)
    add_test(NAME cpx_bmilplib_10_1_nontest COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_1)
    add_test(NAME cpx_bmilplib_10_1 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_1 -test -366.167)
    add_test(NAME cpx_bmilplib_10_2 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_2 -test -229)
    add_test(NAME cpx_bmilplib_10_3 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_3 -test -444)
    add_test(NAME cpx_bmilplib_10_4 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_4 -test -250)
    add_test(NAME cpx_bmilplib_10_5 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_5 -test -263.6)
    add_test(NAME cpx_bmilplib_10_6 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_6 -test -67)
    add_test(NAME cpx_bmilplib_10_7 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_7 -test -224)
    add_test(NAME cpx_bmilplib_10_8 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_8 -test -66.5)
    add_test(NAME cpx_bmilplib_10_9 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_9 -test -75)
    add_test(NAME cpx_bmilplib_10_10 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_10_10 -test -444.333)
    add_test(NAME cpx_bmilplib_460_10 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuConverted/bmilplib_460_10 -test -109.996)
    add_test(NAME cpx_xuLarge500-1 COMMAND $ENV{PWD}/src/runbilevel -f ${CMAKE_SOURCE_DIR}/instances/xuLarge/xuLarge500-1 -test -96)
endif(CPLEXLIB)