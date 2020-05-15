#ifndef __CLASSICAL_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_H__

#include <stdio.h>
#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include <stack>
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/CFG.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/PostOrderIterator.h"

namespace llvm {

enum passDirection{
    FORWARD = 0,
    BACKWARD = 1,
};

enum meetOperator{
    UNION = 0,
    INTERSECTION = 1,
};

enum blockType{
    ENTRY = 0,
    EXIT = 1,
    REGULAR = 2,
};

struct basicBlockDeps{
    BasicBlock* blockRef;
    BitVector genSet;
    BitVector killSet;
  };

struct basicBlockProps{
    enum blockType bType;
    BasicBlock* block;
    BitVector bbInput;
    BitVector bbOutput;
    BitVector genSet;
    BitVector killSet;
    std::vector<BasicBlock*>predBlocks;
    std::vector<BasicBlock*>succBlocks;
};

class dataFlow{
    private:
        int domainSize;
        BitVector boundaryConditions;
        BitVector initConditions;
        enum meetOperator meetOp;
        enum passDirection passDir;
        std::vector<BasicBlock*> poTraversal;
        std::vector<BasicBlock*> rpoTraversal;
        void buildBasicBlockInfo(Function &F);
        void genTraversalOrder(Function &F);
        void getConnectedBlocks(BasicBlock* BB,struct basicBlockProps* basicBlock);
        void applyTransferFunction(struct basicBlockProps* basicBlock);
        void populateDepSets(std::map<BasicBlock*,basicBlockDeps*> bbSets);
    
    public:
        std::map<BasicBlock*,basicBlockProps*> dataFlowHash;
        dataFlow(int ds, enum meetOperator m, enum passDirection p, BitVector b, BitVector i)
        {
            domainSize = ds;
            meetOp = m;
            passDir = p;
            boundaryConditions = b;
            initConditions = i;
        }
        BitVector calculateMeet(std::vector<BitVector> inputVectors);
        void executeDataFlowPass(Function &F,std::map<BasicBlock*,basicBlockDeps*> bbSets);
        void printMapping();
};

}

#endif
