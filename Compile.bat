@ECHO OFF
cd Game\CGame
echo Compiling CGAME...
call cgame.bat
cd..
cd Game
echo Compiling GAME...
call game.bat
cd..
cd UI
echo Compiling UI...
call ui.bat
cd ..
pause