// PhysicsBall.h
#pragma once
#include "GameObject.h"
#include "AABB.h"
#include <vector>
#include <string>
#include <glm/glm.hpp> // For glm::vec3

// 包含 ball.h 以直接存取其全域變數
#include "ball.h" // 假設 ball.h 在 include 路徑中，或者提供相對路徑

class PhysicsBall : public GameObject {
public:
    AABB* boundingBoxAABB; 
    float actualRadius;    

    PhysicsBall(std::string id_param,
                  AABB* aabb_ptr,
                  // 不再從參數接收頂點和局部中心，直接從 ball.h 讀取
                  float mass_param = 1.0f,
                  float elasticity_param = 0.8f);

    ~PhysicsBall();
    void internalUpdate(float deltaTime) override;
    const std::vector<float>& getRenderVerticesList() const { return renderVertices; } // 提供一個獲取頂點列表的方法

private:
    std::vector<float> renderVertices; // 儲存轉換後供渲染的頂點數據 (如果需要)
                                       // 或者渲染時直接使用 ballVertices
    glm::vec3 initialGeometricCenter; // 從 ballCenter 轉換的 vec3
    glm::vec3 initialCenterOfMass;    // 從 ballCenterMass 轉換的 vec3
};