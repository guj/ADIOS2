#ifndef ADIOS_QUERY_TCC
#define ADIOS_QUERY_TCC

#include "Query.h"


namespace adios2 
{
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
       

       template <class T> 
       bool Range<T>::check(T val) const 
	 {
	   bool isHit = false;
	   switch (m_Op)
	     {
	     case adios2::QueryOp::GT:
	       isHit = (val > m_Value);
	       break;
	     case adios2::QueryOp::LT:
	       isHit = (val < m_Value);
	       break;
	     case adios2::QueryOp::GE:
	       isHit = (val >= m_Value);
	       break;
	     case adios2::QueryOp::LE:
	       isHit = (val <= m_Value);
	       break;
	     case adios2::QueryOp::EQ:
	       isHit = (val == m_Value);
	       break;
	     case adios2::QueryOp::NE:
	       isHit = (val != m_Value);
	       break;
	     default:
	       break;
	     }
	   
	   return isHit;
	 }

	 template<class T>
	 bool Range<T>::checkInterval (T& min, T& max) const
	 {
	     bool isHit  = false;
	     switch (m_Op)
	       {
	       case adios2::QueryOp::GT:
		 isHit = (max > m_Value);
		 break;
	       case adios2::QueryOp::LT:
		 isHit = (min < m_Value);
		 break;
	       case adios2::QueryOp::GE:
		 isHit = (max >= m_Value);
		 break;
	       case adios2::QueryOp::LE:
		 isHit = (min <= m_Value);
		 break;
	       case adios2::QueryOp::EQ:
		 isHit = (max >= m_Value) && (min <= m_Value);
		 break;
	       case adios2::QueryOp::NE:
		 isHit = !((max == m_Value) && (min == m_Value));
		 break;
	       default:
		 break;
	       }
	     return isHit;
	 }


template <class T>
       bool RangeTree<T>::check(T value) const 
       {
	 if (adios2::Relation::AND == m_Relation)
	 {
	   for (auto& range : m_Leaves)
	     if (!range.check(value))
	       return false;
	   
	   for (auto& node : m_SubNodes)
	     if (!node.check(value))
	       return false;

	   return true; // even if no leaves or nodes
	 } 
	 
	 // OR
	 for (auto& range : m_Leaves)
	   if (range.check(value))
	       return true;
	 
	 for (auto& node : m_SubNodes)
	   if (node.check(value))
	     return true;
	 
	 return false;
	 
       }

template <class T>
       bool RangeTree<T>::checkInterval (T& min, T& max) const
       {
	 if (adios2::Relation::AND == m_Relation)
	 {
	   for (auto& range : m_Leaves)
	     if (!range.checkInterval(min, max))
	       return false;
	   
	   for (auto& node : m_SubNodes)
	     if (!node.checkInterval(min, max))
	       return false;

	   return true; // even if no leaves or nodes
	 } 
	 
	 // OR
	 for (auto& range : m_Leaves)
	     if (range.checkInterval(min, max))
	       return true;
	 
	 for (auto& node : m_SubNodes)
	   if (node.checkInterval(min, max))
	     return true;
	 
	 return false;
	 
       }


       template <class T>
       void BlockIndex<T>::SetContent(const Query<T>& query, int ts)
       {
	 //
	 // if idx file is empty, use the block info 
	 //

	 if (!m_IdxIO.InquireVariable<T>(query.m_Var.Name()+"_minmax")) {
	   std::cout<<".... idx has no info. using blocks to evaluate. .... "<<std::endl;
	   query.m_Var.SetStepSelection({ts, 1});
	   m_Content.m_SubBlockInfo = query.m_Reader.BlocksInfo(query.m_Var, ts);
	   return;
	 }

	 m_Content.m_SubBlockInfo.clear();
	 
	 adios2::Variable<T> minmax = m_IdxIO.InquireVariable<T>(query.m_Var.Name()+"_minmax");
	 minmax.SetStepSelection({ts, 1});
	 
	 size_t numBlocks = ((minmax.Shape())[0])/2;
	 
	 std::vector<T> mm(numBlocks * 2); 
	 m_IdxReader.Get<T>(minmax, mm, adios2::Mode::Sync);
	 std::string type = m_IdxIO.VariableType(query.m_Var.Name()+"_start");
	 // doing this due to size_t is saved to unsigned long long though supposed to be unsigned long
#define declare_type(X)							\
	 if (type == adios2::GetType<X>())				\
	   {								\
	     adios2::Variable<X> starts = m_IdxIO.InquireVariable<X>(query.m_Var.Name()+"_start"); \
	     adios2::Variable<X> counts = m_IdxIO.InquireVariable<X>(query.m_Var.Name()+"_count"); \
	     size_t ndim = ((starts.Shape())[0])/numBlocks;		\
	     std::vector<X> ss(numBlocks * ndim);			\
	     std::vector<X> cc(numBlocks * ndim);			\
	     m_IdxReader.Get<X> (starts, ss,  adios2::Mode::Sync);	\
	     m_IdxReader.Get<X> (counts, cc,  adios2::Mode::Sync);	\
	     for (size_t i=0; i<numBlocks; i++) {			\
	       typename Variable<T>::Info blockInfo;			\
	       blockInfo.Start = {ss[i*ndim], ss[i*ndim+1]};		\
	       blockInfo.Count = {cc[i*ndim], cc[i*ndim+1]};		\
	       blockInfo.Min = mm[2*i];					\
	       blockInfo.Max = mm[2*i+1];				\
	       adios2::Dims selStart = query.m_BoundaryStart;		\
	       adios2::Dims selCount = query.m_BoundaryCount;		\
	       if (isBlockValid(blockInfo.Start, blockInfo.Count, selStart, selCount)) \
		 m_Content.m_SubBlockInfo .push_back(blockInfo);	\
	     }								\
	   }
	 ADIOS2_FOREACH_SIZE_T_1ARG(declare_type)
#undef declare_type

       }


