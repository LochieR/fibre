#include "RayTracingLayer.h"

#include "Timer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

namespace fibre {
    
    RayTracingLayer::RayTracingLayer()
        : m_Camera(45.0f, 0.1f, 100.0f)
    {
    }

    void RayTracingLayer::onAttach()
    {
        m_Device = wire::Application::get().getDevice();
        m_Renderer = Renderer(m_Device);
        m_Renderer2 = Renderer2(m_Device);

        wire::SamplerDesc samplerInfo{};
        samplerInfo.MinFilter = wire::SamplerFilter::Nearest;
        samplerInfo.MagFilter = wire::SamplerFilter::Nearest;
        samplerInfo.AddressModeU = wire::AddressMode::ClampToEdge;
        samplerInfo.AddressModeV = wire::AddressMode::ClampToEdge;
        samplerInfo.AddressModeW = wire::AddressMode::ClampToEdge;
        samplerInfo.EnableAnisotropy = false;
        samplerInfo.BorderColor = wire::BorderColor::IntOpaqueBlack;
        samplerInfo.MipmapMode = wire::MipmapMode::Nearest;
        samplerInfo.CompareEnable = false;
        samplerInfo.MinLod = 0.0f;
        samplerInfo.MaxLod = 1.0f;
        samplerInfo.NoMaxLodClamp = true;
        samplerInfo.MipLodBias = 0.0f;

        m_Sampler = m_Device->createSampler(samplerInfo, "RayTracingLayer::m_Sampler");

        wire::ShaderResourceLayoutInfo textureLayout{};
        textureLayout.Sets = {
            wire::ShaderResourceSetInfo{
                .Resources = {
                    wire::ShaderResourceInfo{
                        .Binding = 0,
                        .Type = wire::ShaderResourceType::CombinedImageSampler,
                        .ArrayCount = 1,
                        .Stage = wire::ShaderType::Pixel
                    }
                }
            }
        };

        m_ImGuiResourceLayout = m_Device->createShaderResourceLayout(textureLayout, "RayTracingLayer::m_ImGuiResourceLayout");
        m_ImGuiResource = m_Device->createShaderResource(0, m_ImGuiResourceLayout, "RayTracingLayer::m_ImGuiResource");

        if (m_GPURender)
            m_ImGuiResource->update(m_Renderer2.getFinalImage(), m_Sampler, 0, 0);
        else
            m_ImGuiResource->update(m_Renderer.getFinalImage(), m_Sampler, 0, 0);

        m_Scene.Spheres.push_back(Sphere{
            .Position = { 0.0f, 0.0f, 0.0f, 1.0f },
            .Radius = 1.0f,
            .MaterialIndex = 0,
        });

        m_Scene.Spheres.push_back(Sphere{
            .Position = { 25.0f, 0.0f, -22.0f, 1.0f },
            .Radius = 18.0f,
            .MaterialIndex = 2
        });

        m_Scene.Spheres.push_back(Sphere{
            .Position = { 0.0f, -101.0f, 0.0f, 1.0f },
            .Radius = 100.0f,
            .MaterialIndex = 1,
        });

        m_Scene.Materials.push_back(Material{
            .Albedo = { 1.0f, 0.2f, 1.0f, 1.0f },
            .Roughness = 0.0f,
            .Metallic = 0.0f
        });

        m_Scene.Materials.push_back(Material{
            .Albedo = { 0.2f, 0.2f, 1.0f, 1.0f },
            .Roughness = 0.7f,
            .Metallic = 0.0f
        });

        m_Scene.Materials.push_back(Material{
            .Albedo = { 0.8f, 0.5f, 0.2f, 1.0f },
            .Roughness = 0.1f,
            .Metallic = 0.0f,
            .EmissionColor = { 0.8f, 0.5f, 0.2f },
            .EmissionPower = 20.0f
        });
    }

    void RayTracingLayer::onDetach()
    {
    }

