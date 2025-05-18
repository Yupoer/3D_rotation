// PhysicsManager.cpp
#include "PhysicsManager.h"
// #include "ball.h" // 不再直接需要用於類型轉換或 SphereBV
// #include "irregular.h" // 不再直接需要用於類型轉換
#include "PhysicsBall.h" // <--- 新增 (假設檔名已改為 PhysicsBall.h)
#include "PhysicsIrregularObject.h" // <--- 新增
#include "AABB.h" // <--- 確保 AABB 和 OBB 被包含 (即使 PhysicsManager.h 已包含)
#include "OBB.h"  // <---

#include <algorithm>
#include <iostream>
#include <limits> // Required for std::numeric_limits

// Helper for SAT (Separating Axis Theorem) for OBB-OBB
struct Interval {
    float min, max;
};

// overlapOnAxis 函數需要 const OBB&，所以 OBB.h 必須被包含
bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const glm::vec3& axis, Interval& outInterval) {
    // ... (此函數的實作來自您先前的版本，應保持不變，前提是 OBB.h 被正確包含) ...
    // 確保 OBB.h 中有 projectOntoAxis 方法
    float min1, max1, min2, max2;
    obb1.projectOntoAxis(axis, min1, max1);
    obb2.projectOntoAxis(axis, min2, max2);

    if (max1 < min2 || max2 < min1) {
        return false; // No overlap
    } else {
        outInterval.min = glm::max(min1, min2);
        outInterval.max = glm::min(max1, max2);
        return true;
    }
}


PhysicsManager::PhysicsManager() : gravity(0.0f, -9.81f, 0.0f) {
    // Room planes definition (保持不變)
    float roomSize = 10.0f; 
    float halfSize = roomSize / 2.0f;
    roomPlanes.push_back(Plane(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f)); 
    roomPlanes.push_back(Plane(glm::vec3(0.0f, -1.0f, 0.0f), halfSize)); 
    roomPlanes.push_back(Plane(glm::vec3(1.0f, 0.0f, 0.0f), halfSize));  
    roomPlanes.push_back(Plane(glm::vec3(-1.0f, 0.0f, 0.0f), halfSize)); 
    roomPlanes.push_back(Plane(glm::vec3(0.0f, 0.0f, 1.0f), halfSize));  
    roomPlanes.push_back(Plane(glm::vec3(0.0f, 0.0f, -1.0f), halfSize));
}

PhysicsManager::~PhysicsManager() { // (保持不變)
    for (GameObject* obj : physicsObjects) {
        delete obj; 
    }
    physicsObjects.clear();
}

void PhysicsManager::addObject(GameObject* obj) { // (保持不變)
    physicsObjects.push_back(obj);
}

void PhysicsManager::removeObject(GameObject* obj) { // (保持不變)
    physicsObjects.erase(std::remove(physicsObjects.begin(), physicsObjects.end(), obj), physicsObjects.end());
    delete obj; 
}

void PhysicsManager::applyForces(GameObject* obj) { // (邏輯保持不變)
    if (obj->isStatic || obj->mass <= 0) return;
    obj->linearVelocity += gravity; // 簡化 F_gravity = m*g => acc = g
    if (glm::length(obj->linearVelocity) > 0.001f) {
        glm::vec3 dragForceLinear = -obj->linearDragCoefficient * obj->linearVelocity;
        obj->linearVelocity += (dragForceLinear / obj->mass); 
    }
    if (glm::length(obj->angularVelocity) > 0.001f) {
        glm::vec3 dragTorque = -obj->angularDragCoefficient * obj->angularVelocity;
         obj->angularVelocity += obj->invInertiaTensorWorld * dragTorque; 
    }
}

