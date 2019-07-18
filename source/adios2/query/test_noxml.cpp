#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <adios2.h>


#include <pugixml.hpp>


#include "Query.h"




int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const std::size_t Nx = 10;

    try
    {
      adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);

        adios2::IO bpIO = adios.DeclareIO("QueryTest");

	std::string inputFileName = "default.bp";
	if (argc > 1) 
	  inputFileName = argv[1];

        adios2::Engine bpReader =  bpIO.Open(inputFileName, adios2::Mode::Read);	  
	
	std::string varName = "defaultVar";
	if (argc > 2) 
	  varName = argv[2];

        adios2::Variable<double> bpVar =
            bpIO.InquireVariable<double>(varName);

        if (!bpVar) //
        {
	  std::cout<<" No such variable "<<varName<<std::endl;
	  return -1;
	} 
	
	//bpVar.SetSelection({{Nx * rank}, {Nx}});
	adios2::Query<double> simpleQ(bpReader, bpVar);
	//std::cout<<"Query: "<<varName<<" < "<<val<<std::endl;
	simpleQ.addRange(adios2::QueryOp::LT, 2.0);
	simpleQ.m_RangeTree.setRelation(adios2::Relation::OR);
	simpleQ.addRange(adios2::QueryOp::GT, 10.4);
	
	adios2::RangeTree<double> subNode;
	subNode.setRelation(adios2::Relation::AND);
	subNode.addRange(adios2::QueryOp::GT, 1.8);
	subNode.addRange(adios2::QueryOp::LT, 1.9);

	simpleQ.m_RangeTree.addNode(subNode);

	if (rank == 0) 
	  simpleQ.print();

	std::string idxFileName = "idx"+inputFileName;
	//bool overwrite = false;
	bool overwrite = false;
	adios2::BlockIndexBuilder builder(idxFileName, MPI_COMM_WORLD, overwrite);
	double timeStart = MPI_Wtime();
	size_t recommendedSize = 20000;
	adios2::IO idxWriteIO = adios.DeclareIO(std::string("BLOCKINDEX-Write-")+inputFileName);        
	builder.GenerateIndexFrom(bpIO, idxWriteIO, bpReader, recommendedSize);
	double timeEnd = MPI_Wtime();
	
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) 
	  std::cout<<"==> Total time on index file (creation) = "<<timeEnd - timeStart<<std::endl;

        adios2::IO idxIO = adios.DeclareIO("BlockIndex");
	adios2::Engine idxReader = idxIO.Open(idxFileName, adios2::Mode::Read);

	adios2::BlockIndex<double> idx(idxIO, idxReader); 

	adios2::BlockIndexTool<double> queryTool(&idx, MPI_COMM_WORLD);

	
	//while (queryTool.BeginStep() == adios2::StepStatus::OK) 
	for (int i= 0; i< bpVar.Steps(); i++)	       
	  {
	    int ts = bpVar.StepsStart() + i;

	    std::vector<double> result;// result will be resized after blocks are identified
	    std::vector<adios2::Dims> coordinates;
	    queryTool.Process(simpleQ, i, result, coordinates);

	  }
		
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0) 
	  std::cout<<"\n==> Total time on processing query: "<<MPI_Wtime() - timeEnd <<std::endl;

	// results??     
	
        bpReader.Close();    
    } 
    catch (std::exception &e)
    {
        std::cout << "Exception, STOPPING PROGRAM from rank " << rank << "\n";
        std::cout << e.what() << "\n";
    }

    MPI_Finalize();

    return 0;
}
