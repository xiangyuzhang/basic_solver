//
// Created by xiangyu zhang on 6/2/16.
//

#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>




class Gate
{
public:
    Gate() = default;
    Gate(const std::string &gate_name,const unsigned & fanin_num):name(gate_name), fanin(fanin_num){}

    std::string name;
    unsigned fanin;
    unsigned index;

};

class Parser
{
    friend class generator;
    friend class constructor;
    friend class MUX;
    friend void show_map(const std::map<int, Gate>& target);

public:
    Parser() = default;
    ~Parser() = default;
    Parser(const std::string &);
    void show_gate_info() const;


private:
    std::map<int, Gate> gateInfo;
    const std::string path;
    unsigned PI_num;
    unsigned gate_counter;

    std::string find_gateName(const std::string&) const;
    std::string find_gateIn(const std::string& ) const;

    unsigned find_PiNum(const std::string& ) const;


};

Parser::Parser(const std::string& path)
{
    std::fstream infile(path);
    std::string info;
    PI_num = 0;
    gate_counter = 0;
    while(getline(infile, info))
    {
        if(info.find("PI") != std::string::npos)
        {
            PI_num = find_PiNum(info);
            std::cout << "PI: " << PI_num << std::endl;


        }
        else
        {
            std::cout << "info: " << info << std::endl;
            auto gate_name = find_gateName(info);
            auto gate_fanin = std::stoi(find_gateIn(info));
            Gate gt(gate_name, gate_fanin);
            gt.index = gate_counter;
            gateInfo.insert(std::pair<int, Gate>(gate_counter, gt));
            ++gate_counter;
        }
    }
}


std::string Parser::find_gateName(const std::string& info) const
{
    std::smatch result;
    std::regex pattern("([a-zA-Z0-9]*)(:)([0-9]*)");
    std::regex_search(info, result, pattern);
    return result[1].str();
}

std::string Parser::find_gateIn(const std::string &info) const
{
    std::smatch result;
    std::regex pattern("([a-zA-Z0-9]*)(:)([0-9]*)");
    std::regex_search(info, result, pattern);
    return result[3].str();
}

unsigned Parser::find_PiNum(const std::string &info) const {
    std::smatch result;
    std::regex pattern("(PI)(:)([0-9]*)");
    std::regex_search(info, result, pattern);
    auto num = result[3].str();
//    std::cerr << "NUM:"<< num << std::endl;
    return std::stoi(num);
}
void show_map(const std::map<int, Gate>& target);

void Parser::show_gate_info() const
{
    show_map(gateInfo);
}




//=============================================================
// implement friend
void show_map(const std::map<int, Gate>& target)
{
    for(auto i: target)
    {
        std::cout << i.second.index << " == " << i.second.name << " " << i.second.fanin << std::endl;
    }
}




#endif //PARSER_PARSER_H