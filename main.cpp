#include <iostream>
#define STB_IMAGE_IMPLEMENTATION 
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "model_data.h"
#include "DrawBall.h"
#include "AABB.h"
#include "vector"

#pragma region Model Data

float roomVertices[] = {
    // 背面 (Z = -roomSize)
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 左上
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 右上
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 右下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 右下
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f, // 左下
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f, // 左上

    // 正面 (Z = roomSize)
    -5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 左下
     5.0f, -5.0f,  5.0f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 右下
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 右上
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 右上
    -5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f, // 左上
    -5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f, // 左下

    // 左面 (X = -roomSize)
    -5.0f,  5.0f,  5.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 右上
    -5.0f,  5.0f, -5.0f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 左上
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 左下
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 左下
    -5.0f, -5.0f,  5.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f, // 右下
    -5.0f,  5.0f,  5.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f, // 右上

    // 右面 (X = roomSize)
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 左上
     5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 右上
     5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 右下
     5.0f, -5.0f,  5.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 右下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f, // 左下
     5.0f,  5.0f, -5.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f, // 左上

    // 底面 (Y = -roomSize)
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 左下
     5.0f, -5.0f, -5.0f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 右下
     5.0f, -5.0f,  5.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 右上
     5.0f, -5.0f,  5.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 右上
    -5.0f, -5.0f,  5.0f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f, // 左上
    -5.0f, -5.0f, -5.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f, // 左下

    // 頂面 (Y = roomSize)
    -5.0f,  5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f, // 左上
     5.0f,  5.0f, -5.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f, // 右上
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 右下
     5.0f,  5.0f,  5.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 右下
    -5.0f,  5.0f,  5.0f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f, // 左下
    -5.0f,  5.0f, -5.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f  // 左上
};


#pragma endregion


#pragma region Input Declare

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
#pragma endregion

unsigned int LoadImageToGPU(const char* filename, GLint internalFormat, GLenum format, int textureSlot) {
    unsigned int TexBuffer;
    glGenTextures(1, &TexBuffer);
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, TexBuffer);


    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        printf("Texture %s loaded successfully: %d x %d\n", filename, width, height);
    }
    else {
        printf("Failed to load texture: %s\n", stbi_failure_reason());
    }
    stbi_image_free(data);
    return TexBuffer;
}

float x = 7.2f, y = 6.3f, z = 4.8f;
// Room AABB from -5 to 5 in all dimensions
AABB roomAABB(glm::vec3(x-10.0f, y-10.0f, z-10.0f), glm::vec3(x, y, z)); 

// Time tracking for physics
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Add physics controls for GUI
bool pausePhysics = false;
float gravityStrength = 9.8f;
bool resetBall = false;

std::vector<DrawBall*> balls; 
int maxBalls = 10;
int currentBalls = 1; 

void InitializeBalls(int count, GLuint VAO, int vertexCount) {
    // 清空現有的球
    for (auto ball : balls) {
        delete ball;
    }
    balls.clear();
    
    // 獲取房間的邊界
    glm::vec3 roomMin = roomAABB.GetMin();
    glm::vec3 roomMax = roomAABB.GetMax();
    
    // 生成指定數量的球
    for (int i = 0; i < count; i++) {
        DrawBall* ball = new DrawBall(VAO, vertexCount);
        
        // 設置統一大小
        float scale = 0.1f;
        ball->SetScale(scale);
        
        // 隨機生成位置，y 固定為 -1.0f
        float x = roomMin.x + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.x - roomMin.x - 2.0f * scale);
        float z = roomMin.z + scale + (static_cast<float>(rand()) / RAND_MAX) * (roomMax.z - roomMin.z - 2.0f * scale);
        float y = -1.0f;
        glm::vec3 position(x, y, z);
        
        // 確保位置在房間內
        position.x = glm::clamp(position.x, roomMin.x + scale, roomMax.x - scale);
        position.z = glm::clamp(position.z, roomMin.z + scale, roomMax.z - scale);
        
        ball->SetPosition(position);
        
        // 給一個隨機的水平初速度
        float speedRange = 5.0f; // 速度範圍 [-speedRange, speedRange]
        float vx = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speedRange; // 隨機 X 分量
        float vz = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * speedRange; // 隨機 Z 分量
        ball->SetVelocity(glm::vec3(vx, 0.0f, vz)); // 初始 Y 速度為 0，靠重力掉落
        
        ball->SetGravity(-gravityStrength);
        
        balls.push_back(ball);
    }
    
    currentBalls = count;
}

