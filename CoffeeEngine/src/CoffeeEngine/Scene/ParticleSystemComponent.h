#pragma once

#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Core/Billboard.h"
#include <glm/glm.hpp>
#include <vector>

namespace Coffee
{

    class ParticleSystemComponent
    {
      public:
        struct VelocityRange
        {
            glm::vec3 Min = {-1.0f, 0.0f, -1.0f};
            glm::vec3 Max = {1.0f, 2.0f, 1.0f};
            bool UseRange = false; // Toggle para activar/desactivar el rango de velocidad
        };
        struct Particle
        {
            glm::vec3 Position;
            glm::vec3 Velocity;
            glm::vec3 InitialVelocity; // Guardamos la velocidad inicial para interpolación
            glm::vec3 TargetVelocity;  // Velocidad objetivo para interpolación
            glm::vec4 Color;
            float LifeTime;
            float Age;
            float Size;
            Ref<Billboard> Billboard;
        };

        const Ref<Material>& GetParticleMaterial() const { return ParticleMaterial; }
        const Ref<Mesh>& GetParticleMesh() const { return ParticleMesh; }

        ParticleSystemComponent();
        void Update(float deltaTime);
        void Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition, const glm::vec3& cameraUp);

        // Configuración del emisor
        glm::vec3 LocalEmitterPosition = {0.0f, 0.0f, 0.0f}; 
        glm::vec3 GlobalEmitterPosition = {0.0f, 0.0f, 0.0f}; 
        float EmissionRate = 10.0f;
        float ParticleLifetime = 5.0f;
        glm::vec3 Gravity = {0.0f, -9.81f, 0.0f};
        float ParticleSize = 1.0f;

        bool ApplyRotation = false;    // Controla si se aplica rotación a las partículas
        float RotationSpeed = 0.0f;    // Velocidad de rotación de las partículas
        size_t AliveParticleCount = 0; // Contador de partículas activas en el sistema

        // Nuevo: Configuración del rango de velocidad
        VelocityRange VelocityRangeConfig;
        float VelocityChangeInterval = 1.0f; // Intervalo de tiempo para cambiar la velocidad

        std::vector<Particle> Particles;

      private:
        void EmitParticle();
        glm::vec3 GenerateRandomVelocity() const;

        template <class Archive> void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("EmitterPosition", LocalEmitterPosition),
                    cereal::make_nvp("EmissionRate", EmissionRate),
                    cereal::make_nvp("ParticleLifetime", ParticleLifetime), cereal::make_nvp("Gravity", Gravity),
                    cereal::make_nvp("ParticleSize", ParticleSize),
                    cereal::make_nvp("VelocityRangeMin", VelocityRangeConfig.Min),
                   cereal::make_nvp("VelocityRangeMax", VelocityRangeConfig.Max),
                   cereal::make_nvp("UseVelocityRange", VelocityRangeConfig.UseRange),
                   cereal::make_nvp("VelocityChangeInterval", VelocityChangeInterval));
        }

        Ref<Material> ParticleMaterial;
        Ref<Mesh> ParticleMesh;
        float EmissionAccumulator = 0.0f;
    };

} // namespace Coffee
