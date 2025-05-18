// PhysicsManager.h
#pragma once

#include "GameObject.h"
#include <vector>
#include <glm/glm.hpp>
#include "AABB.h"
#include "OBB.h"

// Define Room Boundaries (simple planes for now)
struct Plane {
    glm::vec3 normal;
    float distance; // Distance from origin along the normal
    Plane(glm::vec3 n, float d) : normal(glm::normalize(n)), distance(d) {}
};


class PhysicsManager {
public:
    std::vector<GameObject*> physicsObjects;
    glm::vec3 gravity;
    std::vector<Plane> roomPlanes;


    PhysicsManager();
    ~PhysicsManager();

    void addObject(GameObject* obj);
    void removeObject(GameObject* obj);

    void update(float deltaTime);

    void applyExternalForce(GameObject* obj, const glm::vec3& forceMagnitude, const glm::vec3& applicationPointWorld);
    // Or, if force application point is relative to object's CoG (local space):
    void applyExternalForceLocal(GameObject* obj, const glm::vec3& forceMagnitude, const glm::vec3& applicationPointLocal);


private:
    void integrate(GameObject* obj, float deltaTime);
    void applyForces(GameObject* obj);

    void checkCollisions();
    void checkObjectEnvironmentCollisions(GameObject* obj);
    // 碰撞檢測輔助函數，現在接收 AABB* 或 OBB*
    bool checkAABBPlane(const AABB* aabb, const Plane& plane, float& penetrationDepth, glm::vec3& collisionNormal);
    bool checkOBBPlane(const OBB* obb, const Plane& plane, float& penetrationDepth, glm::vec3& collisionNormal);
    
    void checkObjectObjectCollisions(GameObject* obj1, GameObject* obj2);
    bool checkAABBAABB(const AABB* aabb1, const AABB* aabb2, glm::vec3& collisionNormal, float& penetrationDepth);
    bool checkAABBOBB(const AABB* aabb, const OBB* obb, glm::vec3& collisionNormal, float& penetrationDepth);
    bool checkOBBOBB(const OBB* obb1, const OBB* obb2, glm::vec3& collisionNormal, float& penetrationDepth);

    // 碰撞響應函數不變 (resolveCollision)
    void resolveCollision(GameObject* obj, const Plane& plane);
    void resolveCollision(GameObject* obj1, GameObject* obj2, const glm::vec3& contactNormal, float penetrationDepth);
};