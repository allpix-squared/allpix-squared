#include "Object.hpp"

using namespace corryvreckan;

Object::Object() = default;
Object::Object(std::string detectorID) : m_detectorID(std::move(detectorID)) {}
Object::Object(double timestamp) : m_timestamp(timestamp) {}
Object::Object(std::string detectorID, double timestamp) : m_detectorID(std::move(detectorID)), m_timestamp(timestamp) {}
Object::Object(const Object&) = default;
Object::~Object() = default;

std::ostream& corryvreckan::operator<<(std::ostream& out, const Object& obj) {
    obj.print(out);
    return out;
}
