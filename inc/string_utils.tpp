#pragma once

// == template
#include <sstream>

template <typename T>
std::string to_str(const T &value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
};

template <typename T>
T stringToNumber(const std::string &str, std::ios_base &base(std::ios_base &b))
{
	std::stringstream ss(str);	
	T out_number;

	ss >> base >> out_number;
	if (ss.fail() || !ss.eof())
		throw std::runtime_error("Invalid number convertion");
	
	return (out_number);
}
