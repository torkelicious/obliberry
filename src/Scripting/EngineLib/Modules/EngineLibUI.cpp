#include "Core/EngineContext.h"
#include "Core/ResourceManager.h"
#include "Core/UUID.h"
#include "IO/Loaders/SceneAssetLoader.h"
#include "ObSL/Natives.h"
#include "ObSL/Parser/ast.h"
#include "Scenes/SceneManager.h"
#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/ScriptProperties.h"
#include "Scripting/EngineLib/UICommandBuffer.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "UI/Rendering/UISystem.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIRect.h"
#include "UI/Elements/UIImage.h"
#include "UI/Text/Font.h"
#include "UI/UIElement.h"
#include <ObSL/Interpreter.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string_view>
#include <variant>
#include <vector>

namespace Scripting {

    static std::shared_ptr<UI::Font> AcquireFontForScene(const Core::EngineContext *ctx, const std::string_view id) {
        if (!ctx || !ctx->sceneManager || id.empty()) {
            return nullptr;
        }

        auto *scene = ctx->sceneManager->GetCurrentScene();
        if (!scene) {
            return nullptr;
        }

        IO::SceneAssetLoader::SceneAssetScope scope;

        if (!IO::SceneAssetLoader::Acquire(IO::SceneAssetLoader::AssetKind::Font, id, scope)) {
            return nullptr;
        }

        auto font = Core::ResourceManager::GetInstance().Get<UI::Font>(std::string(id));

        if (!font) {
            return nullptr;
        }

        scene->AddAssetScope(std::move(scope));
        return font;
    }

    static std::shared_ptr<Rendering::Texture> AcquireTextureForScene(const Core::EngineContext *ctx, const std::string_view id) {
        if (!ctx || !ctx->sceneManager || id.empty()) {
            return nullptr;
        }

        auto *scene = ctx->sceneManager->GetCurrentScene();
        if (!scene) {
            return nullptr;
        }

        IO::SceneAssetLoader::SceneAssetScope scope;

        if (!IO::SceneAssetLoader::Acquire(IO::SceneAssetLoader::AssetKind::Texture, id, scope)) {
            return nullptr;
        }

        auto texture = Core::ResourceManager::GetInstance().Get<Rendering::Texture>(std::string(id));

        if (!texture) {
            return nullptr;
        }

        scene->AddAssetScope(std::move(scope));
        return texture;
    }

    static void BuildBaseUIFields(ObSL::ObSLObject *obj, ObSL::Interpreter *interp, std::shared_ptr<const std::string> name, Core::UUID uuid, Core::EngineContext *ctx) {
        obj->fields["GetName"] = interp->gc.allocate<ObSL::NativeFunction>(0, [name](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return *name; }, "GetName");

        obj->fields["GetUUID"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return uuid.ToString(); }, "GetUUID");

        // raw field mirrors entity wrappers so methods can accept another UI object as an argument
        obj->fields["uuid"] = uuid.ToString();

