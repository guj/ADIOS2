#ifndef XML_UTIL_H
#define XML_UTIL_H

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout
#include <mpi.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <adios2.h>


//#include <pugixml/pugixml/src/pugixml.hpp>
#include <pugixml.hpp>
#include <fstream>


#include "Query.h"
#include "Query.tcc"

namespace adios2 
{
  namespace query
  {
    namespace xmlUtil {

      pugi::xml_attribute XMLAttribute(const std::string attributeName,
				       const pugi::xml_node &node,
				       const bool isMandatory = true);
	

      adios2::Dims split (const std::string &s, char delim);
    
      pugi::xml_document XMLDocument(const std::string &xmlContents);
      
      pugi::xml_node XMLNode(const std::string nodeName,
			     const pugi::xml_document &xmlDocument,
			     const bool isMandatory = true,
			     const bool isUnique = false);
      
      pugi::xml_node XMLNode(const std::string nodeName,
			     const pugi::xml_node &upperNode, 
			     const bool isMandatory = true,
			     const bool isUnique = false);
    


      template <class T> 
      void constructTree(RangeTree<T>& host, const pugi::xml_node& node)
      {
	std::string relationStr = adios2::query::xmlUtil::XMLAttribute("value", node).value();
	host.setRelation(adios2::strToRelation(relationStr));   
	for (const pugi::xml_node rangeNode : node.children("range")) {
	  std::string opStr = adios2::query::xmlUtil::XMLAttribute("compare", rangeNode).value();
	  std::string valStr = adios2::query::xmlUtil::XMLAttribute("value", rangeNode).value();
	  
	  std::stringstream convert(valStr);  
	  T val;
	  convert >> val;
	  
	  host.addRange(adios2::strToQueryOp(opStr), val);
	}
	
	for (const pugi::xml_node opNode : node.children("op")) {             
	  adios2::RangeTree<T> subNode;
	  constructTree(subNode, opNode);
	  host.addNode(subNode);
	}
      }

      
      template <class T>
	void constructQuery(adios2::Query<T>& simpleQ,
			    const pugi::xml_node &node)
	{
	  pugi::xml_node bbNode = node.child("boundingbox");
	  if (bbNode) {
	    adios2::Box<adios2::Dims> box =  adios2::Box<adios2::Dims>({100,100}, {200,200});
	    std::string startStr = adios2::query::xmlUtil::XMLAttribute("start", bbNode).value();
	    std::string countStr = adios2::query::xmlUtil::XMLAttribute("count", bbNode).value();
	    
	    adios2::Dims start = split(startStr, ',');
	    adios2::Dims count = split(countStr, ',');
	    
	    if (start.size() != count.size()) {
	      throw std::ios_base::failure("size of startcount does match in bounding box definition");
	    }
	    if (start.size() != simpleQ.m_Var.Shape().size()) {
	      throw std::ios_base::failure(" size of startcount does not match var dimention in bounding box  definition");
	    }
	    
	    //simpleQ.setSelection(box.first, box.second);
	    simpleQ.setSelection(start, count);
	  }

	  pugi::xml_node tsNode = node.child("tstep");
	  if (tsNode) {
	    std::string startStr = adios2::query::xmlUtil::XMLAttribute("start", tsNode).value();
	    std::string countStr = adios2::query::xmlUtil::XMLAttribute("count", tsNode).value(); 
	    
	    if ((startStr.size() > 0) && (countStr.size() > 0)) {
	      std::stringstream ss(startStr), cc(countStr);
	      ss >> simpleQ.m_timestepStart;
	      cc >> simpleQ.m_timestepCount;
	    }
	  }
	  pugi::xml_node relationNode = node.child("op");
	  
	  constructTree(simpleQ.m_RangeTree, relationNode);    	  
	}








      //
      // class Parser
      // 
    class Parser 
    {
    public: 
      Parser(MPI_Comm comm, std::string& queryFileName);	  
      
      void go(bool overwrite, size_t recommendSize);

    private:      
      MPI_Comm m_Comm;
      std::string& m_XMLConfigFile;
      std::string m_idxFileName;
      adios2::ADIOS m_adios2;

      bool m_overwrite; // whether or not overwrite existing idx file
      size_t m_recommendedSize; // num elements per block when building blockindex

    public:
      void parse_IONode(const pugi::xml_node &ioNode);
      void parse_VarNode(const pugi::xml_node &node,
			 adios2::IO &currentIO,
			 adios2::Engine& reader);


      void prepareIdxFile(adios2::Engine& reader,
			  adios2::IO &currentIO,
			  std::string& idxFileName);
      
      static void getIdxFileName(const std::string& input, std::string& idxFileName)
	{

	  auto lf_endsWith = [&](std::string const &fullString, std::string const &endstr) -> bool {
	    if (fullString.length() >= endstr.length()) {
	      return (0 == fullString.compare (fullString.length() - endstr.length(), endstr.length(), endstr));
	    } else {
	      return false;
	    }
	  };

	  std::string dotBP=".bp";
	  if (lf_endsWith(input, dotBP)) 
	    idxFileName = input+"_idx";
	  else
	    idxFileName = input+"_idx.bp";
	}


      template <class T>
	void handleType(const pugi::xml_node &node,
			adios2::IO &currentIO,
			adios2::IO &idxIO2,
			adios2::Engine& reader,
			Variable<T>& var)
	{
	  if (!var) {
	    std::cout<<" No such variable "<<var.Name()<<std::endl;
	    return ;
	  } 
	  
	    adios2::Query<T> simpleQ(reader, var);
	    adios2::query::xmlUtil::constructQuery(simpleQ, node);
	    
	    //std::string idxFileName = reader.Name()+"_idx.bp";
	    //std::string idxFileName;
	    //getIdxFileName(reader.Name(), idxFileName);
	    
	    MPI_Barrier(m_Comm);
	    int rank; MPI_Comm_rank(m_Comm, &rank);
	    if (rank == 0)
	      simpleQ.print();
	    


	    double timeStart = MPI_Wtime();  
	    
	    adios2::Engine idxReader = idxIO2.Open(m_idxFileName, adios2::Mode::Read);
	    adios2::BlockIndex<T> idx(idxIO2, idxReader); 
	    adios2::BlockIndexTool<T> queryTool(&idx, m_Comm);  
	    
	    size_t nsteps = var.Steps();
	    if (simpleQ.m_timestepCount > 0) {
	      nsteps = std::min(var.Steps(), simpleQ.m_timestepCount);
	    }

	    for (size_t i= 0; i< nsteps; i++)	       
	      {
		//size_t ts = var.StepsStart() + i;      
		size_t ts = simpleQ.m_timestepStart + i;
		std::vector<T> result;// result will be resized after blocks are identified
		std::vector<adios2::Dims> pos;// result will be resized after blocks are identified
		queryTool.Process(simpleQ, ts, result, pos);      
	      }
	    
	    MPI_Barrier(m_Comm);
	    if (rank == 0) 
	      std::cout<<"\n==> Total time on processing query: "<<MPI_Wtime() - timeStart <<std::endl;
	    
	    // results??       
	    idxReader.Close();    
	}

    };




    


    }// end namespace xmlutil
  };// end namespace query
}; // end namespace adios2

#endif // XML_UTIL_H
