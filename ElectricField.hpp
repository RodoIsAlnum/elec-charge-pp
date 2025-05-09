#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <functional>
#include <iostream>

#include "TextRender.hpp"

class ElectricCharge {
    public:
        ElectricCharge(float x, float y, float charge)
            : position (x,y), charge(charge) {}
        
        glm::vec2 position;
        float charge;
};

class ElectricField {
public:
    // Find a charge at a specific position (for mouse selection)
    int findChargeAt(float x, float y, float radius = 0.1f) const {
        for (size_t i = 0; i < charges.size(); i++) {
            float dx = charges[i].position.x - x;
            float dy = charges[i].position.y - y;
            float distSquared = dx*dx + dy*dy;
            if (distSquared < radius*radius) {
                return static_cast<int>(i);
            }
        }
        return -1; // No charge found
    }
    
    // Move a charge to a new position
    void moveCharge(int index, float x, float y) {
        if (index >= 0 && index < static_cast<int>(charges.size())) {
            charges[index].position.x = x;
            charges[index].position.y = y;
        }
    }

    void changeChargeSize(int index, float delta) {
        if (index >= 0 && index < static_cast<int>(charges.size())) {
            float scale_factor = 0.25f;
            charges[index].charge += delta * scale_factor;
            
            // Limits to prevent extreme values
            charges[index].charge = std::max(-5.0f, std::min(5.0f, charges[index].charge));
        }
        //std::cout << "void used" << std::endl;
    }

    // Mouse Wheel checks
    void mouseWheelUp(float yScroll);
    void mouseWheelDown(float yScroll);

    // Adds a charge to the field
    void addCharge(float x, float y, float charge) {
        charges.emplace_back(x, y, charge);
    }
    // Clears all the charges from the field
    void clearCharges() {
        charges.clear();
    }
    // Gets all charges
    const std::vector<ElectricCharge>& getCharges() const{
        return charges;
    }

    // Electric field calculation
    glm::vec2 getFieldAt (float x, float y) const{
        const float k = 1.0f;
        const float epsilon = 0.01f; // Just to avoid division by 0

        glm::vec2 totalField(0.0f, 0.0f);

        for(const auto& charge : charges) {
            glm::vec2 r = glm::vec2(x,y) - charge.position;

            float distSquared = glm::dot(r,r);
            if (distSquared < epsilon) continue;
            float magnitude = k * charge.charge / distSquared;
            glm::vec2 direction = glm::normalize(r);
            totalField += magnitude * direction;
        }

        return totalField;
    }

    std::function<glm::vec2(float,float)> getVectorField() {
        return [this](float x, float y) {
            return this -> getFieldAt(x,y);
        };
    }

private:
    std::vector<ElectricCharge> charges;
};
