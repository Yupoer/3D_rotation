// GameObject.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

enum class GameObjectType {
    UNDEFINED,
    PHYSICS_BALL,
    PHYSICS_IRREGULAR
};

class GameObject {
public:
    std::string id;
    GameObjectType objectType;

    // 物理屬性
    glm::vec3 position;         // 物件的質心 (Center of Mass) 在世界座標中的位置
    glm::quat orientation;      // 物件在世界座標中的方向 (旋轉是繞質心)
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;
    glm::vec3 scale;
    float mass;
    glm::mat3 inertiaTensor;        
    glm::mat3 invInertiaTensorBody; 
    glm::mat3 invInertiaTensorWorld; 

    // 新增：從質心到幾何中心/模型原點的局部偏移向量
    // 如果模型的頂點是圍繞其幾何中心定義的，而該幾何中心不是質心，
    // 這個向量就是 CoM_to_GeometricCenter_local = GeometricCenter_local - CoM_local (通常 CoM_local 是 (0,0,0))
    // 在 getModelMatrix() 中使用它來正確渲染。
    glm::vec3 localCoMToGeometricCenterOffset; 

    float restitution;          
    float linearDragCoefficient;
    float angularDragCoefficient;
    bool isStatic;              
    glm::vec3 color;

    GameObject(std::string id_param, GameObjectType type_param, float m = 1.0f, float rest = 0.5f)
        : id(id_param), objectType(type_param), position(0.0f), orientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
          linearVelocity(0.0f), angularVelocity(0.0f), mass(m > 0.0f ? m : 1.0f),
          scale(1.0f, 1.0f, 1.0f),
          localCoMToGeometricCenterOffset(0.0f), // 預設為零，表示質心與幾何中心重合
          restitution(rest), linearDragCoefficient(0.1f), angularDragCoefficient(0.1f),
          isStatic(false), color(0.7f, 0.7f, 0.7f) {
        setInertiaTensorSphere(1.0f); 
    }

    virtual ~GameObject() = default;
    virtual void internalUpdate(float deltaTime) { /* 預留給衍生類別更新碰撞體 */ }

    void setPosition(const glm::vec3& pos) { position = pos; }
    void setOrientation(const glm::quat& orient) {
        orientation = glm::normalize(orient);
        updateInverseInertiaTensorWorld();
    }
    void rotate(const glm::quat& localRotation) {
        orientation = glm::normalize(orientation * localRotation); 
        updateInverseInertiaTensorWorld();
    }
    // ... (setInertiaTensor... 和 updateInverseInertiaTensorWorld 保持不變) ...
    // 範例：設定球體的慣性張量 I = (2/5)mr^2
    void setInertiaTensorSphere(float radius) {
        if (mass > 0 && radius > 0) {
            float I_val = 0.4f * mass * radius * radius;
            setInertiaTensorDiagonal(I_val, I_val, I_val);
        } else {
            setInertiaTensorDiagonal(1.0f, 1.0f, 1.0f); // Fallback
        }
    }

    glm::mat4 getModelMatrix() const {
        glm::mat4 modelToWorldTranslation = glm::translate(glm::mat4(1.0f), position); 
        glm::mat4 worldRotation = glm::mat4_cast(orientation);                         
        glm::mat4 translateToGeomCenterDueToOffset = glm::translate(glm::mat4(1.0f), localCoMToGeometricCenterOffset);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale); // <--- 創建縮放矩陣

        // 套用順序：先縮放（相對於幾何中心/模型原點），然後進行質心到幾何中心的偏移平移，然後旋轉（繞質心），最後平移（質心到世界位置）
        // Model = T_CoM_to_World * R_around_CoM * T_CoM_to_Geom_Local * S_Local
        return modelToWorldTranslation * worldRotation * translateToGeomCenterDueToOffset * scaleMatrix;
    }

    // 範例：設定長方體的慣性張量 (維度 dx, dy, dz 是完整長度)
    void setInertiaTensorCuboid(float full_dx, float full_dy, float full_dz) {
        if (mass > 0) {
            float ix = (1.0f/12.0f) * mass * (full_dy*full_dy + full_dz*full_dz);
            float iy = (1.0f/12.0f) * mass * (full_dx*full_dx + full_dz*full_dz);
            float iz = (1.0f/12.0f) * mass * (full_dx*full_dx + full_dy*full_dy);
            setInertiaTensorDiagonal(ix, iy, iz);
        } else {
            setInertiaTensorDiagonal(1.0f, 1.0f, 1.0f); // Fallback
        }
    }
     void setInertiaTensorDiagonal(float ix, float iy, float iz) { // (保持不變)
        inertiaTensor = glm::mat3(0.0f);
        if (mass > 0) {
            inertiaTensor[0][0] = ix;
            inertiaTensor[1][1] = iy;
            inertiaTensor[2][2] = iz;

            invInertiaTensorBody = glm::mat3(0.0f);
            if (ix != 0.0f) invInertiaTensorBody[0][0] = 1.0f / ix;
            if (iy != 0.0f) invInertiaTensorBody[1][1] = 1.0f / iy;
            if (iz != 0.0f) invInertiaTensorBody[2][2] = 1.0f / iz;
        } else { 
            inertiaTensor = glm::mat3(0.0f);
            invInertiaTensorBody = glm::mat3(0.0f);
        }
        updateInverseInertiaTensorWorld();
    }
     void updateInverseInertiaTensorWorld() { // (保持不變)
        if (mass > 0.0f) {
            glm::mat3 R = glm::mat3_cast(orientation);
            invInertiaTensorWorld = R * invInertiaTensorBody * glm::transpose(R);
        } else {
            invInertiaTensorWorld = glm::mat3(0.0f); 
        }
    }
};