#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "SOIS/ImGuiSample.hpp"
#include "SOIS/ApplicationContext.hpp"
#include "SOIS/DX11Renderer.hpp"

#include "imgui/imgui_stdlib.h"
#include "imgui/imgui_internal.h"



#include "capture.interop.h"
#include "SimpleCapture.h"




#pragma once
#include <dwmapi.h>
#include <d3dcompiler.h>

struct Window
{
public:
    Window(nullptr_t) {}
    Window(HWND hwnd, std::string const& title, std::string const& titleWithNumber, std::string& className)
    {
        mHwnd = hwnd;
        mTitle = title;
        mTitleWithNumber = titleWithNumber;
        mClassName = className;
    }

    HWND mHwnd;
    std::string mTitle;
    std::string mTitleWithNumber;
    std::string mClassName;
};

std::string AquireClassName(HWND hwnd)
{
	std::array<char, 1024> className;

    ::GetClassNameA(hwnd, className.data(), (int)className.size());

    std::string title(className.data());
    return title;
}

std::string AquireWindowText(HWND hwnd)
{
	std::array<char, 1024> windowText;

    ::GetWindowTextA(hwnd, windowText.data(), (int)windowText.size());

    std::string title(windowText.data());
    return title;
}

bool IsAltTabWindow(Window const& window)
{
    HWND hwnd = window.mHwnd;
    HWND shellWindow = GetShellWindow();

    auto title = window.mTitle;
    auto className = window.mClassName;

    if (hwnd == shellWindow)
    {
        return false;
    }

    if (title.length() == 0)
    {
        return false;
    }

    if (!IsWindowVisible(hwnd))
    {
        return false;
    }

    if (GetAncestor(hwnd, GA_ROOT) != hwnd)
    {
        return false;
    }

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!((style & WS_DISABLED) != WS_DISABLED))
    {
        return false;
    }

    DWORD cloaked = FALSE;
    HRESULT hrTemp = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (SUCCEEDED(hrTemp) &&
        cloaked == DWM_CLOAKED_SHELL)
    {
        return false;
    }

    return true;
}

namespace WindowEnumerationDetail 
{
    int gWindowCounter = 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto title = AquireWindowText(hwnd);
    std::string titleWithNumber = std::to_string(WindowEnumerationDetail::gWindowCounter);
    titleWithNumber += ": ";
    titleWithNumber += title;

    auto class_name = AquireClassName(hwnd);

    auto window = Window(hwnd, title, titleWithNumber, class_name);

    if (!IsAltTabWindow(window))
    {
        return TRUE;
    }

    std::vector<Window>& windows = *reinterpret_cast<std::vector<Window>*>(lParam);
    windows.push_back(window);
    
    WindowEnumerationDetail::gWindowCounter++;

    return TRUE;
}

const std::vector<Window> EnumerateWindows()
{
    std::vector<Window> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    WindowEnumerationDetail::gWindowCounter = 0;

    return windows;
}


























































struct WindowSelection
{
    std::vector<Window> mWindows;
    Window* mSelectedWindow = nullptr;

    WindowSelection()
    {
        ResetWindows();
    }

    void ResetWindows()
    {
        mWindows = EnumerateWindows();

        if (mWindows.size())
        {
            mSelectedWindow = &mWindows[0];
        }
        else
        {
            mSelectedWindow = nullptr;
        }
    }


