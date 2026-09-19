#include "SpriteAnimationPanel.h"

#include <algorithm>
#include <cstdio>
#include <vector>
#include "Core/ResourceManager.h"
#include "Rendering/Types/Texture/Texture.h"
#include "ECS/Systems/Animation/Animation.h"
#include "IO/AnimationSerialization.h"
#include "Scenes/SceneManager.h"

void Editor::UI::SpriteAnimationPanel::Open(const std::string &key, const std::shared_ptr<Animation::SpriteAnimationSet> &asset) {
    if (!asset) {
        return;
    }
    Reset();

    m_Open = true;

    m_AssetKey = key;
    m_Set = asset;
    m_Draft = std::make_shared<Animation::SpriteAnimationSet>(*asset);

    if (asset->sheet) {
        m_Draft->sheet = std::make_shared<Rendering::SpriteSheet>(*asset->sheet);
    } else {
        m_Draft->sheet = std::make_shared<Rendering::SpriteSheet>();
    }

    if (!m_Draft->clips.empty()) {
        const auto first = std::ranges::min_element(m_Draft->clips, [](const auto &a, const auto &b) { return a.first < b.first; });
        m_SelectedClip = first->first;
        std::snprintf(m_ClipNameBuffer, sizeof(m_ClipNameBuffer), "%s", m_SelectedClip.c_str());
    }
    ResetPreview();
}
void Editor::UI::SpriteAnimationPanel::Create(const std::string &key, const std::filesystem::path &path) {
    Reset();
    m_AssetKey = key;
    m_Set.reset();

    m_Draft = std::make_shared<Animation::SpriteAnimationSet>();
    m_Draft->sheet = std::make_shared<Rendering::SpriteSheet>();
    m_Draft->path = path;
    m_Open = true;
    m_Dirty = true;
    ResetPreview();
}

void Editor::UI::SpriteAnimationPanel::OnImGuiRender() {
    m_IsHovered = false;

    if (!m_Open || !m_Draft) {
        return;
    }

    if (!ImGui::Begin("Sprite Animation Editor", &m_Open)) {
        ImGui::End();
        return;
    }

    m_IsHovered = ImGui::IsWindowHovered();

    ImGui::Text("%s%s", m_AssetKey.c_str(), m_Dirty ? " *" : "");

    if (ImGui::Button("Save")) {
        Save();
    }

    if (!m_Status.empty()) {
        ImGui::TextWrapped("%s", m_Status.c_str());
    }

    ImGui::SeparatorText("Sheet");
    DrawSheetSettings();

    ImGui::SeparatorText("Clips");
    DrawClipList();

    ImGui::SeparatorText("Selected Clip");
    DrawClipSettings();

    ImGui::SeparatorText("Preview");
    DrawPreview();

    ImGui::End();
}

void Editor::UI::SpriteAnimationPanel::DrawSheetSettings() {
    auto &resources = Core::ResourceManager::GetInstance();
    auto &sheet = *m_Draft->sheet;

    auto textures = resources.GetAll<Rendering::Texture>();

    std::ranges::sort(textures, [](const auto &a, const auto &b) { return a.first < b.first; });

    const std::string current = sheet.texture ? resources.GetKey<Rendering::Texture>(sheet.texture) : "<None>";

    bool changed = false;

    if (ImGui::BeginCombo("Texture", current.c_str())) {
        if (ImGui::Selectable("<None>", !sheet.texture)) {
            sheet.texture.reset();
            changed = true;
        }

        for (const auto &[key, texture] : textures) {
            if (!texture) {
                continue;
            }

            if (ImGui::Selectable(key.c_str(), texture == sheet.texture)) {
                sheet.texture = texture;
                changed = true;
            }
        }

        ImGui::EndCombo();
    }

    changed |= ImGui::InputInt("Columns", &sheet.columns);
    changed |= ImGui::InputInt("Rows", &sheet.rows);
    changed |= ImGui::InputInt("Column spacing (px)", &sheet.columnSpacing);
    changed |= ImGui::InputInt("Row spacing (px)", &sheet.rowSpacing);

    if (changed) {
        m_Dirty = true;
        m_Status.clear();
        ResetPreview();
    }
}


