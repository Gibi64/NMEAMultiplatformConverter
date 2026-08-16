
#include <string>
#include <memory>
#include "Protocole.h"
/* ------------------------------------------------------------------------ --
--                                                                          --
--                        PC serial port connection object                  --
--                           for non-event-driven programs                  --
--                                                                          --
--                                                                          --
--                                                                          --
--  Copyright @ 2001          Thierry Schneider                             --
--                            thierry@tetraedre.com                         --
--                                                                          --
--                                                                          --
--                                                                          --
-- ------------------------------------------------------------------------ --
--                                                                          --
--  Filename : CSerialClient.cpp                                                  --
--  Author   : Thierry Schneider                                            --
--  Created  : April 4th 2000                                               --
--  Modified : April 8th 2001                                               --
--  Plateform: Windows 95, 98, NT, 2000 (Win32)                             --
-- ------------------------------------------------------------------------ --
--                                                                          --
--  This software is given without any warranty. It can be distributed      --
--  free of charge as long as this header remains, unchanged.               --
--                                                                          --
-- ------------------------------------------------------------------------ */




/* ---------------------------------------------------------------------- */

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <conio.h>

#include "SerialClient.h"

/* -------------------------------------------------------------------- */
/* -------------------------    CSerialClient   ----------------------------- */
/* -------------------------------------------------------------------- */
CSerialClient::CSerialClient()
{
    parityMode       = spNONE;
    port[0]          = 0;
    rate             = 0;
    serial_handle    = INVALID_HANDLE_VALUE;
#ifdef _SOLARTRON

	SerialNetPort = gcnew System::IO::Ports::SerialPort ;
#endif
}

