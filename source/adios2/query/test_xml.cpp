#include <mpi.h>

#include "adios2.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Xml.h"
#include "Query.h"
#include "Worker.h"
#include "Worker.tcc"


bool testMe(std::string& queryConfigFile, std::string const& doubleVarName, MPI_Comm comm)
{
  adios2::ADIOS ad(queryConfigFile, comm, adios2::DebugON);
  adios2::IO inIO = ad.DeclareIO("query");

  std::string dataFileName = "test.file";
  adios2::Engine reader = inIO.Open(dataFileName, adios2::Mode::Read, comm);  

  //adios2::query::xmlUtil::Parser p(comm, queryConfigFile);
  adios2::query::Worker<double> w(queryConfigFile, comm);

  bool overwrite = false;
  size_t recommendedSize = 20000;

  w.SetSource(inIO, reader);

  if (!w.PrepareIdx(overwrite, recommendedSize,  doubleVarName))
    return false;

  std::vector<double> dataOutput;
  std::vector<adios2::Dims> coordinateOutput; 

  while (w.HasMoreSteps()) {
    unsigned long long roughEstimate = w.Estimate();
    
    w.GetResult(dataOutput, coordinateOutput); 
  }
  
  return true;
}

/*
bool testQueryManual(std::string& configFile, std::string& doubleVarName, MPI_Comm comm)
{
  adios2::ADIOS ad(configFile, comm, adios2::DebugON);
  adios2::IO inIO = ad.DeclareIO("query");

  std::string dataFileName = "test.file";
  adios2::Engine reader = inIO.Open(dataFileName, adios2::Mode::Read, comm);  
  
  std::vector<double> dataOutput;
  std::vector<adios2::Dims> coorindatesOutput; 
    
  adios2::Variable<double> varIn;

  // initiate and prepare the related idx file
  adios2::BlockIndexTool queryTool(reader, doubleVarName, overwrite, recommendedSize);

  adios2::Query<double> simpleQuery;
  construct simpleQuery here...

  while (true)
    {
      adios2::StepStatus status = reader.BeginStep(adios2::StepMode::Read);      
      if (status != adios2::StepStatus::OK)
	break;
      
      varIn = inIO.InquireVariable<double>(doubleVarName);    
      if (!varIn)
	break;

      tool.processCurrent(simepleQuery, results);


      no need the rest ...

      //Tin.resize(...);    
      //reader.Get<double>(varIn, Tin.data());
      //reader.EndStep();    
      
      // load query on this variable if not already
      queryTool.load(varIn); 
      // if no selection in config and selection in the bbox above conflicts
      // use the config selection
      
      unsigned long estimatedHist;
      // evalute on this timestep, if ts is not in the config, return false
      bool isValid = queryTool.evaluate(&estimatedHits); 
      if (user_is_willing_to_expore_more) {
	queryTool.GetCoordinates(&coordinates);    
	// or  queryTool.GetValues(&Tin);      
	// or  queryTool.GetOutput(&coordinates, &values);      
      }
      reader.EndStep();      
    }
  reader.Close();
}  // testQuery()
*/
