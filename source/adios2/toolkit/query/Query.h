#ifndef ADIOS2_QUERY_H
#define ADIOS2_QUERY_H

#include <ios>      //std::ios_base::failure
#include <iostream> //std::cout

#include <numeric>   // accumulate
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>

//#include "adios2.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "muparser/include/muParser.h"
namespace adios2
{
namespace query
{
enum Op
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

  static const char* m_TermADIOSQuery="adios-query";
  static const char* m_TermIO="io";
  static const char* m_TermQuery="query";

  static const char* m_TermVar="var";
  static const char* m_TermTag="tag";
  static const char* m_TermDerived="derived";
  static const char* m_TermFormula="formula";

  static const char* m_TermOp="op";
  static const char* m_TermName="name";
  static const char* m_TermPath="path";

  static const char* m_TermRange="range";
  static const char* m_TermCompare="compare";
  static const char* m_TermValue="value";
  static const char* m_TermBB="boundingbox";
  static const char* m_TermStart="start";
  static const char* m_TermCount="count";
  static const char* m_TermComp="comp"; // component


adios2::query::Relation strToRelation(std::string relationStr) noexcept;

adios2::query::Op strToQueryOp(std::string opStr) noexcept;

adios2::Dims split(const std::string &s, char delim);

//
// classes
//
class Range
{
public:
    adios2::query::Op m_Op;
    std::string m_StrValue;
    // void* m_Value = nullptr;

    // template<class T> bool Check(T val) const ;

    template <class T>
    bool CheckInterval(T &min, T &max) const;

    void Print() { std::cout << "===> " << m_StrValue << std::endl; }
}; // class Range

class RangeTree
{
public:
    void AddLeaf(adios2::query::Op op, std::string value)
    {
        Range range;
        range.m_Op = op;
        range.m_StrValue = value;

        m_Leaves.push_back(range);
    }

    void AddNode(RangeTree &node) { m_SubNodes.push_back(node); }

    void SetRelation(adios2::query::Relation r) { m_Relation = r; }

    void Print()
    {
        for (auto leaf : m_Leaves)
            leaf.Print();
        for (auto node : m_SubNodes)
            node.Print();
    }

    // template<class T>  bool Check(T value) const ;

    template <class T>
    bool CheckInterval(T &min, T &max) const;

    adios2::query::Relation m_Relation = adios2::query::Relation::AND;
    std::vector<Range> m_Leaves;
    std::vector<RangeTree> m_SubNodes;
}; // class RangeTree

class QueryBase
{
public:
    virtual ~QueryBase(){};
    virtual bool IsCompatible(const adios2::Box<adios2::Dims> &box) = 0;
    virtual void Print() = 0;
    virtual void BlockIndexEvaluate(adios2::core::IO &, adios2::core::Engine &,
                                    std::vector<Box<Dims>> &touchedBlocks) = 0;

    Box<Dims> GetIntersection(const Box<Dims> &box1,
                              const Box<Dims> &box2) noexcept
    {
        Box<Dims> b1 = adios2::helper::StartEndBox(box1.first, box1.second);
        Box<Dims> b2 = adios2::helper::StartEndBox(box2.first, box2.second);

        Box<Dims> result = adios2::helper::IntersectionBox(b1, b2);
        return adios2::helper::StartCountBox(result.first, result.second);
    }

    bool UseOutputRegion(const adios2::Box<adios2::Dims> &region)
    {
        if (!IsCompatible(region))
            return false;

        m_OutputRegion = region;
        BroadcastOutputRegion(region);
        return true;
    }

    virtual void
    BroadcastOutputRegion(const adios2::Box<adios2::Dims> &region) = 0;

    void ApplyOutputRegion(std::vector<Box<Dims>> &touchedBlocks,
                           const adios2::Box<Dims> &referenceRegion);

    adios2::Box<adios2::Dims> m_OutputRegion;

private:
    // bool ResetToOutputRegion(Box<Dims>& block);
};

class QueryVar : public QueryBase
{
public:
    QueryVar(const std::string &varName) : m_VarName(varName) {}
    ~QueryVar() {}

    std::string &GetVarName() { return m_VarName; }
    void BlockIndexEvaluate(adios2::core::IO &, adios2::core::Engine &,
                            std::vector<Box<Dims>> &touchedBlocks);
    void BroadcastOutputRegion(const adios2::Box<adios2::Dims> &region)
    {
        m_OutputRegion = region;
    }

    void Print() { m_RangeTree.Print(); }

    bool IsCompatible(const adios2::Box<adios2::Dims> &box)
    {
        if ((m_Selection.first.size() == 0) || (box.first.size() == 0))
            return true;

        if (box.first.size() != m_Selection.first.size())
            return false;

        for (size_t n = 0; n < box.second.size(); n++)
            if (box.second[n] != m_Selection.second[n])
                return false;

        return true;
    }

    void SetSelection(adios2::Dims &start, adios2::Dims &count)
    {
        m_Selection.first = start;
        m_Selection.second = count;
    }

    bool IsSelectionValid(adios2::Dims &varShape) const;

    bool TouchSelection(adios2::Dims &start, adios2::Dims &count) const;

    void LoadSelection(const std::string &startStr,
                       const std::string &countStr);

