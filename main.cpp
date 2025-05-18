#include <iostream>
#define STB_IMAGE_IMPLEMENTATION 
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.h" // 假設您的 Shader class 有 setInt, setVec3, setMat4 等方法
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
#include "irregular.h"  
#include "room.h"       // 假設 room.h 包含 roomVertices
#include "AABB.h"
#include "OBB.h"
#include "GameObject.h" 
#include "PhysicsBall.h" 
#include "PhysicsIrregularObject.h" 
#include "PhysicsManager.h"

#include <vector> // 確保包含 vector
#include <chrono> // 用於精確計時和睡眠 (FPS 限制)
#include <thread> // 用於 std::this_thread::sleep_for (FPS 限制)

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

// Physics controls for GUI
bool pausePhysics = false; // 由 ImGui 控制
// float gravityStrength = 9.8f; // 如果需要可調重力，可以在 PhysicsManager 或 ImGui 中處理
bool resetBallAndIrr = false; // 用於 ImGui 按鈕觸發重置
bool externalForceApplied = false; // 跟蹤外部力是否已施加

bool light1Enabled = true; 
bool light2Enabled = true; 

// FPS 限制相關
const float TARGET_FPS = 30.0f;
const float TARGET_FRAME_TIME_SECONDS = 1.0f / TARGET_FPS;


// Room AABB 初始化
AABB roomAABB(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 10.0f, 10.0f)); 
// 全域物理物件和碰撞體實例
AABB global_ball_aabb_instance; 
OBB  global_irr_obb_instance;   
PhysicsManager physicsManager_instance; 

PhysicsBall* actual_phys_ball = nullptr;
PhysicsIrregularObject* actual_phys_irr = nullptr;

// 輔助函數：將 C 風格 float 陣列轉換為 glm::vec3 (來自 PhysicsBall.cpp)
glm::vec3 cArrayToVec3_main(const float* arr) {
    if (arr) {
        return glm::vec3(arr[0], arr[1], arr[2]);
    }
    return glm::vec3(0.0f);
}


