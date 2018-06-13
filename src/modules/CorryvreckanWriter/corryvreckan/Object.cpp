// local
#include "Object.hpp"

#include "core/utils/exceptions.h"

using namespace corryvreckan;

Object::Object() = default;
Object::Object(std::string detectorID) : m_detectorID(std::move(detectorID)) {}
Object::Object(double timestamp) : m_timestamp(timestamp) {}
Object::Object(std::string detectorID, double timestamp) : m_detectorID(std::move(detectorID)), m_timestamp(timestamp) {}
Object::~Object() = default;
