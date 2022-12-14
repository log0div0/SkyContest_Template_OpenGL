////////////////////////////////////////////////////////////////////////////////
/// Modern OpenGL Wrapper
///
/// Copyright (c) 2015 Thibault Schueller
/// This file is distributed under the MIT License
///
/// @file shaderprogram.inl
/// @author Thibault Schueller <ryp.sqrt@gmail.com>
////////////////////////////////////////////////////////////////////////////////

#include <mogl/exception/shaderexception.hpp>

#include <iostream>
#include <iomanip>
#include <algorithm>

namespace mogl
{
    inline ShaderProgram::ShaderProgram()
    :   Handle(GL_PROGRAM)
    {
        _handle = glCreateProgram();
    }

    inline ShaderProgram::~ShaderProgram()
    {
        if (_handle)
            glDeleteProgram(_handle);
    }

    inline void ShaderProgram::attach(const Shader& object)
    {
        glAttachShader(_handle, object.getHandle());
    }

    inline void ShaderProgram::detach(const Shader& object)
    {
        glDetachShader(_handle, object.getHandle());
    }

    inline void ShaderProgram::bindAttribLocation(GLuint location, const std::string& attribute)
    {
        glBindAttribLocation(_handle, location, attribute.c_str());
    }

    inline bool ShaderProgram::link()
    {
        GLint       logLength = 0;

        glLinkProgram(_handle);
        if (get(GL_LINK_STATUS) == static_cast<GLint>(GL_FALSE))
        {
            logLength = get(GL_INFO_LOG_LENGTH);
            if (logLength > 1)
            {
                std::vector<GLchar> infoLog(logLength);
                glGetProgramInfoLog(_handle, logLength, &logLength, &infoLog[0]);
                infoLog[logLength - 1] = '\0'; // Overwrite endline
                _log = &infoLog[0];
            }
            return false;
        }
        _log = std::string();
        retrieveLocations();
        // NOTE can be improved
        retrieveSubroutines(GL_VERTEX_SHADER);
        retrieveSubroutines(GL_GEOMETRY_SHADER);
        retrieveSubroutines(GL_TESS_CONTROL_SHADER);
        retrieveSubroutines(GL_TESS_EVALUATION_SHADER);
        retrieveSubroutines(GL_COMPUTE_SHADER);
        retrieveSubroutines(GL_FRAGMENT_SHADER);
        return true;
    }

    inline void ShaderProgram::use()
    {
        glUseProgram(_handle);
    }

    inline const std::string& ShaderProgram::getLog() const
    {
        return _log;
    }

    inline GLint ShaderProgram::getAttribLocation(const std::string& name) const
    {
        std::map<std::string, GLint>::const_iterator it;

        if ((it = _attribs.find(name)) != _attribs.end())
            return it->second;
        std::cerr << "Shader attribute \'" << name << "\' does not exist" << std::endl;
        return -1;
    }

    inline GLint ShaderProgram::getUniformLocation(const std::string& name) const
    {
        std::map<std::string, GLint>::const_iterator it;

        if ((it = _uniforms.find(name)) != _uniforms.end())
            return it->second;
        std::cerr << "Shader uniform \'" << name << "\' does not exist" << std::endl;
        return -1;
    }

    inline void ShaderProgram::setTransformFeedbackVaryings(GLsizei count, const char** varyings, GLenum bufferMode)
    {
        glTransformFeedbackVaryings(_handle, count, varyings, bufferMode);
    }

    inline void ShaderProgram::printDebug()
    {
        std::cout << "Attributes:" << std::endl;
        for (auto attrib : _attribs)
            std::cout << std::setw(6) << attrib.second << ": " << attrib.first << std::endl;
        std::cout << "Uniforms:" << std::endl;
        for (auto uniform : _uniforms)
            std::cout << std::setw(6)  << uniform.second << ": " << uniform.first << std::endl;
        for (auto subroutineMap : _subroutines)
        {
            std::cout << "Subroutines for shader idx: " << static_cast<int>(subroutineMap.first) << std::endl;
            for (auto subroutineUniform : subroutineMap.second)
            {
                std::cout << "Subroutine uniform id=" << subroutineUniform.second.uniform << ": " << subroutineUniform.first << std::endl;
                for (auto subroutine : subroutineUniform.second.subroutines)
                    std::cout << "Subroutine id=" << subroutine.second << ": " << subroutine.first << std::endl;
            }
        }
    }

    inline void ShaderProgram::get(GLenum property, GLint* value)
    {
        glGetProgramiv(_handle, property, value);
    }

