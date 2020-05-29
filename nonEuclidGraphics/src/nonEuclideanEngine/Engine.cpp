#include <nonEuclideanEngine/Engine.h>
#include <fstream>
#include <core/geometry.h>
#include <core/transform.h>
#include <core/gl.h>
#include <string>
#include <sstream>

#include <app/AutoCameraController.h>

using namespace nonEuc;

static void glfw_error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

Engine::Engine()
{
    if (!Init())
    {
        std::cout << "Error:Engine::Init()" << std::endl;
    }
}

Engine::~Engine()
{

}

bool Engine::Init()
{
	// ����GLFW����
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(scrwidth, scrheight, "NonEuclidean", NULL, NULL);
    if (window == NULL)
        return false;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }

    // Imgui����
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Load Font

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.24f, 1.00f);

    // ��ʼ����ɫ��
    programID = LoadShaders("../nonEuclidGraphics/include/Shader/vertex.vert", "../nonEuclidGraphics/include/Shader/fragment.frag");
    
    // ������Ȳ���
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    
    // ͼƬ��Ⱦ��ʼ��
    glGenTextures(1, &imageID);
    glBindTexture(GL_TEXTURE_2D, imageID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    return true;
}

void Engine::Loop()
{
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        glfwGetFramebufferSize(window, &scrwidth, &scrheight);
        if (Roam_status)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        if (Roam_status && ImGui::GetIO().KeysDown['M'])
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            Roam_status = false;
        }
        if (status_line)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // ����Imgui�Ĵ���
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        CreateMainMenu();
        if (show_editor_menu)
            CreateEditorMenu();
        if (show_image)
            ShowImage();
        if (show_load_menu)
            CreateLoadMenu();

        // Rendering
        ImGui::Render();
        
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO:���Ƴ���
        if (Roam_status)
            UpdateCamera();
        matf4 perspective = Perspective(PI<float> / 4, scrheight == 0 ? 1.0f : (float)scrwidth / (float)scrheight, near_plane, far_plane);

        current_world->SetUniformLight(programID);
        for (int j = -2; j <= 2; j++) for(int k = -1; k <= -1; k++) for (int l = -1; l <= -1; l++)
        {
            matf4 view = current_world->camera.GetView(j, k, l);

            auto ViewPos = current_world->regularize(current_world->camera.paraPos, j, k, l);
            
            gl::SetVec3f(programID, "backgroundColor", vecf3{ clear_color.x, clear_color.y, clear_color.z });
            gl::SetFloat(programID, "zFar", far_plane);
            gl::SetMat4f(programID, "V", view);
            gl::SetMat4f(programID, "P", perspective);
            gl::SetMat3f(programID, "G", current_world->metric(ViewPos));
            gl::SetVec3f(programID, "viewPos", ViewPos);

            for (size_t i = 0; i < current_world->objectPtrs.size(); i++)
                current_world->objectPtrs[i]->Draw(programID);
        }

        // ����Imgui�Ĵ���,�����������ʹ�両�����Ϸ�
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
}

void Engine::Clear()
{
    // Cleanup
    glDeleteProgram(programID);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void Engine::UpdateCamera()
{
    static double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);
    lastTime = currentTime;
    ImGuiIO & io = ImGui::GetIO();
    Camera & camera = current_world->camera;

    // Get mouse position
    float dyaw = -mouse_speed * io.MouseDelta.x;
    float dpitch = mouse_speed * io.MouseDelta.y;

    // Get keyboard input
    auto keyboardInput = io.KeysDown;
    vecf3 du = vecf3(0.0f);             // ����Ĳ�������仯��
    vecf3 Front = vecf3{ camera.T(2, 0), camera.T(2, 1), camera.T(2, 2) };
    vecf3 Right = vecf3{ camera.T(0, 0), camera.T(0, 1), camera.T(0, 2) };
    vecf3 Up = vecf3{ camera.T(1, 0), camera.T(1, 1), camera.T(1, 2) };

    if (keyboardInput['A'])
        du = Right * (-move_speed) * deltaTime;
    else if (keyboardInput['S'])
        du = Front * (move_speed) * deltaTime;
    else if (keyboardInput['D'])
        du = Right * (move_speed) * deltaTime;
    else if (keyboardInput['W'])
        du = Front * (-move_speed) * deltaTime;
    else if (keyboardInput['Q'])
        du = Up * (move_speed) * deltaTime;
    else if (keyboardInput['E'])
        du = Up * (-move_speed) * deltaTime;

    else if (keyboardInput['I'])
        dpitch = -100 * move_speed * deltaTime;
    else if (keyboardInput['K'])
        dpitch = 100 * move_speed * deltaTime;
    else if (keyboardInput['J'])
        dyaw = 100 * move_speed * deltaTime;
    else if (keyboardInput['L'])
        dyaw = -100 * move_speed * deltaTime;

    current_world->camera.UpdateDirection(dyaw, dpitch);
    camera.UpdatePosition(du);
}

