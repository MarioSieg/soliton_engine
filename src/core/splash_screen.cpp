// Copyright (c) 2024 Mario "Neo" Sieg. All Rights Reserved.

#include "splash_screen.hpp"

#include <raylib.h>

namespace lu {
    static Texture2D m_logo;
    static constinit int m_screen_width, m_screen_height;

    splash_screen::splash_screen(const char* const logoPath) {
        SetConfigFlags(FLAG_WINDOW_UNDECORATED);
        InitWindow(800, 600, "Lunam Engine Boot Screen");
        int monitorWidth = GetMonitorWidth(0);
        int monitorHeight = GetMonitorHeight(0);
        int screenWidth = monitorWidth / 4;
        int screenHeight = monitorHeight / 4;
        int windowPosX = (monitorWidth - screenWidth) / 2;
        int windowPosY = (monitorHeight - screenHeight) / 2;
        SetWindowSize(screenWidth, screenHeight);
        SetWindowPosition(windowPosX, windowPosY);
        m_screen_width = screenWidth;
        m_screen_height = screenHeight;
        m_logo = LoadTexture(logoPath);
        EnableEventWaiting();
    }

    splash_screen::~splash_screen() {
        SetWindowState(0);
        SetConfigFlags(0);
        UnloadTexture(m_logo);
        CloseWindow();
    }

    auto splash_screen::show() -> void {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        int logoPosX = (m_screen_width - m_logo.width) / 2;
        int logoPosY = (m_screen_height - m_logo.height) / 2;
        DrawTexture(m_logo, logoPosX, logoPosY, WHITE);
        DrawText("Press ENTER to continue", m_screen_width / 2 - 150, m_screen_height / 2 + m_logo.height / 2 + 20, 20, GRAY);
        EndDrawing();
    }
}
