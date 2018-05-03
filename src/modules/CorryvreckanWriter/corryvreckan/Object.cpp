// local
#include "Object.hpp"

#include "core/utils/exceptions.h"

using namespace corryvreckan;

Object::Object() : m_detectorID(), m_timestamp(0) {}
Object::Object(std::string detectorID) : m_detectorID(detectorID), m_timestamp(0) {}
Object::Object(double timestamp) : m_detectorID(), m_timestamp(timestamp) {}
Object::Object(std::string detectorID, double timestamp) : m_detectorID(detectorID), m_timestamp(timestamp) {}
Object::~Object() {}
