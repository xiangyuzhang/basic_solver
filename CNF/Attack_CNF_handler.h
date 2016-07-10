#ifndef CNF_ATTACK_CNF_HANDLER_H
#define CNF_ATTACK_CNF_HANDLER_H

#include <iostream>
#include "CNF/CNF_handler.h"
#include "attack/constructor.h"
#include "Parser/fault_parser.h"

#define SA_NOMATTER 1
#define SA_1 2
#define SA_0 3
#define SA_ALLMATTER 4


class Attack_CNF_handler: public virtual CNF_handler
{
public:
	Attack_CNF_handler() = default;
	Attack_CNF_handler(const netlist_parser_ABC* circuit_info, const constructor constructor_info):CNF_handler(circuit_info), circuit(constructor_info), duplication_amount(0){}


	void start_attack(Fault_parser& parser);
	void print_file(std::ostream&);
private:

	const constructor circuit;
	unsigned duplication_amount;

	std::vector<std::string> CB_constrains;
	std::vector<std::string> CB_connections;
	std::vector<std::vector<std::vector<std::string>>> duplications;
	void connect_CB();
	void learn_clause(Fault_parser& trials);
	void process_inport_CB();
	void add_learnt_constrains(Fault_parser& trials);

	std::vector<std::vector<std::string>> make_S0_duplication(const Trial& round);
	std::vector<std::vector<std::string>> make_S1_duplication(const Trial& round);
	std::vector<std::vector<std::string>> make_FF_duplication(const Trial& round);

	std::map<std::string, unsigned> get_current_varIndexDict(const unsigned& offset);
	std::vector<std::string> connect_internal(const Trial& round, const std::map<std::string, unsigned>& dict);

	std::vector<std::string> process_CB(std::vector<std::string> CBs);
	unsigned determine_case(const Trial&) const;
};

void Attack_CNF_handler::start_attack(Fault_parser& parser)
{
	process_inport_CB();
	learn_clause(parser);
	add_learnt_constrains(parser);
	connect_CB();
}


void Attack_CNF_handler::process_inport_CB()
{

	for(auto for_each_gate: circuit.inports_CB_list)  // each module in netlist
	{
		for(auto for_each_fan: for_each_gate)		// each fanin in netlist
		{
			auto temp = process_CB(for_each_fan);
			CB_constrains += temp;	// generate constrain for certain fan inport
		}
	}	
	CNF.push_back(CB_constrains);
}

void Attack_CNF_handler::learn_clause(Fault_parser& trials)
{
	for(auto &round: trials.trial_list)
	{
		round.fault_case = determine_case(round);
		if(round.fault_case == SA_NOMATTER)
		{
			std::cerr << "case = " << "no matter" << std::endl;
		}
		else if(round.fault_case == SA_1)
		{
			std::cerr<< "case = " << "SA_1" << std::endl;
		}
		else if(round.fault_case == SA_0) 
		{
			std::cerr << "case = " << "SA_0" << std::endl;
		}
		else if(round.fault_case == SA_ALLMATTER)
		{
			std::cerr << "case = " << "SA_ALLMATTER" << std::endl;
		}
		else
		{
			std::cerr << "case can't be decided" << std::endl;
			std::cerr << "\t" << round.fault_site << std::endl;
		}
	}
}

vector<std::string> Attack_CNF_handler::process_CB(std::vector<std::string> CBs)
{
	auto dict = target->varIndexDict;
	std::vector<std::string> result;
	std::string head_cnf;
	for(auto net: CBs)
	{
		head_cnf += std::to_string(dict.at(net)) + " "; 
	}
	head_cnf += "0\n";
	result.push_back(head_cnf);
	for(auto target_net: CBs)
	{
		for(auto net: CBs)
		{
			if(net == target_net) continue;
			else
			{
				std::string temp = "-" + std::to_string(dict.at(target_net)) + " " + "-" + std::to_string(dict.at(net)) + " 0\n";
				result.push_back(temp);
			}
		}
	}

	return result;

}

