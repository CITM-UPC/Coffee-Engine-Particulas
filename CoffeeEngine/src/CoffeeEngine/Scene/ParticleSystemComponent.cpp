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
    float ParticleSystemComponent::GenerateRandomSize() const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(SizeRangeConfig.Min, SizeRangeConfig.Max);
        return dis(gen);
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
                if (VelocityRangeConfig.UseRange)
                {
                    // Calculamos cuánto tiempo ha pasado desde el último cambio de velocidad
                    float timeInCurrentInterval = fmod(particle.Age, VelocityChangeInterval);

                    // Si estamos al inicio de un nuevo intervalo, generamos una nueva velocidad objetivo
                    if (timeInCurrentInterval < deltaTime)
                    {
                        particle.InitialVelocity = particle.Velocity;
                        particle.TargetVelocity = GenerateRandomVelocity();
                    }

                    // Interpolamos suavemente entre la velocidad inicial y la objetivo
                    float t = timeInCurrentInterval / VelocityChangeInterval;
                    t = glm::smoothstep(0.0f, 1.0f, t); // Suavizamos la transición
                    particle.Velocity = glm::mix(particle.InitialVelocity, particle.TargetVelocity, t);
                }
                if (SizeRangeConfig.UseRange)
                {
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
                        particle.InitialSize =
                            SizeRangeConfig.StartWithMin
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
                        t = particle.Age /
                            particle.LifeTime; // Usamos toda la vida de la partícula para la interpolación
                    }
                    t = glm::smoothstep(0.0f, 1.0f, t);
                    particle.Size = glm::mix(particle.InitialSize, particle.TargetSize, t);
                }
                particle.Velocity += Gravity * deltaTime;
                particle.Position += particle.Velocity * deltaTime;
                particle.Age += deltaTime;

                if (particle.Billboard)
                {
                    particle.Billboard->SetPosition(particle.Position);
                    particle.Billboard->SetScale(glm::vec3(particle.Size));
                }

                AliveParticleCount++; // Incrementar el contador para partículas vivas
            }
        }

        // Eliminar partículas expiradas
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
            // Configuración para instancing (si está disponible)
            // Renderizado aquí...
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
        particle.Size = SizeRangeConfig.UseRange ? GenerateRandomSize() : ParticleSize;
        particle.InitialSize = particle.Size;
        particle.TargetSize = particle.Size;
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

        particle.Billboard = Billboard::Create(BillboardType::SCREEN_ALIGNED);
        particle.Billboard->SetPosition(particle.Position);
        particle.Billboard->SetScale(glm::vec3(particle.Size));

        Particles.push_back(particle);
        COFFEE_CORE_INFO("Emitted particle");
    }

} // namespace Coffee
