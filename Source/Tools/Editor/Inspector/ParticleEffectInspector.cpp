//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <Urho3D/Graphics/ParticleEffect.h>
#include <Urho3D/Graphics/ParticleEmitter.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Material.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "ParticleEffectInspector.h"
#include "Editor.h"
#include "Tabs/InspectorTab.h"
#include "ModelPreview.h"

namespace Urho3D
{

ParticleEffectInspector::ParticleEffectInspector(Context* context)
    : Object(context)
{
}

void ParticleEffectInspector::RenderInspector(InspectArgs& args)
{
    auto effect = args.object_->Cast<ParticleEffect>();
    if (effect == nullptr)
        return;

    if (ui::CollapsingHeader("Particle Effect", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto* preview = ui::GetUIState<ModelPreview>(context_);
        auto node = preview->GetNode();
        auto scene = node->GetScene();
        ParticleEmitter* emitter = node->GetComponent<ParticleEmitter>();

        if (emitter == nullptr)
        {
            emitter = node->CreateComponent<ParticleEmitter>();
            emitter->SetEffect(effect);
        }
        preview->RenderPreview();

        if (auto* modelResource = emitter->GetEffect())
        {

            bool changed = false;

            float w = ImGui::GetContentRegionAvail().x;
            ImGui::PushItemWidth(w / 3);
            if (ImGui::Button(ICON_FA_PLAY))
            {
                scene->SetUpdateEnabled(true);
                emitter->SetEmitting(true);
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_PAUSE))
            {
                scene->SetUpdateEnabled(false);
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_UNDO))
                emitter->Reset();
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_SAVE))
            {
                auto fileName = GetSubsystem<ResourceCache>()->GetResourceFileName(effect->GetName());
                effect->SaveFile(fileName);
            }

            ImGui::PopItemWidth();

            ImGui::BeginChild("##contents");

            auto constForce = effect->GetConstantForce();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

            ImGui::TextUnformatted("Material");
            auto material = effect->GetMaterial();
            ResourceRef matRef("Material");
            ResourceRef defRef = matRef;
            if (material)
                matRef.name_ = material->GetName();
            ResourceRef oldRef = matRef;

            Variant matRefVar = matRef;
            if (RenderAttribute("", matRefVar))
            {
                matRef = matRefVar.GetResourceRef();
                if (auto newMaterial = GetSubsystem<ResourceCache>()->GetResource<Material>(matRef.name_))
                    effect->SetMaterial(newMaterial);
                else
                    effect->SetMaterial(nullptr);
                changed = true;
            }

            auto numParticles = effect->GetNumParticles();
            ImGui::TextUnformatted("Particle Count");
            if (changed |= ImGui::DragInt("##particlecount", (int*)&numParticles, 1.0f, 0, 32000))
                effect->SetNumParticles(numParticles);

            int emitterFacing = effect->GetFaceCameraMode();
            ImGui::TextUnformatted("Face Camera Mode");
            if (changed |= ImGui::Combo("##facecammode", &emitterFacing, "None\0Rotate XYZ\0Rotate Y\0Look At XYZ\0Look At Y\0Look At Mixed\0Direction\0Card\0\0"))
                effect->SetFaceCameraMode((FaceCameraMode)emitterFacing);

            auto emitRateMin = effect->GetMinEmissionRate();
            auto emitRateMax = effect->GetMaxEmissionRate();
            Vector2 emitRateVec(emitRateMin, emitRateMax);
            ImGui::TextUnformatted("Emission Rate Range");
            if (changed |= ImGui::DragFloat2("##emitraterange", (float*)emitRateVec.Data(), 0.01f))
            {
                effect->SetMinEmissionRate(emitRateVec.x_);
                effect->SetMaxEmissionRate(emitRateVec.y_);
            }

            int emitterType = effect->GetEmitterType();
            ImGui::TextUnformatted("Emitter Shape");
            if (changed |= ImGui::Combo("##emittershape", &emitterType, "Sphere\0Box\0Sphere Volume\0Cylinder\0Ring\0\0"))
                effect->SetEmitterType((Urho3D::EmitterType)emitterType);

            bool relative = effect->IsRelative();
            if (ImGui::Checkbox("Relative", &relative))
                effect->SetRelative(relative);

            bool fixedSize = effect->IsFixedScreenSize();
            if (ImGui::Checkbox("Fixed Size", &fixedSize))
                effect->SetFixedScreenSize(fixedSize);

