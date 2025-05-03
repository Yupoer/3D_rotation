#include "DrawBall.h"
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>    
#include <GLFW/glfw3.h> 
#include "AABB.h"
#include <iostream>


DrawBall::DrawBall(GLuint VAO, int vertexCount)
    : VAO(VAO), vertexCount(vertexCount),
      position(0.0f), velocity(0.0f), acceleration(0.0f),
      scale(0.03f), gravity(-9.8f),
      color(0.93f, 0.16f, 0.16f),
      defaultColor(0.93f, 0.16f, 0.16f), 
      colorChangeTimer(0.0f), // 初始化計時器
      // init AABB
      boundingBox(AABB::FromSphere(position, scale)),
      isStationary(false){
}

DrawBall::~DrawBall() {
}

void DrawBall::UpdateAABB() {
    // update AABB
    boundingBox = AABB::FromSphere(position, scale);
}


void DrawBall::Update(float deltaTime, const AABB& roomAABB, const std::vector<AABB>& miniRoomAABBs) {
    if (isStationary) {
        return;
    }

    // 應用重力
    acceleration.y = gravity; // gravity = -9.8f

    // 更新速度
    velocity += acceleration * deltaTime;

    // 限制最大速度
    const float maxVelocity = 100.0f;
    if (glm::length(velocity) > maxVelocity) {
        velocity = glm::normalize(velocity) * maxVelocity;
    }

    // 使用子步更新位置
    const int subSteps = 5;
    for (int step = 0; step < subSteps; step++) {
        float subDeltaTime = deltaTime / static_cast<float>(subSteps);
        glm::vec3 newPosition = position + velocity * subDeltaTime;

        // 碰撞檢測與響應
        float restitution = 0.8f; // 彈性係數
        bool collided = false;

        // 1. 處理 roomAABB 碰撞（內部反彈）
        for (int i = 0; i < 3; ++i) {
            float minBound = roomAABB.GetMin()[i] + scale;
            float maxBound = roomAABB.GetMax()[i] - scale;

            // 檢查下界碰撞（房間內部）
            if (newPosition[i] < minBound) {
                newPosition[i] = minBound;
                if (velocity[i] < 0) {
                    velocity[i] = -velocity[i] * restitution;
                    // 避免 Y 軸地板抽動
                    if (i == 1 && std::abs(velocity[i]) < 0.1f) {
                        velocity[i] = 0.0f; // 停止 Y 軸運動
                        isStationary = true; // 標記為靜止
                    }
                }
                collided = true;
            }
            // 檢查上界碰撞（房間內部）
            else if (newPosition[i] > maxBound) {
                newPosition[i] = maxBound;
                if (velocity[i] > 0) velocity[i] = -velocity[i] * restitution;
                collided = true;
            }
        }

        // 2. 處理 miniRoomAABBs 碰撞（外部反彈）
        for (const auto& miniRoomAABB : miniRoomAABBs) {
            for (int i = 0; i < 3; ++i) {
                float minBound = miniRoomAABB.GetMin()[i] - scale; // 外部下界
                float maxBound = miniRoomAABB.GetMax()[i] + scale; // 外部上界

                // 獲取其他兩個軸的索引
                int j = (i + 1) % 3; // 第二個軸
                int k = (i + 2) % 3; // 第三個軸

                // 其他兩個軸的範圍檢查（更嚴格，僅考慮 AABB 範圍）
                bool inRange = newPosition[j] >= miniRoomAABB.GetMin()[j] &&
                               newPosition[j] <= miniRoomAABB.GetMax()[j] &&
                               newPosition[k] >= miniRoomAABB.GetMin()[k] &&
                               newPosition[k] <= miniRoomAABB.GetMax()[k];

                // 檢查下界碰撞（撞到 AABB 的左/下/前表面）
                if (newPosition[i] < minBound && inRange) {
                    newPosition[i] = minBound + 0.001f; // 微調避免卡住
                    if (velocity[i] > 0) velocity[i] = -velocity[i] * restitution; // 速度向正方向
                    collided = true;
                }
                // 檢查上界碰撞（撞到 AABB 的右/上/後表面）
                else if (newPosition[i] > maxBound && inRange) {
                    newPosition[i] = maxBound - 0.001f; // 微調避免卡住
                    if (velocity[i] < 0) velocity[i] = -velocity[i] * restitution; // 速度向負方向
                    collided = true;
                }
            }
        }

        // 碰撞後處理
        if (collided) {
            std::cout << "!!! COLLISION DETECTED !!!\n";
            velocity *= 0.99f; // 速度衰減
            color = glm::vec3(0.0f, 0.0f, 1.0f); // 變為藍色
            colorChangeTimer = colorChangeDuration;
        }

        position = newPosition;

        // 額外限制位置（確保不穿透房間邊界）
        position.x = glm::clamp(position.x, roomAABB.GetMin().x + scale, roomAABB.GetMax().x - scale);
        position.y = glm::clamp(position.y, roomAABB.GetMin().y + scale, roomAABB.GetMax().y - scale);
        position.z = glm::clamp(position.z, roomAABB.GetMin().z + scale, roomAABB.GetMax().z - scale);
    }

    // 更新 AABB
    UpdateAABB();

    // 停止條件
    if (glm::length(velocity) < 0.01f) {
        velocity = glm::vec3(0.0f);
        isStationary = true; // 標記為靜止
    }

    // 更新顏色計時器
    if (colorChangeTimer > 0.0f) {
        colorChangeTimer -= deltaTime;
        if (colorChangeTimer <= 0.0f) {
            color = defaultColor; // 恢復原始顏色
        }
    }
}


void DrawBall::Render(Shader* shader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    glm::mat4 modelMat = glm::mat4(1.0f);
    modelMat = glm::translate(modelMat, position);
    modelMat = glm::scale(modelMat, glm::vec3(scale));

    shader->use();

    glUniform1i(glGetUniformLocation(shader->ID, "isbox"), 0);
    glUniform1i(glGetUniformLocation(shader->ID, "isRoom"), 0);

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "modelMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "viewMat"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projMat"), 1, GL_FALSE, glm::value_ptr(proj));

    glUniform3f(glGetUniformLocation(shader->ID, "objColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(shader->ID, "ambientColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightPos"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightColor"), 0.5f, 0.5f, 0.5f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightPos2"), 0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(shader->ID, "lightColor2"), 0.2f, 0.7f, 0.9f);
    glUniform3f(glGetUniformLocation(shader->ID, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform1i(glGetUniformLocation(shader->ID, "light1Enabled"), light1Enabled); // 傳遞第一個光源開關
    glUniform1i(glGetUniformLocation(shader->ID, "light2Enabled"), light2Enabled); // 傳遞第二個光源開關

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}