#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace fibre {

    struct alignas(16) Material
    {
        glm::vec4 Albedo{ 1.0f };
        float Roughness = 1.0f;
        float Metallic = 0.0f;
        glm::vec2 Padding0;
        glm::vec3 EmissionColor{ 0.0f };
        float EmissionPower = 0.0f;
        glm::vec4 Padding1;

        glm::vec3 getEmission() const { return EmissionColor * EmissionPower; }
    };

    struct Sphere
    {
        glm::vec4 Position{ 0.0f };
        float Radius = 0.5f;
        int MaterialIndex = 0;
        glm::vec2 Padding;
    };

    struct Scene
    {
        std::vector<Sphere> Spheres;
        std::vector<Material> Materials;
    };

}
