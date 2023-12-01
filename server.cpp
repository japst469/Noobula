#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "encryption.hpp"

void handleClient(int clientSocket);
void serverUpdateLoop();

constexpr int kPort = 63337;
constexpr int kMaxConnections = 128;

std::atomic_bool serverRunning(true);
std::vector<int> connections;
std::mutex connectionsMutex;
std::condition_variable connectionsCV;


// Function to handle a client connection
void handleClient(int clientSocket)
{
	// Add the client socket to the connection list
	{
		std::lock_guard<std::mutex> lock(connectionsMutex);
		connections.push_back(clientSocket);
	}
	connectionsCV.notify_one();

	// Process client's requests
	while (serverRunning)
	{
		// Simulating sending a packet of information to the client
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		// std::cout << "Sending packet to client " << clientSocket << std::endl;

		char buffer[1024] = {0};
		std::string response;

		// Receive data from the client
		int valread = read(clientSocket, buffer, 1024);
		if (valread < 0)
		{
			std::cerr << "Error reading from socket" << std::endl;
			break;
		}

		// Process the received data
		std::string data(buffer);
		std::cout << "Received from [" << clientSocket << "] client: " << data << std::endl;

		// Example processing (you can replace this with your own logic)
		// response.append( "Processed: " + data );

		std::string key = "01234567890123456789012345678901";
		std::string encrypted = encryptAES( response, key );
		response.append( encrypted );
		response += "\n";

		// Send response back to client
		send(clientSocket, response.c_str(), response.size(), 0);
	}

	// Remove the client socket from the connection list
	{
		std::lock_guard<std::mutex> lock(connectionsMutex);
		auto it = std::find(connections.begin(), connections.end(), clientSocket);
		if (it != connections.end())
		{
			connections.erase(it);
		}
	}

	// Close the client socket
	close(clientSocket);
}


// Server update loop
void serverUpdateLoop()
{
	while (serverRunning)
	{
		// Wait for a client connection
		std::unique_lock<std::mutex> lock(connectionsMutex);
		connectionsCV.wait(lock, []
											 { return !connections.empty(); });

		// Process each client connection
		for (const auto &clientSocket : connections)
		{
			// Perform update logic for each client connection
			// In this example, we are not performing any specific update logic.
		}
	}
}

int main()
{
	// Create a socket for the server
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == -1)
	{
		std::cerr << "Failed to create server socket." << std::endl;
		return 1;
	}
	ERR_print_errors_fp(stderr);

	// Set up server address information
	sockaddr_in serverAddress{};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(kPort);

	// Bind the server socket to the specified address
	if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == -1)
	{
		std::cerr << "Failed to bind server socket." << std::endl;
		close(serverSocket);
		return 1;
	}

	// Listen for client connections
	if (listen(serverSocket, kMaxConnections) == -1)
	{
		std::cerr << "Failed to listen for connections." << std::endl;
		close(serverSocket);
		return 1;
	}

	// Start the server update loop in a separate thread
	std::thread updateThread(serverUpdateLoop);

	// Accept and handle client connections
	while (serverRunning)
	{
		// Accept a client connection
		sockaddr_in clientAddress{};
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
		if (clientSocket == -1)
		{
			std::cerr << "Failed to accept client connection." << std::endl;
			continue;
		}
		// Handle the client connection in a separate thread
		std::thread clientThread(handleClient, clientSocket);
		clientThread.detach(); // Detach the thread and allow it to run independently
	}

	// Clean up and shutdown the server
	close(serverSocket);
	updateThread.join();

	return 0;
}


