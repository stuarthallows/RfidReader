// RfidConsole.cpp : Defines the entry point for the application.
//

#include "RfidConsole.h"
#include <filesystem>

// https://github.com/jarro2783/cxxopts
#include "cxxopts.hpp" // TODO import from include directory?

using namespace std;

// TODO move to another file
enum class TransferMode
{
	None,
	Clipboard,
	SendKeys,
	NamedPipe
};

// TODO Consider using magic_enum library to automate mapping from enum to string.
static std::ostream& operator<<(std::ostream& os, TransferMode mode) 
{
	switch (mode) 
	{
		case TransferMode::None:       return os << "None";
		case TransferMode::Clipboard:  return os << "Clipboard";
		case TransferMode::SendKeys:   return os << "SendKeys";
		case TransferMode::NamedPipe:  return os << "NamedPipe";
		default:                       return os << "Unknown TransferMode";
	}
}

static std::istream& operator>>(std::istream& is, TransferMode& mode)
{
	std::string token;
	is >> token;
	if (token == "None") mode = TransferMode::None;
	else if (token == "Clipboard") mode = TransferMode::Clipboard;
	else if (token == "SendKeys") mode = TransferMode::SendKeys;
	else if (token == "NamedPipe") mode = TransferMode::NamedPipe;
	else throw std::invalid_argument("Invalid TransferMode: " + token);
	return is;
}

int main(int argc, const char* argv[])
{
	auto exePath = std::filesystem::path{ argv[0] };
	auto appName = exePath.stem().string();
	cxxopts::Options options(appName, "RFID command line reader");

	// TODO test options, add tests
	// TODO encapsulate parsing
	options.add_options()
		("h,help", "Help page.")
		("start", "Start the reader when the application starts.") // TODO supports true and false?
		("mode", "How to transfer tags to another application.", cxxopts::value<TransferMode>()->default_value("Clipboard"))
		("format", "How to format tags when transferred", cxxopts::value<std::string>())
		("power", "Reader power in dBm clamped between 21 and 30", cxxopts::value<int>()->default_value("24"))
		("beep", "Beep when a tag is read")
		("port", "Only connect on the given COM port", cxxopts::value<std::string>())
		("probe-timeout", "Duration to probe a COM port to detect a reader in milliseconds", cxxopts::value<int>()->default_value("15"));

	try
	{
		auto result{ options.parse(argc, argv) };

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			return 0;
		}

		if (result.count("start"))
		{
			auto start{ result["start"].as<bool>() };
			std::cout << "Reader will start with application: " << start << "\n";
		}
		if (result.count("mode"))
		{
			auto mode{ result["mode"].as<TransferMode>() };
			std::cout << "mode: " << mode << "\n";
		}
		if (result.count("format"))
		{
			std::cout << "Transfer format\n";
		}
		if (result.count("power"))
		{
			std::cout << "Reader power: \n";
		}
		if (result.count("beep"))
		{
			std::cout << "Beep on read: \n";
		}
		if (result.count("port"))
		{
			std::cout << "Connect on port: \n";
		}
		if (result.count("probe-timeout"))
		{
			std::cout << "Probe duration: \n";
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error parsing options: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
