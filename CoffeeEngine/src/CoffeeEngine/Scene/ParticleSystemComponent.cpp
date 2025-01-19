#include "CoffeeEngine/Scene/ParticleSystemComponent.h"
#include "CoffeeEngine/Core/Log.h"
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
        AliveParticleCount = 0; // Reiniciar el contador en cada frame

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
                AliveParticleCount++; // Incrementar el contador para partículas vivas
            }
        }

        // Eliminar partículas expiradas
        Particles.erase(
            std::remove_if(Particles.begin(), Particles.end(), [](const Particle& p) { return p.Age >= p.LifeTime; }),
            Particles.end());

        COFFEE_CORE_INFO("Alive particles: {}", AliveParticleCount);
    }


    void ParticleSystemComponent::Render()
    {
        std::vector<glm::mat4> InstanceTransforms;
        for (const auto& particle : Particles)
        {
            if (particle.Age < particle.LifeTime)
            {
                glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(particle.Size));
                InstanceTransforms.push_back(transform);
            }
        }

        if (!InstanceTransforms.empty() && ParticleMesh && ParticleMaterial)
        {
            // Configuración para instancing (si está disponible)
            // Renderizado aquí...
        }
    }

    void ParticleSystemComponent::EmitParticle()
    {
        Particle particle;
        particle.Position = EmitterPosition;
        particle.Velocity = glm::vec3(0.0f); // Velocidad inicial
        particle.Color = glm::vec4(1.0f);    // Color blanco
        particle.LifeTime = ParticleLifetime;
        particle.Age = 0.0f;
        particle.Size = ParticleSize;

        Particles.push_back(particle);
        COFFEE_CORE_INFO("Emitted particle");
    }

} // namespace Coffee
