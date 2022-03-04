#pragma once

#include <cassert>
#include <vector>

class Uniform {
public:
    enum { V = 16, Vec = 1 << V, M = 20, Mat = 1 << M };
    enum Type {
        Unknown = 0,
        Bool = 1 << 0,
        Int = 1 << 1,
        UInt = 1 << 2,
        Float = 1 << 3,
        Double = 1 << 4,
        Sampler = 1 << 5,//TODO: i,u sampler2D, i,u image etc
        BVec2 = Bool | Vec | (2 << (V + 1)),
        BVec3 = Bool | Vec | (3 << (V + 1)),
        BVec4 = Bool | Vec | (4 << (V + 1)),
        IVec2 = Int | Vec | (2 << (V + 1)),
        IVec3 = Int | Vec | (3 << (V + 1)),
        IVec4 = Int | Vec | (4 << (V + 1)),
        UVec2 = UInt | Vec | (2 << (V + 1)),
        UVec3 = UInt | Vec | (3 << (V + 1)),
        UVec4 = UInt | Vec | (4 << (V + 1)),
        Vec2 = Float | Vec | (2 << (V + 1)),
        Vec3 = Float | Vec | (3 << (V + 1)),
        Vec4 = Float | Vec | (4 << (V + 1)),
        Mat2 = Float | Mat | (2 << (M + 1)),//TODO: mat2x3 2x4 3x2 3x4 4x3
        Mat3 = Float | Mat | (3 << (M + 1)),
        Mat4 = Float | Mat | (4 << (M + 1)),
        DMat2 = Double | Mat | (2 << (M + 1)),
        DMat3 = Double | Mat | (3 << (M + 1)),
        DMat4 = Double | Mat | (4 << (M + 1)),
    };
    bool isBool() const
    {
        return type() & Bool;
    }
    bool isInt() const
    {
        return type() & Int;
    }
    bool isUInt() const
    {
        return type() & UInt;
    }
    bool isFloat() const
    {
        return type() & Float;
    }
    bool isDouble() const
    {
        return type() & Double;
    }
    bool isVec() const
    {
        return type() & Vec;
    }
    bool isMat() const
    {
        return type() & Mat;
    }

    bool dirty;
    int location;//TODO: auto resolve location?
    std::string name;
    /*!
     * \brief setType
     * \param count array size, or 1 if not array
     */
    Uniform &setType(Type tp, int count = 1);
    Uniform(Type tp = Float, int count = 1);
    /*!
     * \brief set
     * Set uniform value in host memory. This will mark dirty if value is changed
     * \param v the value
     * \param count number of element T to set. 0: depending on type and array size, the number is arrySize()*tupleSize();
     * TODO: Sampler
     */
    void set(const float &v, int count = 0);
    void set(const unsigned &v, int count = 0);
    void set(const int &v, int count = 0);
    void set(const float *v, int count = 0);
    void set(const unsigned *v, int count = 0);
    void set(const int *v, int count = 0);

    /*!
     * \brief setGL
     * Call glUniformXXX to update uniform values that set by set(const T&, int) and mark dirty false. Currently only use OpenGL ES2 supported functions, i.e. uint, double types are not supported.
     * \return false if location is invalid, or if uniform type is not supported by QOpenGLFunctions
     * TODO: Sampler
     */
    bool setGL();
    bool operator==(const Uniform &other) const
    {
        if (type() != other.type()) return false;
        if (name != other.name) return false;
        if (data != other.data) return false;
        return true;
    }
    Type type() const
    {
        return t;
    }
    /*!
     * \brief tupleSize
     * 2, 3, 4 for vec2, vec3 and vec4; 2^2, 3^2 and 4^2 for mat2, mat3 and mat4
     */
    int tupleSize() const
    {
        return tuple_size;
    }
    /*!
     * \brief arraySize
     * If uniform is an array, it's array size; otherwise 1
     */
    int arraySize() const
    {
        return array_size;
    }
    /*!
     * Return an array of given type. the type T must match type(), for example T is float for Float, VecN, MatN and array of them
     */
    template<typename T>
    std::vector<T> value() const
    {
        assert(sizeof(T) * tupleSize() * arraySize() <= data.size() * sizeof(int) && "Bad type or array size");
        std::vector<T> v(tupleSize() * arraySize());
        memcpy((char *) v.data(), (const char *) data.data(), v.size() * sizeof(T));
        return v;
    }
    template<typename T>
    const T *address() const
    {
        assert(sizeof(T) * tupleSize() * arraySize() <= data.size() * sizeof(int) && "Bad type or array size");
        return reinterpret_cast<const T *>(data.data());
    }

private:
    int tuple_size;
    int array_size;
    Type t;
    std::vector<int> data;//uniform array
};