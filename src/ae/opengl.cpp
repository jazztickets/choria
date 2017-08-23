/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/opengl.h>
#include <SDL_video.h>

// Setup function pointers
void InitGLFunctions() {
#ifdef _WIN32
	glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)SDL_GL_GetProcAddress("glBlendFuncSeparate");
	glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)SDL_GL_GetProcAddress("glMultiDrawArrays");
	glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)SDL_GL_GetProcAddress("glMultiDrawElements");
	glPointParameterf = (PFNGLPOINTPARAMETERFPROC)SDL_GL_GetProcAddress("glPointParameterf");
	glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)SDL_GL_GetProcAddress("glPointParameterfv");
	glPointParameteri = (PFNGLPOINTPARAMETERIPROC)SDL_GL_GetProcAddress("glPointParameteri");
	glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)SDL_GL_GetProcAddress("glPointParameteriv");
	glGenQueries = (PFNGLGENQUERIESPROC)SDL_GL_GetProcAddress("glGenQueries");
	glDeleteQueries = (PFNGLDELETEQUERIESPROC)SDL_GL_GetProcAddress("glDeleteQueries");
	glIsQuery = (PFNGLISQUERYPROC)SDL_GL_GetProcAddress("glIsQuery");
	glBeginQuery = (PFNGLBEGINQUERYPROC)SDL_GL_GetProcAddress("glBeginQuery");
	glEndQuery = (PFNGLENDQUERYPROC)SDL_GL_GetProcAddress("glEndQuery");
	glGetQueryiv = (PFNGLGETQUERYIVPROC)SDL_GL_GetProcAddress("glGetQueryiv");
	glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)SDL_GL_GetProcAddress("glGetQueryObjectiv");
	glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)SDL_GL_GetProcAddress("glGetQueryObjectuiv");
	glBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
	glGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	glIsBuffer = (PFNGLISBUFFERPROC)SDL_GL_GetProcAddress("glIsBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
	glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glGetBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)SDL_GL_GetProcAddress("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)SDL_GL_GetProcAddress("glUnmapBuffer");
	glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)SDL_GL_GetProcAddress("glGetBufferParameteriv");
	glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)SDL_GL_GetProcAddress("glGetBufferPointerv");
	glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)SDL_GL_GetProcAddress("glBlendEquationSeparate");
	glDrawBuffers = (PFNGLDRAWBUFFERSPROC)SDL_GL_GetProcAddress("glDrawBuffers");
	glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)SDL_GL_GetProcAddress("glStencilOpSeparate");
	glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)SDL_GL_GetProcAddress("glStencilFuncSeparate");
	glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)SDL_GL_GetProcAddress("glStencilMaskSeparate");
	glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glBindAttribLocation");
	glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)SDL_GL_GetProcAddress("glDetachShader");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glDisableVertexAttribArray");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)SDL_GL_GetProcAddress("glGetActiveAttrib");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)SDL_GL_GetProcAddress("glGetActiveUniform");
	glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)SDL_GL_GetProcAddress("glGetAttachedShaders");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glGetAttribLocation");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
	glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)SDL_GL_GetProcAddress("glGetShaderSource");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
	glGetUniformfv = (PFNGLGETUNIFORMFVPROC)SDL_GL_GetProcAddress("glGetUniformfv");
	glGetUniformiv = (PFNGLGETUNIFORMIVPROC)SDL_GL_GetProcAddress("glGetUniformiv");
	glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)SDL_GL_GetProcAddress("glGetVertexAttribdv");
	glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)SDL_GL_GetProcAddress("glGetVertexAttribfv");
	glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)SDL_GL_GetProcAddress("glGetVertexAttribiv");
	glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)SDL_GL_GetProcAddress("glGetVertexAttribPointerv");
	glIsProgram = (PFNGLISPROGRAMPROC)SDL_GL_GetProcAddress("glIsProgram");
	glIsShader = (PFNGLISSHADERPROC)SDL_GL_GetProcAddress("glIsShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
	glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
	glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
	glUniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
	glUniform3f = (PFNGLUNIFORM3FPROC)SDL_GL_GetProcAddress("glUniform3f");
	glUniform4f = (PFNGLUNIFORM4FPROC)SDL_GL_GetProcAddress("glUniform4f");
	glUniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
	glUniform2i = (PFNGLUNIFORM2IPROC)SDL_GL_GetProcAddress("glUniform2i");
	glUniform3i = (PFNGLUNIFORM3IPROC)SDL_GL_GetProcAddress("glUniform3i");
	glUniform4i = (PFNGLUNIFORM4IPROC)SDL_GL_GetProcAddress("glUniform4i");
	glUniform1fv = (PFNGLUNIFORM1FVPROC)SDL_GL_GetProcAddress("glUniform1fv");
	glUniform2fv = (PFNGLUNIFORM2FVPROC)SDL_GL_GetProcAddress("glUniform2fv");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)SDL_GL_GetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)SDL_GL_GetProcAddress("glUniform4fv");
	glUniform1iv = (PFNGLUNIFORM1IVPROC)SDL_GL_GetProcAddress("glUniform1iv");
	glUniform2iv = (PFNGLUNIFORM2IVPROC)SDL_GL_GetProcAddress("glUniform2iv");
	glUniform3iv = (PFNGLUNIFORM3IVPROC)SDL_GL_GetProcAddress("glUniform3iv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)SDL_GL_GetProcAddress("glUniform4iv");
	glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)SDL_GL_GetProcAddress("glUniformMatrix2fv");
	glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)SDL_GL_GetProcAddress("glUniformMatrix3fv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
	glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)SDL_GL_GetProcAddress("glVertexAttrib1d");
	glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)SDL_GL_GetProcAddress("glVertexAttrib1dv");
	glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)SDL_GL_GetProcAddress("glVertexAttrib1f");
	glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)SDL_GL_GetProcAddress("glVertexAttrib1fv");
	glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)SDL_GL_GetProcAddress("glVertexAttrib1s");
	glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)SDL_GL_GetProcAddress("glVertexAttrib1sv");
	glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)SDL_GL_GetProcAddress("glVertexAttrib2d");
	glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)SDL_GL_GetProcAddress("glVertexAttrib2dv");
	glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)SDL_GL_GetProcAddress("glVertexAttrib2f");
	glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)SDL_GL_GetProcAddress("glVertexAttrib2fv");
	glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)SDL_GL_GetProcAddress("glVertexAttrib2s");
	glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)SDL_GL_GetProcAddress("glVertexAttrib2sv");
	glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)SDL_GL_GetProcAddress("glVertexAttrib3d");
	glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)SDL_GL_GetProcAddress("glVertexAttrib3dv");
	glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)SDL_GL_GetProcAddress("glVertexAttrib3f");
	glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)SDL_GL_GetProcAddress("glVertexAttrib3fv");
	glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)SDL_GL_GetProcAddress("glVertexAttrib3s");
	glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)SDL_GL_GetProcAddress("glVertexAttrib3sv");
	glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nbv");
	glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Niv");
	glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nsv");
	glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nub");
	glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nubv");
	glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nuiv");
	glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)SDL_GL_GetProcAddress("glVertexAttrib4Nusv");
	glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)SDL_GL_GetProcAddress("glVertexAttrib4bv");
	glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)SDL_GL_GetProcAddress("glVertexAttrib4d");
	glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)SDL_GL_GetProcAddress("glVertexAttrib4dv");
	glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)SDL_GL_GetProcAddress("glVertexAttrib4f");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)SDL_GL_GetProcAddress("glVertexAttrib4fv");
	glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)SDL_GL_GetProcAddress("glVertexAttrib4iv");
	glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)SDL_GL_GetProcAddress("glVertexAttrib4s");
	glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)SDL_GL_GetProcAddress("glVertexAttrib4sv");
	glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)SDL_GL_GetProcAddress("glVertexAttrib4ubv");
	glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)SDL_GL_GetProcAddress("glVertexAttrib4uiv");
	glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)SDL_GL_GetProcAddress("glVertexAttrib4usv");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
	glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)SDL_GL_GetProcAddress("glUniformMatrix2x3fv");
	glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)SDL_GL_GetProcAddress("glUniformMatrix3x2fv");
	glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix2x4fv");
	glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4x2fv");
	glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix3x4fv");
	glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4x3fv");
