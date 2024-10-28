/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef _MIPS_PROCESSOR_HPP_
#define _MIPS_PROCESSOR_HPP_

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <queue>
#include <iostream>
#include <boost/tokenizer.hpp>
using namespace std;
struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext;
    int dummy[32]={0};	//creating dummy registers
	int lock[32]={0};	//initialising locks
    bool check = true; // flag for jump and branch
    bool check_IF1 = false;
    int order_IF1;
	int pc_IF1=0;
    bool check_IF2 = false;
    int order_IF2;
	int pc_IF2=0;
    bool check_ID = false;
    int order_ID;
	int pc_ID=0;
	bool check_DEC1 = false;
    int order_DEC1;
	int pc_DEC1=0;
    bool check_DEC2 = false;
    int order_DEC2;
	int pc_DEC2=0;
	bool check_ALU1 = false;
    int order_ALU1;
	int pc_ALU1=0;
    bool check_ALU2 = false;
    int order_ALU2;
	int pc_ALU2=0;
	bool check_MEM1 = false;
    int order_MEM1;
	int pc_MEM1=0;
	bool check_MEM2 = false;
    int order_MEM2;
	int pc_MEM2=0;
	bool check_WB1 = false;
    int order_WB1;
    int WB1_value;
	int pc_WB1=0;
    bool check_WB2 = false;
    int order_WB2;
    int WB2_value;
	int pc_WB2=0;
	
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::unordered_map<int, int> memoryDelta;
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}

	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		registers[registerMap[r]] = data[address];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		if (data[address] != registers[registerMap[r]])
			memoryDelta[address] = registers[registerMap[r]];
		data[address] = registers[registerMap[r]];
		PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

    pair<int,int> address_find(string location)
	{
		
		int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		string reg = location.substr(lparen + 1);
		reg.pop_back();
		int address = registers[registerMap[reg]] + offset;
		return make_pair(registerMap[reg],offset);
		
	}

    int check_op(string comm){
		if(comm=="slt"||comm=="add"||comm=="sub"||comm=="mul") return 1;
		else if(comm=="lw") return 2;
		else if(comm=="sw") return 3;
		else if(comm=="beq"||comm=="bne") return 4;
		else if(comm=="addi") return 5;
		else return 6;
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

    int l_op(string a1,string a2,string op)
    {
		if(op=="add") return registers[registerMap[a1]]+registers[registerMap[a2]];
		else if(op=="sub") return registers[registerMap[a1]]-registers[registerMap[a2]];
		else if(op=="slt") return registers[registerMap[a1]]<registers[registerMap[a2]];
		else return registers[registerMap[a1]]*registers[registerMap[a2]];
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

	// 7-9 stage pipeline without bypassing
    bool lck1=false;
    int reg1;
    bool lck2=false;
    int reg2;
    int from_ALU1;
    int from_ALU2;
    int from_MEM1;
    int jump_or_not;
    int final_jump;
    int order=1;
	int smth_ID;
	int smth_ALU2;
	int smth_MEM1;
	int smth_MEM2;
	queue<int> q;

	void executeCommandsUnpipelined()
	{
        PCcurr=0;
        int clockCycles = -1;
		while ((check_IF1||check_IF2||check_DEC1||check_DEC2||check_ID||check_ALU1||check_ALU2||check_WB1||check_WB2||check_MEM1||check_MEM2||clockCycles==-1))
		{	
			//cout<<check_IF1<<" "<<check_IF2<<" "<<check_DEC1<<" "<<check_DEC2<<" "<<check_ID<<" "<<check_ALU2<<" "<<check_MEM1<<" "<<check_MEM2<<" "<<check_WB2<<" "<<check_WB1<<endl;
			
			
			if(check_WB1||check_WB2)
            {	
                if(check_WB1&&check_WB2)
                {
                    vector<string> &command_WB1 =commands[pc_WB1];
                    vector<string> &command_WB2 =commands[pc_WB2];
                    int l1=check_op(command_WB1[0]);
                    int l2=check_op(command_WB2[0]);
					bool check1=false;
					if(q.size()==0){check1=true;}
					else if(order_WB1<q.front()){check1=true;}
					//else{cout<<"fff"<<endl;}
                    if(l2==3)
                    {//cout<<"tru"<<endl;
                        if((l1==1||l1==5))
                        {	if(check1){
                            registers[registerMap[command_WB1[1]]]=WB1_value;
                            lck1=true;
                            reg1=registerMap[command_WB1[1]];
							check_WB1=false;
                        	check_WB2=false;
							}
							check_WB2=false;
                        }
						else{
                        check_WB1=false;
                        check_WB2=false;}
                    }
                    else
                    {   //cout<<"tru"<<endl;
                        if(l1==1||l1==5)
                        {
                            if(order_WB1<order_WB2)
                            {
                                registers[registerMap[command_WB1[1]]]=WB1_value;
                                lck1=true;
                                reg1=registerMap[command_WB1[1]];
                                check_WB1=false;
                            }
                            else
                            {	if(l2==2){q.pop();}
                                registers[registerMap[command_WB2[1]]]=WB2_value;
                                lck2=true;
                                reg2=registerMap[command_WB2[1]];
                                check_WB2=false;
                            }
                        }
                        else
                        {	if(l2==2){q.pop();}
                            registers[registerMap[command_WB2[1]]]=WB2_value;
                            lck2=true;
                            reg2=registerMap[command_WB2[1]];
                            check_WB1=false;
                            check_WB2=false;
                        }
                    }

                }
                else if(check_WB1)
                {
                    vector<string> &command_WB1 =commands[pc_WB1];
                    int l1=check_op(command_WB1[0]);
					//cout<<l1<<endl;
					bool check1=false;
					if(q.size()==0){check1=true;}
					else if(order_WB1<q.front()){check1=true;}
					//else{cout<<"fff"<<endl;}
					//if(l1==1||l1==5)cout<<order_WB1<<" "<<q.front() <<endl;
					//cout<<check1<<endl;
                    if(l1==1||l1==5)
                    {	if(check1){
                        registers[registerMap[command_WB1[1]]]=WB1_value;
                        lck1=true;
                        reg1=registerMap[command_WB1[1]];
						check_WB1=false;
						}
                    }
					else{
                    check_WB1=false;}
                }
                else
                {
                    vector<string> &command_WB2 =commands[pc_WB2];
                    int l2=check_op(command_WB2[0]);
                    if(l2==2)
                    {
                        registers[registerMap[command_WB2[1]]]=WB2_value;
						//cout<<WB2_value<<endl;
						//cout<<"lw_reg "<<command_WB2[0]<<" "<<command_WB2[1]<<" "<<command_WB2[2]<<endl;
                        lck2=true;
						if(l2==2){q.pop();}
                        reg2=registerMap[command_WB2[1]];
                    }
                    check_WB2=false;

                }
            }

            if(check_MEM2&&check_WB2==false)
            {
                vector<string> &command_MEM2 =commands[pc_MEM2];
				int l=check_op(command_MEM2[0]);
                if(l==2)
                {
                    WB2_value=data[from_MEM1];
                }
                else
                {
                    data[from_MEM1]=smth_MEM2;
					memoryDelta[from_MEM1]=smth_MEM2;
					
					//cout<<"hii "<<registers[registerMap[command_MEM2[1]]]<<endl;
                }
                check_WB2=true;
                check_MEM2=false;
                pc_WB2=pc_MEM2;
                order_WB2=order_MEM2;
            }
            if(check_MEM1&&check_MEM2==false)
            {
                pc_MEM2=pc_MEM1;
                order_MEM2=order_MEM1;
                check_MEM2=true;
                check_MEM1=false;
                from_MEM1=from_ALU2;
				smth_MEM2=smth_MEM1;
            }
            if(check_ALU2&&check_MEM1==false)
            {
                vector<string> &command_ALU2 =commands[pc_ALU2];
                int l=check_op(command_ALU2[0]);
                pair<int,int> hello=address_find(command_ALU2[2]);
                from_ALU2=(registers[hello.first]+hello.second)/4;
                check_ALU2=false;
                check_MEM1=true;
                pc_MEM1=pc_ALU2;
                order_MEM1=order_ALU2;
				smth_MEM1=smth_ALU2;
            }
            if(check_ALU1&&check_WB1==false)
            {
                vector<string> &command_ALU1 =commands[pc_ALU1];
                int l=check_op(command_ALU1[0]);
                if(l==6)
                {
                    check_ALU1=false;
                    check_WB1=true;
                    pc_WB1=pc_ALU1;
                    order_WB1=order_ALU1;
					
                }
                else if(l==5)
                {
                    WB1_value=registers[registerMap[command_ALU1[2]]]+stoi(command_ALU1[3]);
                    check_ALU1=false;
                    check_WB1=true;
                    pc_WB1=pc_ALU1;
                    order_WB1=order_ALU1;
					//cout<<order_ALU1<<endl;
					
                }
                else if(l==1)
                {
                    WB1_value=l_op(command_ALU1[2],command_ALU1[3],command_ALU1[0]);
                    check_ALU1=false;
                    check_WB1=true;
                    pc_WB1=pc_ALU1;
                    order_WB1=order_ALU1;
					
                }
                else
                {
                    jump_or_not=(registers[registerMap[command_ALU1[1]]] == registers[registerMap[command_ALU1[2]]]);
					final_jump=address[command_ALU1[3]];
                    if(command_ALU1[0]=="bne") {jump_or_not=not(jump_or_not);}
                    check=true;
                    check_ALU1=false;
                    check_WB1=true;
                    pc_WB1=pc_ALU1;
                    order_WB1=order_ALU1;
					


                }
            }
            if(check_ID)
            {
                vector<string> &command_ID = commands[pc_ID];
                int l=check_op(command_ID[0]);
                if(l==2||l==3)
                {
                    if(check_ALU2==false)
                    {	
                        if(l==2)
                        {	
                            if(lock[address_find(command_ID[2]).first]==0)
                            {
                                check_ID=false;
						        check_ALU2=true;
						        pc_ALU2=pc_ID;
                                order_ALU2=order_ID;
						        lock[registerMap[command_ID[1]]]++;
								smth_ALU2=registers[registerMap[command_ID[1]]];
                            }
                        }
                        else
                        {
                            if(lock[registerMap[command_ID[1]]]==0&&lock[address_find(command_ID[2]).first]==0)
                            {
                                check_ID=false;
								// cout<<"fff"<<endl;
						        check_ALU2=true;
						        pc_ALU2=pc_ID;
                                order_ALU2=order_ID;
								smth_ALU2=registers[registerMap[command_ID[1]]];
                            }
                        }
                        
                    }
                }
                else
                {
                    if(check_ALU1==false)
                    {
                        if(l==1)
                        {
                            if( lock[registerMap[command_ID[2]]] == 0 && lock[registerMap[command_ID[3]]] == 0 )
                            {
                                check_ID=false;
						        check_ALU1=true;
						        pc_ALU1=pc_ID;
                                order_ALU1=order_ID;
                                lock[registerMap[command_ID[1]]]++;
                            }
                        }
                        else if(l==5)
                        {
                            if(lock[registerMap[command_ID[2]]]==0)
                            {
                                check_ID=false;
						        check_ALU1=true;
						        pc_ALU1=pc_ID;
                                order_ALU1=order_ID;
								//cout<<order_ALU1<<endl;
                                lock[registerMap[command_ID[1]]]++;
								
                            }
                        }
                        else if(l==4)
                        {
                            if(lock[registerMap[command_ID[2]]] == 0 && lock[registerMap[command_ID[1]]] == 0)
                            {
                                check_ID=false;
						        check_ALU1=true;
						        pc_ALU1=pc_ID;
                                order_ALU1=order_ID;   
                            }
                        }
                        else
                        {
                            check_ID=false;
						    check_ALU1=true;
						    pc_ALU1=pc_ID;
                            order_ALU1=order_ID;   
                        }
                    }
                }
            }
            if(check_DEC2&&check_ID==false)
            {
                vector<string> &command_DEC2 = commands[pc_DEC2];
                int l=check_op(command_DEC2[0]);
                if(l==6)
                {
                    jump_or_not=true;
					check=true;
					final_jump=address[command_DEC2[1]];
                }
				
                check_DEC2=false;
				check_ID=true;
			    pc_ID=pc_DEC2;
                order_ID=order_DEC2;

            }
            if(check_DEC1&&check_DEC2==false)
            {
                check_DEC2=true;
                check_DEC1=false;
                pc_DEC2=pc_DEC1;
                order_DEC2=order_DEC1;
            }
            if(check_IF2&&check_DEC1==false)
            {
                check_IF2=false;
                check_DEC1=true;
                pc_DEC1=pc_IF2;
                order_DEC1=order_IF2;
            }
            if(check_IF1&&check_IF2==false)
            {
                vector<string> &command_IF1=commands[pc_IF1];
				int l=check_op(command_IF1[0]);
                PCcurr++;
                check_IF1=false;
				//cout<<command_IF1[0]<<" "<<order_IF1<<endl;
                check_IF2=true;
                pc_IF2=pc_IF1;
                order_IF2=order_IF1;
                if(l==4||l==6)
                {
					check=false;			
				}
				if(l==2)
				{
					q.push(order_IF1);
					//cout<<order_IF1<<endl;
					//cout<<"hii"<<endl;
				}
            }

            if(jump_or_not)
            {
                PCcurr=final_jump;
                jump_or_not=false;
            }

            if(check && PCcurr<commands.size() && check_IF1==false)
            {
                pc_IF1=PCcurr;
                check_IF1=true;
                order_IF1=order;
                order++;
            }

            if(lck1)
            {
                lock[reg1]--;
                lck1=false;
            }
            if(lck2)
            {
                lock[reg2]--;
                lck2=false;
            }
            ++clockCycles;
			printRegistersAndMemoryDelta(clockCycles);
		}
		
        
		//cout<<clockCycles<<endl;
	}

	// print the register data in hexadecimal
	void printRegistersAndMemoryDelta(int clockCycle)
	{
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << '\n';
		std::cout << memoryDelta.size() << ' ';
		if(memoryDelta.size()==0){
			std::cout<<'\n';
		}
		for (auto &p : memoryDelta)
		std::cout << p.first << ' ' << p.second << '\n';
		memoryDelta.clear();
	}

};

#endif