void ResolveSphereCollision(DrawBall* ball1, DrawBall* ball2) {
    glm::vec3 pos1 = ball1->GetPosition();
    glm::vec3 pos2 = ball2->GetPosition();
    float radius1 = ball1->GetScale();
    float radius2 = ball2->GetScale();

    glm::vec3 delta = pos2 - pos1;
    float distance = glm::length(delta);

    if (distance < 0.0001f) {
        delta = glm::vec3(static_cast<float>(rand()) / RAND_MAX - 0.5f);
        distance = glm::length(delta);
    }

    float overlap = (radius1 + radius2) - distance;

    if (overlap <= 0) {
        return;
    }

    // 設置顏色為綠色
    ball1->SetColor(glm::vec3(0.0f, 1.0f, 0.0f)); // 綠色
    ball2->SetColor(glm::vec3(0.0f, 1.0f, 0.0f)); // 綠色

    glm::vec3 normal = delta / distance;

    float totalMass = 1.0f;
    float correction1 = overlap * 0.5f;
    float correction2 = overlap * 0.5f;

    ball1->SetPosition(pos1 - normal * correction1);
    ball2->SetPosition(pos2 + normal * correction2);

    pos1 = ball1->GetPosition();
    pos2 = ball2->GetPosition();

    glm::vec3 vel1 = ball1->GetVelocity();
    glm::vec3 vel2 = ball2->GetVelocity();

    float v1n = glm::dot(vel1, normal);
    float v2n = glm::dot(vel2, normal);

    if (v1n > v2n) {
        return;
    }

    float restitution = 0.6f;

    float v1nAfter = (v1n * (0.0f) + v2n * 2.0f) / 2.0f;
    float v2nAfter = (v2n * (0.0f) + v1n * 2.0f) / 2.0f;

    v1nAfter = v1n + restitution * (v1nAfter - v1n);
    v2nAfter = v2n + restitution * (v2nAfter - v2n);

    glm::vec3 v1nVector = normal * v1nAfter;
    glm::vec3 v2nVector = normal * v2nAfter;

    glm::vec3 v1t = vel1 - (normal * v1n);
    glm::vec3 v2t = vel2 - (normal * v2n);

    ball1->SetVelocity(v1t + v1nVector);
    ball2->SetVelocity(v2t + v2nVector);

    // 添加隨機擾動（只有在速度大於閾值時）
    float randomFactor = 0.2f;

    if (glm::length(ball1->GetVelocity()) > 0.05f) {
        ball1->SetVelocity(ball1->GetVelocity() + glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor,
            0.0f,
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor
        ));
    }
    if (glm::length(ball2->GetVelocity()) > 0.05f) {
        ball2->SetVelocity(ball2->GetVelocity() + glm::vec3(
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor,
            0.0f,
            (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * randomFactor
        ));
    }

    ball1->SetVelocity(ball1->GetVelocity() * 0.99f);
    ball2->SetVelocity(ball2->GetVelocity() * 0.99f);
}


float ceilingMixFactor = 0.5f;
float initialSpeedRange = 5.0f;
float groundFriction = 0.99f;

bool light1Enabled = true; // 第一個光源開關
bool light2Enabled = true; // 第二個光源開關


// 全局變量部分
glm::vec3 miniRoomPositions[3] = {
    glm::vec3(0.0f, -4.5f, 0.0f),  // 迷你房間 1
    glm::vec3(3.0f, -4.5f, 2.0f),  // 迷你房間 2
    glm::vec3(-3.0f, -4.5f, -2.0f) // 迷你房間 3
};

glm::vec3 miniRoomScales[3] = {
    glm::vec3(0.1f),   // 迷你房間 1 縮放到 10%
    glm::vec3(0.1f),   // 迷你房間 2
    glm::vec3(0.1f)   // 迷你房間 3
};

std::vector<AABB> miniRoomAABBs;

