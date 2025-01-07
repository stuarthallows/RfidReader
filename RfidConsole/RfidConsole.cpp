// RfidConsole.cpp : Defines the entry point for the application.
//

#include "RfidConsole.h"
#include <filesystem>
#include <Windows.h>
#include <thread>

// https://github.com/jarro2783/cxxopts
// #include "cxxopts.hpp" // TODO import from include directory?

std::atomic<bool> monitoring{ false };
std::thread monitorThread;

// TODO move to another file
enum class TransferMode
{
	None,
	Clipboard,
	SendKeys,
	NamedPipe
};

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

// TODO https://learn.microsoft.com/en-us/cpp/code-quality/c6262?view=msvc-170

static std::vector<std::string> getAvailableSerialPorts() {
	std::vector<std::string> serialPorts;
	DWORD bufferSize = 65536;

	while (true) {
		// Dynamically allocate buffer to avoid stack overflow
		std::unique_ptr<char[]> devices(new char[bufferSize]);
		// Retrieve information about MS-DOS device names.
		DWORD charCount = QueryDosDeviceA(NULL, devices.get(), bufferSize);

		if (charCount == 0) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				bufferSize *= 2; // Double the buffer size and retry
				continue;
			}
			else {
				std::cerr << "Error querying devices. Error code: " << GetLastError() << std::endl;
				return serialPorts;
			}
		}

		char* currentDevice = devices.get();
		while (*currentDevice) {
			std::string deviceName(currentDevice);
			if (deviceName.rfind("COM", 0) == 0) { // Device name starts with "COM"
				serialPorts.push_back(deviceName);
			}
			currentDevice += strlen(currentDevice) + 1;
		}
		break;
	}

	return serialPorts;
}

void monitorSerialPorts() {
	std::vector<std::string> previousPorts;

	while (monitoring.load()) {
		auto currentPorts = getAvailableSerialPorts();

		// Check for new connections
		for (const auto& port : currentPorts) {
			if (std::find(previousPorts.begin(), previousPorts.end(), port) == previousPorts.end()) {
				std::cout << "Device connected: " << port << std::endl;
			}
		}

		// Check for disconnections
		for (const auto& port : previousPorts) {
			if (std::find(currentPorts.begin(), currentPorts.end(), port) == currentPorts.end()) {
				std::cout << "Device disconnected: " << port << std::endl;
			}
		}

		previousPorts = currentPorts;

		// Sleep for a while to avoid excessive CPU usage
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

int main(int argc, const char* argv[])
{
	// auto exePath = std::filesystem::path{ argv[0] };
	// auto appName = exePath.stem().string();
	//cxxopts::Options options(appName, "RFID command line reader");

	//options.add_options()
	//	("h,help", "Help page.")
	//	("start", "Start the reader when the application starts.") // TODO supports true and false?
	//	("mode", "How to transfer tags to another application.", cxxopts::value<TransferMode>()->default_value("Clipboard"))
	//	("format", "How to format tags when transferred", cxxopts::value<std::string>())
	//	("power", "Reader power in dBm clamped between 21 and 30", cxxopts::value<int>()->default_value("24"))
	//	("beep", "Beep when a tag is read")
	//	("port", "Only connect on the given COM port", cxxopts::value<std::string>())
	//	("probe-timeout", "Duration to probe a COM port to detect a reader in milliseconds", cxxopts::value<int>()->default_value("15"));

	//try
	//{
	//	auto result{ options.parse(argc, argv) };

	//	if (result.count("help"))
	//	{
	//		std::cout << options.help() << std::endl;
	//		return 0;
	//	}

	//	if (result.count("start"))
	//	{
	//		auto start{ result["start"].as<bool>() };
	//		std::cout << "Reader will start with application: " << start << "\n";
	//	}
	//	if (result.count("mode"))
	//	{
	//		auto mode{ result["mode"].as<TransferMode>() };
	//		std::cout << "mode: " << mode << "\n";
	//	}
	//	if (result.count("format"))
	//	{
	//		std::cout << "Transfer format\n";
	//	}
	//	if (result.count("power"))
	//	{
	//		std::cout << "Reader power: \n";
	//	}
	//	if (result.count("beep"))
	//	{
	//		std::cout << "Beep on read: \n";
	//	}
	//	if (result.count("port"))
	//	{
	//		std::cout << "Connect on port: \n";
	//	}
	//	if (result.count("probe-timeout"))
	//	{
	//		std::cout << "Probe duration: \n";
	//	}
	//}
	//catch (const std::exception& e)
	//{
	//	std::cerr << "Error parsing options: " << e.what() << std::endl;
	//	return 1;
	//}

	std::string command;
	std::cout << "Commands: start, stop, exit" << std::endl;

	while (true) {
		std::cout << "Enter command: ";
		std::getline(std::cin, command);

		if (command == "start") {
			if (monitoring.load()) {
				std::cout << "Already monitoring\n";
			}
			else {
				monitoring.store(true);
				monitorThread = std::thread(monitorSerialPorts);
				std::cout << "Monitoring started\n";
			}
		} else if (command == "stop") {
			if (!monitoring.load()) {
				std::cout << "Not monitoring\n";
			}
			else {
				monitoring.store(false);
				if (monitorThread.joinable()) {
					monitorThread.join();
				}
				std::cout << "Stopped monitoring\n";
			}
		} else if (command == "exit") {
			if (monitoring.load()) {
				monitoring.store(false);
				if (monitorThread.joinable()) {
					monitorThread.join();
				}
			}
			std::cout << "Exiting app";
			break;
		}
		else {
			std::cerr << "Unknown command. Try 'start', 'stop' or 'exit'" << std::endl;
		}
	}

	return 0;
}
