#include "ScriptAssetAnalyzer.h"

#include <optional>
#include <set>
#include <string>
#include <unordered_map>

#include "ObSL/Parser/ast.h"
#include "nlohmann/json.hpp"

namespace IO::Package::Tools {
    namespace {
        using CatalogIndex = std::unordered_map<std::string, std::set<std::string>>;

        CatalogIndex BuildCatalogIndex(const nlohmann::json &assets) {
            CatalogIndex index;

            for (const auto &[type, entries] : assets.items()) {
                for (const auto &asset : entries) {
                    index[asset.at("id").get<std::string>()].insert(type);
                }
            }

            return index;
        }

        std::optional<std::string> TryResolveString(const ObSL::Expr *expression) {
            if (!expression) {
                return std::nullopt;
            }

            using namespace ObSL;

            switch (expression->type()) {
                case ExprType::Literal: {
                    const auto *node = static_cast<const LiteralExpr *>(expression);

                    if (const auto *text = std::get_if<std::string>(&node->value)) {
                        return *text;
                    }

                    return std::nullopt;
                }

                case ExprType::Grouping: {
                    const auto *node = static_cast<const GroupingExpr *>(expression);

                    return TryResolveString(node->expr.get());
                }

                case ExprType::Binary: {
                    const auto *node = static_cast<const BinaryExpr *>(expression);

                    if (node->oprt_type != TokenType::PLUS) {
                        return std::nullopt;
                    }

                    const auto left = TryResolveString(node->left.get());
                    const auto right = TryResolveString(node->right.get());

                    if (!left || !right) {
                        return std::nullopt;
                    }

                    return *left + *right;
                }

                default:
                    return std::nullopt;
            }
        }

        bool IsNonAssetNativeCall(const std::string &name) {
            static const std::set<std::string> names{// Entities
                    "GetEntity", "Find", "CreateEntity", "DestroyEntity",

                    "Window_GetHeight", "Window_GetWidth", "Window_SetFullscreen", "CloseWindow",

                    "get_dt", "GetRawDt", "GetFrameCount", "GetTimeScale", "SetTimeScale",

                    "Input_IsKeyDown", "Input_IsKeyPressed", "Input_IsKeyReleased", "Input_IsMouseDown", "Input_IsMousePressed", "Input_IsMouseReleased", "Input_GetMouseX", "Input_GetMouseY", "Input_GetScrollX",
                    "Input_GetScrollY", "Input_GetMouseWorldPos",

                    "Camera_GetPosition", "Camera_SetPosition", "Camera_Move", "Camera_PanScreenSpace", "Camera_GetZoom", "Camera_SetZoom", "Camera_GetAngleX", "Camera_GetAngleZ", "Camera_SetAngle",

                    "Math_WorldToHex", "GetSelectedHex", "SetSelectedHex", "SetPathToHex", "ClearSelectionOverlay", "ClearPathTarget", "Map_IsHexWalkable", "Map_GetMapEntity", "Hex_Distance", "Hex_GetNeighbors",
                    "Hex_HexToWorld", "Map_SetHexWalkable", "Map_SetTileType",

                    "StopMusic", "SetMasterVolume",

                    "GetCurrentScenePath",

                    "FindUI", "CreateUIButton", "CreateUIText", "CreateUIRect", "CreateUIImage", "DestroyUI",

                    "save_set", "save_get", "save_has", "save_remove", "save_clear", "save_new", "save_create", "save_write", "save_load", "save_delete", "save_list",

                    "contains"

            };

            return names.contains(name);
        }

        bool IsNonAssetMemberCall(const std::string &name) {
            static const std::set<std::string> names{
                    "SetName",
                    "GetName",
                    "GetComponent",
                    "HasComponent",
                    "GetComponents",
                    "GetChildren",
                    "GetParent",
                    "SetParent",
                    "GetChildCount",
                    "Find",
                    "Destroy",
                    "SetPersistent",
                    "IsPersistent",

                    "SetPosition",
                    "SetRotation",
                    "SetScale",
                    "GetPosition",
                    "GetRotation",
                    "GetScale",
                    "IsMoving",

                    "GetIsMoving",
                    "WasClicked",
                    "SetSize",
                    "SetText",

            };

            return names.contains(name);
        }

        struct LiteralCollector {
            const CatalogIndex &index;
            ScriptAssetAnalysis &result;
            const std::set<std::string> &scriptFunctions;

            void Collect(const std::string &id) {
                const auto found = index.find(id);
                if (found == index.end()) {
                    return;
                }

                for (const auto &type : found->second) {
                    auto &required = result.required;

                    if (type == "textures") {
                        required.textures.insert(id);
                    } else if (type == "shaders") {
                        required.shaders.insert(id);
                    } else if (type == "meshes") {
                        required.meshes.insert(id);
                    } else if (type == "materials") {
                        required.materials.insert(id);
                    } else if (type == "fonts") {
                        required.fonts.insert(id);
                    } else if (type == "animation_sets") {
                        required.animationSets.insert(id);
                    } else {
                        result.includeAllCatalogAssets = true;
                    }
                }
            }

