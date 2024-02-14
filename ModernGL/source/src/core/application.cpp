#include "core/application.hpp"
#include "core/object.hpp"
#include "core/scene.hpp"

#define _USE_MATH_DEFINES
#include <math.h>

#include "renderer/camera.hpp"
#include "renderer/point_light.hpp"
#include "renderer/directional_light.hpp"
#include "renderer/spot_light.hpp"

#include "resources/resource_manager.hpp"
#include "resources/texture.hpp"
#include "resources/model.hpp"
#include "resources/shader.hpp"

#include "core/debug/assert.hpp"
#include "core/debug/log.hpp"
#include "core/debug/file_logger.hpp"
#include "core/debug/console_logger.hpp"
#include "core/debug/fatal_logger.hpp"

#include "ui/engine_ui.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

bool Application::m_FirstMouse;
float Application::m_LastMouseX;
float Application::m_LastMouseY;

bool Application::m_LookingWithCamera;

GLFWwindow* Application::m_Window;
float Application::m_DeltaTime;

void Application::ResizeCallback(GLFWwindow* window, int32_t width, int32_t height)
{
	Camera::Instance->ScreenSize = Vector2(width, height);
	glViewport(0, 0, width, height);
}

void Application::ErrorCallback(int32_t error, const char* const description)
{
    Log::LogFatal(std::string("GLFW error ").append(std::to_string(error)).append(" : ").append(description));
}

void Application::MouseCallback(GLFWwindow* window, double xPosIn, double yPosIn)
{

    const float xPos = static_cast<float>(xPosIn);
    const float yPos = static_cast<float>(yPosIn);

    if (m_FirstMouse)
    {
        m_LastMouseX = xPos;
        m_LastMouseY = yPos;
        m_FirstMouse = false;
    }

    const float xOffset = xPos - m_LastMouseX;
    const float yOffset = m_LastMouseY - yPos; // reversed since y-coordinates go from bottom to top
    m_LastMouseX = xPos;
    m_LastMouseY = yPos;

    if (!m_LookingWithCamera)
        return;

    Camera* const cam = Camera::Instance;
    cam->ProcessMouse(xOffset, yOffset);
}

void Application::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    Camera* const cam = Camera::Instance;
    cam->ProcessScroll(yOffset);
}


