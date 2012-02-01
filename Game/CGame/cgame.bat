@echo off
md vm
cd vm

set cc=call ..\..\..\Tools\Compiler\Perform.bat -DCGAME

if "%1"=="TA" goto TA

%cc% ../../Game/bg_misc.c
@if errorlevel 1 goto quit
%cc% ../../Game/bg_pmove.c
@if errorlevel 1 goto quit
%cc% ../../Game/bg_slidemove.c
@if errorlevel 1 goto quit
%cc% ../../Game/bg_lib.c
@if errorlevel 1 goto quit
%cc% ../../../Shared/q_math.c
@if errorlevel 1 goto quit
%cc% ../../../Shared/q_shared.c
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
%cc% ../cg_music.c
@if errorlevel 1 goto quit
%cc% ../cg_frameHist.c
@if errorlevel 1 goto quit
%cc% ../cg_motionblur.c
@if errorlevel 1 goto quit


..\..\..\Tools\Compiler\q3asm -f ../cgame
:quit
del *.asm
cd ..
rd vm