            void TrackAssetArgument(const char *type, const ObSL::Expr *arg) {
                if (const auto id = TryResolveString(arg)) {
                    Collect(*id);
                } else {
                    result.dynamicAssets.insert(type);
                }
            }

            void TrackFileArgument(const std::string &callName, const ObSL::Expr *arg, std::set<std::string> &out) {
                if (!arg) {
                    return;
                }

                if (const auto path = TryResolveString(arg)) {
                    if (!path->empty()) {
                        out.insert(*path);
                    }
                } else {
                    result.unresolvedFileCalls.insert(callName);
                }
            }

            void VisitExpr(const ObSL::Expr *expression) {
                if (!expression) {
                    return;
                }

                using namespace ObSL;

                switch (expression->type()) {
                    case ExprType::Literal: {
                        const auto *node = static_cast<const LiteralExpr *>(expression);

                        if (const auto *text = std::get_if<std::string>(&node->value)) {
                            Collect(*text);
                        }
                        break;
                    }

                    case ExprType::Call: {
                        const auto *node = static_cast<const CallExpr *>(expression);

                        const Expr *callee = node->callee.get();
                        while (callee && callee->type() == ExprType::Grouping) {
                            callee = static_cast<const GroupingExpr *>(callee)->expr.get();
                        }

                        const Expr *argument = node->arguments.empty() ? nullptr : node->arguments.front().get();

                        bool recognizedCall = false;

                        if (callee && callee->type() == ExprType::Get) {
                            const auto *member = static_cast<const GetExpr *>(callee);

                            if (member->name == "SetTexture") {
                                recognizedCall = true;
                                TrackAssetArgument("textures", argument);
                            } else if (member->name == "SetFont") {
                                recognizedCall = true;
                                TrackAssetArgument("fonts", argument);
                            } else if (IsNonAssetMemberCall(member->name)) {
                                recognizedCall = true;
                            }
                        } else if (callee && callee->type() == ExprType::Variable) {
                            const auto *func = static_cast<const VariableExpr *>(callee);
                            const auto &name = func->name;

                            if (name == "PlaySound2D" || name == "PlayMusic") {
                                recognizedCall = true;
                                TrackFileArgument(name, argument, result.files);
                            } else if (name == "LoadScene") {
                                recognizedCall = true;
                                TrackFileArgument(name, argument, result.scenes);
                            } else if (name == "Instantiate") {
                                recognizedCall = true;
                                TrackFileArgument(name, argument, result.prefabs);
                            } else if (IsNonAssetNativeCall(name) || scriptFunctions.contains(name)) {
                                recognizedCall = true;
                            }
                        }

                        if (!recognizedCall) {
                            result.includeAllCatalogAssets = true;

                            if (callee && callee->type() == ExprType::Variable) {
                                result.unrecognizedCalls.insert(static_cast<const VariableExpr *>(callee)->name);
                            } else if (callee && callee->type() == ExprType::Get) {
                                result.unrecognizedCalls.insert("." + static_cast<const GetExpr *>(callee)->name);
                            } else {
                                result.unrecognizedCalls.insert("<indirect call>");
                            }
                        }

                        if (recognizedCall && callee && callee->type() == ExprType::Get) {
                            VisitExpr(static_cast<const GetExpr *>(callee)->obj.get());
                        } else if (!recognizedCall) {
                            VisitExpr(node->callee.get());
                        }

                        for (const auto &arg : node->arguments) {
                            VisitExpr(arg.get());
                        }
                        break;
                    }

                    case ExprType::Binary: {
                        const auto *node = static_cast<const BinaryExpr *>(expression);

                        if (const auto text = TryResolveString(expression)) {
                            Collect(*text);
                        }

                        VisitExpr(node->left.get());
                        VisitExpr(node->right.get());
                        break;
                    }

                    case ExprType::Logical: {
                        const auto *node = static_cast<const LogicalExpr *>(expression);

                        VisitExpr(node->left.get());
                        VisitExpr(node->right.get());
                        break;
                    }

                    case ExprType::Grouping: {
                        const auto *node = static_cast<const GroupingExpr *>(expression);

                        VisitExpr(node->expr.get());
                        break;
                    }

                    case ExprType::Unary: {
                        const auto *node = static_cast<const UnaryExpr *>(expression);

                        VisitExpr(node->right.get());
                        break;
                    }

                    case ExprType::Assignment: {
                        const auto *node = static_cast<const AssignmentExpr *>(expression);

                        VisitExpr(node->value.get());
                        break;
                    }

                    case ExprType::Array: {
                        const auto *node = static_cast<const ArrayExpr *>(expression);

                        for (const auto &element : node->elements) {
                            VisitExpr(element.get());
                        }
                        break;
                    }

                    case ExprType::Index: {
                        const auto *node = static_cast<const IndexExpr *>(expression);

                        result.includeAllCatalogAssets = true;

                        VisitExpr(node->callee.get());
                        VisitExpr(node->index.get());
                        break;
                    }

                    case ExprType::IndexAssignment: {
                        const auto *node = static_cast<const IndexAssignmentExpr *>(expression);

                        VisitExpr(node->callee.get());
                        VisitExpr(node->index.get());
                        VisitExpr(node->value.get());
                        break;
                    }

                    case ExprType::Get: {
                        const auto *node = static_cast<const GetExpr *>(expression);

                        if (node->name == "SetTexture" || node->name == "SetFont") {
                            result.includeAllCatalogAssets = true;
                        }

                        VisitExpr(node->obj.get());
                        break;
                    }

                    case ExprType::Set: {
                        const auto *node = static_cast<const SetExpr *>(expression);

                        VisitExpr(node->obj.get());
                        VisitExpr(node->value.get());
                        break;
                    }

                    case ExprType::TypeCheck: {
                        const auto *node = static_cast<const TypeCheckExpr *>(expression);

                        VisitExpr(node->left.get());
                        break;
                    }

                    case ExprType::Variable: {
                        const auto *node = static_cast<const VariableExpr *>(expression);

                        if (node->name == "PlaySound2D" || node->name == "PlayMusic" || node->name == "LoadScene" || node->name == "Instantiate") {
                            result.unresolvedFileCalls.insert(node->name);
                        }

                        break;
                    }

                    case ExprType::Update:
                        break;

                    default:
                        result.includeAllCatalogAssets = true;
                        break;
                }
            }

