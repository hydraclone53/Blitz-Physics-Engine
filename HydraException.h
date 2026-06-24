#pragma once
#include <exception>
#include <string>

class HydraException : public std::exception
{
public:
	HydraException(int line, const char* file) noexcept;
	const char* what() const noexcept override;
	virtual const char* GetType() const noexcept;// By putting const at the end of the function, we basically promise c++ that we will only read
	int GetLine() const noexcept;
	const std::string& GetFile() const noexcept;
	std::string GetOriginString() const noexcept;
private:
	int line;
	std::string file;
protected:
	mutable std::string whatBuffer;//mutable gives exception to the read only rule in the const function
};