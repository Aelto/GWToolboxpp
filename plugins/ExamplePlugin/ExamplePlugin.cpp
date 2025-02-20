#include "ExamplePlugin.h"

#include <GWCA/Constants/Constants.h>
#include <GWCA/Managers/MapMgr.h>
#include <GWCA/Utilities/Hook.h>

#include <Utils/GuiUtils.h>
#include "GWCA/Managers/ChatMgr.h"

namespace {
    bool redirect_slash_ee_to_eee = false;
    GW::HookEntry ChatCmd_HookEntry;
}

DLLAPI ToolboxPlugin* ToolboxPluginInstance()
{
    static ExamplePlugin instance;
    return &instance;
}

void ExamplePlugin::LoadSettings(const wchar_t* folder)
{
    ToolboxPlugin::LoadSettings(folder);
    PLUGIN_LOAD_BOOL(redirect_slash_ee_to_eee);
}

void ExamplePlugin::SaveSettings(const wchar_t* folder)
{
    PLUGIN_SAVE_BOOL(redirect_slash_ee_to_eee);
    ToolboxPlugin::SaveSettings(folder);
}

void ExamplePlugin::DrawSettings()
{
    if (!toolbox_handle) {
        return;
    }
    ImGui::Checkbox("Redirect ee to eee", &redirect_slash_ee_to_eee);
}

void EeCmd(GW::HookStatus*, const wchar_t*, const int, const LPWSTR*)
{
    if (redirect_slash_ee_to_eee) {
        GW::Chat::SendChat('/', "eee");
    }
}

void ExamplePlugin::Initialize(ImGuiContext* ctx, const ImGuiAllocFns allocator_fns, const HMODULE toolbox_dll)
{
    ToolboxPlugin::Initialize(ctx, allocator_fns, toolbox_dll);
    GW::Chat::CreateCommand(&ChatCmd_HookEntry, L"ee", EeCmd);
}

void ExamplePlugin::SignalTerminate()
{
    ToolboxPlugin::SignalTerminate();
    GW::Chat::DeleteCommand(&ChatCmd_HookEntry);
}

bool ExamplePlugin::CanTerminate() {
    return true;
}

void ExamplePlugin::Draw(IDirect3DDevice9*)
{
    if (GW::Map::GetInstanceType() != GW::Constants::InstanceType::Loading) {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    if (ImGui::Begin(Name(), GetVisiblePtr(), ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::Text("Example plugin: area loading...");
    }
    ImGui::End();
    ImGui::PopStyleColor();
}
