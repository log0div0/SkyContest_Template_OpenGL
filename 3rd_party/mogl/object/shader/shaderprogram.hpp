////////////////////////////////////////////////////////////////////////////////
/// Modern OpenGL Wrapper
///
/// Copyright (c) 2015 Thibault Schueller
/// This file is distributed under the MIT License
///
/// @file shaderprogram.hpp
/// @author Thibault Schueller <ryp.sqrt@gmail.com>
////////////////////////////////////////////////////////////////////////////////

#ifndef MOGL_SHADERPROGRAM_INCLUDED
#define MOGL_SHADERPROGRAM_INCLUDED

#include <map>

#include <mogl/object/handle.hpp>
#include <mogl/object/shader/shader.hpp>

namespace mogl
{
    class ShaderProgram : public Handle<GLuint>
    {
    public:
        ShaderProgram();
        ~ShaderProgram();

        ShaderProgram(ShaderProgram&& other) = default;
        ShaderProgram& operator=(ShaderProgram&& other) = default;

    public:
        void                attach(const Shader& object);
        void                detach(const Shader& object);
        void                bindAttribLocation(GLuint location, const std::string& attribute);
        bool                link();
        void                use();
        const std::string&  getLog() const;
        GLint               getAttribLocation(const std::string& name) const;
        GLint               getUniformLocation(const std::string& name) const;
        void                setTransformFeedbackVaryings(GLsizei count,
                                                         const char** varyings,
                                                         GLenum bufferMode);

    public:
        void    setVertexAttribPointer(GLuint location,
                                       GLint size,
                                       GLenum type,
                                       GLboolean normalized = GL_FALSE,
                                       GLsizei stride = 0,
                                       const GLvoid* pointerOffset = nullptr);
        void    setVertexAttribPointer(const std::string& name,
                                       GLint size,
                                       GLenum type,
                                       GLboolean normalized = GL_FALSE,
                                       GLsizei stride = 0,
                                       const GLvoid* pointerOffset = nullptr);

    public:
        template <class T> void setUniform(const std::string& name, T v1);
        template <class T> void setUniform(const std::string& name, T v1, T v2);
        template <class T> void setUniform(const std::string& name, T v1, T v2, T v3);
        template <class T> void setUniform(const std::string& name, T v1, T v2, T v3, T v4);
        template <std::size_t Size, class T>
        void    setUniformPtr(const std::string& name, const T* ptr, GLsizei count = 1);
        template <std::size_t Columns, std::size_t Rows, class T>
        void    setUniformMatrixPtr(const std::string& name, const T* ptr, GLboolean transpose = GL_FALSE, GLsizei count = 1);
        template <std::size_t Size, class T>
        void    setUniformMatrixPtr(const std::string& name, const T* ptr, GLboolean transpose = GL_FALSE, GLsizei count = 1);

    public:
        void    setUniformSubroutine(GLenum type, const std::string& uniform, const std::string& subroutine);

    public:
        void    printDebug();
        void    get(GLenum property, GLint* value); // Direct call to glGetProgramiv()
        GLint   get(GLenum property);
        void    set(GLenum property, GLint value);
        bool    isValid() const override final;

    private:
        void    retrieveLocations();
        void    retrieveSubroutines(GLenum type);

    private:
        using HandleMap = std::map<std::string, GLint>;
        struct SubroutineUniform {
            GLuint      uniform;
            HandleMap   subroutines;
        };
        using SubroutineMap = std::map<std::string, SubroutineUniform>;
        using ShaderSubroutineMap = std::map<GLenum, SubroutineMap>;

        std::string         _log;
        HandleMap           _attribs;
        HandleMap           _uniforms;
        ShaderSubroutineMap _subroutines;
    };
}

#include "shaderprogram.inl"

#endif // MOGL_SHADERPROGRAM_INCLUDED
