/*

SIN Toolchain
HeapObjects.cpp
Copyright 2019 Riley Lannon

This file contains the implementations of the various SINVM functions that manage the heap, specifically:
	1) void allocate_heap_memory()	-	allocate memory on the heap, adding it to SINVM::dynamic_objects
	2) void reallocate_heap_memory(bool error_if_not_found)	-	reallocate memory at some address; depending on the syscall used, may generate an error if the object is not found or allocate a new one
	3) void free_heap_memory()	-	free the memory for the object beginning at the specified location

*/

#include "SINVM.h"


void SINVM::allocate_heap_memory()
{
	/*

	Attempts to allocate some memory on the heap. It tries to allocate REG_A bytes, and will load REG_B with the address where the object is located. If it cannot find any space for the object, it will load REG_A and REG_B with 0x00.

	*/

	// first, check to see the next available address in the heap that is large enough for this object -- use an iterator
	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	DynamicObject previous(_HEAP_START, 0);
	uint16_t next_available_address = 0x00;

	// if we have no DynamicObjects, the first location is _HEAP_START
	if (this->dynamic_objects.size() == 0) {
		next_available_address = _HEAP_START;
	}

	bool found_space = false;	// if we found space for the object somewhere in the middle of the list, use this to terminate the loop; we will use list::insert to add a new heap object at the position of the iterator

	while (obj_iter != this->dynamic_objects.end() && !found_space) {
		// check to see if there's room between the end of the previous object (which is the start address + size) and the start of the next object
		if (REG_A <= (obj_iter->get_start_address() - (previous.get_start_address() + previous.get_size()))) {
			// if there is, update the start address
			next_available_address = previous.get_start_address() + previous.get_size();
			found_space = true;
		}
		else {
			// update 'previous' and increment the object iterator
			previous = *obj_iter;
			obj_iter++;
		}
	}
	// do one last check against the very last item and the end of the heap
	if (REG_A <= (_HEAP_MAX - previous.get_start_address() + previous.get_size())) {
		next_available_address = previous.get_start_address() + previous.get_size();
	}

	// if the next available address is within our heap space, we are ok
	if ((next_available_address >= _HEAP_START) || (next_available_address <= _HEAP_MAX)) {
		REG_B = next_available_address;	// set the B register to the available address
		this->dynamic_objects.insert(obj_iter, DynamicObject(REG_B, REG_A));	// instead of appending and sorting, insert the object in the list -- this will be less computationally expensive ( O(n) vs O(n log n) )
	}
	else {
		// if the memory allocation fails, return a NULL pointer
		REG_B = 0x00;
		REG_A = 0x00;
	}
}

void SINVM::reallocate_heap_memory(bool error_if_not_found)
{
	/*

	Attempts to reallocate the dynamic object at the location specified by REG_B with the number of bytes in REG_A.
	If there is room for the new size where the object is currently allocated, then it will leave it where it is and simply change the size in the VM. If not, it will try to find a new place. If it can't reallocate the memory, it will load REG_A and REG_B with 0x00.
	If the VM cannot find an object at the location specified, it will:
		- Load the registers with 0x00 if 'error_if_not_found' is true
		- Allocate a new heap object if 'error_if_not_found' is false

	*/

	// iterate through the dynamic objects, trying to find the object we are looking for
	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	std::list<DynamicObject>::iterator target_object;
	bool found = false;

	while (obj_iter != dynamic_objects.end() && !found) {
		// if REG_B is equal to the address of obj_iter, we have found our object
		if (obj_iter->get_start_address() == this->REG_B) {
			found = true;
			target_object = obj_iter;
		}
		else {
			obj_iter++;
		}
	}

	// if we can't find the dynamic object, load the registers with 0x00
	if (found) {
		uint16_t original_address = obj_iter->get_start_address();
		uint16_t old_size = obj_iter->get_size();

		// because we sort the list when we allocate a heap object, the next object (if there is one) will be next in the order
		obj_iter++;	// increment the iterator

		// if we have another object in the vector, see if there is space for the new length
		if (obj_iter != dynamic_objects.end()) {
			// get the space between the start address and the end of the current object
			uint16_t buffer_space = obj_iter->get_start_address() - (target_object->get_start_address() + target_object->get_size());

			// if the new size is greater than the target object's size, but less than the target object's size plus the buffer, update the size
			// if it's less than or equal to the target object's size, we don't need to do anything
			if ((this->REG_A > target_object->get_size()) && (this->REG_A < (target_object->get_size() + buffer_space))) {
				target_object->set_size(this->REG_A);	// update the size to the value in REG_A
			}
			// otherwise, if it overflows the buffer, reallocate it
			else if (this->REG_A > (target_object->get_size() + buffer_space)) {
				// try to allocate it normally
				this->allocate_heap_memory();

				// copy the data from the old space into the new one
				memcpy(&this->memory[this->REG_B], &this->memory[original_address], old_size);

				// finally, remove the target object from the vector
				this->dynamic_objects.erase(target_object);	// because target_object is an iterator, we can remove it
			}
		}
		else {
			// otherwise, as long as we won't overrun the heap, it can stay
			if ((target_object->get_start_address() + this->REG_A) <= _HEAP_MAX) {
				target_object->set_size(this->REG_A);	// all we have to do is update the size
			}
			else {
				this->REG_A = 0x00;	// there's no room, so load them with 0x00
				this->REG_B = 0x00;
			}
		}
	}
	else {
		// depending on our parameter, the SINVM will behave differently -- load registers with NULL vs allocating a new object
		if (error_if_not_found) {
			this->REG_A = 0x00;	// we can't find the object, so set the registers to 0x00
			this->REG_B = 0x00;
		}
		else {
			this->allocate_heap_memory();	// allocate heap memory for the object if we can't find it
		}
	}
}

void SINVM::free_heap_memory()
{
	/*

	Free the memory block starting at the memory address indicated by the B register. If there is no memory there, throw a VMException

	*/

	std::list<DynamicObject>::iterator obj_iter = this->dynamic_objects.begin();
	bool found = false;

	while ((obj_iter != this->dynamic_objects.end()) && !found) {
		// if the address of the object is the address we want to free, break from the loop; obj_iter contains the reference
		if (obj_iter->get_start_address() == REG_B) {
			found = true;
		}
		// otherwise, increment the iterator
		else {
			obj_iter++;
		}
	}

	if (found) {
		this->dynamic_objects.remove(*obj_iter);
	}
	else {
		throw VMException("Cannot free memory at location specified.");
	}
}
