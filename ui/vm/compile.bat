@echo off
echo compiling %1:
lcc -DQ3_VM -DMISSIONPACK -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui ..\%1
