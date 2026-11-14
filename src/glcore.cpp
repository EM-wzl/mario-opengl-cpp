// glcore.cpp — 极简 OpenGL 3.3 Core 函数加载器实现（替代 GLEW）。
// 用 GLFW 的函数指针解析接口填充 glcore.h 中声明的全局函数指针。
// 注：glTexSubImage2D 属于 OpenGL 1.1 核心函数，<GL/gl.h> 已声明、opengl32
// 直接导出，调用方直接调用即可，不在此重复加载（详见 glcore.h 说明）。
#include "glcore.h"

#include <GLFW/glfw3.h>

// 定义 glcore.h 中声明的函数指针（全工程唯一一处定义，避免多重定义）。
PFNGLGENVERTEXARRAYSPROC         glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC         glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC      glDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC              glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC              glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC              glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC           glDeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC     glVertexAttribPointer = nullptr;
PFNGLCREATESHADERPROC            glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC            glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC           glCompileShader = nullptr;
PFNGLGETSHADERIVPROC             glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC        glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC            glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC           glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC            glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC             glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC            glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC       glGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC           glDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC              glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC      glGetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC               glUniform1i = nullptr;
PFNGLUNIFORMMATRIX4FVPROC        glUniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC           glActiveTexture = nullptr;

namespace {
// 借助宏把函数名与其 PFNGL...PROC 类型一并解析并赋值，减少重复、避免手写错位。
#define LOAD_GL(pfn, name) \
    name = reinterpret_cast<pfn>(getProcAddress(#name))

bool loadAll() {
    // 用 lambda 包装 GLFW 的函数解析接口，供下方宏统一调用。
    auto getProcAddress = [](const char* name) -> GLFWglproc {
        return glfwGetProcAddress(name);
    };

    LOAD_GL(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    LOAD_GL(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    LOAD_GL(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    LOAD_GL(PFNGLGENBUFFERSPROC, glGenBuffers);
    LOAD_GL(PFNGLBINDBUFFERPROC, glBindBuffer);
    LOAD_GL(PFNGLBUFFERDATAPROC, glBufferData);
    LOAD_GL(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    LOAD_GL(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    LOAD_GL(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    LOAD_GL(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_GL(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_GL(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_GL(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_GL(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_GL(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_GL(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_GL(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_GL(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_GL(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    LOAD_GL(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    LOAD_GL(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_GL(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_GL(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GL(PFNGLUNIFORM1IPROC, glUniform1i);
    LOAD_GL(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    LOAD_GL(PFNGLACTIVETEXTUREPROC, glActiveTexture);

    // 抽查关键函数是否成功加载，任一缺失即认为加载失败。
    return glGenVertexArrays != nullptr && glCreateShader != nullptr &&
           glCreateProgram != nullptr && glGenBuffers != nullptr &&
           glActiveTexture != nullptr;
}
#undef LOAD_GL
}  // namespace

bool loadGL() {
    return loadAll();
}
