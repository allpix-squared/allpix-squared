#include "Pixel.hpp"

using namespace corryvreckan;

void Pixel::print(std::ostream& out) const {
    out << "Pixel " << this->column() << ", " << this->row() << ", " << this->raw() << ", " << this->charge() << ", "
        << this->timestamp();
}
