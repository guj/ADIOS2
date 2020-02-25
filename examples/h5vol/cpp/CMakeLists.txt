#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(H5VolWriter 
     H5VolWriter.cpp 
     H5Vol.cpp
     main.cpp
  )
  target_include_directories(H5VolWriter
    PRIVATE ${HDF5_C_INCLUDE_DIRS}
  )

  target_link_libraries(H5VolWriter 
    ${HDF5_C_LIBRARIES} MPI::MPI_C ${CMAKE_THREAD_LIBS_INIT}
  )
endif()
target_link_libraries(H5VolWriter adios2)
