#include "Query.h"
//#include "Query.tcc"

#include <sys/stat.h>

namespace adios2
{
#define declare_type(T)							\
									\
  template<>								\
  Query<T>::Query(adios2::Engine& engine, Variable<T>& var)		\
    :m_Var(var), m_Reader(engine)					\
  {									\
  }									\
									\
  template<>								\
  void Query<T>::addRange(adios2::QueryOp op, T value)			\
  {									\
    m_RangeTree.addRange(op, value);					\
  }									\
  template<>								\
  void Query<T>::setSelection(adios2::Dims& start, adios2::Dims& count)	\
  {									\
    m_BoundaryStart.resize(start.size());				\
    m_BoundaryCount.resize(count.size());				\
    m_BoundaryStart = start;						\
    m_BoundaryCount = count;						\
  }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type





#define declare_type(T)						\
  template<> 							\
  adios2::BlockIndex<T>::BlockIndex(adios2::IO& io, adios2::Engine& idxReader) \
    :m_IdxIO(io), m_IdxReader(idxReader)       			\
  {								\
  }								\
  template<> 							\
  void BlockIndex<T>::Init (adios2::Engine& idxReader, const adios2::Variable<T>& var, int ts) \
  {								\
  }								\
  template<> 							\
  void BlockIndex<T>::Evaluate(const Query<T>& query, std::vector<unsigned int>& resultSubBlocks) \
  {								\
  }					
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type


#define declare_type(T)						\
  template<> 							\
  BlockIndexTool<T>::BlockIndexTool(BlockIndex<T>* tool, MPI_Comm comm) { \
    m_Tool = tool;						\
    m_Comm = comm;						\
    MPI_Comm_rank(m_Comm, &m_Rank);				\
    MPI_Comm_size(m_Comm, &m_Size);				\
  }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
  /*
#define declare_template_instantiation(T)			\
  template void BlockIndexTool::Process(const Query<T>&q, int ts,  std::vector<T> &dataV); \

  ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
  */



adios2::BlockIndexBuilder::BlockIndexBuilder(std::string& idxFileName, 
					     MPI_Comm comm,
					     bool overwrite)
:m_OverwriteExisting(overwrite),
  m_IdxFileName(idxFileName),
  m_Finished(false),
  m_MPIComm(comm)
{
  struct stat buf;
  bool exists = (stat(idxFileName.c_str(), &buf) == 0);
    
  if (exists && !overwrite) 
    m_Finished = true;  

  MPI_Comm_rank(m_MPIComm, &m_Rank);	       
  MPI_Comm_size(m_MPIComm, &m_Size);	       

}




void adios2::BlockIndexBuilder::GenerateIndexFrom(adios2::IO& dataIO, 
						  adios2::IO& idxIO,
						  adios2::Engine& dataReader,
						  size_t unitElement)
{
    if (m_Finished) // nothing to be done
      return;

    int rank;								
    MPI_Comm_rank(m_MPIComm, &rank);	       


    //adios2::Engine idxWriter =  dataIO.Open(m_IdxFileName, adios2::Mode::Write);
    adios2::Engine idxWriter =  idxIO.Open(m_IdxFileName, adios2::Mode::Write);
    const std::map<std::string, adios2::Params> variables =  dataIO.AvailableVariables();

    while (dataReader.BeginStep() == adios2::StepStatus::OK)	
      {
	idxWriter.BeginStep();					
	for (const auto variablePair : variables)
	  {
	    std::string name = variablePair.first;
	    auto it = variablePair.second.find("Type");
	    
	    const std::string &type = it->second;
	    it = variablePair.second.find("Shape");

	    //const std::string &shape = it->second;
	    if (type == "compound")
	      {
		// not supported 
		std::cout<<" not supporting compound types .. "<<std::endl;
	      }
	    // :::   NOTE   :::
	    // looks like variable shapes are fixed at DefineVar
	    // and it can be called once in io.
	    // so until furthur development, we only deal with
	    // vars with a fixed shape for all timesteps
	    //
#define declare_template_instantiation(T)				\
	    else if (type == adios2::GetType<T>())			\
	      {								\
		Variable<T> var = dataIO.InquireVariable<T>(name);	\
		if (var) {						\
		  handleVar(idxIO, var, dataReader, idxWriter, unitElement); \
		}							\
	      }
	    //ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
	    //NOTE: not expecting to deal with complex types and std::string in the query
	    ADIOS2_FOREACH_ATTRIBUTE_PRIMITIVE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation	  
	  
	  } // for

	idxWriter.EndStep();
	dataReader.EndStep();
      }

    idxWriter.Close();
}



/*

     template <class T> 
     void Range<T>::print(std::string& varName, int level)
	 {
	   for (int i=0; i<level; i++)
	     std::cout<<"\t";

	   switch (m_Op)
	     {
	     case adios2::QueryOp::GT:
	       std::cout<<varName<<" > "<<m_Value<<std::endl;
	       break;
	     case adios2::QueryOp::LT:
	       std::cout<<varName<<" < "<<m_Value<<std::endl;
	       break;
	     case adios2::QueryOp::GE:
	       std::cout<<varName<<" >= "<<m_Value<<std::endl;
	       break;
	     case adios2::QueryOp::LE:
	       std::cout<<varName<<" <= "<<m_Value<<std::endl;
	       break;
	     case adios2::QueryOp::EQ:
	       std::cout<<varName<<" == "<<m_Value<<std::endl;
	       break;
	     case adios2::QueryOp::NE:
	       std::cout<<varName<<" != "<<m_Value<<std::endl;
	       break;
	     default:
	       break;
	     }
	 }// print;

template <class T> 
       void RangeTree<T>::print(std::string& varName, int level)
       {
	 for (int i=0; i<level; i++)
	   std::cout<<"\t";

	 if (adios2::Relation::AND == m_Relation)
	   std::cout<<"AND:"<<std::endl;
	 else 
	   std::cout<<"OR:"<<std::endl;

	 for (auto& range : m_Leaves)
	   range.print(varName, level+1);

	 for (auto& node : m_SubNodes)
	   node.print(varName, level+1);
       }


template <class T>
void Query<T>::print() {
  int i;
  std::cout<<std::endl;
  std::string varName = m_Var.Name();
  m_RangeTree.print(varName, 0);
  std::cout<<std::endl;
  if (m_BoundaryStart.size() > 0) {
    std::stringstream ss, cc;
    for (i=0; i<m_BoundaryStart.size(); i++) {
      ss << m_BoundaryStart[i]<<" ";
      cc << m_BoundaryCount[i]<<" ";
    }
    std::cout<<"bounding box: start: ("<<ss.str()<<") count: ("<<cc.str()<<") "<<std::endl;
  } 
  
  if (m_timestepCount > 0) {
    std::cout<<" timesteps of interest: "<<m_timestepStart<<" for: "<<m_timestepCount<<" steps."<<std::endl;
  } 	 
}
*/