/* -------------------------------------------------------------------- */
/* --------------------------    ~CSerialClient     ------------------------- */
/* -------------------------------------------------------------------- */
CSerialClient::~CSerialClient()
{
#ifndef _SOLARTRON
    if (serial_handle!=INVALID_HANDLE_VALUE)
        CloseHandle(serial_handle);
    serial_handle = INVALID_HANDLE_VALUE;
#endif
}
/* -------------------------------------------------------------------- */
/* --------------------------    disconnect   ------------------------- */
/* -------------------------------------------------------------------- */
void CSerialClient::disconnect(void)
{
#ifndef _SOLARTRON

    if (serial_handle!=INVALID_HANDLE_VALUE)
        CloseHandle(serial_handle);
    serial_handle = INVALID_HANDLE_VALUE;
#else
	SerialNetPort->Close();
#endif
}
/* -------------------------------------------------------------------- */
/* --------------------------    connect      ------------------------- */
/* -------------------------------------------------------------------- */
int  CSerialClient::connect          (char *port_arg, int rate_arg, char parity,int number_of_bytes,int stop_bit)
{
#ifndef _SOLARTRON

	int erreur;
   DCB  dcb;
    COMMTIMEOUTS cto = { 0xFFFFFFFF, 5, 10, 5, 10 };
 //   COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };
    // --------------------------------------------- 
    if (serial_handle!=INVALID_HANDLE_VALUE)
        CloseHandle(serial_handle);
    serial_handle = INVALID_HANDLE_VALUE;

    erreur = 0;

    if (port_arg!=0)
    {
        strncpy_s(port, 10,port_arg, 10);
        rate      = rate_arg;

        memset(&dcb,0,sizeof(dcb));

        // -------------------------------------------------------------------- 
        // set DCB to configure the serial port
        dcb.DCBlength       = sizeof(dcb);                   
        
        // ---------- Serial Port Config ------- 
        dcb.BaudRate        = rate;

        switch(parity)
        {
            case 'N':
                            dcb.Parity      = NOPARITY;
                            dcb.fParity     = 0;
                            break;
            case 'E':
                            dcb.Parity      = EVENPARITY;
                            dcb.fParity     = 1;
                            break;
            case 'O':
                            dcb.Parity      = ODDPARITY;
                            dcb.fParity     = 1;
                            break;
        }


        dcb.StopBits        = ONESTOPBIT;
		if (stop_bit == 2) dcb.StopBits        = TWOSTOPBITS;
        dcb.ByteSize        = 8;
        if (number_of_bytes == 7) dcb.ByteSize        = 7;
        dcb.fOutxCtsFlow    = 0;
        dcb.fOutxDsrFlow    = 0;
        dcb.fDtrControl     = DTR_CONTROL_DISABLE;
        dcb.fDsrSensitivity = 0;
        dcb.fRtsControl     = RTS_CONTROL_DISABLE;
        dcb.fOutX           = 0;
        dcb.fInX            = 0;
        
        // ----------------- misc parameters ----- 
        dcb.fErrorChar      = 0;
        dcb.fBinary         = 1;
        dcb.fNull           = 0;
        dcb.fAbortOnError   = 0;
        dcb.wReserved       = 0;
        dcb.XonLim          = 100;
        dcb.XoffLim         = 100;
        dcb.XonChar         = 0;
        dcb.XoffChar        = 0;
        dcb.EvtChar         = 0;
		dcb.fInX=0;
		dcb.fOutX=0;
        
        // -------------------------------------------------------------------- 
        serial_handle    = CreateFileA(port, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING,NULL,NULL);
                   // opening serial port


        if (serial_handle    != INVALID_HANDLE_VALUE)
        {
            if(!SetCommMask(serial_handle, 0))
                erreur = 1;

            // set timeouts
            if(!SetCommTimeouts(serial_handle,&cto))
                erreur = 2;

            // set DCB
            if(!SetCommState(serial_handle,&dcb))
                erreur = 4;
			SetupComm(serial_handle,4096,4096);
        }
        else
            erreur = 8;
    }
    else
        erreur = 16;


    // --------------------------------------------- 
    if (erreur!=0)
    {
        CloseHandle(serial_handle);
        serial_handle = INVALID_HANDLE_VALUE;
    }
    return(erreur);	
#else

System::IO::Ports::Parity::Odd;
System::IO::Ports::Parity::Mark;
System::IO::Ports::Parity::None;
	SerialNetPort->BaudRate = rate_arg;
	gcroot<System::String ^> NetString = gcnew System::String(port_arg);
	SerialNetPort->PortName = NetString;
	switch (parity)
	{
	case 'E':
		SerialNetPort->Parity= System::IO::Ports::Parity::Even;
		break;
	case 'O':
		SerialNetPort->Parity = System::IO::Ports::Parity::Odd;
		break;
	case 'N':
		SerialNetPort->Parity = System::IO::Ports::Parity::None;
		break;
	}

	SerialNetPort->DataBits = number_of_bytes;
	System::IO::Ports::StopBits::None;
	System::IO::Ports::StopBits::Two;
	System::IO::Ports::StopBits::One;
	System::IO::Ports::StopBits::OnePointFive;
	switch (stop_bit)
	{
	case 0:
		SerialNetPort->StopBits = System::IO::Ports::StopBits::None;
		break;
	case 1:
		SerialNetPort->StopBits = System::IO::Ports::StopBits::One;
		break;
	case 2:
		SerialNetPort->StopBits = System::IO::Ports::StopBits::Two;
		break;
	default:
		SerialNetPort->StopBits = System::IO::Ports::StopBits::None;
		break;
	}



	SerialNetPort->Open();
	return 0;
#endif
}


/* -------------------------------------------------------------------- */
/* --------------------------    sendChar     ------------------------- */
/* -------------------------------------------------------------------- */
void CSerialClient::sendChar(char data)
{

#ifdef _SOLARTRON
	gcroot<System::String ^> NetString = gcnew System::String(&data);
	SerialNetPort->Write(NetString);
#else
    sendArray(&data, 1);
#endif	
}

/* -------------------------------------------------------------------- */
/* --------------------------    sendArray    ------------------------- */
/* -------------------------------------------------------------------- */
void CSerialClient::sendArray(char *buffer, int len)
{
#ifdef _SOLARTRON

	gcroot<System::String ^> NetString = gcnew System::String(buffer);
	SerialNetPort->Write(NetString);
#else
    unsigned long result;

    if (serial_handle!=INVALID_HANDLE_VALUE)
	{
		PurgeTransmitBuffer();
		PurgeReceiveBuffer();
        WriteFile(serial_handle, buffer, len, &result, NULL);
	}
	#endif


}

