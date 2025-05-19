#include "physicManager.h"

static glm::vec3 NormalizeVec3(const glm::vec3& v) {
    float lenSq = glm::dot(v, v);
    if (lenSq > 0.00001f) {
        return v / std::sqrt(lenSq);
    }
    return glm::vec3(0.0f);
}


PhysicManager::PhysicManager(float g) : gravityAcceleration(g) {}

void PhysicManager::applyGravity(Object& obj) const {
    if (obj.mass > 0.0f) {
        obj.applyForce(glm::vec3(0.0f, -obj.mass * gravityAcceleration, 0.0f), obj.getWorldCenterOfMass());
    }
}

void PhysicManager::update(std::vector<Object*>& objects, const AABB& roomAABB, float deltaTime) {
    if (pausePhysics) return;

    for (Object* objPtr : objects) {
        if (!objPtr) continue;
        Object& obj = *objPtr;

        // Apply gravity
        applyGravity(obj);

        // Update physics (integrates forces, updates position/rotation)
        obj.updatePhysics(deltaTime);

        // --- Collision Detection and Response ---

        // 1. Object vs Room (AABB)
        if (obj.IsSphereObject()) {
            if (AABB::SphereToAABB(obj.GetBoundingSphere(), roomAABB)) {
                glm::vec3 closestPointInRoom = roomAABB.ClosestPoint(obj.GetBoundingSphere().center);
                glm::vec3 normalFromRoom = NormalizeVec3(obj.GetBoundingSphere().center - closestPointInRoom);

                float penetration = obj.GetBoundingSphere().radius - glm::distance(obj.GetBoundingSphere().center, closestPointInRoom);
                if (penetration > 0.0001f) {
                    obj.SetPosition(obj.GetPosition() + normalFromRoom * penetration);
                }

                glm::vec3 v_cm = obj.GetVelocity();
                glm::vec3 r = closestPointInRoom - obj.getWorldCenterOfMass();
                glm::vec3 v_point = v_cm + glm::cross(obj.angularVelocity, r);
                float v_n_close = glm::dot(v_point, normalFromRoom);

                if (v_n_close >= 0) continue;

                float e = obj.restitution;
                float invMass = (obj.mass > 0.0f) ? 1.0f / obj.mass : 0.0f;
                glm::mat3 invIA_world = obj.getWorldInertiaTensor();
                if (glm::determinant(invIA_world) != 0.0f) invIA_world = glm::inverse(invIA_world);
                else invIA_world = glm::mat3(0.0f);

                glm::vec3 r_cross_n = glm::cross(r, normalFromRoom);
                glm::vec3 term_in_denominator = glm::cross(invIA_world * r_cross_n, r);
                float denominator = invMass + glm::dot(normalFromRoom, term_in_denominator);

                if (denominator <= 0.00001f) continue;

                float j_scalar = -(1.0f + e) * v_n_close / denominator;
                glm::vec3 J_impulse = j_scalar * normalFromRoom;

                obj.SetVelocity(v_cm + J_impulse * invMass);
                obj.angularVelocity += invIA_world * glm::cross(r, J_impulse);
            }
        } else {
            AABB worldObjAABB = obj.GetAABB();
            if (!worldObjAABB.Intersects(roomAABB)) continue;

            float minPenetration = FLT_MAX;
            glm::vec3 collisionNormal(0.0f);
            bool collisionDetected = false;

            // Check penetration against each face of the roomAABB
            // Object's left face vs Room's left wall
            float pen = roomAABB.GetMin().x - worldObjAABB.GetMin().x;
            if (pen > 0 && pen < minPenetration) { minPenetration = pen; collisionNormal = glm::vec3(1.0f, 0.0f, 0.0f); collisionDetected = true; }
            // Object's right face vs Room's right wall
            pen = roomAABB.GetMax().x - worldObjAABB.GetMax().x;
            if (pen < 0 && -pen < minPenetration) { minPenetration = -pen; collisionNormal = glm::vec3(-1.0f, 0.0f, 0.0f); collisionDetected = true; }
            
            // Object's bottom face vs Room's floor
            pen = roomAABB.GetMin().y - worldObjAABB.GetMin().y;
            if (pen > 0 && pen < minPenetration) { minPenetration = pen; collisionNormal = glm::vec3(0.0f, 1.0f, 0.0f); collisionDetected = true; }
            // Object's top face vs Room's ceiling
            pen = roomAABB.GetMax().y - worldObjAABB.GetMax().y;
            if (pen < 0 && -pen < minPenetration) { minPenetration = -pen; collisionNormal = glm::vec3(0.0f, -1.0f, 0.0f); collisionDetected = true; }

            // Object's back face vs Room's back wall
            pen = roomAABB.GetMin().z - worldObjAABB.GetMin().z;
            if (pen > 0 && pen < minPenetration) { minPenetration = pen; collisionNormal = glm::vec3(0.0f, 0.0f, 1.0f); collisionDetected = true; }
            // Object's front face vs Room's front wall
            pen = roomAABB.GetMax().z - worldObjAABB.GetMax().z;
            if (pen < 0 && -pen < minPenetration) { minPenetration = -pen; collisionNormal = glm::vec3(0.0f, 0.0f, -1.0f); collisionDetected = true; }

            if (collisionDetected && minPenetration > 0.0001f) {
                obj.SetPosition(obj.GetPosition() + collisionNormal * minPenetration);

                // Approximate contact point on the object's AABB face center
                glm::vec3 objHalfExtents = (worldObjAABB.GetMax() - worldObjAABB.GetMin()) * 0.5f;
                glm::vec3 r_local(0.0f);
                if(std::abs(collisionNormal.x) > 0.9f) r_local.x = -collisionNormal.x * objHalfExtents.x;
                else if(std::abs(collisionNormal.y) > 0.9f) r_local.y = -collisionNormal.y * objHalfExtents.y;
                else if(std::abs(collisionNormal.z) > 0.9f) r_local.z = -collisionNormal.z * objHalfExtents.z;
                
                // r is vector from CoM to contact point. Here r_local is already that vector in local space of AABB (if CoM is AABB center).
                // For simplicity, assuming CoM is at the center of the worldObjAABB for this contact 'r' calculation.
                // More accurately, r should be: (obj.GetWorldCenterOfMass() + r_local_rotated_to_world) - obj.GetWorldCenterOfMass()
                // Since collisionNormal is world-aligned, and r_local is aligned with AABB face, this is complex for OBB.
                // For AABB vs AABB, r can be approximated from CoM to face center.
                glm::vec3 contactPoint = obj.getWorldCenterOfMass() + r_local; // This assumes r_local is in world space or obj is not rotated, which is an approximation.
                                                                        // A simpler r for AABB vs AABB (axis aligned collision):
                glm::vec3 r = glm::vec3( (collisionNormal.x != 0 ? -collisionNormal.x * objHalfExtents.x : 0.0f),
                                       (collisionNormal.y != 0 ? -collisionNormal.y * objHalfExtents.y : 0.0f),
                                       (collisionNormal.z != 0 ? -collisionNormal.z * objHalfExtents.z : 0.0f) );
                // This 'r' is the vector from the AABB center to the center of the colliding face.
                // If obj.getWorldCenterOfMass() is not obj.GetAABB().GetCenter(), this needs adjustment.
                // Assuming obj.centerOfMass is local to object origin, and obj.rotation transforms it.
                // The vector from the TRUE CoM to the contact point on the AABB surface is needed.
                // Let's use the same 'r' as the sphere for now, with contactPoint being on the AABB surface.
                glm::vec3 pointOnObjAABBFace = obj.GetPosition(); // Placeholder, needs better calculation
                if (std::abs(collisionNormal.x) > 0.5f) pointOnObjAABBFace.x += collisionNormal.x > 0 ? worldObjAABB.GetMin().x - obj.GetPosition().x : worldObjAABB.GetMax().x - obj.GetPosition().x; 
                // This contact point logic is getting complicated. For AABB vs static AABB, the contact point for impulse is tricky.
                // Simplification: assume contact point is at obj.getWorldCenterOfMass() + r_vec_to_colliding_face_center
                // where r_vec_to_colliding_face_center for AABB is simple:
                r = glm::vec3(0.0f); // Default r to CoM if using point collision model
                // For face collision:
                if (abs(collisionNormal.x) > 0.5f) r.x = (collisionNormal.x < 0) ? objHalfExtents.x : -objHalfExtents.x;
                if (abs(collisionNormal.y) > 0.5f) r.y = (collisionNormal.y < 0) ? objHalfExtents.y : -objHalfExtents.y;
                if (abs(collisionNormal.z) > 0.5f) r.z = (collisionNormal.z < 0) ? objHalfExtents.z : -objHalfExtents.z;
                // This 'r' is in AABB's local frame (if AABB axes are object's local axes). It needs to be rotated to world by obj.rotation.
                r = obj.GetRotation() * r; // Rotate r to world space


                glm::vec3 v_cm = obj.GetVelocity();
                glm::vec3 v_point = v_cm + glm::cross(obj.angularVelocity, r);
                float v_n_close = glm::dot(v_point, collisionNormal);

                if (v_n_close >= 0) continue;

                float e = obj.restitution;
                float invMass = (obj.mass > 0.0f) ? 1.0f / obj.mass : 0.0f;
                glm::mat3 invIA_world = obj.getWorldInertiaTensor();
                if (glm::determinant(invIA_world) != 0.0f) invIA_world = glm::inverse(invIA_world);
                else invIA_world = glm::mat3(0.0f);

                glm::vec3 r_cross_n = glm::cross(r, collisionNormal);
                glm::vec3 term_in_denominator = glm::cross(invIA_world * r_cross_n, r);
                float denominator = invMass + glm::dot(collisionNormal, term_in_denominator);

                if (denominator <= 0.00001f) continue;

                float j_scalar = -(1.0f + e) * v_n_close / denominator;
                glm::vec3 J_impulse = j_scalar * collisionNormal;

                obj.SetVelocity(v_cm + J_impulse * invMass);
                obj.angularVelocity += invIA_world * glm::cross(r, J_impulse);
            }
        }

        // 2. Object vs Object
        for (Object* otherPtr : objects) {
            if (!otherPtr || objPtr == otherPtr) continue;
            Object& other = *otherPtr;

            bool collisionDetectedThisPair = false;
            glm::vec3 normalForThisPair(0.0f);
            glm::vec3 pointOfContactThisPair(0.0f);

            // --- 碰撞檢測邏輯開始 ---
            if (obj.IsSphereObject() && other.IsSphereObject()) {
                // 球體 vs 球體
                if (AABB::SphereToSphere(obj.GetBoundingSphere(), other.GetBoundingSphere())) {
                    collisionDetectedThisPair = true;
                    normalForThisPair = NormalizeVec3(other.GetBoundingSphere().center - obj.GetBoundingSphere().center);
                    // 接觸點可以取兩個球心連線的中點，或者一個球體表面
                    pointOfContactThisPair = obj.GetBoundingSphere().center + normalForThisPair * obj.GetBoundingSphere().radius;
                }
            } else if (obj.IsSphereObject() && !other.IsSphereObject()) {
                // 球體 (obj) vs AABB (other)
                if (AABB::SphereToAABB(obj.GetBoundingSphere(), other.GetAABB())) {
                    collisionDetectedThisPair = true;
                    pointOfContactThisPair = other.GetAABB().ClosestPoint(obj.GetBoundingSphere().center);
                    normalForThisPair = NormalizeVec3(obj.GetBoundingSphere().center - pointOfContactThisPair);
                    // 可選：將接觸點精確移到球體表面
                    // pointOfContactThisPair = obj.GetBoundingSphere().center - normalForThisPair * obj.GetBoundingSphere().radius;
                }
            } else if (!obj.IsSphereObject() && other.IsSphereObject()) {
                // AABB (obj) vs 球體 (other)
                if (AABB::SphereToAABB(other.GetBoundingSphere(), obj.GetAABB())) {
                    collisionDetectedThisPair = true;
                    pointOfContactThisPair = obj.GetAABB().ClosestPoint(other.GetBoundingSphere().center);
                    normalForThisPair = NormalizeVec3(other.GetBoundingSphere().center - pointOfContactThisPair);
                }
            } else {
                // AABB (obj) vs AABB (other)
                if (obj.GetAABB().Intersects(other.GetAABB())) {
                    collisionDetectedThisPair = true;
                    // 簡化 AABB-AABB 的法線和接觸點計算
                    // 法線可以從一個中心指向另一個中心，或者更複雜的 SAT 分離軸
                    normalForThisPair = NormalizeVec3(other.GetPosition() - obj.GetPosition()); 
                    // 接觸點可以近似為兩個AABB中心的連線與其中一個AABB表面的交點
                    // 或者更簡單地，取兩個AABB重疊區域的中心（如果能計算出來的話）
                    // 這裡使用一個非常簡化的版本：兩個物體位置的中點
                    pointOfContactThisPair = (obj.GetPosition() + other.GetPosition()) * 0.5f;
                    // 注意：這個簡化的 AABB-AABB 接觸點和法線可能導致不夠真實的物理行為，
                    // 特別是對於旋轉。更精確的方法（如SAT）會更複雜。
                }
            }
            // --- 碰撞檢測邏輯結束 ---

            if (collisionDetectedThisPair) {
                // 計算質心世界座標
                glm::vec3 cmA = obj.getWorldCenterOfMass();
                glm::vec3 cmB = other.getWorldCenterOfMass();

                // 計算 r_A 和 r_B（質心到碰撞點的向量）
                glm::vec3 rA = pointOfContactThisPair - cmA;
                glm::vec3 rB = pointOfContactThisPair - cmB;

                // 計算質心速度
                glm::vec3 v_cmA = obj.GetVelocity();
                glm::vec3 v_cmB = other.GetVelocity();

                // 計算碰撞點的速度
                glm::vec3 v_PA = v_cmA + glm::cross(obj.angularVelocity, rA);
                glm::vec3 v_PB = v_cmB + glm::cross(other.angularVelocity, rB);

                // 計算相對速度
                glm::vec3 v_rel = v_PA - v_PB;

                // 沿法線方向的接近速度 (使用 normalForThisPair)
                float v_n_close = glm::dot(v_rel, normalForThisPair);

                // 如果物體正在分離（或平行移動），則不處理碰撞
                if (v_n_close >= 0.0f) {
                    continue; // 跳過此碰撞對的響應
                }

                // 調用 Object 類的 handleCollision 方法來處理衝量和速度更新
                // normalForThisPair 是從 obj 指向 other 的方向
                obj.handleCollision(other, pointOfContactThisPair, normalForThisPair);
            }
        } // 結束對 otherPtr 的迴圈
    } // 結束對 objPtr 的迴圈
} // 結束 PhysicManager::update 方法