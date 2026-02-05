add_library(libear INTERFACE)
target_include_directories(libear INTERFACE "include/")
target_compile_options(libear INTERFACE "-w")

target_link_libraries(libear INTERFACE
        libear_impl
        Boost::optional
        Boost::variant
        Boost::math
        Boost::algorithm
)