#ifndef ADIOS2_QUERY_H
#define ADIOS2_QUERY_H

 #include <ios>      //std::ios_base::failure
 #include <iostream> //std::cout
 #include <mpi.h>
 #include <stdexcept> //std::invalid_argument std::exception
 #include <vector>
 #include <numeric>   // accumulate

 #include <adios2.h>


// this is for the idx file variables... all int
#define ADIOS2_FOREACH_SIZE_T_1ARG(MACRO)				\
  MACRO(unsigned int)							\
  MACRO(unsigned long int)						\
  MACRO(unsigned long long int)						\


 namespace adios2 
 {
   enum QueryOp
   {
     GT,
     LT,
     GE,
     LE,
     NE,
     EQ
   };

   enum Relation
   {
     AND,
     OR,
     NOT
   };


   static adios2::Relation strToRelation(std::string relationStr) noexcept
     {
       if ((relationStr.compare("or") == 0) || (relationStr.compare("OR") == 0)) 
	 return adios2::Relation::OR;    
       
       return adios2::Relation::AND; // default
     }

   static adios2::QueryOp strToQueryOp(std::string opStr) noexcept
   {
     if ((opStr.compare("lt") == 0) || (opStr.compare("LT") == 0))
       return adios2::QueryOp::LT;
     if ((opStr.compare("gt") == 0) || (opStr.compare("GT") == 0))
       return adios2::QueryOp::GT;
     if ((opStr.compare("ge") == 0) || (opStr.compare("GE") == 0))
       return adios2::QueryOp::GE;
     if ((opStr.compare("le") == 0) || (opStr.compare("LE") == 0))
       return adios2::QueryOp::LE;
     if ((opStr.compare("eq") == 0) || (opStr.compare("EQ") == 0))
       return adios2::QueryOp::EQ;
     if ((opStr.compare("ne") == 0) || (opStr.compare("NE") == 0))
       return adios2::QueryOp::NE;
     
     return adios2::QueryOp::EQ; // default
   }


   template <class T>
     class Range 
       {
       public:
	 adios2::QueryOp m_Op;
	 T m_Value;

	 bool check(T val) const ;
	 /*
	 */

	 bool checkInterval (T& min, T& max) const;

	 void print(std::string& varName, int level);

       }; // class Range



   template <class T>
     class RangeTree
     {
     public:
       void addRange(adios2::QueryOp op, T value)
       {
	 Range<T> range;
	 range.m_Op = op;
	 range.m_Value = value;

	 m_Leaves.push_back(range);
       }

       void addNode(RangeTree<T>& node)
       {
	 m_SubNodes.push_back(node);
       }

       void setRelation(adios2::Relation r)
       {
	 m_Relation = r;
       }

       void print(std::string& varName, int level);

       bool check(T value) const ;

       bool checkInterval (T& min, T& max) const;

       adios2::Relation m_Relation = adios2::Relation::AND;
       std::vector<Range<T>> m_Leaves;
       std::vector<RangeTree<T>> m_SubNodes;
     }; // class RangeTree




   template <class T>
     class Query
     {
     public:
       Query<T>(adios2::Engine& readEngine, Variable<T>& var);
       ~Query<T>() = default;

       void addRange(adios2::QueryOp, T value);

       void  print();

       void setSelection(adios2::Dims& start, adios2::Dims& count);

     //private:
       adios2::Variable<T>&  m_Var;
       adios2::Engine& m_Reader;
       //Range<T> m_Range;
       RangeTree<T> m_RangeTree;

       adios2::Dims m_BoundaryStart;
       adios2::Dims m_BoundaryCount;
       size_t  m_timestepStart = 0;
       size_t  m_timestepCount = 0;
     }; // class Query




   class BlockIndexBuilder
   {
   public:
     BlockIndexBuilder(std::string& idxFileName, MPI_Comm comm, bool overwrite=false);

     void GenerateIndexFrom(adios2::IO& dataIO, 
			    adios2::IO& idxIO, 
			    adios2::Engine& dataReader, size_t unitElement);

     bool divideMe(adios2::Dims& varShape, size_t suggestedBlockSize, adios2::Dims& divider);
     void GetIdx(const adios2::Dims& scope, int pos, adios2::Dims& result) ;

     template <class T> 
       int GetBlocks(adios2::Engine& dataReader, adios2::Dims & dimDivider, Variable<T> var, 
		     std::vector<typename Variable<T>::Info>& blocksInfo)  
       {	
	 size_t nboxes = GetSize(dimDivider);

	 if (m_Rank > nboxes-1) 
	   return -1;

	 int loadPerRank = 1;
	 if (nboxes > m_Size) {
	   loadPerRank = nboxes/m_Size;
	 }

	 int loadStart = m_Rank*loadPerRank; // start idx of this box;

	 if (m_Rank == m_Size-1) 
	   loadPerRank = nboxes - loadStart;

	 blocksInfo.reserve(loadPerRank);

	 adios2::Dims idx(dimDivider.size(), 0);
	 //idx.reserve(dimDivider.size());
	 for (auto blockCounter = 0; blockCounter< nboxes; blockCounter ++)
	 {
	     
	   if ((blockCounter >= loadStart)  && (blockCounter < loadStart + loadPerRank))
	     {
	       GetIdx(dimDivider, blockCounter, idx);

	       typename Variable<T>::Info currBlock;
	       ComputeBlock(dataReader, var, dimDivider, idx, currBlock);

	       blocksInfo.push_back(currBlock);		 
	     }
	 }       

	 return loadStart;
       }


     template <class T>
       void ComputeBlock(adios2::Engine& dataReader, Variable<T> var, const adios2::Dims& scope, adios2::Dims& idx, typename Variable<T>::Info& currBlock)
       {
	 int ndim = scope.size();
	 currBlock.Start.assign(ndim, 0);
	 currBlock.Count.assign(ndim, 0);


	 for (auto i=0; i<ndim; i++)
	   {
	     auto unit = var.Shape()[i]/scope[i];
	     currBlock.Start[i] = idx[i] * unit;
	     currBlock.Count[i] = unit;
	   }

	 std::vector<T> data; 
	 data.reserve(GetSize(currBlock.Count));
	 var.SetSelection({currBlock.Start, currBlock.Count});
	 dataReader.Get<T>(var, data, adios2::Mode::Sync);
	 
	 auto bounds = std::minmax_element(data.data(), data.data() + data.size());
	 currBlock.Min = *bounds.first;
	 currBlock.Max = *bounds.second;

       } 


     template <class T>
       void handleVar(adios2::IO& dataIO,
		      adios2::Variable<T> var, 
		      adios2::Engine& dataReader,
		      adios2::Engine& idxWriter,
		      size_t suggestedBlockSize) 
       {
	 adios2::Dims dimDivider(var.Shape().size());
	 adios2::Dims varShape = var.Shape();
	 if (!divideMe(varShape, suggestedBlockSize, dimDivider)) {
	   return;
	 }

	 std::string name = var.Name();

	 size_t nboxes = GetSize(dimDivider);
	 if (m_Rank > nboxes) {
	   return;
	 }

	 size_t ndim = varShape.size();

	 //calculate how many boxes per rank..
	 std::vector<typename Variable<T>::Info> blocks;
	 int startBlock  = GetBlocks(dataReader, dimDivider, var, blocks);
	 if (startBlock < 0) return;

	 size_t  numBlocks = blocks.size();


	 if (var.StepsStart() == dataReader.CurrentStep()) 
	   {
	     auto mm = dataIO.DefineVariable<T>(name+"_minmax", {nboxes*2}, {(size_t)2*startBlock}, {2*numBlocks}); 
	     auto boxStart = dataIO.DefineVariable<size_t>(name+"_start", {nboxes*ndim}, {startBlock * ndim}, {ndim * numBlocks}); 
	     auto boxCount = dataIO.DefineVariable<size_t>(name+"_count", {nboxes*ndim}, {startBlock * ndim}, {ndim * numBlocks}); 
	   }
	 {
	     auto mm = dataIO.InquireVariable<T>(name+"_minmax");	
	     auto boxStart=dataIO.InquireVariable<size_t>(name+"_start"); 
	     auto boxCount=dataIO.InquireVariable<size_t>(name+"_count"); 

	     std::vector<T> minmax;
	     std::vector<size_t> starts, counts;
	     for (int i=0; i<numBlocks; i++) {
	       minmax.push_back(blocks[i].Min); minmax.push_back(blocks[i].Max);	       
	       starts.insert(starts.end(), blocks[i].Start.begin(), blocks[i].Start.end());
	       counts.insert(counts.end(), blocks[i].Count.begin(), blocks[i].Count.end());
	     }
	     idxWriter.Put<T>(mm, minmax.data(), adios2::Mode::Sync);	
	     idxWriter.Put<size_t>(boxStart, starts.data(), adios2::Mode::Sync); 
	     idxWriter.Put<size_t>(boxCount, counts.data(), adios2::Mode::Sync); 
	 }

       }

     size_t GetSize(const adios2::Dims &dimensions) noexcept
     {
       return std::accumulate(dimensions.begin(), dimensions.end(),
			      static_cast<size_t>(1), std::multiplies<size_t>());
     }


     std::string& m_IdxFileName;
     bool m_OverwriteExisting;
     MPI_Comm m_MPIComm;
     bool m_Finished;

     int m_Rank;
     int m_Size;

   }; // class BlockIndexBuilder




   template <class T>
     class BlockIndex
     {
       struct Tree
       {
	 //
	 // ** no need to keep the original block. might be smaller than blockIndex
	 // typename Variable<T>::info& m_BlockInfo;
	 //
	 std::vector<typename Variable<T>::Info> m_SubBlockInfo;
       };


     public:
       BlockIndex<T>(adios2::IO& io, adios2::Engine& idxReader);

       void Init (adios2::Engine& idxReader, const adios2::Variable<T>& var, int ts);
       void Evaluate(const Query<T>& query, std::vector<unsigned int>& resultSubBlocks);

       
       void SetContent(const Query<T>& query, int ts);
       
       bool isBlockValid(adios2::Dims& blockStart, adios2::Dims& blockCount, 
			 adios2::Dims& selStart, adios2::Dims& selCount);

       void WalkThrough(const Query<T>& query, std::vector<unsigned int>& results);



      Tree m_Content;

    private:
      //
      // blockid <=> vector of subcontents
      //
      //std::string& m_DataFileName;
      adios2::Engine& m_IdxReader;
      adios2::IO& m_IdxIO;
     
     }; // class blockIndex
  


  
   //
   //
   //
   template <class T>
   class BlockIndexTool
   {
   public:
     
     BlockIndexTool(BlockIndex<T>* tool, MPI_Comm comm);

     //
     // Process result at a given timestep (ts).
     //
     // -1) read in  all the blocks from idx for the query variable
     // -2) filter out blocks that has no hits
     // -3) read in data from each block that has hits (which is dataV)
     //
     void Process(const Query<T>& query, int ts, 
		  std::vector<T> &dataV,
		  std::vector<adios2::Dims> & posV);
       
     adios2::Dims getCoordinate(const adios2::Dims& blockStart, 
				const adios2::Dims& blockCount, 
				size_t linearPos)
       {
	 size_t pos = linearPos;
	 adios2::Dims result(blockStart.size());

	 for (auto i=blockStart.size()-1; i> 0; i--) {
	   result[i] = pos % blockCount[i];
	   pos = (pos - result[i])/blockCount[i];
	   result[i] += blockStart[i];
	 }
	 result[0] = pos + blockStart[0];

	 return result;
       }


     BlockIndex<T>* m_Tool;
     MPI_Comm m_Comm;
     int m_Rank;
     int m_Size;
   };// end class BlockIndexTool
  
  
 }; //end namespace adios2

#endif
