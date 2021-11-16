#ifndef __NICE_SERVER___
#define __NICE_SERVER___

#include "Type.h"
#include <string>
#include "NoCopy.h"

namespace nicehero
{
	const ui32 PUBLIC_KEY_SIZE = 64;
	const ui32 HASH_SIZE = 32;
	const ui32 SIGN_SIZE = 64;
	const ui32 PRIVATE_KEY_SIZE = 32;
	class Server
		:public NoCopy
	{
	public:
		Server();

		bool SetPrivateKeyString(const std::string& privateKeyString);
		std::string GetPrivateKeyString();
		std::string GetPublicKeyString();
		ui8 m_privateKey[PRIVATE_KEY_SIZE] = { 0 };
		ui8 m_publicKey[PUBLIC_KEY_SIZE] = { 0 };
	protected:
	private:

	};
}

#endif