void Application::ProcessInput()
{
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(m_Window, true);
        return;
    }

    Camera* const cam = Camera::Instance;

    if (glfwGetKey(m_Window, GLFW_KEY_W))
        cam->ProcessKeyboard(CameraMovement::FORWARD, m_DeltaTime);

    if (glfwGetKey(m_Window, GLFW_KEY_S))
        cam->ProcessKeyboard(CameraMovement::BACKWARD, m_DeltaTime);

    if (glfwGetKey(m_Window, GLFW_KEY_A))
        cam->ProcessKeyboard(CameraMovement::LEFT, m_DeltaTime);

    if (glfwGetKey(m_Window, GLFW_KEY_D))
        cam->ProcessKeyboard(CameraMovement::RIGHT, m_DeltaTime);

    int32_t cState = glfwGetKey(m_Window, GLFW_KEY_C);
    if (cState == GLFW_PRESS)
        m_LookingWithCamera = true;
    else if (cState == GLFW_RELEASE)
        m_LookingWithCamera = false;

    glfwSetInputMode(m_Window, GLFW_CURSOR, m_LookingWithCamera ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Application::SetupLogger()
{
    Log::Init();

    Log::AddLogger<ConsoleLogger>();
    Log::AddLogger(new FileLogger("log.txt"));
    Log::AddLogger(new FatalLogger("fatal_log.txt"));
}

void Application::SetupImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable viewports
    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void Application::PreLoop()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::PostLoop()
{
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    GLFWwindow* ctxBackup = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(ctxBackup);

    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}


void Application::Init()
{
    m_FirstMouse = true;

    // Setup the logger first so that it can immediatly be used for glfw
    SetupLogger();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit())
    {
        Log::LogFatal("Couldn't innitialize glfw");
    }

    glfwSetErrorCallback(ErrorCallback);

    m_Window = glfwCreateWindow(800, 600, "ModernGl", nullptr, nullptr);
    if (m_Window == nullptr)
    {
        glfwTerminate();
        Log::LogFatal("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_Window);

    glfwSetFramebufferSizeCallback(m_Window, ResizeCallback);
    glfwSetCursorPosCallback(m_Window, MouseCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    gladLoadGL();

    glfwSwapInterval(1); // Enable vsync

    SetupImgui();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void Application::MainLoop()
{
    // Task example
    /*auto&& tex = ResourceManager::CreateAsync<Texture>("assets/textures/viking_room.jpg").Then(
        [](auto t)
        {
            t->Load();
        }
    );*/

    Texture* const tex = ResourceManager::Create<Texture>("assets/textures/viking_room.jpg");
    tex->Load();

    Model* const model = ResourceManager::Create<Model>("assets/models/viking_room.obj");
    model->Load();

    Shader* const shader = ResourceManager::Create<Shader>("modelShader");
    shader->Load("source/shaders/default.vert", "source/shaders/default.frag");

    Camera camera(M_PI / 2.f, Vector2(800, 600), 0.1f, 100.f, Vector3(0.f, 0.f, 2.f), Vector3(0.f, 0.f, 0.f));

    float time = 0.f;

    Scene scene("Test scene");

    Object vikingRoom(shader, model, tex, Vector3(0.f), Vector3(-M_PI / 2, -M_PI / 2, 0), Vector3(1.f));
    vikingRoom.Name = "Viking room";

    Object vkSmall1(shader, model, tex, Vector3(1.f, 0.f, 0.f), Vector3(-M_PI / 2, -M_PI / 2, M_PI / 2),
        Vector3(1.f));
    vkSmall1.Name = "Viking room small 1";
    Object vkSmall2(shader, model, tex, Vector3(-1.f, 0.f, 0.f), Vector3(-M_PI / 2, -M_PI / 2, -M_PI / 2),
        Vector3(1.f));
    vkSmall2.Name = "Viking room small 2";

    Object lightObj(nullptr, nullptr, nullptr, Vector3(0.f), Vector3(0.f), Vector3(1.f));
    lightObj.Name = "Light";

    DirectionalLight* dirLight = new DirectionalLight(&lightObj, Vector3(0.f, 1.f, 0.f),
        Vector4(1.0f), Vector4(1.0f), Vector4(1.0f));
    /*PointLight* pointLight = new PointLight(&lightObj, Vector4(.1f, .1f, .1f, 1.f), Vector4(.1f, .8f, .1f, 1.f),
        Vector4(1.f, 0.f, 0.f, 1.f));
    SpotLight* const spotLight = new SpotLight(&lightObj, Vector3(0.f, 0.f, 1.f), 0.218f, 0.305f,
        Vector4(1.f), Vector4(1.f), Vector4(1.f));
    lightObj.AddComponent(spotLight);
    lightObj.AddComponent(pointLight);*/
    lightObj.AddComponent(dirLight);

    scene.AddObject(vikingRoom);
    scene.AddObject(lightObj);

    vkSmall1.SetParent(&vikingRoom);
    vkSmall2.SetParent(&vikingRoom);

    float lastFrame = 0;
    while (!glfwWindowShouldClose(m_Window))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        m_DeltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        PreLoop();
        ProcessInput();

        EngineUi::DrawSceneGraph(scene);

        time += m_DeltaTime;
        camera.Update();

        // lightObj.Transformation.Position = camera.Position;
        // spotLight->Direction = camera.GetFront();

        scene.ApplyLights(vikingRoom.GetShader());

        // vikingRoom.Transformation.Position.z = std::sin(time) * .5f;
        vikingRoom.Render();
        vkSmall1.Render();
        vkSmall2.Render();

        PostLoop();
    }

    delete dirLight;
    /*delete spotLight;
    delete pointLight;*/
}

void Application::Shutdown()
{
    ResourceManager::DeleteAll();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    Log::Stop();
}
