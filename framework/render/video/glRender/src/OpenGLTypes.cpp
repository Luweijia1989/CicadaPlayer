#include "OpenGLTypes.h"
#include "OpenGLHelper.h"
#include "platform/platform_gl.h"

struct uniform_type_name {
    std::string name;
    Uniform::Type type;
} uniform_type_names[] = {
        {"sample2D", Uniform::Sampler}, {"bool", Uniform::Bool},   {"int", Uniform::Int},     {"uint", Uniform::Int},
        {"float", Uniform::Float},      {"vec2", Uniform::Vec2},   {"vec3", Uniform::Vec3},   {"vec4", Uniform::Vec4},
        {"mat2", Uniform::Mat2},        {"mat3", Uniform::Mat3},   {"mat4", Uniform::Mat4},   {"bvec2", Uniform::BVec2},
        {"bvec3", Uniform::BVec3},      {"bvec4", Uniform::BVec4}, {"ivec2", Uniform::IVec2}, {"ivec3", Uniform::IVec3},
        {"ivec4", Uniform::IVec4},      {"uvec2", Uniform::UVec2}, {"uvec3", Uniform::UVec3}, {"uvec4", Uniform::UVec4},
        {"mat2x2", Uniform::Mat2},      {"mat3x3", Uniform::Mat3}, {"mat4x4", Uniform::Mat4}, {"dmat2", Uniform::DMat2},
        {"dmat3", Uniform::DMat3},      {"dmat4", Uniform::DMat4},
};

static Uniform::Type UniformTypeFromName(const std::string &name)
{
    for (const uniform_type_name *un = uniform_type_names;
         un < uniform_type_names + sizeof(uniform_type_names) / sizeof(uniform_type_names[0]); ++un) {
        if (un->name == name) return un->type;
    }
    return Uniform::Unknown;
}

static std::string UniformTypeToName(Uniform::Type ut)
{
    for (const uniform_type_name *un = uniform_type_names;
         un < uniform_type_names + sizeof(uniform_type_names) / sizeof(uniform_type_names[0]); ++un) {
        if (un->type == ut) return un->name;
    }
    return "unknown";
}

Uniform::Uniform(Type tp, int count) : dirty(true), location(-1), tuple_size(1), array_size(1), t(tp)
{
    setType(tp, count);
}

Uniform &Uniform::setType(Type tp, int count)
{
    t = tp;
    array_size = count;
    if (isVec()) {
        tuple_size = (t >> (V + 1)) & ((1 << 3) - 1);
    } else if (isMat()) {
        tuple_size = (t >> (M + 1)) & ((1 << 3) - 1);
        tuple_size *= tuple_size;
    }
    int element_size = sizeof(float);
    if (isInt() || isUInt() || isBool()) {
        element_size = sizeof(int);
    }
    data = std::vector<int>(element_size / sizeof(int) * tupleSize() * arraySize());
    return *this;
}

template<typename T>
bool set_uniform_value(std::vector<int> &dst, const T *v, int count)
{
    assert(sizeof(T) * count <= sizeof(int) * dst.size() && "set_uniform_value: Bad type or array size");
    // why not dst.constData()?
    const std::vector<int> old(dst);
    memcpy((char *) dst.data(), (const char *) v, count * sizeof(T));
    return old != dst;
}

template<>
bool set_uniform_value<bool>(std::vector<int> &dst, const bool *v, int count)
{
    const std::vector<int> old(dst);
    for (int i = 0; i < count; ++i) {
        dst[i] = *(v + i);
    }
    return old != dst;
}

void Uniform::set(const float &v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, &v, count);
}

void Uniform::set(const int &v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, &v, count);
}

void Uniform::set(const unsigned &v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, &v, count);
}


void Uniform::set(const float *v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, v, count);
}

void Uniform::set(const int *v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, v, count);
}

void Uniform::set(const unsigned *v, int count)
{
    if (count <= 0) count = tupleSize() * arraySize();
    dirty = set_uniform_value(data, v, count);
}

bool Uniform::setGL()
{
    if (location < 0) {
        return false;
    }
    switch (type()) {
        case Uniform::Bool:
        case Uniform::Int:
            glUniform1iv(location, arraySize(), address<int>());
            break;
        case Uniform::Float:
            glUniform1fv(location, arraySize(), address<float>());
            break;
        case Uniform::Vec2:
            glUniform2fv(location, arraySize(), address<float>());
            break;
        case Uniform::Vec3:
            glUniform3fv(location, arraySize(), address<float>());
            break;
        case Uniform::Vec4:
            glUniform4fv(location, arraySize(), address<float>());
            break;
        case Uniform::Mat2:
            glUniformMatrix2fv(location, arraySize(), GL_FALSE, address<float>());
            break;
        case Uniform::Mat3:
            glUniformMatrix3fv(location, arraySize(), GL_FALSE, address<float>());
            break;
        case Uniform::Mat4:
            glUniformMatrix4fv(location, arraySize(), GL_FALSE, address<float>());
            break;
        case Uniform::IVec2:
            glUniform2iv(location, arraySize(), address<int>());
            break;
        case Uniform::IVec3:
            glUniform3iv(location, arraySize(), address<int>());
            break;
        case Uniform::IVec4:
            glUniform4iv(location, arraySize(), address<int>());
            break;
        default:
            return false;
    }
    dirty = false;
    return true;
}

std::vector<Uniform> ParseUniforms(const std::string &text, GLuint programId = 0)
{
    std::vector<Uniform> uniforms;
    //const std::string code = OpenGLHelper::removeComments(text);
    //std::vector<std::string> lines;
    //split(code, lines, ";");
    //// TODO: highp lowp etc.
    //const QString exp(QStringLiteral("\\s*uniform\\s+([\\w\\d]+)\\s+([\\w\\d]+)\\s*"));
    //const QString exp_array = exp + QStringLiteral("\\[(\\d+)\\]\\s*");
    //foreach (QString line, lines) {
    //    line = line.trimmed();
    //    if (!line.startsWith(QStringLiteral("uniform "))) continue;
    //    QRegExp rx(exp_array);
    //    if (rx.indexIn(line) < 0) {
    //        rx = QRegExp(exp);
    //        if (rx.indexIn(line) < 0) continue;
    //    }
    //    Uniform u;
    //    const QStringList x = rx.capturedTexts();
    //    //qDebug() << x;
    //    u.name = x.at(2).toUtf8();
    //    int array_size = 1;
    //    if (x.size() > 3) array_size = x[3].toInt();
    //    const QByteArray t(x[1].toLatin1());
    //    u.setType(UniformTypeFromName(t), array_size);
    //    if (programId > 0) u.location = gl().GetUniformLocation(programId, u.name.constData());
    //    uniforms.append(u);
    //}
    //qDebug() << uniforms;
    return uniforms;
}
