#include "Scripting/EngineLib/EngineLib.h"
#include "Scripting/EngineLib/UICommandBuffer.h"
#include "Scripting/EngineLib/EngineLibFactories.h"
#include "UI/Rendering/UISystem.h"
#include "UI/Elements/UIButton.h"
#include "UI/Elements/UIText.h"
#include "UI/Elements/UIRect.h"
#include "UI/Elements/UIImage.h"
#include <ObSL/Interpreter.h>

// todo:
//  finish and test this shit

namespace Scripting {

    void BuildBaseUIFields(ObSL::ObSLObject *obj, ObSL::Interpreter *interp, const std::string &name, Core::EngineContext *ctx) {
        obj->fields["GetName"] = interp->gc.allocate<ObSL::NativeFunction>(0, [name](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value { return name; }, "GetName");

        obj->fields["GetPosition"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (const auto *el = ctx->uiSystem->FindByName(name)) {
                        auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                        arr->elements.emplace_back(static_cast<double>(el->Rect.Position.x));
                        arr->elements.emplace_back(static_cast<double>(el->Rect.Position.y));
                        return arr;
                    }
                    return std::monostate{};
                },
                "GetPosition");
        obj->fields["GetSize"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (const auto *el = ctx->uiSystem->FindByName(name)) {
                        auto *arr = interp->gc.allocate<ObSL::ObSLArray>();
                        arr->elements.emplace_back(static_cast<double>(el->Rect.Scale.x));
                        arr->elements.emplace_back(static_cast<double>(el->Rect.Scale.y));
                        return arr;
                    }
                    return std::monostate{};
                },
                "GetSize");
        obj->fields["IsVisible"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    if (const auto *el = ctx->uiSystem->FindByName(name))
                        return el->HasFlag(UI::UIFlags::VISIBLE);
                    return false;
                },
                "IsVisible");

        obj->fields["IsEnabled"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    if (const auto *el = ctx->uiSystem->FindByName(name))
                        return el->HasFlag(UI::UIFlags::ENABLED);
                    return false;
                },
                "IsEnabled");

        obj->fields["IsFocused"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    if (const auto *el = ctx->uiSystem->FindByName(name))
                        return el->HasFlag(UI::UIFlags::FOCUSED);
                    return false;
                },
                "IsFocused");

        obj->fields["SetPosition"] = interp->gc.allocate<ObSL::NativeFunction>(
                2,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
                        return std::monostate{};
                    float x = static_cast<float>(std::get<double>(args[0]));
                    float y = static_cast<float>(std::get<double>(args[1]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, x, y](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name)) {
                                el->Rect.Position.x = x;
                                el->Rect.Position.y = y;
                            }
                        });
                    }
                    return std::monostate{};
                },
                "SetPosition");
        obj->fields["SetSize"] = interp->gc.allocate<ObSL::NativeFunction>(
                2,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1]))
                        return std::monostate{};
                    float x = static_cast<float>(std::get<double>(args[0]));
                    float y = static_cast<float>(std::get<double>(args[1]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, x, y](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name)) {
                                el->Rect.Scale.x = x;
                                el->Rect.Scale.y = y;
                            }
                        });
                    }
                    return std::monostate{};
                },
                "SetSize");
        obj->fields["SetVisible"] = interp->gc.allocate<ObSL::NativeFunction>(
                1,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    bool visible = false;
                    if (std::holds_alternative<bool>(args[0]))
                        visible = std::get<bool>(args[0]);
                    else if (std::holds_alternative<double>(args[0]))
                        visible = std::get<double>(args[0]) != 0.0;
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, visible](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name)) {
                                if (visible)
                                    el->AddFlag(UI::UIFlags::VISIBLE);
                                else
                                    el->RemoveFlag(UI::UIFlags::VISIBLE);
                            }
                        });
                    }
                    return std::monostate{};
                },
                "SetVisible");
    }

    ObSL::ObSLObject *CreateUIButtonObject(ObSL::Interpreter *interp, const std::string &name, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interp, obj);

        BuildBaseUIFields(obj, interp, name, ctx);

        obj->fields["GetText"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::string{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
                        if (const auto *btn = dynamic_cast<UI::UIButton *>(el))
                            return btn->GetText();
                    return std::string{};
                },
                "GetText");

        obj->fields["GetTextColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
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
                },
                "GetTextColor");

        obj->fields["GetBackgroundColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
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
                },
                "GetBackgroundColor");

        obj->fields["WasClicked"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    return ctx->uiSystem->GetButtonState(name) == UI::ButtonState::CLICKED;
                },
                "WasClicked");

        obj->fields["IsHovered"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    return ctx->uiSystem->GetButtonState(name) == UI::ButtonState::HOVERED;
                },
                "IsHovered");

        obj->fields["IsHeld"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return false;
                    return ctx->uiSystem->GetButtonState(name) == UI::ButtonState::HELD;
                },
                "IsHeld");

        obj->fields["SetText"] = interp->gc.allocate<ObSL::NativeFunction>(
                1,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    if (!std::holds_alternative<std::string>(args[0]))
                        return std::monostate{};
                    auto text = std::get<std::string>(args[0]);
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, text](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                                    btn->SetText(text);
                        });
                    }
                    return std::monostate{};
                },
                "SetText");

        obj->fields["SetTextColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                4,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    float a = static_cast<float>(std::get<double>(args[3]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, r, g, b, a](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                                    btn->SetColor({r, g, b, a});
                        });
                    }
                    return std::monostate{};
                },
                "SetTextColor");

        obj->fields["SetBackgroundColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                4,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    float a = static_cast<float>(std::get<double>(args[3]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, r, g, b, a](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *btn = dynamic_cast<UI::UIButton *>(el))
                                    btn->SetBackgroundColor({r, g, b, a});
                        });
                    }
                    return std::monostate{};
                },
                "SetBackgroundColor");

        return obj;
    }

    ObSL::ObSLObject *CreateUITextObject(ObSL::Interpreter *interp, const std::string &name, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interp, obj);

        BuildBaseUIFields(obj, interp, name, ctx);

        obj->fields["GetText"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::string{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
                        if (const auto *txt = dynamic_cast<UI::UIText *>(el))
                            return txt->GetText();
                    return std::string{};
                },
                "GetText");

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
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
                },
                "GetColor");

        obj->fields["SetText"] = interp->gc.allocate<ObSL::NativeFunction>(
                1,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    if (!std::holds_alternative<std::string>(args[0]))
                        return std::monostate{};
                    auto text = std::get<std::string>(args[0]);
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, text](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *txt = dynamic_cast<UI::UIText *>(el))
                                    txt->SetText(text);
                        });
                    }
                    return std::monostate{};
                },
                "SetText");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                4,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    float a = static_cast<float>(std::get<double>(args[3]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, r, g, b, a](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *txt = dynamic_cast<UI::UIText *>(el))
                                    txt->SetColor({r, g, b, a});
                        });
                    }
                    return std::monostate{};
                },
                "SetColor");

        return obj;
    }

    ObSL::ObSLObject *CreateUIRectObject(ObSL::Interpreter *interp, const std::string &name, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interp, obj);

        BuildBaseUIFields(obj, interp, name, ctx);

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
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
                },
                "GetColor");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                4,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    float a = static_cast<float>(std::get<double>(args[3]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, r, g, b, a](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *rect = dynamic_cast<UI::UIRect *>(el))
                                    rect->SetColor({r, g, b, a});
                        });
                    }
                    return std::monostate{};
                },
                "SetColor");

        return obj;
    }

    ObSL::ObSLObject *CreateUIImageObject(ObSL::Interpreter *interp, const std::string &name, Core::EngineContext *ctx) {
        auto *obj = interp->gc.allocate<ObSL::ObSLObject>();
        EngineLibFactories::GCProtectGuard guard(interp, obj);

        BuildBaseUIFields(obj, interp, name, ctx);

        obj->fields["GetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                0,
                [name, ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &) -> ObSL::Value {
                    if (!ctx->uiSystem)
                        return std::monostate{};
                    if (auto *el = ctx->uiSystem->FindByName(name))
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
                },
                "GetColor");

        obj->fields["SetColor"] = interp->gc.allocate<ObSL::NativeFunction>(
                4,
                [name, ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                    float r = static_cast<float>(std::get<double>(args[0]));
                    float g = static_cast<float>(std::get<double>(args[1]));
                    float b = static_cast<float>(std::get<double>(args[2]));
                    float a = static_cast<float>(std::get<double>(args[3]));
                    if (ctx->uiCmdBuf) {
                        ctx->uiCmdBuf->push([name, r, g, b, a](UI::UISystem &ui) {
                            if (auto *el = ui.FindByName(name))
                                if (auto *img = dynamic_cast<UI::UIImage *>(el))
                                    img->SetColor({r, g, b, a});
                        });
                    }
                    return std::monostate{};
                },
                "SetColor");

        return obj;
    }

    ObSL::ObSLObject *WrapExistingElement(ObSL::Interpreter *interp, UI::UIElement *el, Core::EngineContext *ctx) {
        const std::string &name = el->Name;
        if (dynamic_cast<UI::UIButton *>(el))
            return CreateUIButtonObject(interp, name, ctx);
        if (dynamic_cast<UI::UIText *>(el))
            return CreateUITextObject(interp, name, ctx);
        if (dynamic_cast<UI::UIRect *>(el))
            return CreateUIRectObject(interp, name, ctx);
        if (dynamic_cast<UI::UIImage *>(el))
            return CreateUIImageObject(interp, name, ctx);
        return nullptr;
    }

} // namespace Scripting