    void Update()
    {
        // General BeginCombo() API, you have full control over your selection data and display type.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag stored in the object itself, etc.)
        if (ImGui::BeginCombo("Window To Display", mSelectedWindow->mTitle.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            for (auto& window : mWindows)
            {
                bool is_selected = (mSelectedWindow == &window);
                if (ImGui::Selectable(window.mTitleWithNumber.c_str(), is_selected))
                    mSelectedWindow = &window;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
            ImGui::EndCombo();
        }
    }
};










std::unique_ptr<SimpleCapture> CaptureWindow(HWND aWindowHandle, SOIS::ApplicationContext& aContext)
{
    auto renderer = dynamic_cast<SOIS::DX11Renderer*>(aContext.GetRenderer());

    if (renderer == nullptr)
    {
        printf("We're running on the wrong SOIS renderer, need DX");
        __debugbreak();
        std::terminate();
    }

    return std::make_unique<SimpleCapture>(renderer->mD3DDevice, aWindowHandle);
}




struct ImageDisplay
{
    ImVec2 Dimensions;
    ImVec2 Position;
};

// https://codereview.stackexchange.com/a/70916
ImageDisplay StretchToFit(ImVec2 aImageResolution, ImVec2 aWindowResolution)
{
    float scaleHeight = aWindowResolution.y / aImageResolution.y;
    float scaleWidth = aWindowResolution.x / aImageResolution.x;
    float scale = std::min(scaleHeight, scaleWidth);
    
    auto dimensions = ImVec2(scale * aImageResolution.x, scale * aImageResolution.y);
    auto position = ImVec2(0.f, 0.f);
    
    position = ImVec2((aWindowResolution.x - dimensions.x)/2, (aWindowResolution.y - dimensions.y)/2);

    return ImageDisplay{ dimensions, position };
}




ImageDisplay GetRenderDimensions(ImVec2 aImageResolution)
{
    ImGuiIO& io = ImGui::GetIO();
    return StretchToFit(aImageResolution, io.DisplaySize);
}




struct Vertex
{
    glm::vec4 mPosition;
    glm::vec4 mColor;
    glm::vec2 mTextureCoordinates;
};



struct VS_CONSTANT_BUFFER
{
    glm::mat4 mWorldViewProj;
};


struct DrawTextures
{
    std::vector<std::unique_ptr<SimpleCapture>> mTextures;

    void Render()
    {
        static bool zeroPosition = false;
        for (auto& window : mTextures)
        {
            auto drawData = window->GetDrawData();
            auto dimensions = GetRenderDimensions(ImVec2(drawData.mWidth, drawData.mHeight));
            
            auto heuristicClientPosition = window->HeuristicClientPosition();
            auto clientPosition = window->ClientPositionInWindow();
            auto textureSize = window->GetLastSize();

            RECT windowPosition;
            GetWindowRect(window->GetWindowHandle(), &windowPosition);
            
            RECT clientRect;
            GetClientRect(window->GetWindowHandle(), &clientRect);
            
            POINT clientPositionInScreen{0, 0};
            ClientToScreen(window->GetWindowHandle(), &clientPositionInScreen);

            ImGui::Begin("DebugText");
            ImGui::LabelText("WindowPosition", "WindowPosition: LT{%d, %d}, RB{%d, %d}", windowPosition.left, windowPosition.top, windowPosition.right, windowPosition.bottom);
            ImGui::LabelText("ClientPositionInScreen", "ClientPositionInScreen: %f, %f", clientPositionInScreen.x, clientPositionInScreen.y);
            ImGui::LabelText("ClientPositionInImage", "ClientPositionInImage: %f, %f", (float)clientPosition.x, (float)clientPosition.y);
            ImGui::LabelText("ImagePosition", "ImagePosition : %f, %f", dimensions.Position.x, dimensions.Position.y);
            ImGui::LabelText("ImageDimension", "ImageDimension: %f, %f", dimensions.Dimensions.x, dimensions.Dimensions.y);
            ImGui::Checkbox("Zero Out Position", &zeroPosition);
            ImGui::End();
            
            
            ImGui::SetCursorPos(ImVec2{10.0f, 10.0f});
            if (!zeroPosition)
            {
                ImGui::SetCursorPos(ImVec2{ dimensions.Position.x + 10, dimensions.Position.y + 10 });
            }

            ImVec2 uv1 = ImVec2{
                (float)heuristicClientPosition.x / (float)clientRect.right,
                (float)heuristicClientPosition.y / (float)clientRect.bottom};
            

            ImVec2 uv2 = ImVec2{
                (float)clientRect.right  / (float)textureSize.Width,
                (float)clientRect.bottom / (float)textureSize.Height};

            ImGui::Image((void*)drawData.mTextureView, dimensions.Dimensions, uv1, uv2);
        }
    }

    void ResizeIfRequired()
    {
        for (auto& window : mTextures)
        {
            window->ResizeIfRequired();
        }
    }
};


DrawTextures* gDrawTextures = nullptr;
SOIS::ApplicationContext* gContext = nullptr;








LRESULT CALLBACK WindowCreationHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (HSHELL_WINDOWCREATED == nCode)
    {
        HWND windowHandle = (HWND)wParam;

        for (auto& windowCapture : gDrawTextures->mTextures)
        {
            if (IsChild(windowCapture->GetWindowHandle(), windowHandle))
            {
                try
                {
                    auto window = CaptureWindow(windowHandle, *gContext);

                    window->StartCapture();
                    gDrawTextures->mTextures.emplace_back(std::move(window));
                }
                catch (...)
                {
                    std::cout << "Exception occurred";
                }

                return 0;
            }
        }
    }
}



























// Returns float between 0 and 1, inclusive;
float Rand()
{
  return ((float)rand() / (RAND_MAX));
}

ImVec2 ToImgui(glm::vec2 vec)
{
    return ImVec2(vec.x, vec.y);
}

class FancyPoint
{
public:
    glm::vec2 mPos;
    glm::vec2 mVelocity;
    glm::vec3 mColor;
    float mRadius = 2.f;