void PhysicsManager::integrate(GameObject* obj, float deltaTime) { // (邏輯保持不變)
    if (obj->isStatic) return;

    obj->position += obj->linearVelocity * deltaTime;

    if (glm::length(obj->angularVelocity) > 0.0001f) {
        glm::quat q_omega(0.0f, obj->angularVelocity.x * deltaTime, obj->angularVelocity.y * deltaTime, obj->angularVelocity.z * deltaTime);
        obj->orientation = glm::normalize(obj->orientation + 0.5f * q_omega * obj->orientation);
    }
    obj->updateInverseInertiaTensorWorld(); 

    // 呼叫物件自身的 internalUpdate 來同步碰撞體
    obj->internalUpdate(deltaTime); // <--- 修改：讓物件自己更新其碰撞體
}

void PhysicsManager::update(float deltaTime) { // (邏輯保持不變)
    for (GameObject* obj : physicsObjects) {
        if (obj->isStatic) continue;
        applyForces(obj);       
        integrate(obj, deltaTime); 
    }
    checkCollisions(); 
}

void PhysicsManager::applyExternalForceLocal(GameObject* obj, const glm::vec3& forceMagnitude, const glm::vec3& applicationPointLocal) { // (邏輯保持不變)
    if (obj->isStatic || obj->mass <= 0.0f) return;
    
    // 線性效果 (假設 forceMagnitude 是 F，在一個 deltaTime 內施加)
    obj->linearVelocity += (forceMagnitude / obj->mass) * 1.0f; // 假設 deltaTime 效果已包含或這是衝量式的改變

    // 旋轉效果 (力矩 T = r x F)
    // applicationPointLocal 是相對於質心的施力點
    glm::vec3 r_local = applicationPointLocal; 
    glm::mat3 R = glm::mat3_cast(obj->orientation);
    glm::vec3 r_world = R * r_local; // 如果需要世界座標的r (但通常力矩計算用局部座標r和世界力F，或世界r和世界F)
                                     // 或者，如果 forceMagnitude 是在世界座標系定義的，但施力點是局部，
                                     // 則 Torque_world = cross(R * r_local, forceMagnitude_world)
                                     // 如果 forceMagnitude 是局部力，Torque_local = cross(r_local, forceMagnitude_local)
                                     // Torque_world = R * Torque_local
    
    // 假設 forceMagnitude 是在世界座標定義的力（例如，總是向上的力）
    // 施力點 applicationPointLocal 相對於質心。
    // Torque_world = cross(R * applicationPointLocal, forceMagnitude_world)
    // obj->angularVelocity += obj->invInertiaTensorWorld * Torque_world * 1.0f;

    // 在您的版本中，您直接用 applicationPointLocal 和 forceMagnitude 計算叉乘，
    // 然後乘以 invInertiaTensorWorld。這暗示 forceMagnitude 是在世界座標系下，
    // 而 applicationPointLocal 被隱式地當作是從質心出發、在世界座標系下描述的向量，
    // 或者是 forceMagnitude 和 applicationPointLocal 的叉乘結果已經是世界座標下的力矩。
    // 為了保持與您原版 applyExternalForceLocal 一致的行為：
    // Torque = cross(applicationPointLocal_rotated_to_world, forceMagnitude_world)
    // 或者 Torque_about_CoM_world = R_body_to_world * cross(applicationPointLocal_body, invR_world_to_body * forceMagnitude_world)

    // 假設原始意圖：forceMagnitude 是世界力，applicationPointLocal 是局部座標下的施力點到質心的向量
    glm::vec3 world_force_application_arm = R * applicationPointLocal;
    glm::vec3 torque_world = glm::cross(world_force_application_arm, forceMagnitude);
    obj->angularVelocity += obj->invInertiaTensorWorld * torque_world; // 假設這是衝量式的改變
}


// --- Collision Detection ---
// checkSpherePlane, checkOBBPlane, checkSphereSphere, checkSphereOBB, checkOBBOBB 的定義來自您先前的版本
// 這些函數現在接收 const AABB* 或 const OBB*

