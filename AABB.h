// AABB.h (與您提供的一致或稍作補充)
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm> // Required for std::min and std::max

class AABB {
public:
    glm::vec3 minExtents;
    glm::vec3 maxExtents;

    AABB() : minExtents(0.0f), maxExtents(0.0f) {}

    AABB(const glm::vec3& minE, const glm::vec3& maxE) : minExtents(minE), maxExtents(maxE) {}

    // Constructor from a list of glm::vec3 points (vertices)
    AABB(const std::vector<glm::vec3>& points) {
        if (points.empty()) {
            minExtents = glm::vec3(0.0f);
            maxExtents = glm::vec3(0.0f);
            return;
        }

        minExtents = points[0];
        maxExtents = points[0];

        for (const auto& point : points) {
            minExtents.x = std::min(minExtents.x, point.x);
            minExtents.y = std::min(minExtents.y, point.y);
            minExtents.z = std::min(minExtents.z, point.z);

            maxExtents.x = std::max(maxExtents.x, point.x);
            maxExtents.y = std::max(maxExtents.y, point.y);
            maxExtents.z = std::max(maxExtents.z, point.z);
        }
    }
    
    // Constructor from vertices stored as floats (x,y,z,x,y,z...)
    AABB(const std::vector<float>& vertices_f) {
        if (vertices_f.empty() || vertices_f.size() % 3 != 0) {
            minExtents = glm::vec3(0.0f);
            maxExtents = glm::vec3(0.0f);
            return;
        }

        minExtents = glm::vec3(vertices_f[0], vertices_f[1], vertices_f[2]);
        maxExtents = minExtents;

        for (size_t i = 0; i < vertices_f.size(); i += 3) {
            glm::vec3 point(vertices_f[i], vertices_f[i+1], vertices_f[i+2]);
            minExtents.x = std::min(minExtents.x, point.x);
            minExtents.y = std::min(minExtents.y, point.y);
            minExtents.z = std::min(minExtents.z, point.z);

            maxExtents.x = std::max(maxExtents.x, point.x);
            maxExtents.y = std::max(maxExtents.y, point.y);
            maxExtents.z = std::max(maxExtents.z, point.z);
        }
    }

    glm::vec3 getCenter() const {
        return (minExtents + maxExtents) * 0.5f;
    }

    glm::vec3 getHalfExtents() const { // Renamed for clarity, returns half-lengths
        return (maxExtents - minExtents) * 0.5f;
    }

    // Check collision with another AABB
    bool intersects(const AABB& other) const {
        return (maxExtents.x >= other.minExtents.x &&
                minExtents.x <= other.maxExtents.x &&
                maxExtents.y >= other.minExtents.y &&
                minExtents.y <= other.maxExtents.y &&
                maxExtents.z >= other.minExtents.z &&
                minExtents.z <= other.maxExtents.z);
    }
    
    // Update AABB based on a new world center and its constant half-extents.
    // This is suitable when the AABB's size is fixed but its position changes.
    void updateWithNewWorldCenter(const glm::vec3& worldCenter, const glm::vec3& halfExtents) {
        minExtents = worldCenter - halfExtents;
        maxExtents = worldCenter + halfExtents;
    }
};