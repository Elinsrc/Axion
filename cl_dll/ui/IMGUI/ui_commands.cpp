#include "ui_commands.h"
#include "imgui.h"
#include "imgui_internal.h" 
#include "hud.h"
#include "keydefs.h"
#include "cl_util.h"
#include "imgui_viewport.h"

#include "icons.h"

ImGuiImage m_pAppIcon;
ImGuiImage m_pCLSettingsIcon;
ImGuiImage m_pCloseIcon;
ImGuiImage m_pCrosshairIcon;
ImGuiImage m_pDrawIcon;
ImGuiImage m_pSettingsIcon;

bool CImGuiCommands::m_ShowCommands = false;

void CImGuiCommands::Initialize()
{
    gEngfuncs.pfnAddCommand("ui_commands", CImGuiCommands::CmdShowCommandsWindow);
}

void CImGuiCommands::VidInitialize()
{
    m_pAppIcon = m_CImguiUtils.LoadImageFromMemory(app_icon, app_icon_len);
    m_pCLSettingsIcon = m_CImguiUtils.LoadImageFromMemory(cl_settings_icon_png, cl_settings_icon_png_len);
    m_pCloseIcon = m_CImguiUtils.LoadImageFromMemory(close_icon_png, close_icon_png_len);
    m_pCrosshairIcon = m_CImguiUtils.LoadImageFromMemory(crosshair_icon_png, crosshair_icon_png_len);
    m_pDrawIcon = m_CImguiUtils.LoadImageFromMemory(draw_icon_png, draw_icon_png_len);
    m_pSettingsIcon = m_CImguiUtils.LoadImageFromMemory(settings_icon_png, settings_icon_png_len);
}

void CImGuiCommands::Terminate()
{
    m_CImguiUtils.FreeImage(m_pAppIcon);
    m_CImguiUtils.FreeImage(m_pCLSettingsIcon);
    m_CImguiUtils.FreeImage(m_pCloseIcon);
    m_CImguiUtils.FreeImage(m_pCrosshairIcon);
    m_CImguiUtils.FreeImage(m_pDrawIcon);
    m_CImguiUtils.FreeImage(m_pSettingsIcon);
}

void CImGuiCommands::Think()
{
}

bool SidebarButton(const char* label, ImTextureID icon, bool selected, const ImVec2& size_arg, float scale, float x_pos)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGui::SetCursorPosX(x_pos);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) 
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImU32 bg_col = IM_COL32(0,0,0,0);
    ImU32 text_col = IM_COL32(140, 140, 140, 255);
    
    if (selected) 
    {
        bg_col = IM_COL32(45, 45, 52, 255); 
        text_col = IM_COL32(255, 255, 255, 255);
    }
    else if (hovered)
    {
        bg_col = IM_COL32(35, 35, 40, 255);
        text_col = IM_COL32(220, 220, 220, 255);
    }

    if (bg_col != 0)
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, 8.0f * scale);

    if (selected) 
    {
        window->DrawList->AddRectFilled(
            ImVec2(bb.Min.x, bb.Min.y + (8 * scale)),
            ImVec2(bb.Min.x + (3 * scale), bb.Max.y - (8 * scale)),
            IM_COL32(70, 150, 250, 255), 4.0f * scale
        );
    }

    float icon_sz = 18.0f * scale;
    ImVec2 icon_pos = ImVec2(bb.Min.x + (12 * scale), bb.Min.y + (size.y - icon_sz) / 2);
    ImU32 icon_tint = selected ? IM_COL32(255,255,255,255) : IM_COL32(160,160,160,200);
    window->DrawList->AddImage(icon, icon_pos, ImVec2(icon_pos.x + icon_sz, icon_pos.y + icon_sz), ImVec2(0,0), ImVec2(1,1), icon_tint);

    if (label_size.x > 0.0f) {
        ImVec2 text_pos = ImVec2(bb.Min.x + (40 * scale), bb.Min.y + (size.y - label_size.y) / 2);
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);
        ImGui::RenderText(text_pos, label);
        ImGui::PopStyleColor();
    }

    return pressed;
}

