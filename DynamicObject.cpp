#include "DynamicObject.h"


bool DynamicObject::operator==(DynamicObject right) {
	return ((this->size == right.get_size()) && (this->start_address == right.get_start_address()));
}

uint16_t DynamicObject::get_start_address()
{
	return this->start_address;
}

uint16_t DynamicObject::get_size()
{
	return this->size;
}

DynamicObject::DynamicObject(uint16_t start_address, uint16_t size) : start_address(start_address), size(size) {

}

DynamicObject::DynamicObject()
{
	this->size = 0;
	this->start_address = 0;
}


DynamicObject::~DynamicObject()
{
}
