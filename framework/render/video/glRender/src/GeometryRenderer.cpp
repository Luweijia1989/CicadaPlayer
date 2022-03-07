#define LOG_TAG "GeometryRenderer"
#include "GeometryRenderer.h"
#include <utils/frame_work_log.h>

GeometryRenderer::GeometryRenderer()
    : g(NULL), features_(kVBO | kIBO | kVAO | kMapBuffer), vbo_size(0), ibo_size(0), vao(0), vbo(0), ibo(0), stride(0)
{}

void GeometryRenderer::setFeature(int f, bool on)
{
    if (on)
        features_ |= f;
    else
        features_ ^= f;
}

void GeometryRenderer::setFeatures(int value)
{
    features_ = value;
}

int GeometryRenderer::features() const
{
    return features_;
}

int GeometryRenderer::actualFeatures() const
{
    int f = 0;
    if (vbo) f |= kVBO;
    if (ibo) f |= kIBO;
    if (vao) f |= kVAO;

    return f;
}

bool GeometryRenderer::testFeatures(int value) const
{
    return !!(features() & value);
}

void GeometryRenderer::updateGeometry(Geometry *geo)
{
    g = geo;
    if (!g) {
        glDeleteBuffers(1, &ibo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);

        vbo_size = 0;
        ibo_size = 0;
        return;
    }

    if (testFeatures(kIBO) && !ibo) {
        if (g->indexCount() > 0) {
            AF_LOGD("creating IBO...");
            glGenBuffers(1, &ibo);
            if (!ibo) AF_LOGD("IBO create error");
        }
    }
    if (ibo) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        const int bs = g->indexDataSize();
        if (bs == ibo_size) {
            void *p = NULL;
            if (testFeatures(kMapBuffer)) p = glMapBuffer(ibo, GL_WRITE_ONLY);
            if (p) {
                memcpy(p, g->constIndexData(), bs);
                glUnmapBuffer(ibo);
            } else {
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bs, g->constIndexData());
            }
        } else {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bs, g->indexData(), GL_STATIC_DRAW);
            ibo_size = bs;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    if (testFeatures(kVBO) && !vbo) {
        AF_LOGD("creating VBO...");
        glGenBuffers(1, &vbo);
        if (!vbo) AF_LOGD("VBO create error");
    }
    if (vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        const int bs = g->vertexCount() * g->stride();
        /* Notes from https://www.opengl.org/sdk/docs/man/html/glBufferSubData.xhtml
           When replacing the entire data store, consider using glBufferSubData rather than completely recreating the data store with glBufferData. This avoids the cost of reallocating the data store.
         */
        if (bs == vbo_size) {// vbo.size() error 0x501 on rpi, and query gl value can be slow
            void *p = NULL;
            if (testFeatures(kMapBuffer)) p = glMapBuffer(vbo, GL_WRITE_ONLY);
            if (p) {
                memcpy(p, g->constVertexData(), bs);
                glUnmapBuffer(vbo);
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, 0, bs, g->constVertexData());
                vbo_size = bs;
            }
        } else {
            glBufferData(GL_ARRAY_BUFFER, bs, g->vertexData(), GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (stride == g->stride() && attrib == g->attributes()) return;
    stride = g->stride();
    attrib = g->attributes();

    if (testFeatures(kVAO) && !vao) {
        AF_LOGD("creating VAO...");
        glGenVertexArrays(1, &vao);
        if (!vao) AF_LOGD("VAO create error");
    }
    AF_LOGD("vao updated");
    if (vao)// can not use vao binder because it will create a vao if necessary
        glBindVertexArray(vao);

    if (!vao) return;
    AF_LOGD("geometry attributes changed, rebind vao...");
    // call once is enough if no feature and no geometry attribute is changed
    if (vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        for (int an = 0; an < g->attributes().size(); ++an) {
            // FIXME: assume bind order is 0,1,2...
            const Attribute &a = g->attributes().at(an);
            glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(),
                                  reinterpret_cast<const void *>(ptrdiff_t(a.offset())));//TODO: in setActiveShader
            glEnableVertexAttribArray(an);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    // bind ibo to vao thus no bind is required later
    if (ibo)// if not bind here, glDrawElements(...,NULL) crashes and must use ibo data ptr, why?
        glBindBuffer(GL_ARRAY_BUFFER, ibo);

    glBindVertexArray(vao);
    if (ibo) glBindBuffer(GL_ARRAY_BUFFER, 0);

    AF_LOGD("geometry updated");
}

void GeometryRenderer::bindBuffers()
{
    bool bind_vbo = vbo != 0;
    bool bind_ibo = ibo != 0;
    bool setv_skip = false;

    if (vao) {
        glBindVertexArray(vao);
        setv_skip = bind_vbo;
        bind_vbo = false;
        bind_ibo = false;
    }

    if (bind_ibo) glBindBuffer(GL_ARRAY_BUFFER, ibo);
    // no vbo: set vertex attributes
    // has vbo, no vao: bind vbo & set vertex attributes
    // has vbo, has vao: skip
    if (setv_skip) return;
    if (!g) return;
    const char *vdata = static_cast<const char *>(g->vertexData());
    if (bind_vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        vdata = NULL;
    }
    for (int an = 0; an < g->attributes().size(); ++an) {
        const Attribute &a = g->attributes().at(an);
        glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), vdata + a.offset());
        glEnableVertexAttribArray(an);//TODO: in setActiveShader
    }
}

void GeometryRenderer::unbindBuffers()
{
    bool unbind_vbo = vbo != 0;
    bool unbind_ibo = ibo != 0;
    bool unsetv_skip = false;

    if (vao) {
        glBindVertexArray(0);
        unsetv_skip = unbind_vbo;
        unbind_vbo = false;
        unbind_ibo = false;
    }


    if (unbind_ibo) glBindBuffer(GL_ARRAY_BUFFER, 0);
    // release vbo. qpainter is affected if vbo is bound
    if (unbind_vbo) glBindBuffer(GL_ARRAY_BUFFER, 0);
    // no vbo: disable vertex attributes
    // has vbo, no vao: unbind vbo & set vertex attributes
    // has vbo, has vao: skip
    if (unsetv_skip) return;
    if (!g) return;
    for (int an = 0; an < g->attributes().size(); ++an) {
        glDisableVertexAttribArray(an);
    }
}

void GeometryRenderer::render()
{
    if (!g) return;
    bindBuffers();
    if (g->indexCount() > 0) {
        glDrawElements(g->primitive(), g->indexCount(), g->indexType(),
                       ibo ? NULL : g->indexData());// null: data in vao or ibo. not null: data in memory
    } else {
        glDrawArrays(g->primitive(), 0, g->vertexCount());
    }
    unbindBuffers();
}
