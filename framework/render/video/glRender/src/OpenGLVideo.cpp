#include "OpenGLVideo.h"
#include "GeometryRenderer.h"
#include "OpenGLHelper.h"
#include "VideoShader.h"
#include "base/utils.h"

// FIXME: why crash if inherits both QObject and DPtrPrivate?
class OpenGLVideoPrivate {
public:
    OpenGLVideoPrivate()
        : manager(new ShaderManager), material(new VideoMaterial()), material_type(0), norm_viewport(true), update_geo(true), tex_target(0),
          valiad_tex_width(1.0), mesh_type(OpenGLVideo::RectMesh), geometry(NULL), gr(NULL), user_shader(NULL)
    {}
    ~OpenGLVideoPrivate()
    {
        if (material) {
            delete material;
            material = 0;
        }

        if (geometry) {
            delete geometry;
            geometry = nullptr;
        }

        if (gr) {
            delete gr;
            gr = nullptr;
        }

        if (manager) {
            delete manager;
            manager = nullptr;
        }
    }

    // update geometry(vertex array) set attributes or bind VAO/VBO.
    void updateGeometry(VideoShader *shader, const QRectF &t, const QRectF &r);

public:
    ShaderManager *manager;
    VideoMaterial *material;
    __int64 material_type;
    bool norm_viewport;
    bool has_a;
    bool update_geo;
    int tex_target;
    double valiad_tex_width;
    QSize video_size;
    QRectF target;
    QRectF roi;//including invalid padding width
    OpenGLVideo::MeshType mesh_type;
    TexturedGeometry *geometry;
    GeometryRenderer *gr;
    QRectF rect;
    QMatrix4x4 matrix;
    QMatrix4x4 drawTransform;
    VideoShader *user_shader;
};

void OpenGLVideoPrivate::updateGeometry(VideoShader *shader, const QRectF &t, const QRectF &r)
{
    // also check size change for normalizedROI computation if roi is not normalized
    const bool roi_changed = valiad_tex_width != material->validTextureWidth() || roi != r || video_size != material->frameSize();
    const int tc = shader->textureLocationCount();
    if (roi_changed) {
        roi = r;
        valiad_tex_width = material->validTextureWidth();
        video_size = material->frameSize();
    }
    if (tex_target != shader->textureTarget()) {
        tex_target = shader->textureTarget();
        update_geo = true;
    }

    if (!gr) {// TODO: only update VAO, not the whole GeometryRenderer
        update_geo = true;
        gr = new GeometryRenderer();// local var is captured by lambda
    }
    // (-1, -1, 2, 2) must flip y
    QRectF target_rect = norm_viewport ? QRectF(-1, 1, 2, -2) : rect;
    if (target.isValid()) {
        if (roi_changed || target != t) {
            target = t;
            update_geo = true;
            //target_rect = target (if valid). // relate to gvf bug?
        }
    } else {
        if (roi_changed) {
            update_geo = true;
        }
    }
    if (!update_geo) return;
    delete geometry;
    geometry = NULL;
    if (mesh_type == OpenGLVideo::SphereMesh)
        geometry = new Sphere();
    else
        geometry = new TexturedGeometry();

    // setTextureCount may change the vertex data. Call it before setRect()
    geometry->setTextureCount(shader->textureTarget() == GL_TEXTURE_RECTANGLE ? tc : 1);
    geometry->setGeometryRect(target_rect);
    geometry->setTextureRect(material->mapToTexture(0, roi));
    if (shader->textureTarget() == GL_TEXTURE_RECTANGLE) {
        for (int i = 1; i < tc; ++i) {
            // tc can > planes, but that will compute chroma plane
            geometry->setTextureRect(material->mapToTexture(i, roi), i);
        }
    }
    geometry->create();
    update_geo = false;
    gr->updateGeometry(geometry);
}

OpenGLVideo::OpenGLVideo() : d(new OpenGLVideoPrivate)
{}

OpenGLVideo::~OpenGLVideo()
{
    delete d;
}

bool OpenGLVideo::isSupported(VideoFormat::PixelFormat pixfmt)
{
    return pixfmt != VideoFormat::Format_RGB48BE && pixfmt != VideoFormat::Format_Invalid;
}

void OpenGLVideo::setViewport(const QRectF &r)
{
    d->rect = r;
    if (d->norm_viewport) {
        d->matrix.setToIdentity();
        if (d->mesh_type == SphereMesh) d->matrix.perspective(45, 1, 0.1, 100);// for sphere
    } else {
        d->matrix.setToIdentity();
        d->matrix.ortho(r);
        d->update_geo = true;// even true for target_rect != d.rect
    }
}

void OpenGLVideo::setBrightness(double value)
{
    d->material->setBrightness(value);
}

void OpenGLVideo::setContrast(double value)
{
    d->material->setContrast(value);
}

void OpenGLVideo::setHue(double value)
{
    d->material->setHue(value);
}

void OpenGLVideo::setSaturation(double value)
{
    d->material->setSaturation(value);
}

void OpenGLVideo::setUserShader(VideoShader *shader)
{
    d->user_shader = shader;
}

VideoShader *OpenGLVideo::userShader() const
{
    return d->user_shader;
}

void OpenGLVideo::setMeshType(MeshType value)
{
    if (d->mesh_type == value) return;
    d->mesh_type = value;
    d->update_geo = true;
    if (d->mesh_type == SphereMesh && d->norm_viewport) {
        d->matrix.setToIdentity();
        d->matrix.perspective(45, 1, 0.1, 100);// for sphere
    }
}

OpenGLVideo::MeshType OpenGLVideo::meshType() const
{
    return d->mesh_type;
}

void OpenGLVideo::fill(const int &color)
{
    float c[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    cicada::convertToGLColor(color, c);
    glClearColor(c[0], c[1], c[2], c[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLVideo::render(std::unique_ptr<IAFFrame> &frame, const QRectF &target, const QRectF &roi, const QMatrix4x4 &transform)
{
    if (!d->material->setCurrentFrame(frame)) return;

    const __int64 mt = d->material->type();
    if (d->material_type != mt) {
        d->material_type = mt;
    }

    if (!d->material->bind())// bind first because texture parameters(target) mapped from native buffer is unknown before it
        return;
    VideoShader *shader = d->user_shader;
    if (!shader) shader = d->manager->prepareMaterial(d->material, mt);
    glViewport(
            d->rect.x(), d->rect.y(), d->rect.width(),
            d->rect.height());// viewport was used in gpu filters is wrong, qt quick fbo item's is right(so must ensure setProjectionMatrixToRect was called correctly)
    shader->update(d->material);
    auto matrix = transform * d->matrix;
    glUniformMatrix4fv(shader->matrixLocation(), 1, GL_FALSE, matrix.constData());
    // uniform end. attribute begin
    d->updateGeometry(shader, target, roi);
    // normalize?
    const bool blending = d->has_a;
    if (blending) {
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//
    }

    d->gr->render();
    if (blending) glDisable(GL_BLEND);

    d->material->unbind();
}