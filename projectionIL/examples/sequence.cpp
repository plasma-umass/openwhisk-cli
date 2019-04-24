#include <ast.h>
#include <iostream>

int main ()
{
  //Sequence of 10
  {
    ComplexCommand cmds;
    JSONInput input;
    JSONIdentifier X1 ("X1");
    Action A1 ("A1");
    Action A2 ("A1");
    Action A3 ("A1");
    Action A4 ("A1");
    Action A5 ("A1");
    Action A6 ("A1");
    Action A7 ("A1");
    Action A8 ("A1");
    Action A9 ("A1");
    Action A10 ("A1");
    cmds (A1 (&X1, &input));
    cmds (A2 (&X1, &X1));
    cmds (A3 (&X1, &X1));
    cmds (A4 (&X1, &X1));
    cmds (A5 (&X1, &X1));
    cmds (A6 (&X1, &X1));
    cmds (A7 (&X1, &X1));
    cmds (A8 (&X1, &X1));
    cmds (A9 (&X1, &X1));
    cmds (A10 (&X1, &X1));
    ReturnJSON r (&X1);
    cmds (&r);
    convertToWhiskCommands (cmds, std::cout, true);
    std::cout << std::endl;
  }
  
  return 0;
  /*
  //test0  
  {
    ComplexCommand cmds;
    JSONInput input;
    JSONIdentifier X1("X1"), X2("X2"), X3("X3"), X4("X4"), X5("X5");
    Action A1 ("A1"), A2 ("A2"), A3 ("A3");
    cmds (A1 (&X1, &input));
    //auto pq = X1[0]["x"];
    //auto qq = pq;
    cmds (new JSONAssignment (&X2, &X1["x"]["y"]["z"]));
    cmds (new JSONAssignment (&X3, &X1["x"]["y"]["c"]));
    cmds (A1 (&X2, &X2));
    cmds (new JSONAssignment (&X5, &X2["X2_a"]["X2_b"]["X2_c"]));
    IfThenElseCommand ifthen (&(X5 == X3));
    cmds (&ifthen);
    //~ ifthen.thenStart ()
    ifthen.getThenBranch() (A2 (&X3, &X5));
    IfThenElseCommand ifThen2 (&(X3 == X1));
    ifThen2.getThenBranch () (A3 (&X3, &X1));
    ReturnJSON r (&X3);
    ifThen2.getThenBranch () (&r);
    ifthen.getThenBranch() (&ifThen2);
    //~ifthen.thenEnd ()
    //~ifthen.else()
    ifthen.getElseBranch() (A2 (&X4, &X5));
    Program* program = convertToSSA (&cmds, true);
    optimize (program);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::vector<WhiskSequence*> seqs;
    WhiskProgram* p = (WhiskProgram*)program->convert (program, seqs);
    p->generateCommand (std::cout);
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  return 0;
  //test1
  {
    //~ // X1 = A1 (input)
    //~ // X2 = A2 (X1)
    std::cout << "Test 1" << std::endl;
    JSONIdentifier X1("X1"), X2("X2");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&CallA2);
    ComplexCommand allCmds(v);
    
    Program* program = convertToSSA (&allCmds);
    optimize (program);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::vector<LLSPLSequence*> seqs;
    LLSPLProgram* p = (LLSPLProgram*)program->convertToLLSPL (seqs);
    //p->generateCommand (std::cout);
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  
  //~ //test2
  identifiers.clear ();
   {
    //X1 = A1 (input)
    //if X1 then X2 = A2(X1) else X2 = A3(X1)
    //X3 = A4 (X2)
    std::cout << "If then else "<< std::endl;
    JSONIdentifier X1("X1"), X2("X2"), X3 ("X3");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    CallAction CallA3(&X2, "A3", &X1);
    IfThenElseCommand ifX1(&(X1==0), &CallA2, &CallA3);
    CallAction CallA4(&X3, "A4", &X2);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&ifX1);
    v.push_back(&CallA4);
    
    ComplexCommand cmd1(v);
    
    Program* program = convertToSSA (&cmd1);
    //optimize (program);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::cout << std::endl;
    
    std::vector<WhiskSequence*> seqs;
    WhiskAction * p = program->convert (program, seqs);
    p->generateCommand (std::cout);
    
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  
  return 0;
  
  identifiers.clear ();
   {
    //X1 = A1 (input)
    //if X1 then X2 = A2(X1) else X2 = A3(X1)
    //X3 = A4 (X2)
    std::cout << "If then else "<< std::endl;
    JSONIdentifier X1("X1"), X2("X2"), X3 ("X3");
    JSONInput input;
    CallAction CallA1(&X1, "A1", &input);
    CallAction CallA2(&X2, "A2", &X1);
    CallAction CallA3(&X2, "A3", &X1);
    JSONPatternApplication cond (&X1, new FieldGetJSONPattern ("text"));
    IfThenElseCommand ifX1(&(cond==0), &CallA2, &CallA3);
    CallAction CallA4(&X3, "A4", &X2);
    std::vector<SimpleCommand*> v;
    v.push_back(&CallA1);
    v.push_back(&ifX1);
    v.push_back(&CallA4);
    
    ComplexCommand cmd1(v);
    
    Program* program = convertToSSA (&cmd1);
    optimize (program);
    //convertToSSA (firstBlock);
    //firstBlock->print (std::cout);
    std::cout << std::endl;
    
    std::vector<WhiskSequence*> seqs;
    WhiskAction * p = program->convert (program, seqs);
    p->generateCommand (std::cout);
    
    //~ seqs[0]->print ();
    std::cout << std::endl;
  }
  
  //~ {
    //~ //X1 = A1 (input)
    //~ //if X1 then X2_1 = A2(X1) else X2_2 = A3(X1)
    //~ //X2 = phi(then, X2_1, else, X2_2)
    //~ std::cout << "If then else "<< std::endl;
    //~ JSONIdentifier X1("X1"), X2("X2"), X2_1("X2_1"), X2_2("X2_2");
    //~ Input input;
    //~ CallAction CallA1(&X1, "A1", &input);
    //~ CallAction CallA2(&X2_1, "A2", &X1);
    //~ CallAction CallA3(&X2_2, "A3", &X1);
    //~ IfThenElseCommand ifX1(&X1, &CallA2, &CallA3);
    //~ std::vector<std::pair<Command*, JSONIdentifier*> > phi_v;
    //~ phi_v.push_back(std::make_pair (ifX1.getThenBranch (), &X2_1));
    //~ phi_v.push_back(std::make_pair (ifX1.getElseBranch (), &X2_2));
    //~ PHINode X2Phi(phi_v);
    
    //~ ComplexCommand phi_block;
    //~ phi_block.appendSimpleCommand(&X2Phi);
    //~ ifX1.getThenBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    //~ ifX1.getElseBranch ()->appendSimpleCommand (new DirectBranch (&phi_block));
    
    //~ std::vector<SimpleCommand*> v;
    //~ v.push_back(&CallA1);
    //~ v.push_back(&ifX1);
    
    //~ ComplexCommand cmd1(v);
    
    //~ std::vector<WhiskSequence*> seqs = Converter::convert (&cmd1);
    
    //~ for (auto seq : seqs) {
        //~ seq->generateCommand (std::cout);
        //~ std::cout << std::endl << std::endl;
    //~ }
    //~ //seq->print ();
    //~ std::cout << std::endl;
  //~ }
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test3
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  X1 = A2 (X1)
    //~ //}
    
    std::cout << "While Loop Test" << std::endl;
    JSONIdentifier X1 ("X1");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop (&(X1 < 10), new CallAction (&X1, "A2", &X1));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* program = convertToSSA (&cmd1);
    optimize (program);
    std::vector<WhiskSequence*> seqs;
    program->convert (program, seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test3
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  while (X1) {
    //~ //    X1 = A2 (X1)
    //~ //  }
    //~ //}
    
    std::cout << "Nested While Loop Test" << std::endl;
    JSONIdentifier X1 ("X1"), X2("X2");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop2 (&(X1 < 10), new CallAction (&X1, "A2", &X1));
    WhileLoop loop (&(X1 < 10), &loop2);
    loop.getBody().appendSimpleCommand (new CallAction (&X2, "A3", &X1));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* program = convertToSSA (&cmd1);
    optimize (program);
    std::vector<WhiskSequence*> seqs;
    program->convert (program, seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  BasicBlock::numberOfBasicBlocks = 0;
  identifiers.clear ();
  //test4
  {
    //~ //X1 = A1 (input)
    //~ //while (X1) {
    //~ //  while (X1) {
    //~ //    X1 = A2 (X1)
    //~ //  }
    //~ //}
    
    std::cout << "2 Nested While Loops Test" << std::endl;
    JSONIdentifier X1 ("X1"), X2("X2");
    JSONInput input;
    
    CallAction A1 (&X1, "A1", &input);
    WhileLoop loop2 (&(X1 < 5), new CallAction (&X1, "A2", &X1));
    WhileLoop loop (&(X1 < 5), &loop2);
    loop.getBody().appendSimpleCommand (new CallAction (&X2, "A3", &X1));
    loop.getBody().appendSimpleCommand (new WhileLoop (&(X2 ==0), new CallAction (&X2, "A4", &X1)));
    ComplexCommand cmd1;
    cmd1.appendSimpleCommand (&A1);
    cmd1.appendSimpleCommand (&loop);
    
    Program* program = convertToSSA (&cmd1);
    optimize (program);
    std::vector<WhiskSequence*> seqs;
    program->convert (program, seqs);
    for (auto seq : seqs) {
        seq->generateCommand (std::cout);
        std::cout << std::endl;
    }
    //~ for (auto seq : seqs) {
      //~ seq->print ();
      //~ std::cout << std::endl;
    //~ }
  }
  
  //While Inside If
  //If Inside while
  //If inside if
  */
}