bool PhysicsManager::checkAABBPlane(const AABB* aabb, const Plane& plane, float& penetrationDepth, glm::vec3& collisionNormal) {
    // ... (此函數的實作已在上次提供，基於 aabb->getCenter() 和 aabb->getHalfExtents()) ...
    if (!aabb) return false;
    glm::vec3 center = aabb->getCenter();
    glm::vec3 halfExtents = aabb->getHalfExtents(); 

    float r_projection = halfExtents.x * glm::abs(plane.normal.x) +
                       halfExtents.y * glm::abs(plane.normal.y) +
                       halfExtents.z * glm::abs(plane.normal.z);
    
    float signedDistance = glm::dot(center, plane.normal) - plane.distance;

    if (glm::abs(signedDistance) <= r_projection) { 
        if (signedDistance <= r_projection) { // 確保是朝向平面內部穿透
             penetrationDepth = r_projection - signedDistance;
             if (penetrationDepth < 0) penetrationDepth = r_projection + signedDistance; // 修正可能的負穿透深度
             
             // 再次檢查以避免符號問題導致的極大 penetrationDepth
             if ( (r_projection - glm::abs(signedDistance)) < 0 ) return false; // 沒有真的重疊
             penetrationDepth = r_projection - glm::abs(signedDistance);


             collisionNormal = plane.normal;
             return true;
        }
    }
    return false;
}

bool PhysicsManager::checkOBBPlane(const OBB* obb, const Plane& plane, float& penetrationDepth, glm::vec3& collisionNormal) {
    // ... (此函數的實作來自您先前的版本，基於 obb->center, obb->extents, obb->axes) ...
    if (!obb) return false;
    float r = obb->extents.x * glm::abs(glm::dot(obb->axes[0], plane.normal)) +
              obb->extents.y * glm::abs(glm::dot(obb->axes[1], plane.normal)) +
              obb->extents.z * glm::abs(glm::dot(obb->axes[2], plane.normal));
    float signedDistance = glm::dot(obb->center, plane.normal) - plane.distance;
    if (glm::abs(signedDistance) <= r) {
        if (signedDistance <=r) { //確保是穿透
            penetrationDepth = r - signedDistance;
            if (penetrationDepth < 0) penetrationDepth = r + signedDistance; // 修正
            
            if ( (r - glm::abs(signedDistance)) < 0 ) return false;
            penetrationDepth = r - glm::abs(signedDistance);

            collisionNormal = plane.normal;
            return true;
        }
    }
    return false;
}

bool PhysicsManager::checkAABBAABB(const AABB* aabb1, const AABB* aabb2, glm::vec3& collisionNormal, float& penetrationDepth) {
    // ... (此函數的實作已在上次提供，基於 aabb->intersects() 和簡化穿透計算) ...
    if (!aabb1 || !aabb2) return false;
    if (aabb1->intersects(*aabb2)) {
        glm::vec3 dir = aabb2->getCenter() - aabb1->getCenter();
        if (glm::length(dir) < 0.0001f) dir = glm::vec3(1,0,0); // Avoid zero vector if centers coincide
        collisionNormal = glm::normalize(dir);

        float overlapX = (aabb1->getHalfExtents().x + aabb2->getHalfExtents().x) - glm::abs(aabb1->getCenter().x - aabb2->getCenter().x);
        float overlapY = (aabb1->getHalfExtents().y + aabb2->getHalfExtents().y) - glm::abs(aabb1->getCenter().y - aabb2->getCenter().y);
        float overlapZ = (aabb1->getHalfExtents().z + aabb2->getHalfExtents().z) - glm::abs(aabb1->getCenter().z - aabb2->getCenter().z);
        
        if (overlapX <=0 || overlapY <=0 || overlapZ <=0) return false; 

        penetrationDepth = std::min({overlapX, overlapY, overlapZ});
        // Refine collision normal based on minimum overlap axis if needed
        if (penetrationDepth == overlapX) collisionNormal = glm::vec3(dir.x > 0 ? 1 : -1, 0, 0);
        else if (penetrationDepth == overlapY) collisionNormal = glm::vec3(0, dir.y > 0 ? 1 : -1, 0);
        else collisionNormal = glm::vec3(0, 0, dir.z > 0 ? 1 : -1);
        collisionNormal = glm::normalize(collisionNormal);


        return true;
    }
    return false;
}

