#include <string>

enum MessageType
{
	LOGIN,
	LOGINRESULT
};

struct MessageHeader {

	MessageHeader(int data_length, MessageType type) : _data_length(data_length), _message_type(type) {}

	int _data_length;
	MessageType _message_type;
};

struct Login : public MessageHeader {

	Login(const std::string& username, const std::string& password) : MessageHeader(sizeof(Login), MessageType::LOGIN) {
		std::strcpy(_username, username.c_str());
		std::strcpy(_password, password.c_str());
	}

	char _username[32];
	char _password[32];
	char data[32];
};

struct LoginResult : public MessageHeader {

	LoginResult(const std::string& login_result) : MessageHeader(sizeof(LoginResult), MessageType::LOGINRESULT) {
		std::strcpy(_login_result, login_result.c_str());
	}
	char _login_result[1024];
};