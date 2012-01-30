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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "../renderer/tr_local.h"


qboolean ( * qwglSwapIntervalEXT)( int interval );
void ( * qglActiveTextureARB )( GLenum texture );


// GL_ARB_shader_objects
GLvoid (APIENTRYP qglDeleteObjectARB) (GLhandleARB obj);
GLhandleARB (APIENTRYP qglGetHandleARB) (GLenum pname);
GLvoid (APIENTRYP qglDetachObjectARB) (GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB (APIENTRYP qglCreateShaderObjectARB) (GLenum shaderType);
GLvoid (APIENTRYP qglShaderSourceARB) (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string,
				       const GLint *length);
GLvoid (APIENTRYP qglCompileShaderARB) (GLhandleARB shaderObj);
GLhandleARB (APIENTRYP qglCreateProgramObjectARB) (void);
GLvoid (APIENTRYP qglAttachObjectARB) (GLhandleARB containerObj, GLhandleARB obj);
GLvoid (APIENTRYP qglLinkProgramARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglUseProgramObjectARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglValidateProgramARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglUniform1fARB) (GLint location, GLfloat v0);
GLvoid (APIENTRYP qglUniform2fARB) (GLint location, GLfloat v0, GLfloat v1);
GLvoid (APIENTRYP qglUniform3fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid (APIENTRYP qglUniform4fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid (APIENTRYP qglUniform1iARB) (GLint location, GLint v0);
GLvoid (APIENTRYP qglUniform2iARB) (GLint location, GLint v0, GLint v1);
GLvoid (APIENTRYP qglUniform3iARB) (GLint location, GLint v0, GLint v1, GLint v2);
GLvoid (APIENTRYP qglUniform4iARB) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GLvoid (APIENTRYP qglUniform1fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform2fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform3fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform4fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform1ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform2ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform3ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform4ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniformMatrix2fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglUniformMatrix3fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglUniformMatrix4fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglGetObjectParameterfvARB) (GLhandleARB obj, GLenum pname, GLfloat *params);
GLvoid (APIENTRYP qglGetObjectParameterivARB) (GLhandleARB obj, GLenum pname, GLint *params);
GLvoid (APIENTRYP qglGetInfoLogARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
GLvoid (APIENTRYP qglGetAttachedObjectsARB) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count,
					     GLhandleARB *obj);
GLint(APIENTRYP qglGetUniformLocationARB) (GLhandleARB programObj, const GLcharARB * name);
GLvoid (APIENTRYP qglGetActiveUniformARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength,
					   GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLvoid (APIENTRYP qglGetUniformfvARB) (GLhandleARB programObj, GLint location, GLfloat *params);
GLvoid (APIENTRYP qglGetUniformivARB) (GLhandleARB programObj, GLint location, GLint *params);
GLvoid (APIENTRYP qglGetShaderSourceARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length,
					  GLcharARB *source);
GLvoid (APIENTRYP qglVertexAttrib1fARB) (GLuint index, GLfloat v0);
GLvoid (APIENTRYP qglVertexAttrib1sARB) (GLuint index, GLshort v0);
GLvoid (APIENTRYP qglVertexAttrib1dARB) (GLuint index, GLdouble v0);
GLvoid (APIENTRYP qglVertexAttrib2fARB) (GLuint index, GLfloat v0, GLfloat v1);
GLvoid (APIENTRYP qglVertexAttrib2sARB) (GLuint index, GLshort v0, GLshort v1);
GLvoid (APIENTRYP qglVertexAttrib2dARB) (GLuint index, GLdouble v0, GLdouble v1);
GLvoid (APIENTRYP qglVertexAttrib3fARB) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid (APIENTRYP qglVertexAttrib3sARB) (GLuint index, GLshort v0, GLshort v1, GLshort v2);
GLvoid (APIENTRYP qglVertexAttrib3dARB) (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2);
GLvoid (APIENTRYP qglVertexAttrib4fARB) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid (APIENTRYP qglVertexAttrib4sARB) (GLuint index, GLshort v0, GLshort v1, GLshort v2, GLshort v3);
GLvoid (APIENTRYP qglVertexAttrib4dARB) (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
GLvoid (APIENTRYP qglVertexAttrib4NubARB) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GLvoid (APIENTRYP qglVertexAttrib1fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib1svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib1dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib2fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib2svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib2dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib3fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib3svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib3dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib4fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib4svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib4dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib4ivARB) (GLuint index, GLint *v);
GLvoid (APIENTRYP qglVertexAttrib4bvARB) (GLuint index, GLbyte *v);
GLvoid (APIENTRYP qglVertexAttrib4ubvARB) (GLuint index, GLubyte *v);
GLvoid (APIENTRYP qglVertexAttrib4usvARB) (GLuint index, GLushort *v);
GLvoid (APIENTRYP qglVertexAttrib4uivARB) (GLuint index, GLuint *v);
GLvoid (APIENTRYP qglVertexAttrib4NbvARB) (GLuint index, const GLbyte *v);
GLvoid (APIENTRYP qglVertexAttrib4NsvARB) (GLuint index, const GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib4NivARB) (GLuint index, const GLint *v);
GLvoid (APIENTRYP qglVertexAttrib4NubvARB) (GLuint index, const GLubyte *v);
GLvoid (APIENTRYP qglVertexAttrib4NusvARB) (GLuint index, const GLushort *v);
GLvoid (APIENTRYP qglVertexAttrib4NuivARB) (GLuint index, const GLuint *v);
GLvoid (APIENTRYP qglVertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized,
					      GLsizei stride, const GLvoid *pointer);
GLvoid (APIENTRYP qglEnableVertexAttribArrayARB) (GLuint index);
GLvoid (APIENTRYP qglDisableVertexAttribArrayARB) (GLuint index);
GLvoid (APIENTRYP qglBindAttribLocationARB) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
GLvoid (APIENTRYP qglGetActiveAttribARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength,
					  GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint(APIENTRYP qglGetAttribLocationARB) (GLhandleARB programObj, const GLcharARB * name);
GLvoid (APIENTRYP qglGetVertexAttribdvARB) (GLuint index, GLenum pname, GLdouble *params);
GLvoid (APIENTRYP qglGetVertexAttribfvARB) (GLuint index, GLenum pname, GLfloat *params);
GLvoid (APIENTRYP qglGetVertexAttribivARB) (GLuint index, GLenum pname, GLint *params);
GLvoid (APIENTRYP qglGetVertexAttribPointervARB) (GLuint index, GLenum pname, GLvoid **pointer);


void		GLimp_EndFrame( void ) {
}

int 		GLimp_Init( void )
{
}

void		GLimp_Shutdown( void ) {
}

void		GLimp_EnableLogging( qboolean enable ) {
}

void GLimp_LogComment( char *comment ) {
}

qboolean QGL_Init( const char *dllname ) {
	return qtrue;
}

void		QGL_Shutdown( void ) {
}
