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
        struct VelocityRange
        {
            glm::vec3 Min = {-1.0f, 0.0f, -1.0f};
            glm::vec3 Max = {1.0f, 2.0f, 1.0f};
            bool UseRange = false; // Toggle para activar/desactivar el rango de velocidad
        };
        struct SizeRange
        {
            float Min = 0.5f;
            float Max = 2.0f;
            bool UseRange = false;
            bool StartWithMin = false; 
            bool StartWithMax = false;
            bool RepeatInterval = true;
        };
        struct EmissionArea
        {
            glm::vec3 Size = {0.0f, 0.0f, 0.0f}; 
            bool UseEmissionArea = false;        
            enum class Shape
            {
                Box,
                Sphere,
                Circle
            } AreaShape = Shape::Box;
        };
        struct Particle
        {
            glm::vec3 Position;
            glm::vec3 Velocity;
            glm::vec3 InitialVelocity; 
            glm::vec3 TargetVelocity;  
            glm::vec4 Color;
            float LifeTime;
            float Age;
            float Size;
            float InitialSize; 
            float TargetSize;  
        };

        const Ref<Material>& GetParticleMaterial() const { return ParticleMaterial; }
        const Ref<Mesh>& GetParticleMesh() const { return ParticleMesh; }

        ParticleSystemComponent();
        void Update(float deltaTime);
        void Render();

        // Configuraci�n del emisor
        glm::vec3 LocalEmitterPosition = {0.0f, 0.0f, 0.0f};
        glm::vec3 GlobalEmitterPosition = {0.0f, 0.0f, 0.0f};
        float EmissionRate = 10.0f;
        float ParticleLifetime = 5.0f;
        glm::vec3 Gravity = {0.0f, -9.81f, 0.0f};
        float ParticleSize = 1.0f;

        bool ApplyRotation = false;    // Controla si se aplica rotaci�n a las part�culas
        float RotationSpeed = 0.0f;    // Velocidad de rotaci�n de las part�culas
        size_t AliveParticleCount = 0; // Contador de part�culas activas en el sistema

        VelocityRange VelocityRangeConfig;
        float VelocityChangeInterval = 1.0f;

        SizeRange SizeRangeConfig;
        float SizeChangeInterval = 1.0f;

        EmissionArea EmissionAreaConfig;

        std::vector<Particle> Particles;

      private:
        void EmitParticle();
        glm::vec3 GenerateRandomVelocity() const;
        float GenerateRandomSize() const;
        glm::vec3 GenerateRandomPositionInArea() const;
        template <class Archive> void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("EmitterPosition", LocalEmitterPosition),
                    cereal::make_nvp("EmissionRate", EmissionRate),
                    cereal::make_nvp("ParticleLifetime", ParticleLifetime), cereal::make_nvp("Gravity", Gravity),
                    cereal::make_nvp("ParticleSize", ParticleSize),
                    cereal::make_nvp("VelocityRangeMin", VelocityRangeConfig.Min),
                    cereal::make_nvp("VelocityRangeMax", VelocityRangeConfig.Max),
                    cereal::make_nvp("UseVelocityRange", VelocityRangeConfig.UseRange),
                    cereal::make_nvp("VelocityChangeInterval", VelocityChangeInterval),
                    cereal::make_nvp("SizeRangeMin", SizeRangeConfig.Min),
                    cereal::make_nvp("SizeRangeMax", SizeRangeConfig.Max),
                    cereal::make_nvp("UseSizeRange", SizeRangeConfig.UseRange),
                    cereal::make_nvp("EmissionArea", EmissionAreaConfig.Size),
                    cereal::make_nvp("EmissionArea", EmissionAreaConfig.UseEmissionArea),
                    cereal::make_nvp("EmissionArea", EmissionAreaConfig.AreaShape)); 
        }

        Ref<Material> ParticleMaterial;
        Ref<Mesh> ParticleMesh;
        float EmissionAccumulator = 0.0f;
    };
} // namespace Coffee