bool PhysicsManager::checkAABBOBB(const AABB* aabb, const OBB* obb, glm::vec3& collisionNormal, float& penetrationDepth) {
    // ... (SAT for AABB-OBB, 較複雜, 此為簡化或需完整 SAT) ...
    // This requires testing 3 axes from AABB, 3 from OBB, and 9 cross products.
    // For simplicity, one might use a closest point algorithm or a less precise check.
    // Full SAT implementation is extensive. Here is a very basic placeholder concept:
    if (!aabb || !obb) return false;

    // Simplified: treat AABB as an OBB with world-aligned axes
    OBB aabbAsObb;
    aabbAsObb.center = aabb->getCenter();
    aabbAsObb.extents = aabb->getHalfExtents();
    aabbAsObb.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity orientation
    aabbAsObb.updateAxes(); // Set axes to world axes

    return checkOBBOBB(&aabbAsObb, obb, collisionNormal, penetrationDepth); // Reuse OBB-OBB
}

bool PhysicsManager::checkOBBOBB(const OBB* obb1, const OBB* obb2, glm::vec3& collisionNormal, float& penetrationDepth) {
    // ... (SAT 實作，如先前提供的框架) ...
    if(!obb1 || !obb2) return false;
    std::vector<glm::vec3> axesToTest;
    axesToTest.push_back(obb1->axes[0]);
    axesToTest.push_back(obb1->axes[1]);
    axesToTest.push_back(obb1->axes[2]);
    axesToTest.push_back(obb2->axes[0]);
    axesToTest.push_back(obb2->axes[1]);
    axesToTest.push_back(obb2->axes[2]);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            glm::vec3 crossProd = glm::cross(obb1->axes[i], obb2->axes[j]);
            if (glm::length(crossProd) > 0.0001f) { 
                axesToTest.push_back(glm::normalize(crossProd));
            }
        }
    }

    float minOverlap = std::numeric_limits<float>::max(); // Smallest overlap found
    glm::vec3 mtvAxis;                                    // Axis of minimum overlap

    for (const auto& axis : axesToTest) {
        if (glm::length(axis) < 0.0001f) continue; 
        Interval interval1, interval2; // SAT helper struct, not used by overlapOnAxis
        float min1, max1, min2, max2;
        obb1->projectOntoAxis(axis, min1, max1);
        obb2->projectOntoAxis(axis, min2, max2);

        if (max1 < min2 || max2 < min1) {
            return false; // Separating axis found
        } else {
            // Calculate overlap on this axis
            float overlap = std::min(max1, max2) - std::max(min1, min2);
            if (overlap < minOverlap) {
                minOverlap = overlap;
                mtvAxis = axis;
            }
        }
    }
    
    // If no separating axis, they are colliding
    penetrationDepth = minOverlap;
    collisionNormal = mtvAxis;

    // Ensure collisionNormal points from obj1 to obj2
    glm::vec3 dirObj1ToObj2 = obb2->center - obb1->center;
    if (glm::dot(collisionNormal, dirObj1ToObj2) < 0.0f) {
        collisionNormal *= -1.0f;
    }
    return true;
}


void PhysicsManager::checkCollisions() { // (修改：使用 GameObjectType 進行轉換)
    for (size_t i = 0; i < physicsObjects.size(); ++i) {
        GameObject* obj1 = physicsObjects[i];
        if (obj1->isStatic) continue;

        checkObjectEnvironmentCollisions(obj1);

        for (size_t j = i + 1; j < physicsObjects.size(); ++j) {
            GameObject* obj2 = physicsObjects[j];
            if (obj2->isStatic && obj1->isStatic) continue; 
            checkObjectObjectCollisions(obj1, obj2);
        }
    }
}

