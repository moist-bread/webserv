#pragma once

// ==┊ needed libs by class
#include <iostream>
#include "ansi_color_codes.h"

// =====>┊( CLASS )┊

class Class
{
public:
	Class(void); 				// default constructor
	Class(Class const &source);	// copy constructor
	~Class(void);				// destructor

	Class &operator=(Class const &source); // copy assignment operator overload
};

std::ostream &operator<<(std::ostream &out, Class const &source);
