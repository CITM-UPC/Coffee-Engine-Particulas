// ParticleSystemComponent.cpp (Modificado)
#include "CoffeeEngine/Scene/ParticleSystemComponent.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Renderer/Renderer.h"
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
            particle.LocalRotation = ParticleRotation;
            UpdateParticleFrame(particle, deltaTime);
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
                    t = particle.Age / particle.LifeTime;
                }
                t = glm::smoothstep(0.0f, 1.0f, t);
                particle.Size = glm::mix(particle.InitialSize, particle.TargetSize, t);
            }
            if (particle.Age < particle.LifeTime)
            {
                particle.Velocity += Gravity * deltaTime;
                particle.Position += particle.Velocity * deltaTime;
                particle.Age += deltaTime;

                if (particle.Billboard)
                {
                    particle.Billboard->SetPosition(particle.Position);
                }
                //else
                //{
                //    COFFEE_CORE_ERROR("Particle has no valid Billboard assigned.");
                //}

                AliveParticleCount++;
            }
            // Color interpolation
            if (particle.UseColorInterpolation)
            {
                float t = particle.Age / particle.LifeTime;
                t = glm::smoothstep(0.0f, 1.0f, t);
                particle.Color = glm::mix(particle.InitialColor, particle.TargetColor, t);
            }

            if (particle.UseColorInterpolation)
            {
                float t = particle.Age / particle.LifeTime;

                if (particle.RepeatColorGradient)
                {
                    t = fmod(t, 1.0f);
                }

                t = glm::smoothstep(0.0f, 1.0f, t);
                particle.Color = glm::mix(particle.InitialColor, particle.TargetColor, t);
            }

            if (particle.UseAlphaFade)
            {
                float t = particle.Age / particle.LifeTime;

                if (particle.RepeatAlphaFade)
                {
                    t = fmod(t, 1.0f);
                }

                t = glm::smoothstep(0.0f, 1.0f, t);
                particle.Color.a = glm::mix(particle.InitialColor.a, particle.TargetColor.a, t);
            }
        }

        Particles.erase(
            std::remove_if(Particles.begin(), Particles.end(), [](const Particle& p) { return p.Age >= p.LifeTime; }),
            Particles.end());

        //COFFEE_CORE_INFO("Alive particles: {}", AliveParticleCount);
    }

