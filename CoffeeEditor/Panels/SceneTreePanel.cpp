#include "SceneTreePanel.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/FileDialog.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/Camera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "CoffeeEngine/Scripting/Lua/LuaBackend.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "imgui_internal.h"
#include <IconsLucide.h>

#include <CoffeeEngine/Scripting/Script.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>

namespace Coffee {

    SceneTreePanel::SceneTreePanel(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::OnImGuiRender()
    {
        if (!m_Visible) return;

        ImGui::Begin("Scene Tree");

        //delete node and all children if supr is pressed and the node is selected
        if(ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SelectionContext)
        {
            m_Context->DestroyEntity(m_SelectionContext);
            m_SelectionContext = {};
        }

        //Button for adding entities to the scene tree
        if(ImGui::Button(ICON_LC_PLUS, {24,24}))
        {
            ImGui::OpenPopup("Add Entity...");
        }
        ShowCreateEntityMenu();
        ImGui::SameLine();

        static std::array<char, 256> searchBuffer;
        ImGui::InputTextWithHint("##searchbar", ICON_LC_SEARCH " Search by name:", searchBuffer.data(), searchBuffer.size());

        ImGui::BeginChild("entity tree", {0,0}, ImGuiChildFlags_Border);

        auto view = m_Context->m_Registry.view<entt::entity>();
        for(auto entityID: view)
        {
            Entity entity{ entityID, m_Context.get()};
            auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

            if(hierarchyComponent.m_Parent == entt::null)
            {
                DrawEntityNode(entity);
            }
        }

        ImGui::EndChild();
        
        // Entity Tree Drag and Drop functionality
        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
            {
                const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                switch(resource->GetType())
                {
                    case ResourceType::Model:
                    {
                        const Ref<Model>& model = std::static_pointer_cast<Model>(resource);
                        AddModelToTheSceneTree(m_Context.get(), model);
                        break;
                    }
                    default:
                        break;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if(ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_SelectionContext = {};
        }

        ImGui::End();

        ImGui::Begin("Inspector");
        if(m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);
        }

        ImGui::End();
    }

    void SceneTreePanel::DrawEntityNode(Entity entity)
    {
        auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ((hierarchyComponent.m_First == entt::null) ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entityNameTag.c_str());

        if(ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        //Code of Double clicking the item for changing the name (WIP)

        ImVec2 itemSize = ImGui::GetItemRectSize();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImVec2 popupPos = ImGui::GetItemRectMin();
            float indent = ImGui::GetStyle().IndentSpacing;
            ImGui::SetNextWindowPos({popupPos.x + indent, popupPos.y});
            ImGui::OpenPopup("EntityPopup");
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginPopup("EntityPopup"/*, ImGuiWindowFlags_NoBackground*/))
        {
            auto buff = entity.GetComponent<TagComponent>().Tag.c_str();
            ImGui::SetNextItemWidth(itemSize.x - ImGui::GetStyle().IndentSpacing);
            ImGui::InputText("##entity-name", (char*)buff, 128);
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_NODE", &entity, sizeof(Entity)); // Use the entity ID or a pointer as payload
            ImGui::Text("%s", entityNameTag.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
            {
                // Assuming payload is an Entity, but you need to cast and check appropriately
                Entity payloadEntity = *(const Entity*)payload->Data;
                // Process the drop, e.g., reparenting the entity in the hierarchy
                // This is where you would update the ECS or scene graph
                HierarchyComponent::Reparent(m_Context->m_Registry, (entt::entity)payloadEntity, entity); //I think is not necessary do the casting, it does it automatically;
            }
            ImGui::EndDragDropTarget();
        }

        if(opened)
        {
            if(hierarchyComponent.m_First != entt::null)
            {
                // Recursively draw all children
                Entity childEntity{ hierarchyComponent.m_First, m_Context.get()};
                while((entt::entity)childEntity != entt::null)
                {
                    DrawEntityNode(childEntity);
                    auto& childHierarchyComponent = childEntity.GetComponent<HierarchyComponent>();
                    childEntity = Entity{ childHierarchyComponent.m_Next, m_Context.get() };
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneTreePanel::DrawComponents(Entity entity)
    {
        if(entity.HasComponent<TagComponent>())
        {
            auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

            ImGui::Text(ICON_LC_TAG " Tag");
            ImGui::SameLine();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, entityNameTag.c_str());

            if(ImGui::InputText("##", buffer, sizeof(buffer)))
            {
                entityNameTag = std::string(buffer);
            }

            ImGui::Separator();
        }

        if(entity.HasComponent<TransformComponent>())
        {
            auto& transformComponent = entity.GetComponent<TransformComponent>();

            if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", glm::value_ptr(transformComponent.Position), 0.1f);

                ImGui::Text("Rotation");
                ImGui::DragFloat3("##Rotation", glm::value_ptr(transformComponent.Rotation),  0.1f);

                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", glm::value_ptr(transformComponent.Scale),  0.1f);
            }
        }

        if(entity.HasComponent<CameraComponent>())
        {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();
            SceneCamera& sceneCamera = cameraComponent.Camera;
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Camera", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Projection Type");
                if(ImGui::BeginCombo("##Projection Type", sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE ? "Perspective" : "Orthographic"))
                {
                    if(ImGui::Selectable("Perspective", sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::PERSPECTIVE);
                    }
                    if(ImGui::Selectable("Orthographic", sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::ORTHOGRAPHIC);
                    }
                    ImGui::EndCombo();
                }

                if(sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE)
                {
                    ImGui::Text("Field of View");
                    float fov = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Field of View", &fov, 0.1f, 0.0f, 180.0f))
                    {
                        sceneCamera.SetFOV(fov);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if(sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC)
                {
                    ImGui::Text("Orthographic Size");
                    float orthoSize = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Orthographic Size", &orthoSize, 0.1f))
                    {
                        sceneCamera.SetFOV(orthoSize);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<CameraComponent>();
                }
            }
        }

        if(entity.HasComponent<LightComponent>())
        {
            auto& lightComponent = entity.GetComponent<LightComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Light", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Light Type");
                ImGui::Combo("##Light Type", (int*)&lightComponent.type, "Directional\0Point\0Spot\0");

                ImGui::Text("Color");
                ImGui::ColorEdit3("##Color", glm::value_ptr(lightComponent.Color));

                ImGui::Text("Intensity");
                ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f);

                if(lightComponent.type == LightComponent::Type::PointLight || lightComponent.type == LightComponent::Type::SpotLight)
                {
                    ImGui::Text("Range");
                    ImGui::DragFloat("##Range", &lightComponent.Range, 0.1f);
                }

                if(lightComponent.type == LightComponent::Type::PointLight)
                {
                    ImGui::Text("Attenuation");
                    ImGui::DragFloat("##Attenuation", &lightComponent.Attenuation, 0.1f);
                }
                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<LightComponent>();
                }
            }
        }

        if(entity.HasComponent<MeshComponent>())
        {
            auto& meshComponent = entity.GetComponent<MeshComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Mesh", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Mesh");
                ImGui::SameLine();
                if(ImGui::Button(meshComponent.GetMesh()->GetName().c_str(), {64, 32}))
                {
                    ImGui::OpenPopup("MeshPopup");
                }
                if(ImGui::BeginPopup("MeshPopup"))
                {
                    if(ImGui::MenuItem("Quad"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateQuad();
                    }
                    if(ImGui::MenuItem("Cube"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCube();
                    }
                    if(ImGui::MenuItem("Sphere"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateSphere();
                    }
                    if(ImGui::MenuItem("Plane"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreatePlane();
                    }
                    if(ImGui::MenuItem("Cylinder"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCylinder();
                    }
                    if(ImGui::MenuItem("Cone"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCone();
                    }
                    if(ImGui::MenuItem("Torus"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateTorus();
                    }
                    if(ImGui::MenuItem("Capsule"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCapsule();
                    }
                    if(ImGui::MenuItem("Save Mesh"))
                    {
                        COFFEE_ERROR("Save Mesh not implemented yet!");
                    }
                    ImGui::EndPopup();
                }
                ImGui::Checkbox("Draw AABB", &meshComponent.drawAABB);

                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<MeshComponent>();
                }
            }
        }

        if(entity.HasComponent<MaterialComponent>())
        {
            // Move this function to another site
            auto DrawTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture)
            {
                auto& materialComponent = entity.GetComponent<MaterialComponent>();
                uint32_t textureID = texture ? texture->GetID() : 0;
                ImGui::ImageButton(label.c_str(), (ImTextureID)textureID, {64, 64});

                auto textureImageFormat = [](ImageFormat format) -> std::string {
                    switch (format)
                    {
                        case ImageFormat::R8: return "R8";
                        case ImageFormat::RGB8: return "RGB8";
                        case ImageFormat::RGBA8: return "RGBA8";
                        case ImageFormat::SRGB8: return "SRGB8";
                        case ImageFormat::SRGBA8: return "SRGBA8";
                        case ImageFormat::RGBA32F: return "RGBA32F";
                        case ImageFormat::DEPTH24STENCIL8: return "DEPTH24STENCIL8";
                    }
                };

                if (ImGui::IsItemHovered() and texture)
                {
                    ImGui::SetTooltip("Name: %s\nSize: %d x %d\nPath: %s",
                      texture->GetName().c_str(),
                      texture->GetWidth(),
                      texture->GetHeight(),
                      textureImageFormat(texture->GetImageFormat()).c_str(),
                      texture->GetPath().c_str()
                      );
                }

                if(ImGui::BeginDragDropTarget())
                {
                    if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                    {
                        const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                        if(resource->GetType() == ResourceType::Texture2D)
                        {
                            const Ref<Texture2D>& t = std::static_pointer_cast<Texture2D>(resource);
                            texture = t;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                
                ImGui::SameLine();
                if(ImGui::BeginCombo((label + "texture").c_str(), "", ImGuiComboFlags_NoPreview))
                {
                    if(ImGui::Selectable("Clear"))
                    {
                        texture = nullptr;
                    }
                    if(ImGui::Selectable("Open"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if(!path.empty())
                        {
                            Ref<Texture2D> t = Texture2D::Load(path);
                            texture = t;
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            auto DrawCustomColorEdit4 = [&](const std::string& label, glm::vec4& color, const glm::vec2& size = {100, 32})
            {
                //ImGui::ColorEdit4("##Albedo Color", glm::value_ptr(materialProperties.color), ImGuiColorEditFlags_NoInputs);
                if(ImGui::ColorButton(label.c_str(), ImVec4(color.r, color.g, color.b, color.a), NULL, {size.x, size.y}))
                {
                    ImGui::OpenPopup("AlbedoColorPopup");
                }
                if(ImGui::BeginPopup("AlbedoColorPopup"))
                {
                    ImGui::ColorPicker4((label + "Picker").c_str(), glm::value_ptr(color), ImGuiColorEditFlags_NoInputs);
                    ImGui::EndPopup();
                }
            };

            auto& materialComponent = entity.GetComponent<MaterialComponent>();
            bool isCollapsingHeaderOpen = true;
            if(ImGui::CollapsingHeader("Material", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                MaterialTextures& materialTextures = materialComponent.material->GetMaterialTextures();
                MaterialProperties& materialProperties = materialComponent.material->GetMaterialProperties();

                if(ImGui::TreeNode("Albedo"))
                {
                    ImGui::BeginChild("##Albedo Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    
                    ImGui::Text("Color");
                    DrawCustomColorEdit4("##Albedo Color", materialProperties.color);

                    ImGui::Text("Texture");
                    DrawTextureWidget("##Albedo", materialTextures.albedo);

                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Metallic"))
                {
                    ImGui::BeginChild("##Metallic Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Metallic");
                    ImGui::SliderFloat("##Metallic Slider", &materialProperties.metallic, 0.0f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Metallic", materialTextures.metallic);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Roughness"))
                {
                    ImGui::BeginChild("##Roughness Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Roughness");
                    ImGui::SliderFloat("##Roughness Slider", &materialProperties.roughness, 0.1f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Roughness", materialTextures.roughness);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Emission"))
                {
                    ImGui::BeginChild("##Emission Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    //FIXME: Emissive color variable is local and do not affect the materialProperties.emissive!!
                    glm::vec4& emissiveColor = reinterpret_cast<glm::vec4&>(materialProperties.emissive);
                    emissiveColor.a = 1.0f;
                    DrawCustomColorEdit4("Color", emissiveColor);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Emissive", materialTextures.emissive);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Normal Map"))
                {
                    ImGui::BeginChild("##Normal Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##Normal", materialTextures.normal);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Ambient Occlusion"))
                {
                    ImGui::BeginChild("##AO Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                    ImGui::Text("AO");
                    ImGui::SliderFloat("##AO Slider", &materialProperties.ao, 0.0f, 1.0f);
                    ImGui::Text("Texture");
                    DrawTextureWidget("##AO", materialTextures.ao);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            
                if(!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<MaterialComponent>();
                }
            }
        }

        if (entity.HasComponent<ScriptComponent>())
        {
            auto& scriptComponent = entity.GetComponent<ScriptComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Script", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                /*
                ImGui::Text("Script Name: ");
                ImGui::Text(scriptComponent.script.GetLanguage() == ScriptingLanguage::Lua ? "Lua" : "CSharp");

                ImGui::Text("Script Path: ");
                ImGui::Text(scriptComponent.script.GetPath().string().c_str());
                */

                // Get the exposed variables
                std::vector<LuaVariable> exposedVariables = LuaBackend::MapVariables(scriptComponent.script.GetPath().string());

                // print the exposed variables
                for (auto& variable : exposedVariables)
                {
                    auto it = LuaBackend::scriptEnvironments.find(scriptComponent.script.GetPath().string());
                    if (it == LuaBackend::scriptEnvironments.end()) {
                        COFFEE_CORE_ERROR("Script environment for {0} not found", scriptComponent.script.GetPath().string());
                        continue;
                    }

                    sol::environment& env = it->second;

                    switch (variable.type)
                    {
                    case sol::type::boolean: {
                        bool value = env[variable.name];
                        if (ImGui::Checkbox(variable.name.c_str(), &value))
                        {
                            env[variable.name] = value;
                        }
                        break;
                    }
                    case sol::type::number: {
                        float number = env[variable.name];
                        if (ImGui::InputFloat(variable.name.c_str(), &number))
                        {
                            env[variable.name] = number;
                        }
                        break;
                    }
                    case sol::type::string: {
                        std::string str = env[variable.name];
                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, str.c_str());

                        if (ImGui::InputText(variable.name.c_str(), buffer, sizeof(buffer)))
                        {
                            env[variable.name] = std::string(buffer);
                        }
                        break;
                    }
                    case sol::type::none: {
                        ImGui::SeparatorText(variable.value.c_str());
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }

        ImGui::Separator();

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        float buttonWidth = 200.0f;
        float buttonHeight = 32.0f;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float cursorPosX = (availableWidth - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(cursorPosX);

        if(ImGui::Button("Add Component", {buttonWidth, buttonHeight}))
        {
            ImGui::OpenPopup("Add Component...");
        }

        if(ImGui::BeginPopupModal("Add Component..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:",buffer, 256);

            std::string items[] = {"Tag Component",        "Transform Component",      "Mesh Component",
                                   "Material Component",   "Light Component",          "Camera Component",
                                   "Lua Script Component", "Particle System Component"};
            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Add Component"))
            {
                if(items[item_current] == "Tag Component")
                {
                    if(!entity.HasComponent<TagComponent>())
                        entity.AddComponent<TagComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Transform Component")
                {
                    if(!entity.HasComponent<TransformComponent>())
                        entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Mesh Component")
                {
                    if(!entity.HasComponent<MeshComponent>())
                        entity.AddComponent<MeshComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Material Component")
                {
                    if(!entity.HasComponent<MaterialComponent>())
                        entity.AddComponent<MaterialComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Light Component")
                {
                    if(!entity.HasComponent<LightComponent>())
                        entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Camera Component")
                {
                    if(!entity.HasComponent<CameraComponent>())
                        entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Script Component")
                {
                    if(!entity.HasComponent<ScriptComponent>())
                        //entity.AddComponent<ScriptComponent>();
                        // TODO add script component
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Particle System Component")
                {
                    if (!entity.HasComponent<ParticleSystemComponent>())
                    {
                        entity.AddComponent<ParticleSystemComponent>();
                        COFFEE_CORE_INFO("ParticleSystemComponent added to entity: {}", (uint32_t)entity);
                    }
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        if (entity.HasComponent<ParticleSystemComponent>())
        {
            auto DrawParticleTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture) {
                auto& particleSystem = entity.GetComponent<ParticleSystemComponent>();

                uint32_t textureID = texture ? texture->GetID() : 0;
                ImGui::ImageButton(label.c_str(), (ImTextureID)textureID, {64, 64});

                auto textureImageFormat = [](ImageFormat format) -> std::string {
                    switch (format)
                    {
                    case ImageFormat::R8:
                        return "R8";
                    case ImageFormat::RGB8:
                        return "RGB8";
                    case ImageFormat::RGBA8:
                        return "RGBA8";
                    case ImageFormat::SRGB8:
                        return "SRGB8";
                    case ImageFormat::SRGBA8:
                        return "SRGBA8";
                    case ImageFormat::RGBA32F:
                        return "RGBA32F";
                    case ImageFormat::DEPTH24STENCIL8:
                        return "DEPTH24STENCIL8";
                    }
                };

                if (ImGui::IsItemHovered() && texture)
                {
                    ImGui::SetTooltip("Name: %s\nSize: %d x %d\nPath: %s", texture->GetName().c_str(),
                                      texture->GetWidth(), texture->GetHeight(),
                                      textureImageFormat(texture->GetImageFormat()).c_str(),
                                      texture->GetPath().c_str());
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                    {
                        const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                        if (resource->GetType() == ResourceType::Texture2D)
                        {
                            const Ref<Texture2D>& t = std::static_pointer_cast<Texture2D>(resource);
                            texture = t;
                            particleSystem.SetParticleTexture(texture);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                if (ImGui::BeginCombo((label + "texture").c_str(), "", ImGuiComboFlags_NoPreview))
                {
                    if (ImGui::Selectable("Clear"))
                    {
                        texture = nullptr;
                        particleSystem.SetParticleTexture(nullptr); 
                    }
                    if (ImGui::Selectable("Open"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            Ref<Texture2D> t = Texture2D::Load(path);
                            texture = t;
                            particleSystem.SetParticleTexture(
                                texture); 
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            auto& particleSystem = entity.GetComponent<ParticleSystemComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Particle System", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {

                ImGui::Text("Emitter Properties");
                ImGui::DragFloat3("Emitter Position", glm::value_ptr(particleSystem.LocalEmitterPosition), 0.1f);
                ImGui::DragFloat("Emission Rate", &particleSystem.EmissionRate, 0.1f);
                ImGui::DragFloat("Particle Lifetime", &particleSystem.ParticleLifetime, 0.1f);

                ImGui::Separator();
                ImGui::Text("Modifiers");
                ImGui::DragFloat3("Gravity", glm::value_ptr(particleSystem.Gravity), 0.1f);
          /*      ImGui::Checkbox("Apply Rotation", &particleSystem.ApplyRotation);
                ImGui::DragFloat("Rotation Speed", &particleSystem.RotationSpeed, 0.1f);*/
                ImGui::Text("Particle Rotation");
                ImGui::DragFloat("Rotation", &particleSystem.ParticleRotation, 0.1f, -180.0f, 180.0f);
                ImGui::DragFloat("Particle Size", &particleSystem.ParticleSize, 0.1f, 0.1f); // Ajustar el tamaño
                ImGui::Text("Velocity Range");
                ImGui::Checkbox("Use Velocity Range", &particleSystem.VelocityRangeConfig.UseRange);
                ImGui::Text("Size Properties");
                ImGui::Checkbox("Use Size Range", &particleSystem.SizeRangeConfig.UseRange);
                ImGui::Text("Emission Area");
                ImGui::Checkbox("Use Emission Area", &particleSystem.EmissionAreaConfig.UseEmissionArea);
                ImGui::Separator();
                ImGui::Text("Live Particle Count: %zu", particleSystem.AliveParticleCount); // Mostrar el contador

                ImGui::Separator();

                ImGui::Text("Visual Properties");

                auto& material = particleSystem.GetParticleMaterial();
                if (material)
                {
                    auto& materialTextures = material->GetMaterialTextures();
                    if (materialTextures.albedo)
                    {
                        ImGui::Text("Current Texture: %s", materialTextures.albedo->GetName().c_str());
                        if (ImGui::Button("Remove Texture"))
                        {
                            particleSystem.SetParticleTexture(nullptr);
                        }
                    }
                    else
                    {
                        ImGui::Text("No texture assigned");
                    }

                   DrawParticleTextureWidget("Load Particle Texture", materialTextures.albedo);

                   if (!isCollapsingHeaderOpen)
                   {
                       entity.RemoveComponent<ParticleSystemComponent>();
                   }
                }

                if (particleSystem.VelocityRangeConfig.UseRange)
                {
                    ImGui::DragFloat3("Min Velocity", glm::value_ptr(particleSystem.VelocityRangeConfig.Min), 0.1f);
                    ImGui::DragFloat3("Max Velocity", glm::value_ptr(particleSystem.VelocityRangeConfig.Max), 0.1f);
                    ImGui::DragFloat("Change Interval", &particleSystem.VelocityChangeInterval, 0.1f, 0.1f, 10.0f);
                    ImGui::SameLine();
                    if (ImGui::Button("Reset##VelocityRange"))
                    {
                        particleSystem.VelocityRangeConfig.Min = glm::vec3(-1.0f, 0.0f, -1.0f);
                        particleSystem.VelocityRangeConfig.Max = glm::vec3(1.0f, 2.0f, 1.0f);
                        particleSystem.VelocityChangeInterval = 1.0f;
                    }
                }
                if (particleSystem.SizeRangeConfig.UseRange)
                {
                    ImGui::DragFloat("Min Size", &particleSystem.SizeRangeConfig.Min, 0.1f, 0.1f,
                                     particleSystem.SizeRangeConfig.Max);
                    ImGui::DragFloat("Max Size", &particleSystem.SizeRangeConfig.Max, 0.1f,
                                     particleSystem.SizeRangeConfig.Min, 10.0f);
                    ImGui::DragFloat("Size Change Interval", &particleSystem.SizeChangeInterval, 0.1f, 0.1f, 10.0f);
                    ImGui::Checkbox("Repeat Size Interval", &particleSystem.SizeRangeConfig.RepeatInterval);
                    ImGui::Checkbox("Start with Min Size", &particleSystem.SizeRangeConfig.StartWithMin);
                    if (particleSystem.SizeRangeConfig.StartWithMin && particleSystem.SizeRangeConfig.StartWithMax)
                    {
                        // Si se activa Min, desactivamos Max
                        particleSystem.SizeRangeConfig.StartWithMax = false;
                    }

                    ImGui::Checkbox("Start with Max Size", &particleSystem.SizeRangeConfig.StartWithMax);
                    if (particleSystem.SizeRangeConfig.StartWithMax && particleSystem.SizeRangeConfig.StartWithMin)
                    {
                        // Si se activa Max, desactivamos Min
                        particleSystem.SizeRangeConfig.StartWithMin = false;
                    }
                    ImGui::SameLine();
                   
                    if (ImGui::Button("Reset##SizeRange"))
                    {
                        particleSystem.SizeRangeConfig.Min = 0.5f;
                        particleSystem.SizeRangeConfig.Max = 2.0f;
                        particleSystem.SizeChangeInterval = 1.0f;
                    }
                }
                if (particleSystem.EmissionAreaConfig.UseEmissionArea)
                {
                    // Selector de forma
                    const char* shapes[] = {"Box", "Sphere", "Circle"};
                    int currentShape = static_cast<int>(particleSystem.EmissionAreaConfig.AreaShape);
                    if (ImGui::Combo("Area Shape", &currentShape, shapes, IM_ARRAYSIZE(shapes)))
                    {
                        particleSystem.EmissionAreaConfig.AreaShape =
                            static_cast<ParticleSystemComponent::EmissionArea::Shape>(currentShape);
                    }

                    // Control de tamaño del área
                    ImGui::Text("Area Size");
                    switch (particleSystem.EmissionAreaConfig.AreaShape)
                    {
                    case ParticleSystemComponent::EmissionArea::Shape::Box:
                        ImGui::DragFloat3("Size", glm::value_ptr(particleSystem.EmissionAreaConfig.Size), 0.1f, 0.0f,
                                          100.0f);
                        break;

                    case ParticleSystemComponent::EmissionArea::Shape::Sphere: {

                        float sphereSize = particleSystem.EmissionAreaConfig.Size.x;
                        if (ImGui::DragFloat("Radius", &sphereSize, 0.1f, 0.0f, 100.0f))
                        {
                            particleSystem.EmissionAreaConfig.Size = glm::vec3(sphereSize);
                        }
                        break;
                    }
                    case ParticleSystemComponent::EmissionArea::Shape::Circle:
                        ImGui::DragFloat2("Radius (X,Z)", glm::value_ptr(particleSystem.EmissionAreaConfig.Size), 0.1f,
                                          0.0f, 100.0f);
                        break;
                    }
                }
                if (particleSystem.GetParticleMaterial())
                {
                    ImGui::Text("Material: %s", particleSystem.GetParticleMaterial()->GetName().c_str());
                }
                else
                {
                    ImGui::Text("Material: None");
                }
                if (particleSystem.GetParticleMesh())
                {
                    ImGui::Text("Mesh: %s", particleSystem.GetParticleMesh()->GetName().c_str());
                }
                else
                {
                    ImGui::Text("Mesh: None");
                }                


                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<ParticleSystemComponent>();
                }
            }
        }


    }


    // UI functions for scenetree menus
    void SceneTreePanel::ShowCreateEntityMenu()
    {
        if (ImGui::BeginPopupModal("Add Entity..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:", buffer, 256);

            std::string items[] = {"Empty", "Camera", "Primitive", "Light"};
            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer "
                               "scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi "
                               "lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Component"))
            {
                if (items[item_current] == "Empty")
                {
                    Entity e = m_Context->CreateEntity();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Camera")
                {
                    Entity e = m_Context->CreateEntity("Camera");
                    e.AddComponent<CameraComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Primitive")
                {
                    Entity e = m_Context->CreateEntity("Primitive");
                    e.AddComponent<MeshComponent>();
                    e.AddComponent<MaterialComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Light")
                {
                    Entity e = m_Context->CreateEntity("Light");
                    e.AddComponent<LightComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }

}
