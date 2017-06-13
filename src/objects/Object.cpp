/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "Object.hpp"

using namespace allpix;

Object::Object() = default;
Object::~Object() = default;

Object::Object(const Object&) = default;
Object& Object::operator=(const Object&) = default;

// FIXME: check why this cannot be noexcept
Object::Object(Object&&) = default;
Object& Object::operator=(Object&&) = default;

ClassImp(Object)
