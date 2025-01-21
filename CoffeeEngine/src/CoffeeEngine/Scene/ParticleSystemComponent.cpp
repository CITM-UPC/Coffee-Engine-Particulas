#include "CoffeeEngine/Scene/ParticleSystemComponent.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Renderer/BillboardRenderer.h"
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
            if (VelocityRangeConfig.UseRange)
            {
                // Calculamos cu�nto tiempo ha pasado desde el �ltimo cambio de velocidad
                float timeInCurrentInterval = fmod(particle.Age, VelocityChangeInterval);

                // Si estamos al inicio de un nuevo intervalo, generamos una nueva velocidad objetivo
                if (timeInCurrentInterval < deltaTime)
                {
                    particle.InitialVelocity = particle.Velocity;
                    particle.TargetVelocity = GenerateRandomVelocity();
                }

                // Interpolamos suavemente entre la velocidad inicial y la objetivo
                float t = timeInCurrentInterval / VelocityChangeInterval;
                t = glm::smoothstep(0.0f, 1.0f, t); // Suavizamos la transici�n
                particle.Velocity = glm::mix(particle.InitialVelocity, particle.TargetVelocity, t);
            }

            if (particle.Age < particle.LifeTime)
            {
                particle.Velocity += Gravity * deltaTime;
                particle.Position += particle.Velocity * deltaTime;
                particle.Age += deltaTime;

                if (particle.Billboard)
                {
                    particle.Billboard->SetPosition(particle.Position);
                    particle.Billboard->SetScale(glm::vec3(particle.Size));
                }

                AliveParticleCount++; // Incrementar el contador para part�culas vivas
            }
        }

        // Eliminar part�culas expiradas
        Particles.erase(
            std::remove_if(Particles.begin(), Particles.end(), [](const Particle& p) { return p.Age >= p.LifeTime; }),
            Particles.end());

        COFFEE_CORE_INFO("Alive particles: {}", AliveParticleCount);
    }


    void ParticleSystemComponent::Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition,
                                         const glm::vec3& cameraUp)
    {
        BillboardRenderer::BeginScene(viewProjection, cameraPosition, cameraUp);

        std::vector<glm::mat4> InstanceTransforms;
        for (const auto& particle : Particles)
        {
            if (particle.Age < particle.LifeTime)
            {
                BillboardRenderer::Submit(particle.Billboard, ParticleMaterial, /* entityID */ 0);

                glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(particle.Size));
                InstanceTransforms.push_back(transform);
            }
        }

        BillboardRenderer::EndScene();

        if (!InstanceTransforms.empty() && ParticleMesh && ParticleMaterial)
        {
            // Configuraci�n para instancing (si est� disponible)
            // Renderizado aqu�...
        }
    }

    void ParticleSystemComponent::EmitParticle()
    {
        Particle particle;
        particle.Position = GlobalEmitterPosition;
        particle.Velocity = VelocityRangeConfig.UseRange ? GenerateRandomVelocity() : glm::vec3(0.0f);
        particle.InitialVelocity = particle.Velocity;
        particle.TargetVelocity = particle.Velocity;
        particle.Color = glm::vec4(1.0f);    // Color blanco
        particle.LifeTime = ParticleLifetime;
        particle.Age = 0.0f;
        particle.Size = ParticleSize;

        particle.Billboard = Billboard::Create(BillboardType::SCREEN_ALIGNED);
        particle.Billboard->SetPosition(particle.Position);
        particle.Billboard->SetScale(glm::vec3(particle.Size));

        Particles.push_back(particle);
        COFFEE_CORE_INFO("Emitted particle");
    }

} // namespace Coffee