            if (ImGui::CollapsingHeader("Sizing"))
            {
                ImGui::Indent();

                auto emitterSize = effect->GetEmitterSize();
                ImGui::TextUnformatted("Emitter Size");
                if (changed |= ImGui::DragFloat3("##emittersize", (float*)emitterSize.Data()))
                    effect->SetEmitterSize(emitterSize);

                auto sizeMin = effect->GetMinParticleSize();
                ImGui::TextUnformatted("Min Particle Size");
                if (changed |= ImGui::DragFloat2("##sizemin", (float*)sizeMin.Data()))
                    effect->SetMinParticleSize(sizeMin);

                auto sizeMax = effect->GetMaxParticleSize();
                ImGui::TextUnformatted("Max Particle Size");
                if (changed |= ImGui::DragFloat2("##sizemax", (float*)sizeMax.Data()))
                    effect->SetMaxParticleSize(sizeMax);

                auto sizeAdd = effect->GetSizeAdd();
                ImGui::TextUnformatted("Size Add");
                if (changed |= ImGui::DragFloat("##sizeadd", &sizeAdd))
                    effect->SetSizeAdd(sizeAdd);

                auto sizeMul = effect->GetSizeMul();
                ImGui::TextUnformatted("Size Mul");
                if (changed |= ImGui::DragFloat("##sizemul", &sizeMul))
                    effect->SetSizeMul(sizeMul);

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Forces"))
            {
                ImGui::Indent();

                ImGui::TextUnformatted("Constant Force");
                if (changed |= ImGui::DragFloat3("##conforce", (float*)constForce.Data()))
                    effect->SetConstantForce(constForce);

                auto minDir = effect->GetMinDirection();
                ImGui::TextUnformatted("Min Direction");
                if (changed |= ImGui::DragFloat3("##mindir", (float*)minDir.Data()))
                    effect->SetMinDirection(minDir);

                auto maxDir = effect->GetMaxDirection();
                ImGui::TextUnformatted("Max Direction");
                if (changed |= ImGui::DragFloat3("##maxdir", (float*)maxDir.Data()))
                    effect->SetMaxDirection(maxDir);

                auto dampingForce = effect->GetDampingForce();
                ImGui::TextUnformatted("Damping Force");
                if (changed |= ImGui::DragFloat("##dampforce", &dampingForce))
                    effect->SetDampingForce(dampingForce);

                //VORTEX auto vortexAxis = effect->GetVortexAxis();
                //VORTEX ImGui::TextUnformatted("Vortex Axis");
                //VORTEX if (changed |= ImGui::DragFloat3("##vortax", (float*)vortexAxis.Data()))
                //VORTEX     effect->SetVortexAxis(vortexAxis);
                //VORTEX 
                //VORTEX auto vortexForce = effect_->GetVortexForce();
                //VORTEX ImGui::TextUnformatted("Vortex Force");
                //VORTEX if (changed |= ImGui::DragFloat2("##vortfor", (float*)vortexForce.Data()))
                //VORTEX     effect_->SetVortexForce(vortexForce);

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Timing"))
            {
                ImGui::Indent();

                auto activeTime = effect->GetActiveTime();
                ImGui::TextUnformatted("Active Time");
                if (changed |= ImGui::DragFloat("##activetime", &activeTime, 0.01f))
                    effect->SetActiveTime(activeTime);

                auto inactiveTime = effect->GetInactiveTime();
                ImGui::TextUnformatted("Inactive Time");
                if (changed |= ImGui::DragFloat("##inactivetime", &inactiveTime, 0.01f))
                    effect->SetInactiveTime(inactiveTime);

                auto ttlMin = effect->GetMinTimeToLive();
                auto ttlMax = effect->GetMaxTimeToLive();

                ImGui::TextUnformatted("Min Time to Live");
                if (changed |= ImGui::DragFloat("##minttl", &ttlMin, 0.01f))
                    effect->SetMinTimeToLive(ttlMin);

                ImGui::TextUnformatted("Max Time to Live");
                if (changed |= ImGui::DragFloat("##maxttl", &ttlMax, 0.01f))
                    effect->SetMaxTimeToLive(ttlMax);

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Velocity and Rotation"))
            {
                ImGui::Indent();

                auto velMin = effect->GetMinVelocity();
                auto velMax = effect->GetMaxVelocity();
                Vector2 velVec(velMin, velMax);
                ImGui::TextUnformatted("Velocity Range");
                if (changed |= ImGui::DragFloat2("##minvel", (float*)velVec.Data(), 0.01f))
                {
                    effect->SetMinVelocity(velVec.x_);
                    effect->SetMaxVelocity(velVec.y_);
                }

                auto rotMin = effect->GetMinRotation();
                auto rotMax = effect->GetMaxRotation();
                Vector2 rotVec(rotMin, rotMax);

                ImGui::TextUnformatted("Rotation Range");
                if (changed |= ImGui::DragFloat2("##rotrange", (float*)rotVec.Data(), 0.01f))
                {
                    effect->SetMinRotation(rotVec.x_);
                    effect->SetMaxRotation(rotVec.y_);
                }

                auto rotMinSpeed = effect->GetMinRotationSpeed();
                auto rotMaxSpeed = effect->GetMaxRotationSpeed();
                Vector2 rotSpeedVec(rotMinSpeed, rotMaxSpeed);

                ImGui::TextUnformatted("Rotation Speed Range");
                if (changed |= ImGui::DragFloat2("##rotspdrange", (float*)rotSpeedVec.Data(), 0.01f))
                {
                    effect->SetMinRotationSpeed(rotSpeedVec.x_);
                    effect->SetMaxRotationSpeed(rotSpeedVec.y_);
                }

                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Color Key Frames"))
            {
                ImGui::PushID("COLOR_KEYS");

                ImGui::Indent();

                auto colorFrames = effect->GetColorFrames();

                float w = ImGui::GetContentRegionAvail().x / 2 - 60;

                ImGui::PushItemWidth(w);
                for (size_t i = 0; i < colorFrames.size(); ++i)
                {
                    ImGui::PushID(i + 1);

                    float t = colorFrames[i].time_;
                    bool frameChanged = ImGui::InputFloat("##time", &t, 0.01f, 0.1f);

                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_ANGLE_UP) && i > 0 && colorFrames.size() > 1)
                    {
                        ea::swap(colorFrames[i - 1].time_, colorFrames[i].time_);
                        effect->SetColorFrame(i, colorFrames[i]);
                        effect->SetColorFrame(i - 1, colorFrames[i - 1]);
                        effect->SortColorFrames();
                        colorFrames = effect->GetColorFrames();
                        changed = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_ANGLE_DOWN) && i < colorFrames.size() - 1)
                    {
                        ea::swap(colorFrames[i + 1].time_, colorFrames[i].time_);
                        effect->SetColorFrame(i, colorFrames[i]);
                        effect->SetColorFrame(i + 1, colorFrames[i + 1]);
                        effect->SortColorFrames();
                        effect->SortColorFrames();
                        colorFrames = effect->GetColorFrames();
                        changed = true;
                    }
                    ImGui::SameLine();

                    auto col = colorFrames[i].color_;
                    frameChanged |= ImGui::ColorEdit4("##color", &col.r_, ImGuiColorEditFlags_AlphaBar);
                    ImGui::SameLine();
                    if (frameChanged)
                    {
                        changed = true;
                        effect->SetColorFrame(i, ColorFrame(col, t));
                        effect->SortColorFrames();
                    }
                    if (ImGui::Button(ICON_FA_MINUS))
                    {
                        effect->RemoveColorFrame(i);
                        changed = true;
                        --i;
                    }

                    ImGui::PopID();
                }
                if (ImGui::Button(ICON_FA_PLUS " Add Color Frame"))
                {
                    if (effect->GetNumColorFrames())
                        effect->AddColorFrame(ColorFrame(Color::WHITE, effect->GetColorFrames().back().time_ + 0.1f));
                    else
                        effect->AddColorFrame(ColorFrame(Color::WHITE, 0.0f));
                    changed = true;
                }
                ImGui::PopItemWidth();

                ImGui::PopID();
            }

            if (ImGui::CollapsingHeader("Texture Key Frames"))
            {
                ImGui::PushID("TEXTURE_KEYS");

                ImGui::Indent();

                float w = ImGui::GetContentRegionAvail().x / 2 - 30;

                auto textureKeys = effect->GetTextureFrames();

                ImGui::PushItemWidth(w);
                for (size_t i = 0; i < textureKeys.size(); ++i)
                {
                    ImGui::PushID(i + 1);

                    float t = textureKeys[i].time_;
                    auto rect = textureKeys[i].uv_;
                    bool frameChanged = ImGui::InputFloat("##time", &t, 0.01f, 0.1f);
                    ImGui::SameLine();
                    frameChanged |= ImGui::DragFloat4("##rect", &rect.min_.x_, 0.01f, 0.0f, 1.0f);
                    ImGui::SameLine();
                    if (frameChanged)
                    {
                        changed = true;
                        TextureFrame tf;
                        tf.time_ = t;
                        tf.uv_ = rect;
                        effect->SetTextureFrame(i, tf);
                        effect->SortTextureFrames();
                    }
                    if (ImGui::Button(ICON_FA_MINUS))
                    {
                        effect->RemoveTextureFrame(i);
                        changed = true;
                        --i;
                    }

                    ImGui::PopID();
                }
                if (ImGui::Button(ICON_FA_PLUS " Add Texture Frame"))
                {
                    TextureFrame tf;
                    tf.uv_ = Urho3D::Rect(0, 0, 1, 1);
                    if (effect->GetNumTextureFrames())
                        tf.time_ = effect->GetTextureFrames().back().time_ + 0.1f;
                    else
                        tf.time_ = 0.0f;
                    effect->AddTextureFrame(tf);
                    changed = true;
                }
                ImGui::PopItemWidth();

                ImGui::PopID();
            }

            if (changed)
            {
                emitter->ApplyEffect();
                //??MarkDirty();
            }

            ImGui::PopItemWidth();

            ImGui::EndChild();
        }
    }
}

}
