// ParticleSystemComponent.cpp (Modificado)
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
        particle.Velocity = glm::vec3(0.0f);
        particle.Color = glm::vec4(1.0f);
        particle.LifeTime = ParticleLifetime;
        particle.Age = 0.0f;
        particle.Size = ParticleSize;

        Particles.push_back(particle);
        COFFEE_CORE_INFO("Emitted particle");
    }
} // namespace Coffee