void Attack_CNF_handler::add_learnt_constrains(Fault_parser& parser)
{
	auto trials = parser.trial_list;
	for(auto round:trials)
	{
		if(round.fault_case == SA_NOMATTER) duplications.push_back(make_FF_duplication(round));
		else if(round.fault_case == SA_1) duplications.push_back(make_S1_duplication(round));
		else if(round.fault_case == SA_0) duplications.push_back(make_S0_duplication(round));
		else if(round.fault_case == SA_ALLMATTER) std::cerr << "Attack_CNF_handler: ALL Matter case!" << std::endl;
	}
}

unsigned Attack_CNF_handler::determine_case(const Trial& round) const
{
	if((round.output_vector_FF == round.output_vector_S1) && (round.output_vector_FF == round.output_vector_S0)) return SA_NOMATTER;
	else if(round.output_vector_FF == round.output_vector_S1) return SA_0;
	else if(round.output_vector_FF == round.output_vector_S0) return SA_1;
	else if((round.output_vector_FF != round.output_vector_S0) && (round.output_vector_FF != round.output_vector_S1)) return SA_ALLMATTER;
	else return -1;
}

std::vector<std::vector<std::string>> Attack_CNF_handler::make_S0_duplication(const Trial& round)
{
	auto dict_1 = get_current_varIndexDict(net_amount*duplication_amount);
	auto dict_2 = get_current_varIndexDict(net_amount*(duplication_amount + 1));
	// connect all the internal nets
	auto connect_in_1 = connect_internal(round, dict_1);
	auto connect_in_2 = connect_internal(round, dict_2);
	// make first duplication and make constrains
	auto dup_1 = duplicate_circuit(net_amount * duplication_amount);
	auto assign_fault_site_1_in = assign(dict_1.at(round.fault_site + "_in"),true);
	auto assign_fault_site_1_out = assign(dict_1.at(round.fault_site + "_out"),true);
	connect_in_1.push_back(assign_fault_site_1_in);
	connect_in_1.push_back(assign_fault_site_1_out);
	dup_1.push_back(connect_in_1);
	dup_1.push_back(assign(circuit.PI_list, round.input_vector, dict_1));
	dup_1.push_back(assign(circuit.PO_list, round.output_vector_FF, dict_1));
	// make another duplication and make constrains
	auto dup_2 = duplicate_circuit(net_amount * (duplication_amount + 1));
	auto assign_fault_site_2_in = assign(dict_2.at(round.fault_site + "_in"),false);
	auto assign_fault_site_2_out = assign(dict_2.at(round.fault_site + "_out"),true);
	connect_in_2.push_back(assign_fault_site_2_in);
	connect_in_2.push_back(assign_fault_site_2_out);
	dup_2.push_back(connect_in_2);
	dup_2.push_back(assign(circuit.PI_list, round.input_vector, dict_2));
	dup_2.push_back(assign(circuit.PO_list, round.output_vector_S0, dict_2));
	duplication_amount += 2;
	return dup_1 + dup_2;
}

std::vector<std::vector<std::string>> Attack_CNF_handler::make_S1_duplication(const Trial& round)
{
	auto dict_1 = get_current_varIndexDict(net_amount*duplication_amount);
	auto dict_2 = get_current_varIndexDict(net_amount*(duplication_amount + 1));
	// connect all the internal nets
	auto connect_in_1 = connect_internal(round, dict_1);
	auto connect_in_2 = connect_internal(round, dict_2);
	// make first duplication and make constrains
	auto dup_1 = duplicate_circuit(net_amount * duplication_amount);
	auto assign_fault_site_1_in = assign(dict_1.at(round.fault_site + "_in"),true);
	auto assign_fault_site_1_out = assign(dict_1.at(round.fault_site + "_out"),false);
	connect_in_1.push_back(assign_fault_site_1_in);
	connect_in_1.push_back(assign_fault_site_1_out);
	dup_1.push_back(connect_in_1);
	dup_1.push_back(assign(circuit.PI_list, round.input_vector, dict_1));
	dup_1.push_back(assign(circuit.PO_list, round.output_vector_S1, dict_1));
	// make another duplication and make constrains
	auto dup_2 = duplicate_circuit(net_amount * (duplication_amount + 1));
	auto assign_fault_site_2_in = assign(dict_2.at(round.fault_site + "_in"),false);
	auto assign_fault_site_2_out = assign(dict_2.at(round.fault_site + "_out"),false);
	connect_in_2.push_back(assign_fault_site_2_in);
	connect_in_2.push_back(assign_fault_site_2_out);
	dup_2.push_back(connect_in_2);
	dup_2.push_back(assign(circuit.PI_list, round.input_vector, dict_2));
	dup_2.push_back(assign(circuit.PO_list, round.output_vector_FF, dict_2));
	duplication_amount += 2;
	return dup_1 + dup_2;
}

