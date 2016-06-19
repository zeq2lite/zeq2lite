@echo off

if %~x1 equ .fbx (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .FBX (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .obj (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .OBJ (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .smd (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .SMD (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .md5 (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .MD5 (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .iqe (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .IQE (
	iqm %~n1.iqm %~n1%~x1
	goto end
) else if %~x1 equ .iqm (
	upgrade %~n1
	goto end
) else if %~x1 equ .IQM (
	upgrade %~n1
	goto end
) else (
	goto err
)

:err
	echo Error: only FBX, OBJ, SMD, MD5, IQE and IQM (version 1) formats are supported.
	goto end

:end
	pause