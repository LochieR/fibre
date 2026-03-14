#pragma once

#include "Wire.h"

#include "Renderer.h"
#include "Renderer2.h"
#include "Camera.h"
#include "Bloom.h"

namespace fibre {

    class RayTracingLayer : public wire::Layer
    {
    public:
        RayTracingLayer();
        ~RayTracingLayer() = default;
        
        virtual void onAttach() override;
        virtual void onDetach() override;
        virtual void onImGuiRender() override;
        virtual void onUpdate(float timestep) override;
    private:
        std::shared_ptr<wire::Device> m_Device;

        float m_RenderTime = 0.0f;

        bool m_GPURender = true;

        Renderer m_Renderer;
        Renderer2 m_Renderer2;
        Camera m_Camera;
        Scene m_Scene;
        Bloom m_Bloom;
        std::shared_ptr<wire::Sampler> m_Sampler;
        std::shared_ptr<wire::ShaderResourceLayout> m_ImGuiResourceLayout;
        std::shared_ptr<wire::ShaderResource> m_ImGuiResource;
    };

}