void Scripting::EngineLib::register_gui_modules(ObSL::Interpreter &interpreter) {
    interpreter.get_global_environment()->define("FindUI", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                   1,
                                                                   [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                       if (!std::holds_alternative<std::string>(args[0]) || !ctx->uiSystem)
                                                                           return std::monostate{};
                                                                       const auto name = std::get<std::string>(args[0]);
                                                                       if (auto *el = ctx->uiSystem->FindByName(name))
                                                                           return WrapExistingElement(interp, el, ctx);
                                                                       return std::monostate{};
                                                                   },
                                                                   "FindUI"));

    interpreter.get_global_environment()->define("CreateUIButton", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                           1,
                                                                           [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                               std::string name = "NewButton";
                                                                               if (std::holds_alternative<std::string>(args[0]))
                                                                                   name = std::get<std::string>(args[0]);
                                                                               if (ctx->uiCmdBuf) {
                                                                                   ctx->uiCmdBuf->push([name](UI::UISystem &ui) {
                                                                                       auto btn = std::make_unique<UI::UIButton>();
                                                                                       btn->Name = name;
                                                                                       ui.AddChild(ui.GetRoot(), std::move(btn));
                                                                                   });
                                                                               }
                                                                               return CreateUIButtonObject(interp, name, ctx);
                                                                           },
                                                                           "CreateUIButton"));

    interpreter.get_global_environment()->define("CreateUIText", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                         1,
                                                                         [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                             std::string name = "NewText";
                                                                             if (std::holds_alternative<std::string>(args[0]))
                                                                                 name = std::get<std::string>(args[0]);
                                                                             if (ctx->uiCmdBuf) {
                                                                                 ctx->uiCmdBuf->push([name](UI::UISystem &ui) {
                                                                                     auto txt = std::make_unique<UI::UIText>();
                                                                                     txt->Name = name;
                                                                                     ui.AddChild(ui.GetRoot(), std::move(txt));
                                                                                 });
                                                                             }
                                                                             return CreateUITextObject(interp, name, ctx);
                                                                         },
                                                                         "CreateUIText"));

    interpreter.get_global_environment()->define("CreateUIRect", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                         1,
                                                                         [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                             std::string name = "NewRect";
                                                                             if (std::holds_alternative<std::string>(args[0]))
                                                                                 name = std::get<std::string>(args[0]);
                                                                             if (ctx->uiCmdBuf) {
                                                                                 ctx->uiCmdBuf->push([name](UI::UISystem &ui) {
                                                                                     auto rect = std::make_unique<UI::UIRect>();
                                                                                     rect->Name = name;
                                                                                     ui.AddChild(ui.GetRoot(), std::move(rect));
                                                                                 });
                                                                             }
                                                                             return CreateUIRectObject(interp, name, ctx);
                                                                         },
                                                                         "CreateUIRect"));

    interpreter.get_global_environment()->define("CreateUIImage", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                          1,
                                                                          [ctx = m_ctx](ObSL::Interpreter *interp, const std::vector<ObSL::Value> &args) -> ObSL::Value {
                                                                              std::string name = "NewImage";
                                                                              if (std::holds_alternative<std::string>(args[0]))
                                                                                  name = std::get<std::string>(args[0]);
                                                                              if (ctx->uiCmdBuf) {
                                                                                  ctx->uiCmdBuf->push([name](UI::UISystem &ui) {
                                                                                      auto img = std::make_unique<UI::UIImage>();
                                                                                      img->Name = name;
                                                                                      ui.AddChild(ui.GetRoot(), std::move(img));
                                                                                  });
                                                                              }
                                                                              return CreateUIImageObject(interp, name, ctx);
                                                                          },
                                                                          "CreateUIImage"));

    // Destroy via name
    interpreter.get_global_environment()->define("DestroyUI", interpreter.gc.allocate<ObSL::NativeFunction>(
                                                                      1,
                                                                      [ctx = m_ctx](ObSL::Interpreter *, const std::vector<ObSL::Value> &args) -> ObSL::Value {
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
                                                                      },
                                                                      "DestroyUI"));
}