#pragma region Helper Function to Create VAO and VBO
void setupModelBuffers(unsigned int& VAO, unsigned int& VBO, const float* vertices_data, int vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount * 8, vertices_data, GL_STATIC_DRAW);
    
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


    // 創建物理物件
    actual_phys_ball = new PhysicsBall("ball_01", &global_ball_aabb_instance, 1.0f, 0.8f);
    physicsManager_instance.addObject(actual_phys_ball);

    actual_phys_irr = new PhysicsIrregularObject("irr_01", &global_irr_obb_instance, 2.0f, 0.4f);
    physicsManager_instance.addObject(actual_phys_irr);

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

    
    // 時間相關變數
    float lastFrameTimestamp = static_cast<float>(glfwGetTime());
    float deltaTime = 0.0f; // 將在迴圈中計算
    
    // Removed ball initialization

    while (!glfwWindowShouldClose(window)) {
        float currentFrameTimestamp = static_cast<float>(glfwGetTime());
        deltaTime = currentFrameTimestamp - lastFrameTimestamp;

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

        if (ImGui::CollapsingHeader("Camera Controls")) {
            ImGui::Text("Adjust Camera Position");
            ImGui::SliderFloat3("##CamPos", &camera.Position[0], -30.0f, 30.0f); // 增加範圍
            // viewMat = camera.GetViewMatrix(); // 在渲染前統一獲取

            ImGui::Text("Adjust Camera Pitch and Yaw");
            float pitch_deg = glm::degrees(camera.Pitch);
            float yaw_deg = glm::degrees(camera.Yaw);
            bool camera_updated = false;
            if (ImGui::SliderFloat("Pitch", &pitch_deg, -89.0f, 89.0f)) {
                camera.Pitch = glm::radians(pitch_deg);
                camera_updated = true;
            }
            if (ImGui::SliderFloat("Yaw", &yaw_deg, -360.0f, 360.0f)) { // 擴大 Yaw 範圍
                camera.Yaw = glm::radians(yaw_deg);
                camera_updated = true;
            }
            if (camera_updated) {
                camera.UpdateCameraVectors();
            }
            ImGui::Text("Front: %.2f, %.2f, %.2f", camera.Forward.x, camera.Forward.y, camera.Forward.z);
        }
        
        if (ImGui::CollapsingHeader("Light Controls")) {
            ImGui::Checkbox("Light 1 Enabled", &light1Enabled);
            ImGui::Checkbox("Light 2 Enabled", &light2Enabled);
        }

        if (ImGui::CollapsingHeader("Physics Controls")) {
            ImGui::Checkbox("Pause Physics", &pausePhysics); 
            if (ImGui::Button("Reset Objects & Force")) {
                if (actual_phys_ball) {
                    actual_phys_ball->position = cArrayToVec3_main(ballCenterMass); 
                    actual_phys_ball->orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    actual_phys_ball->linearVelocity = glm::vec3(0.0f);
                    actual_phys_ball->angularVelocity = glm::vec3(0.0f);
                    actual_phys_ball->internalUpdate(0.0f); 
                }
                if (actual_phys_irr) {                
                    glm::vec3 irr_initial_com = glm::vec3(2.0f, 1.0f, 5.0f); // 示例值，應從數據或計算得到
                    if (irregularCount > 0) { // 從 irregularVertices 計算幾何中心作為近似質心
                        glm::vec3 sum_v(0.0f);
                        for(int i=0; i<irregularCount*8; i+=8) sum_v += glm::vec3(irregularVertices[i], irregularVertices[i+1], irregularVertices[i+2]);
                        irr_initial_com = sum_v / (float)irregularCount;
                    }
                    actual_phys_irr->position = irr_initial_com;

                    actual_phys_irr->orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    actual_phys_irr->linearVelocity = glm::vec3(0.0f);
                    actual_phys_irr->angularVelocity = glm::vec3(0.0f);
                    actual_phys_irr->internalUpdate(0.0f);
                }
                externalForceApplied = false; 
                std::cout << "Objects and force state reset." << std::endl;
            }
            ImGui::Text("Press SPACE to apply jump force.");
            ImGui::Text("Press N to allow re-applying jump force.");
        }
        
        ImGui::End();
        #pragma endregion
    
        // 設置視口為整個窗口
        glm::mat4 viewMat = camera.GetViewMatrix();
        glm::mat4 projMat = glm::perspective(glm::radians(45.0f), 1600.0f / 1200.0f, 0.1f, 100.0f);
        
        // 物理更新
        if (!pausePhysics) {
            if (!externalForceApplied && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                if (actual_phys_ball) {
                    glm::vec3 ballForceAppPointLocal(0.1f, -actual_phys_ball->actualRadius * 0.8f , 0.0f); // 施力點在底部附近
                    physicsManager_instance.applyExternalForceLocal(actual_phys_ball, glm::vec3(0.0f, 20.0f, 0.0f), ballForceAppPointLocal); // 調整力的大小
                     std::cout << "Applied force to ball. Radius: " << actual_phys_ball->actualRadius << std::endl;
                }
                if (actual_phys_irr && actual_phys_irr->boundingBoxOBB) { // 確保 OBB 有效
                    glm::vec3 irrForceAppPointLocal(actual_phys_irr->boundingBoxOBB->extents.x * 0.3f, -actual_phys_irr->boundingBoxOBB->extents.y * 0.8f, 0.0f);
                    physicsManager_instance.applyExternalForceLocal(actual_phys_irr, glm::vec3(0.0f, 25.0f, 0.0f), irrForceAppPointLocal);
                     std::cout << "Applied force to irregular. ExtentsY: " << actual_phys_irr->boundingBoxOBB->extents.y << std::endl;
                }
                externalForceApplied = true; 
            }
            if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) { // 允許再次施力
                 externalForceApplied = false;
                 std::cout << "Force application re-enabled." << std::endl;
            }
            physicsManager_instance.update(deltaTime); // 使用實際的 deltaTime
        }

        myShader->use();
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(viewMat));
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(projMat));
        glUniform3f(glGetUniformLocation(myShader->ID, "cameraPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniform1i(glGetUniformLocation(myShader->ID, "light1Enabled"), light1Enabled);
        glUniform1i(glGetUniformLocation(myShader->ID, "light2Enabled"), light2Enabled);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos"), 0.0f, 10.0f, 0.0f); // 調整光源位置
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(myShader->ID, "lightPos2"), 10.0f, 5.0f, 10.0f); 
        glUniform3f(glGetUniformLocation(myShader->ID, "lightColor2"), 0.5f, 0.5f, 0.9f);
        glUniform3f(glGetUniformLocation(myShader->ID, "ambientColor"), 0.15f, 0.15f, 0.15f);

        #pragma region Create room
        glm::mat4 roomModelMat = glm::mat4(1.0f);
        roomModelMat = glm::translate(roomModelMat, glm::vec3(5.0f, 5.0f, 5.0f)); 
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 1);
        glUniform1i(glGetUniformLocation(myShader->ID, "isbox"), 0); 
        glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(roomModelMat));
        
        glActiveTexture(GL_TEXTURE0); 
        glBindTexture(GL_TEXTURE_2D, TexBufferA); 
        glUniform1i(glGetUniformLocation(myShader->ID, "diffuseTexture"), 0); 
        glUniform1i(glGetUniformLocation(myShader->ID, "useTexture"), 1); 
        glBindVertexArray(roomVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        #pragma endregion
        
        glUniform1i(glGetUniformLocation(myShader->ID, "isRoom"), 0);  
        glUniform1i(glGetUniformLocation(myShader->ID, "diffuseTexture"), 0);

        #pragma region Draw Irregular Object
        if (actual_phys_irr) { 
            glUniform3fv(glGetUniformLocation(myShader->ID, "objColor"), 1, glm::value_ptr(actual_phys_irr->color)); 
            glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(actual_phys_irr->getModelMatrix()));
            glBindVertexArray(irregularVAO);
            glDrawArrays(GL_TRIANGLES, 0, irregularCount); 
        }
        #pragma endregion

        #pragma region Draw Ball Object
        if (actual_phys_ball) { 
            glUniform3fv(glGetUniformLocation(myShader->ID, "objColor"), 1, glm::value_ptr(actual_phys_ball->color)); 
            glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(actual_phys_ball->getModelMatrix()));
            glBindVertexArray(ballVAO);
            glDrawArrays(GL_TRIANGLES, 0, ballCount); 
        }
        #pragma endregion
        
        glBindVertexArray(0);
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL Error in main loop: " << err << std::endl;
        }
        
        #pragma region Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        #pragma endregion

        glfwSwapBuffers(window);
        glfwPollEvents();

        float frameProcessingTime = static_cast<float>(glfwGetTime()) - currentFrameTimestamp;
        if (frameProcessingTime < TARGET_FRAME_TIME_SECONDS) {
            // 使用 chrono 進行更精確的睡眠
            std::chrono::duration<double> sleep_duration_seconds(TARGET_FRAME_TIME_SECONDS - frameProcessingTime);
            // 轉換為微秒，因為 sleep_for 通常接受整數微秒或毫秒
            auto sleep_duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration_seconds);
            if (sleep_duration_microseconds.count() > 0) { // 只有當需要睡眠時才執行
                 std::this_thread::sleep_for(sleep_duration_microseconds);
            }
        }
        lastFrameTimestamp = static_cast<float>(glfwGetTime());
    }

    //Exit program
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}