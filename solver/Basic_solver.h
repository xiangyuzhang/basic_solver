#ifndef BASIC_SOLVER_BASIC_SOLVER_H
#define BASIC_SOLVER_BASIC_SOLVER_H

#include <string>
#include <fstream>
#include <vector>
#include "core/Solver.h"
#include "Parser/netlist_parser.h"
#include "CNF/CNF_handler.h"

#include <cmath>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "simp/SimpSolver.h"
#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
using namespace Minisat;

class Basic_solver
{

public:
	Basic_solver() = default;
	Basic_solver(std::string, std::string);
	virtual void solve(std::vector<unsigned>& freeze_variable);
	virtual void parse();
	virtual void assign();
	virtual void printSolution(std::ostream&);
	virtual void printSolution_netname(std::ostream&);
	virtual void printCNF(std::ostream&);

	std::string result;

private:
	SimpSolver *S;
	netlist_parser_ABC *parser;
	CNF_handler *handler;

	std::string netlist_path;
	std::string assignment_path;

	std::vector<std::string> nets_value_one;
	std::vector<std::string> nets_value_zero;

	std::vector<std::vector<std::string>> CNF;

	std::vector<std::string> get_assignment(std::string, const std::string&);
	void freeze(const std::vector<unsigned>& freeze_variable);

};

Basic_solver::Basic_solver(std::string n_path, std::string a_path):netlist_path(n_path),assignment_path(a_path)
{
	S = new SimpSolver;
	parser = new netlist_parser_ABC(netlist_path);
}

void Basic_solver::parse()
{
	parser->parse();
	std::ifstream infile(assignment_path);
	std::string line;
	while(getline(infile, line))
	{
		if(line != "")
		{
			if(line.find("true") != std::string::npos) nets_value_one = get_assignment(line, "true");
			if(line.find("false") != std::string::npos) nets_value_zero = get_assignment(line, "false");
		}
	}

}

void Basic_solver::assign()
{
	ofstream outfile("target_pure.cnf");
	parser->printCNF(outfile);
	handler = new CNF_handler(parser);
	handler->assign(nets_value_one, true);
	handler->assign(nets_value_zero, false);
}

void Basic_solver::solve(std::vector<unsigned>& freeze_variable)
{
	std::ofstream outfile("target.cnf");
	printCNF(outfile);
	result = "NULL";
	gzFile in = gzopen("target.cnf", "rb");
	parse_DIMACS(in, *S);
	gzclose(in);
	freeze(freeze_variable);
	S->eliminate(true);
	os << "Number of variable = " << S->nVars() << std::endl;
	os << "Number of Clause = " << S->nClauses() << std::endl;	
	if(!S->okay())
	{
		result = "UNSAT";
		return;
	}

	vec<Lit> dummy;
	lbool ret = S->solveLimited(dummy);

	if(ret == l_True)
	{
		result = "SAT";
	}
	else if(ret == l_False) 
	{
		result = "UNSAT";
		return;
	}
	else 
	{
		result = "INDET";
		return;
	}

}

void Basic_solver::printCNF(std::ostream& s)
{
	for(auto gate: handler->CNF)
	{
		for(auto line: gate)
		{
			s << line;
		}
	}
	s << std::endl;
}

std::vector<std::string> Basic_solver::get_assignment(std::string line, const std::string& keyword)
{
	auto res = split_wire_info(line, keyword);
	return res;
}


void Basic_solver::freeze(const std::vector<unsigned>& freeze_variable)
{
	static int already_frozen = 0;
	if(already_frozen == 0)
	{
		for(auto i: freeze_variable)
		{
			S->setFrozen(i - 1, true);
		}
		++already_frozen;
	}
}

void Basic_solver::printSolution(std::ostream& os)
{
	if(result == "NULL")
	{
		os << "SAT_solver: solver not yet started!!!" << std::endl;
		return;
	}
	else
	{
		os << result + "\n";
		for(int i = 0; i != S->nVars(); ++i)
		{
			if(S->model[i] != l_Undef)
			{
				os << " ";
				if(S->model[i] == l_True) os << "" + std::to_string(i + 1);
				else os << "-" + std::to_string(i + 1);
			}
		}
		os << std::endl;
	}
}

void Basic_solver::printSolution_netname(std::ostream& os)
{
	if(result == "SAT")
	{
		os << "SAT" << std::endl;
		os << "input ";
		for(auto input: parser->PI_name_to_index)
		{
			os << input.first << "=";
			if(S->model[input.second - 1] == l_True) os << "1\t";
			else if(S->model[input.second - 1] == l_False) os << "0\t";
			else os << "Undef\t";
		}
		os << std::endl;

		os << "output ";
		for(auto output: parser->PO_name_to_index)
		{
			os << output.first << "=";
			if(S->model[output.second - 1] == l_True) os << "1\t";
			else if(S->model[output.second - 1] == l_False) os << "0\t";
			else os << "Undef\t";
		}
		os << std::endl;

		os << "wire ";
		for(auto wire: parser->wire_name_to_index)
		{
			os << wire.first << "=";
			if(S->model[wire.second - 1] == l_True) os << "1\t";
			else if(S->model[wire.second - 1] == l_False) os << "0\t";
			else os << "Undef\t";
		}
		os << std::endl;	

	}
	else os << result << std::endl;
}

#endif