#include "Server.h"
#include <micro-ecc/uECC.h>
#include <memory.h>

namespace nicehero
{

	Server::Server()
	{
		if (!uECC_make_key(m_publicKey,m_privateKey, uECC_secp256k1())) {
			printf("uECC_make_key() failed\n");
			return;
		}
	}
	static ui8 get0xCharValue(char c)
	{
		if (c == '0')
		{
			return 0;
		}
		if (c >= '1' && c <= '9')
		{
			return 1 + (c - '1');
		}
		if (c >= 'a' && c <= 'f')
		{
			return 10 + (c - 'a');
		}
		if (c >= 'A' && c <= 'F')
		{
			return 10 + (c - 'A');
		}
		return 255;
	}
	bool Server::SetPrivateKeyString(const std::string& privateKeyString)
	{
		const char* s = privateKeyString.c_str();
		ui8 privateKey[PRIVATE_KEY_SIZE] = { 0 };
		ui8 publicKey[PUBLIC_KEY_SIZE] = { 0 };
		if (privateKeyString[0] == '0' && privateKeyString[1] == 'x')
		{
			if (privateKeyString.size() < 66)
			{
				return false;
			}
			s = privateKeyString.c_str() + 2;
		}
		else if (privateKeyString.size() < 64)
		{
			return false;
		}
		for (size_t i = 0;i < PRIVATE_KEY_SIZE;++ i)
		{
			ui8 a = get0xCharValue(s[i * 2 + 1]);
			ui8 b = get0xCharValue(s[i * 2]);
			if (a > 15 || b > 15)
			{
				return false;
			}
			privateKey[i] = b * 16 + a;
		}
		if (uECC_compute_public_key(privateKey, publicKey, uECC_secp256k1()) != 1)
		{
			return false;
		}
		memcpy(m_privateKey, privateKey, PRIVATE_KEY_SIZE);
		memcpy(m_publicKey, publicKey, PUBLIC_KEY_SIZE);
		return true;
	}

	std::string Server::GetPrivateKeyString()
	{
		char privateKey[256] = "0x";
		for (size_t i = 0; i < PRIVATE_KEY_SIZE;++ i)
		{
			snprintf(&privateKey[2 + i * 2],256 - 2 - i * 2, "%02x"	, m_privateKey[i]);
		}
		return privateKey;
	}

	std::string Server::GetPublicKeyString()
	{
		char publicKey[256] = "0x";
		for (size_t i = 0; i < PUBLIC_KEY_SIZE; ++i)
		{
			snprintf(&publicKey[2 + i * 2], 256 - 2 - i * 2, "%02x", m_publicKey[i]);
		}
		return publicKey;
	}

}