    void RayTracingLayer::onImGuiRender()
    {
        glm::vec2 extent = m_Device->getExtent();

        glm::ivec2 position;
        glfwGetWindowPos(wire::Application::get().getWindow(), &position.x, &position.y);

        ImGui::SetNextWindowSize({ extent.x, extent.y });
        ImGui::SetNextWindowPos({ static_cast<float>(position.x), static_cast<float>(position.y) });
        ImGui::PushStyleColor(ImGuiCol_Border, { 0.0f, 0.0f, 0.0f, 0.0f });
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("##raytracing", nullptr, ImGuiWindowFlags_NoDecoration);
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.0f, 0.0f });
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(-1, m_Renderer.getImageSize().x);
        
        ImGui::Image(m_ImGuiResource->getHandle(), { m_Renderer.getImageSize().x, m_Renderer.getImageSize().y }, { 0, 1 }, { 1, 0 });
        
        ImGui::NextColumn();
        ImGui::PopStyleVar();

        ImGui::Text("Last render time = %.3fms", m_RenderTime);

        ImGui::SeparatorText("Settings");

        if (ImGui::Checkbox("GPU render", &m_GPURender))
        {
            if (m_GPURender)
            {
                m_ImGuiResource->update(m_Renderer2.getFinalImage(), m_Sampler, 0, 0);
                m_Renderer2.resetFrameIndex();
            }
            else
            {
                m_ImGuiResource->update(m_Renderer.getFinalImage(), m_Sampler, 0, 0);
                m_Renderer.resetFrameIndex();
            }
        }

        if (ImGui::Button("Reset"))
        {
            if (m_GPURender)
                m_Renderer2.resetFrameIndex();
            else
                m_Renderer.resetFrameIndex();
        }

        if (m_GPURender)
            ImGui::Checkbox("Accumulate##1", &m_Renderer2.getSettings().Accumulate);
        else
            ImGui::Checkbox("Accumulate##2", &m_Renderer.getSettings().Accumulate);

        ImGui::SeparatorText("Scene");
        uint32_t i = 0;
        std::vector<uint32_t> toDelete;
        for (Sphere& sphere : m_Scene.Spheres)
        {
            ImVec2 currentPos = ImGui::GetCursorPos();

            ImGui::PushID(("sphere" + std::to_string(i)).c_str());
            ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f, 0.0f, 0.0f, "%.2f");

            ImVec2 nextPos = ImGui::GetCursorPos();

            ImGui::SetCursorPos({ currentPos.x + ImGui::GetContentRegionAvail().x - (ImGui::CalcTextSize("x").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().WindowPadding.x), currentPos.y });
            if (ImGui::Button("x"))
                toDelete.push_back(i);

            ImGui::DragFloat("Radius", &sphere.Radius, 0.01f, 0.0f, 0.0f, "%.2f");
            ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);
            
            ImGui::Separator();
            ImGui::PopID();

            i++;
        }

        if (ImGui::Button("Add Sphere", { ImGui::GetContentRegionAvail().x, 0.0f }))
            m_Scene.Spheres.push_back({});

        i = 0;
        for (Material& material : m_Scene.Materials)
        {
            ImGui::PushID(("mat" + std::to_string(i)).c_str());

            ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
            ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Metallic", &material.Metallic, 0.01f, 0.0f, 1.0f, "%.2f");
            ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
            ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.01f, 0.0f, std::numeric_limits<float>::max(), "%.2f");

            ImGui::Separator();
            ImGui::PopID();
            i++;
        }

        if (ImGui::Button("Add Material", { ImGui::GetContentRegionAvail().x, 0.0f }))
            m_Scene.Materials.push_back({});

        for (uint32_t index : toDelete)
            m_Scene.Spheres.erase(m_Scene.Spheres.begin() + index);

        ImGui::End();
    }

    void RayTracingLayer::onUpdate(float timestep)
    {
        if (m_Camera.onUpdate(timestep))
        {
            if (m_GPURender)
                m_Renderer2.resetFrameIndex();
            else
                m_Renderer.resetFrameIndex();
        }

        if (m_GPURender)
            m_Camera.onResize((uint32_t)m_Renderer2.getImageSize().x, (uint32_t)m_Renderer2.getImageSize().y);
        else
            m_Camera.onResize((uint32_t)m_Renderer.getImageSize().x, (uint32_t)m_Renderer.getImageSize().y);

        Timer timer;
        if (!m_GPURender)
            m_Renderer.render(m_Scene, m_Camera);
        else
            m_Renderer2.render(m_Scene, m_Camera);
        m_RenderTime = timer.elapsedMillis();

    }

}
