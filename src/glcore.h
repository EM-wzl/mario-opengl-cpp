// glcore.h — 极简 OpenGL 3.3 Core 函数加载器（替代 GLEW）。
// 仅依赖 GLFW 提供的 glfwGetProcAddress 在运行时解析 GL 函数指针，
// 不读取任何外部文件，也不需要 GLEW/GLAD 等第三方加载库。
// 配套实现见 glcore.cpp。
//
// 说明：OpenGL 1.0/1.1 基础函数（纹理、绘制、状态等）由 opengl32 直接导出、
// 并在 <GL/gl.h> 中以普通函数原型声明，无需加载；此处只处理 1.2 及以上、
// 必须经 wglGetProcAddress 解析的现代函数。
// 中文动态图集用到的 glTexSubImage2D 属于 1.1 核心函数，<GL/gl.h> 已声明、
// opengl32 直接导出，调用方直接调用即可；不应在此重复声明同名函数指针
// （会与 gl.h 的函数原型冲突，且 glext.h 并未定义 PFNGLTEXSUBIMAGE2DPROC）。
#pragma once

#include <GL/gl.h>
#include <GL/glext.h>

// 现代 GL 函数声明为全局函数指针，由 loadGL() 在上下文创建后填充。
// 调用代码无需改动，仍按原始函数名书写即可（经由指针调用）。
extern PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC         glBindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays;
extern PFNGLGENBUFFERSPROC              glGenBuffers;
extern PFNGLBINDBUFFERPROC              glBindBuffer;
extern PFNGLBUFFERDATAPROC              glBufferData;
extern PFNGLDELETEBUFFERSPROC           glDeleteBuffers;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer;
extern PFNGLCREATESHADERPROC            glCreateShader;
extern PFNGLSHADERSOURCEPROC            glShaderSource;
extern PFNGLCOMPILESHADERPROC           glCompileShader;
extern PFNGLGETSHADERIVPROC             glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC            glDeleteShader;
extern PFNGLCREATEPROGRAMPROC           glCreateProgram;
extern PFNGLATTACHSHADERPROC            glAttachShader;
extern PFNGLLINKPROGRAMPROC             glLinkProgram;
extern PFNGLGETPROGRAMIVPROC            glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC           glDeleteProgram;
extern PFNGLUSEPROGRAMPROC              glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation;
extern PFNGLUNIFORM1IPROC               glUniform1i;
extern PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv;
extern PFNGLACTIVETEXTUREPROC           glActiveTexture;

// 在创建好 GL 上下文、且当前上下文已绑定后调用。
// 返回是否所有必需函数都已成功解析（缺失任一则视为失败）。
bool loadGL();
