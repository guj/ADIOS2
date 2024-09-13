#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.12)
project(ADIOS2Opmd8c)

if(NOT TARGET adios2_core)
  set(_components CXX)

  find_package(ADIOS2 REQUIRED COMPONENTS ${_components})
endif()

add_executable(adios2_opmd_8c_mpi 8c.cpp)
target_link_libraries(adios2_opmd_8c_mpi adios2::cxx11_mpi MPI::MPI_CXX)
install(TARGETS adios2_opmd_8c_mpi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})



