#pragma once
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "Shader.h"
#include "AABB.h"


class DrawBall {
private:
    GLuint VAO;
    int vertexCount;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float scale;
    float gravity;
    glm::vec3 color;
    glm::vec3 defaultColor; // 儲存原始顏色
    float colorChangeTimer; // 計時器，用於恢復顏色
    static constexpr float colorChangeDuration = 0.5f; // 顏色改變持續時間（秒）
    AABB boundingBox;
    bool isStationary; // 添加靜止狀態標誌

public:
    DrawBall(GLuint VAO, int vertexCount);
    ~DrawBall();
    void UpdateAABB();
    void Update(float deltaTime, const AABB& roomAABB, const std::vector<AABB>& miniRoomAABBs);
    void Render(Shader* shader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);

    void SetPosition(const glm::vec3& pos) { position = pos; UpdateAABB(); }
    void SetVelocity(const glm::vec3& vel) { velocity = vel; isStationary = false; }
    void SetGravity(float g) { gravity = g; }
    void SetScale(float s) { scale = s; UpdateAABB(); }
    void SetColor(const glm::vec3& c) { color = c; } // 新增設置顏色方法
    glm::vec3 GetPosition() const { return position; }
    glm::vec3 GetVelocity() const { return velocity; }
    float GetScale() const { return scale; }
    glm::vec3 GetColor() const { return color; } // 新增獲取顏色方法
    AABB GetAABB() const { return boundingBox; }
    bool IsStationary() const { return isStationary; } // 檢查是否靜止
};

extern bool light1Enabled; // 第一個光源開關
extern bool light2Enabled; // 第二個光源開關