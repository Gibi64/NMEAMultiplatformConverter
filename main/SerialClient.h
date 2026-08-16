


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
--  Filename : sertest2.cpp                                                 --
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
#ifndef CSerialClient_H
#define CSerialClient_H

#include <stdio.h>
#include <windows.h>
class CProtocole;

enum serial_parity  { spNONE,    spODD, spEVEN };


/* -------------------------------------------------------------------- */
/* -----------------------------  CSerialClient  ---------------------------- */
/* -------------------------------------------------------------------- */
class CSerialClient
{
    // -------------------------------------------------------- //
protected:
    char              port[10];                      // port name "com1",...
    int               rate;                          // baudrate
    serial_parity     parityMode;
    HANDLE            serial_handle;                 // ...
    
	
    // ++++++++++++++++++++++++++++++++++++++++++++++
    // .................. EXTERNAL VIEW .............
    // ++++++++++++++++++++++++++++++++++++++++++++++
public:
    HANDLE GetSerialHandle()
    {
        return serial_handle;
    }
                  CSerialClient();
                 ~CSerialClient();
				 void InvalidateHandle()
				 {
					 serial_handle = INVALID_HANDLE_VALUE;
				 }
    int           connect          (char *port_arg, int rate_arg,
                                    char parity,int number_of_bytes,int stop_bit);
    void          sendChar         (char c);
    void          sendArray        (char *buffer, int len);
    char          getChar          (void);
    int           getArray         (char *buffer, int len);
    int           getNbrOfBytes    (void);
    void          disconnect       (void);
	char BufferCommande[512];
    CProtocole* m_pProtocole;
    std::string *GetBufferReponse();
	char cAttente;
	int PurgeReceiveBuffer(void);
	int PurgeTransmitBuffer();
#ifdef _SOLARTRON

	gcroot<System::IO::Ports::SerialPort ^> SerialNetPort;
#endif
	int ClientSendRequest(char *RequestToSend, int TypeOfWaiting, int nWait,int wait_shift, int time_out_in_ms);
private:
	bool ChekForEndOfRequestInput(int TypeOfWaiting, int nWait,int wait_shift);
};
/* -------------------------------------------------------------------- */

#endif CSerialClient_H


