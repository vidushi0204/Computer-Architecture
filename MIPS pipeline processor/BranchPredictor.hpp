#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include <cassert>
#include <bits/stdc++.h>
using namespace std;

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        pc=(pc<<18);
        pc=(pc>>18);
        if(table[pc].test(1)){return true;}
        else {false;}
        return false;
        
    }

    void update(uint32_t pc, bool taken) {
        pc=(pc<<18);
        pc=(pc>>18);
        if(taken)
        {
            if(!table[pc].test(0))
            {
                table[pc].set(0);
            }
            else
            {
                if(!(table[pc].test(1)))
                {
                    table[pc].set(1);
                    table[pc].reset(0);
                }
            }
        }
        else
        {
            if(table[pc].test(0))
            {
                table[pc].reset(0);
            }
            else
            {
                if(table[pc].test(1))
                {
                    table[pc].reset(1);
                    table[pc].set(0);
                }
            }
        }
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        int x= bhr.to_ulong();
        std :: bitset<2> dum = bhrTable[x];
         if(dum.test(1)){return true;}
        else {false;}
        return false;
    }

    void update(uint32_t pc, bool taken) {
        int x= bhr.to_ulong();
        std :: bitset<2> dum = bhrTable[x];
        if(taken)
        {
            if(!bhrTable[x].test(0))
            {
                bhrTable[x].set(0);
            }
            else
            {
                if(!(bhrTable[x].test(1)))
                {
                    bhrTable[x].set(1);
                    bhrTable[x].reset(0);
                }
            }
        }
        else
        {
            if(bhrTable[x].test(0))
            {
                bhrTable[x].reset(0);
            }
            else
            {
                if(bhrTable[x].test(1))
                {
                    bhrTable[x].reset(1);
                    bhrTable[x].set(0);
                }
            }
        }
        if(bhr.test(0))
        {
            bhr.set(1);
        }
        else
        {
            bhr.reset(1);
        }
        if(taken)
        {
            bhr.set(0);
        }
        else
        {
            bhr.reset(0);
        }
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        pc=(pc<<18);
        pc=(pc>>18);
        int x = table[pc].to_ulong();
        std::bitset<2> dum = combination[4*pc+x];
        if(dum.test(1)){return true;}
        else {false;}
        return false;
    }

    void update(uint32_t pc, bool taken) {
        pc=(pc<<18);
        pc=(pc>>18);
        long long int num = table[pc].to_ulong();
        long long int x=4*pc+num;
        
        if(taken)
        {
            if(!combination[x].test(0))
            {
                combination[x].set(0);
            }
            else
            {
                if(!(combination[x].test(1)))
                {
                    combination[x].set(1);
                    combination[x].reset(0);
                }
            }
        }
        else
        {
            if(combination[x].test(0))
            {
                combination[x].reset(0);
            }
            else
            {
                if(combination[x].test(1))
                {
                    combination[x].reset(1);
                    combination[x].set(0);
                }
            }
        }
        
        if(table[pc].test(0))
        {
            table[pc].set(1);
        }
        else
        {
            table[pc].reset(1);
        }
        if(taken)
        {
            table[pc].set(0);
        }
        else
        {
            table[pc].reset(0);
        }
    }
};

#endif