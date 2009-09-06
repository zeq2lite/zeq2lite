set LCCDIR=..\..\lcc\
@..\..\lcc -D_stdcall= -DQ3_VM -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1 %2 %3