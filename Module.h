#ifndef ____MODULE____
#define ____MODULE____
#include "NoCopy.h"
#include <typeinfo>
#include "Type.h"
namespace nicehero
{
	template <typename T>
	class Module:public NoCopy
	{
		class Zero
		{
		public:
			operator int() const;
		private:
			char m_reserved[11];
		};
		class True
		{
		public:
			operator bool() const;
		private:
			char m_reserved[11];
		};
	public:
		static T * getInstance(void);
		bool isStarted() const;
		True checkStart();
		const char* getModuleName() const;
	protected:
		Module();
		virtual ~Module() = 0;
		Zero initial();
		Zero start();
		Zero run();
		Zero stop();

	private:
		bool			m_started;		
		unsigned int	m_refs;			
		const char *	m_moduleName;	
		static T*				m_instance;

		bool checkStop();

	};

	template <typename T>
	nicehero::Module<T>::~Module()
	{

	}

	template <typename T>
	nicehero::Module<T>::Module()
	{
		if (sizeof(static_cast<T *>(0)->initial()) == sizeof(int) || sizeof(static_cast<T *>(0)->initial()) == sizeof(bool))
		{
			initial();
		}
		m_moduleName = typeid(*this).name();
	}

	template <class T>
	Module<T>::Zero::operator int() const
	{
		return 0;
	}

	template <class T>
	Module<T>::True::operator bool() const
	{
		return true;
	}
	template <class T>
	bool Module<T>::isStarted() const
	{
		return m_started;
	}

	template <class T>
	typename Module<T>::True Module<T>::checkStart()
	{
		return True();
	}
	template <class T>
	typename Module<T>::Zero Module<T>::initial()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::start()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::run()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::stop()
	{
		return Zero();
	}

	template <class T>
	bool Module<T>::checkStop()
	{
		return m_refs == 0;
	}

	template <class T>
	const char* Module<T>::getModuleName() const
	{
		return m_moduleName;
	}

}

#define MODULE_IMPL(T) \
namespace nicehero \
{\
	template <>    \
	T* Module<T>::m_instance = nullptr;\
	template <>		\
	T * Module<T>::getInstance() \
	{				\
		if (!m_instance)\
		{m_instance = new T();}\
		return m_instance;	\
	}\
}\

#endif // !____MODULE____

