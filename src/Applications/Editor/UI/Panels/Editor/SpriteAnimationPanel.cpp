#include "SpriteAnimationPanel.h"

#include <algorithm>
#include <cstdio>
#include <vector>
#include <cmath>
#include <utility>
#include "Core/ResourceManager.h"
#include "Applications/Editor/UI/Panels/Editor/EditorWidgetsCombo.h"
#include "IO/AssetCatalog.h"
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

    ImGui::SetNextWindowSize(ImVec2(960, 700), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Sprite Animation Editor", &m_Open)) {
        ImGui::End();
        return;
    }

    m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    if (ImGui::Button("Save")) {
        Save();
    }

    ImGui::SameLine();
    ImGui::Text("%s%s", m_AssetKey.c_str(), m_Dirty ? " *" : "");

    if (!m_Status.empty()) {
        ImGui::TextWrapped("%s", m_Status.c_str());
    }

    ImGui::Separator();

    if (ImGui::BeginTable("AnimationLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch, 0.35f);

        ImGui::TableSetupColumn("Frames", ImGuiTableColumnFlags_WidthStretch, 0.65f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        if (ImGui::BeginChild("AnimationAssets", ImVec2(0, 0))) {
            if (ImGui::CollapsingHeader("Sheet Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawSheetSettings();
            }

            ImGui::SeparatorText("Clips");
            DrawClipList();
        }
        ImGui::EndChild();

        ImGui::TableSetColumnIndex(1);

        if (ImGui::BeginChild("AnimationFrames", ImVec2(0, 0))) {
            if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawPreview();
            }

            ImGui::SeparatorText("Selected Clip");
            DrawClipSettings();
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    ImGui::End();
}


void Editor::UI::SpriteAnimationPanel::DrawSheetSettings() {
    auto &sheet = *m_Draft->sheet;
    bool changed = TextureCombo("Texture", m_EngineContext, sheet.texture);

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

    ImGui::PushID(m_SelectedClip.c_str());

    auto &clip = it->second;
    bool changed = false;

    ImGui::TextUnformatted(m_SelectedClip.c_str());
    changed |= ImGui::Checkbox("Loop", &clip.loop);

    double totalDuration = 0.0;
    bool validDurations = true;

    for (const auto &frame : clip.frames) {
        if (!std::isfinite(frame.duration) || frame.duration <= 0.0f) {
            validDurations = false;
            break;
        }

        totalDuration += frame.duration;
    }

    if (validDurations) {
        ImGui::Text("%zu frames | %.2f seconds", clip.frames.size(), totalDuration);
    } else {
        ImGui::Text("%zu frames | Invalid duration", clip.frames.size());
    }

    const bool validSheet = m_Draft->sheet && Animation::ValidateSheet(*m_Draft->sheet);

    const int frameCount = validSheet ? m_Draft->sheet->FrameCount() : 0;

    ImGui::SeparatorText("Add Frames");

    if (validSheet) {
        ImGui::TextDisabled("Sheet indices: 0 to %d", frameCount - 1);
    } else {
        ImGui::TextWrapped("Choose a texture and valid sheet layout to generate frames.");
    }

    ImGui::InputInt("First index", &m_RangeFirst);
    ImGui::InputInt("Last index (inclusive)", &m_RangeLast);
    ImGui::InputFloat("FPS", &m_FrameFPS, 1.0f, 5.0f, "%.2f");

    const float duration = m_FrameFPS > 0.0f ? 1.0f / m_FrameFPS : 0.0f;

    const bool validFPS = std::isfinite(m_FrameFPS) && m_FrameFPS > 0.0f && std::isfinite(duration) && duration > 0.0f;

    const bool validRange = validSheet && m_RangeFirst >= 0 && m_RangeLast >= 0 && m_RangeFirst < frameCount && m_RangeLast < frameCount;

    if (!validFPS) {
        ImGui::TextDisabled("FPS must be a finite number greater than zero.");
    }

    if (validSheet && !validRange) {
        ImGui::TextDisabled("Both indices must be inside the sheet.");
    }

    ImGui::BeginDisabled(!validRange || !validFPS);

    const bool appendRange = ImGui::Button("Append Range");

    ImGui::SameLine();

    const bool replaceRange = ImGui::Button("Replace Frames");

    ImGui::EndDisabled();

    if (appendRange || replaceRange) {
        if (replaceRange) {
            clip.frames.clear();
        }

        const int step = m_RangeFirst <= m_RangeLast ? 1 : -1;

        for (int index = m_RangeFirst;; index += step) {
            Animation::SpriteFrame frame;
            frame.index = static_cast<uint32_t>(index);
            frame.duration = duration;

            clip.frames.push_back(frame);

            if (index == m_RangeLast) {
                break;
            }
        }

        changed = true;
    }

    ImGui::BeginDisabled(!validFPS || clip.frames.empty());

    if (ImGui::Button("Apply FPS to All Frames")) {
        for (auto &frame : clip.frames) {
            frame.duration = duration;
        }

        changed = true;
    }

    ImGui::EndDisabled();

    ImGui::SeparatorText("Frame List");

    int expansion = -1;

    if (ImGui::SmallButton("Expand All")) {
        expansion = 1;
    }

    ImGui::SameLine();

    if (ImGui::SmallButton("Collapse All")) {
        expansion = 0;
    }

    enum class FrameAction { None, Remove, Duplicate, MoveUp, MoveDown };

    FrameAction action = FrameAction::None;
    size_t actionIndex = 0;

    for (size_t i = 0; i < clip.frames.size(); ++i) {
        auto &frame = clip.frames[i];

        ImGui::PushID(static_cast<int>(i));

        if (expansion >= 0) {
            ImGui::SetNextItemOpen(expansion == 1, ImGuiCond_Always);
        }

        const bool expanded = ImGui::TreeNodeEx("Frame", ImGuiTreeNodeFlags_SpanAvailWidth, "Frame %zu | Sheet %u | %.3f s", i + 1, frame.index, frame.duration);

        if (expanded) {
            changed |= ImGui::InputScalar("Sheet index", ImGuiDataType_U32, &frame.index);

            changed |= ImGui::InputFloat("Duration (seconds)", &frame.duration, 0.01f, 0.1f, "%.3f");

            if (validSheet && frame.index >= static_cast<uint32_t>(frameCount)) {
                ImGui::TextDisabled("This index is outside the sheet.");
            }

            if (!std::isfinite(frame.duration) || frame.duration <= 0.0f) {
                ImGui::TextDisabled("Duration must be greater than zero.");
            }

            ImGui::BeginDisabled(i == 0);

            if (ImGui::SmallButton("Up")) {
                action = FrameAction::MoveUp;
                actionIndex = i;
            }

            ImGui::EndDisabled();
            ImGui::SameLine();

            ImGui::BeginDisabled(i + 1 == clip.frames.size());

            if (ImGui::SmallButton("Down")) {
                action = FrameAction::MoveDown;
                actionIndex = i;
            }

            ImGui::EndDisabled();
            ImGui::SameLine();

            if (ImGui::SmallButton("Duplicate")) {
                action = FrameAction::Duplicate;
                actionIndex = i;
            }

            ImGui::SameLine();

            if (ImGui::SmallButton("Remove")) {
                action = FrameAction::Remove;
                actionIndex = i;
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    switch (action) {
        case FrameAction::Remove: {
            clip.frames.erase(clip.frames.begin() + actionIndex);
            changed = true;
            break;
        }

        case FrameAction::Duplicate: {
            const auto copy = clip.frames[actionIndex];

            clip.frames.insert(clip.frames.begin() + actionIndex + 1, copy);

            changed = true;
            break;
        }

        case FrameAction::MoveUp: {
            std::swap(clip.frames[actionIndex], clip.frames[actionIndex - 1]);

            changed = true;
            break;
        }

        case FrameAction::MoveDown: {
            std::swap(clip.frames[actionIndex], clip.frames[actionIndex + 1]);

            changed = true;
            break;
        }

        case FrameAction::None: {
            break;
        }
    }

    if (clip.frames.empty()) {
        ImGui::TextDisabled("Use Append Range to add frames.");
    }

    if (changed) {
        m_Dirty = true;
        m_Status.clear();
        ResetPreview();
    }
    ImGui::PopID();
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
    if (!m_Draft)
        return false;

    if (m_AssetKey.empty()) {
        m_Status = "Enter resource ID!";
        return false;
    }

    auto &resources = Core::ResourceManager::GetInstance();

    const auto registered = resources.Get<Animation::SpriteAnimationSet>(m_AssetKey);

    const auto *catalogEntry = IO::AssetCatalog::Find("animation_sets", m_AssetKey);

    if (!m_Set && (registered || catalogEntry)) {
        m_Status = "Resource ID already in use.";
        return false;
    }

    if (!Animation::ValidateSet(*m_Draft)) {
        m_Status = "Cannot save, set validation failed.";
        return false;
    }

    if (m_Draft->path.empty()) {
        m_Status = "No filepath, please select path before saving.";
        return false;
    }

    Animation::SpriteAnimationSet saved = *m_Draft;
    saved.sheet = std::make_shared<Rendering::SpriteSheet>(*m_Draft->sheet);

    if (!IO::AnimationIO::Serialize(saved, saved.path)) {
        m_Status = "Failed to save animation.";
        return false;
    }

    if (!IO::AssetCatalog::Upsert("animation_sets", {{"id", m_AssetKey}, {"path", saved.path.generic_string()}})) {
        m_Status = "Animation was saved, but assets.json could not be updated.";
        return false;
    }

    if (m_Set) {
        *m_Set = std::move(saved);
    } else {
        m_Set = std::make_shared<Animation::SpriteAnimationSet>(std::move(saved));
    }

    // it is being used by the animation editor, so keep this one loaded
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