int main() {

    #pragma region Open a Window
        if (!glfwInit()) {
            printf("Failed to initialize GLFW\n");
            return -1;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(1600, 1200, "3D render", NULL, NULL);
        if (window == NULL) {
            const char* description;
            int code = glfwGetError(&description);
            printf("GLFW Error %d: %s\n", code, description);
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  

        // init GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            glfwTerminate();
            return -1;
        }

        glViewport(0, 0, 1600, 1200);
        glEnable(GL_DEPTH_TEST);
    #pragma endregion
    
    #pragma region Init ImGui
    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); 
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400 core"); 
    #pragma endregion

    #pragma region Init Shader Program
    // load vertex and fragment shader
    Shader* myShader = new Shader("vertexShaderSource.vert", "fragmentShaderSource.frag");
    
    #pragma endregion
    
    #pragma region Init Mini Rooms AABB
    for (int i = 0; i < 3; ++i) {
        glm::vec3 center = miniRoomPositions[i];
        glm::vec3 halfSize = glm::vec3(1.0f); // 半徑為 1
    
        glm::vec3 minPt = center - halfSize;
        glm::vec3 maxPt = center + halfSize;
    
        AABB box(minPt, maxPt);
        miniRoomAABBs.push_back(box);
    
        std::cout << "[Init] MiniRoom[" << i << "] AABB Min: (" 
                  << minPt.x << ", " << minPt.y << ", " << minPt.z << ") Max: ("
                  << maxPt.x << ", " << maxPt.y << ", " << maxPt.z << ")\n";
    }
    
    #pragma endregion

    #pragma region Init and load Model to VAO & VBO
    // create & bind cube VAO & VBO
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // set attrib pointer 
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);   
    glEnableVertexAttribArray(6);
    //glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(7);
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); 
    glEnableVertexAttribArray(9);

    
    // room VAO & VBO
    unsigned int roomVAO, roomVBO;
    glGenVertexArrays(1, &roomVAO);
    glBindVertexArray(roomVAO);
    
    glGenBuffers(1, &roomVBO);
    glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roomVertices), roomVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);  
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); 
    glEnableVertexAttribArray(9);
    #pragma endregion

    #pragma region Init and Load Texture
    unsigned int TexBufferA;
    unsigned int TexBufferB;
    TexBufferA = LoadImageToGPU("picSource/grid.jpg", GL_RGB, GL_RGB, 0);
    TexBufferB = LoadImageToGPU("picSource/container.jpg", GL_RGB, GL_RGB, 3);
    #pragma endregion  

    #pragma region Init Camera
    glm::vec3 position = { -4.0f, 1.0f, -4.0f }; //-4.0f, 3.0f, -4.0f
    glm::vec3 worldup = { 0.0f, 1.0f, 0.0f };
    Camera camera(position, glm::radians(0.0f), glm::radians(0.0f), worldup);
    Camera camera2(glm::vec3(0.0f, 4.9f, 0.0f), glm::radians(-90.0f), glm::radians(0.0f), worldup); // 頂視圖
    Camera camera3(glm::vec3(-4.9f, -1.2f, 0.0f), glm::radians(0.0f), glm::radians(0.0f), { 0.0f, -1.0f, 0.0f }); // 左視圖
    Camera camera4(glm::vec3(0.0f, -1.2f, -4.9f), glm::radians(0.0f), glm::radians(90.0f), { 0.0f, -1.0f, 0.0f }); // 前視圖
    #pragma endregion

    #pragma region Prepare MVP(model view proj) Matrices
    glm::mat4 viewMat = glm::mat4(1.0f);
    glm::mat4 viewMat2 = glm::mat4(1.0f);
    glm::mat4 viewMat3 = glm::mat4(1.0f);
    glm::mat4 viewMat4 = glm::mat4(1.0f);
    viewMat = camera.GetViewMatrix();
    viewMat2 = camera2.GetViewMatrix();
    viewMat3 = camera3.GetViewMatrix();
    viewMat4 = camera4.GetViewMatrix();

    glm::mat4 modelMat = glm::mat4(1.0f);
    //modelMat = glm::rotate(modelMat, glm::radians(45.0f), glm::vec3(0.0f, 0.5f, 1.0f));

    glm::mat4 projMat = glm::mat4(1.0f);
    // 透視投影（FOV 45 度，寬高比 1600/1200，近裁剪面 0.1，遠裁剪面 100）
    projMat = glm::perspective(glm::radians(60.0f), 1600.0f / 1200.0f, 0.1f, 100.0f);

    // 正交投影（其他三個攝影機）
    glm::mat4 orthoProjMat = glm::mat4(1.0f);
    float aspectRatio = 800.0f / 600.0f; // 視口寬高比
    float width = 5.0f; // 房間一半寬度
    float height = width / aspectRatio; // 根據比例計算高度
    orthoProjMat = glm::ortho(-width, width, -height, height, 0.1f, 100.0f);
    #pragma endregion
    
    // Time initialization
    lastFrame = glfwGetTime();
    
    InitializeBalls(currentBalls, VAO, vertexCount);

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);
            
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        #pragma region ImGui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::SetWindowPos(ImVec2(10, 10));
        ImGui::SetWindowSize(ImVec2(300, 400));

        // 相機控制（僅控制主攝影機）
        ImGui::Text("Adjust Main Camera Position (Top-Left View)");
        ImGui::SliderFloat3("Camera Position", &camera.Position[0], -10.0f, 10.0f);
        viewMat = camera.GetViewMatrix();

        ImGui::Text("Adjust Camera Pitch and Yaw");
        float pitch_deg = glm::degrees(camera.Pitch);
        float yaw_deg = glm::degrees(camera.Yaw);
        bool camera_updated = false;
        if (ImGui::SliderFloat("Pitch", &pitch_deg, -89.0f, 89.0f)) {
            camera.Pitch = glm::radians(pitch_deg);
            camera_updated = true;
        }
        if (ImGui::SliderFloat("Yaw", &yaw_deg, -180.0f, 180.0f)) {
            camera.Yaw = glm::radians(yaw_deg);
            camera_updated = true;
        }
        if (camera_updated) {
            camera.UpdateCameraVectors();
            viewMat = camera.GetViewMatrix();
        }

        ImGui::Separator();
        ImGui::Text("Light Controls");
        ImGui::Checkbox("Light 1 Enabled", &light1Enabled);
        ImGui::Checkbox("Light 2 Enabled", &light2Enabled);

        // 物理控制
        ImGui::Separator();
        ImGui::Text("Physics Controls");
        ImGui::Checkbox("Pause Physics", &pausePhysics);
        
        if (ImGui::SliderFloat("Gravity", &gravityStrength, 0.0f, 20.0f)) {
            for (auto ball : balls) {
                ball->SetGravity(-gravityStrength);
            }
        }
        
        // 球數量控制
        int oldBallCount = currentBalls;
        if (ImGui::SliderInt("Ball Count", &currentBalls, 1, maxBalls)) {
            if (oldBallCount != currentBalls) {
                InitializeBalls(currentBalls, VAO, vertexCount);
            }
        }
        
        if (ImGui::Button("Reset Balls")) {
            InitializeBalls(currentBalls, VAO, vertexCount);
        }
        
        if (!balls.empty()) {
            glm::vec3 vel = balls[0]->GetVelocity();
            glm::vec3 pos = balls[0]->GetPosition();
            ImGui::Text("Ball 1 Position: (%.6f, %.6f, %.6f)", pos.x, pos.y, pos.z); // 使用 %.6f
            ImGui::Text("Ball 1 Velocity: (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
        }
        
        ImGui::Text("Camera Pitch: %.2f degrees", glm::degrees(camera.Pitch));
        ImGui::Text("Camera Yaw: %.2f degrees", glm::degrees(camera.Yaw));
        
        ImGui::End();
        #pragma endregion
    
        // 啟用剪裁測試
        glEnable(GL_SCISSOR_TEST);

        // 視口 1：左上（主攝影機，使用透視投影）
        glViewport(0, 600, 800, 600);
        glScissor(0, 600, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat)); // 透視投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        // 渲染迷你房間
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, TexBufferB);
        glUniform1i(glGetUniformLocation(myShader->ID, "miniTex"), 3);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        // 迷你房間 1
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[0]);
        modelMat = glm::scale(modelMat, miniRoomScales[0]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 2
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[1]);
        modelMat = glm::scale(modelMat, miniRoomScales[1]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 3
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[2]);
        modelMat = glm::scale(modelMat, miniRoomScales[2]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        for (auto ball : balls) {
            ball->Render(myShader, viewMat, projMat, camera.Position);
        }

        // 視口 2：右上（頂視圖，使用正交投影）
        glViewport(800, 600, 800, 600);
        glScissor(800, 600, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat2));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(orthoProjMat)); // 正交投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera2.Position.x, camera2.Position.y, camera2.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        // 渲染迷你房間
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, TexBufferB);
        glUniform1i(glGetUniformLocation(myShader->ID, "miniTex"), 3);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        // 迷你房間 1
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[0]);
        modelMat = glm::scale(modelMat, miniRoomScales[0]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 2
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[1]);
        modelMat = glm::scale(modelMat, miniRoomScales[1]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 3
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[2]);
        modelMat = glm::scale(modelMat, miniRoomScales[2]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        for (auto ball : balls) {
            ball->Render(myShader, viewMat2, orthoProjMat, camera2.Position);
        }

        // 視口 3：左下（左視圖，使用正交投影）
        glViewport(0, 0, 800, 600);
        glScissor(0, 0, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat3));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(orthoProjMat)); // 正交投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera3.Position.x, camera3.Position.y, camera3.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        // 渲染迷你房間
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, TexBufferB);
        glUniform1i(glGetUniformLocation(myShader->ID, "miniTex"), 3);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        // 迷你房間 1
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[0]);
        modelMat = glm::scale(modelMat, miniRoomScales[0]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 2
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[1]);
        modelMat = glm::scale(modelMat, miniRoomScales[1]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 3
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[2]);
        modelMat = glm::scale(modelMat, miniRoomScales[2]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        for (auto ball : balls) {
            ball->Render(myShader, viewMat3, orthoProjMat, camera3.Position);
        }

        // 視口 4：右下（前視圖，使用正交投影）
        glViewport(800, 0, 800, 600);
        glScissor(800, 0, 800, 600);
        glClear(GL_DEPTH_BUFFER_BIT);
        #pragma region Create room
        modelMat = glm::mat4(1.0f);

        myShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexBufferA);
        glUniform1i(glGetUniformLocation(myShader->ID, "roomTex"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);

        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat4));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(orthoProjMat)); // 正交投影

        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera4.Position.x, camera4.Position.y, camera4.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion

        // 渲染迷你房間
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, TexBufferB);
        glUniform1i(glGetUniformLocation(myShader->ID, "miniTex"), 3);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        // 迷你房間 1
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[0]);
        modelMat = glm::scale(modelMat, miniRoomScales[0]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 2
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[1]);
        modelMat = glm::scale(modelMat, miniRoomScales[1]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 迷你房間 3
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, miniRoomPositions[2]);
        modelMat = glm::scale(modelMat, miniRoomScales[2]);
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);

        for (auto ball : balls) {
            ball->Render(myShader, viewMat4, orthoProjMat, camera4.Position);
        }

        // 禁用剪裁測試
        glDisable(GL_SCISSOR_TEST);

        if (!pausePhysics) {
            for (auto ball : balls) {
                ball->Update(deltaTime, roomAABB, miniRoomAABBs);
            }
            
            for (size_t i = 0; i < balls.size(); i++) {
                for (size_t j = i + 1; j < balls.size(); j++) {
                    glm::vec3 pos1 = balls[i]->GetPosition();
                    glm::vec3 pos2 = balls[j]->GetPosition();
                    float radius1 = balls[i]->GetScale();
                    float radius2 = balls[j]->GetScale();
                    
                    if (AABB::SphereToSphere(pos1, radius1, pos2, radius2)) {
                        ResolveSphereCollision(balls[i], balls[j]);
                    }
                }
            }
        }
        
        // 渲染所有球
        for (auto ball : balls) {
            ball->Render(myShader, viewMat, projMat, camera.Position);
        }

        // 檢查 OpenGL 錯誤
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error: " << err << std::endl;
        }
        
        #pragma region Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理
    for (auto ball : balls) {
        delete ball;
    }
    balls.clear();

    //Exit program
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}