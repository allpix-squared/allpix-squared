/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Object.hpp"

using namespace allpix;

Object::Object() = default;
Object::~Object() = default;

Object::Object(const Object&) = default;
Object& Object::operator=(const Object&) = default;

Object::Object(Object&&) noexcept = default;
Object& Object::operator=(Object&&) noexcept = default;
