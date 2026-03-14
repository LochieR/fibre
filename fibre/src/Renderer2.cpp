#include "Renderer2.h"

namespace fibre {

    struct CameraUniformBuffer
    {
        glm::mat4 InverseProjection;
        glm::mat4 InverseView;
        glm::vec4 Position;
        glm::vec4 ViewportSize;
    };

    struct SceneUniformBuffer
    {
        int SphereCount;
        int MaterialCount;
        glm::vec2 Padding;
        Sphere Spheres[10];
        Material Materials[10];
    };

    struct PushConstantBuffer
    {
        uint32_t FrameIndex;
        bool Accumulate;
    };

    Renderer2::Renderer2(const std::shared_ptr<wire::Device>& device)
        : m_Device(device)
    {
        wire::ShaderResourceLayoutInfo pipelineLayoutInfo{
            .Sets = {
                wire::ShaderResourceSetInfo{
                    .Resources = {
                        wire::ShaderResourceInfo{ // accumulation image
                            .Binding = 0,
                            .Type = wire::ShaderResourceType::StorageImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{ // final image
                            .Binding = 1,
                            .Type = wire::ShaderResourceType::StorageImage,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{ // camera uniform buffer
                            .Binding = 2,
                            .Type = wire::ShaderResourceType::UniformBuffer,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        },
                        wire::ShaderResourceInfo{ // scene uniform buffer
                            .Binding = 3,
                            .Type = wire::ShaderResourceType::UniformBuffer,
                            .ArrayCount = 1,
                            .Stage = wire::ShaderType::Compute
                        }
                    }
                }
            }
        };
        m_PipelineLayout = device->createShaderResourceLayout(pipelineLayoutInfo, "Renderer2::m_PipelineLayout");

        wire::ComputeInputLayout pipelineLayout{
            .PushConstantInfos = {
                wire::PushConstantInfo{
                    .Size = sizeof(PushConstantBuffer),
                    .Offset = 0,
                    .Shader = wire::ShaderType::Compute
                }
            },
            .ResourceLayout = m_PipelineLayout
        };

        wire::ComputePipelineDesc pipelineInfo{};
        pipelineInfo.Layout = pipelineLayout;
        pipelineInfo.ShaderPath = "shadercache://main.compute.hlsl";

        m_Pipeline = device->createComputePipeline(pipelineInfo, "Renderer2::m_Pipeline");
        m_CommandList = device->createCommandList();

        m_PipelineResource = device->createShaderResource(0, m_PipelineLayout, "Renderer2::m_PipelineResource");
        
        wire::FramebufferDesc framebufferInfo{};
        framebufferInfo.Format = wire::AttachmentFormat::RGBA32_SFloat;
        framebufferInfo.Usage = wire::AttachmentUsage::Storage | wire::AttachmentUsage::Sampled;
        framebufferInfo.Layout = wire::AttachmentLayout::General;
        framebufferInfo.Extent = { (float)m_ViewportWidth, (float)m_ViewportHeight };
        framebufferInfo.MipCount = 1;
        framebufferInfo.HasDepth = false;

        m_FinalImage = device->createFramebuffer(framebufferInfo, "Renderer2::m_FinalImage");
        framebufferInfo.Usage |= wire::AttachmentUsage::TransferDst;
        m_AccumulatedImage = device->createFramebuffer(framebufferInfo, "Renderer2::m_AccumulatedImage");

        m_CameraBuffer = device->createBuffer(wire::BufferType::UniformBuffer, sizeof(CameraUniformBuffer));
        m_SceneBuffer = device->createBuffer(wire::BufferType::UniformBuffer, sizeof(SceneUniformBuffer));

        m_PipelineResource->update(m_AccumulatedImage, 0, 0);
        m_PipelineResource->update(m_FinalImage, 1, 0);
        m_PipelineResource->update(m_CameraBuffer, 2, 0);
        m_PipelineResource->update(m_SceneBuffer, 3, 0);

        wire::CommandList commandList = device->beginSingleTimeCommands();
        commandList.imageMemoryBarrier(m_FinalImage, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly);
        //commandList.imageMemoryBarrier(m_AccumulatedImage, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly);
        device->endSingleTimeCommands(commandList);

        m_FinalImageTexture = m_FinalImage->asTexture2D();
        m_AccumulatedImageTexture = m_AccumulatedImage->asTexture2D();

        m_Bloom = Bloom(m_Device, m_FinalImage, m_ViewportWidth, m_ViewportHeight);
    }

    void Renderer2::render(const Scene& scene, const Camera& camera)
    {
        constexpr uint32_t localSizeX = 16, localSizeY = 16;
        uint32_t groupCountX = (m_ViewportWidth + localSizeX - 1) / localSizeX;
        uint32_t groupCountY = (m_ViewportHeight + localSizeY - 1) / localSizeY;

        CameraUniformBuffer cameraUniformData{
            .InverseProjection = camera.getInverseProjection(),
            .InverseView = camera.getInverseView(),
            .Position = glm::vec4(camera.getPosition(), 1.0),
            .ViewportSize = glm::vec4((float)m_ViewportWidth, (float)m_ViewportHeight, 0.0, 0.0)
        };

        SceneUniformBuffer sceneUniformData{
            .SphereCount = static_cast<int>(scene.Spheres.size()),
            .MaterialCount = static_cast<int>(scene.Materials.size()),
        };

        std::memcpy(sceneUniformData.Spheres, scene.Spheres.data(), scene.Spheres.size() * sizeof(Sphere));
        std::memcpy(sceneUniformData.Materials, scene.Materials.data(), scene.Materials.size() * sizeof(Material));

        PushConstantBuffer pushConstants{
            .FrameIndex = m_FrameIndex,
            .Accumulate = m_Settings.Accumulate
        };

        m_CameraBuffer->setData(&cameraUniformData, sizeof(CameraUniformBuffer));
        m_SceneBuffer->setData(&sceneUniformData, sizeof(SceneUniformBuffer));

        m_CommandList.begin();

        if (m_FrameIndex == 1)
        {
            m_CommandList.clearImage(m_AccumulatedImage, { 0.0f, 0.0f, 0.0f, 0.0f }, wire::AttachmentLayout::General);
            //m_CommandList.imageMemoryBarrier(m_AccumulatedImage, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly);
        }
        
        m_CommandList.imageMemoryBarrier(m_FinalImage, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General);
        //m_CommandList.imageMemoryBarrier(m_AccumulatedImage, wire::AttachmentLayout::ShaderReadOnly, wire::AttachmentLayout::General);

        m_CommandList.bindPipeline(m_Pipeline);
        m_CommandList.bindShaderResource(0, m_PipelineResource);
        m_CommandList.pushConstants(wire::ShaderType::Compute, pushConstants);
        m_CommandList.dispatch(groupCountX, groupCountY, 1);

        if (m_Settings.Accumulate)
        {
            for (uint32_t i = 0; i < 50; i++)
            {
                m_FrameIndex += 1;
                pushConstants.FrameIndex = m_FrameIndex;
                m_CommandList.pushConstants(wire::ShaderType::Compute, pushConstants);
                m_CommandList.dispatch(groupCountX, groupCountY, 1);
            }
        }
        else
        {
            m_FrameIndex = 1;
            pushConstants.FrameIndex = m_FrameIndex;
            m_CommandList.pushConstants(wire::ShaderType::Compute, pushConstants);
            m_CommandList.dispatch(groupCountX, groupCountY, 1);
        }

        //m_CommandList.imageMemoryBarrier(m_AccumulatedImage, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly);
        m_CommandList.imageMemoryBarrier(m_FinalImage, wire::AttachmentLayout::General, wire::AttachmentLayout::ShaderReadOnly);

        m_Bloom.apply(m_CommandList);

        m_CommandList.end();
        m_Device->submitCommandList(m_CommandList);
    }

}