    FancyPoint(glm::vec2 pos, float r = 2.f, glm::vec3 c = glm::vec3(1.f, 1.f, 1.f), glm::vec2 velocity = glm::vec2(1, 1))
    {
        mPos = pos;
        mRadius = r;
        mColor = c;
        mVelocity = velocity;
    }

    bool IsOutCanvas()
    {
        ImGuiIO& io = ImGui::GetIO();
        glm::vec2 p = mPos;
        float w = io.DisplaySize.x;
        float h = io.DisplaySize.y;
        return (p.x > w || p.y > h || p.x < 0 || p.y < 0);
    }

    void update()
    {
        mPos.x += mVelocity.x;
        mPos.y += mVelocity.y;

        //mVelocity.x *= (Rand() > .01) ? 1 : -1;
        //mVelocity.y *= (Rand() > .01) ? 1 : -1;
        if (IsOutCanvas())
        {
            ImGuiIO& io = ImGui::GetIO();

            mPos = glm::vec2(Rand() * io.DisplaySize.x, Rand() * io.DisplaySize.y);
            mVelocity.x *= Rand() > .5 ? 1 : -1;
            mVelocity.y *= Rand() > .5 ? 1 : -1;
        }
    }

    void draw() const
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddCircleFilled(ImVec2(mPos.x, mPos.y), mRadius, ImColor{mColor.r, mColor.g, mColor.b});
    }

};

constexpr float cMinDistance = 73.f;

std::vector<FancyPoint> InitPoints()
{
    std::vector<FancyPoint> points;
    

    ImGuiIO& io = ImGui::GetIO();
    float canvasWidth = io.DisplaySize.x;
    float canvasHeight = io.DisplaySize.y;

    for (size_t i = 0; i < 100; i++)
    {
        points.emplace_back(glm::vec2(Rand() * canvasWidth, Rand() * canvasHeight), 3.4f, glm::vec3(1,0,0), glm::vec2(Rand()>.5?Rand():-Rand(), Rand()>.5?Rand():-Rand()));
    }

    return std::move(points);
}

