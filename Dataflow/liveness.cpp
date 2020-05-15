// ECE 5984 S20 Assignment 2: liveness.cpp
// Group: Sunny Wadkar and Ramprasath Pitta Sekar

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"
#include "available-support.h"

using namespace llvm;
using namespace std;

namespace {

  class Liveness : public FunctionPass {
  public:
    static char ID;

    Liveness() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function& F) {

      std::vector<Value*> values;
      std::vector<string> variables;
      for(Function::arg_iterator A = F.arg_begin(); A != F.arg_end(); ++A) // arguments iterator
      {
        Value* arg = (Value*)A;
        values.push_back(arg);
        variables.push_back(arg->getName());
      }
      for (BasicBlock &BB : F) 
      {
        for (Instruction &II : BB) 
        {
          Instruction *I = &II;
          for(User::op_iterator i = I->op_begin(), j = I->op_end(); i != j; ++i)
          {
            Value* var = *i;
            if (isa<Instruction>(var) || isa<Argument>(var)) 
            {
              if(std::find(values.begin(),values.end(),var) != values.end())
              {
                continue;
              }
              else
              {
                values.push_back(var);
                variables.push_back(var->getName());
              }
            }
          }
	      }
      }
      outs() << "Variables used by this function:\n";
      printStringSet(&variables);

      po_iterator<BasicBlock*> start = po_begin(&F.getEntryBlock());
      po_iterator<BasicBlock*> end = po_end(&F.getEntryBlock());
      while(start != end) 
      {
        backorder.push_back(*start);
        ++start;
      }

      BitVector boundaryCond(variables.size(), false);
      BitVector initCond(variables.size(), false);
      dataFlow* df = new dataFlow(variables.size(),UNION,BACKWARD,boundaryCond,initCond);
      genTFDependencies(F,variables);
      df->executeDataFlowPass(F,bbSets);
      // df->printMapping();
      printDFAResults(df->dataFlowHash);
      // Did not modify the incoming Function.
      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

  private:
    std::vector<BasicBlock*> backorder;
    std::map<BasicBlock*,basicBlockDeps*> bbSets;
    std::map<string,int> domainMap;
    std::map<int,string> revDomainMap;
    BitVector phiValues;
    void genTFDependencies(Function &F, std::vector<string> domain)
    {
      BitVector empty((int)domain.size(), false);
      phiValues = empty;
      int vectorIdx = 0;
      for(string v : domain)
      {
        domainMap[v] = vectorIdx;
        revDomainMap[vectorIdx] = v;
        ++vectorIdx;
      }
      for(BasicBlock* BB : backorder)
      {
        struct basicBlockDeps* bbSet = new basicBlockDeps();
        bbSet->blockRef = BB;
        bbSet->genSet = empty;
        bbSet->killSet = empty;
        for(BasicBlock::reverse_iterator II = BB->rbegin(); II != BB->rend(); ++II)
        {
          Instruction *I = &*II;
          for(int opc = 0; opc < (int)I->getNumOperands(); ++opc)
          {
            Value* op = I->getOperand(opc);
            if(PHINode* phi = dyn_cast<PHINode>(I))
            {
              for(int z = 0; z < (int)phi->getNumIncomingValues();++z)
              {
                Value* phi_val = phi->getIncomingValue(z);
                if(std::find(domain.begin(),domain.end(),phi_val->getName()) != domain.end())
                {
                  phiValues.set(domainMap[phi_val->getName()]);
                }
              }
            }
            if(std::find(domain.begin(),domain.end(),op->getName()) != domain.end())
            {
              bbSet->genSet.set(domainMap[op->getName()]);
            }
          }
          if(std::find(domain.begin(),domain.end(),I->getName()) != domain.end())
          {
            bbSet->killSet.set(domainMap[I->getName()]); 
            if(bbSet->genSet[domainMap[I->getName()]])
            {
              bbSet->genSet.reset(domainMap[I->getName()]); 
            }     
          }
        }
        bbSets[BB] = bbSet;
      }
      phiValues.flip();
    }

    void printDFAResults(std::map<BasicBlock*,basicBlockProps*> dFAHash)
    {
      std::map<BasicBlock*,basicBlockProps*>::iterator it = dFAHash.begin();
      while(it != dFAHash.end())
      {
        struct basicBlockProps* temp = dFAHash[it->first];
        outs() << "Basic Block Name : ";
        outs() << temp->block->getName() << "\n";
        std::vector<string> genbb;
        // temp->genSet &= phiValues;   // Remove Values corressponding to Phi Nodes
        outs() << "use[BB] : ";
        for(int m = 0; m < (int)temp->genSet.size(); m++)
        {
          if(temp->genSet[m])
          {
            genbb.push_back(revDomainMap[m]);
          }
        }
        printStringSet(&genbb);
        std::vector<string> killbb;
        // temp->killSet &= phiValues;  // Remove Values corressponding to Phi Nodes
        outs() << "def[BB] : ";
        for(int n = 0; n < (int)temp->killSet.size(); n++)
        {
          if(temp->killSet[n])
          {
            killbb.push_back(revDomainMap[n]);
          }
        }
        printStringSet(&killbb);
        std::vector<string> inbb;
        // temp->bbInput &= phiValues;  // Remove Values corressponding to Phi Nodes
        outs() << "IN[BB] : ";
        for(int k = 0; k < (int)temp->bbInput.size(); k++)
        {
          if(temp->bbInput[k])
          {
            inbb.push_back(revDomainMap[k]);
          }
        }
        printStringSet(&inbb);
        std::vector<string> outbb;
        // temp->bbOutput &= phiValues;  // Remove Values corressponding to Phi Nodes
        outs() << "OUT[BB] : ";
        for(int l = 0; l < (int)temp->bbOutput.size(); l++)
        {
          if(temp->bbOutput[l])
          {
            outbb.push_back(revDomainMap[l]);
          }
        }
        printStringSet(&outbb);
        outs() << "\n";
        it++;
      }
    }
  };

  char Liveness::ID = 0;
  RegisterPass<Liveness> X("liveness", "ECE 5984 Liveness");
}
