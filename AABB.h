#pragma once
#include <glm/glm.hpp>
#include <vector> 

class AABB {
private:
    glm::vec3 minPoint;
    glm::vec3 maxPoint;

public:
    AABB(const glm::vec3& min, const glm::vec3& max) : minPoint(min), maxPoint(max) {}

    bool Intersects(const AABB& other) const {
        return (minPoint.x <= other.maxPoint.x && maxPoint.x >= other.minPoint.x) &&
               (minPoint.y <= other.maxPoint.y && maxPoint.y >= other.minPoint.y) &&
               (minPoint.z <= other.maxPoint.z && maxPoint.z >= other.minPoint.z);
    }

    glm::vec3 GetMin() const { return minPoint; }
    glm::vec3 GetMax() const { return maxPoint; }

    // 計算距離 AABB 最近的點
    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        return glm::vec3(
            glm::clamp(point.x, minPoint.x, maxPoint.x),
            glm::clamp(point.y, minPoint.y, maxPoint.y),
            glm::clamp(point.z, minPoint.z, maxPoint.z)
        );
    }

    // 球與 AABB 的碰撞檢測
    static bool SphereToAABB(const glm::vec3& sphereCenter, float sphereRadius, const AABB& aabb) {
        glm::vec3 closest = aabb.ClosestPoint(sphereCenter);
        float distanceSquared = glm::dot(sphereCenter - closest, sphereCenter - closest);
        return distanceSquared <= sphereRadius * sphereRadius;
    }

    static bool SphereToSphere(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
        float distance = glm::length(pos2 - pos1);
        return distance < (radius1 + radius2);
    }

    static AABB FromSphere(const glm::vec3& center, float radius) {
        glm::vec3 min = center - glm::vec3(radius);
        glm::vec3 max = center + glm::vec3(radius);
        return AABB(min, max);
    }
};