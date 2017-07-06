/**
 * @file
 * @brief Implementation of Object base class
 * @copyright MIT License
 */

#include "Object.hpp"

using namespace allpix;

Object::Object(const Object&) = default;
Object& Object::operator=(const Object&) = default;

Object::Object(Object&&) = default;
Object& Object::operator=(Object&&) = default;

ClassImp(Object)
