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

install(FILES H5Vol.h H5VolReadWrite.h H5VolError.h
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/adios2/h5vol/
)

# Library installation
install(TARGETS adios2_h5vol
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

