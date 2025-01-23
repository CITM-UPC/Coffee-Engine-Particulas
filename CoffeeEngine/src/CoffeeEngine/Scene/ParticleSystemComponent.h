#pragma once

#include "CoffeeEngine/Core/Billboard.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Scene/Components.h"
#include <cereal/cereal.hpp> // Incluir cereal para serialización
#include <glm/glm.hpp>
#include <vector>

namespace Coffee
{

    class ParticleSystemComponent
    {
      public:
        // Rango de velocidades para las partículas
        struct VelocityRange
        {
            glm::vec3 Min = {-1.0f, 0.0f, -1.0f};
            glm::vec3 Max = {1.0f, 2.0f, 1.0f};
            bool UseRange = false;

            // Función de serialización
            template <class Archive> void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Min", Min), cereal::make_nvp("Max", Max),
                        cereal::make_nvp("UseRange", UseRange));
            }
        };

        // Rango de tamaños para las partículas
        struct SizeRange
        {
            float Min = 0.5f;
            float Max = 2.0f;
            bool UseRange = false;
            bool StartWithMin = false;
            bool StartWithMax = false;
            bool RepeatInterval = true;

            // Función de serialización
            template <class Archive> void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Min", Min), cereal::make_nvp("Max", Max),
                        cereal::make_nvp("UseRange", UseRange), cereal::make_nvp("StartWithMin", StartWithMin),
                        cereal::make_nvp("StartWithMax", StartWithMax),
                        cereal::make_nvp("RepeatInterval", RepeatInterval));
            }
        };

        // Área de emisión de partículas
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

            // Función de serialización
            template <class Archive> void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Size", Size), cereal::make_nvp("UseEmissionArea", UseEmissionArea),
                        cereal::make_nvp("AreaShape", AreaShape));
            }
        };

        // Partícula individual
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
            Ref<Billboard> Billboard;

            // Función de serialización
            template <class Archive> void serialize(Archive& archive)
            {
                archive(cereal::make_nvp("Position", Position), cereal::make_nvp("Velocity", Velocity),
                        cereal::make_nvp("InitialVelocity", InitialVelocity),
                        cereal::make_nvp("TargetVelocity", TargetVelocity), cereal::make_nvp("Color", Color),
                        cereal::make_nvp("LifeTime", LifeTime), cereal::make_nvp("Age", Age),
                        cereal::make_nvp("Size", Size), cereal::make_nvp("InitialSize", InitialSize),
                        cereal::make_nvp("TargetSize", TargetSize));
            }
        };

        // Getters y setters
        const Ref<Material>& GetParticleMaterial() const { return ParticleMaterial; }
        const Ref<Mesh>& GetParticleMesh() const { return ParticleMesh; }
        const Ref<Texture2D>& GetParticleTexture() const { return ParticleTexture; }
        void SetParticleTexture(const Ref<Texture2D>& texture) { ParticleTexture = texture; }

        // Constructor
        ParticleSystemComponent();

        // Métodos principales
        void Update(float deltaTime);
        void Render(const glm::vec3& cameraPosition, const glm::vec3& cameraUp);

        // Configuración del emisor
        glm::vec3 LocalEmitterPosition = {0.0f, 0.0f, 0.0f};
        glm::vec3 GlobalEmitterPosition = {0.0f, 0.0f, 0.0f};
        float EmissionRate = 10.0f;
        float ParticleLifetime = 5.0f;
        glm::vec3 Gravity = {0.0f, -9.81f, 0.0f};
        float ParticleSize = 1.0f;

        // Configuración de rotación y partículas activas
        bool ApplyRotation = false;
        float RotationSpeed = 0.0f;
        size_t AliveParticleCount = 0;

        // Configuraciones avanzadas
        VelocityRange VelocityRangeConfig;
        float VelocityChangeInterval = 1.0f;

        SizeRange SizeRangeConfig;
        float SizeChangeInterval = 1.0f;

        EmissionArea EmissionAreaConfig;

        std::vector<Particle> Particles;

        BillboardType ParticleBillboardType = BillboardType::WORLD_ALIGNED;

        // Serialización principal
        template <class Archive> void serialize(Archive& archive)
        {
            archive(
                cereal::make_nvp("EmitterPosition", LocalEmitterPosition),
                cereal::make_nvp("EmissionRate", EmissionRate), cereal::make_nvp("ParticleLifetime", ParticleLifetime),
                cereal::make_nvp("Gravity", Gravity), cereal::make_nvp("ParticleSize", ParticleSize),
                cereal::make_nvp("VelocityRangeConfig", VelocityRangeConfig),
                cereal::make_nvp("VelocityChangeInterval", VelocityChangeInterval),
                cereal::make_nvp("SizeRangeConfig", SizeRangeConfig),
                cereal::make_nvp("SizeChangeInterval", SizeChangeInterval),
                cereal::make_nvp("EmissionAreaConfig", EmissionAreaConfig), cereal::make_nvp("Particles", Particles));
        }

      private:
        // Métodos internos
        void EmitParticle();
        glm::vec3 GenerateRandomVelocity() const;
        float GenerateRandomSize() const;
        glm::vec3 GenerateRandomPositionInArea() const;

        // Recursos
        Ref<Material> ParticleMaterial;
        Ref<Mesh> ParticleMesh;
        Ref<Texture2D> ParticleTexture;

        float EmissionAccumulator = 0.0f;
    };

} // namespace Coffee
