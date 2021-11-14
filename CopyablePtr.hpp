#ifndef ____NICE__COPYABLEPTR____
#define ____NICE__COPYABLEPTR____
#include <memory>
namespace nicehero {

	/* 
		A copyable unique_ptr
	*/
	template <class T>
	class CopyablePtr {
	public:
		/** If value can be default-constructed, why not?
			Then we don't have to move it in */
		CopyablePtr() = default;

		/// Move a value in.
		explicit CopyablePtr(T&& t) = delete;
		explicit CopyablePtr(T* t) : value(t) {}

		/// copy is move
		CopyablePtr(const CopyablePtr& other) : value(std::move(other.value)) {}

		/// move is also move
		CopyablePtr(CopyablePtr&& other) : value(std::move(other.value)) {}

		const T& operator*() const {
			return (*value);
		}
		T& operator*() {
			return (*value);
		}
		T* get() {
			return value.get();
		}

		const T* operator->() const {
			return value.get();
		}
		T* operator->() {
			return value.get();
		}

		/// move the value out (sugar for std::move(*CopyablePtr))
		T&& move() {
			return std::move(value);
		}
		explicit operator bool() const noexcept {
			if (!value) return false;
			return true;
		}
		// If you want these you're probably doing it wrong, though they'd be
		// easy enough to implement
		CopyablePtr& operator=(CopyablePtr const&) = delete;
		CopyablePtr& operator=(CopyablePtr&& other) {
			value.release();
			value = std::move(other.value);
			return *this;
		}

	private:
		mutable std::unique_ptr<T> value;
	};

	/// Make a CopyablePtr from the argument. Because the name "makeCopyablePtr"
	/// is already quite transparent in its intent, this will work for lvalues as
	/// if you had wrapped them in std::move.
	template <class T, class... _Types>
	CopyablePtr<T> make_copyable(_Types&&... _Args) {
		return CopyablePtr<T>(new T(std::forward<_Types>(_Args)...));
	}

} // namespace folly

#endif
