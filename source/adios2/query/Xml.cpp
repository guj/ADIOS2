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

#include "Xml.h"
#include "Query.h"
#include "Query.tcc"


namespace adios2 
{
  namespace query
  {
    namespace xmlUtil {
      
      pugi::xml_document XMLDocument(const std::string &xmlContents)
      {
	pugi::xml_document document;
	auto parse_result = document.load_buffer_inplace(const_cast<char *>(xmlContents.data()), xmlContents.size());
	
	if (!parse_result)
	  { 
	    throw std::invalid_argument("ERROR: XML parse error: " + std::string(parse_result.description()));
	  }
	
	return document;
      }
      
      
      
      pugi::xml_node XMLNode(const std::string nodeName,
			     const pugi::xml_document &xmlDocument,
			     const bool isMandatory,
			     const bool isUnique )
      {
	const pugi::xml_node node = xmlDocument.child(nodeName.c_str());
	
	if (isMandatory && !node)
	  {
	    throw std::invalid_argument("ERROR: XML: no <" + nodeName + "> element found");
	  }
	
	if (isUnique)
	  {
	    const size_t nodes =
	      std::distance(xmlDocument.children(nodeName.c_str()).begin(),
			    xmlDocument.children(nodeName.c_str()).end());
	    if (nodes > 1)
	      {
		throw std::invalid_argument("ERROR: XML only one <" + nodeName +
					    "> element can exist inside " +
					    std::string(xmlDocument.name()));
	      }
	  }
	
	return node;
      }
      
      pugi::xml_node XMLNode(const std::string nodeName,
			     const pugi::xml_node &upperNode, 
			     const bool isMandatory,
			     const bool isUnique)
      {
	const pugi::xml_node node = upperNode.child(nodeName.c_str());
	
	if (isMandatory && !node)
	  {
	    throw std::invalid_argument("ERROR: XML: no <" + nodeName + "> element found, inside <" +
					std::string(upperNode.name()) + "> element ");
	  }
	
	if (isUnique)
	  {
	    const size_t nodes =
	      std::distance(upperNode.children(nodeName.c_str()).begin(),
			    upperNode.children(nodeName.c_str()).end());
	    if (nodes > 1)	     
		throw std::invalid_argument("ERROR: XML only one <" + nodeName +
					    "> element can exist inside <" +
					    std::string(upperNode.name()) +
					    "> element. ");
	    
	  }    
	return node;
      }
      
      
      pugi::xml_attribute XMLAttribute(const std::string attributeName,
				       const pugi::xml_node &node,
				       const bool isMandatory)
      {
	const pugi::xml_attribute attribute = node.attribute(attributeName.c_str());
	
	if (isMandatory && !attribute)
	  {
	    const std::string nodeName(node.name());
	    throw std::invalid_argument("ERROR: XML: No attribute " +
					attributeName + " found on <" +
					nodeName + "> element");
	  }
	return attribute;
      }     



      adios2::Dims split (const std::string &s, char delim) {
	adios2::Dims dim; 
	
	std::stringstream ss (s);
	std::string item;
	
	while (getline (ss, item, delim)) {
	  std::stringstream curr(item);
	  size_t val; 
	  curr >> val;
	  dim.push_back (val);
	}
	
	return dim;
      }







      //
      // parser
      // 
      //adios2::query::xmlUtil::Parser::Parser(MPI_Comm comm, std::string& queryFileName)
      Parser::Parser(MPI_Comm comm, std::string& queryFileName)
	:m_Comm(comm), m_XMLConfigFile(queryFileName)
      {
	m_adios2 = adios2::ADIOS(comm, adios2::DebugOFF);
      }

      void Parser::prepareIdxFile(adios2::Engine& reader,
				  adios2::IO &currentIO,
				  std::string& idxFileName)
      {
	adios2::IO idxWriteIO = m_adios2.DeclareIO(std::string("BLOCKINDEX-Write-")+reader.Name());        
	adios2::BlockIndexBuilder builder(idxFileName, m_Comm, m_overwrite);
	builder.GenerateIndexFrom(currentIO, idxWriteIO, reader, m_recommendedSize);
      }