void Engine::CreateMainMenu()
{
    ImGui::Begin("Engine");
    //ImGui::Text("Information about the scene.(TODO)");
    ImGui::Text(" %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Camera (%f, %f, %f)", current_world->camera.paraPos[0], current_world->camera.paraPos[1], current_world->camera.paraPos[2]);

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::DragFloat3("Pos", current_world->camera.paraPos.data, 0);
        ImGui::SliderFloat("FarPlane", &far_plane, 5.f, 20.f);
        ImGui::SliderFloat("NearPlane", &near_plane, 0.001f, 1.0f);
    }
    if (ImGui::CollapsingHeader("Mesh"))
    {
        ImGui::Checkbox("Line", &status_line);
    }
    if (ImGui::CollapsingHeader("Global Render"))
    {
        static int imgWidth = 128;
        static float dt = 0.01f;
        static float distanceLimit = 5.0f;
        static float decay = 3.0f;
        ImGui::DragInt("Width", &imgWidth);
        ImGui::SliderFloat("StepSize", &dt, 0.001f, 0.1f);
        ImGui::SliderFloat("Distance", &distanceLimit, 1.0f, 20.0f);
        ImGui::SliderFloat("Decay", &decay, 1.0f, 10.0f);
        if (ImGui::Button("RayTracing"))
        {
            nonEuc::RayTracer rayTracer(&(*current_world));
            rayTracer.SetParameter(distanceLimit, decay, rgbf{ clear_color.x, clear_color.y, clear_color.z }, dt);    //��ʼ��Ⱦ����
            rayTracer.BuildBVH();                                               //����BVH

            image = rayTracer.RenderTracing(PI<float> / 4.0f, scrheight == 0 ? 1.0f : (float)scrwidth / (float)scrheight, imgWidth);

            show_image = true;

            cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
            for (int i = 0; i < image.cols; i++)
                for (int j = 0; j < image.rows; j++)
                    image.at<cv::Vec3f>(j, i) /= 256.f;
        }
        static char path[100] = "../data/Scripts/test.txt";
        ImGui::InputText("ScriptPath", path, 100);
        if (ImGui::Button("UseScript"))
        {
            AutoCameraController acc;
            if (acc.Init(path, current_world, distanceLimit, decay))
            {
                acc.Run();
                std::cout << "Done" << std::endl;
            }
            else
                std::cout << "ERROR:Failed to initialize AutoCameraController" << std::endl;
        }
    }
    if (ImGui::CollapsingHeader("Controller"))
    {
        ImGui::Checkbox("Roam", &Roam_status);
        ImGui::SameLine();
        ImGui::Text("(Press M to make the mouse unfocused)");
    }
    if (ImGui::CollapsingHeader("Settings"))
    {
        ImGui::ColorEdit4("Background", &clear_color.x);
    }
    ImGui::Checkbox("EditorMenu", &show_editor_menu);

    CreateWorldSelector();
    if (ImGui::Button("LoadMesh"))
        show_load_menu = true;
    ImGui::End();
}

