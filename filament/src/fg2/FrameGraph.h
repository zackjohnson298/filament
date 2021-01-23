/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TNT_FILAMENT_WIP_FRAMEGRAPH_H
#define TNT_FILAMENT_WIP_FRAMEGRAPH_H

#include <backend/DriverEnums.h>
#include <backend/Handle.h>

namespace filament::fg2 {

struct Texture {
    struct Descriptor {
        uint32_t width = 1;     // width of resource in pixel
        uint32_t height = 1;    // height of resource in pixel
        uint32_t depth = 1;     // # of images for 3D textures
        uint8_t levels = 1;     // # of levels for textures
        uint8_t samples = 0;    // 0=auto, 1=request not multisample, >1 only for NOT SAMPLEABLE
        backend::SamplerType type = backend::SamplerType::SAMPLER_2D;     // texture target type
        backend::TextureFormat format = backend::TextureFormat::RGBA8;    // resource internal format
    };

    enum class Usage {
        SAMPLE,
        UPLOAD
    };

    backend::Handle<backend::HwTexture> texture;
};

struct TextureSubresource {

};


/** A handle on a resource */
class FrameGraphHandle;

/** A typed handle on a resource */
template<typename RESOURCE>
class FrameGraphId<RESOURCE> : public FrameGraphHandle {
public:
    using FrameGraphHandle::FrameGraphHandle;
    FrameGraphId() noexcept = default;
    explicit FrameGraphId(FrameGraphHandle r) : FrameGraphHandle(r) { }
};

template<typename Data, typename Execute>
class FrameGraphPass;

class FrameGraph {
public:

    class RenderTargetId;

    class Builder {
    public:
        Builder(Builder const&) = delete;
        Builder& operator=(Builder const&) = delete;

//        struct RenderTargetDescriptor {
//            struct Attachments {
//                    FrameGraphId<TextureSubresource> color[4] = {};
//                    FrameGraphId<TextureSubresource> depth{};
//                    FrameGraphId<TextureSubresource> stencil{};
//            };
//            Attachments attachments{};
//            Viewport viewport{};
//            math::float4 clearColor{};
//            uint8_t samples = 0; // # of samples (0 = unset, default)
//            backend::TargetBufferFlags clearFlags{};
//        };
//        RenderTargetId useAsRenderTarget(RenderTargetDescriptor const& desc);
//        RenderTargetId useAsRenderTarget(FrameGraphId<TextureSubresource> color);
//        RenderTargetId useAsRenderTarget(FrameGraphId<TextureSubresource> color, FrameGraphId<TextureSubresource> depth);


        /**
         * Creates a virtual resource of type RESOURCE
         * @tparam RESOURCE Type of the resource to create
         * @param name      Name of the resource
         * @param desc      Descriptor for this resources
         * @return          A typed resource handle
         */
        template<typename RESOURCE>
        FrameGraphId<RESOURCE> create(const char* name, typename RESOURCE::Descriptor const& desc = {}) noexcept;


        /**
         * Creates a subresource of the virtual resource of type RESOURCE. This adds a reference
         * from the subresource to the resource.
         *
         * @tparam RESOURCE         Type of the virtual resource
         * @tparam SUBRESOURCE      Type of the subresource
         * @param name              A name for the subresource
         * @param desc              Descriptor of the subresource
         * @return                  A handle to the subresource
         */
        template<typename RESOURCE, typename SUBRESOURCE>
        FrameGraphId<SUBRESOURCE> createSubresource(FrameGraphId<RESOURCE>,
                const char* name, typename SUBRESOURCE::Descriptor const& desc = {}) noexcept;


        /**
         * Declares a read access by this pass to a virtual resource. This adds a reference from
         * the pass to the resource.
         * @tparam RESOURCE Type of the resource
         * @param input     Handle to the resource
         * @param usage     How is this resource used. e.g.: sample vs. upload for textures. This is resource dependant.
         * @return          A new handle to the resource. The input handle is no-longer valid.
         */
        template<typename RESOURCE>
        FrameGraphId<RESOURCE> read(FrameGraphId<RESOURCE> input, RESOURCE::Usage usage = {});

        /**
         * Declares a write access by this pass to a virtual resource. This adds a reference from
         * the resource to the pass.
         * @tparam RESOURCE Type of the resource
         * @param input     Handle to the resource
         * @param usage     How is this resource used. This is resource dependant.
         * @return          A new handle to the resource. The input handle is no-longer valid.
         */
        template<typename RESOURCE>
        FrameGraphId<RESOURCE> write(FrameGraphId<RESOURCE> input, RESOURCE::Usage usage = {});

