@echo off
md vm
cd vm

set cc=call ..\..\compile.bat -DCGAME

if "%1"=="TA" goto TA

%cc% ../../game/bg_misc.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_pmove.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_slidemove.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_lib.c
@if errorlevel 1 goto quit
%cc% ../../game/q_math.c
@if errorlevel 1 goto quit
%cc% ../../game/q_shared.c
@if errorlevel 1 goto quit
%cc% ../cg_consolecmds.c
@if errorlevel 1 goto quit
%cc% ../cg_draw.c
@if errorlevel 1 goto quit
%cc% ../cg_drawtools.c
@if errorlevel 1 goto quit
%cc% ../cg_effects.c
@if errorlevel 1 goto quit
%cc% ../cg_ents.c
@if errorlevel 1 goto quit
%cc% ../cg_event.c
@if errorlevel 1 goto quit
%cc% ../cg_info.c
@if errorlevel 1 goto quit
%cc% ../cg_localents.c
@if errorlevel 1 goto quit
%cc% ../cg_main.c
@if errorlevel 1 goto quit
%cc% ../cg_marks.c
@if errorlevel 1 goto quit
%cc% ../cg_players.c
@if errorlevel 1 goto quit
%cc% ../cg_playerstate.c
@if errorlevel 1 goto quit
%cc% ../cg_predict.c
@if errorlevel 1 goto quit
%cc% ../cg_scoreboard.c
@if errorlevel 1 goto quit
%cc% ../cg_servercmds.c
@if errorlevel 1 goto quit
%cc% ../cg_snapshot.c
@if errorlevel 1 goto quit
%cc% ../cg_view.c
@if errorlevel 1 goto quit
%cc% ../cg_weapons.c
@if errorlevel 1 goto quit
%cc% ../cg_auras.c
@if errorlevel 1 goto quit
%cc% ../cg_userweapons.c
@if errorlevel 1 goto quit
%cc% ../cg_beamtables.c
@if errorlevel 1 goto quit
%cc% ../cg_particlesystem.c
@if errorlevel 1 goto quit
%cc% ../cg_particlesystem_cache.c
@if errorlevel 1 goto quit
%cc% ../cg_trails.c
@if errorlevel 1 goto quit
%cc% ../cg_radar.c
@if errorlevel 1 goto quit
%cc% ../cg_weapGfxParser.c
@if errorlevel 1 goto quit
%cc% ../cg_weapGfxScanner.c
@if errorlevel 1 goto quit
%cc% ../cg_weapGfxAttributes.c
@if errorlevel 1 goto quit
%cc% ../cg_tiers.c
@if errorlevel 1 goto quit
%cc% ../cg_frameHist.c
@if errorlevel 1 goto quit
%cc% ../cg_motionblur.c
@if errorlevel 1 goto quit


..\..\q3asm -f ../cgame
goto quit

:TA

call compile TA ../game/bg_misc.c
@if errorlevel 1 goto quit
call compile TA ../game/bg_pmove.c
@if errorlevel 1 goto quit
call compile TA ../game/bg_slidemove.c
@if errorlevel 1 goto quit
call compile TA ../game/bg_lib.c
@if errorlevel 1 goto quit
call compile TA ../game/q_math.c
@if errorlevel 1 goto quit
call compile TA ../game/q_shared.c
@if errorlevel 1 goto quit
call compile TA cg_consolecmds.c
@if errorlevel 1 goto quit
call compile TA cg_draw.c
@if errorlevel 1 goto quit
call compile TA cg_drawtools.c
@if errorlevel 1 goto quit
call compile TA cg_effects.c
@if errorlevel 1 goto quit
call compile TA cg_ents.c
@if errorlevel 1 goto quit
call compile TA cg_event.c
@if errorlevel 1 goto quit
call compile TA cg_info.c
@if errorlevel 1 goto quit
call compile TA cg_localents.c
@if errorlevel 1 goto quit
call compile TA cg_main.c
@if errorlevel 1 goto quit
call compile TA cg_marks.c
@if errorlevel 1 goto quit
 call compile TA cg_players.c
@if errorlevel 1 goto quit
call compile TA cg_playerstate.c
@if errorlevel 1 goto quit
call compile TA cg_predict.c
@if errorlevel 1 goto quit
call compile TA cg_scoreboard.c
@if errorlevel 1 goto quit
call compile TA cg_servercmds.c
@if errorlevel 1 goto quit
call compile TA cg_snapshot.c
@if errorlevel 1 goto quit
call compile TA cg_view.c
@if errorlevel 1 goto quit
call compile TA cg_weapons.c
@if errorlevel 1 goto quit
call compile TA ../ui/ui_shared.c
@if errorlevel 1 goto quit
call compile TA cg_newDraw.c
@if errorlevel 1 goto quit

q3asm -f ../cgame_ta

:quit
del *.asm
cd ..
rd vm