void ParticleSystemComponent::Render(const glm::vec3& cameraPosition, const glm::vec3& cameraUp)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::vector<RenderCommand> renderCommands;

        std::sort(renderCommands.begin(), renderCommands.end(), [&](const RenderCommand& a, const RenderCommand& b) {
            float distA = glm::length(a.transform[3] - glm::vec4(cameraPosition, 1.0f));
            float distB = glm::length(b.transform[3] - glm::vec4(cameraPosition, 1.0f));
            return distA > distB;
        });

        if (ParticleTexture)
        {
            ParticleMaterial->GetMaterialTextures().albedo = ParticleTexture;
        }

        for (const auto& particle : Particles)
        {
            if (particle.Age < particle.LifeTime && particle.Billboard)
            {
                glm::mat4 transform = particle.Billboard->CalculateTransform(cameraPosition, cameraUp);
                transform = glm::rotate(transform, particle.LocalRotation, glm::vec3(0, 0, 1));
                particle.Billboard->SetScale(glm::vec3(particle.Size));
                particle.Billboard->SetColor(particle.Color);
                renderCommands.push_back({
                    transform,        // Transformación del Billboard
                    ParticleMesh,     // Malla de la partícula
                    ParticleMaterial, // Material de la partícula
                    0                 // Entity ID (opcional)
                });
            }
            else if (!particle.Billboard)
            {
                COFFEE_CORE_ERROR("Particle has no valid Billboard during render.");
            }
        }

        // Enviar comandos al renderer
        for (const auto& command : renderCommands)
        {
            Renderer::Submit(command);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    }


    void ParticleSystemComponent::SetSpritesheet(const Ref<Texture2D>& spritesheet, int columns, int rows)
    {
        ParticleTexture = spritesheet;
        if (ParticleMaterial)
        {
            ParticleMaterial->GetMaterialTextures().albedo = spritesheet;
        }

        // Configure particle frames
        for (auto& particle : Particles)
        {
            particle.TotalFrames = columns * rows;
        }
    }
    void ParticleSystemComponent::SetParticleColorTransition(const glm::vec4& startColor, const glm::vec4& endColor)
    {
        for (auto& particle : Particles)
        {
            particle.InitialColor = startColor;
            particle.TargetColor = endColor;
            particle.UseColorInterpolation = true;
        }
    }
    void ParticleSystemComponent::SetParticleAlphaFade(float startAlpha, float endAlpha)
    {
        for (auto& particle : Particles)
        {
            particle.Color.a = startAlpha;
            particle.InitialColor.a = startAlpha;
            particle.TargetColor.a = endAlpha;
            particle.UseAlphaFade = true;
        }
    }
    void ParticleSystemComponent::UpdateParticleFrame(Particle& particle, float deltaTime)
    {
        particle.FrameTime += deltaTime;

        if (particle.FrameTime >= particle.FrameInterval)
        {
            particle.CurrentFrame = (particle.CurrentFrame + 1) % particle.TotalFrames;
            particle.FrameTime = 0.0f;
        }
    }
    void ParticleSystemComponent::EmitParticle()
    {
        Particle particle;
        particle.Position = GlobalEmitterPosition;
        particle.Velocity = VelocityRangeConfig.UseRange ? GenerateRandomVelocity() : glm::vec3(0.0f);
        particle.InitialVelocity = particle.Velocity;
        particle.TargetVelocity = particle.Velocity;
        particle.Color = particle.ColorConfig;
        particle.LifeTime = ParticleLifetime;
        particle.Age = 0.0f;
        particle.UseColorInterpolation = particle.UseColorInterpolation;
        particle.RepeatColorGradient = particle.RepeatColorGradient;
        particle.InitialColor = particle.InitialColor;
        particle.TargetColor = particle.TargetColor;

        particle.UseAlphaFade = particle.UseAlphaFade;
        particle.RepeatAlphaFade = particle.RepeatAlphaFade;
        particle.Color.a = particle.InitialAlpha;
        particle.InitialColor.a = particle.InitialAlpha;
        particle.TargetColor.a = particle.EndAlpha;
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

        // Crear y asignar el Billboard
        particle.Billboard = Billboard::Create(ParticleBillboardType);
        if (!particle.Billboard)
        {
            COFFEE_CORE_ERROR("Failed to create Billboard for particle");
            return;
        }

        particle.Billboard->SetPosition(particle.Position);
        particle.Billboard->SetScale(glm::vec3(particle.Size));
        particle.Billboard->SetMaterial(ParticleMaterial);
        particle.Billboard->SetColor(particle.Color); // Asegúrate de que el color se esté configurando aquí

        Particles.push_back(particle);
        // COFFEE_CORE_INFO("Emitted particle");
    }

    void ParticleSystemComponent::SetParticleColorGradient(const glm::vec4& startColor, const glm::vec4& endColor,  bool repeatGradient = false)
    {
        for (auto& particle : Particles)
        {
            particle.InitialColor = startColor;
            particle.TargetColor = endColor;
            particle.UseColorInterpolation = true;
            particle.RepeatColorGradient = repeatGradient;
        }
    }

    void ParticleSystemComponent::SetParticleAlphaFade(float startAlpha, float endAlpha, bool repeatFade = false)
    {
        for (auto& particle : Particles)
        {
            particle.Color.a = startAlpha;
            particle.InitialColor.a = startAlpha;
            particle.TargetColor.a = endAlpha;
            particle.UseAlphaFade = true;
            particle.RepeatAlphaFade = repeatFade;
        }
    }
} // namespace Coffee
