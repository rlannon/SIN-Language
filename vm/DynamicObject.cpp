#include "DynamicObject.h"


bool DynamicObject::operator==(DynamicObject right) {
	return ((this->size == right.get_size()) && (this->start_address == right.get_start_address()));
}

bool DynamicObject::operator<(DynamicObject right)
{
	return this->get_start_address() < right.get_start_address();
}

uint16_t DynamicObject::get_start_address()
{
	return this->start_address;
}

uint16_t DynamicObject::get_size()
{
	return this->size;
}

void DynamicObject::set_start_address(uint16_t new_address)
{
	this->start_address = new_address;
}

void DynamicObject::set_size(uint16_t new_size)
{
	this->size = new_size;
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
