#!/bin/bash

echo ". * {\"input\": .saved.input}" > /tmp/wsk-proj-2WAHHW
./wsk -i action update Proj_A1_TxnkWrSaQy --projection /tmp/wsk-proj-2WAHHW
./wsk -i action update Fork_A1_tUbO4bNPB0 --fork A1
echo ". * {\"saved\": {\"X1_0\": .input}}" > /tmp/wsk-proj-TYKNjX
./wsk -i action update Proj_hol4EgN5e4 --projection /tmp/wsk-proj-TYKNjX
echo "if (.saved.X1_0) then (. * {\"action\": \"Sequence_UuEa0X9Q5m\"}) else (. * {\"action\":\"Sequence_HJYTrWz0w2\"})" > /tmp/wsk-proj-82oUVX
./wsk -i action update Proj_9N6L5cdFnD --projection /tmp/wsk-proj-82oUVX
./wsk -i action update Seq_IF_THEN_ELSE_qc4M3tt2un --sequence Proj_9N6L5cdFnD
./wsk -i action update Sequence_UG67LaSXd5 --sequence Proj_A1_TxnkWrSaQy,Fork_A1_tUbO4bNPB0,Proj_hol4EgN5e4,Seq_IF_THEN_ELSE_qc4M3tt2un

echo ". * {\"input\": .saved.X1_0}" > /tmp/wsk-proj-vkV1xY
./wsk -i action update Proj_A2_eBlQtFFJpg --projection /tmp/wsk-proj-vkV1xY
./wsk -i action update Fork_A2_VwG4JLgeip --fork A2
echo ". * {\"saved\": {\"X2_0\": .input}}" > /tmp/wsk-proj-SfX99Y
./wsk -i action update Proj_poSt7VVlkJ --projection /tmp/wsk-proj-SfX99Y
echo ". * {\"action\":\"Sequence_4cDzdiEvq8\"}" > /tmp/wsk-proj-vOriMZ
./wsk -i action update Proj_DirectBranch_gOChWKCDa6 --projection /tmp/wsk-proj-vOriMZ
./wsk -i action update Sequence_UuEa0X9Q5m --sequence Proj_A2_eBlQtFFJpg,Fork_A2_VwG4JLgeip,Proj_poSt7VVlkJ,Proj_DirectBranch_gOChWKCDa6

echo ". * {\"saved\":{\"X2_2\":if (.saved.X2_0 != null) then .saved.X2_0 else ( .saved.X2_1)}}" > /tmp/wsk-proj-Y7zro0
./wsk -i action update Proj_YDamCCSQ7G --projection /tmp/wsk-proj-Y7zro0
echo ". * {\"input\": .saved.X2_2}" > /tmp/wsk-proj-1v8A00
./wsk -i action update Proj_A4_AZ6Cu86nAy --projection /tmp/wsk-proj-1v8A00
./wsk -i action update Fork_A4_vmf7TZAPjU --fork A4
echo ". * {\"saved\": {\"X3_0\": .input}}" > /tmp/wsk-proj-AVeLC1
./wsk -i action update Proj_w5jSYm4TKi --projection /tmp/wsk-proj-AVeLC1
./wsk -i action update Sequence_4cDzdiEvq8 --sequence Proj_YDamCCSQ7G,Proj_A4_AZ6Cu86nAy,Fork_A4_vmf7TZAPjU,Proj_w5jSYm4TKi

echo ". * {\"input\": .saved.X1_0}" > /tmp/wsk-proj-B57Ve2
./wsk -i action update Proj_A3_yWBjw3yCMl --projection /tmp/wsk-proj-B57Ve2
./wsk -i action update Fork_A3_kQw1UFK8u2 --fork A3
echo ". * {\"saved\": {\"X2_1\": .input}}" > /tmp/wsk-proj-eJG7Q2
./wsk -i action update Proj_92Ws4iYQoC --projection /tmp/wsk-proj-eJG7Q2
echo ". * {\"action\":\"Sequence_4cDzdiEvq8\"}" > /tmp/wsk-proj-1HLjt3
./wsk -i action update Proj_DirectBranch_A5NQxJt0jk --projection /tmp/wsk-proj-1HLjt3
./wsk -i action update Sequence_HJYTrWz0w2 --sequence Proj_A3_yWBjw3yCMl,Fork_A3_kQw1UFK8u2,Proj_92Ws4iYQoC,Proj_DirectBranch_A5NQxJt0jk

./wsk -i action update Program_LSWV3bvYgh --program Sequence_UG67LaSXd5,Sequence_UuEa0X9Q5m,Sequence_4cDzdiEvq8,Sequence_HJYTrWz0w2


