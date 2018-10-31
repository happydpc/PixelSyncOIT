//
// Created by christoph on 30.10.18.
//

#ifndef PIXELSYNCOIT_OIT_MLABBUCKET_HPP
#define PIXELSYNCOIT_OIT_MLABBUCKET_HPP

#include <Math/Geometry/AABB3.hpp>

#include "OIT_Renderer.hpp"

/**
 * An order independent transparency renderer using pixel sync.
 *
 * (To be precise, it doesn't use the Intel-specific Pixel Sync extension
 * INTEL_fragment_shader_ordering, but the vendor-independent ARB_fragment_shader_interlock).
 */
class OIT_MLABBucket : public OIT_Renderer {
public:
    /**
     *  The gather shader is used to render our transparent objects.
     *  Its purpose is to store the fragments in an offscreen-buffer.
     */
    virtual sgl::ShaderProgramPtr getGatherShader() { return gatherShader; }

    OIT_MLABBucket();
    virtual void create();
    virtual void resolutionChanged(sgl::FramebufferObjectPtr &sceneFramebuffer, sgl::TexturePtr &sceneTexture,
                                   sgl::RenderbufferObjectPtr &sceneDepthRBO);

    virtual void gatherBegin();
    // In between "gatherBegin" and "gatherEnd", we can render our objects using the gather shader
    virtual void gatherEnd();

    // Blit accumulated transparent objects to screen
    virtual void renderToScreen();

    void renderGUI();
    void updateLayerMode();
    void reloadShaders();

    // For determining minimum and maximum (screen-space) depth
    void setScreenSpaceBoundingBox(const sgl::AABB3 &screenSpaceBB, sgl::CameraPtr &camera);

    // For changing performance measurement modes
    void setNewState(const InternalState &newState);

private:
    void clear();
    void setUniformData();

    sgl::GeometryBufferPtr fragmentNodes;
    //sgl::GeometryBufferPtr numFragmentsBuffer;

    // Blit data (ignores model-view-projection matrix and uses normalized device coordinates)
    sgl::ShaderAttributesPtr blitRenderData;
    sgl::ShaderAttributesPtr clearRenderData;

    sgl::FramebufferObjectPtr sceneFramebuffer;
    sgl::TexturePtr sceneTexture;
    sgl::RenderbufferObjectPtr sceneDepthRBO;

    bool clearBitSet;
};

#endif //PIXELSYNCOIT_OIT_MLABBUCKET_HPP