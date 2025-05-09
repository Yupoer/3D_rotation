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
#include "ball.h"
#include "AABB.h"
#include "vector"
#include "irregular.h"
#include "room.h"



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

// Removed ball and mini room related variables
bool light1Enabled = true; // 第一個光源開關
bool light2Enabled = true; // 第二個光源開關

#pragma region Helper Function to Create VAO and VBO
void setupModelBuffers(unsigned int& VAO, unsigned int& VBO, const float* vertices, int vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertexCount * 8, vertices, GL_STATIC_DRAW);
    
    // 位置屬性
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(6);
    
    // 紋理座標屬性
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(8);
    
    // 法線屬性
    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(9);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
#pragma endregion

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
    
    // Room AABB 初始化
    AABB roomAABB(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f)); 

    #pragma region Init and load Model to VAO & VBO
    // room VAO & VBO
    unsigned int roomVAO, roomVBO;
    setupModelBuffers(roomVAO, roomVBO, roomVertices, sizeof(roomVertices) / sizeof(roomVertices[0]));
    
    // 為 irregularVertices 物件創建 VAO 和 VBO
    unsigned int irregularVAO, irregularVBO;
    setupModelBuffers(irregularVAO, irregularVBO, irregularVertices, irregularCount);
    
    // 為 ball 物件創建 VAO 和 VBO
    unsigned int ballVAO, ballVBO;
    setupModelBuffers(ballVAO, ballVBO, ballVertices, ballCount);
    #pragma endregion

    #pragma region Init and Load Texture
    unsigned int TexBufferA;
    unsigned int TexBufferB;
    TexBufferA = LoadImageToGPU("picSource/grid.jpg", GL_RGB, GL_RGB, 0);
    TexBufferB = LoadImageToGPU("picSource/container.jpg", GL_RGB, GL_RGB, 3);
    #pragma endregion  

    #pragma region Init Camera
    glm::vec3 position = { 1.0f, 9.0f, 1.0f };
    glm::vec3 worldup = { 0.0f, 1.0f, 0.0f };
    Camera camera(position, glm::radians(0.0f), glm::radians(0.0f), worldup);
    #pragma endregion

    #pragma region Prepare MVP(model view proj) Matrices
    glm::mat4 viewMat = glm::mat4(1.0f);
    viewMat = camera.GetViewMatrix();

    glm::mat4 modelMat = glm::mat4(1.0f);
    //modelMat = glm::rotate(modelMat, glm::radians(45.0f), glm::vec3(0.0f, 0.5f, 1.0f));

    glm::mat4 projMat = glm::mat4(1.0f);
    // 透視投影（FOV 45 度，寬高比 1600/1200，近裁剪面 0.1，遠裁剪面 100）
    projMat = glm::perspective(glm::radians(60.0f), 1600.0f / 1200.0f, 0.1f, 100.0f);
    #pragma endregion
    
    // Time initialization
    lastFrame = glfwGetTime();
    
    // Removed ball initialization

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window);
            
        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
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

        // Removed physics controls
        
        ImGui::Text("Camera Pitch: %.2f degrees", glm::degrees(camera.Pitch));
        ImGui::Text("Camera Yaw: %.2f degrees", glm::degrees(camera.Yaw));
        
        ImGui::End();
        #pragma endregion
    
        // 設置視口為整個窗口
        glViewport(0, 0, 1600, 1200);
        
        #pragma region Create room
        modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(5.0f, 5.0f, 5.0f)); // 將房間中心從(0,0,0)移動到(5,5,5)，使範圍為(0,0,0)到(10,10,10)

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

        #pragma region Draw Irregular Object
        myShader->use();
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.8f, 0.2f, 0.2f); // 紅色
        glm::mat4 irregularModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 2.0f, 5.0f)); // 放置在房間的新底部中心
        irregularModelMat = glm::rotate(irregularModelMat, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // 繞 X 軸旋轉 180 度
        irregularModelMat = glm::scale(irregularModelMat, glm::vec3(0.1f, 0.1f, 0.1f)); // 縮小到原本尺寸的50%
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(irregularModelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(irregularVAO);
        glDrawArrays(GL_TRIANGLES, 0, irregularCount * 3);
        #pragma endregion

        #pragma region Draw Ball Object
        myShader->use();
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0);
        glUniform3f(glGetUniformLocation(myShader->ID, "objColor"), 0.2f, 0.8f, 0.2f); // 綠色
        glm::mat4 ballModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, 4.0f, 7.0f)); // 放置在房間的新底部，稍微偏移
        ballModelMat = glm::scale(ballModelMat, glm::vec3(0.3f, 0.3f, 0.3f)); // 縮小到原本尺寸的30%
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(ballModelMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);

        glBindVertexArray(ballVAO);
        glDrawArrays(GL_TRIANGLES, 0, ballCount * 3);
        #pragma endregion
        
        glBindVertexArray(0);
        
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

    //Exit program
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}