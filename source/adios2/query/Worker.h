#ifndef QUERY_WORKER_H
#define QUERY_WORKER_H

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <adios2.h>


#include "Xml.h"

#include "Query.h"
#include "Query.tcc"


namespace adios2 
{
  namespace query
  {
    template <class T>
      class Worker {
    public:
      Worker(std::string& configFile, MPI_Comm& comm);

      bool PrepareIdx(bool overwrite, size_t recommendedSize, std::string const& varName);
      
      void SetSource(adios2::IO& io, adios2::Engine& reader) 
      {*m_DataReader = reader;  *m_DataIO = io;}         

      unsigned long Estimate();
      
      void GetResult(std::vector<T>& dataV,
		     std::vector<adios2::Dims>& posV)
      { /*tbd*/}

      bool BuildIdxFile(bool overwrite, size_t recommendedSize) 
      {
	adios2::IO idxWriteIO = m_adios2.DeclareIO(std::string("BLOCKINDEX-Write-")+m_DataReader->Name());        
	adios2::BlockIndexBuilder builder(m_IdxFileName, m_Comm, overwrite);
	builder.GenerateIndexFrom(*m_DataIO, idxWriteIO, *m_DataReader, recommendedSize);	
      }

      bool HasMoreSteps() {return false; /*tbd*/}
    private:
      MPI_Comm m_Comm;
      adios2::ADIOS m_adios2;
      adios2::IO* m_DataIO = nullptr;
      adios2::Engine* m_DataReader = nullptr;
      adios2::Query<T>* m_Query = nullptr;

      std::string m_ConfigFile;
      std::string m_IdxFileName;

    }; // worker
  }; // namespace query  
}; // name space adios2

#endif //QUERY_WORKER_H