void Editor::UI::SpriteAnimationPanel::DrawClipList() {
    std::vector<std::string> names;
    names.reserve(m_Draft->clips.size());

    for (const auto &name : m_Draft->clips | std::views::keys) {
        names.push_back(name);
    }

    std::ranges::sort(names);

    if (ImGui::BeginListBox("###Clips")) {
        for (const auto &name : names) {

            if (ImGui::Selectable(name.c_str(), name == m_SelectedClip)) {
                m_SelectedClip = name;
                std::snprintf(m_ClipNameBuffer, sizeof(m_ClipNameBuffer), "%s", name.c_str());

                ResetPreview();
            }
        }
        ImGui::EndListBox();
    }
    ImGui::InputText("Clip Name", m_ClipNameBuffer, sizeof(m_ClipNameBuffer));

    const std::string name = m_ClipNameBuffer;
    if (ImGui::Button("Add Clip")) {
        if (name.empty()) {
            m_Status = "Enter clip name.";
        } else if (m_Draft->clips.contains(name)) {
            m_Status = "Name already taken.";
        } else {
            Animation::SpriteClip clip;
            clip.frames.push_back(Animation::SpriteFrame{});
            m_Draft->clips.emplace(name, std::move(clip));
            m_SelectedClip = name;

            m_Dirty = true;
            m_Status.clear();
            ResetPreview();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Rename Clip")) {
        if (!m_Draft->clips.contains(m_SelectedClip)) {
            m_Status = "Select a clip first.";
        } else if (name.empty()) {
            m_Status = "Enter clip name.";
        } else if (name != m_SelectedClip) {
            if (m_Draft->clips.contains(name)) {
                m_Status = "Clip name already exists";
            } else {
                auto node = m_Draft->clips.extract(m_SelectedClip);
                node.key() = name;
                m_Draft->clips.insert(std::move(node));
                m_SelectedClip = name;
                m_Dirty = true;
                m_Status.clear();
                ResetPreview();
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete Clip")) {
        if (m_Draft->clips.erase(m_SelectedClip) > 0) {
            m_SelectedClip.clear();
            m_ClipNameBuffer[0] = '\0';
            m_Dirty = true;
            m_Status.clear();
            ResetPreview();
        }
    }
}

void Editor::UI::SpriteAnimationPanel::DrawClipSettings() {
    const auto it = m_Draft->clips.find(m_SelectedClip);
    if (it == m_Draft->clips.end()) {
        ImGui::TextDisabled("Select a clip.");
        return;
    }

    auto &clip = it->second;
    bool changed = false;

    changed |= ImGui::Checkbox("Loop", &clip.loop);

    int rmIdx = -1;

    for (size_t i = 0; i < clip.frames.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto &frame = clip.frames[i];
        ImGui::Text("Frame %zu", i);

        changed |= ImGui::InputScalar("Sheet Index", ImGuiDataType_U32, &frame.index);
        changed |= ImGui::InputFloat("Duration (seconds)", &frame.duration, 0.01f, 0.1f);

        if (ImGui::SmallButton("Remove")) {
            rmIdx = static_cast<int>(i);
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    if (rmIdx >= 0) {
        clip.frames.erase(clip.frames.begin() + rmIdx);
        changed = true;
    }

    if (ImGui::Button("Add Frame")) {
        clip.frames.push_back(Animation::SpriteFrame{});
        changed = true;
    }

    if (changed) {
        m_Dirty = true;
        ResetPreview();
    }
}

void Editor::UI::SpriteAnimationPanel::DrawPreview() {
    const auto *clip = Animation::FindClip(m_PreviewPlayer);

    if (!clip) {
        ImGui::TextDisabled("Select a clip.");
        return;
    }

    if (!Animation::ValidateClip(*m_Draft, *clip)) {
        ImGui::TextWrapped("Preview unavailable. Check if sheet is valid.");
        return;
    }

    if (ImGui::Button("Play")) {
        Animation::Resume(m_PreviewPlayer);
    }

    ImGui::SameLine();

    if (ImGui::Button("Pause")) {
        Animation::Pause(m_PreviewPlayer);
    }

    ImGui::SameLine();

    if (ImGui::Button("Restart")) {
        Animation::Play(m_PreviewPlayer, m_SelectedClip, Animation::RestartSetting::Restart);
    }

    Animation::Advance(m_PreviewPlayer, *clip, ImGui::GetIO().DeltaTime);

    if (!Animation::ResolvePose(m_PreviewPlayer, m_PreveiwSprite)) {
        return;
    }

    const auto &sheet = *m_PreveiwSprite.sheet;
    const auto &texture = sheet.texture;

    if (texture->GetID() == 0) {
        ImGui::TextDisabled("Texture is not uploaded yet.");
        return;
    }

    const auto uv = sheet.GetFrameUV(static_cast<int>(m_PreveiwSprite.frame));

    const float frameWidth = uv.z * texture->GetWidth();
    const float frameHeight = uv.w * texture->GetHeight();

    const float availableWidth = ImGui::GetContentRegionAvail().x;

    if (availableWidth <= 0.0f) {
        return;
    }

    const float scale = std::min(std::min(availableWidth, 256.0f) / frameWidth, 256.0f / frameHeight);

    const ImVec2 size(frameWidth * scale, frameHeight * scale);
    const ImVec2 position = ImGui::GetCursorScreenPos();

    ImGui::GetWindowDrawList()->AddImage(texture->GetID(), position, ImVec2(position.x + size.x, position.y + size.y), ImVec2(uv.x, uv.y + uv.w), ImVec2(uv.x + uv.z, uv.y));

    ImGui::Dummy(size);

    ImGui::Text("Clip frame: %zu / %zu | Sheet index: %u", m_PreviewPlayer.frameIndex + 1, clip->frames.size(), m_PreveiwSprite.frame);
}

void Editor::UI::SpriteAnimationPanel::ResetPreview() {
    m_PreviewPlayer = {};
    m_PreveiwSprite = {};
    if (!m_Draft) {
        return;
    }
    m_PreviewPlayer.animations = m_Draft;
    m_PreviewPlayer.initialClip = m_SelectedClip;
    m_PreviewPlayer.autoplay = false;
    Animation::ResetToInitial(m_PreviewPlayer);
    Animation::ResolvePose(m_PreviewPlayer, m_PreveiwSprite);
}
void Editor::UI::SpriteAnimationPanel::Reset() {
    m_Set = nullptr;
    m_Draft = nullptr;
    m_AssetKey.clear();
    m_SelectedClip.clear();
    m_ClipNameBuffer[0] = '\0';
    m_Status.clear();
    m_Dirty = false;
    m_Open = true;
    ResetPreview();
}


bool Editor::UI::SpriteAnimationPanel::Save() {
    if (!m_Draft) {
        return false;
    }

    if (m_AssetKey.empty()) {
        m_Status = "Enter resource ID!";
        return false;
    }

    auto &resources = Core::ResourceManager::GetInstance();
    const auto registered = resources.Get<Animation::SpriteAnimationSet>(m_AssetKey);

    if (registered && registered != m_Set) {
        m_Status = "Resource ID already in use.";
        return false;
    }

    if (!Animation::ValidateSet(*m_Draft)) {
        m_Status = "Cannot save, set validation failed.";
        return false;
    }

    if (m_Draft->path.empty()) {
        m_Status = "No filepath, please select path before saving";
        return false;
    }

    Animation::SpriteAnimationSet saved = *m_Draft;
    saved.sheet = std::make_shared<Rendering::SpriteSheet>(*m_Draft->sheet);

    if (!IO::AnimationIO::Serialize(saved, saved.path)) {
        m_Status = "Failed to save animation.";
        return false;
    }

    if (m_Set) {
        *m_Set = std::move(saved);
    } else {
        m_Set = std::make_shared<Animation::SpriteAnimationSet>(std::move(saved));
    }

    resources.Register<Animation::SpriteAnimationSet>(m_AssetKey, m_Set);

    if (m_EngineContext && m_EngineContext->sceneManager) {
        if (auto *scene = m_EngineContext->sceneManager->GetCurrentScene()) {
            scene->MarkAsChanged();
        }
    }

    m_Dirty = false;
    m_Status = "Saved.";
    return true;
}
