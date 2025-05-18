// OBB.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include <algorithm> // For std::min/max if calculating from points

class OBB {
public:
    glm::vec3 center;    // OBB 在世界座標中的中心點
    glm::vec3 extents;   // OBB 沿其局部座標軸的半長度 (half-lengths)
    glm::quat orientation; // OBB 的方向 (與其所屬物理物件的方向一致)
    std::array<glm::vec3, 3> axes; // OBB 的局部 X, Y, Z 軸在世界座標中的表示

    OBB() : center(0.0f), extents(0.5f), orientation(glm::quat(1.0f,0.0f,0.0f,0.0f)) {
        updateAxes();
    }

    OBB(const glm::vec3& c, const glm::vec3& e, const glm::quat& o = glm::quat(1.0f,0.0f,0.0f,0.0f))
        : center(c), extents(e), orientation(o) {
        updateAxes();
    }

    // 從原始頂點數據計算 OBB 的 extents (這是一個簡化版本，更精確的方法可能涉及 PCA)
    // 這裡假設頂點已經相對於局部原點 (0,0,0)
    void calculateExtentsFromVertices(const std::vector<float>& localVertices) {
        if (localVertices.empty() || localVertices.size() % 3 != 0) {
            extents = glm::vec3(0.1f); // Default small extents
            return;
        }
        // 先計算一個局部空間的 AABB 來近似 OBB 的 extents
        glm::vec3 minP(localVertices[0], localVertices[1], localVertices[2]);
        glm::vec3 maxP = minP;

        for (size_t i = 0; i < localVertices.size(); i += 3) {
            minP.x = std::min(minP.x, localVertices[i]);
            minP.y = std::min(minP.y, localVertices[i+1]);
            minP.z = std::min(minP.z, localVertices[i+2]);
            maxP.x = std::max(maxP.x, localVertices[i]);
            maxP.y = std::max(maxP.y, localVertices[i+1]);
            maxP.z = std::max(maxP.z, localVertices[i+2]);
        }
        // extents 是半長度
        this->extents = (maxP - minP) * 0.5f;
        // 注意：這種方式得到的 extents 是沿著世界座標軸的，除非頂點數據已經被旋轉到 OBB 的主軸方向。
        // 一個更準確的方法是找到物體的最小包圍盒，這比較複雜。
        // 為了符合需求，我們假設提供的 extents 已經是期望的 OBB 半長度。
        // 或者，使用者直接設定 extents。
    }


    // 更新 OBB 的世界座標中心和方向
    void update(const glm::vec3& newWorldCenter, const glm::quat& newWorldOrientation) {
        center = newWorldCenter;
        orientation = newWorldOrientation;
        updateAxes();
    }

    // 根據目前方向更新世界座標軸
    void updateAxes() {
        glm::mat3 rotMatrix = glm::mat3_cast(orientation);
        axes[0] = rotMatrix * glm::vec3(1.0f, 0.0f, 0.0f); // 局部 X 軸
        axes[1] = rotMatrix * glm::vec3(0.0f, 1.0f, 0.0f); // 局部 Y 軸
        axes[2] = rotMatrix * glm::vec3(0.0f, 0.0f, 1.0f); // 局部 Z 軸
    }

    // 獲取 OBB 在世界座標中的 8 個頂點
    std::vector<glm::vec3> getVerticesWorld() const {
        std::vector<glm::vec3> corners(8);
        glm::vec3 u = axes[0] * extents.x;
        glm::vec3 v = axes[1] * extents.y;
        glm::vec3 w = axes[2] * extents.z;

        corners[0] = center - u - v - w;
        corners[1] = center + u - v - w;
        corners[2] = center + u + v - w;
        corners[3] = center - u + v - w;
        corners[4] = center - u - v + w;
        corners[5] = center + u - v + w;
        corners[6] = center + u + v + w;
        corners[7] = center - u + v + w;
        return corners;
    }

    // 將 OBB 投影到一個軸上，並返回 [min, max] 區間
    void projectOntoAxis(const glm::vec3& axis, float& minProj, float& maxProj) const {
        minProj = maxProj = glm::dot(center, axis); // 投影中心點

        // 投影半長度
        float r0 = extents.x * glm::abs(glm::dot(axes[0], axis));
        float r1 = extents.y * glm::abs(glm::dot(axes[1], axis));
        float r2 = extents.z * glm::abs(glm::dot(axes[2], axis));
        float projectionRadius = r0 + r1 + r2;

        minProj -= projectionRadius;
        maxProj += projectionRadius;
    }
};