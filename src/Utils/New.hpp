#pragma once

// new
void * operator new(unsigned long int cbSize);

// new[]
void* operator new[](unsigned long int cbSize);

// placement new
void * operator new(unsigned long int cbSize, void * pv);

// delete
void operator delete(void* p) noexcept;

// delete[]
void operator delete[] (void* p) noexcept;
