#include "Worker.h"

#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosXMLUtil.h"

#include <pugixml.hpp>

namespace adios2
{
namespace query
{
void XmlWorker::ParseMe()
{
    auto lf_FileContents = [&](const std::string &configXML) -> std::string {
        std::ifstream fileStream(configXML);
        if (!fileStream)
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "query::XmlWorker", "ParseMe",
                "file " + configXML + " not found");
        }
        std::ostringstream fileSS;
        fileSS << fileStream.rdbuf();
        fileStream.close();

        if (fileSS.str().empty())
        {
            helper::Throw<std::invalid_argument>("Toolkit", "query::XmlWorker",
                                                 "ParseMe",
                                                 "config xml file is empty");
        }

        return fileSS.str();
    }; // local  function lf_FileContents

    const std::string fileContents = lf_FileContents(m_QueryFile);
    const std::unique_ptr<pugi::xml_document> document =
        adios2::helper::XMLDocument(fileContents, "in Query XMLWorker");

    const std::unique_ptr<pugi::xml_node> config = adios2::helper::XMLNode(
        m_TermADIOSQuery, *document, "in adios-query", true);

    const pugi::xml_node ioNode = config->child(adios2::query::m_TermIO);
    ParseIONode(ioNode);

} // Parse()

void XmlWorker::ParseIONode(const pugi::xml_node &ioNode)
{
    const std::unique_ptr<pugi::xml_attribute> ioName =
        adios2::helper::XMLAttribute(m_TermName, ioNode, "in query");
    if (m_SourceReader->m_IO.m_Name.compare(ioName->value()) != 0)
    {
        helper::Throw<std::ios_base::failure>(
            "Toolkit", "query::XmlWorker", "ParseIONode",
            "invalid query io. Expecting io name = " +
                m_SourceReader->m_IO.m_Name + " found:" + ioName->value());
    }

    std::map<std::string, QueryBase *> subqueries;

    adios2::Box<adios2::Dims> ref;
    for (const pugi::xml_node &qTagNode : ioNode.children(m_TermTag))
    {
        const std::unique_ptr<pugi::xml_attribute> name =
            adios2::helper::XMLAttribute(m_TermName, qTagNode, "in query");
        const pugi::xml_node &variable = qTagNode.child(m_TermVar);
        QueryVar *q =
            ParseVarNode(variable, m_SourceReader->m_IO, *m_SourceReader);
        if (!q)
            continue;

        if (ref.first.size() == 0)
        {
            ref = q->m_Selection;
        }
        else if (!q->IsCompatible(ref))
        {
            helper::Throw<std::ios_base::failure>(
                "Toolkit", "query::XmlWorker", "ParseIONode",
                "impactible query found on var:" + q->GetVarName());
        }
        subqueries[name->value()] = q;
    }

    const pugi::xml_node &qNode = ioNode.child(m_TermQuery);
    if (qNode == nullptr)
    {
        const pugi::xml_node &variable = ioNode.child(m_TermVar);
        m_Query = ParseVarNode(variable, m_SourceReader->m_IO, *m_SourceReader);
    }
    else
    {
        const std::unique_ptr<pugi::xml_attribute> op =
            adios2::helper::XMLAttribute(m_TermOp, qNode, "in query");
        QueryComposite *q =
            new QueryComposite(adios2::query::strToRelation(op->value()));
        for (const pugi::xml_node &sub : qNode.children())
        {
            q->AddNode(subqueries[sub.name()]);
        }
        m_Query = q;
    }
} // parse_io_node

// node is the variable node
QueryVar *XmlWorker::ParseVarNode(const pugi::xml_node &node,
                                  adios2::core::IO &currentIO,
                                  adios2::core::Engine &reader)

{
    const std::string variableName = std::string(
        adios2::helper::XMLAttribute(m_TermName, node, "in query")->value());

    pugi::xml_node derivedNode = node.child(m_TermDerived);
    if (derivedNode) {
      const std::string formula =
	std::string(adios2::helper::XMLAttribute(m_TermFormula, derivedNode, "in query")->value());

      if (formula.size() < 0)
	helper::Throw<std::ios_base::failure>(
                "Toolkit", "query::XmlWorker", "ParseVarNode",
                "formula needs to be defined for derived variables!");

      QueryDerived *derivedQ = new QueryDerived(variableName, formula);
      for (const pugi::xml_node &varNode : derivedNode.children(m_TermVar))
      {
        std::string name =
	  adios2::helper::XMLAttribute(m_TermName, varNode, "in query")->value();

        std::string pathStr =
	  adios2::helper::XMLAttribute(m_TermPath, varNode, "in query")->value();

	const DataType varType = currentIO.InquireVariableType(pathStr);

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
      ConstructQuery(*derivedQ, node);
      return derivedQ;
    } else {
      QueryVar *simpleQ = GetBasicVarQuery(currentIO, variableName);
      if (simpleQ)
	ConstructQuery(*simpleQ, node);
      return simpleQ;
    }
} //  parse_var_node


void XmlWorker::ConstructTree(RangeTree &host, const pugi::xml_node &node)
{
    std::string relationStr =
        adios2::helper::XMLAttribute(m_TermValue, node, "in query")->value();
    host.SetRelation(adios2::query::strToRelation(relationStr));
    for (const pugi::xml_node &rangeNode : node.children(m_TermRange))
    {
        std::string valStr =
            adios2::helper::XMLAttribute(m_TermValue, rangeNode, "in query")
                ->value();
        std::string opStr =
            adios2::helper::XMLAttribute(m_TermCompare, rangeNode, "in query")
                ->value();

        host.AddLeaf(adios2::query::strToQueryOp(opStr), valStr);
    }

    for (const pugi::xml_node &opNode : node.children(m_TermOp))
    {
        adios2::query::RangeTree subNode;
        ConstructTree(subNode, opNode);
        host.AddNode(subNode);
    }
}

void XmlWorker::ConstructQuery(QueryVar &simpleQ, const pugi::xml_node &node)
{
    // QueryVar* simpleQ = new QueryVar(variableName);
    pugi::xml_node bbNode = node.child(m_TermBB);

    if (bbNode)
    {
        std::string startStr =
            adios2::helper::XMLAttribute(m_TermStart, bbNode, "in query")->value();
        std::string countStr =
            adios2::helper::XMLAttribute(m_TermCount, bbNode, "in query")->value();

	simpleQ.LoadSelection(startStr, countStr);
    }

    pugi::xml_node relationNode = node.child(m_TermOp);
    ConstructTree(simpleQ.m_RangeTree, relationNode);
}

} // namespace query
} // namespace adios2
