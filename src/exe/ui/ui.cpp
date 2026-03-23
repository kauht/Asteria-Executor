#include "ui.h"
#include <imgui.h>
#include "ipc/ipc_client.h"
#include "ipc/log_server.h"
#include "injector/injector.h"
#include <windows.h>

void SetupStyle() {
    static bool init = false;
    if (init) return;
    init = true;

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.32f, 0.16f, 0.48f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.44f, 0.22f, 0.64f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.30f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.88f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.12f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.12f, 0.32f, 1.00f);
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
}

void RenderUI(IpcClient& ipc, LogServer& logServer, std::string& statusText, char* codeBuffer, size_t codeBufferSize, const std::string& dllPath) {
    SetupStyle();

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 0.95f; 
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("ExecutorMain", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::Text("Status: %s", statusText.c_str());
    ImGui::Separator();

    ImGui::Text("Lua Editor:");
    ImGui::InputTextMultiline("##code", codeBuffer, codeBufferSize, ImVec2(-FLT_MIN, io.DisplaySize.y * 0.55f));

    if (ImGui::Button("Execute", ImVec2(100, 0))) {
        ipc.SendCode(codeBuffer);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Logs", ImVec2(100, 0))) {
        logServer.ClearLogs();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy Logs", ImVec2(100, 0))) {
        std::string fullLogs = "";
        auto logs = logServer.GetLogs();
        for (const auto& log : logs) {
            fullLogs += log + "\n";
        }
        ImGui::SetClipboardText(fullLogs.c_str());
    }

    ImGui::SameLine();
    float rightAlign = ImGui::GetWindowWidth() - ImGui::GetCursorPosX() - 110.0f;
    if (rightAlign > 0) ImGui::Dummy(ImVec2(rightAlign, 0));
    ImGui::SameLine();

    if (ImGui::Button("Attach", ImVec2(100, 0))) {
        DWORD pid = Injector::GetProcessIdByName("cs2.exe");
        if (pid != 0) {
            if (Injector::InjectDll(pid, dllPath)) {
                 statusText = "Injected Successfully";
            } else {
                 statusText = "Injection Failed!";
            }
        } else {
            statusText = "cs2.exe not found!";
        }
    }

    ImGui::Separator();
    ImGui::Text("Logs:");
    
    ImGui::BeginChild("ScrollingLog", ImVec2(0, -FLT_MIN), true, ImGuiWindowFlags_HorizontalScrollbar);
    auto logs = logServer.GetLogs();
    for (const auto& log : logs) {
        ImGui::TextUnformatted(log.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();

    ImGui::End();
}
