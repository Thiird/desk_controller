#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

class SerialPort
{
public:
	SerialPort(char* portName, UINT baudRate);
	~SerialPort();
	
	int Read(char* buffer, UINT buffSize);
	int Write(char* buffer, UINT buffSize);
	bool isConnected();

private:
	HANDLE serialHandle;
	bool connected;
	COMSTAT status;
	UINT baudRate;
	DWORD errors;
};