void CImGuiCommands::Draw()
{
    if (!m_ShowCommands)
        return;

    float uiScale = gEngfuncs.pfnGetCvarFloat("ui_imgui_scale");

    float headerH = 40.0f * uiScale;
    float sidebarW = 160.0f * uiScale;
    float padding = 12.0f * uiScale;
    float rounding = 12.0f * uiScale;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10 * uiScale, 10 * uiScale));
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f)); 
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0,0,0,0)); 

    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::Begin("AxionMenu", &m_ShowCommands, flags))
    {
        ImGui::SetWindowFontScale(uiScale);
        
        ImVec2 size = ImGui::GetWindowSize();
        ImVec2 pos = ImGui::GetWindowPos();
        
        if (pos.x < 0) 
            pos.x = 0; 
        
        if (pos.y < 0) 
            pos.y = 0; 
    
        if (pos.x + size.x > g_ImGuiViewport.scrWidth()) 
            pos.x = g_ImGuiViewport.scrWidth() - size.x; 

        if (pos.y + size.y > g_ImGuiViewport.scrHeight()) 
            pos.y = g_ImGuiViewport.scrHeight() - size.y; 
        
        ImGui::SetWindowPos(pos);

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        drawList->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(30, 30, 35, 255), rounding, ImDrawFlags_RoundCornersTop);
        drawList->AddLine( ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + winSize.x, winPos.y + headerH), IM_COL32(255, 255, 255, 15));

        float iconSize = 20.0f * uiScale;
        ImGui::SetCursorPos(ImVec2(padding, (headerH - iconSize) / 2));
        ImGui::Image(m_pAppIcon.texture, ImVec2(iconSize, iconSize));
        ImGui::SetCursorPos(ImVec2(padding + iconSize + padding, (headerH - ImGui::GetTextLineHeight()) / 2));
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Axion: CVars Menu");

        float closeBtnSz = 22.0f * uiScale;
        ImGui::SetCursorPos(ImVec2(winSize.x - closeBtnSz - padding, (headerH - closeBtnSz) / 2));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0)); 
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::ImageButton("##close", m_pCloseIcon.texture, ImVec2(closeBtnSz, closeBtnSz))) 
            m_ShowCommands = false;
        
        ImGui::PopStyleColor(2);

        drawList->AddRectFilled(ImVec2(winPos.x, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(22, 22, 26, 255), rounding, ImDrawFlags_RoundCornersBottomLeft);
        drawList->AddLine(ImVec2(winPos.x + sidebarW, winPos.y + headerH), ImVec2(winPos.x + sidebarW, winPos.y + winSize.y), IM_COL32(255, 255, 255, 10));

 
        ImGui::SetCursorPosY(headerH); 
        
        ImGui::BeginGroup();
        static int activeTab = 0;
        float btnH = 42.0f * uiScale;
        ImVec2 btnSize(sidebarW - padding, btnH);
        float btnX = padding / 2.0f;

        ImGui::Dummy(ImVec2(0, padding));

        if (SidebarButton("Client", m_pCLSettingsIcon.texture, activeTab == 0, btnSize, uiScale, btnX)) 
            activeTab = 0;
        if (SidebarButton("HUD", m_pSettingsIcon.texture, activeTab == 1, btnSize, uiScale, btnX)) 
            activeTab = 1;
        if (SidebarButton("Visuals", m_pDrawIcon.texture, activeTab == 2, btnSize, uiScale, btnX)) 
            activeTab = 2;
        if (SidebarButton("Crosshair", m_pCrosshairIcon.texture, activeTab == 3, btnSize, uiScale, btnX)) 
            activeTab = 3;
        
        ImGui::Dummy(ImVec2(sidebarW, 200 * uiScale)); 
        
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        
        ImGui::SetCursorPosY(headerH + padding);
 
        ImGui::Indent(padding); 

        ImGui::PushItemWidth(350.0f * uiScale);

        if (activeTab == 0) 
        {
            ImGui::TextDisabled("MOVEMENT");
            ImGui::Separator();
            
            bool cl_autojump = gEngfuncs.pfnGetCvarFloat("cl_autojump");
            if (ImGui::Checkbox("cl_autojump", &cl_autojump))
                m_CImguiUtils.SetCvarFloat("cl_autojump", cl_autojump);

            ImGui::TextDisabled("WEAPON & MODELS");
            ImGui::Separator();

            bool cl_weaponlowering = gEngfuncs.pfnGetCvarFloat("cl_weaponlowering");
            if (ImGui::Checkbox("cl_weaponlowering", &cl_weaponlowering)) 
                m_CImguiUtils.SetCvarFloat("cl_weaponlowering", cl_weaponlowering);

            bool cl_weaponsway = gEngfuncs.pfnGetCvarFloat("cl_weaponsway");
            if (ImGui::Checkbox("cl_weaponsway", &cl_weaponsway)) 
                m_CImguiUtils.SetCvarFloat("cl_weaponsway", cl_weaponsway);

            bool cl_weaponlag = gEngfuncs.pfnGetCvarFloat("cl_weaponlag");
            if (ImGui::Checkbox("cl_weaponlag", &cl_weaponlag)) 
                m_CImguiUtils.SetCvarFloat("cl_weaponlag", cl_weaponlag);

            bool cl_hidecorpses = gEngfuncs.pfnGetCvarFloat("cl_hidecorpses");
            if (ImGui::Checkbox("cl_hidecorpses", &cl_hidecorpses)) 
                m_CImguiUtils.SetCvarFloat("cl_hidecorpses", cl_hidecorpses);

            ImGui::TextDisabled("GAUSS SPRITES");
            ImGui::Separator();

            bool cl_gauss_balls = gEngfuncs.pfnGetCvarFloat("cl_gauss_balls");
            if (ImGui::Checkbox("cl_gauss_balls", &cl_gauss_balls)) m_CImguiUtils.SetCvarFloat("cl_gauss_balls", cl_gauss_balls);

            bool cl_gauss_hits = gEngfuncs.pfnGetCvarFloat("cl_gauss_hits");
            if (ImGui::Checkbox("cl_gauss_hits", &cl_gauss_hits)) m_CImguiUtils.SetCvarFloat("cl_gauss_hits", cl_gauss_hits);

            ImGui::TextDisabled("CHAT & SOUNDS");
            ImGui::Separator();

            int cl_logchat = (int)gEngfuncs.pfnGetCvarFloat("cl_logchat");
            int new_logchat = cl_logchat;
            
            ImGui::Text("cl_logchat:"); 
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("0", new_logchat == 0)) 
                new_logchat = 0;
            
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("1", new_logchat == 1))
                new_logchat = 1;
            
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("2", new_logchat == 2)) 
                new_logchat = 2;
            
            if (new_logchat != cl_logchat) 
                m_CImguiUtils.SetCvarFloat("cl_logchat", (float)new_logchat);

            bool cl_chatsound = gEngfuncs.pfnGetCvarFloat("cl_chatsound");
            if (ImGui::Checkbox("cl_chatsound", &cl_chatsound))
                m_CImguiUtils.SetCvarFloat("cl_chatsound", cl_chatsound);
            
            if (cl_chatsound) 
            {
                ImGui::Indent();
                const char* cl_chatsound_path = gEngfuncs.pfnGetCvarString("cl_chatsound_path");

                char buffer[256];
                strncpy(buffer, cl_chatsound_path, sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::InputText("cl_chatsound_path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::Unindent();
            }

            bool cl_killsound = gEngfuncs.pfnGetCvarFloat("cl_killsound");
            if (ImGui::Checkbox("cl_killsound", &cl_killsound))
                m_CImguiUtils.SetCvarFloat("cl_killsound", cl_killsound);

            if (cl_killsound)
             {
                ImGui::Indent();
                const char* cl_killsound_path = gEngfuncs.pfnGetCvarString("cl_killsound_path");

                char buffer[256];
                strncpy(buffer, cl_killsound_path, sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::InputText("cl_killsound_path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::Unindent();
            }
        }
        else if (activeTab == 1) 
        {
            ImGui::TextDisabled("HUD ELEMENTS");
            ImGui::Separator();
            
            bool hud_weapon = gEngfuncs.pfnGetCvarFloat("hud_weapon");
            if (ImGui::Checkbox("hud_weapon", &hud_weapon)) 
                m_CImguiUtils.SetCvarFloat("hud_weapon", hud_weapon);

            bool hud_watermark = gEngfuncs.pfnGetCvarFloat("hud_watermark");
            if (ImGui::Checkbox("hud_watermark", &hud_watermark)) 
                m_CImguiUtils.SetCvarFloat("hud_watermark", hud_watermark);

            bool hud_deathnotice_bg = gEngfuncs.pfnGetCvarFloat("hud_deathnotice_bg");
            if (ImGui::Checkbox("hud_deathnotice_bg", &hud_deathnotice_bg)) 
                m_CImguiUtils.SetCvarFloat("hud_deathnotice_bg", hud_deathnotice_bg);

            bool hud_health_divider = gEngfuncs.pfnGetCvarFloat("hud_health_divider");
            if (ImGui::Checkbox("hud_health_divider", &hud_health_divider)) 
                m_CImguiUtils.SetCvarFloat("hud_health_divider", hud_health_divider);

            bool hud_allow_hd = gEngfuncs.pfnGetCvarFloat("hud_allow_hd");
            if (ImGui::Checkbox("hud_allow_hd", &hud_allow_hd)) 
                m_CImguiUtils.SetCvarFloat("hud_allow_hd", hud_allow_hd);

            int hud_timer = (int)gEngfuncs.pfnGetCvarFloat("hud_timer");
            int new_timer = hud_timer;
            ImGui::Text("hud_timer:");
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("0##t", new_timer == 0)) 
                new_timer = 0;
            
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("1##t", new_timer == 1)) new_timer = 1;
            
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("2##t", new_timer == 2)) 
                new_timer = 2;
            
            ImGui::SameLine(); 
            
            if(ImGui::RadioButton("3##t", new_timer == 3)) 
                new_timer = 3;
            
            if(new_timer != hud_timer) 
                m_CImguiUtils.SetCvarFloat("hud_timer", (float)new_timer);

            ImGui::TextDisabled("SPEED & STRAFE");
            ImGui::Separator();

            bool hud_speedometer = gEngfuncs.pfnGetCvarFloat("hud_speedometer");
            if (ImGui::Checkbox("hud_speedometer", &hud_speedometer)) 
                m_CImguiUtils.SetCvarFloat("hud_speedometer", hud_speedometer);
            
            if (hud_speedometer)
            {
                ImGui::Indent();
                bool below = gEngfuncs.pfnGetCvarFloat("hud_speedometer_below_cross");
                if (ImGui::Checkbox("hud_speedometer_below_cross", &below)) 
                    m_CImguiUtils.SetCvarFloat("hud_speedometer_below_cross", below);
                ImGui::Unindent();
            }

            bool hud_jumpspeed = gEngfuncs.pfnGetCvarFloat("hud_jumpspeed");
            if (ImGui::Checkbox("hud_jumpspeed", &hud_jumpspeed))
                m_CImguiUtils.SetCvarFloat("hud_jumpspeed", hud_jumpspeed);
            if (hud_jumpspeed) 
            {
                ImGui::Indent();
                bool hud_jumpspeed_below_cross = gEngfuncs.pfnGetCvarFloat("hud_jumpspeed_below_cross");
                if (ImGui::Checkbox("hud_jumpspeed_below_cross", &hud_jumpspeed_below_cross)) 
                    m_CImguiUtils.SetCvarFloat("hud_jumpspeed_below_cross", hud_jumpspeed_below_cross);
                ImGui::Unindent();
            }

            bool hud_strafeguide = gEngfuncs.pfnGetCvarFloat("hud_strafeguide");
            if (ImGui::Checkbox("hud_strafeguide", &hud_strafeguide))
                m_CImguiUtils.SetCvarFloat("hud_strafeguide", hud_strafeguide);
        }
        else if (activeTab == 2) 
        {
            ImGui::TextDisabled("HUD COLORS");
            ImGui::Separator();

            bool hud_rainbow = gEngfuncs.pfnGetCvarFloat("hud_rainbow");
            if (ImGui::Checkbox("hud_rainbow", &hud_rainbow))
                m_CImguiUtils.SetCvarFloat("hud_rainbow", hud_rainbow);

            static ImVec4 hud_color;
            m_CImguiUtils.GetCvarColor("hud_color", (float*)&hud_color);

            if (ImGui::ColorEdit3("HUD Color", (float*)&hud_color))
            {
                m_CImguiUtils.SetCvarColor("hud_color", (float*)&hud_color);
            }

            if (ImGui::Button("Reset hud_color"))
            {
                hud_color = ImVec4(1.0f, 160.0f / 255.0f, 0.0f, 1.0f);
                m_CImguiUtils.SetCvarColor("hud_color", (float*)&hud_color);
            }
            
            bool vis_reload = gEngfuncs.pfnGetCvarFloat("vis_reload");
            if (ImGui::Checkbox("vis_reload", &vis_reload))
                m_CImguiUtils.SetCvarFloat("vis_reload", vis_reload);

            if (vis_reload)
            {
                ImGui::Indent();

                static ImVec4 vis_reload_color;
                m_CImguiUtils.GetCvarColor("vis_reload_color", (float*)&vis_reload_color);

                if (ImGui::ColorEdit3("Reload Color", (float*)&vis_reload_color))
                {
                    m_CImguiUtils.SetCvarColor("vis_reload_color", (float*)&vis_reload_color);
                }

                if (ImGui::Button("Reset Reload Color"))
                {
                    vis_reload_color = ImVec4(250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f, 1.0f);
                    m_CImguiUtils.SetCvarColor("vis_reload_color", (float*)&vis_reload_color);
                }

                ImGui::Unindent();
            }

            ImGui::TextDisabled("INDICATOR COLORS");
            ImGui::Separator();
            
            bool hud_vis = gEngfuncs.pfnGetCvarFloat("hud_vis");
            if (ImGui::Checkbox("hud_vis", &hud_vis))
                m_CImguiUtils.SetCvarFloat("hud_vis", hud_vis);

            if (hud_vis)
            {
                ImGui::Indent();

                static ImVec4 vis_health100;
                m_CImguiUtils.GetCvarColor("vis_health100", (float*)&vis_health100);

                static ImVec4 vis_health80;
                m_CImguiUtils.GetCvarColor("vis_health80", (float*)&vis_health80);

                static ImVec4 vis_health60;
                m_CImguiUtils.GetCvarColor("vis_health60", (float*)&vis_health60);

                static ImVec4 vis_health40;
                m_CImguiUtils.GetCvarColor("vis_health40", (float*)&vis_health40);

                static ImVec4 vis_health20;
                m_CImguiUtils.GetCvarColor("vis_health20", (float*)&vis_health20);

                if (ImGui::ColorEdit3("Health 100%", (float*)&vis_health100))
                    m_CImguiUtils.SetCvarColor("vis_health100", (float*)&vis_health100);

                if (ImGui::ColorEdit3("Health 80%", (float*)&vis_health80))
                    m_CImguiUtils.SetCvarColor("vis_health80", (float*)&vis_health80);

                if (ImGui::ColorEdit3("Health 60%", (float*)&vis_health60))
                    m_CImguiUtils.SetCvarColor("vis_health60", (float*)&vis_health60);

                if (ImGui::ColorEdit3("Health 40%", (float*)&vis_health40))
                    m_CImguiUtils.SetCvarColor("vis_health40", (float*)&vis_health40);

                if (ImGui::ColorEdit3("Health 20%", (float*)&vis_health20))
                    m_CImguiUtils.SetCvarColor("vis_health20", (float*)&vis_health20);

                if (ImGui::Button("Reset Health Colors"))
                {
                    vis_health100 = ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_health80 = ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_health60 = ImVec4(250.0f/255.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_health40 = ImVec4(250.0f/255.0f, 100.0f/255.0f, 0.0f, 1.0f);
                    vis_health20 = ImVec4(250.0f/255.0f, 0.0f, 0.0f, 1.0f);

                    m_CImguiUtils.SetCvarColor("vis_health100", (float*)&vis_health100);
                    m_CImguiUtils.SetCvarColor("vis_health80", (float*)&vis_health80);
                    m_CImguiUtils.SetCvarColor("vis_health60", (float*)&vis_health60);
                    m_CImguiUtils.SetCvarColor("vis_health40", (float*)&vis_health40);
                    m_CImguiUtils.SetCvarColor("vis_health20", (float*)&vis_health20);
                }

                static ImVec4 vis_battery100;
                m_CImguiUtils.GetCvarColor("vis_battery100", (float*)&vis_battery100);

                static ImVec4 vis_battery80;
                m_CImguiUtils.GetCvarColor("vis_battery80", (float*)&vis_battery80);

                static ImVec4 vis_battery60;
                m_CImguiUtils.GetCvarColor("vis_battery60", (float*)&vis_battery60);

                static ImVec4 vis_battery40;
                m_CImguiUtils.GetCvarColor("vis_battery40", (float*)&vis_battery40);

                static ImVec4 vis_battery20;
                m_CImguiUtils.GetCvarColor("vis_battery20", (float*)&vis_battery20);

                if (ImGui::ColorEdit3("Battery 100%", (float*)&vis_battery100))
                    m_CImguiUtils.SetCvarColor("vis_battery100", (float*)&vis_battery100);

                if (ImGui::ColorEdit3("Battery 80%", (float*)&vis_battery80))
                    m_CImguiUtils.SetCvarColor("vis_battery80", (float*)&vis_battery80);

                if (ImGui::ColorEdit3("Battery 60%", (float*)&vis_battery60))
                    m_CImguiUtils.SetCvarColor("vis_battery60", (float*)&vis_battery60);

                if (ImGui::ColorEdit3("Battery 40%", (float*)&vis_battery40))
                    m_CImguiUtils.SetCvarColor("vis_battery40", (float*)&vis_battery40);

                if (ImGui::ColorEdit3("Battery 20%", (float*)&vis_battery20))
                    m_CImguiUtils.SetCvarColor("vis_battery20", (float*)&vis_battery20);

                if (ImGui::Button("Reset Battery Colors"))
                {
                    vis_battery100 = ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_battery80 = ImVec4(0.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_battery60 = ImVec4(250.0f/255.0f, 250.0f/255.0f, 0.0f, 1.0f);
                    vis_battery40 = ImVec4(250.0f/255.0f, 100.0f/255.0f, 0.0f, 1.0f);
                    vis_battery20 = ImVec4(250.0f/255.0f, 0.0f, 0.0f, 1.0f);

                    m_CImguiUtils.SetCvarColor("vis_battery100", (float*)&vis_battery100);
                    m_CImguiUtils.SetCvarColor("vis_battery80",  (float*)&vis_battery80);
                    m_CImguiUtils.SetCvarColor("vis_battery60",  (float*)&vis_battery60);
                    m_CImguiUtils.SetCvarColor("vis_battery40",  (float*)&vis_battery40);
                    m_CImguiUtils.SetCvarColor("vis_battery20",  (float*)&vis_battery20);
                }
                ImGui::Unindent();
            }
        }
        else if (activeTab == 3) 
        {
            ImGui::TextDisabled("CROSSHAIR");
            ImGui::Separator();

            bool cl_cross = gEngfuncs.pfnGetCvarFloat("cl_cross");
            if (ImGui::Checkbox("cl_cross", &cl_cross))
                m_CImguiUtils.SetCvarFloat("cl_cross", cl_cross);

            if (cl_cross)
            {
                ImGui::Indent();

                static ImVec4 cross_color;
                m_CImguiUtils.GetCvarColor("cl_cross_color", (float*)&cross_color);

                if (ImGui::ColorEdit3("cl_cross_color", (float*)&cross_color))
                    m_CImguiUtils.SetCvarColor("cl_cross_color", (float*)&cross_color);

                if (ImGui::Button("Reset cl_cross_color"))
                {
                    cross_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    m_CImguiUtils.SetCvarColor("cl_cross_color", (float*)&cross_color);
                }

                int cl_cross_alpha = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_alpha");
                if (ImGui::SliderInt("cl_cross_alpha", &cl_cross_alpha, 0, 255))
                    m_CImguiUtils.SetCvarFloat("cl_cross_alpha", (float)cl_cross_alpha);

                int cl_cross_size = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_size");
                if (ImGui::SliderInt("cl_cross_size", &cl_cross_size, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_size", (float)cl_cross_size);

                int cl_cross_gap = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_gap");
                if (ImGui::SliderInt("cl_cross_gap", &cl_cross_gap, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_gap", (float)cl_cross_gap);

                int cl_cross_thickness = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_thickness");
                if (ImGui::SliderInt("cl_cross_thickness", &cl_cross_thickness, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_thickness", (float)cl_cross_thickness);

                bool cl_cross_outline = gEngfuncs.pfnGetCvarFloat("cl_cross_outline");
                if (ImGui::Checkbox("cl_cross_outline", &cl_cross_outline))
                    m_CImguiUtils.SetCvarFloat("cl_cross_outline", cl_cross_outline);

                bool cl_cross_top_line = gEngfuncs.pfnGetCvarFloat("cl_cross_top_line");
                if (ImGui::Checkbox("cl_cross_top_line", &cl_cross_top_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_top_line", cl_cross_top_line);

                bool cl_cross_bottom_line = gEngfuncs.pfnGetCvarFloat("cl_cross_bottom_line");
                if (ImGui::Checkbox("cl_cross_bottom_line", &cl_cross_bottom_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_bottom_line", cl_cross_bottom_line);

                bool cl_cross_left_line = gEngfuncs.pfnGetCvarFloat("cl_cross_left_line");
                if (ImGui::Checkbox("cl_cross_left_line", &cl_cross_left_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_left_line", cl_cross_left_line);

                bool cl_cross_right_line = gEngfuncs.pfnGetCvarFloat("cl_cross_right_line");
                if (ImGui::Checkbox("cl_cross_right_line", &cl_cross_right_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_right_line", cl_cross_right_line);

                int cl_cross_dot_size = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_dot_size");
                if (ImGui::SliderInt("cl_cross_dot_size", &cl_cross_dot_size, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_dot_size", (float)cl_cross_dot_size);

                if (cl_cross_dot_size)
                {
                    ImGui::Indent();

                    static ImVec4 dot_color;
                    m_CImguiUtils.GetCvarColor("cl_cross_dot_color", (float*)&dot_color);

                    if (ImGui::ColorEdit3("cl_cross_dot_color", (float*)&dot_color))
                        m_CImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dot_color);

                    if (ImGui::Button("Reset cl_cross_dot_color"))
                    {
                        dot_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                        m_CImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dot_color);
                    }

                    ImGui::Unindent();
                }

                int cl_cross_circle_radius = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_circle_radius");
                if (ImGui::SliderInt("cl_cross_circle_radius", &cl_cross_circle_radius, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_circle_radius", (float)cl_cross_circle_radius);

                if (cl_cross_circle_radius)
                {
                    ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

                    int cl_cross_circle_gap = (int)gEngfuncs.pfnGetCvarFloat("cl_cross_circle_gap");
                    if (ImGui::SliderInt("cl_cross_circle_gap", &cl_cross_circle_gap, 0, 100))
                        m_CImguiUtils.SetCvarFloat("cl_cross_circle_gap", (float)cl_cross_circle_gap);

                    static ImVec4 circle_color;
                    m_CImguiUtils.GetCvarColor("cl_cross_circle_color", (float*)&circle_color);

                    if (ImGui::ColorEdit3("cl_cross_circle_color", (float*)&circle_color))
                        m_CImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&circle_color);

                    if (ImGui::Button("Reset cl_cross_circle_color"))
                    {
                        circle_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                        m_CImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&circle_color);
                    }

                    ImGui::Unindent();
                }

                ImGui::Unindent();

                ImGui::Separator();

                if (ImGui::Button("Reset All Crosshair Settings", ImVec2(0, 0)))
                {
                    ImVec4 default_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    m_CImguiUtils.SetCvarColor("cl_cross_color", (float*)&default_color);
                    m_CImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&default_color);
                    m_CImguiUtils.SetCvarColor("cl_cross_circle_color", (float*)&default_color);
                    m_CImguiUtils.SetCvarFloat("cl_cross_alpha", 200.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_thickness", 2.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_size", 10.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_gap", 3.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_outline", 0.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_circle_radius", 0.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_circle_gap", 3.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_dot_size", 0.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_top_line", 1.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_bottom_line", 1.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_left_line", 1.0f);
                    m_CImguiUtils.SetCvarFloat("cl_cross_right_line", 1.0f);
                }
            }
        }
        
        ImGui::PopItemWidth();
        ImGui::Unindent(padding);
        ImGui::EndGroup();

        drawList->AddRect(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), IM_COL32(60, 60, 65, 255), rounding, ImDrawFlags_RoundCornersAll, 1.0f);
    }

    ImGui::PopStyleColor(2); 
    ImGui::PopStyleVar(4);
    ImGui::End();
}

bool CImGuiCommands::Active()
{
    return m_ShowCommands;
}

bool CImGuiCommands::CursorRequired()
{
    return true;
}

bool CImGuiCommands::HandleKey(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyNumber == K_ESCAPE)
    {
        m_ShowCommands = false;
    }
    return false;
}

void CImGuiCommands::CmdShowCommandsWindow()
{
    m_ShowCommands = true;
}
