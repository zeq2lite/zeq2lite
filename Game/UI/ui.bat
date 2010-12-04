@echo off
md vm
cd vm

set cc=call ..\..\..\Tools\Compiler\Perform.bat

%cc% ../ui_shared.c
@if errorlevel 1 goto quit
%cc% ../ui_main.c
@if errorlevel 1 goto quit
%cc% ../ui_ingame.c
@if errorlevel 1 goto quit
%cc% ../ui_serverinfo.c
@if errorlevel 1 goto quit
%cc% ../ui_confirm.c
@if errorlevel 1 goto quit
%cc% ../ui_setup.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_misc.c
@if errorlevel 1 goto quit
%cc% ../../game/bg_lib.c
@if errorlevel 1 goto quit
%cc% ../../../Shared/q_math.c
@if errorlevel 1 goto quit
%cc% ../../../Shared/q_shared.c
@if errorlevel 1 goto quit
%cc% ../ui_gameinfo.c
@if errorlevel 1 goto quit
%cc% ../ui_atoms.c
@if errorlevel 1 goto quit
%cc% ../ui_connect.c
@if errorlevel 1 goto quit
%cc% ../ui_controls2.c
@if errorlevel 1 goto quit
%cc% ../ui_mfield.c
@if errorlevel 1 goto quit
%cc% ../ui_credits.c
@if errorlevel 1 goto quit
%cc% ../ui_menu.c
@if errorlevel 1 goto quit
%cc% ../ui_options.c
@if errorlevel 1 goto quit
%cc% ../ui_display.c
@if errorlevel 1 goto quit
%cc% ../ui_sound.c
@if errorlevel 1 goto quit
%cc% ../ui_network.c
@if errorlevel 1 goto quit
%cc% ../ui_playermodel.c
@if errorlevel 1 goto quit
%cc% ../ui_players.c
@if errorlevel 1 goto quit
%cc% ../ui_playersettings.c
@if errorlevel 1 goto quit
%cc% ../ui_preferences.c
@if errorlevel 1 goto quit
%cc% ../ui_qmenu.c
@if errorlevel 1 goto quit
%cc% ../ui_servers2.c
@if errorlevel 1 goto quit
%cc% ../ui_specifyserver.c
@if errorlevel 1 goto quit
%cc% ../ui_startserver.c
@if errorlevel 1 goto quit
%cc% ../ui_camera.c
@if errorlevel 1 goto quit
%cc% ../ui_video.c
@if errorlevel 1 goto quit

..\..\..\Tools\Compiler\q3asm -f ../ui
del *.asm
:quit
cd ..
rd vm