std::vector<std::vector<std::string>> Attack_CNF_handler::make_FF_duplication(const Trial& round)
{
	auto dict_1 = get_current_varIndexDict(net_amount*duplication_amount);
	auto dict_2 = get_current_varIndexDict(net_amount*(duplication_amount + 1));
	// connect all the internal nets
	auto connect_in_1 = connect_internal(round, dict_1);
	auto connect_in_2 = connect_internal(round, dict_2);
	// make first duplication and make constrains
	auto dup_1 = duplicate_circuit(net_amount * duplication_amount);
	auto assign_fault_site_1 = assign(dict_1.at(round.fault_site + "_in"),true);
	connect_in_1.push_back(assign_fault_site_1);
	dup_1.push_back(connect_in_1);
	dup_1.push_back(assign(circuit.PI_list, round.input_vector, dict_1));
	dup_1.push_back(assign(circuit.PO_list, round.output_vector_FF, dict_1));
	// make another duplication and make constrains
	auto dup_2 = duplicate_circuit(net_amount * (duplication_amount + 1));
	auto assign_fault_site_2 = assign(dict_2.at(round.fault_site + "_in"),false);
	connect_in_2.push_back(assign_fault_site_2);
	dup_2.push_back(connect_in_2);
	dup_2.push_back(assign(circuit.PI_list, round.input_vector, dict_2));
	dup_2.push_back(assign(circuit.PO_list, round.output_vector_FF, dict_2));
	duplication_amount += 2;
	return dup_1 + dup_2;
}

std::vector<std::string> Attack_CNF_handler::connect_internal(const Trial& round, const std::map<std::string, unsigned>& dict)
{
	std::vector<std::string> result;
	for(auto element: circuit.internal_input_list)
	{
		if(element  != round.fault_site + "_in")
		{
			strip_all(element, "_in");
			result+=connect_nets(dict.at(element + "_in"), dict.at(element + "_out"));
		}
	}
	return result;
}

std::map<std::string, unsigned> Attack_CNF_handler::get_current_varIndexDict(const unsigned& offset)
{
	auto result = target->varIndexDict;
	for(auto &element: result)
	{
		element.second += offset;
	}
	return result;
}

void Attack_CNF_handler::connect_CB()
{
	for(unsigned index = 1; index <= duplication_amount; ++index)
	{
		for(auto &each_gate: circuit.CB_list)
		{
			for(auto &each_CB: each_gate)
			{
				auto original_index = target->varIndexDict.at(each_CB);
				auto target_index = original_index + index*net_amount;
				CB_connections += connect_nets(original_index, target_index);
			}
		}
	}
}

void Attack_CNF_handler::print_file(std::ostream& os)
{
	for(auto &each_dup: duplications)
	{
		for(auto &each_gate: each_dup)
		{
			for(auto &each_line: each_gate)
			{
				os << each_line;
			}
		}
		os << std::endl;
	}
	os << std::endl;

	for(auto &each_line: CB_constrains)
	{
		os << each_line;
	}
	os << std::endl;

	for(auto &line: CB_connections)
	{
		os << line;
	}
	os << std::endl;

}

#endif