    void LimitToSelection(std::vector<Box<Dims>> &touchedBlocks)
    {
        for (auto it = touchedBlocks.begin(); it != touchedBlocks.end(); it++)
        {
            Box<Dims> overlap = GetIntersection(m_Selection, *it);
            // adios2::helper::IntersectionBox(m_Selection, *it);
            it->first = overlap.first;
            it->second = overlap.second;
        }
    }

    RangeTree m_RangeTree;
    adios2::Box<adios2::Dims> m_Selection;

    std::string m_VarName;

private:
}; // class QueryVar


//
//  Processing a variable derived from actual adios vars
//      e.g. if "x" "y" are adios vars, then
//        "radius"=sqrt(x**2 + y**2)
//       defines a derived variable on top of x & y
//
//  Assumption in order for derived variable to work with block index
//     - all the member variables in formula have the same blocks structures
//        i.e. for each step, there are consuimg the same blocks
//     - formula needs be either monotonic
//       or bell shaped around the original point. e.g. x**2
//       This way it is convenient to compute stats for the derived variables
//       by appling formula on the max/min/original_point of the underlying variables
//
  class QueryDerived: public QueryVar
{
public:
  QueryDerived(const std::string &derivedVarName, const std::string& formula)
    : QueryVar(derivedVarName), m_Formula(formula)
  {
    m_MuParser.SetExpr(formula);
  }

  ~QueryDerived() {}

  void BlockIndexEvaluate(adios2::core::IO &, adios2::core::Engine &,
			  std::vector<Box<Dims>> &touchedBlocks);

  void Add(std::string name, std::string varPath)
  {
    m_VarRef[name] = varPath;
  }

  bool SetType(DataType d);

  template<typename T>
  void
  Update(adios2::core::IO& io, adios2::core::Engine & reader,
	 std::vector<typename adios2::core::Variable<double>::BPInfo>& currStepBlocksInfo)
  {
    core::Variable<double> *var = io.InquireVariable<double>(m_VarName);
    core::Variable<T> *first = io.InquireVariable<T>(m_VarRef.begin()->second);

    /*
    if (m_Selection.first.size() == 0) {
      adios2::Dims zero(first->Shape().size(), 0);
      adios2::Dims shape = first->Shape();
      std::cout<<"  2 no skip  "<<m_VarRef.begin()->second<<" "<<first->Shape()<<std::endl;
      //SetSelection(zero, shape);
    }
    */

    size_t currStep = reader.CurrentStep();

    std::vector<std::vector<std::pair<double, double>>>  minmax;

    for (auto const& vr : m_VarRef) {
      core::Variable<T> * v = io.InquireVariable<T>(vr.second);
      std::vector<typename adios2::core::Variable<T>::BPInfo> varBlocksInfo = reader.BlocksInfo(*v, currStep);
      if (minmax.size() == 0)
	minmax.resize(varBlocksInfo.size());

      if (varBlocksInfo[0].MinMaxs.size() > 0)
	helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::QueryDeerived", "Update",
            "Currently no support for query on derived variables with sub block level stats");

      for (int i=0; i<varBlocksInfo.size(); i++) {
	minmax[i].push_back({(double)varBlocksInfo[i].Min, (double)varBlocksInfo[i].Max});
      }
    }

    std::vector<typename adios2::core::Variable<T>::BPInfo> varBlocksInfo =
      reader.BlocksInfo(*first, currStep);

    currStepBlocksInfo = std::vector<typename adios2::core::Variable<double>::BPInfo>(varBlocksInfo.size());

    for (int i=0; i<varBlocksInfo.size(); i++) {
      currStepBlocksInfo[i].Shape = varBlocksInfo[i].Shape;
      currStepBlocksInfo[i].Start = varBlocksInfo[i].Start;
      currStepBlocksInfo[i].Count = varBlocksInfo[i].Count;
      UpdateMinMax(minmax[i], currStepBlocksInfo[i]);
    }
  }

void UpdateMinMax(std::vector<std::pair<double, double> >& input,
		  typename adios2::core::Variable<double>::BPInfo& out);

void Access();

private:
  std::string m_Formula;
  std::map<std::string, std::string> m_VarRef;

  DataType m_DataType = DataType::None;

  mu::Parser m_MuParser;
  bool m_IsMonotonic = false;

}; // class QueryDerived



class QueryComposite : public QueryBase
{
public:
    QueryComposite(adios2::query::Relation relation) : m_Relation(relation) {}
    ~QueryComposite()
    {
        for (auto n : m_Nodes)
            delete n;
        m_Nodes.clear();
    }

    void BroadcastOutputRegion(const adios2::Box<adios2::Dims> &region)
    {
        if (m_Nodes.size() == 0)
            return;

        for (auto n : m_Nodes)
            n->BroadcastOutputRegion(region);
    }

    void BlockIndexEvaluate(adios2::core::IO &, adios2::core::Engine &,
                            std::vector<Box<Dims>> &touchedBlocks);

    bool AddNode(QueryBase *v);

    void Print()
    {
        std::cout << " Composite query" << std::endl;
        for (auto n : m_Nodes)
            n->Print();
    }

    bool IsCompatible(const adios2::Box<adios2::Dims> &box)
    {
        if (m_Nodes.size() == 0)
            return true;
        return (m_Nodes[0])->IsCompatible(box);
    }

private:
    adios2::query::Relation m_Relation = adios2::query::Relation::AND;

    std::vector<QueryBase *> m_Nodes;
}; // class QueryComposite

/*
 */

} // namespace query
} //  namespace adiso2

#endif // define
