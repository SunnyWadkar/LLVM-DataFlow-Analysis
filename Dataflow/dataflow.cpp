// Group : Sunny Wadkar and Ramprasath Pitta Sekar

#include "dataflow.h"

namespace llvm {

  void dataFlow::populateDepSets(std::map<BasicBlock*,basicBlockDeps*> bbSets)
  {
    std::map<BasicBlock*,basicBlockDeps*>::iterator it = bbSets.begin();
    while(it != bbSets.end())
    {
      struct basicBlockDeps* temp = bbSets[it->first];
      struct basicBlockProps* temp2 = dataFlowHash[it->first];
      temp2->genSet = temp->genSet;
      temp2->killSet = temp->killSet;
      it++;
    }
  }

  BitVector dataFlow::calculateMeet(std::vector<BitVector> inputVectors)
  {
    BitVector out_vector = inputVectors[0];
    for(int i = 1; i < (int)inputVectors.size(); ++i)
    {
      if(meetOp == UNION)
      {
        out_vector |= inputVectors[i];
      }
      else if(meetOp == INTERSECTION)
      {
        out_vector &= inputVectors[i];
      }
    }
    return out_vector;
  }

  void dataFlow::applyTransferFunction(struct basicBlockProps* basicBlock)
  {
    BitVector killSet = basicBlock->killSet;
    if(passDir == FORWARD)
    {
      basicBlock->bbOutput = killSet.flip();
      basicBlock->bbOutput &= basicBlock->bbInput;
      basicBlock->bbOutput |= basicBlock->genSet;
    }
    else if(passDir == BACKWARD)
    {
      basicBlock->bbInput = killSet.flip();
      basicBlock->bbInput &= basicBlock->bbOutput;
      basicBlock->bbInput |= basicBlock->genSet;
    }
  }

 void dataFlow::getConnectedBlocks(BasicBlock* BB, struct basicBlockProps* basicBlock)
 {
   for(BasicBlock* predblock: predecessors(BB))
   {
     basicBlock->predBlocks.push_back(predblock);
   }

   for(BasicBlock* succblock: successors(BB))
   {
     basicBlock->succBlocks.push_back(succblock);
   }
 }

 void dataFlow::buildBasicBlockInfo(Function &F)
 {
   BitVector empty(domainSize, false);
   for(BasicBlock &BB : F)
   {
     struct basicBlockProps* bbProp = new basicBlockProps();
     bbProp->block = &BB;
     bbProp->bbInput = empty;
     bbProp->bbOutput = empty;
     bbProp->genSet = empty;
     bbProp->killSet = empty;
     getConnectedBlocks(&BB,bbProp);
     if(&BB == &F.getEntryBlock())
     {
       bbProp->bType = ENTRY;
     }
     else
     {
       bbProp->bType = REGULAR;
     }
     for(Instruction &II: BB)
     {
       Instruction *I = &II;
       if(dyn_cast<ReturnInst>(I))
       {
         bbProp->bType = EXIT;
       }
     }
     dataFlowHash[&BB] = bbProp;
   }
   std::map<BasicBlock*,basicBlockProps*>::iterator it = dataFlowHash.begin();
   struct basicBlockProps* temp;
   while(it != dataFlowHash.end())
   {
     temp = dataFlowHash[it->first];
     if(passDir == FORWARD)
     {
       if(temp->bType == ENTRY)
       {
         temp->bbInput = boundaryConditions;
       }
       temp->bbOutput = initConditions;
     }
     else if(passDir == BACKWARD)
     {
       if(temp->bType == EXIT)
       {
         temp->bbOutput = boundaryConditions;
       }
       temp->bbInput = initConditions;
     }
     ++it;
   }
  }

  void dataFlow::genTraversalOrder(Function &F)
  {
    using namespace std;
    stack<BasicBlock*> rpoStack;
    po_iterator<BasicBlock*> start = po_begin(&F.getEntryBlock());
    po_iterator<BasicBlock*> end = po_end(&F.getEntryBlock());
    while(start != end) 
    {
      poTraversal.push_back(*start);
      rpoStack.push(*start);
      ++start;
    }
    while(!rpoStack.empty())
    {
        rpoTraversal.push_back(rpoStack.top());
        rpoStack.pop();
    }
  }