  bool BlockIndexBuilder::divideMe(adios2::Dims& varShape, size_t suggestedBlockSize, adios2::Dims& divider)
     {
	 int n = GetSize(varShape) / suggestedBlockSize;			
	 if (n == 0) {						
	   return false;
	 }		

	 int ndim = varShape.size();
	   
	 //
	 // A simple algorithm. divide along the largest dimension
	 //
	 int maxDim = 0; 
	 divider[0]=1;
	 for (int i=1; i<ndim; i++) {
	   divider[i] = 1;
	   if (varShape[i] > varShape[maxDim]) {
	     maxDim = i;
	   }
	 }

	 if (varShape[maxDim]/n < 2)
	   return false; // simple algorithm is not handling this case yet

	 divider[maxDim] = n;

	 return true;
     }


  void BlockIndexBuilder::GetIdx(const adios2::Dims& scope, int pos, adios2::Dims& result) 
     {
       int ndim = scope.size();
       if (1 == ndim) {
	 result[0] = pos;
	 return;
       }

       result[ndim-1] = pos % scope[ndim-1];

       for (auto i=0; i<ndim-1; i++) {
	 auto chunk = scope[ndim-1];
	 for (auto k=1; k<ndim-1; k++) {
	   chunk *= scope[k];
	 }
	 result[i] = pos/chunk;
       }	 
     }
       


} // namespace adios2