void DrawPoints(std::vector<FancyPoint>& points)
{
    for (auto const& point1 : points)
    {
        for (auto const& point2 : points)
        {
            if ((&point1 != &point2))
            {
                auto distance = glm::distance(point1.mPos, point2.mPos);

                if (distance <=  cMinDistance)
                {
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddLine(ToImgui(point1.mPos), ToImgui(point2.mPos), ImColor(1.f, 1.f, 1.f, 1 - (distance / cMinDistance)));
                }
            }
        }
    }
    
    // Draw the points separately to make them draw on top
    for (auto const& point1 : points)
    {
        point1.draw();
    }
}

void UpdatePoints(std::vector<FancyPoint>& points)
{
    ImGuiIO& io = ImGui::GetIO();

    glm::vec2 mouse = glm::vec2(io.MousePos.x, io.MousePos.y);

    for (auto& point : points)
    {
        point.update();

        if (glm::distance(mouse, point.mPos) < 100.f)
        {
            auto direction = glm::normalize(point.mPos - mouse);
        
            point.mPos = mouse + (direction * 100.f);
        }
    }
}

std::string GetImGuiIniPath()
{
    auto sdlIniPath = SDL_GetPrefPath("PlaymerTools", "PadInput");

    std::filesystem::path path{ sdlIniPath };
    SDL_free(sdlIniPath);

    path /= "imgui.ini";

    return path.u8string();
}

int main(int, char**)
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    SOIS::ApplicationInitialization();
  
    auto iniPath = GetImGuiIniPath();

    SOIS::ApplicationContextConfig config;
    config.aBlocking = false;
    config.aIniFile = iniPath.c_str();
    config.aWindowName = "SOIS Template";

    SOIS::ApplicationContext context{config};
    gContext = &context;
    //SOIS::ImGuiSample sample;

    std::vector<FancyPoint> points;
    
    WindowSelection windowSelection;


    auto renderer = static_cast<SOIS::DX11Renderer*>(context.GetRenderer());
    DrawTextures drawWindows;
    gDrawTextures = &drawWindows;

    SetWindowsHookExA(WH_SHELL, &WindowCreationHook, NULL, 0);

    while (context.Update())
    {
        auto io = ImGui::GetIO();

        drawWindows.ResizeIfRequired();


        

        UpdatePoints(points);

        //sample.Update();

        ImGui::SetNextWindowPos(ImVec2(-10.0f, -10.0f));
        auto displaySize = ImVec2{ io.DisplaySize.x + 20, io.DisplaySize.y + 20};

        ImGui::SetNextWindowSize(displaySize);
        
		//ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(0.f, 0.f, 0.f, 0.f).Value);
        ImGui::Begin(
            "ImageWindow", 
            nullptr, 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing|
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        drawWindows.Render();
        ImGui::End();
        //ImGui::PopStyleColor();












        ImGui::Begin(
            "Canvas", 
            nullptr, 
            ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoBringToFrontOnFocus | 
            ImGuiWindowFlags_NoCollapse |  
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoInputs | 
            ImGuiWindowFlags_NoMove);
        {
            static bool firstRun = true;

            if (firstRun)
            {
                points = InitPoints();
                firstRun = false;
            }

            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
            ImGui::SetWindowPos(ImVec2(0, 0));

            DrawPoints(points);
        }
        ImGui::End();

        ImGui::Begin("Test Window");
        {
            if (ImGui::Button("Reset Windows"))
            {
                windowSelection.ResetWindows();
            }

            windowSelection.Update();


            
            if (ImGui::Button("Capture Selected Window"))
            {
                try
                {
                    auto window = CaptureWindow(windowSelection.mSelectedWindow->mHwnd, context);

                    window->StartCapture();
                    drawWindows.mTextures.clear();
                    drawWindows.mTextures.emplace_back(std::move(window));
                }
                catch (std::exception e)
                {
                    std::cout << e.what() << "\n";
                }
                catch (...)
                {
                    std::cout << "Unidentified exception occurred\n";
                }
            }

            //ImGui::ColorPicker4("Clear Color", &context.mClearColor[0]);
        }
        ImGui::End();
    }

    return 0;
}
