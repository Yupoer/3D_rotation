// PhysicsBall.cpp
#include "PhysicsBall.h"
#include <iostream> 

std::vector<float> cArrayToVector(const float* arr, int count, int elementsPerVertex = 1) {
    std::vector<float> vec;
    if (arr && count > 0) {
        vec.assign(arr, arr + count * elementsPerVertex); // 假設 ballVertices 是連續的
    }
    return vec;
}

// 輔助函數：將 C 風格陣列 (大小為3) 轉換為 glm::vec3
glm::vec3 cArrayToVec3(const float* arr) {
    if (arr) {
        return glm::vec3(arr[0], arr[1], arr[2]);
    }
    return glm::vec3(0.0f);
}


PhysicsBall::PhysicsBall(std::string id_param,
                         AABB* aabb_ptr,
                         float mass_param,
                         float elasticity_param)
    : GameObject(id_param, GameObjectType::PHYSICS_BALL, mass_param, elasticity_param),
      boundingBoxAABB(aabb_ptr) {
    this->scale = glm::vec3(0.3f, 0.3f, 0.3f);
    // 1. 從 ball.h 讀取數據
    initialGeometricCenter = cArrayToVec3(ballCenter); // ball.h 中的幾何中心
    initialCenterOfMass = cArrayToVec3(ballCenterMass); // ball.h 中的質心

    // 設定 GameObject 的 position 為質心
    this->position = initialCenterOfMass; 

    
    this->localCoMToGeometricCenterOffset = initialGeometricCenter - initialCenterOfMass;


    glm::vec3 minP = cArrayToVec3(ballMin);
    glm::vec3 maxP = cArrayToVec3(ballMax);
    this->actualRadius = glm::length(maxP - minP) / 2.0f; 
    // 或者，如果 ball.h 中有一個 ballActualRadius，直接使用它。
    // 例如: this->actualRadius = some_global_ball_radius_from_ball_h;

    std::cout << "PhysicsBall '" << this->id << "' created." << std::endl;
    std::cout << "  Initial CoM (pos): " << this->position.x << ", " << this->position.y << ", " << this->position.z << std::endl;
    std::cout << "  Local CoM to Geom Offset: " << this->localCoMToGeometricCenterOffset.x << ", " << this->localCoMToGeometricCenterOffset.y << ", " << this->localCoMToGeometricCenterOffset.z << std::endl;
    std::cout << "  Calculated Radius: " << this->actualRadius << std::endl;


    setInertiaTensorSphere(this->actualRadius);
    if (this->boundingBoxAABB) {
        glm::vec3 aabbHalfExtents(this->actualRadius, this->actualRadius, this->actualRadius);
        // AABB 的中心應該是幾何中心的世界位置
        glm::vec3 geometricCenterWorld = this->position + this->localCoMToGeometricCenterOffset; // 錯誤：偏移應該在旋轉後應用
                                                                                                // 或者，AABB 應該包圍整個旋轉後的物體，並以質心為中心。
                                                                                                // 為了簡單起見，讓 AABB 以質心為中心，並有足夠大的 halfExtents
        this->boundingBoxAABB->updateWithNewWorldCenter(this->position, aabbHalfExtents);
    }

    this->linearDragCoefficient = 0.05f;
    this->angularDragCoefficient = 0.02f;
    this->color = glm::vec3(1.0f, 0.0f, 0.0f); 

    // 渲染頂點：直接使用 ball.h 中的全域 ballVertices 陣列來進行渲染設置 (在 main.cpp 中)
    // PhysicsBall 內部不需要儲存 renderVertices 的拷貝，除非需要修改它們
    // 如果需要，可以這樣複製：
    // this->renderVertices = cArrayToVector(ballVertices, ballCount, 8); // 8 floats per vertex
}

PhysicsBall::~PhysicsBall() {}

void PhysicsBall::internalUpdate(float deltaTime) {
    GameObject::internalUpdate(deltaTime); 
    if (this->boundingBoxAABB) {
        // AABB 應以質心為中心，其大小應能包圍整個球體
        glm::vec3 aabbHalfExtents(this->actualRadius, this->actualRadius, this->actualRadius);
        this->boundingBoxAABB->updateWithNewWorldCenter(this->position, aabbHalfExtents);
    }
}