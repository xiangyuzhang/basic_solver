#include <iostream>
#include <string>
#include "solver/Basic_solver.h"
#include "CNF/CNF_handler.h"
#include "Parser/netlist_parser.h"
using namespace std;

int main(int argc, char const *argv[])
{
	cerr << "============ Start Project ===========" << endl;

	string netlist = argv[1];
	string assignment = argv[2];
	vector<unsigned> frozen_val({1,2,3,4});
	cout << "Reading netlist from " << netlist << endl;
	cout << "Reading assignment from " << assignment << endl;

	Basic_solver *solver = new Basic_solver(netlist, assignment);
	cerr << "\tInitialized!!!" << endl;
	solver->parse();
	cerr << "\tFinish parsing!!!" << endl;
	solver->assign();
	cerr << "\tFinish assign!!!" << endl;
	solver->solve(frozen_val);
	cerr << "\tFinish solve!!!" << endl;
	solver->printSolution_netname(cout);


	return 0;
}