void PhysicsManager::checkObjectEnvironmentCollisions(GameObject* obj) { // (修改：使用 GameObjectType)
    for (const auto& plane : roomPlanes) {
        float penetration = 0.0f;
        glm::vec3 normal;
        bool collided = false;

        if (obj->objectType == GameObjectType::PHYSICS_BALL) { // <--- 使用新的 GameObjectType
            PhysicsBall* ball = static_cast<PhysicsBall*>(obj);
            if (ball->boundingBoxAABB) { 
                collided = checkAABBPlane(ball->boundingBoxAABB, plane, penetration, normal);
            }
        } else if (obj->objectType == GameObjectType::PHYSICS_IRREGULAR) { // <--- 使用新的 GameObjectType
            PhysicsIrregularObject* irr = static_cast<PhysicsIrregularObject*>(obj);
            if (irr->boundingBoxOBB) { 
                collided = checkOBBPlane(irr->boundingBoxOBB, plane, penetration, normal);
            }
        }

        if (collided && penetration > 0.001f) { //增加一個小的閾值
            resolveCollision(obj, plane); 
        }
    }
}

void PhysicsManager::checkObjectObjectCollisions(GameObject* obj1, GameObject* obj2) { // (修改：使用 GameObjectType)
    glm::vec3 collisionNormal;
    float penetrationDepth = 0.0f;
    bool collided = false;

    AABB* aabb1 = nullptr; OBB* obb1 = nullptr;
    AABB* aabb2 = nullptr; OBB* obb2 = nullptr;

    if (obj1->objectType == GameObjectType::PHYSICS_BALL) aabb1 = static_cast<PhysicsBall*>(obj1)->boundingBoxAABB;
    else if (obj1->objectType == GameObjectType::PHYSICS_IRREGULAR) obb1 = static_cast<PhysicsIrregularObject*>(obj1)->boundingBoxOBB;

    if (obj2->objectType == GameObjectType::PHYSICS_BALL) aabb2 = static_cast<PhysicsBall*>(obj2)->boundingBoxAABB;
    else if (obj2->objectType == GameObjectType::PHYSICS_IRREGULAR) obb2 = static_cast<PhysicsIrregularObject*>(obj2)->boundingBoxOBB;
    
    if (!aabb1 && !obb1) return; 
    if (!aabb2 && !obb2) return; 

    if (aabb1 && aabb2) { 
        collided = checkAABBAABB(aabb1, aabb2, collisionNormal, penetrationDepth);
    } else if (aabb1 && obb2) { 
        collided = checkAABBOBB(aabb1, obb2, collisionNormal, penetrationDepth);
    } else if (obb1 && aabb2) { 
        collided = checkAABBOBB(aabb2, obb1, collisionNormal, penetrationDepth); 
        if (collided) collisionNormal *= -1.0f; 
    } else if (obb1 && obb2) { 
        collided = checkOBBOBB(obb1, obb2, collisionNormal, penetrationDepth);
    }

    if (collided && penetrationDepth > 0.001f) { //增加一個小的閾值
        resolveCollision(obj1, obj2, collisionNormal, penetrationDepth);
    }
}

