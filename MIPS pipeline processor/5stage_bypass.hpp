/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef MIPS_PROCESSOR_HPP
#define MIPS_PROCESSOR_HPP

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>
using namespace std;
struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext;
	int final_jump;
	bool jump_or_not=false;
	bool check_ko_true_karna=false;
	int dummy[32]={0};	//creating dummy registers
	int lock[32]={0};	//initialising locks
	bool check = true;
	bool check_IF = false;
	int pc_IF=0;
	
	bool check_ID = false;
	int pc_ID=0;
	
	bool check_ALU = false;
	int pc_ALU=0;
	// int mem_from_reg;
	
	bool check_MEM = false;
	int pc_MEM=0;
	int from_alu;
	
	bool check_WB = false;
	int pc_WB=0;
	
	int wb_value;

	unordered_map<string, function<int(MIPS_Architecture &, string, string, string)>> instructions;
	unordered_map<string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	vector<vector<string>> commands;
	vector<int> commandCount;
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
	MIPS_Architecture(ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + to_string(i)] = i + 8, registerMap["$s" + to_string(i)] = i + 16;
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
	int add(string r1, string r2, string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}

	// perform subtraction operation
	int sub(string r1, string r2, string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(string r1, string r2, string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a * b; });
	}

	// perform the binary operation
	int op(string r1, string r2, string r3, function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(string r1, string r2, string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(string r1, string r2, string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(string r1, string r2, string label, function<bool(int, int)> comp)
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
	int slt(string r1, string r2, string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(string label, string unused1 = "", string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(string r, string location, string unused1 = "")
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
	int sw(string r, string location, string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		PCnext = PCcurr + 1;
		return 0;
	}

	int locateAddress(string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (exception &e)
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
		catch (exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(string r1, string r2, string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
			return 0;
		}
		catch (exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(vector<string> regs)
	{
		return all_of(regs.begin(), regs.end(), [&](string r)
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
		cout << '\n';
		switch (code)
		{
		case 1:
			cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			cerr << "Syntax error encountered\n";
			break;
		case 5:
			cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				cerr << s << ' ';
			cerr << '\n';
		}
		cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				cout << 4 * i << '-' << 4 * i + 3 << hex << ": " << data[i] << '\n'
						  << dec;
		cout << "\nTotal number of cycles: " << cycleCount << '\n';
		cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				cout << s << ' ';
			cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		vector<string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = vector<string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != string::npos)
		{
			int idx = command[0].find(':');
			string label = command[0].substr(0, idx);
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
	void constructCommands(ifstream &file)
	{
		string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}
	int check_op(string comm){
		if(comm=="slt"||comm=="add"||comm=="sub"||comm=="mul") return 1;
		else if(comm=="lw") return 2;
		else if(comm=="sw") return 3;
		else if(comm=="beq"||comm=="bne") return 4;
		else if(comm=="addi") return 5;
		else return 6;
	}
	int l_op(string a1,string a2,string op){
		if(op=="add") return dummy[registerMap[a1]]+dummy[registerMap[a2]];
		else if(op=="sub") return dummy[registerMap[a1]]-dummy[registerMap[a2]];
		else if(op=="slt") return dummy[registerMap[a1]]<dummy[registerMap[a2]];
		else return dummy[registerMap[a1]]*dummy[registerMap[a2]];
	}
	pair<int,int> address_find(string location)
	{
		
		int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		string reg = location.substr(lparen + 1);
		reg.pop_back();
		int address = registers[registerMap[reg]] + offset;
		return make_pair(registerMap[reg],offset);
		
	}

	int jeet_gaye=false;
	string store;
	void executeCommandsUnpipelined()
	{
		PCcurr=0;
		int nothing_count=0;
		int clockCycles = -1;
		while ((check_ALU||check_ID||check_IF||check_MEM||check_WB||clockCycles==-1))
		{
			printRegisters(clockCycles);
			// WRITE BACK STAGE
			//if(!(check_ALU||check_ID||check_IF||check_MEM||check_WB)){cout<<PCcurr<<endl;}
			cout<<check_IF<<" "<<check_ID<<" "<<check_ALU<<" "<<check_MEM<<" "<<check_WB<<endl;
			if(check_WB){
				vector<string> &command_WB =commands[pc_WB];
				int l=check_op(command_WB[0]);
				if(l==1||l==5){
					registers[registerMap[command_WB[1]]]=dummy[registerMap[command_WB[1]]];
					// lock[registerMap[command_WB[1]]]--;
				}
				if(l==2){
					registers[registerMap[command_WB[1]]]=dummy[registerMap[command_WB[1]]];
				}
				check_WB=false;
			}
			//DATA MEMORY STAGE
			if(check_MEM){
				vector<string> &command_MEM =commands[pc_MEM];
				int l=check_op(command_MEM[0]);
				if(l==2){
					dummy[registerMap[command_MEM[1]]]=data[from_alu];
					jeet_gaye=true;	
					store=command_MEM[1];				
				}
				else if(l==3){
					if(lock[registerMap[command_MEM[1]]]==0)
					data[from_alu]=dummy[registerMap[command_MEM[1]]];		
				}else if(l==1||l==5){
					dummy[registerMap[command_MEM[1]]]=from_alu;
				}
				if(l!=3 || (lock[registerMap[command_MEM[1]]]==0)){
					check_MEM=false;
					check_WB=true;
					pc_WB=pc_MEM;
				}
				// command_WB=command_MEM;		
			}
			//ALU STAGE
			if(check_ALU){
				vector<string> &command_ALU =commands[pc_ALU];
				int l=check_op(command_ALU[0]);
				if(l==1){
					if( lock[registerMap[command_ALU[2]]] == 0 && lock[registerMap[command_ALU[3]]] == 0  ){
						lock[registerMap[command_ALU[1]]]--;
						from_alu=l_op(command_ALU[2],command_ALU[3],command_ALU[0]);
						check_ALU=false;
						check_MEM=true;
						pc_MEM=pc_ALU;
						lock[registerMap[command_ALU[1]]]++;
						
						// command_MEM=command_ALU;
					}	
				}else if(l==2||l==3){
					if(l==3){
						if(lock[address_find(command_ALU[2]).first]==0){
							pair<int,int> hello=address_find(command_ALU[2]);
							from_alu=(dummy[hello.first]+hello.second)/4;
							check_ALU=false;
							check_MEM=true;
							pc_MEM=pc_ALU;	
							// command_MEM=command_ALU;
						}
					}else{
						if(lock[address_find(command_ALU[2]).first]==0){
							pair<int,int> hello=address_find(command_ALU[2]);
							from_alu=(dummy[hello.first]+hello.second)/4;
							check_ALU=false;
							check_MEM=true;
							pc_MEM=pc_ALU;
							lock[registerMap[command_ALU[1]]]++;	
							// command_MEM=command_ALU;
						}
					}
					
				}else if(l==4){
					if(lock[registerMap[command_ALU[2]]] == 0 && lock[registerMap[command_ALU[1]]] == 0){
						//cout<<"say cheese"<<endl;
						jump_or_not=(dummy[registerMap[command_ALU[1]]] == dummy[registerMap[command_ALU[2]]]);
						final_jump=address[command_ALU[3]];
						//cout<<"say hii"<<endl;
						if(command_ALU[0]=="bne") jump_or_not=not(jump_or_not);
						check_ALU=false;
						check_ko_true_karna=true;
						check_MEM=true;
						pc_MEM=pc_ALU;

					}
				}else if(l==5){
					if( lock[registerMap[command_ALU[2]]]==0){
						lock[registerMap[command_ALU[1]]]--;
						from_alu=dummy[registerMap[command_ALU[2]]]+stoi(command_ALU[3]);
						check_ALU=false;
						check_MEM=true;
						pc_MEM=pc_ALU;	
						lock[registerMap[command_ALU[1]]]++;					
						// command_MEM=command_ALU;
					}
				}
				else{
					check_MEM=true;
					check_ALU=false;
					pc_MEM=pc_ALU;
				}
			}
			if(check_ID && check_ALU==false){
				
				vector<string> &command_ID=commands[pc_ID];
				int l=check_op(command_ID[0]);
				if(l==1){
					//cout<<"add"<<endl;
					if( true){
						// dummy[registerMap[command_ID[2]]]=registers[registerMap[command_ID[2]]];
						// dummy[registerMap[command_ID[3]]]=registers[registerMap[command_ID[3]]];
						check_ID=false;
						check_ALU=true;
						pc_ALU=pc_ID;
						
						// command_ALU=command_ID;
					}						
				}else if(l==4)
				{//cout<<"beq"<<endl;
					if(true){
						check_ID=false;
						check_ALU=true;
						pc_ALU=pc_ID;
						check_ID=false;
						}}

				else if(l==5)
				{//cout<<"addi"<<endl;
					if(true){
						// dummy[registerMap[command_ID[2]]]=registers[registerMap[command_ID[2]]];
						check_ID=false;
						check_ALU=true;
						pc_ALU=pc_ID;
						
						//if(command_ID[1]=="$t1"){cout<<"increment"<<endl;}
						// command_ALU=command_ID;
					}
					//if(command_ID[1]=="$t1"){cout<<"fuck"<<endl;}
				}
				else if(l==3)
				{	//cout<<"sw"<<endl;
					if(true)
					{
						check_ID=false;
						check_ALU=true;
						pc_ALU=pc_ID;
					}
					//{cout<<"say hii"<<endl;}	
				}
				else if(l==2)
				{//cout<<"lw"<<endl;
					if(true)
					{
						// dummy[registerMap[command_ID[2]]]=registers[registerMap[command_ID[2]]];
						check_ID=false;
						check_ALU=true;
						pc_ALU=pc_ID;
						

					}
				}
				else{
					//cout<<"jump"<<endl;
					jump_or_not=true;
					check_ko_true_karna=true;
					final_jump=address[command_ID[1]];
					check_ID=false;
					check_ALU=true;
					pc_ALU=pc_ID;
				}
				// if(l==1||l==5||l==2){
				// 	if(registerMap[command_ID[1]]==2)
				// 	cout<<"locked"<<endl;
				// 	lock[registerMap[command_ID[1]]]++;
				// }

			}
			if(check_IF && check_ID==false){
				vector<string> &command_IF=commands[pc_IF];
				int l=check_op(command_IF[0]);
				if(l==4||l==6){
					
					check=false;			
				}
				check_ID=true;
				check_IF=false;
				pc_ID=pc_IF;		
				// command_ID=command_IF;
				PCcurr++;
			}
			if(jeet_gaye){lock[registerMap[store]]--;jeet_gaye=false;}
			if(jump_or_not){
				PCcurr=final_jump;
				
				//cout<<"less go"<<endl;
				jump_or_not=false;
			}
			if(check_ko_true_karna) check=true;
			check_ko_true_karna=false;
			if(check && PCcurr<commands.size() && check_IF==false){

				pc_IF=PCcurr;
				// vector<string> &command_IF = commands[PCcurr];
				// cout<<"hii"<<endl;
				// command_IF[0]=command[0];
				// command_IF[1]=command[1];
				// command_IF[2]=command[2];
				// command_IF[3]=command[3];
				// cout<<"hii"<<endl;
				check_IF=true;

			}
			
			

			++clockCycles;
			// cout<<lock[2]<<endl;

			
			
		}
		//cout<<clockCycles;
		
		printRegisters(clockCycles);
		
	} 

	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		cout << "Cycle number: " << clockCycle << '\n';
				//   << hex;
		for (int i = 0; i < 32; ++i)
			cout << registers[i] << ' ';
		cout << dec << '\n';
	}
};

#endif