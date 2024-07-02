file(GLOB_RECURSE HDR_FILES *.hpp)
file(GLOB_RECURSE SRC_FILES *.cpp)

add_library(rgnb ${HDR_FILES} ${SRC_FILES})

target_compile_options(rgnb PRIVATE -Wall -Wextra -pedantic -Wno-unused-parameter)

target_link_libraries(rgnb asn-ngap)
target_link_libraries(rgnb asn-rrc)
target_link_libraries(rgnb common-lib)

