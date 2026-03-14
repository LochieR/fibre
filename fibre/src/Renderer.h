#pragma once

#include "Ray.h"
#include "Camera.h"
#include "Scene.h"

#include "Wire/Renderer/Device.h"

#include <vector>

namespace fibre {

    class Renderer
    {
    public:
        struct Settings
        {
            bool Accumulate = true;
        };
    public:
        Renderer() = default;
        Renderer(const std::shared_ptr<wire::Device>& device);

        void render(const Scene& scene, const Camera& camera);

        std::shared_ptr<wire::Texture2D> getFinalImage() const { return m_FinalImage; }
        glm::vec2 getImageSize() const { return { (float)m_ViewportWidth, (float)m_ViewportHeight }; };

        void resetFrameIndex() { m_FrameIndex = 1; }
        Settings& getSettings() { return m_Settings; }
    private:
        struct HitPayload
        {
            float HitDistance;
            glm::vec3 WorldPosition;
            glm::vec3 WorldNormal;

            int ObjectIndex;
        };
    private:
        glm::vec4 perPixel(uint32_t x, uint32_t y);

        HitPayload traceRay(const Ray& ray);
        HitPayload closestHit(const Ray& ray, float hitDistance, uint32_t objectIndex);
        HitPayload miss(const Ray& ray);
    private:
        uint32_t m_ViewportWidth = 600, m_ViewportHeight = 600;
        uint32_t* m_ImageData = nullptr;
        glm::vec4* m_AccumulationData = nullptr;

        uint32_t m_FrameIndex = 1;
        Settings m_Settings;

        const Scene* m_ActiveScene = nullptr;
        const Camera* m_ActiveCamera = nullptr;

        std::shared_ptr<wire::Device> m_Device;
        std::shared_ptr<wire::Texture2D> m_FinalImage;

        std::vector<uint32_t> m_ImageVerticalIter;
        std::vector<uint32_t> m_ImageHorizontalIter;
    };

}
