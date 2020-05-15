// ECE 5984 S20 Assignment 2: available.cpp
// Group: Sunny Wadkar and Ramprasath Pitta Sekar

////////////////////////////////////////////////////////////////////////////////

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"
#include "available-support.h"

using namespace llvm;
using namespace std;

namespace {
  class AvailableExpressions : public FunctionPass {
    
  public:
    static char ID;
    
    AvailableExpressions() : FunctionPass(ID) { }
    
    virtual bool runOnFunction(Function& F) {
      
      // Here's some code to familarize you with the Expression
      // class and pretty printing code we've provided:
      
      vector<Expression> expressions;
      for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
	       BasicBlock* block = &*FI;
	       for (BasicBlock::iterator i = block->begin(), e = block->end(); i!=e; ++i) {
            
	         Instruction* I = &*i;
	         // We only care about available expressions for BinaryOperators
	         if (BinaryOperator *BI = dyn_cast<BinaryOperator>(I)) 
           {
             if(std::find(expressions.begin(),expressions.end(),Expression(BI)) != expressions.end())
             {
               continue;
             }
             else
             {
               expressions.push_back(Expression(BI));
             }
	         }
	       }
      }
      
      // Print out the expressions used in the function
      outs() << "Expressions used by this function:\n";
      printSet(&expressions);

      BitVector boundaryCond(expressions.size(), false);
      BitVector initCond(expressions.size(), true);
      dataFlow* df = new dataFlow(expressions.size(),INTERSECTION,FORWARD,boundaryCond,initCond);
      genTFDependencies(F,expressions);
      df->executeDataFlowPass(F,bbSets);
      printDFAResults(df->dataFlowHash);
      // Did not modify the incoming Function.
      return false;
    }
    
    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }
    
  private:
    std::map<BasicBlock*,basicBlockDeps*> bbSets;
    std::map<Expression,int> domainMap;
    std::map<int,string> revDomainMap;
    void genTFDependencies(Function &F, std::vector<Expression> domain)
    {
      BitVector empty((int)domain.size(), false);
      int vectorIdx = 0;
      StringRef lhs;
      for(Expression e : domain)
      {
        domainMap[e] = vectorIdx;
        revDomainMap[vectorIdx] = e.toString();
        ++vectorIdx;
      }
      for(BasicBlock &BB : F)
      {
        struct basicBlockDeps* bbSet = new basicBlockDeps();
        bbSet->blockRef = &BB;
        bbSet->genSet = empty;
        bbSet->killSet = empty;
        for(Instruction &II : BB)
        {
          Instruction *I = &II;
          if(dyn_cast<BinaryOperator>(I))
          {
            lhs = I->getName();
            if(std::find(domain.begin(),domain.end(),Expression(I)) != domain.end())
            {
              bbSet->genSet.set(domainMap[Expression(I)]); 
            }
            for(Expression itr: domain)
            {
              StringRef exprOp1 = itr.v1->getName();
              StringRef exprOp2 = itr.v2->getName();
              if(exprOp1.equals(lhs) || exprOp2.equals(lhs))
              {
                bbSet->killSet.set(domainMap[itr]);
                bbSet->genSet.reset(domainMap[itr]);
              }
            }
          }
        }
        bbSets[&BB] = bbSet;
      }
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
        outs() << "gen[BB] : ";
        for(int m = 0; m < (int)temp->genSet.size(); m++)
        {
          if(temp->genSet[m])
          {
            genbb.push_back(revDomainMap[m]);
          }
        }
        printStringSet(&genbb);
        std::vector<string> killbb;
        outs() << "kill[BB] : ";
        for(int n = 0; n < (int)temp->killSet.size(); n++)
        {
          if(temp->killSet[n])
          {
            killbb.push_back(revDomainMap[n]);
          }
        }
        printStringSet(&killbb);
        std::vector<string> inbb;
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
  
  char AvailableExpressions::ID = 0;
  RegisterPass<AvailableExpressions> X("available",
				       "ECE 5984 Available Expressions");
}