       template<class T>
       bool BlockIndex<T>::isBlockValid(adios2::Dims& blockStart, adios2::Dims& blockCount, 
			                adios2::Dims& selStart, adios2::Dims& selCount)
       {
	 if (0 == selStart.size()) {
	   return true;
	 }
	 const size_t dimensionsSize = blockStart.size();
	 
	 for (size_t i=0; i<dimensionsSize; i++) {
	   size_t blockEnd = blockStart[i] + blockCount[i];
	   size_t selEnd   = selStart[i] + selCount[i];

	   if (blockEnd < selStart[i]) 
	     return false;
	   if (selEnd < blockStart[i])
	     return false;
	 }

	 return true;
       }

       template<class T>
       void BlockIndex<T>::WalkThrough(const Query<T>& query, std::vector<unsigned int>& results)
       {
	 int size = m_Content.m_SubBlockInfo.size();

	 unsigned int counter = 0;
	 for (auto &blockInfo : m_Content.m_SubBlockInfo) 
	   {
	     T min = blockInfo.Min;
	     T max = blockInfo.Max;	     

	     bool isHit = query.m_RangeTree.checkInterval(min, max);
	     if (isHit) 
	       results.push_back(counter);

	     counter ++;
	  }
       }



       template <class T>
     void BlockIndexTool<T>::Process(const Query<T>& query, int ts, 
     	  			     std::vector<T> &dataV,
				     std::vector<adios2::Dims>& posV)
     {
       Variable<T>& var = query.m_Var;
              
       m_Tool->SetContent(query, ts);

       std::vector<unsigned int> results; 
       m_Tool->WalkThrough(query, results);
      
       if (m_Rank == 0)
	 std::cout<<"[ts] = "<<ts<<" blocks touched:"<<results.size()<<std::endl;
	 
       int blockStart = m_Rank, numBlocks = 1;

       if (m_Size == 1) {
	 numBlocks = results.size();
       } else if (results.size() < m_Size) {	 
	 if (m_Rank >= results.size()) 
	   numBlocks = 0;
       } else {
	 int residue = results.size() % m_Size;
	 if (0 == residue) {
	   numBlocks = results.size()/m_Size;
	   blockStart = m_Rank* numBlocks;
	 } else {
	   numBlocks = (results.size() - residue)/(m_Size-1);
	   blockStart = m_Rank * numBlocks;
	   if (m_Rank == m_Size - 1) {
	     numBlocks = results.size() - numBlocks * m_Rank;
	   }
	 }
       }
       
       size_t total_local = 0;
       for (int i=0; i<numBlocks; i++) {
            auto block = m_Tool->m_Content.m_SubBlockInfo[results[i+blockStart]];
	   query.m_Var.SetSelection({block.Start, block.Count});
	   query.m_Var.SetStepSelection({ts, 1});

	   if (0 == query.m_Var.SelectionSize()) {
	     continue;
	   }

	   std::vector<T> batch(query.m_Var.SelectionSize());	   
	   query.m_Reader.Get (query.m_Var, batch.data(), adios2::Mode::Sync);
	   
	   size_t counter = 0;

	    //for (auto pointValue: batch) {
	    for (auto i=0; i<batch.size(); i++) {
	     T& pointValue = batch[i];
	     if (query.m_RangeTree.check(pointValue)) {
	       dataV.push_back(pointValue);

	       adios2::Dims loc = getCoordinate(block.Start, block.Count, i);      
	       posV.push_back(loc);

	       counter ++;			
	     }
	   }
	   if (m_Size == 1)
	     std::cout<<"\t"<<block.Min<<" -> "<<block.Max<<"  size = "<<batch.size()<<" hits="<<counter<<std::endl;

	   total_local += counter;
       }

       size_t  global_sum;
       MPI_Reduce(&total_local, &global_sum, 1, MPI_LONG, MPI_SUM, 0, m_Comm);

       if (m_Rank == 0) 
	 std::cout<<" total = "<<global_sum<<std::endl;

       return;

     }


}

#endif