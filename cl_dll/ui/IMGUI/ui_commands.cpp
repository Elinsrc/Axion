#include "ui_commands.h"
#include "imgui.h"
#include "hud.h"
#include "keydefs.h"
#include "cl_util.h"
#include "imgui_viewport.h"

bool CImGuiCommands::m_ShowCommands = false;

void CImGuiCommands::Initialize()
{
    gEngfuncs.pfnAddCommand("ui_commands", CImGuiCommands::CmdShowCommandsWindow);
}

void CImGuiCommands::VidInitialize()
{
}

void CImGuiCommands::Terminate()
{
}

void CImGuiCommands::Think()
{
}

void CImGuiCommands::Draw()
{
    if (!m_ShowCommands)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Axion: CVars Menu", &m_ShowCommands, window_flags);

    ImVec2 size = ImGui::GetWindowSize();

    if (size.x > g_ImGuiViewport.scrWidth())
        size.x = g_ImGuiViewport.scrWidth();

    if (size.y > g_ImGuiViewport.scrHeight())
        size.y = g_ImGuiViewport.scrHeight();

    ImGui::SetWindowSize(size);

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

    if (ImGui::BeginTabBar("MainTabBar"))
    {
        if (ImGui::BeginTabItem("Client"))
        {
            bool cl_autojump = m_CImguiUtils.GetCvarFloat("cl_autojump");
            if (ImGui::Checkbox("cl_autojump", &cl_autojump))
                m_CImguiUtils.SetCvarFloat("cl_autojump", cl_autojump);


            int cl_logchat = (int)m_CImguiUtils.GetCvarFloat("cl_logchat");
            int new_logchat = cl_logchat;
            const char* logchat_options[] = { "0", "1", "2" };

            ImGui::Text("cl_logchat:");
            for (int j = 0; j <= 2; j++)
            {
                ImGui::SameLine();
                bool checked = (new_logchat == j);
                if (ImGui::Checkbox(logchat_options[j], &checked) && checked)
                {
                    new_logchat = j;
                }
            }

            if (new_logchat != cl_logchat)
                m_CImguiUtils.SetCvarFloat("cl_logchat", (float)new_logchat);

            bool cl_chatsound = m_CImguiUtils.GetCvarFloat("cl_chatsound");
            if (ImGui::Checkbox("cl_chatsound", &cl_chatsound))
                m_CImguiUtils.SetCvarFloat("cl_chatsound", cl_chatsound);

            if (cl_chatsound)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());
                const char* cl_chatsound_path = gEngfuncs.pfnGetCvarString("cl_chatsound_path");

                char buffer[256];
                strncpy(buffer, cl_chatsound_path, sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::InputText("cl_chatsound_path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
            }

            bool cl_weaponlowering = m_CImguiUtils.GetCvarFloat("cl_weaponlowering");
            if (ImGui::Checkbox("cl_weaponlowering", &cl_weaponlowering))
                m_CImguiUtils.SetCvarFloat("cl_weaponlowering", cl_weaponlowering);

            bool cl_weaponsway = m_CImguiUtils.GetCvarFloat("cl_weaponsway");
            if (ImGui::Checkbox("cl_weaponsway", &cl_weaponsway))
                m_CImguiUtils.SetCvarFloat("cl_weaponsway", cl_weaponsway);

            bool cl_weaponlag = m_CImguiUtils.GetCvarFloat("cl_weaponlag");
            if (ImGui::Checkbox("cl_weaponlag", &cl_weaponlag))
                m_CImguiUtils.SetCvarFloat("cl_weaponlag", cl_weaponlag);

            bool cl_gauss_balls = m_CImguiUtils.GetCvarFloat("cl_gauss_balls");
            if (ImGui::Checkbox("cl_gauss_balls", &cl_gauss_balls))
                m_CImguiUtils.SetCvarFloat("cl_gauss_balls", cl_gauss_balls);

            bool cl_gauss_hits = m_CImguiUtils.GetCvarFloat("cl_gauss_hits");
            if (ImGui::Checkbox("cl_gauss_hits", &cl_gauss_hits))
                m_CImguiUtils.SetCvarFloat("cl_gauss_hits", cl_gauss_hits);

            bool cl_hidecorpses = m_CImguiUtils.GetCvarFloat("cl_hidecorpses");
            if (ImGui::Checkbox("cl_hidecorpses", &cl_hidecorpses))
                m_CImguiUtils.SetCvarFloat("cl_hidecorpses", cl_hidecorpses);

            bool cl_killsound = m_CImguiUtils.GetCvarFloat("cl_killsound");
            if (ImGui::Checkbox("cl_killsound", &cl_killsound))
                m_CImguiUtils.SetCvarFloat("cl_killsound", cl_killsound);

            if (cl_killsound)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());
                const char* cl_killsound_path = gEngfuncs.pfnGetCvarString("cl_killsound_path");

                char buffer[256];
                strncpy(buffer, cl_killsound_path, sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';

                ImGui::InputText("cl_killsound_path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("HUD"))
        {
            bool hud_weapon = m_CImguiUtils.GetCvarFloat("hud_weapon");
            if (ImGui::Checkbox("hud_weapon", &hud_weapon))
                m_CImguiUtils.SetCvarFloat("hud_weapon", hud_weapon);

            bool hud_speedometer = m_CImguiUtils.GetCvarFloat("hud_speedometer");
            if (ImGui::Checkbox("hud_speedometer", &hud_speedometer))
                m_CImguiUtils.SetCvarFloat("hud_speedometer", hud_speedometer);

            if (hud_speedometer)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());
                bool hud_speedometer_below_cross = m_CImguiUtils.GetCvarFloat("hud_speedometer_below_cross");
                if (ImGui::Checkbox("hud_speedometer_below_cross", &hud_speedometer_below_cross))
                    m_CImguiUtils.SetCvarFloat("hud_speedometer_below_cross", hud_speedometer_below_cross);
                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
            }

            bool hud_jumpspeed = m_CImguiUtils.GetCvarFloat("hud_jumpspeed");
            if (ImGui::Checkbox("hud_jumpspeed", &hud_jumpspeed))
                m_CImguiUtils.SetCvarFloat("hud_jumpspeed", hud_jumpspeed);

            if (hud_jumpspeed)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());
                bool hud_jumpspeed_below_cross = m_CImguiUtils.GetCvarFloat("hud_jumpspeed_below_cross");
                if (ImGui::Checkbox("hud_jumpspeed_below_cross", &hud_jumpspeed_below_cross))
                    m_CImguiUtils.SetCvarFloat("hud_jumpspeed_below_cross", hud_jumpspeed_below_cross);
                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());

            }

            bool hud_strafeguide = m_CImguiUtils.GetCvarFloat("hud_strafeguide");
            if (ImGui::Checkbox("hud_strafeguide", &hud_strafeguide))
                m_CImguiUtils.SetCvarFloat("hud_strafeguide", hud_strafeguide);

            bool hud_watermark = m_CImguiUtils.GetCvarFloat("hud_watermark");
            if (ImGui::Checkbox("hud_watermark", &hud_watermark))
                m_CImguiUtils.SetCvarFloat("hud_watermark", hud_watermark);

            bool hud_deathnotice_bg = m_CImguiUtils.GetCvarFloat("hud_deathnotice_bg");
            if (ImGui::Checkbox("hud_deathnotice_bg", &hud_deathnotice_bg))
                m_CImguiUtils.SetCvarFloat("hud_deathnotice_bg", hud_deathnotice_bg);

            bool hud_health_divider = m_CImguiUtils.GetCvarFloat("hud_health_divider");
            if (ImGui::Checkbox("hud_health_divider", &hud_health_divider))
                m_CImguiUtils.SetCvarFloat("hud_health_divider", hud_health_divider);

            bool hud_allow_hd = m_CImguiUtils.GetCvarFloat("hud_allow_hd");
            if (ImGui::Checkbox("hud_allow_hd", &hud_allow_hd))
                m_CImguiUtils.SetCvarFloat("hud_allow_hd", hud_allow_hd);

            int hud_timer = (int)m_CImguiUtils.GetCvarFloat("hud_timer");
            int new_timer = hud_timer;
            const char* timer_options[] = { "0", "1", "2", "3" };

            ImGui::Text("hud_timer:");
            for (int j = 0; j <= 3; j++)
            {
                ImGui::SameLine();
                bool checked = (new_timer == j);
                if (ImGui::Checkbox(timer_options[j], &checked) && checked)
                {
                    new_timer = j;
                }
            }

            if (new_timer != hud_timer)
                m_CImguiUtils.SetCvarFloat("hud_timer", (float)new_timer);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("HUD Color"))
        {
            bool hud_rainbow = m_CImguiUtils.GetCvarFloat("hud_rainbow");
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

            bool vis_reload = m_CImguiUtils.GetCvarFloat("vis_reload");
            if (ImGui::Checkbox("vis_reload", &vis_reload))
                m_CImguiUtils.SetCvarFloat("vis_reload", vis_reload);

            if (vis_reload)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

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

                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
            }

            bool hud_vis = m_CImguiUtils.GetCvarFloat("hud_vis");
            if (ImGui::Checkbox("hud_vis", &hud_vis))
                m_CImguiUtils.SetCvarFloat("hud_vis", hud_vis);

            if (hud_vis)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

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

                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Crosshairs"))
        {
            bool cl_cross = m_CImguiUtils.GetCvarFloat("cl_cross");
            if (ImGui::Checkbox("cl_cross", &cl_cross))
                m_CImguiUtils.SetCvarFloat("cl_cross", cl_cross);

            if (cl_cross)
            {
                ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

                static ImVec4 cross_color;
                m_CImguiUtils.GetCvarColor("cl_cross_color", (float*)&cross_color);

                if (ImGui::ColorEdit3("cl_cross_color", (float*)&cross_color))
                    m_CImguiUtils.SetCvarColor("cl_cross_color", (float*)&cross_color);

                if (ImGui::Button("Reset cl_cross_color"))
                {
                    cross_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                    m_CImguiUtils.SetCvarColor("cl_cross_color", (float*)&cross_color);
                }

                int cl_cross_alpha = (int)m_CImguiUtils.GetCvarFloat("cl_cross_alpha");
                if (ImGui::SliderInt("cl_cross_alpha", &cl_cross_alpha, 0, 255))
                    m_CImguiUtils.SetCvarFloat("cl_cross_alpha", (float)cl_cross_alpha);

                int cl_cross_size = (int)m_CImguiUtils.GetCvarFloat("cl_cross_size");
                if (ImGui::SliderInt("cl_cross_size", &cl_cross_size, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_size", (float)cl_cross_size);

                int cl_cross_gap = (int)m_CImguiUtils.GetCvarFloat("cl_cross_gap");
                if (ImGui::SliderInt("cl_cross_gap", &cl_cross_gap, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_gap", (float)cl_cross_gap);

                int cl_cross_thickness = (int)m_CImguiUtils.GetCvarFloat("cl_cross_thickness");
                if (ImGui::SliderInt("cl_cross_thickness", &cl_cross_thickness, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_thickness", (float)cl_cross_thickness);

                bool cl_cross_outline = m_CImguiUtils.GetCvarFloat("cl_cross_outline");
                if (ImGui::Checkbox("cl_cross_outline", &cl_cross_outline))
                    m_CImguiUtils.SetCvarFloat("cl_cross_outline", cl_cross_outline);

                bool cl_cross_top_line = m_CImguiUtils.GetCvarFloat("cl_cross_top_line");
                if (ImGui::Checkbox("cl_cross_top_line", &cl_cross_top_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_top_line", cl_cross_top_line);

                bool cl_cross_bottom_line = m_CImguiUtils.GetCvarFloat("cl_cross_bottom_line");
                if (ImGui::Checkbox("cl_cross_bottom_line", &cl_cross_bottom_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_bottom_line", cl_cross_bottom_line);

                bool cl_cross_left_line = m_CImguiUtils.GetCvarFloat("cl_cross_left_line");
                if (ImGui::Checkbox("cl_cross_left_line", &cl_cross_left_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_left_line", cl_cross_left_line);

                bool cl_cross_right_line = m_CImguiUtils.GetCvarFloat("cl_cross_right_line");
                if (ImGui::Checkbox("cl_cross_right_line", &cl_cross_right_line))
                    m_CImguiUtils.SetCvarFloat("cl_cross_right_line", cl_cross_right_line);

                int cl_cross_dot_size = (int)m_CImguiUtils.GetCvarFloat("cl_cross_dot_size");
                if (ImGui::SliderInt("cl_cross_dot_size", &cl_cross_dot_size, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_dot_size", (float)cl_cross_dot_size);

                if (cl_cross_dot_size)
                {
                    ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

                    static ImVec4 dot_color;
                    m_CImguiUtils.GetCvarColor("cl_cross_dot_color", (float*)&dot_color);

                    if (ImGui::ColorEdit3("cl_cross_dot_color", (float*)&dot_color))
                        m_CImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dot_color);

                    if (ImGui::Button("Reset cl_cross_dot_color"))
                    {
                        dot_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                        m_CImguiUtils.SetCvarColor("cl_cross_dot_color", (float*)&dot_color);
                    }

                    ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
                }

                int cl_cross_circle_radius = (int)m_CImguiUtils.GetCvarFloat("cl_cross_circle_radius");
                if (ImGui::SliderInt("cl_cross_circle_radius", &cl_cross_circle_radius, 0, 100))
                    m_CImguiUtils.SetCvarFloat("cl_cross_circle_radius", (float)cl_cross_circle_radius);

                if (cl_cross_circle_radius)
                {
                    ImGui::Indent(ImGui::GetFrameHeightWithSpacing());

                    int cl_cross_circle_gap = (int)m_CImguiUtils.GetCvarFloat("cl_cross_circle_gap");
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

                    ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());
                }

                ImGui::Unindent(ImGui::GetFrameHeightWithSpacing());

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
            ImGui::EndTabItem();
        }

        /*if (ImGui::BeginTabItem("Debug"))
        {
            int cl_debug = (int)m_CImguiUtils.GetCvarFloat("cl_debug");
            int new_debug = cl_debug;
            const char* debug_options[] = { "0", "1", "2", "3" };

            ImGui::Text("cl_debug:");
            for (int j = 0; j <= 3; j++)
            {
                ImGui::SameLine();
                bool checked = (new_debug == j);
                if (ImGui::Checkbox(debug_options[j], &checked) && checked)
                {
                    new_debug = j;
                }
            }

            if (new_debug != cl_debug)
                m_CImguiUtils.SetCvarFloat("cl_debug", (float)new_debug);

            bool cl_debug_showfps = m_CImguiUtils.GetCvarFloat("cl_debug_showfps");
            if (ImGui::Checkbox("cl_debug_showfps", &cl_debug_showfps))
                m_CImguiUtils.SetCvarFloat("cl_debug_showfps", cl_debug_showfps);

            ImGui::EndTabItem();
        }*/

        ImGui::EndTabBar();
    }

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
