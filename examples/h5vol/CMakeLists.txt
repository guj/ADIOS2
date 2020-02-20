#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

if(ADIOS2_HAVE_MPI)
  add_executable(H5VolReadWriteC
     H5VolReadWrite.c
     H5Vol.c
     main.c
  )
  target_include_directories(H5VolReadWriteC
    PRIVATE ${HDF5_C_INCLUDE_DIRS}
  )

  target_link_libraries(H5VolReadWriteC 
    ${HDF5_C_LIBRARIES} MPI::MPI_C ${CMAKE_THREAD_LIBS_INIT}
  )

  set(VOLSRC
      H5VolReadWrite.c
      H5Vol.c
  )

  add_library(adios2_h5vol  SHARED ${VOLSRC})	

  target_include_directories(adios2_h5vol  
            PUBLIC
            ${HDF5_C_INCLUDE_DIRS}
  )	 
  target_link_libraries(adios2_h5vol
      ${HDF5_C_LIBRARIES} MPI::MPI_C ${CMAKE_THREAD_LIBS_INIT}      
  ) 				
  target_link_libraries(adios2_h5vol adios2)	

endif()

target_link_libraries(H5VolReadWriteC adios2)
target_link_libraries(H5VolReadWriteC MPI::MPI_C)