/* -------------------------------------------------------------------- */
/* --------------------------    getChar      ------------------------- */
/* -------------------------------------------------------------------- */
char CSerialClient::getChar(void)
{
#ifdef _SOLARTRON
	return SerialNetPort->ReadChar();
#else

    char c;
    getArray(&c, 1);
    return(c);
#endif
}

/* -------------------------------------------------------------------- */
/* --------------------------    getArray     ------------------------- */
/* -------------------------------------------------------------------- */
int  CSerialClient::getArray         (char *buffer, int len)
{
	unsigned long read_nbr;

#ifdef _SOLARTRON

	System::String const^ NetString;
	NetString =	SerialNetPort->ReadExisting();
	String^ string = const_cast<String^>(NetString);
	const wchar_t* chars = reinterpret_cast<const wchar_t*>((System::Runtime::InteropServices::Marshal::StringToHGlobalUni(string)).ToPointer());
	std::wstring os = chars;
	System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr((void*)chars));

	size_t nReceive;
	wcstombs_s(&nReceive, buffer, len,os.data(), (int)os.length());
	read_nbr = (int) os.length();
	if (read_nbr)
	{
		buffer[read_nbr] = 0;
		//		printf("%s\n", buffer);
	}
#else
    if (serial_handle!=INVALID_HANDLE_VALUE)
    {
        ReadFile(serial_handle, buffer, len, &read_nbr, NULL);
    }
    return((int) read_nbr);
#endif
}
/* -------------------------------------------------------------------- */
/* --------------------------    getNbrOfBytes ------------------------ */
/* -------------------------------------------------------------------- */
int CSerialClient::getNbrOfBytes    (void)
{
    struct _COMSTAT status;
    int             n;
    unsigned long   etat;

    n = 0;

    if (serial_handle!=INVALID_HANDLE_VALUE)
    {
        ClearCommError(serial_handle, &etat, &status);
        n = status.cbInQue;
    }


    return(n);
}




int CSerialClient::PurgeReceiveBuffer(void)
{
	PurgeComm(serial_handle,PURGE_RXCLEAR);
	return 0;
}

int CSerialClient::PurgeTransmitBuffer(void)
{
	PurgeComm(serial_handle,PURGE_TXCLEAR);
	return 0;
}
std::string* CSerialClient::GetBufferReponse()
{
	return m_pProtocole->GetBufferReponse();
}

int CSerialClient::ClientSendRequest(char *RequestToSend, int TypeOfWaiting, int nWait, int wait_shift, int time_out_in_ms)
{

	//				TypeOfWiting =0 -> Wait for specific character
	//				TypeOfWiting =1 -> Wait length of characters
	// Purge the communication buffer
//?	SerialNetPort->ReadExisting();
	GetBufferReponse()->clear();



	sendArray(RequestToSend, 512);

//				Wait for Answer
	time_t time_start = GetCurrentTime();
	int iReturn = 0;

	while (1)

	{
		int lMax = 512;
		std::unique_ptr<char> Mem = std::make_unique<char>(lMax+1);
		memset(Mem.get(), 0, lMax);
		int nReceive = getArray(Mem.get(), lMax);
		if (nReceive)
		{
			*GetBufferReponse() += Mem.get();
			if (ChekForEndOfRequestInput(TypeOfWaiting, nWait, wait_shift))
			{
				return 0;
			}
		}

		if (GetCurrentTime()> time_start + time_out_in_ms ) 
		{
			GetBufferReponse()->clear();
//			MessageBoxA(NULL, BufferReponse, "TimeOut", MB_OK);
			return 1;
		}


	}
	return 1;

}

bool CSerialClient::ChekForEndOfRequestInput(int TypeOfWaiting, int nWait,int wait_shift)
{
	int n = (int) GetBufferReponse()->length() - 1;
	int n_to_check = n - wait_shift;
	switch (TypeOfWaiting)
	{
	case 0:
		if (n_to_check >= 0)
		{ 
			if ((*GetBufferReponse())[n_to_check] == nWait)
			{
				GetBufferReponse()->clear();

				return true;
			}
		}
		break;
	case 1:
		if (n >= nWait) return true;
		break;
	}
	return false;
}

