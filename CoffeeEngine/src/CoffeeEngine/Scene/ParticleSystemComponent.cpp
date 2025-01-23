﻿// ParticleSystemComponent.cpp (Modificado)
#include "CoffeeEngine/Scene/ParticleSystemComponent.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include <glm/gtx/transform.hpp>
#include <random>

namespace Coffee
{
    ParticleSystemComponent::ParticleSystemComponent()
    {
        ParticleMaterial = Material::Create("Default Particle Material");
        ParticleMesh = ResourceRegistry::Get<Mesh>("DefaultQuadMesh");
        if (!ParticleMesh)
        {
            COFFEE_CORE_WARN("DefaultQuadMesh not found. Falling back to a generated quad.");
            ParticleMesh = PrimitiveMesh::CreateQuad();
        }
    }
    glm::vec3 ParticleSystemComponent::GenerateRandomVelocity() const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        auto randomFloat = [](float min, float max) -> float {
            std::uniform_real_distribution<float> dis(min, max);
            return dis(gen);
        };

        return glm::vec3(randomFloat(VelocityRangeConfig.Min.x, VelocityRangeConfig.Max.x),
                         randomFloat(VelocityRangeConfig.Min.y, VelocityRangeConfig.Max.y),
                         randomFloat(VelocityRangeConfig.Min.z, VelocityRangeConfig.Max.z));
    }
    float ParticleSystemComponent::GenerateRandomSize() const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(SizeRangeConfig.Min, SizeRangeConfig.Max);
        return dis(gen);
    }
    glm::vec3 ParticleSystemComponent::GenerateRandomPositionInArea() const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        auto randomFloat = [](float min, float max) -> float {
            std::uniform_real_distribution<float> dis(min, max);
            return dis(gen);
        };

        glm::vec3 randomPosition = LocalEmitterPosition;

        if (EmissionAreaConfig.UseEmissionArea)
        {
            switch (EmissionAreaConfig.AreaShape)
            {
            case EmissionArea::Shape::Box: {
                randomPosition.x += randomFloat(-EmissionAreaConfig.Size.x, EmissionAreaConfig.Size.x) * 0.5f;
                randomPosition.y += randomFloat(-EmissionAreaConfig.Size.y, EmissionAreaConfig.Size.y) * 0.5f;
                randomPosition.z += randomFloat(-EmissionAreaConfig.Size.z, EmissionAreaConfig.Size.z) * 0.5f;
                break;
            }
            case EmissionArea::Shape::Sphere: {
                float radius = glm::length(EmissionAreaConfig.Size) * 0.5f;
                float theta = randomFloat(0.0f, glm::two_pi<float>());
                float phi = randomFloat(0.0f, glm::pi<float>());
                float r = randomFloat(0.0f, radius);

                randomPosition.x += r * sin(phi) * cos(theta);
                randomPosition.y += r * sin(phi) * sin(theta);
                randomPosition.z += r * cos(phi);
                break;
            }
            case EmissionArea::Shape::Circle: {
                float radius = glm::length(glm::vec2(EmissionAreaConfig.Size.x, EmissionAreaConfig.Size.z)) * 0.5f;
                float theta = randomFloat(0.0f, glm::two_pi<float>());
                float r = randomFloat(0.0f, radius);

                randomPosition.x += r * cos(theta);
                randomPosition.z += r * sin(theta);
                break;
            }
            }
        }

        return randomPosition;
    }
    void ParticleSystemComponent::Update(float deltaTime)
    {
        AliveParticleCount = 0;

        EmissionAccumulator += EmissionRate * deltaTime;
        while (EmissionAccumulator >= 1.0f)
        {
            EmitParticle();
            EmissionAccumulator -= 1.0f;
        }

        for (auto& particle : Particles)
        {
            if (VelocityRangeConfig.UseRange)
            {
                float timeInCurrentInterval = fmod(particle.Age, VelocityChangeInterval);
                if (timeInCurrentInterval < deltaTime)
                {
                    particle.InitialVelocity = particle.Velocity;
                    particle.TargetVelocity = GenerateRandomVelocity();
                }
                float t = timeInCurrentInterval / VelocityChangeInterval;
                t = glm::smoothstep(0.0f, 1.0f, t); 
                particle.Velocity = glm::mix(particle.InitialVelocity, particle.TargetVelocity, t);
            }
            if (SizeRangeConfig.UseRange)
            {
                // Si RepeatInterval es false, usamos el tiempo total de vida en lugar de hacer módulo
                float timeInCurrentInterval;
                if (SizeRangeConfig.RepeatInterval)
                {
                    timeInCurrentInterval = fmod(particle.Age, SizeChangeInterval);
                }
                else
                {
                    timeInCurrentInterval = particle.Age;
                }

                // Solo generamos nuevo tamaño objetivo si estamos repitiendo intervalos
                if (SizeRangeConfig.RepeatInterval && timeInCurrentInterval < deltaTime)
                {
                    particle.InitialSize = particle.Size;
                    particle.TargetSize = GenerateRandomSize();
                }
                else if (!SizeRangeConfig.RepeatInterval && particle.Age < deltaTime)
                {
                    // Si no repetimos, solo establecemos los tamaños inicial y objetivo una vez
                    particle.InitialSize = SizeRangeConfig.StartWithMin
                                               ? SizeRangeConfig.Min
                                               : (SizeRangeConfig.StartWithMax ? SizeRangeConfig.Max : particle.Size);
                    particle.TargetSize =
                        SizeRangeConfig.StartWithMin
                            ? SizeRangeConfig.Max
                            : (SizeRangeConfig.StartWithMax ? SizeRangeConfig.Min : GenerateRandomSize());
                }

                float t;
                if (SizeRangeConfig.RepeatInterval)
                {
                    t = timeInCurrentInterval / SizeChangeInterval;
                }
                else
                {
                    t = particle.Age / particle.LifeTime; // Usamos toda la vida de la partícula para la interpolación
                }
                t = glm::smoothstep(0.0f, 1.0f, t);
                particle.Size = glm::mix(particle.InitialSize, particle.TargetSize, t);
            }
            if (particle.Age < particle.LifeTime)
            {
                particle.Velocity += Gravity * deltaTime;
                particle.Position += particle.Velocity * deltaTime;
                particle.Age += deltaTime;
                AliveParticleCount++;
            }
        }

        Particles.erase(
            std::remove_if(Particles.begin(), Particles.end(), [](const Particle& p) { return p.Age >= p.LifeTime; }),
            Particles.end());

        COFFEE_CORE_INFO("Alive particles: {}", AliveParticleCount);
    }

    void ParticleSystemComponent::Render()
    {
        std::vector<RenderCommand> renderCommands;

        if (ParticleTexture)
        {
            ParticleMaterial->GetMaterialTextures().albedo = ParticleTexture;
        }

        for (const auto& particle : Particles)
        {
            if (particle.Age < particle.LifeTime)
            {
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(particle.Size));

                renderCommands.push_back({
                    transform, ParticleMesh, ParticleMaterial,
                    0 // Entity ID opcional para identificación
                });
            }
        }

        for (const auto& command : renderCommands)
        {
            Renderer::Submit(command);
        }
    }


    void ParticleSystemComponent::EmitParticle()
    {
        Particle particle;
        particle.Position = LocalEmitterPosition;
        particle.Velocity = VelocityRangeConfig.UseRange ? GenerateRandomVelocity() : glm::vec3(0.0f);
        particle.InitialVelocity = particle.Velocity;
        particle.TargetVelocity = particle.Velocity;
        particle.Color = glm::vec4(1.0f);
        particle.LifeTime = ParticleLifetime;
        particle.Age = 0.0f;

        if (SizeRangeConfig.UseRange)
        {
            if (SizeRangeConfig.StartWithMin)
            {
                particle.Size = SizeRangeConfig.Min;
                particle.InitialSize = SizeRangeConfig.Min;
            }
            else if (SizeRangeConfig.StartWithMax)
            {
                particle.Size = SizeRangeConfig.Max;
                particle.InitialSize = SizeRangeConfig.Max;
            }
            else
            {
                particle.Size = GenerateRandomSize();
                particle.InitialSize = particle.Size;
            }
            particle.TargetSize = particle.Size;
        }
        else
        {
            particle.Size = ParticleSize;
            particle.InitialSize = particle.Size;
            particle.TargetSize = particle.Size;
        }
        if (EmissionAreaConfig.UseEmissionArea)
        {
            particle.Position = GenerateRandomPositionInArea();
        }

        Particles.push_back(particle);
        COFFEE_CORE_INFO("Emitted particle");
    }
} // namespace Coffee
