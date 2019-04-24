#include <ast.h>
#include <iostream>

int main ()
{
//test0  
  {
    ComplexCommand cmds;
    JSONInput input;
    JSONIdentifier X1("X1"), X2("X2"), X3("X3"), X4("X4"), X5("X5");
    Action A1 ("A1"), A2 ("A2"), A3 ("A3");
    cmds (A1 (&X1, &input));
    //auto pq = X1[0]["x"];
    //auto qq = pq;
    cmds (new JSONAssignment (&X2, &X1["x"]["y"]["z"][0]));
    cmds (new JSONAssignment (&X3, &X1["x"]["y"][0]["c"]));
    cmds (A1 (&X2, &X2));
    cmds (new JSONAssignment (&X5, &X2["X2_a"]["X2_b"]["X2_c"]));
    IfThenElseCommand ifthen (&(X5 == X3));
    WhileLoop loop (&(X1 < 10), new CallAction (&X1, "A2", &X1));
    cmds (&ifthen);
    //~ ifthen.thenStart ()
    ifthen.getThenBranch() (A2 (&X3, &X5));
    ifthen.getThenBranch() (&loop);
    //~ifthen.thenEnd ()
    //~ifthen.else()
    ifthen.getElseBranch() (A2 (&X4, &X5));
    convertToWhiskCommands (cmds, std::cout, true, true);
  }
}