            void VisitStmt(const ObSL::Stmt *statement) {
                if (!statement) {
                    return;
                }

                using namespace ObSL;

                switch (statement->type()) {
                    case StmtType::Expression: {
                        const auto *node = static_cast<const ExpressionStmt *>(statement);

                        VisitExpr(node->expression.get());
                        break;
                    }

                    case StmtType::Print: {
                        const auto *node = static_cast<const PrintStmt *>(statement);

                        VisitExpr(node->expression.get());
                        break;
                    }

                    case StmtType::Println: {
                        const auto *node = static_cast<const PrintlnStmt *>(statement);

                        VisitExpr(node->expression.get());
                        break;
                    }

                    case StmtType::Var: {
                        const auto *node = static_cast<const VarStmt *>(statement);

                        VisitExpr(node->initializer.get());
                        break;
                    }

                    case StmtType::Block: {
                        const auto *node = static_cast<const BlockStmt *>(statement);

                        for (const auto &child : node->statements) {
                            VisitStmt(child.get());
                        }
                        break;
                    }

                    case StmtType::Function: {
                        const auto *node = static_cast<const FunctionStmt *>(statement);

                        for (const auto &parameter : node->params) {
                            VisitExpr(parameter.default_value.get());
                        }

                        VisitStmt(node->body.get());
                        break;
                    }

                    case StmtType::If: {
                        const auto *node = static_cast<const IfStmt *>(statement);

                        VisitExpr(node->condition.get());
                        VisitStmt(node->then_branch.get());
                        VisitStmt(node->else_branch.get());
                        break;
                    }

                    case StmtType::Switch: {
                        const auto *node = static_cast<const SwitchStmt *>(statement);

                        VisitExpr(node->condition.get());

                        for (const auto &branch : node->cases) {
                            VisitExpr(branch.match_value.get());

                            for (const auto &child : branch.statements) {
                                VisitStmt(child.get());
                            }
                        }
                        break;
                    }

                    case StmtType::While: {
                        const auto *node = static_cast<const WhileStmt *>(statement);

                        VisitExpr(node->condition.get());
                        VisitStmt(node->body.get());
                        break;
                    }

                    case StmtType::Foreach: {
                        const auto *node = static_cast<const ForeachStmt *>(statement);

                        VisitExpr(node->iterable.get());
                        VisitStmt(node->body.get());
                        break;
                    }

                    case StmtType::Return: {
                        const auto *node = static_cast<const ReturnStmt *>(statement);

                        VisitExpr(node->value.get());
                        break;
                    }

                    case StmtType::TryCatch: {
                        const auto *node = static_cast<const TryCatchStmt *>(statement);

                        VisitStmt(node->try_body.get());
                        VisitStmt(node->catch_body.get());
                        break;
                    }

                    case StmtType::Struct: {
                        const auto *node = static_cast<const StructStmt *>(statement);

                        for (const auto &field : node->fields) {
                            VisitExpr(field.default_value.get());
                        }
                        break;
                    }

                    case StmtType::Using:
                    case StmtType::Break:
                        break;

                    default:
                        result.includeAllCatalogAssets = true;
                        break;
                }
            }
        };
    } // namespace

    ScriptAssetAnalysis AnalyzeScriptAssets(const std::vector<std::unique_ptr<ObSL::Stmt>> &statements, const nlohmann::json &catalogAssets) {
        const auto index = BuildCatalogIndex(catalogAssets);
        ScriptAssetAnalysis result;

        std::set<std::string> scriptFunctions;

        for (const auto &stmt : statements) {
            if (stmt && stmt->type() == ObSL::StmtType::Function) {
                scriptFunctions.insert(static_cast<const ObSL::FunctionStmt *>(stmt.get())->name);
            }
        }

        LiteralCollector collector{index, result, scriptFunctions};

        for (const auto &statement : statements) {
            collector.VisitStmt(statement.get());
        }

        return result;
    }
} // namespace IO::Package::Tools