  void dataFlow::executeDataFlowPass(Function &F,std::map<BasicBlock*,basicBlockDeps*> bbSets)
  {
    bool hasConverged = false;
    std::map<BasicBlock*,BitVector> prevOutputs;
    int iter = 0;
    buildBasicBlockInfo(F);
    genTraversalOrder(F);
    populateDepSets(bbSets);
    std::vector<BasicBlock*> blockTraverse = (passDir == FORWARD) ? rpoTraversal : poTraversal;
    while(!hasConverged)
    {
      for(BasicBlock* BB : blockTraverse)
      {
        prevOutputs[BB] = dataFlowHash[BB]->bbOutput;
        std::vector<BitVector> meetVectors;
        if(passDir == FORWARD)
        {
          for(std::vector<BasicBlock*>::iterator f = dataFlowHash[BB]->predBlocks.begin(); f != dataFlowHash[BB]->predBlocks.end(); ++f) 
          {
            meetVectors.push_back(dataFlowHash[*f]->bbOutput);
          }
        }
        else if(passDir == BACKWARD)
        {
          for(std::vector<BasicBlock*>::iterator b = dataFlowHash[BB]->succBlocks.begin(); b != dataFlowHash[BB]->succBlocks.end(); ++b) 
          {
            meetVectors.push_back(dataFlowHash[*b]->bbInput);
          }
        }
        if(!meetVectors.empty())
        {
          BitVector meet = calculateMeet(meetVectors);
          if(passDir == FORWARD)
          {
            dataFlowHash[BB]->bbInput = meet;
          }
          else if(passDir == BACKWARD)
          {
            dataFlowHash[BB]->bbOutput = meet;
          } 
        }
        applyTransferFunction(dataFlowHash[BB]);
      }
      hasConverged = true;
      for(std::map<BasicBlock*,BitVector>::iterator it = prevOutputs.begin();it != prevOutputs.end();++it)
      { 
        if((dataFlowHash[it->first]->bbOutput) != it->second)
        {
          hasConverged = false;
          break;
        }
      }
      ++iter;
      // printMapping();
    }
    outs() << "DFA Algorithm converged after " << iter << "  iterations\n";
  }

  void dataFlow::printMapping()
  {
   std::map<BasicBlock*,basicBlockProps*>::iterator it = dataFlowHash.begin();
   while(it != dataFlowHash.end())
   {
      struct basicBlockProps* temp = dataFlowHash[it->first];
      outs() << temp->block << " "<< temp->block->getName() << "\n";
      outs() << temp->bType << "\n";
      outs() << "Input ";
      for(int k = 0; k < (int)temp->bbInput.size(); k++)
          outs() << temp->bbInput[k] << ' ';
      outs() << '\n';
      outs() << "Output ";
      for(int l = 0; l < (int)temp->bbOutput.size(); l++)
          outs() << temp->bbOutput[l] << ' ';
      outs() << '\n';
      outs() << "GenSet ";
      for(int m = 0; m < (int)temp->genSet.size(); m++)
          outs() << temp->genSet[m] << ' ';
      outs() << '\n';
      outs() << "KillSet ";
      for(int n = 0; n < (int)temp->killSet.size(); n++)
          outs() << temp->killSet[n] << ' ';
      outs() << '\n';
      for(std::vector<BasicBlock*>::iterator i = temp->predBlocks.begin(); i != temp->predBlocks.end(); ++i) 
      {
        outs() << "pred " << *i << "\n";
      }
      for(std::vector<BasicBlock*>::iterator j = temp->succBlocks.begin(); j != temp->succBlocks.end(); ++j) 
      {
        outs() << "succ " << *j << "\n";
      }
      it++;
    }
  }
}
