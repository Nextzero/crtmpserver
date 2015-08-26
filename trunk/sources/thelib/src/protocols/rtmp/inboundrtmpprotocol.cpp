/*
 *  Copyright (c) 2010,
 *  Gavriloaie Eugen-Andrei (shiretu@gmail.com)
 *
 *  This file is part of crtmpserver.
 *  crtmpserver is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  crtmpserver is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with crtmpserver.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAS_PROTOCOL_RTMP
#include "protocols/rtmp/inboundrtmpprotocol.h"
#include "protocols/rtmp/rtmpeprotocol.h"
#include "protocols/rtmp/basertmpappprotocolhandler.h"

//#define DEBUG_HANDSHAKE(...) do{printf("%6d - ",__LINE__);printf(__VA_ARGS__);printf("\n");} while(0)
#define DEBUG_HANDSHAKE(...)

InboundRTMPProtocol::InboundRTMPProtocol()
: BaseRTMPProtocol(PT_INBOUND_RTMP) {
	_pKeyIn = NULL;
	_pKeyOut = NULL;
	_pOutputBuffer = NULL;
	_currentFPVersion = 0;
	_usedScheme = 0;
}

InboundRTMPProtocol::~InboundRTMPProtocol() {
	if (_pKeyIn != NULL) {
		delete _pKeyIn;
		_pKeyIn = NULL;
	}

	if (_pKeyOut != NULL) {
		delete _pKeyOut;
		_pKeyOut = NULL;
	}

	if (_pOutputBuffer != NULL) {
		delete[] _pOutputBuffer;
		_pOutputBuffer = NULL;
	}
}

bool InboundRTMPProtocol::PerformHandshake(IOBuffer &buffer) {
	switch (_rtmpState) {
		case RTMP_STATE_NOT_INITIALIZED:
		{
			if (GETAVAILABLEBYTESCOUNT(buffer) < 1537) {
				return true;
			}
			uint8_t handshakeType = GETIBPOINTER(buffer)[0];
			if (!buffer.Ignore(1)) {
				FATAL("Unable to ignore one byte");
				return false;
			}

			_currentFPVersion = ENTOHLP(GETIBPOINTER(buffer) + 4);

			switch (handshakeType) {
				case 3: //plain
				{
					return PerformHandshake(buffer, false);
				}
				case 6: //encrypted
				{
					return PerformHandshake(buffer, true);
				}
				default:
				{
					FATAL("Handshake type not implemented: %hhu", handshakeType);
					return false;
				}
			}
		}
		case RTMP_STATE_SERVER_RESPONSE_SENT:
		{
			if (GETAVAILABLEBYTESCOUNT(buffer) < 1536) {
				return true;
			} else {
				//ignore the client's last handshake part
				if (!buffer.Ignore(1536)) {
					FATAL("Unable to ignore inbound data");
					return false;
				}
				_handshakeCompleted = true;
				_rtmpState = RTMP_STATE_DONE;

				if (_pKeyIn != NULL && _pKeyOut != NULL) {
					//insert the RTMPE protocol in the current protocol stack
					BaseProtocol *pFarProtocol = GetFarProtocol();
					RTMPEProtocol *pRTMPE = new RTMPEProtocol(_pKeyIn, _pKeyOut);
					ResetFarProtocol();
					pFarProtocol->SetNearProtocol(pRTMPE);
					pRTMPE->SetNearProtocol(this);
					//FINEST("New protocol chain: %s", STR(*pFarProtocol));

					//decrypt the leftovers
					RC4(_pKeyIn, GETAVAILABLEBYTESCOUNT(buffer),
							GETIBPOINTER(buffer),
							GETIBPOINTER(buffer));
				}

				return true;
			}
		}
		default:
		{
			FATAL("Invalid RTMP state: %d", _rtmpState);
			return false;
		}
	}
}

bool InboundRTMPProtocol::ValidateClient(IOBuffer &inputBuffer) {
	if (_currentFPVersion == 0) {
		//WARN("This version of player doesn't support validation");
		return false;
	}
	if (ValidateClientScheme(inputBuffer, 0)) {
		_usedScheme = 0;
		return true;
	}
	if (ValidateClientScheme(inputBuffer, 1)) {
		_usedScheme = 1;
		return true;
	}
	FATAL("Unable to validate client");
	return false;
}

bool InboundRTMPProtocol::ValidateClientScheme(IOBuffer &inputBuffer, uint8_t scheme) {
	DEBUG_HANDSHAKE("SERVER: Validate: 1. _usedScheme %"PRIu8, scheme);
	uint8_t *pBuffer = GETIBPOINTER(inputBuffer);

	uint32_t clientDigestOffset = GetDigestOffset(pBuffer, scheme);
	DEBUG_HANDSHAKE("SERVER: Validate: 2. clientDigestOffset %"PRIu32"; _usedScheme: %"PRIu8, clientDigestOffset, scheme);

	uint8_t *pTempBuffer = new uint8_t[1536 - 32];
	memcpy(pTempBuffer, pBuffer, clientDigestOffset);
	memcpy(pTempBuffer + clientDigestOffset, pBuffer + clientDigestOffset + 32,
			1536 - clientDigestOffset - 32);

	uint8_t *pTempHash = new uint8_t[512];
	HMACsha256(pTempBuffer, 1536 - 32, genuineFPKey, 30, pTempHash);
	DEBUG_HANDSHAKE("SERVER: Validate: 3. computed clientDigest %s", STR(hex(pTempHash, 32)));
	DEBUG_HANDSHAKE("SERVER: Validate: 4.    found clientDigest %s", STR(hex(pBuffer + clientDigestOffset, 32)));

	int result = memcmp(pTempHash, pBuffer + clientDigestOffset, 32);

	delete[] pTempBuffer;
	delete[] pTempHash;

	return result == 0;
}

bool InboundRTMPProtocol::PerformHandshake(IOBuffer &buffer, bool encrypted) {
	if (ValidateClient(buffer)) {
		return PerformComplexHandshake(buffer, encrypted);
	} else {
		if (encrypted || _pProtocolHandler->ValidateHandshake()) {
			FATAL("Unable to validate client");
			return false;
		} else {
			return PerformSimpleHandshake(buffer);
		}
	}
}

bool InboundRTMPProtocol::PerformSimpleHandshake(IOBuffer &buffer) {
	if (_pOutputBuffer == NULL) {
		_pOutputBuffer = new uint8_t[1536];
	} else {
		delete[] _pOutputBuffer;
		_pOutputBuffer = new uint8_t[1536];
	}

	for (uint32_t i = 0; i < 1536; i++) {
		_pOutputBuffer[i] = rand() % 256;
	}
	for (uint32_t i = 0; i < 10; i++) {
		uint32_t index = (rand() + 8) % (1536 - HTTP_HEADERS_SERVER_US_LEN);
		memcpy(_pOutputBuffer + index, HTTP_HEADERS_SERVER_US, HTTP_HEADERS_SERVER_US_LEN);
	}

	_outputBuffer.ReadFromByte(3);
	_outputBuffer.ReadFromBuffer(_pOutputBuffer, 1536);
	_outputBuffer.ReadFromBuffer(GETIBPOINTER(buffer), 1536);

	//final cleanup
	delete[] _pOutputBuffer;
	_pOutputBuffer = NULL;
	if (!buffer.Ignore(1536)) {
		FATAL("Unable to ignore input buffer");
		return false;
	}

	//signal outbound data
	if (!EnqueueForOutbound()) {
		FATAL("Unable to signal outbound data");
		return false;
	}

	//move to the next stage in the handshake
	_rtmpState = RTMP_STATE_SERVER_RESPONSE_SENT;

	return true;
}

bool InboundRTMPProtocol::PerformComplexHandshake(IOBuffer &buffer, bool encrypted) {
	//get the buffers
	uint8_t *pInputBuffer = GETIBPOINTER(buffer);
	if (_pOutputBuffer == NULL) {
		_pOutputBuffer = new uint8_t[3072];
	} else {
		delete[] _pOutputBuffer;
		_pOutputBuffer = new uint8_t[3072];
	}

	//timestamp
	EHTONLP(_pOutputBuffer, (uint32_t) time(NULL));

	//version
	EHTONLP(_pOutputBuffer + 4, (uint32_t) 0x00000000);

	//generate random data
	for (uint32_t i = 8; i < 3072; i++) {
		_pOutputBuffer[i] = rand() % 256;
	}
	for (uint32_t i = 0; i < 10; i++) {
		uint32_t index = rand() % (3072 - HTTP_HEADERS_SERVER_US_LEN);
		memcpy(_pOutputBuffer + index, HTTP_HEADERS_SERVER_US, HTTP_HEADERS_SERVER_US_LEN);
	}

	//**** FIRST 1536 bytes from server response ****//
	//compute DH key position
	uint32_t serverDHOffset = GetDHOffset(_pOutputBuffer, _usedScheme);
	uint32_t clientDHOffset = GetDHOffset(pInputBuffer, _usedScheme);
	DEBUG_HANDSHAKE("SERVER: 1. serverDHOffset: %"PRIu32"; clientDHOffset: %"PRIu32"; _usedScheme: %"PRIu8,
			serverDHOffset, clientDHOffset, _usedScheme);

	//generate DH key
	DHWrapper dhWrapper(1024);

	if (!dhWrapper.Initialize()) {
		FATAL("Unable to initialize DH wrapper");
		return false;
	}

	DEBUG_HANDSHAKE("SERVER: 2. clientDHOffset: %"PRIu32"; _usedScheme: %"PRIu8"; clientPublicKey: %s",
			clientDHOffset,
			_usedScheme,
			STR(hex(pInputBuffer + clientDHOffset, 128)));
	if (!dhWrapper.CreateSharedKey(pInputBuffer + clientDHOffset, 128)) {
		FATAL("Unable to create shared key");
		return false;
	}

	if (!dhWrapper.CopyPublicKey(_pOutputBuffer + serverDHOffset, 128)) {
		FATAL("Couldn't write public key!");
		return false;
	}
	DEBUG_HANDSHAKE("SERVER: 3. serverDHOffset: %"PRIu32"; serverPublicKey: %s",
			serverDHOffset,
			STR(hex(_pOutputBuffer + serverDHOffset, 128)));

	if (encrypted) {
		uint8_t secretKey[128];
		if (!dhWrapper.CopySharedKey(secretKey, sizeof (secretKey))) {
			FATAL("Unable to copy shared key");
			return false;
		}
		DEBUG_HANDSHAKE("SERVER: 4. secretKey: %s", STR(hex(secretKey, 128)));

		_pKeyIn = new RC4_KEY;
		_pKeyOut = new RC4_KEY;
		InitRC4Encryption(
				secretKey,
				(uint8_t*) & pInputBuffer[clientDHOffset],
				(uint8_t*) & _pOutputBuffer[serverDHOffset],
				_pKeyIn,
				_pKeyOut);

		//bring the keys to correct cursor
		uint8_t data[1536];
		RC4(_pKeyIn, 1536, data, data);
		RC4(_pKeyOut, 1536, data, data);
	}

	//generate the digest
	uint32_t serverDigestOffset = GetDigestOffset(_pOutputBuffer, _usedScheme);

	uint8_t *pTempBuffer = new uint8_t[1536 - 32];
	memcpy(pTempBuffer, _pOutputBuffer, serverDigestOffset);
	memcpy(pTempBuffer + serverDigestOffset, _pOutputBuffer + serverDigestOffset + 32,
			1536 - serverDigestOffset - 32);

	uint8_t *pTempHash = new uint8_t[512];
	HMACsha256(pTempBuffer, 1536 - 32, genuineFMSKey, 36, pTempHash);

	//put the digest in place
	memcpy(_pOutputBuffer + serverDigestOffset, pTempHash, 32);
	DEBUG_HANDSHAKE("SERVER: 5. serverDigestOffset: %"PRIu32"; _usedScheme: %"PRIu8"; serverDigest: %s",
			serverDigestOffset, _usedScheme,
			STR(hex(pTempHash, 32)));

	//cleanup
	delete[] pTempBuffer;
	delete[] pTempHash;


	//**** SECOND 1536 bytes from server response ****//
	//Compute the chalange index from the initial client request
	uint32_t clientDigestOffset = GetDigestOffset(pInputBuffer, _usedScheme);
	DEBUG_HANDSHAKE("SERVER: 6. clientDigestOffset: %"PRIu32"; _usedScheme: %"PRIu8, clientDigestOffset, _usedScheme);

	//compute the key
	pTempHash = new uint8_t[512];
	HMACsha256(pInputBuffer + clientDigestOffset, //pData
			32, //dataLength
			BaseRTMPProtocol::genuineFMSKey, //key
			68, //keyLength
			pTempHash //pResult
			);

	//generate the hash
	uint8_t *pLastHash = new uint8_t[512];
	HMACsha256(_pOutputBuffer + 1536, //pData
			1536 - 32, //dataLength
			pTempHash, //key
			32, //keyLength
			pLastHash //pResult
			);

	//put the hash where it belongs
	memcpy(_pOutputBuffer + 1536 * 2 - 32, pLastHash, 32);
	DEBUG_HANDSHAKE("SERVER: 7. serverChallange: %s", STR(hex(pLastHash, 32)));


	//cleanup
	delete[] pTempHash;
	delete[] pLastHash;
	//***** DONE BUILDING THE RESPONSE ***//


	//wire the response
	if (encrypted)
		_outputBuffer.ReadFromByte(6);
	else
		_outputBuffer.ReadFromByte(3);
	_outputBuffer.ReadFromBuffer(_pOutputBuffer, 3072);

	//final cleanup
	delete[] _pOutputBuffer;
	_pOutputBuffer = NULL;
	if (!buffer.IgnoreAll()) {
		FATAL("Unable to ignore input buffer");
		return false;
	}

	//signal outbound data
	if (!EnqueueForOutbound()) {
		FATAL("Unable to signal outbound data");
		return false;
	}

	//move to the next stage in the handshake
	_rtmpState = RTMP_STATE_SERVER_RESPONSE_SENT;

	return true;
}

#endif /* HAS_PROTOCOL_RTMP */

