#pragma once
#include <queue>
#include <bitset>

class Keyboard
{
	friend class Window;

public:
	class Event
	{
	public:
		enum class Type
		{
			Press,Release,Invalid
		};
	private:
		Type type;
		unsigned char code;
	public: 
		Event() noexcept
			:
			type(Type::Invalid), code(0u)
		{ }
		Event(Type type, unsigned char code) noexcept
			:
			type(type),code(code)
		{ }
		bool isPress() const noexcept
		{
			return type == Type::Press;
		}
		bool isRelease() const noexcept
		{
			return type == Type::Release;
		}
		bool isValid() const noexcept
		{
			return type != Type::Invalid;
		}
		unsigned char GetCode() const noexcept
		{
			return code;
		}
	};

public:
	Keyboard() = default;
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator=(const Keyboard&) = delete;
	//key event stuff
	bool KeyIsPressed(unsigned char keyword) const noexcept;
	Event ReadKey() noexcept;
	bool KeyIsEmpty() const noexcept;
	void FlushKey() noexcept;
	//char event stuff
	char ReadChar() noexcept;
	bool CharIsEmpty() const noexcept;
	void FlushChar() noexcept;
	void Flush() noexcept;
	//autorepeat stuff
	void EnableAutoRepeat() noexcept;
	void DisableAutoRepeat() noexcept;
	bool AutoRepeatIsEnabled() const noexcept;
private:
	void OnKeyPressed(unsigned char keyword) noexcept;
	void OnKeyReleased(unsigned char keyword) noexcept;
	void OnChar(char character) noexcept;
	void ClearState() noexcept;
	template <typename T>
	static void TrimBuffer(std::queue<T>& buffer ) noexcept;
private:
	static constexpr unsigned int nKeys = 256u;
	static constexpr unsigned int bufferSize = 16u;
	bool autorepeatEnabled = false;
	std::bitset<nKeys>keystates;
	std::queue<Event> keybuffer;
	std::queue<char> charbuffer;




};