      void Parser::go(bool overwrite, size_t recommendedSize) {
	  m_overwrite = overwrite;
	  m_recommendedSize = recommendedSize; 

	  auto lf_FileContents = [&](const std::string &configXML) -> std::string {    
	    std::ifstream fileStream(configXML);
	    
	    if (!fileStream)	      
		throw std::ios_base::failure("ERROR: file " + configXML +  " not found. ");
	      
  
	    std::ostringstream fileSS;
	    fileSS << fileStream.rdbuf();
	    fileStream.close();
	    
	    //const std::string fileContents(fileSS.str());
	    
	    if (fileSS.str().empty())	      
		throw std::invalid_argument("ERROR: config xml file is empty.");				    
	    
	    return fileSS.str();
	  };


	  const std::string fileContents = lf_FileContents(m_XMLConfigFile);
	  const pugi::xml_document document = adios2::query::xmlUtil::XMLDocument(fileContents);
    
	  const pugi::xml_node config = adios2::query::xmlUtil::XMLNode("adios-query", document,  true);
					
	  for (const pugi::xml_node &ioNode : config.children("io"))	    
	      parse_IONode(ioNode);	  
      }


      void Parser::parse_IONode(const pugi::xml_node &ioNode) 
      {
	auto lf_GetParametersXML = [&](const pugi::xml_node &node) -> adios2::Params {
	  const std::string errorMessage("in node " + std::string(node.value()));
	  
	  adios2::Params parameters;
	  
	  for (const pugi::xml_node paramNode : node.children("parameter"))
	    {
	      const pugi::xml_attribute key = adios2::query::xmlUtil::XMLAttribute("key", paramNode);	  
	      const pugi::xml_attribute value = adios2::query::xmlUtil::XMLAttribute("value", paramNode);
	      parameters.emplace(key.value(), value.value());
	    }
	  return parameters;
	};
	

	const pugi::xml_attribute ioName = adios2::query::xmlUtil::XMLAttribute("name", ioNode);
	const pugi::xml_attribute fileName = adios2::query::xmlUtil::XMLAttribute("file", ioNode);
	
	// must be unique per io
	const pugi::xml_node &engineNode = adios2::query::xmlUtil::XMLNode("engine", ioNode,  false, true);
	//adios2::ADIOS adios(m_Comm, adios2::DebugON);
	adios2::IO currIO = m_adios2.DeclareIO(ioName.value());        
	
	if (engineNode) {
	  const pugi::xml_attribute type = adios2::query::xmlUtil::XMLAttribute("type", engineNode);	  
	  currIO.SetEngine(type.value());
	  
	  const adios2::Params parameters = lf_GetParametersXML(engineNode);
	  currIO.SetParameters(parameters);
	}
	
	adios2::Engine reader =  currIO.Open(fileName.value(), adios2::Mode::Read, m_Comm);	  
	
	//std::string idxFileName = reader.Name()+"_idx.bp";

	getIdxFileName(reader.Name(), m_idxFileName);

	prepareIdxFile(reader, currIO, m_idxFileName);
	
	for (const pugi::xml_node &variable : ioNode.children("var"))
	  parse_VarNode(variable, currIO, reader);

	reader.Close();
      } // parse_IONode



      // node is the variable node
      void Parser::parse_VarNode(const pugi::xml_node &node,
				 adios2::IO &currentIO,
				 adios2::Engine& reader)
      {
	adios2::IO idxReadIO = m_adios2.DeclareIO(std::string("BLOCKINDEX-Read-")+reader.Name());        
	const std::string variableName = std::string(adios2::query::xmlUtil::XMLAttribute("name", node).value());
	
	const std::string varType = currentIO.VariableType(variableName);	
	if (varType.size() == 0) {
	  std::cout<<"No such variable: "<<variableName<<std::endl;
	  return;
	}
#define declare_type(T)							\
	if (varType == adios2::GetType<T>())				\
	  {								\
	    Variable<T> var = currentIO.InquireVariable<T>(variableName); \
	    handleType(node, currentIO, idxReadIO, reader, var);	\
	  }
	ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type) //skip complex types
#undef declare_type    	  
	  }


    }; // end namespace xmlUtil



      

  }// end namespace query
} // end namespace adios2