        /**
         * Marks the current pass as a leaf. Adds a reference to it, so it's not culled.
         */
        void sideEffect() noexcept;
    };

    // --------------------------------------------------------------------------------------------

    class Resources {
    public:
        Resources(Resources const&) = delete;
        Resources& operator=(Resources const&) = delete;

        /**
         * Return the name of the pass being executed
         * @return a pointer to a null terminated string. The caller doesn't get ownership.
         */
        const char* getPassName() const noexcept;

        /**
         * Retrieves the concrete resource for a given handle to a virtual resource.
         * @tparam RESOURCE Type of the resource
         * @param handle    Handle to a virtual resource
         * @return          Reference to the concrete resource
         */
        template<typename RESOURCE>
        RESOURCE const& get(FrameGraphId<RESOURCE> handle) const noexcept;

        /**
         * Retrieves the descriptor associated to a resource
         * @tparam RESOURCE Type of the resource
         * @param handle    Handle to a virtual resource
         * @return          Reference to the descriptor
         */
        template<typename RESOURCE>
        typename RESOURCE::Descriptor const& getDescriptor(FrameGraphId<RESOURCE> handle) const;
    };

    // --------------------------------------------------------------------------------------------

    explicit FrameGraph(ResourceAllocatorInterface& resourceAllocator);
    FrameGraph(FrameGraph const&) = delete;
    FrameGraph& operator=(FrameGraph const&) = delete;
    ~FrameGraph();

    /**
     * Add a pass to the framegraph.
     *
     * @tparam Data     A user-defined structure containing this pass data
     * @tparam Setup    A lambda of type [](Builder&, Data&).
     * @tparam Execute  A lambda of type [](Resources const&, Data const&, DriverApi&)
     *
     * @param name      A name for this pass. Used for debugging only.
     * @param setup     lambda called synchronously, used to declare which and how resources are
     *                  used by this pass. Captures should be done by reference.
     * @param execute   lambda called asynchronously from FrameGraph::execute(),
     *                  where immediate drawing commands can be issued.
     *                  Captures must be done by copy.
     *
     * @return          A reference to a FrameGraphPass object
     */
    template<typename Data, typename Setup, typename Execute>
    FrameGraphPass<Data, Execute>& addPass(const char* name, Setup setup, Execute&& execute);


    /**
     * Allocates concrete resources and culls unreferenced passes.
     * @return a reference to the FrameGraph, for chaining calls.
     */
    FrameGraph& compile() noexcept;

    /**
     * Execute all referenced passes
     *
     * @param driver a reference to the backend to execute the commands
     */
    void execute(backend::DriverApi& driver) noexcept;


    /**
     * Moves the resource associated to the handle 'from' to the handle 'to'. After this call,
     * all handles referring to the resource 'to' are redirected to the resource 'from'
     * (including handles used in the past).
     * All writes to 'from' are disconnected (i.e. these passes loose a reference).
     *
     * @tparam RESOURCE     Type of the resources
     * @param from          Resource to be moved
     * @param to            Resource to be replaced
     */
    template<typename RESOURCE>
    void moveResource(FrameGraphId<RESOURCE> from, FrameGraphId<RESOURCE> to);

    /**
     * Adds a reference to 'input', preventing it from being culled
     *
     * @param input a resource handle
     */
    void present(FrameGraphHandle input);


    /**
     * Imports a concrete resource to the frame graph. The lifetime management is not transferred
     * to the frame graph.
     *
     * @tparam RESOURCE     Type of the resource to import
     * @param name          A name for this resource
     * @param desc          The descriptor for this resource
     * @param resource      A reference to the resource itself
     * @return              A handle that can be used normally in the frame graph
     */
    template<typename RESOURCE>
    FrameGraphId<RESOURCE> import(const char* name,
            typename RESOURCE::Descriptor const& desc, const RESOURCE& resource) noexcept;

    /**
     * Imports a RenderTarget as a TextureSubresource into the frame graph. Later, this
     * TextureSubresource can be used with useAsRenderTarget(), the resulting concrete RenderTarget
     * will be the the one passed as argument here, instead of being dynamically created.
     *
     * @param name      A name for the rendter target
     * @param desc      Descriptor for the imported TextureSubresource
     * @param target    handle to the concrete render target to import
     * @return          A handle to a TextureSubresource
     */
    FrameGraphId<TextureSubresource> import(const char* name,
            const TextureSubresource::Descriptor& desc,
            backend::Handle<backend::HwRenderTarget> target)
};

} // namespace filament

#endif //TNT_FILAMENT_WIP_FRAMEGRAPH_H
