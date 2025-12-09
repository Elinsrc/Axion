#pragma once
#include "hud.h"
#include "cl_util.h"
#include "imgui.h"

class CImGuiBackend
{
    cvar_t *ui_imgui_scale;

public:
    CImGuiBackend();
    ~CImGuiBackend();

    bool Init();
    void Shutdown();
    void NewFrame();
    void RenderDrawData(ImDrawData *draw_data);

private:
    struct ImplData
    {
        ImplData() { memset((void*)this, 0, sizeof(*this)); }
    };

    ImplData* GetBackendData();
    void SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height);
    void UpdateTexture(ImTextureData* tex);
    bool CreateDeviceObjects();
    void DestroyDeviceObjects();
};
