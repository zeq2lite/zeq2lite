/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmdlib.c

#include "cmdlib.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h>
#include <windows.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#define PATHSEPERATOR   '/'

// set these before calling CheckParm
int myargc;
char **myargv;

char		com_token[1024];
qboolean	com_eof;

/*
=================
Error

For abnormal program terminations in console apps
=================
*/
void Error( const char *error, ...) {
	va_list argptr;

	_printf ("\n************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	_printf ("\r\n");

	exit (1);
}

void _printf( const char *format, ... ) {
	va_list argptr;
  char text[4096];

	va_start (argptr,format);
	vsprintf (text, format, argptr);
	va_end (argptr);

  printf(text);
}

/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (const char *check) {
	int             i;

	for (i = 1;i<myargc;i++) {
		if ( !strcmp(check, myargv[i]) )
			return i;
	}

	return 0;
}

/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

void Q_mkdir (const char *path) {
#ifdef WIN32
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
	if (errno != EEXIST)
		Error ("mkdir %s: %s",path, strerror(errno));
}

/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (const char *path) {
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f) {
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

FILE *SafeOpenWrite (const char *filename) {
	FILE	*f;

	f = fopen(filename, "wb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}

FILE *SafeOpenRead (const char *filename) {
	FILE	*f;

	f = fopen(filename, "rb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}

void SafeRead (FILE *f, void *buffer, int count) {
	if ( fread (buffer, 1, count, f) != (size_t)count)
		Error ("File read failure");
}

void SafeWrite (FILE *f, const void *buffer, int count) {
	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Error ("File write failure");
}

/*
==============
FileExists
==============
*/
qboolean	FileExists (const char *filename) {
	FILE	*f;

	f = fopen (filename, "r");
	if (!f)
		return qfalse;
	fclose (f);
	return qtrue;
}

/*
==============
LoadFile
==============
*/
int    LoadFile( const char *filename, void **bufferptr ) {
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpenRead (filename);
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}

/*
==============
LoadFileBlock
-
rounds up memory allocation to 4K boundry
-
==============
*/
int    LoadFileBlock( const char *filename, void **bufferptr ) {
	FILE	*f;
	int    length, nBlock, nAllocSize;
	void    *buffer;

	f = SafeOpenRead (filename);
	length = Q_filelength (f);
  nAllocSize = length;
  nBlock = nAllocSize % MEM_BLOCKSIZE;
  if ( nBlock > 0) {
    nAllocSize += MEM_BLOCKSIZE - nBlock;
  }
	buffer = malloc (nAllocSize+1);
  memset(buffer, 0, nAllocSize+1);
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}

/*
==============
TryLoadFile

Allows failure
==============
*/
int    TryLoadFile (const char *filename, void **bufferptr) {
	FILE	*f;
	int    length;
	void    *buffer;

	*bufferptr = NULL;

	f = fopen (filename, "rb");
	if (!f)
		return -1;
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}

/*
==============
SaveFile
==============
*/
void    SaveFile (const char *filename, const void *buffer, int count) {
	FILE	*f;

	f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	fclose (f);
}

void DefaultExtension (char *path, const char *extension) {
	char    *src;
//
// if path doesnt have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && *src != '\\' && src != path) {
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}
	if(extension[0] != '.') strcat(path,".");
	strcat (path, extension);
}

void DefaultPath (char *path, const char *basepath) {
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}

void    StripFilename (char *path) {
	int             length;

	length = (int)strlen(path)-1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

void    StripExtension (char *path) {
	int             length;

	length = (int)strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}

/*
====================
Extract file parts
====================
*/
// FIXME: should include the slash, otherwise
// backing to an empty path will be wrong when appending a slash
void ExtractFilePath (const char *path, char *dest) {
	const char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '\\' && *(src-1) != '/')
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (const char *path, char *dest) {
	const char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void ExtractFileExtension (const char *path, char *dest) {
	const char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path) {
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}

/*
============
CreatePath
============
*/
void	CreatePath (const char *path) {
	const char	*ofs;
	char		c;
	char		dir[1024];

#ifdef _WIN32
	int		olddrive = -1;

	if ( path[1] == ':' )
	{
		olddrive = _getdrive();
		_chdrive( toupper( path[0] ) - 'A' + 1 );
	}
#endif

	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++) {
		c = *ofs;
		if (c == '/' || c == '\\') {
			// create the directory
			memcpy( dir, path, ofs - path );
			dir[ ofs - path ] = 0;
			Q_mkdir( dir );
		}
	}

#ifdef _WIN32
	if ( olddrive != -1 ) {
		_chdrive( olddrive );
	}
#endif
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short   LittleShort (short l) {
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   BigShort (short l) {
	return l;
}


int    LittleLong (int l) {
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l) {
	return l;
}


float	LittleFloat (float l) {
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	BigFloat (float l) {
	return l;
}


#else


short   BigShort (short l) {
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   LittleShort (short l) {
	return l;
}


int    BigLong (int l) {
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l) {
	return l;
}

float	BigFloat (float l) {
	union {byte b[4]; float f;} in, out;
	
	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}

float	LittleFloat (float l) {
	return l;
}


#endif
