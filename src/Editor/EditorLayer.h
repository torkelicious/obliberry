#pragma once
#include <memory>

#include "EditorCamera.h"
#include "Core/ApplicationLayer.h"
#include "Map/HexCoords.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"


class EditorLayer : public ApplicationLayer {
public:
    void Init(EngineContext &ctx) override;

    void Update(float dt) override;

    void Render() override;

    void Shutdown() override;

private:
    void DrawInterface();

    void HandleInput(float dt); // just call from update so i dont make update a mess

    void LoadScene(const std::string &path);

    void SaveScene();

    EngineContext m_Context;
    Scene *m_Scene = nullptr;
    std::string m_CurrentScenePath;
    Camera* m_Camera;
    InputManager* m_Input = nullptr;
    Registry *m_Registry = nullptr;
    // switch out later
    SceneManager m_SceneManager;


    bool m_Playing = false; // run scripts systems etc play mode

    // could use the mapruntimecomponent for this but i dont want to make editor dependent too much on ecs components
    HexCoords m_SelectedTile;
};
