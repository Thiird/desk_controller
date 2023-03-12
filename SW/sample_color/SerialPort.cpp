#include "SerialPort.h"
#include <string>

SerialPort::SerialPort(char* portName, UINT baudRate)
{
	errors = 0;
	status = { 0 };
	connected = false;

	std::string name = "COM1";

	serialHandle = CreateFile(L"COM1",
	GENERIC_READ | GENERIC_WRITE,
	0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD errMsg = GetLastError();

	if(errMsg == 2)
		printf("Serial port was not found.\n");

	if (errMsg == 5)
		printf("Serial port is already in use.\n");

	if(errMsg != 0)
		return;

	DCB dcbSerialParameters = { 0 };

	if (!GetCommState(serialHandle, &dcbSerialParameters))
	{
		printf("Failed to get current serial parameters.\n");
		return;
	}

	dcbSerialParameters.BaudRate = baudRate;
	dcbSerialParameters.ByteSize = 8;
	dcbSerialParameters.StopBits = ONESTOPBIT;
	dcbSerialParameters.Parity = NOPARITY;
	//dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

	if(!SetCommState(serialHandle, &dcbSerialParameters))
	{
		printf("Could not set serial port parameters.\n");
		return;
	}

	connected = true;
	PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

SerialPort::~SerialPort()
{
	if(connected)
	{
		CloseHandle(serialHandle);
		connected = false;
	}
}

int SerialPort::Read(char* buffer, UINT buffSize)
{
	DWORD bytesRead;
	UINT toRead = 0;

	// I want to read, so clear any current error
	ClearCommError(serialHandle, &errors, &status);

	// cbInQue is bytes received but not yet read
	toRead = status.cbInQue;
	if(toRead > buffSize)
		toRead = buffSize;

	ZeroMemory((void *) buffer, buffSize);

	if (ReadFile(serialHandle, buffer, toRead, &bytesRead, NULL))
		return bytesRead;

	return 0;
}

int SerialPort::Write(char* buffer, UINT toWrite)
{
	DWORD bytesWritten;

	// I want to read, so clear any current error
	//ClearCommError(serialHandle, &errors, &status);
	
	if (WriteFile(serialHandle, buffer, toWrite, &bytesWritten, NULL))
		return bytesWritten;

	return bytesWritten;
}

bool SerialPort::isConnected()
{
	return connected;
}
