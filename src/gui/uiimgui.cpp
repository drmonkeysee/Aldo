//
//  uiimgui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/6/22.
//

#include "uiimgui.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

bool aldo::imgui_init(SDL_Window* win, SDL_Renderer* ren) noexcept
{
    if (!IMGUI_CHECKVERSION()) return false;

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer_Init(ren);

    ImGui::StyleColorsDark();

    return true;
}

void aldo::imgui_handle_input(SDL_Event* ev) noexcept
{
    ImGui_ImplSDL2_ProcessEvent(ev);
}

void aldo::imgui_render(bool* show_demo) noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Support")) {
            ImGui::MenuItem("ImGui Demo", nullptr, show_demo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (*show_demo) {
        ImGui::ShowDemoWindow();
    }
    ImGui::Begin("First Window");
    ImGui::Text("Hello From Aldo+Dear ImGui");
    ImGui::End();

    ImGui::Render();
}

void aldo::imgui_commit_render() noexcept
{
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
}

void aldo::imgui_cleanup() noexcept
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}
