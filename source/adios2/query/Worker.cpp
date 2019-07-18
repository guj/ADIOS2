#include "Worker.h"

namespace adios2 {
  namespace query {
    template <class T>
    Worker<T>::Worker(std::string& configFile, MPI_Comm& comm)
      :m_ConfigFile(configFile), m_Comm(comm)
    {
      m_adios2 = adios2::ADIOS(comm, adios2::DebugOFF);
    }

    template <class T>
    unsigned long Worker<T>::Estimate()
    {
      return 0; /*tbd*/
    }

    template <class T>
    bool Worker<T>::PrepareIdx(bool overwrite, size_t recommendedSize, std::string const& varName) 
    {
      if (m_DataReader == nullptr) {
	return false;
      }
      bool result = false;
            
      auto lf_FileContents = [&](const std::string &configXML) -> std::string {    
	std::ifstream fileStream(configXML);	    
	if (!fileStream)	      
	  throw std::ios_base::failure("ERROR: file " + configXML +  " not found. ");	      
	
	std::ostringstream fileSS;
	fileSS << fileStream.rdbuf();
	fileStream.close();
	
	if (fileSS.str().empty())	      
	  throw std::invalid_argument("ERROR: config xml file is empty.");				    
	
	return fileSS.str();
      }; // lf_FileContents
      

      const std::string fileContents = lf_FileContents(m_ConfigFile);
      const pugi::xml_document document = adios2::query::xmlUtil::XMLDocument(fileContents);
      
      const pugi::xml_node config = adios2::query::xmlUtil::XMLNode("adios-query", document,  true);
      
      for (const pugi::xml_node &ioNode : config.children("io"))	  
	{
	  const pugi::xml_attribute currIOName = adios2::query::xmlUtil::XMLAttribute("name", ioNode);
	  if (m_DataIO->Name().compare(currIOName.value()) != 0) 
	    continue;
	 
	  // work with target IO
	  for (const pugi::xml_node &varNode : ioNode.children("var")) {
	    const std::string variableName =
	      std::string(adios2::query::xmlUtil::XMLAttribute("name", varNode).value());
	    if (varName.compare(variableName) != 0)
	      continue;	
	    else  {
	      result = true;
	      break;
	    }
	  }
	} // io node loop

      if (!result) 
	return false; // no matching io & var names found in config file
      
      adios2::query::xmlUtil::Parser::getIdxFileName(m_DataReader->Name(), m_IdxFileName);	      
      BuildIdxFile(overwrite, recommendedSize);
      /*
	const std::string varType = m_DataIO->VariableType(varName);	
	#define declare_type(T)						\
	if (varType == adios2::GetType<T>())				\
	{								\
	Variable<T> var = currentIO.InquireVariable<T>(varName);	\		
	adios2::Query<T> simpleQ(*m_DataReader, var);			\
	adios2::query::xmlUtil::constructQuery(simpleQ, node);		\
	}
	ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type) //skip complex types
	#undef declare_type    	  
	
	parse_VarNode(variable, *m_DataIO, *m_DataReader);
	result = true; // found IO & var
	}
      */
      return true;
    } // preprocess
    
  };
}; 

