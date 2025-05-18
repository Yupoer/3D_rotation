// PhysicsIrregularObject.cpp
#include "PhysicsIrregularObject.h"
#include <iostream>


// 假設 irregular.h 中有類似以下的定義 (您需要提供 irregular.h 的確切內容)
// const float irregularCenter[] = {x,y,z};
// const float irregularCenterMass[] = {x,y,z};
// const float irregularObbHalfExtents[] = {hx, hy, hz}; // 或者 min/max
// const float irregularVertices[] = {...};
// const int irregularCount = ...;

glm::vec3 cArrayToVec3_irr(const float* arr) { // Helper
    if (arr) return glm::vec3(arr[0], arr[1], arr[2]);
    return glm::vec3(0.0f);
}

PhysicsIrregularObject::PhysicsIrregularObject(
    std::string id_param,
    OBB* obb_ptr,
    float mass_param,
    float elasticity_param)
    : GameObject(id_param, GameObjectType::PHYSICS_IRREGULAR, mass_param, elasticity_param),
      boundingBoxOBB(obb_ptr) {
    this->scale = glm::vec3(0.3f, 0.3f, 0.3f);
    // 為了編譯，使用臨時值 (請用 irregular.h 中的實際值替換)
    glm::vec3 com = glm::vec3(0,0,0);         // 應來自 irregularCenterMass
    glm::vec3 geomCenter = glm::vec3(0,0,0);  // 應來自 irregularCenter
    this->obbInitialHalfExtents = glm::vec3(1,1,1); // 應來自 irregularObbHalfExtents 或計算得到


    this->position = com; // GameObject 的 position 是質心
    this->localCoMToGeometricCenterOffset = geomCenter - com;

    std::cout << "PhysicsIrregularObject '" << this->id << "' created." << std::endl;
    std::cout << "  Initial CoM (pos): " << this->position.x << ", " << this->position.y << ", " << this->position.z << std::endl;
    std::cout << "  Local CoM to Geom Offset: " << this->localCoMToGeometricCenterOffset.x << ", " << this->localCoMToGeometricCenterOffset.y << ", " << this->localCoMToGeometricCenterOffset.z << std::endl;
    std::cout << "  OBB Half Extents: " << this->obbInitialHalfExtents.x << ", " << this->obbInitialHalfExtents.y << ", " << this->obbInitialHalfExtents.z << std::endl;

    // 使用 OBB 的實際維度（完整長度）設定慣性張量
    setInertiaTensorCuboid(this->obbInitialHalfExtents.x * 2.0f, 
                           this->obbInitialHalfExtents.y * 2.0f, 
                           this->obbInitialHalfExtents.z * 2.0f);
    
    if (this->boundingBoxOBB) {
        this->boundingBoxOBB->extents = this->obbInitialHalfExtents;
        // OBB 的中心應該是其幾何中心的世界位置
        // 但 OBB 的 center 成員是其自身的中心，這個中心應與 GameObject 的質心同步
        // extents 和 orientation 描述了它圍繞這個中心的形狀和方向
        this->boundingBoxOBB->update(this->position, this->orientation); 
    }

    this->linearDragCoefficient = 0.2f;
    this->angularDragCoefficient = 0.1f;
    this->color = glm::vec3(1.0f, 0.0f, 0.0f); 
}

PhysicsIrregularObject::~PhysicsIrregularObject() {}

void PhysicsIrregularObject::internalUpdate(float deltaTime) {
    GameObject::internalUpdate(deltaTime);
    if (this->boundingBoxOBB) {
        // OBB 的中心 (this->boundingBoxOBB->center) 應與 GameObject 的質心 (this->position) 同步
        // OBB 的方向 (this->boundingBoxOBB->orientation) 應與 GameObject 的方向 (this->orientation) 同步
        this->boundingBoxOBB->update(this->position, this->orientation);
    }
}