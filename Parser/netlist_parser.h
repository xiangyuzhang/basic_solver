#ifndef PARSER_NETLIST_PARSER_H
#define PARSER_NETLIST_PARSER_H
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <regex>
#include "utils/dict.h"
#include "utils/tools.h"
class netlist_parser_ABC
{
friend class CNF_handler;
friend class Attack_CNF_handler;
friend class print_solution;
public:
	netlist_parser_ABC() = default;
	netlist_parser_ABC(const std::string);

	virtual ~netlist_parser_ABC() = default;
	virtual void parse_input(std::string);
	virtual void parse_CB(std::string);
	virtual void parse_wire(std::string);
	virtual void parse_output(std::string);
	virtual void parse_gate(std::string);

	virtual void parse();

	virtual void printCNF(std::ostream&);


	std::vector<std::string> find_netname(std::string gate);
	std::string find_gatetype(std::string line);
	std::vector<std::string> split_wire_info(std::string line, const std::string& line_type);
	std::string input_file;

	std::map<std::string, unsigned> PI_name_to_index;
	std::map<std::string, unsigned> PO_name_to_index;
	std::map<std::string, unsigned> CB_name_to_index;
	std::map<std::string, unsigned> wire_name_to_index;
	std::map<std::string, unsigned> varIndexDict;

	std::map<unsigned, std::string> PI_index_to_name;
	std::map<unsigned, std::string> PO_index_to_name;
	std::map<unsigned, std::string> CB_index_to_name;

	std::map<unsigned, std::string> wire_index_to_name;
	std::map<unsigned, std::string> indexVarDict;

	std::vector<std::vector<std::string>> CNF;
	std::vector<std::string> Vline;
	std::map<std::string, int> gateTypeDict;

private:
	unsigned net_counter = 1;
};

//======================================================
// interface 
netlist_parser_ABC::netlist_parser_ABC(const std::string input):input_file(input)
{
	load_gateTypeDict(gateTypeDict);
	SplitString(stripComments(Readall(input_file.c_str())), Vline, ";");
	if(Vline.size() == 0) std::cerr << "netlist_parser_ABC: open input file failed" << std::endl;
}

void netlist_parser_ABC::parse_input(std::string line)
{
	auto net_list = split_wire_info(line, "input");
	for(auto net: net_list)
	{
		strip_all(net, " ");
		strip_all(net, "\t");
		PI_name_to_index.insert(std::pair<std::string, unsigned>(net, net_counter));
		PI_index_to_name.insert(std::pair<unsigned, std::string>(net_counter, net));
		varIndexDict.insert(std::pair<std::string, unsigned>(net, net_counter));
		indexVarDict.insert(std::pair<unsigned, std::string>(net_counter, net));
		++net_counter;
	}
}
void netlist_parser_ABC::parse_CB(std::string line)
{
	auto net_list = split_wire_info(line, "CB");
	for(auto net: net_list)
	{
		strip_all(net, " ");
		strip_all(net, "\t");
		CB_name_to_index.insert(std::pair<std::string, unsigned>(net, net_counter));
		CB_index_to_name.insert(std::pair<unsigned, std::string>(net_counter, net));
		varIndexDict.insert(std::pair<std::string, unsigned>(net, net_counter));
		indexVarDict.insert(std::pair<unsigned, std::string>(net_counter, net));
		++net_counter;
	}
}

void netlist_parser_ABC::parse_wire(std::string line)
{
	auto net_list = split_wire_info(line, "wire");
	for(auto net: net_list)
	{
		strip_all(net, " ");
		strip_all(net, "\t");
		wire_name_to_index.insert(std::pair<std::string, unsigned>(net, net_counter));
		wire_index_to_name.insert(std::pair<unsigned, std::string>(net_counter, net));
		varIndexDict.insert(std::pair<std::string, unsigned>(net, net_counter));
		indexVarDict.insert(std::pair<unsigned, std::string>(net_counter, net));
		++net_counter;
	}
}

void netlist_parser_ABC::parse_output(std::string line)
{
	auto net_list = split_wire_info(line, "output");
	for(auto net: net_list)
	{
		strip_all(net, " ");
		strip_all(net, "\t");
		PO_name_to_index.insert(std::pair<std::string, unsigned>(net, net_counter));
		PO_index_to_name.insert(std::pair<unsigned, std::string>(net_counter, net));
		varIndexDict.insert(std::pair<std::string, unsigned>(net, net_counter));
		indexVarDict.insert(std::pair<unsigned, std::string>(net_counter, net));
		++net_counter;
	}
}

void netlist_parser_ABC::parse_gate(std::string gate)
{
	auto gate_type = find_gatetype(gate);
	auto gate_nets = find_netname(gate);
	std::string gate_output(gate_nets.back());
	std::vector<int> gate_input_index;
	int gate_output_index = varIndexDict[gate_output];
	for(auto iter = gate_nets.begin(); iter != gate_nets.end() - 1;  ++iter)
	{
		gate_input_index.push_back(varIndexDict[*iter]);
	}
	strip_all(gate_type, " ");
	strip_all(gate_type, "\t");
	auto caseNO = gateTypeDict[gate_type];
	auto cnfLines = transGATE(caseNO, gate_input_index, gate_output_index);
	CNF.push_back(cnfLines);
}

void netlist_parser_ABC::parse() 
{
	for(auto line: Vline)
	{
		strip_all(line, "\r");
		strip_all(line, "\n");
		if(line.find("input") != std::string::npos) parse_input(line);
		else if(line.find("CB") != std::string::npos) parse_CB(line);
		else if(line.find("output") != std::string::npos) parse_output(line);
		else if(line.find("wire") != std::string::npos) parse_wire(line);
		else if(line != "") parse_gate(line);
	}	
}

void netlist_parser_ABC::printCNF(std::ostream& os)
{
	for(auto gate: CNF)
	{
		for(auto line: gate)
		{
			os << line;
		}
	}
	os << std::endl;
}

//========================================================
// implementation
std::vector<std::string> netlist_parser_ABC::split_wire_info(std::string line, const std::string& line_type)
{
	std::vector<std::string> result;

	strip_all(line, line_type);
	strip_all(line, " ");

	SplitString(line, result, ",");
	for(auto &net: result)
	{
		strip_all(net, "[");
		strip_all(net, "]");
		strip_all(net, " ");
		strip_all(net, "\t");
	}
	return result;
}

std::vector<string> netlist_parser_ABC::find_netname(std::string gate)
{
	std::string s = gate;
	std::vector<string> container;
	std::vector<string> netnames;
	SplitString(s, container, "(");
	for (std::vector<string>::iterator iter = container.begin(); iter != container.end(); ++iter) {
		std::smatch nets;
		std::regex pattern("([^\\)]+)([\\)])");
		std::regex_search(*iter, nets, pattern);
		for (unsigned int i = 0; i < nets.size(); i++) 
		{
			if (nets[i].str().find(")") == string::npos) netnames.push_back(nets[i].str());
		}
	}
	return netnames;
}

std::string netlist_parser_ABC::find_gatetype(std::string line)
{
    strip_all(line, " ");
    std::regex pattern("^([a-z]*)([0-9]*)");
    std::smatch result;
    std::regex_search(line, result, pattern);
    return result[1].str();
}

#endif