void Engine::CreateEditorMenu()
{
    ImGui::Begin("Editor", &show_editor_menu);

    std::string strbuf;

    for (size_t i = 0; i < current_world->objectPtrs.size(); i++)
    {
        std::ostringstream s;
        std::string object_name = "";
        auto objPtr = current_world->objectPtrs[i];
        std::stringstream ss;
        switch (objPtr->obj_type)
        {
        case Object::ObjType::_AreaLight:
            ss << int(i);
            ss >> strbuf;
            object_name += "AreaLight " + strbuf;

            if (ImGui::CollapsingHeader(object_name.c_str()))
                CreateLightMenu(dynamic_cast<AreaLight*>(&*objPtr), &object_name);
            break;
        case Object::ObjType::_Object:
            ss << int(i);
            ss >> strbuf;
            object_name += "Mesh " + strbuf;

            if (ImGui::CollapsingHeader(object_name.c_str()))
                CreateMeshMenu(dynamic_cast<Object*>(&*objPtr), &object_name);
            break;
        default:
            break;
        }
        
    }
    ImGui::End();
}

void Engine::CreateMeshMenu(Object* pobject, std::string* name)
{
    float scale = pobject->scale(1, 1);
    ImGui::DragFloat3((*name+":Center").c_str(), pobject->center.data, 0.01f);

    pobject->center =  current_world->regularize(pobject->center, 0, 0, 0);
    ImGui::DragFloat((*name + ":Scale").c_str(), &scale, 0.01f, 0.0f, 10.0f);
    for (int i = 0; i < 3; i++) pobject->scale(i, i) = scale;

    ImGui::DragFloat3((*name + ":Rotation").c_str(), pobject->rotate.data, 1.0f, -180.0f, 180.0f);
    pobject->UpdateR();
    ImGui::Text("-----Material-----");
    ImGui::ColorEdit3((*name + ":Ambient").c_str(), &pobject->mesh->material.ambient[0]);
    ImGui::ColorEdit3((*name + ":Diffuse").c_str(), &pobject->mesh->material.diffuse[0]);
    ImGui::ColorEdit3((*name + ":Specular").c_str(), &pobject->mesh->material.specular[0]);
    if (ImGui::Button((*name + ":Delete").c_str()))
    {
        current_world->DeleteObj(pobject);
    }
}

void Engine::CreateLightMenu(AreaLight* plight, std::string* name)
{
    //ImGui::InputFloat3("Center", plight->.data, 3);
}

bool checkFilePath(std::string path);

void Engine::CreateLoadMenu()
{
    ImGui::Begin("Load", &show_load_menu);

    static char objPath[100] = "../data/ball.obj";
    static char texPath[100] = "../data/test.png";
    static float pos[3] = {PI<float> / 2, PI<float> / 2, PI<float> / 2};
    static float Scale = 0.05f;

    ImGui::InputText("ObjectPath", objPath, 100);
    ImGui::InputText("TexturePath", texPath, 100);
    ImGui::InputFloat3("Pos", pos, 3);
    ImGui::InputFloat("Scale", &Scale, 0.01f, 0.05f, 2);
    
    if (ImGui::Button("Load"))
    {
        if (!checkFilePath(objPath) || !checkFilePath(texPath))
            std::cout << "Fail to load mesh" << std::endl;
        else
        {
            current_world->AddObj(std::make_shared<Mesh>(objPath, texPath, Material::MaterialType::DEFAULT), vecf3(pos), vecf3(Scale), vecf3{ 0.f, 0.f, 0.f });
            ImGui::SameLine();
            std::cout << "Load Successfully" << std::endl;
        }
    }
    ImGui::End();
}

void Engine::CreateWorldSelector()
{
    static int idx = -1;
    if (ImGui::Combo("WorldExamples", &idx, "Euclidean\0Gaussian\0Hypersphere\0OneRecursive\0Hyperbolic\0Schwarzschild\0"))
    {
        //delete current_world;
        current_world = std::make_shared<nonEuc::World>(idx);
    }
}

void Engine::ShowImage()
{
    ImGui::Begin("Result", &show_image);
    
    glBindTexture(GL_TEXTURE_2D, imageID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_RGB, GL_FLOAT, image.data);
    auto size = ImGui::GetWindowSize();
    auto scale = std::max(size.x / image.cols, 0.0f);
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(imageID)), ImVec2(image.cols * scale, image.rows * scale));

    ImGui::End();
}

bool checkFilePath(std::string path)
{
    std::ifstream objfile(path);
    if (!objfile) {
        std::cout << "ERROR::Engine::checkFilePath" << std::endl
            << "\t" << "open file (" << path << ") fail" << std::endl;
        return false;
    }
    objfile.close();
    return true;
}