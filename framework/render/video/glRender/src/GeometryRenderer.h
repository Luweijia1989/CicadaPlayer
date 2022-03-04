#pragma once

#include "Geometry.h"
#include "platform/platform_gl.h"

class GeometryRenderer
{
public:
    // rendering features. Use all possible features as the default.
    static const int kVBO = 0x01;
    static const int kIBO = 0x02;
    static const int kVAO = 0x04;
    static const int kMapBuffer = 1<<16;
    // TODO: VAB, VBUM etc.
    GeometryRenderer();
    virtual ~GeometryRenderer() {}
    // call updateBuffer internally in bindBuffer if feature is changed
    void setFeature(int f, bool on);
    void setFeatures(int value);
    int features() const;
    int actualFeatures() const;
    bool testFeatures(int value) const;
    /*!
     * \brief updateGeometry
     * Update geometry buffer. Rebind VBO, IBO to VAO if geometry attributes is changed.
     * Assume attributes are bound in the order 0, 1, 2,....
     * Make sure the old geometry is not destroyed before this call because it will be used to compare with the new \l geo
     * \param geo null: release vao/vbo
     */
    void updateGeometry(Geometry* geo = NULL);
    virtual void render();
protected:
    void bindBuffers();
    void unbindBuffers();
private:
    Geometry *g;
    int features_;
    int vbo_size, ibo_size; // QOpenGLBuffer.size() may get error 0x501
    GLuint vbo; //VertexBuffer
    GLuint vao;
    GLuint ibo;

    // geometry characteristic
    int stride;
    std::vector<Attribute> attrib;
};
