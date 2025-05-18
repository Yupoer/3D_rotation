// PhysicsIrregularObject.h
#pragma once
#include "GameObject.h"
#include "OBB.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

// 包含 irregular.h 以直接存取其全域變數
#include "irregular.h" // 假設 irregular.h 在 include 路徑中

class PhysicsIrregularObject : public GameObject {
public:
    OBB* boundingBoxOBB; 

    PhysicsIrregularObject(std::string id_param,
                           OBB* obb_ptr,
                           // 假設 irregular.h 提供質心、幾何中心和 OBB 半長度
                           float mass_param = 2.0f,
                           float elasticity_param = 0.4f);
    ~PhysicsIrregularObject();
    void internalUpdate(float deltaTime) override;

private:
    // glm::vec3 initialGeometricCenter;
    // glm::vec3 initialCenterOfMass;
    glm::vec3 obbInitialHalfExtents; // 從 irregular.h 讀取
};