// --- Collision Response ---
void PhysicsManager::resolveCollision(GameObject* obj, const Plane& plane) { // (修改：使用 GameObjectType 和對應的碰撞體)
    float penetration = 0.0f; // 用來實際計算的穿透值
    glm::vec3 collisionPointNormal = plane.normal; // 碰撞點的法線就是平面的法線

    // 重新計算穿透深度，避免依賴 checkXPlane 函數的輸出（它們可能只返回布林值或近似值）
    if (obj->objectType == GameObjectType::PHYSICS_BALL) {
        PhysicsBall* ball = static_cast<PhysicsBall*>(obj);
        if (!ball->boundingBoxAABB) return;
        // AABB 中心到平面的距離
        float dist = glm::dot(ball->boundingBoxAABB->getCenter(), plane.normal) - plane.distance;
        // AABB 沿法線的投影半徑
        glm::vec3 aabbHalfExtents = ball->boundingBoxAABB->getHalfExtents();
        float radius_on_normal = glm::abs(aabbHalfExtents.x * plane.normal.x) +
                                 glm::abs(aabbHalfExtents.y * plane.normal.y) +
                                 glm::abs(aabbHalfExtents.z * plane.normal.z);
        penetration = radius_on_normal - dist;
    } else if (obj->objectType == GameObjectType::PHYSICS_IRREGULAR) {
        PhysicsIrregularObject* irr = static_cast<PhysicsIrregularObject*>(obj);
        if (!irr->boundingBoxOBB) return;
        // OBB 中心到平面的距離
        float dist = glm::dot(irr->boundingBoxOBB->center, plane.normal) - plane.distance;
        // OBB 沿法線的投影半徑
        float radius_on_normal = irr->boundingBoxOBB->extents.x * glm::abs(glm::dot(irr->boundingBoxOBB->axes[0], plane.normal)) +
                                 irr->boundingBoxOBB->extents.y * glm::abs(glm::dot(irr->boundingBoxOBB->axes[1], plane.normal)) +
                                 irr->boundingBoxOBB->extents.z * glm::abs(glm::dot(irr->boundingBoxOBB->axes[2], plane.normal));
        penetration = radius_on_normal - dist;
    } else {
        return; 
    }

    if (penetration > 0.001f) { // 確保真的有穿透
         obj->position += collisionPointNormal * penetration; // 位置修正
    } else {
        return; // 如果計算出的穿透深度不大於閾值，則不進行速度響應
    }

    float velAlongNormal = glm::dot(obj->linearVelocity, collisionPointNormal);
    if (velAlongNormal < 0) { 
        glm::vec3 vNormal = collisionPointNormal * velAlongNormal;
        glm::vec3 vTangent = obj->linearVelocity - vNormal;
        vNormal = -obj->restitution * vNormal; 
        obj->linearVelocity = vTangent + vNormal;
        // 簡易摩擦 (可選)
        // float frictionCoeff = 0.3f;
        // if (glm::length(vTangent) > 0.001f) {
        //     obj->linearVelocity -= glm::normalize(vTangent) * std::min(frictionCoeff * glm::abs(velAlongNormal) , glm::length(vTangent) );
        // }
    }
}

void PhysicsManager::resolveCollision(GameObject* obj1, GameObject* obj2, const glm::vec3& contactNormal, float penetrationDepth) { // (邏輯保持不變)
    if (obj1->isStatic && obj2->isStatic) return;

    const float k_slop = 0.005f; // 允許的微小穿透
    const float percent = 0.6f; // 穿透校正百分比 (0.2-0.8)
    float invMass1 = obj1->isStatic || obj1->mass <= 0.0f ? 0.0f : 1.0f / obj1->mass;
    float invMass2 = obj2->isStatic || obj2->mass <= 0.0f ? 0.0f : 1.0f / obj2->mass;

    // 防止 invMass1 + invMass2 為零導致除以零
    float totalInvMass = invMass1 + invMass2;
    if (totalInvMass < 0.00001f) totalInvMass = 1.0f; // 如果兩者都不可移動，則避免 NaN

    glm::vec3 correction = glm::max(penetrationDepth - k_slop, 0.0f) / totalInvMass * percent * contactNormal;
    
    if (!obj1->isStatic) obj1->position -= invMass1 * correction;
    if (!obj2->isStatic) obj2->position += invMass2 * correction;

    glm::vec3 rv = obj2->linearVelocity - obj1->linearVelocity;
    float velAlongNormal = glm::dot(rv, contactNormal);

    if (velAlongNormal > 0) {
        return;
    }

    float e = std::min(obj1->restitution, obj2->restitution);
    float j_numerator = -(1 + e) * velAlongNormal;
    float j_denominator = totalInvMass; // 已計算
    
    if (j_denominator < 0.00001f) return;
    float j = j_numerator / j_denominator;

    glm::vec3 impulse = j * contactNormal;
    if (!obj1->isStatic) obj1->linearVelocity -= invMass1 * impulse;
    if (!obj2->isStatic) obj2->linearVelocity += invMass2 * impulse;
    // TODO: 旋轉衝量的處理
}