#! /usr/bin/python3

from ilswig import *
import ilswig
X1 = ilswig.JSONIdentifier("X1")
X2 = ilswig.JSONIdentifier("X2")
_input = ilswig.Input()
callA1 = CallAction(X1, "A1", _input)
callA2 = CallAction(X2, "A2", X1)
v = VectorSimpleCommand()
v.push_back(callA1)
v.push_back(callA2)
allCmds = ComplexCommand (v)
seq = Converter.convert (allCmds)
seq._print()

