ilswig = require('./build/Release/ilswig')
X1 = new ilswig.JSONIdentifier("X1")
X2 = new ilswig.JSONIdentifier("X2")

_input = new ilswig.Input()
callA1 = new ilswig.CallAction(X1, "A1", _input)
callA2 = new ilswig.CallAction(X2, "A2", X1)
v = new ilswig.VectorSimpleCommand()
v.add(callA1)
v.add(callA2)
allCmds = new ilswig.ComplexCommand (v)
seq = new ilswig.Converter.convert (allCmds)
seq.print()

