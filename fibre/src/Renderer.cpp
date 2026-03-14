#include "Renderer.h"

#include <random>
#include <vector>
#include <execution>
#include <algorithm>

namespace fibre {

    namespace utils {

        static uint32_t convertToRGBA(const glm::vec4& color)
        {
            uint8_t r = static_cast<uint8_t>(color.r * 255.0f);
            uint8_t g = static_cast<uint8_t>(color.g * 255.0f);
            uint8_t b = static_cast<uint8_t>(color.b * 255.0f);
            uint8_t a = static_cast<uint8_t>(color.a * 255.0f);

            uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
            return result;
        }

        static uint32_t pcgHash(uint32_t input)
        {
            uint32_t state = input * 747796405u + 2891336453u;
            uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            return (word >> 22u) ^ word;
        }

        static float randomFloat(uint32_t& seed)
        {
            seed = pcgHash(seed);
            return (float)seed / (float)std::numeric_limits<uint32_t>::max();
        }

        static glm::vec3 inUnitSphere(uint32_t& seed)
        {
            return glm::normalize(glm::vec3(randomFloat(seed) * 2.0f - 1.0f, randomFloat(seed) * 2.0f - 1.0f, randomFloat(seed) * 2.0f - 1.0f));
        }

    }
    
    Renderer::Renderer(const std::shared_ptr<wire::Device>& device)
        : m_Device(device)
    {
        delete[] m_AccumulationData;
        delete[] m_ImageData;
        m_ImageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];
        m_AccumulationData = new glm::vec4[m_ViewportWidth * m_ViewportHeight];
        m_FinalImage = m_Device->createTexture2D(m_ImageData, m_ViewportWidth, m_ViewportHeight, "Renderer::m_FinalImage");

        m_ImageHorizontalIter.resize(m_ViewportWidth);
        m_ImageVerticalIter.resize(m_ViewportHeight);
        for (uint32_t i = 0; i < m_ViewportWidth; i++)
            m_ImageHorizontalIter[i] = i;
        for (uint32_t i = 0; i < m_ViewportHeight; i++)
            m_ImageVerticalIter[i] = i;
    }

    void Renderer::render(const Scene& scene, const Camera& camera)
    {
        // TODO: resize
        if (!m_FinalImage)
        {
            m_FinalImage = m_Device->createTexture2D(nullptr, m_ViewportWidth, m_ViewportHeight, "Renderer::m_FinalImage");
            delete[] m_AccumulationData;
            delete[] m_ImageData;
            m_ImageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];
            m_AccumulationData = new glm::vec4[m_ViewportWidth * m_ViewportHeight];
        }

        m_ActiveScene = &scene;
        m_ActiveCamera = &camera;

        if (m_FrameIndex == 1)
            std::memset(m_AccumulationData, 0, sizeof(glm::vec4) * m_ViewportWidth * m_ViewportHeight);

#define MT 1

#if MT
        std::for_each(std::execution::par, m_ImageVerticalIter.begin(), m_ImageVerticalIter.end(), [this](uint32_t y) {
            std::for_each(std::execution::par, m_ImageHorizontalIter.begin(), m_ImageHorizontalIter.end(), [this, y](uint32_t x) {
                glm::vec4 color = perPixel(x, y);
                m_AccumulationData[x + y * m_ViewportWidth] += color;

                glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_ViewportWidth];
                accumulatedColor /= (float)m_FrameIndex;

                accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_ViewportWidth] = utils::convertToRGBA(accumulatedColor);
            });
        });
#else
        for (uint32_t y = 0; y < m_ViewportHeight; y++)
        {
            for (uint32_t x = 0; x < m_ViewportWidth; x++)
            {
                glm::vec4 color = perPixel(x, y);
                m_AccumulationData[x + y * m_ViewportWidth] += color;

                glm::vec4 accumulatedColor = m_AccumulationData[x + y * m_ViewportWidth];
                accumulatedColor /= (float)m_FrameIndex;

                accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
                m_ImageData[x + y * m_ViewportWidth] = utils::convertToRGBA(accumulatedColor);
            }
        }
#endif

        m_FinalImage->setData(m_ImageData, sizeof(uint32_t) * m_ViewportWidth * m_ViewportHeight);

        if (m_Settings.Accumulate)
            m_FrameIndex++;
        else
            m_FrameIndex = 1;
    }

    glm::vec4 Renderer::perPixel(uint32_t x, uint32_t y)
    {
        Ray ray;
        ray.Origin = m_ActiveCamera->getPosition();
        ray.Direction = m_ActiveCamera->getRayDirections()[x + y * m_ViewportWidth];

        glm::vec3 light(0.0f);
        glm::vec3 contribution(1.0f);

        uint32_t seed = x + y * m_ViewportWidth;
        seed *= m_FrameIndex;

        const uint32_t bounces = 5;
        for (uint32_t i = 0; i < bounces; i++)
        {
            seed += i;

            Renderer::HitPayload payload = traceRay(ray);
            if (payload.HitDistance < 0.0f)
            {
                glm::vec3 skyColor(0.6f, 0.7f, 0.9f);
                //light += skyColor * contribution;
                break;
            }

            const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
            const Material& mat = m_ActiveScene->Materials[sphere.MaterialIndex];

            contribution *= glm::vec3(mat.Albedo);
            light += mat.getEmission();

            ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.0001f;
            ray.Direction = glm::normalize(payload.WorldNormal + utils::inUnitSphere(seed));
        }

        return glm::vec4(light, 1.0f);
    }

    Renderer::HitPayload Renderer::traceRay(const Ray &ray)
    {
        int closestSphere = -1;
        float hitDistance = std::numeric_limits<float>::max();

        for (int i = 0; i < m_ActiveScene->Spheres.size(); i++)
        {
            const Sphere& sphere = m_ActiveScene->Spheres[i];
            glm::vec3 origin = ray.Origin - glm::vec3(sphere.Position);

            float a = glm::dot(ray.Direction, ray.Direction);
            float b = 2.0f * glm::dot(origin, ray.Direction);
            float c = glm::dot(origin, origin) - (sphere.Radius * sphere.Radius);

            float discriminant = b * b - 4 * a * c;

            if (discriminant < 0)
                continue;

            float t0 = (-b + std::sqrtf(discriminant)) / (2 * a);
            float closestT = (-b - std::sqrtf(discriminant)) / (2 * a);

            if (closestT > 0.0f && closestT < hitDistance)
            {
                hitDistance = closestT;
                closestSphere = i;
            }
        }

        if (closestSphere < 0)
            return miss(ray);

        return closestHit(ray, hitDistance, closestSphere);
    }

    Renderer::HitPayload Renderer::closestHit(const Ray& ray, float hitDistance, uint32_t objectIndex)
    {
        Renderer::HitPayload payload;
        payload.HitDistance = hitDistance;
        payload.ObjectIndex = objectIndex;

        const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

        glm::vec3 origin = ray.Origin - glm::vec3(closestSphere.Position);

        payload.WorldPosition = origin + ray.Direction * hitDistance;
        payload.WorldNormal = glm::normalize(payload.WorldPosition);
        payload.WorldPosition += closestSphere.Position;

        return payload;
    }

    Renderer::HitPayload Renderer::miss(const Ray &ray)
    {
        return HitPayload{
            .HitDistance = -1.0f
        };
    }

}