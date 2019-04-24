#!/bin/bash
echo ". * {\"input\": .saved.input}" > /tmp/wsk-proj-cO1OLL
./wsk -i action update Proj_A1_IhMFSv8kP0 --projection  /tmp/wsk-proj-cO1OLL
./wsk -i action update Fork_A1_zLOZ2nOXpP --fork A1
echo ". * {\"saved\": {\"X1_0\": .input}}" > /tmp/wsk-proj-hoSWlu
./wsk -i action update Proj_L8b7mtKLHI --projection /tmp/wsk-proj-hoSWlu
echo ". * {\"input\": .saved.X1_0}" > /tmp/wsk-proj-3kaVYc
./wsk -i action update Proj_A2_A0GWXIIwo7 --projection  /tmp/wsk-proj-3kaVYc
./wsk -i action update Fork_A2_7U20o0J90x --fork A2
echo ". * {\"saved\": {\"X2_0\": .input}}" > /tmp/wsk-proj-5uWMEV
./wsk -i action update Proj_GhIh5JuWcF --projection /tmp/wsk-proj-5uWMEV
./wsk -i action update Sequence_72tK0LK0e1 --sequence Proj_A1_IhMFSv8kP0,Fork_A1_zLOZ2nOXpP,Proj_L8b7mtKLHI,Proj_A2_A0GWXIIwo7,Fork_A2_7U20o0J90x,Proj_GhIh5JuWcF

./wsk -i action update Program_ETPVzxlFrX --program Sequence_72tK0LK0e1
