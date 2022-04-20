#include "Worker.h"
#include "adios2/helper/adiosLog.h"

#include <nlohmann_json.hpp>

#include "muparser/include/muParser.h"

namespace adios2
{
namespace query
{
namespace JsonUtil
{
bool HasEntry(nlohmann::json &jsonO, const char *name)
{
  // test holder
    int countMe = jsonO.count(name);
    if (countMe == 0)
        return false;
    return true;
}

void ConstructTree(adios2::query::RangeTree &host, nlohmann::json &opO)
{
    if (!HasEntry(opO, m_TermValue))
        return;
    auto relationStr = opO[m_TermValue];
    host.SetRelation(adios2::query::strToRelation(relationStr));

    if (HasEntry(opO, m_TermRange))
    {
        auto const rangeOs = opO[m_TermRange];
        for (auto r : rangeOs)
        {
            std::string valStr = r[m_TermValue];
            std::string opStr = r[m_TermCompare];
            host.AddLeaf(adios2::query::strToQueryOp(opStr), valStr);
        }
    }
    if (HasEntry(opO, m_TermOp))
    {
        auto subOpO = opO[m_TermOp];
        if (subOpO.is_array())
        {
            for (auto sub : subOpO)
            {
                adios2::query::RangeTree subNode;
                ConstructTree(subNode, sub);
                host.AddNode(subNode);
            }
        }
        else
        {
            adios2::query::RangeTree subNode;
            ConstructTree(subNode, subOpO);
            host.AddNode(subNode);
        }
    }
} // construct tree

void LoadVarQuery(QueryVar *q, nlohmann::json &varO)
{
    if (!adios2::query::JsonUtil::HasEntry(varO, m_TermOp))
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::JsonWorker", "LoadVarQuery",
            "No op entry specified for var:" + q->m_VarName);

    if (adios2::query::JsonUtil::HasEntry(varO, m_TermBB))
    {
        auto bbO = varO[m_TermBB];
        q->LoadSelection(bbO[m_TermStart], bbO[m_TermCount]);
    }
    if (adios2::query::JsonUtil::HasEntry(varO, m_TermOp))
    {
        auto opO = varO[m_TermOp];
        adios2::query::JsonUtil::ConstructTree(q->m_RangeTree, opO);
    }
} // LoadVarQuery
}
}
}

namespace adios2
{
namespace query
{
void JsonWorker::ParseJson()
{
    // local functions:

    auto lf_assertEntry = [&](nlohmann::json &jsonO,
			      const char*  name,
			      const std::string& parentName) -> void {
      if (!adios2::query::JsonUtil::HasEntry(jsonO, name)) {
	std::string nameStr (name);
	helper::Throw<std::ios_base::failure>("Toolkit", "query::JsonWorker", "ParseJson",
					      parentName+" has missing entry: "+nameStr);
      }

    };
    auto lf_assertArray = [&](nlohmann::json &jsonO,
                              const std::string &name,
			      const std::string &parentName) -> void {
        if (!jsonO.is_array())
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "query::JsonWorker", "ParseJson",
                "Node["+parentName+"] is expecting Array for entry:" + name);
    }; // lf assert

    auto lf_parseVar = [&](nlohmann::json &varO) -> QueryVar * {
        lf_assertEntry(varO, m_TermName, m_TermVar);
        auto varName = (varO)[m_TermName];
	adios2::core::IO &currIO = m_SourceReader->m_IO;

	if (adios2::query::JsonUtil::HasEntry(varO, m_TermDerived))
	  {
	    auto derivedO = varO[m_TermDerived];
	    lf_assertEntry(derivedO, m_TermFormula, m_TermDerived);
	    lf_assertEntry(derivedO, m_TermVar, m_TermDerived);
	    
	    auto formula = derivedO [m_TermFormula];	    
	    auto varList = derivedO [m_TermVar];
	    lf_assertArray(varList, m_TermVar, m_TermDerived);

	    QueryDerived *derivedQ = new QueryDerived(varName, formula);
	    for (auto var : varList)
	    {
	      std::string name = var[m_TermName];	      
	      std::string pathStr = var[m_TermPath];

	      const DataType varType = currIO.InquireVariableType(pathStr);
	
	      if ((varType == DataType::None) || (!derivedQ->SetType(varType)))
		{
		  helper::Throw<std::ios_base::failure>
		    ("Toolkit", "query::XmlWorker", "ParseVarNode",	       
		     "No such variable => " + pathStr,
		     helper::LogMode::ERROR);
		}
       
	      derivedQ->SetType(varType);
	      derivedQ->Add(name, pathStr);
	    }
	    derivedQ->Access();
	    adios2::query::JsonUtil::LoadVarQuery(derivedQ, varO);
	    return derivedQ;
	  }
	else
	  {
	    QueryVar *simpleQ = GetBasicVarQuery(currIO, varName);
	    if (simpleQ)
	      adios2::query::JsonUtil::LoadVarQuery(simpleQ, varO);
	    return simpleQ;
	  }
    }; // local function to Parse var

    auto lf_parseTag = [&](nlohmann::json &tagO) -> QueryBase * {
        if (adios2::query::JsonUtil::HasEntry(tagO, m_TermVar))
            return lf_parseVar(tagO[m_TermVar]);
        return nullptr;
    };

    std::ifstream fileStream(m_QueryFile);
    nlohmann::json jsonObj = nlohmann::json::parse(fileStream);

    if (!adios2::query::JsonUtil::HasEntry(jsonObj, m_TermIO))
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::JsonWorker", "ParseJson",
            "No io node in json query file. Expecting the io node: " +
                m_SourceReader->m_IO.m_Name);
    }

    auto ioO = jsonObj.find(m_TermIO);
    std::string const ioName = (*ioO)[m_TermName];
    if (m_SourceReader->m_IO.m_Name.compare(ioName) != 0)
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::JsonWorker", "ParseJson",
            "invalid query io. Expecting io name = " +
                m_SourceReader->m_IO.m_Name);
    if (adios2::query::JsonUtil::HasEntry(*ioO, m_TermVar))
    {
        auto varO = (*ioO).find(m_TermVar);
        m_Query = lf_parseVar(*varO);
        m_Query->Print();
        return;
    }
    if (!adios2::query::JsonUtil::HasEntry(*ioO, m_TermQuery))
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::JsonWorker", "ParseJson",
            "no query entry was defined for composite query");
    auto queryO = (*ioO)[m_TermQuery];
    auto relationO = queryO[m_TermOp];
    QueryComposite *result =
        new QueryComposite(adios2::query::strToRelation(relationO));

    auto tagO = (*ioO)[m_TermTag];
    lf_assertArray(tagO, m_TermTag, m_TermIO);
    std::map<std::string, QueryBase *> subqueries;

    for (auto tag : tagO)
    {
        QueryBase *q = lf_parseTag(tag);
        subqueries[tag[m_TermName]] = q;
    }

    auto compO = queryO[m_TermComp];
    lf_assertArray(compO, m_TermComp, m_TermQuery);

    for (auto qname : compO)
    {
        std::cout << qname << std::endl;
        result->AddNode(subqueries[qname]);
    }

    m_Query = result;
    return;
} // parse
}
}
