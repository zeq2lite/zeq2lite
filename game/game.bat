@echo off
md vm
cd vm

set cc=call ..\..\compile.bat
if "%1"=="TA" goto TA

%cc%  ../g_main.c
@if errorlevel 1 goto quit
%cc%  ../g_syscalls.c
@if errorlevel 1 goto quit
%cc%  ../bg_misc.c
@if errorlevel 1 goto quit
%cc%  ../bg_lib.c
@if errorlevel 1 goto quit
%cc%  ../bg_pmove.c
@if errorlevel 1 goto quit
%cc%  ../bg_slidemove.c
@if errorlevel 1 goto quit
%cc%  ../q_math.c
@if errorlevel 1 goto quit
%cc%  ../q_shared.c
@if errorlevel 1 goto quit
%cc%  ../ai_dmnet.c
@if errorlevel 1 goto quit
%cc%  ../ai_dmq3.c
@if errorlevel 1 goto quit
%cc%  ../ai_main.c
@if errorlevel 1 goto quit
%cc%  ../ai_chat.c
@if errorlevel 1 goto quit
%cc%  ../ai_cmd.c
@if errorlevel 1 goto quit
%cc%  ../ai_team.c
@if errorlevel 1 goto quit
%cc%  ../g_active.c
@if errorlevel 1 goto quit
%cc%  ../g_arenas.c
@if errorlevel 1 goto quit
%cc%  ../g_bot.c
@if errorlevel 1 goto quit
%cc%  ../g_client.c
@if errorlevel 1 goto quit
%cc%  ../g_cmds.c
@if errorlevel 1 goto quit
%cc%  ../g_combat.c
@if errorlevel 1 goto quit
%cc%  ../g_items.c
@if errorlevel 1 goto quit
%cc%  ../g_mem.c
@if errorlevel 1 goto quit
%cc%  ../g_misc.c
@if errorlevel 1 goto quit
%cc%  ../g_missile.c
@if errorlevel 1 goto quit
%cc%  ../g_mover.c
@if errorlevel 1 goto quit
%cc%  ../g_session.c
@if errorlevel 1 goto quit
%cc%  ../g_spawn.c
@if errorlevel 1 goto quit
%cc%  ../g_svcmds.c
@if errorlevel 1 goto quit
%cc%  ../g_target.c
@if errorlevel 1 goto quit
%cc%  ../g_team.c
@if errorlevel 1 goto quit
%cc%  ../g_trigger.c
@if errorlevel 1 goto quit
%cc%  ../g_utils.c
@if errorlevel 1 goto quit
%cc%  ../g_weapon.c
@if errorlevel 1 goto quit
%cc%  ../ai_vcmd.c
@if errorlevel 1 goto quit
%cc%  ../g_usermissile.c
@if errorlevel 1 goto quit
%cc%  ../g_userweapons.c
@if errorlevel 1 goto quit
%cc%  ../g_radar.c
@if errorlevel 1 goto quit
%cc%  ../g_weapPhysParser.c
@if errorlevel 1 goto quit
%cc%  ../g_weapPhysScanner.c
@if errorlevel 1 goto quit
%cc%  ../g_weapPhysAttributes.c
@if errorlevel 1 goto quit

..\..\q3asm -f ../game
goto quit

:TA

call compile TA  g_main.c
@if errorlevel 1 goto quit
call compile TA bg_misc.c
@if errorlevel 1 goto quit
call compile TA bg_lib.c
@if errorlevel 1 goto quit
call compile TA bg_pmove.c
@if errorlevel 1 goto quit
call compile TA bg_slidemove.c
@if errorlevel 1 goto quit
call compile TA q_math.c
@if errorlevel 1 goto quit
call compile TA q_shared.c
@if errorlevel 1 goto quit
call compile TA ai_dmnet.c
@if errorlevel 1 goto quit
call compile TA ai_dmq3.c
@if errorlevel 1 goto quit
call compile TA ai_main.c
@if errorlevel 1 goto quit
call compile TA ai_chat.c
@if errorlevel 1 goto quit
call compile TA ai_cmd.c
@if errorlevel 1 goto quit
call compile TA ai_team.c
@if errorlevel 1 goto quit
call compile TA g_active.c
@if errorlevel 1 goto quit
call compile TA g_arenas.c
@if errorlevel 1 goto quit
call compile TA g_bot.c
@if errorlevel 1 goto quit
call compile TA g_client.c
@if errorlevel 1 goto quit
call compile TA g_cmds.c
@if errorlevel 1 goto quit
call compile TA g_combat.c
@if errorlevel 1 goto quit
call compile TA g_items.c
@if errorlevel 1 goto quit
call compile TA g_mem.c
@if errorlevel 1 goto quit
call compile TA g_misc.c
@if errorlevel 1 goto quit
call compile TA g_missile.c
@if errorlevel 1 goto quit
call compile TA g_mover.c
@if errorlevel 1 goto quit
call compile TA g_session.c
@if errorlevel 1 goto quit
call compile TA g_spawn.c
@if errorlevel 1 goto quit
call compile TA g_svcmds.c
@if errorlevel 1 goto quit
call compile TA g_target.c
@if errorlevel 1 goto quit
call compile TA g_team.c
@if errorlevel 1 goto quit
call compile TA g_trigger.c
@if errorlevel 1 goto quit
call compile TA g_utils.c
@if errorlevel 1 goto quit
call compile TA g_weapon.c
@if errorlevel 1 goto quit
call compile TA ai_vcmd.c
@if errorlevel 1 goto quit

q3asm -f ../game

:quit
del *.asm
cd ..
rd vm