#pragma once

#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Scene/Components.h"
#include <glm/glm.hpp>
#include <vector>

namespace Coffee
{

    class ParticleSystemComponent
    {
      public:
        struct Particle
        {
            glm::vec3 Position;
            glm::vec3 Velocity;
            glm::vec4 Color;
            float LifeTime;
            float Age;
            float Size;
        };

        const Ref<Material>& GetParticleMaterial() const { return ParticleMaterial; }
        const Ref<Mesh>& GetParticleMesh() const { return ParticleMesh; }

        ParticleSystemComponent();
        void Update(float deltaTime);
        void Render();

        // Configuración del emisor
        glm::vec3 EmitterPosition = {0.0f, 0.0f, 0.0f};
        float EmissionRate = 10.0f;
        float ParticleLifetime = 5.0f;
        glm::vec3 Gravity = {0.0f, -9.81f, 0.0f};
        float ParticleSize = 1.0f;

        bool ApplyRotation = false;    // Controla si se aplica rotación a las partículas
        float RotationSpeed = 0.0f;    // Velocidad de rotación de las partículas
        size_t AliveParticleCount = 0; // Contador de partículas activas en el sistema

        std::vector<Particle> Particles;

      private:
        void EmitParticle();

        template <class Archive> void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("EmitterPosition", EmitterPosition),
                    cereal::make_nvp("EmissionRate", EmissionRate),
                    cereal::make_nvp("ParticleLifetime", ParticleLifetime), cereal::make_nvp("Gravity", Gravity),
                    cereal::make_nvp("ParticleSize", ParticleSize));
        }

        Ref<Material> ParticleMaterial;
        Ref<Mesh> ParticleMesh;
        float EmissionAccumulator = 0.0f;
    };

} // namespace Coffee