        obj->fields["GetPosition"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (const auto *el = ctx->uiSystem->FindByUUID(uuid)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                arr->elements.emplace_back(static_cast<double>(el->Rect.Position.x));
                arr->elements.emplace_back(static_cast<double>(el->Rect.Position.y));
                return arr;
            }
            return std::monostate{};
        }, "GetPosition");
        obj->fields["GetSize"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (const auto *el = ctx->uiSystem->FindByUUID(uuid)) {
                auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                arr->elements.emplace_back(static_cast<double>(el->Rect.Scale.x));
                arr->elements.emplace_back(static_cast<double>(el->Rect.Scale.y));
                return arr;
            }
            return std::monostate{};
        }, "GetSize");
        obj->fields["IsVisible"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            if (const auto *el = ctx->uiSystem->FindByUUID(uuid))
                return el->HasFlag(UI::UIFlags::VISIBLE);
            return false;
        }, "IsVisible");

        obj->fields["IsEnabled"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            if (const auto *el = ctx->uiSystem->FindByUUID(uuid))
                return el->HasFlag(UI::UIFlags::ENABLED);
            return false;
        }, "IsEnabled");

        obj->fields["IsFocused"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            if (const auto *el = ctx->uiSystem->FindByUUID(uuid))
                return el->HasFlag(UI::UIFlags::FOCUSED);
            return false;
        }, "IsFocused");

        obj->fields["SetPosition"] = interp->gc.allocate<ObSL::NativeFunction>(2, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
                return std::monostate{};
            float x = static_cast<float>(std::get<double>(args[0]));
            float y = static_cast<float>(std::get<double>(args[1]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuid, x, y](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuid)) {
                        el->Rect.Position.x = x;
                        el->Rect.Position.y = y;
                    }
                });
            }
            return std::monostate{};
        }, "SetPosition");
        obj->fields["SetSize"] = interp->gc.allocate<ObSL::NativeFunction>(2, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
                return std::monostate{};
            float x = static_cast<float>(std::get<double>(args[0]));
            float y = static_cast<float>(std::get<double>(args[1]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuid, x, y](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuid)) {
                        el->Rect.Scale.x = x;
                        el->Rect.Scale.y = y;
                    }
                });
            }
            return std::monostate{};
        }, "SetSize");
        obj->fields["SetVisible"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            bool visible = false;
            if (std::holds_alternative<bool>(args[0]))
                visible = std::get<bool>(args[0]);
            else if (std::holds_alternative<double>(args[0]))
                visible = std::get<double>(args[0]) != 0.0;
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuid, visible](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuid)) {
                        if (visible)
                            el->AddFlag(UI::UIFlags::VISIBLE);
                        else
                            el->RemoveFlag(UI::UIFlags::VISIBLE);
                    }
                });
            }
            return std::monostate{};
        }, "SetVisible");
        obj->fields["Destroy"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuid](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuid)) {
                        if (el->Parent)
                            ui.RemoveChild(el->Parent, el);
                    }
                });
            }
            return std::monostate{};
        }, "Destroy");

        obj->fields["SetParent"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuid, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            std::string parentUuidText;

            if (!args.empty()) {
                if (std::holds_alternative<std::string>(args[0])) {
                    parentUuidText = std::get<std::string>(args[0]);
                } else if (std::holds_alternative<ObSL::ObSLObject *>(args[0])) {
                    // resolve the parent's uuid text now; never capture the GC-managed object
                    if (const auto *parentObj = std::get<ObSL::ObSLObject *>(args[0])) {
                        if (const auto it = parentObj->fields.find("uuid"); it != parentObj->fields.end() && std::holds_alternative<std::string>(it->second))
                            parentUuidText = std::get<std::string>(it->second);
                    }
                }
            }

            std::optional<Core::UUID> parentUuid;
            if (!parentUuidText.empty()) {
                parentUuid = Core::UUID::FromString(parentUuidText);
                if (!parentUuid) {
                    return std::monostate{};
                }
            }

            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuid, parentUuid](UI::UISystem &ui) {
                    auto *child = ui.FindByUUID(uuid);
                    if (!child)
                        return;

                    // no parent argument detaches to the root
                    UI::UIElement *parent = parentUuid ? ui.FindByUUID(*parentUuid) : ui.GetRoot();
                    if (!parent)
                        return;

                    ui.Reparent(child, parent);
                });
            }
            return std::monostate{};
        }, "SetParent");
    }

    static ObSL::ObSLObject *CreateUIButtonObject(ObSL::Interpreter *interp, const std::string &name, Core::UUID uuid, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        obj->identity = Identity::ForUI(uuid);
        obj->readonly = true;
        EngineLibFactories::GCProtectGuard guard(interp, obj);
        auto namePtr = std::make_shared<const std::string>(name);
        const Core::UUID uuidPtr = uuid;

        BuildBaseUIFields(obj, interp, namePtr, uuidPtr, ctx);

        obj->fields["GetText"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::string{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *btn = dynamic_cast<UI::UIButton *>(el))
                    return btn->GetText();
            return std::string{};
        }, "GetText");

        obj->fields["GetTextColor"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *btn = dynamic_cast<UI::UIButton *>(el)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    const auto &c = btn->GetColor();
                    arr->elements.emplace_back(static_cast<double>(c.r));
                    arr->elements.emplace_back(static_cast<double>(c.g));
                    arr->elements.emplace_back(static_cast<double>(c.b));
                    arr->elements.emplace_back(static_cast<double>(c.a));
                    return arr;
                }
            return std::monostate{};
        }, "GetTextColor");

        obj->fields["GetBackgroundColor"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *btn = dynamic_cast<UI::UIButton *>(el)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    const auto &c = btn->GetBackgroundColor();
                    arr->elements.emplace_back(static_cast<double>(c.r));
                    arr->elements.emplace_back(static_cast<double>(c.g));
                    arr->elements.emplace_back(static_cast<double>(c.b));
                    arr->elements.emplace_back(static_cast<double>(c.a));
                    return arr;
                }
            return std::monostate{};
        }, "GetBackgroundColor");

        obj->fields["WasClicked"] = interp->gc.allocate<ObSL::NativeFunction>(0, [namePtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            return ctx->uiSystem->GetButtonState(*namePtr) == UI::ButtonState::CLICKED;
        }, "WasClicked");

        obj->fields["IsHovered"] = interp->gc.allocate<ObSL::NativeFunction>(0, [namePtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            return ctx->uiSystem->GetButtonState(*namePtr) == UI::ButtonState::HOVERED;
        }, "IsHovered");

        obj->fields["IsHeld"] = interp->gc.allocate<ObSL::NativeFunction>(0, [namePtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return false;
            return ctx->uiSystem->GetButtonState(*namePtr) == UI::ButtonState::HELD;
        }, "IsHeld");

        obj->fields["SetText"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto text = std::get<std::string>(args[0]);
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, text](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                            btn->SetText(text);
                });
            }
            return std::monostate{};
        }, "SetText");

        obj->fields["SetTextColor"] = interp->gc.allocate<ObSL::NativeFunction>(4, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            float r = static_cast<float>(std::get<double>(args[0]));
            float g = static_cast<float>(std::get<double>(args[1]));
            float b = static_cast<float>(std::get<double>(args[2]));
            float a = static_cast<float>(std::get<double>(args[3]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, r, g, b, a](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                            btn->SetColor({r, g, b, a});
                });
            }
            return std::monostate{};
        }, "SetTextColor");

        obj->fields["SetBackgroundColor"] = interp->gc.allocate<ObSL::NativeFunction>(4, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            float r = static_cast<float>(std::get<double>(args[0]));
            float g = static_cast<float>(std::get<double>(args[1]));
            float b = static_cast<float>(std::get<double>(args[2]));
            float a = static_cast<float>(std::get<double>(args[3]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, r, g, b, a](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                            btn->SetBackgroundColor({r, g, b, a});
                });
            }
            return std::monostate{};
        }, "SetBackgroundColor");

        obj->fields["SetFont"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto fontname = std::get<std::string>(args[0]);
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, fontname, ctx](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr)) {
                        if (auto *btn = dynamic_cast<UI::UIButton *>(el)) {
                            if (auto font = AcquireFontForScene(ctx, fontname)) {
                                btn->SetFont(std::move(font));
                            }
                        }
                    }
                });
            }
            return std::monostate{};
        }, "SetFont");

        obj->fields["GetFont"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::string{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *btn = dynamic_cast<UI::UIButton *>(el)) {
                    if (const auto font = btn->GetFont()) {
                        return Core::ResourceManager::GetInstance().GetKey(font);
                    }
                }
            return std::string{};
        }, "GetFont");

        return obj;
    }

    static ObSL::ObSLObject *CreateUITextObject(ObSL::Interpreter *interp, const std::string &name, Core::UUID uuid, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        obj->identity = Identity::ForUI(uuid);
        obj->readonly = true;
        EngineLibFactories::GCProtectGuard guard(interp, obj);
        auto namePtr = std::make_shared<const std::string>(name);
        const Core::UUID uuidPtr = uuid;

        BuildBaseUIFields(obj, interp, namePtr, uuidPtr, ctx);

        obj->fields["GetText"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::string{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *txt = dynamic_cast<UI::UIText *>(el))
                    return txt->GetText();
            return std::string{};
        }, "GetText");

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *txt = dynamic_cast<UI::UIText *>(el)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    const auto &c = txt->GetColor();
                    arr->elements.emplace_back(static_cast<double>(c.r));
                    arr->elements.emplace_back(static_cast<double>(c.g));
                    arr->elements.emplace_back(static_cast<double>(c.b));
                    arr->elements.emplace_back(static_cast<double>(c.a));
                    return arr;
                }
            return std::monostate{};
        }, "GetColor");

        obj->fields["SetText"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto text = std::get<std::string>(args[0]);
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, text](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *txt = dynamic_cast<UI::UIText *>(el))
                            txt->SetText(text);
                });
            }
            return std::monostate{};
        }, "SetText");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(4, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            float r = static_cast<float>(std::get<double>(args[0]));
            float g = static_cast<float>(std::get<double>(args[1]));
            float b = static_cast<float>(std::get<double>(args[2]));
            float a = static_cast<float>(std::get<double>(args[3]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, r, g, b, a](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *txt = dynamic_cast<UI::UIText *>(el))
                            txt->SetColor({r, g, b, a});
                });
            }
            return std::monostate{};
        }, "SetColor");

        obj->fields["SetFont"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto fontname = std::get<std::string>(args[0]);
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, fontname, ctx](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *txt = dynamic_cast<UI::UIText *>(el)) {
                            if (const auto font = AcquireFontForScene(ctx, fontname)) {
                                txt->SetFont(font);
                            }
                        }
                });
            }
            return std::monostate{};
        }, "SetFont");

        return obj;
    }

    static ObSL::ObSLObject *CreateUIRectObject(ObSL::Interpreter *interp, const std::string &name, Core::UUID uuid, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        obj->identity = Identity::ForUI(uuid);
        obj->readonly = true;
        EngineLibFactories::GCProtectGuard guard(interp, obj);
        auto namePtr = std::make_shared<const std::string>(name);
        const Core::UUID uuidPtr = uuid;

        BuildBaseUIFields(obj, interp, namePtr, uuidPtr, ctx);

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *rect = dynamic_cast<UI::UIRect *>(el)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    const auto &c = rect->GetColor();
                    arr->elements.emplace_back(static_cast<double>(c.r));
                    arr->elements.emplace_back(static_cast<double>(c.g));
                    arr->elements.emplace_back(static_cast<double>(c.b));
                    arr->elements.emplace_back(static_cast<double>(c.a));
                    return arr;
                }
            return std::monostate{};
        }, "GetColor");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(4, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            float r = static_cast<float>(std::get<double>(args[0]));
            float g = static_cast<float>(std::get<double>(args[1]));
            float b = static_cast<float>(std::get<double>(args[2]));
            float a = static_cast<float>(std::get<double>(args[3]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, r, g, b, a](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *rect = dynamic_cast<UI::UIRect *>(el))
                            rect->SetColor({r, g, b, a});
                });
            }
            return std::monostate{};
        }, "SetColor");

        return obj;
    }

    static ObSL::ObSLObject *CreateUIImageObject(ObSL::Interpreter *interp, const std::string &name, Core::UUID uuid, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        obj->identity = Identity::ForUI(uuid);
        obj->readonly = true;
        EngineLibFactories::GCProtectGuard guard(interp, obj);
        auto namePtr = std::make_shared<const std::string>(name);
        const Core::UUID uuidPtr = uuid;

        BuildBaseUIFields(obj, interp, namePtr, uuidPtr, ctx);

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::monostate{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *img = dynamic_cast<UI::UIImage *>(el)) {
                    auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                    const auto &c = img->GetColor();
                    arr->elements.emplace_back(static_cast<double>(c.r));
                    arr->elements.emplace_back(static_cast<double>(c.g));
                    arr->elements.emplace_back(static_cast<double>(c.b));
                    arr->elements.emplace_back(static_cast<double>(c.a));
                    return arr;
                }
            return std::monostate{};
        }, "GetColor");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(4, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            float r = static_cast<float>(std::get<double>(args[0]));
            float g = static_cast<float>(std::get<double>(args[1]));
            float b = static_cast<float>(std::get<double>(args[2]));
            float a = static_cast<float>(std::get<double>(args[3]));
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, r, g, b, a](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *img = dynamic_cast<UI::UIImage *>(el))
                            img->SetColor({r, g, b, a});
                });
            }
            return std::monostate{};
        }, "SetColor");

        obj->fields["SetImage"] = interp->gc.allocate<ObSL::NativeFunction>(1, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
            if (!std::holds_alternative<std::string>(args[0]))
                return std::monostate{};
            auto texname = std::get<std::string>(args[0]);
            if (ctx->uiCmdBuf) {
                ctx->uiCmdBuf->push([uuidPtr, texname, ctx](UI::UISystem &ui) {
                    if (auto *el = ui.FindByUUID(uuidPtr))
                        if (auto *img = dynamic_cast<UI::UIImage *>(el))
                            if (auto texture = AcquireTextureForScene(ctx, texname))
                                img->SetImage(texture);
                });
            }
            return std::monostate{};
        }, "SetImage");

        obj->fields["GetImage"] = interp->gc.allocate<ObSL::NativeFunction>(0, [uuidPtr, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
            if (!ctx->uiSystem)
                return std::string{};
            if (auto *el = ctx->uiSystem->FindByUUID(uuidPtr))
                if (const auto *img = dynamic_cast<UI::UIImage *>(el))
                    if (const auto texture = img->GetImage())
                        return Core::ResourceManager::GetInstance().GetKey(texture);
            return std::string{};
        }, "GetImage");

        return obj;
    }

    static ObSL::ObSLObject *WrapExistingElement(ObSL::Interpreter *interp, UI::UIElement *el, Core::EngineContext *ctx) {
        const std::string &name = el->Name;
        const Core::UUID uuid = el->GetUUID();
        if (dynamic_cast<UI::UIButton *>(el))
            return CreateUIButtonObject(interp, name, uuid, ctx);
        if (dynamic_cast<UI::UIText *>(el))
            return CreateUITextObject(interp, name, uuid, ctx);
        if (dynamic_cast<UI::UIRect *>(el))
            return CreateUIRectObject(interp, name, uuid, ctx);
        if (dynamic_cast<UI::UIImage *>(el))
            return CreateUIImageObject(interp, name, uuid, ctx);
        return nullptr;
    }

} // namespace Scripting

