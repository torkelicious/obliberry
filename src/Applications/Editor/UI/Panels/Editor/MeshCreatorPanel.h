#pragma once
#include "Applications/Editor/UI/Panels/EditorPanel.h"
#include "Core/ResourceManager.h"
#include "Rendering/Types/Mesh/Mesh.h"
#include <ImVectorEditor.h>
#include <glm/glm.hpp>
#include <vector>

namespace Editor::UI {
    class MeshCreatorPanel : public EditorPanel {
    public:
        MeshCreatorPanel();

        void OnImGuiRender() override;
        void Reset();

        [[nodiscard]] std::vector<glm::vec2> GetShapeVec2Points() const;
        [[nodiscard]] std::vector<glm::vec2> GetShapeVec2Points_CanvasSpace() const;

    private:
        void DrawToolbar();
        void DrawHint() const;
        void DrawHandleModeControls();
        void DrawPathOperationControls();
        void DrawStatusBar() const;
        void DrawMeshSection();
        void GenerateMesh();

        void InvalidateMesh();

        void ApplyViewDeltas(const ImVectorEditor::Result &res);

        ImVectorEditor::Path m_Path;
        ImVectorEditor::Editor m_PathEditor;
        ImVectorEditor::Config m_IVEConfig;

        bool m_RequestClearConfirm = false;

        float m_FlattenTolerance = 1.0f;
        bool m_MeshGenerated = false;
        bool m_MeshStale = false;
        bool m_MeshTriangFailure = false;
        int m_GeneratedVertexCount = 0;
        int m_GeneratedTriangleCount = 0;
        Rendering::MeshData m_GeneratedMeshData;
        char m_MeshNameBuffer[128] = "custom_mesh";

        static constexpr float kMinZoom = 0.1f;
        static constexpr float kMaxZoom = 10.0f;
    };
} // namespace Editor::UI
