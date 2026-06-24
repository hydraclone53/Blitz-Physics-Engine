#include "HydraException.h"
#include <sstream> //instead of printing the string to console, it prints to the memory


HydraException::HydraException(int line, const char* file) noexcept
	:
	line(line),
	file(file) //Get initiallized even before the {} body runs (Its faster!)
{
}

const char* HydraException::what() const noexcept
{
	std::ostringstream oss;//if we return only oss, the oss will die at the end of the function. Hence we store it in the buffer
	oss << GetType() << std::endl<< GetOriginString();//std::endl is basically new line
	whatBuffer = oss.str();//stores the string in this mutable variable
	return whatBuffer.c_str();
}

const char* HydraException::GetType() const noexcept
{
	return "Hydra Exception";
}

int HydraException::GetLine() const noexcept
{
	return line;
}

const std::string& HydraException::GetFile() const noexcept
{
	return file;
}

std::string HydraException::GetOriginString() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << file << std::endl<< "[Line] " << line;
	return oss.str();
}