#endif
}

#ifdef _WIN32
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays;
PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements;
PFNGLPOINTPARAMETERFPROC glPointParameterf;
PFNGLPOINTPARAMETERFVPROC glPointParameterfv;
PFNGLPOINTPARAMETERIPROC glPointParameteri;
PFNGLPOINTPARAMETERIVPROC glPointParameteriv;
PFNGLGENQUERIESPROC glGenQueries;
PFNGLDELETEQUERIESPROC glDeleteQueries;
PFNGLISQUERYPROC glIsQuery;
PFNGLBEGINQUERYPROC glBeginQuery;
PFNGLENDQUERYPROC glEndQuery;
PFNGLGETQUERYIVPROC glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLISBUFFERPROC glIsBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;
PFNGLDRAWBUFFERSPROC glDrawBuffers;
PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate;
PFNGLSTENCILFUNCSEPARATEPROC glStencilFuncSeparate;
PFNGLSTENCILMASKSEPARATEPROC glStencilMaskSeparate;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERSOURCEPROC glGetShaderSource;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLGETUNIFORMFVPROC glGetUniformfv;
PFNGLGETUNIFORMIVPROC glGetUniformiv;
PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv;
PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv;
PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv;
PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
PFNGLISPROGRAMPROC glIsProgram;
PFNGLISSHADERPROC glIsShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM3IPROC glUniform3i;
PFNGLUNIFORM4IPROC glUniform4i;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d;
PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv;
PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s;
PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv;
PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d;
PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv;
PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s;
PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv;
PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d;
PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv;
PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s;
PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv;
PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv;
PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv;
PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv;
PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub;
PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv;
PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv;
PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv;
PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d;
PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv;
PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s;
PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv;
PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv;
PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv;
PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;
#endif