void Scripting::EngineLib::register_gui_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("FindUI", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!std::holds_alternative<std::string>(args[0]) || !ctx->uiSystem)
            return std::monostate{};
        const auto name = std::get<std::string>(args[0]);
        if (auto *el = ctx->uiSystem->FindByName(name))
            return WrapExistingElement(interp, el, ctx);
        return std::monostate{};
    }, "FindUI"));

    interpreter.get_global_environment()->define("FindUIByUUID", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!std::holds_alternative<std::string>(args[0]) || !ctx->uiSystem)
            return std::monostate{};
        const auto parsed = Core::UUID::FromString(std::get<std::string>(args[0]));
        if (!parsed)
            return std::monostate{};
        if (auto *el = ctx->uiSystem->FindByUUID(*parsed))
            return WrapExistingElement(interp, el, ctx);
        return std::monostate{};
    }, "FindUIByUUID"));

    interpreter.get_global_environment()->define("CreateUIButton", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        std::string name = "NewButton";
        if (std::holds_alternative<std::string>(args[0]))
            name = std::get<std::string>(args[0]);
        const Core::UUID uuid = Core::UUIDGenerator::Generate();
        if (ctx->uiCmdBuf) {
            ctx->uiCmdBuf->push([name, uuid](UI::UISystem &ui) {
                auto btn = std::make_unique<UI::UIButton>();
                btn->Name = name;
                ui.AddChild(ui.GetRoot(), std::move(btn), uuid);
            });
        }
        return CreateUIButtonObject(interp, name, uuid, ctx);
    }, "CreateUIButton"));

    interpreter.get_global_environment()->define("CreateUIText", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        std::string name = "NewText";
        if (std::holds_alternative<std::string>(args[0]))
            name = std::get<std::string>(args[0]);
        const Core::UUID uuid = Core::UUIDGenerator::Generate();
        if (ctx->uiCmdBuf) {
            ctx->uiCmdBuf->push([name, uuid](UI::UISystem &ui) {
                auto txt = std::make_unique<UI::UIText>();
                txt->Name = name;
                ui.AddChild(ui.GetRoot(), std::move(txt), uuid);
            });
        }
        return CreateUITextObject(interp, name, uuid, ctx);
    }, "CreateUIText"));

    interpreter.get_global_environment()->define("CreateUIRect", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        std::string name = "NewRect";
        if (std::holds_alternative<std::string>(args[0]))
            name = std::get<std::string>(args[0]);
        const Core::UUID uuid = Core::UUIDGenerator::Generate();
        if (ctx->uiCmdBuf) {
            ctx->uiCmdBuf->push([name, uuid](UI::UISystem &ui) {
                auto rect = std::make_unique<UI::UIRect>();
                rect->Name = name;
                ui.AddChild(ui.GetRoot(), std::move(rect), uuid);
            });
        }
        return CreateUIRectObject(interp, name, uuid, ctx);
    }, "CreateUIRect"));

    interpreter.get_global_environment()->define("CreateUIImage", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        std::string name = "NewImage";
        if (std::holds_alternative<std::string>(args[0]))
            name = std::get<std::string>(args[0]);
        const Core::UUID uuid = Core::UUIDGenerator::Generate();
        if (ctx->uiCmdBuf) {
            ctx->uiCmdBuf->push([name, uuid](UI::UISystem &ui) {
                auto img = std::make_unique<UI::UIImage>();
                img->Name = name;
                ui.AddChild(ui.GetRoot(), std::move(img), uuid);
            });
        }
        return CreateUIImageObject(interp, name, uuid, ctx);
    }, "CreateUIImage"));


    interpreter.get_global_environment()->define("PropUI", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            return std::monostate{};
        }
        const PropertyMap *props = CurrentInitProps();
        if (!props || !ctx || !ctx->uiSystem) {
            return std::monostate{};
        }

        const auto it = props->find(std::get<std::string>(args[0]));
        if (it == props->end() || !it->second.is_string()) {
            return std::monostate{};
        }

        const auto uuid = Core::UUID::FromString(it->second.get<std::string>());
        if (!uuid) {
            return std::monostate{};
        }

        const UI::UIElement *el = ctx->uiSystem->FindByUUID(*uuid);
        if (!el) {
            return std::monostate{};
        }

        if (dynamic_cast<const UI::UIRect *>(el)) {
            return CreateUIRectObject(interp, el->Name, *uuid, ctx);
        }
        if (dynamic_cast<const UI::UIImage *>(el)) {
            return CreateUIImageObject(interp, el->Name, *uuid, ctx);
        }
        if (dynamic_cast<const UI::UIText *>(el)) {
            return CreateUITextObject(interp, el->Name, *uuid, ctx);
        }
        if (dynamic_cast<const UI::UIButton *>(el)) {
            return CreateUIButtonObject(interp, el->Name, *uuid, ctx);
        } else {
            return std::monostate{};
        }
    }, "PropUI"));

    // Destroy via name
    interpreter.get_global_environment()->define("DestroyUI", interpreter.gc.allocate<ObSL::NativeFunction>(1, [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
        if (!std::holds_alternative<std::string>(args[0]) || !ctx->uiCmdBuf)
            return std::monostate{};
        const auto name = std::get<std::string>(args[0]);
        ctx->uiCmdBuf->push([name](UI::UISystem &ui) {
            if (auto *el = ui.FindByName(name)) {
                if (el->Parent)
                    ui.RemoveChild(el->Parent, el);
            }
        });
        return std::monostate{};
    }, "DestroyUI"));
}