    inline GLint ShaderProgram::get(GLenum property)
    {
        GLint   value;
        glGetProgramiv(_handle, property, &value);
        return value;
    }

    inline void ShaderProgram::set(GLenum property, GLint value)
    {
        glProgramParameteri(_handle, property, value);
    }

    inline bool ShaderProgram::isValid() const
    {
        return glIsProgram(_handle) == GL_TRUE;
    }

    inline void ShaderProgram::retrieveLocations()
    {
        GLint       n, maxLen;
        GLint       size;
        GLuint      location;
        GLsizei     written;
        GLenum      type;
        GLchar*     name;
        std::string uniformName;
        std::string arraySuffix("[0]");

        maxLen = get(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
        n = get(GL_ACTIVE_ATTRIBUTES);
        name = new GLchar[maxLen];
        for (int i = 0; i < n; ++i)
        {
            glGetActiveAttrib(_handle, i, maxLen, &written, &size, &type, name);
            location = glGetAttribLocation(_handle, name);
            _attribs[name] = location;
        }
        delete[] name;
        maxLen = get(GL_ACTIVE_UNIFORM_MAX_LENGTH);
        n = get(GL_ACTIVE_UNIFORMS);
        name = new GLchar[maxLen];
        for (int i = 0; i < n; ++i)
        {
            glGetActiveUniform(_handle, i, maxLen, &written, &size, &type, name);
            location = glGetUniformLocation(_handle, name);
            uniformName = name;
            _uniforms[uniformName] = location;
            // If the uniform is an array, add the name stripped of its '[0]' suffix
            if (std::mismatch(arraySuffix.rbegin(),arraySuffix.rend(), uniformName.rbegin()).first == arraySuffix.rend())
                _uniforms[uniformName.substr(0, uniformName.length() - arraySuffix.length())] = location;
        }
        delete[] name;
    }

    inline void ShaderProgram::retrieveSubroutines(GLenum type)
    {
        int     countActiveSU;
        char    sname[256]; // FIXME
        int     len;
        int     numCompS;

        glGetProgramStageiv(_handle, type, GL_ACTIVE_SUBROUTINE_UNIFORMS, &countActiveSU);
        for (int i = 0; i < countActiveSU; ++i)
        {
            glGetActiveSubroutineUniformName(_handle, type, i, 256, &len, sname);
            glGetActiveSubroutineUniformiv(_handle, type, i, GL_NUM_COMPATIBLE_SUBROUTINES, &numCompS);
            SubroutineUniform& subUniform = _subroutines[type][sname];

            subUniform.uniform = i;
            GLint* s = new GLint[numCompS];
            glGetActiveSubroutineUniformiv(_handle, type, i, GL_COMPATIBLE_SUBROUTINES, s);
            for (int j = 0; j < numCompS; ++j)
            {
                glGetActiveSubroutineName(_handle, type, s[j], 256, &len, sname);
                subUniform.subroutines[sname] = s[j];
            }
            delete[] s;
        }
    }

    inline void ShaderProgram::setVertexAttribPointer(GLuint location, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointerOffset)
    {
        glVertexAttribPointer(location, size, type, normalized, stride, pointerOffset);
    }

    inline void ShaderProgram::setVertexAttribPointer(const std::string& name, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointerOffset)
    {
        glVertexAttribPointer(getAttribLocation(name), size, type, normalized, stride, pointerOffset);
    }

    /*
     * GLfloat uniform specialization
     */

    template <>
    inline void ShaderProgram::setUniform<GLfloat>(const std::string& name, GLfloat v1)
    {
        glProgramUniform1f(_handle, getUniformLocation(name), v1);
    }

    template <>
    inline void ShaderProgram::setUniform<GLfloat>(const std::string& name, GLfloat v1, GLfloat v2)
    {
        glProgramUniform2f(_handle, getUniformLocation(name), v1, v2);
    }

    template <>
    inline void ShaderProgram::setUniform<GLfloat>(const std::string& name, GLfloat v1, GLfloat v2, GLfloat v3)
    {
        glProgramUniform3f(_handle, getUniformLocation(name), v1, v2, v3);
    }

    template <>
    inline void ShaderProgram::setUniform<GLfloat>(const std::string& name, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4)
    {
        glProgramUniform4f(_handle, getUniformLocation(name), v1, v2, v3, v4);
    }

    /*
     * GLint uniform specialization
     */

    template <>
    inline void ShaderProgram::setUniform<GLint>(const std::string& name, GLint v1)
    {
        glProgramUniform1i(_handle, getUniformLocation(name), v1);
    }

    template <>
    inline void ShaderProgram::setUniform<GLint>(const std::string& name, GLint v1, GLint v2)
    {
        glProgramUniform2i(_handle, getUniformLocation(name), v1, v2);
    }

    template <>
    inline void ShaderProgram::setUniform<GLint>(const std::string& name, GLint v1, GLint v2, GLint v3)
    {
        glProgramUniform3i(_handle, getUniformLocation(name), v1, v2, v3);
    }

    template <>
    inline void ShaderProgram::setUniform<GLint>(const std::string& name, GLint v1, GLint v2, GLint v3, GLint v4)
    {
        glProgramUniform4i(_handle, getUniformLocation(name), v1, v2, v3, v4);
    }

    /*
     * GLuint uniform specialization
     */

    template <>
    inline void ShaderProgram::setUniform<GLuint>(const std::string& name, GLuint v1)
    {
        glProgramUniform1ui(_handle, getUniformLocation(name), v1);
    }

    template <>
    inline void ShaderProgram::setUniform<GLuint>(const std::string& name, GLuint v1, GLuint v2)
    {
        glProgramUniform2ui(_handle, getUniformLocation(name), v1, v2);
    }

    template <>
    inline void ShaderProgram::setUniform<GLuint>(const std::string& name, GLuint v1, GLuint v2, GLuint v3)
    {
        glProgramUniform3ui(_handle, getUniformLocation(name), v1, v2, v3);
    }

    template <>
    inline void ShaderProgram::setUniform<GLuint>(const std::string& name, GLuint v1, GLuint v2, GLuint v3, GLuint v4)
    {
        glProgramUniform4ui(_handle, getUniformLocation(name), v1, v2, v3, v4);
    }

    /*
     * GLfloat uniform array specialization
     */

    template <>
    inline void ShaderProgram::setUniformPtr<1, GLfloat>(const std::string& name, const GLfloat* ptr, GLsizei count)
    {
        glProgramUniform1fv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<2, GLfloat>(const std::string& name, const GLfloat* ptr, GLsizei count)
    {
        glProgramUniform2fv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<3, GLfloat>(const std::string& name, const GLfloat* ptr, GLsizei count)
    {
        glProgramUniform3fv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<4, GLfloat>(const std::string& name, const GLfloat* ptr, GLsizei count)
    {
        glProgramUniform4fv(_handle, getUniformLocation(name), count, ptr);
    }

    /*
     * GLint uniform array specialization
     */

    template <>
    inline void ShaderProgram::setUniformPtr<1, GLint>(const std::string& name, const GLint* ptr, GLsizei count)
    {
        glProgramUniform1iv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<2, GLint>(const std::string& name, const GLint* ptr, GLsizei count)
    {
        glProgramUniform2iv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<3, GLint>(const std::string& name, const GLint* ptr, GLsizei count)
    {
        glProgramUniform3iv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<4, GLint>(const std::string& name, const GLint* ptr, GLsizei count)
    {
        glProgramUniform4iv(_handle, getUniformLocation(name), count, ptr);
    }

    /*
     * GLuint uniform array specialization
     */

    template <>
    inline void ShaderProgram::setUniformPtr<1, GLuint>(const std::string& name, const GLuint* ptr, GLsizei count)
    {
        glProgramUniform1uiv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<2, GLuint>(const std::string& name, const GLuint* ptr, GLsizei count)
    {
        glProgramUniform2uiv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<3, GLuint>(const std::string& name, const GLuint* ptr, GLsizei count)
    {
        glProgramUniform3uiv(_handle, getUniformLocation(name), count, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformPtr<4, GLuint>(const std::string& name, const GLuint* ptr, GLsizei count)
    {
        glProgramUniform4uiv(_handle, getUniformLocation(name), count, ptr);
    }

    /*
     * GLfloat matrix uniform array specialization
     */

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<2, 2, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix2fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<3, 3, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix3fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<4, 4, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix4fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<2, 3, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix2x3fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<3, 2, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix3x2fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<2, 4, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix2x4fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<4, 2, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix4x2fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<3, 4, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix3x4fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<4, 3, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix4x3fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    /*
     * GLfloat square matrix uniform array specialization
     */

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<2, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix2fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<3, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix3fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }

    template <>
    inline void ShaderProgram::setUniformMatrixPtr<4, GLfloat>(const std::string& name, const GLfloat* ptr, GLboolean transpose, GLsizei count)
    {
        glProgramUniformMatrix4fv(_handle, getUniformLocation(name), count, transpose, ptr);
    }
}
