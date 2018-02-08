#include "Memory.h"

#include <TlHelp32.h>
#include <stdexcept>

void Memory::attach(const wchar_t* processName, DWORD dwAccessRights)
{
	PROCESSENTRY32W processEntry;
	HANDLE hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hProcess == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Invalid process entry handle for snapshots!");

	processEntry.dwSize = sizeof(PROCESSENTRY32W);

	if (!Process32FirstW(hProcess, &processEntry))
	{
		CloseHandle(hProcess);
		throw std::runtime_error("Error enumerating through processes!");
	}

	processName_ = processName; // set process name

	do
	{
		if (lstrcmpW(processName, processEntry.szExeFile) == 0)
		{
			CloseHandle(hProcess);
			hProcess_ = OpenProcess(dwAccessRights, false, processEntry.th32ProcessID);

			if (hProcess_ == nullptr) // Open process returns a nullptr in an error while createtoolhelp32snapshot returns an INVALID_HANDLE_VALUE
				throw std::runtime_error("Error opening process!");

			pId_ = processEntry.th32ProcessID;
			return;
		}
	} while (Process32NextW(hProcess, &processEntry));

	CloseHandle(hProcess);
	throw std::runtime_error("Could not find process!");
}

DWORD Memory::getbaseAddress(const wchar_t* moduleName)
{
	MODULEENTRY32W moduleEntry;
	HANDLE hModule = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pId_);

	if (hModule == INVALID_HANDLE_VALUE)
		throw std::runtime_error("Invalid Handle value!");

	moduleEntry.dwSize = sizeof(moduleEntry);

	if (!Module32FirstW(hModule, &moduleEntry))
	{
		CloseHandle(hModule);
		throw std::runtime_error("Module32First error!");
	}

	if (moduleName == nullptr)
		moduleName = processName_;

	do
	{
		if (lstrcmpW(moduleEntry.szModule, moduleName) == 0)
		{
			CloseHandle(hModule);
			return (DWORD)moduleEntry.modBaseAddr;
		}
	} while (Module32NextW(hModule, &moduleEntry));

	CloseHandle(hModule);
	throw std::runtime_error("Could not find process!");
}

template<typename T>
bool Memory::readProcess(const DWORD addr, T data, const size_t size) const
{
	if (!ReadProcessMemory(hProcess_, (LPCVOID)addr, &data, size, nullptr))
		return false;
	return true;
}

template<typename T>
bool Memory::write(const DWORD addr, T data, const size_t size) const
{
	if (!WriteProcessMemory(hProcess_, (LPCVOID)addr, &data, size, nullptr))
		return false;
	return true;
}

template<typename T>
bool Memory::writeBuffer(const DWORD addr, T data, const size_t size) const
{
	if (!WriteProcessMemory(hProcess_, (LPCVOID)addr, data, size, nullptr))
		return false;
	return true;
}

std::wostream& operator<<(std::wostream& stream, const Memory& obj)
{
	return stream << "Process name: " << obj.processName_ << " process identifier: " << obj.pId_;
}