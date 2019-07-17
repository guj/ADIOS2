#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <adios2.h>


#include <pugixml/pugixml/src/pugixml.hpp>
#include <fstream>


#include "xml.h"
#include "Query.h"
#include "Query.tcc"












int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const std::size_t Nx = 10;

    try
    {
      //adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

      //adios2::IO bpIO = adios.DeclareIO("QueryTest");
      //adios2::IO idxIO = adios.DeclareIO("BlockIndex");

      size_t recommendedSize = 20000;
      bool overwrite = false;

      std::string configFileName = "query.xml";
      if (argc > 1)
	configFileName = argv[1];
      if (argc > 3) {
	if (argv[2][0] == 'w') {
	  overwrite = true;
	  recommendedSize = atol(argv[3]);
	} else {
	  recommendedSize = atol(argv[2]);
	  overwrite = (argv[3][0] == 'w');
	}
      } else if (argc == 3)  {	// argv == 2
	if (argv[2][0] == 'w') 
	  overwrite = true;
	else 
	  recommendedSize = atol(argv[2]);	  
      }
      
      if (rank == 0) {
	std::cout<<" file = "<<configFileName<<std::endl;
	std::cout<<"     overwrite existing idx file? "<<overwrite<<std::endl;
	std::cout<<"     recommended block size for idx? "<<recommendedSize<<std::endl;
      }
      
      adios2::query::xmlUtil::Parser p(MPI_COMM_WORLD, configFileName);
      p.go(overwrite, recommendedSize);
      return 0;
      
	/*
	*/
    } 
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
