#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>
#include "runClient.hpp"
#include "Testing.hpp"
#include "Net/SocketUtil.hpp"
#include "Net/SocketAddressFactory.hpp"
#include <fstream>

const std::string CONTENT_DELIMITER = ":";
const std::string FILE_ADDED    = "FA" + CONTENT_DELIMITER;
const std::string FILE_MODIFIED = "FM" + CONTENT_DELIMITER;
const std::string FILE_DELETED  = "FD" + CONTENT_DELIMITER;

// Define statics
std::string ClientGlobals::passableMessage = "";
std::mutex ClientGlobals::passableMessageMutex;
std::atomic<bool> ClientGlobals::canPollDirectory = false;

// Namespace inclusion
namespace fs = std::experimental::filesystem;

// Runs the client, taking a pointer to the address of the server. This is formatted as IP:Port.
void RunClient(const char* executableLocation, const char * serverAddress)
{
  const std::string path = std::string(executableLocation, std::string(executableLocation).find_last_of("\\"));
  const std::string executable = std::string(executableLocation).substr(std::string(executableLocation).find_last_of("\\") + 1);

  // Debug print and flush before threading
  DEBUG_PRINT_VAR(path);
  DEBUG_PRINT_VAR(executable);
  std::cout.flush();

  // Start second thread for file scanning
  std::thread helperThread([path]() {
    std::vector<fs::directory_entry> entries;

    // Wait to proceed.
    while (!ClientGlobals::canPollDirectory)
      std::this_thread::sleep_for(std::chrono::milliseconds(STANDARD_REFRESH_RATE_MS));

    // Run forever.
    size_t loops = 0;
    while (true)
    {
      // Directory entry reference
      std::vector<std::string> additions;            // Keeps track of any file additions.
      std::vector<bool> tags(entries.size(), false); // Keeps track of any file removals.

      // Iterate through directory
      for (auto &directry_entry : fs::directory_iterator(path))
      {
        bool add_to_vec = true;
        for (size_t i = 0; i < entries.size(); ++i)
          if (entries[i].path().filename() == directry_entry.path().filename())
          {
            add_to_vec = false;
            tags[i] = true;
            break;
          }

        if (add_to_vec)
        {
          additions.push_back(directry_entry.path().filename().generic_string());
          entries.push_back(directry_entry);
        }
      }

      std::string std_cout_msg_builder = "";

      // Handle any additions
      if (additions.size() > 0)
      {
        // Construct local log
        std_cout_msg_builder += "Detected file addition(s): \n";
        for (std::string &s : additions)
          std_cout_msg_builder += "  " + s + "\n";

        // Construct network message
        for (std::string &s : additions)
        {
          //FILE_ADDED + s + CONTENT_DELIMITER + 
        }
      }

      // Handle any deletions
      bool removal_title_displayed = false;
      for (size_t i = 0; i < tags.size();)
      {
        if (tags[i] == false)
        {
          if (!removal_title_displayed)
          {
            std_cout_msg_builder += "Detected file deletion(s): \n";
            removal_title_displayed = true;
          }

          std_cout_msg_builder += "  " + entries[i].path().filename().generic_string() + "\n";
          entries.erase(entries.begin() + i);
          tags.erase(tags.begin() + i);
        }
        else
          ++i;
      }

      // send constructed message
      if (std_cout_msg_builder.size() > 0)
        std::cout << std_cout_msg_builder << '\n';

      // Log sent message locally
      if (std_cout_msg_builder.size() != 0)
      {
        if (loops > 0)
          printf("\n");
        printf("%s", std_cout_msg_builder.c_str());
      }


      // Loop upkeep
      std::this_thread::sleep_for(std::chrono::milliseconds(STANDARD_REFRESH_RATE_MS));
      printf("\rRe-polling directory...%c", GetLoadingPattern(loops));
      ++loops;
    } // END WHILE(TRUE)
  });

  DEBUG_PRINT("Starting TCP client...");

  // Acquire socket connection. We just create a generic socket, and we point it in a direction.
  // In this case, we are providng it with an IP to connect to.
  TCPSocketPtr tcpSocket{ SocketUtil::CreateTCPSocket(SocketUtil::IPv4) };
  std::string IPAddr = std::string(serverAddress).append(":" + std::to_string(PORT)).c_str();
  DEBUG_PRINT_VAR(IPAddr);
  SocketAddressPtr addr{ SocketAddressFactory::CreateIPv4FromString(IPAddr.c_str()) };

  // This is a client socket, and we only have one of them, so setting it to non-blocking so we 
  // can just casually loop over it is a good idea. We wouldn't do this on the server because
  // asking every single socket manually can be a bit of a performance issue, although if you
  // only have a few connecting at a time then you're probably fine to do it anyways.
  tcpSocket->SetNonBlockingMode(true);

  // Wait for an initial connection - we will be sent a message when we get something.
  DEBUG_PRINT("Waiting to connect to " + IPAddr + "...");
  char data[BUFFER_SIZE + 1]{ 0 };
  size_t loops = 0;
  while (tcpSocket->Connect(*addr) < 0) {}
  while (true)
  {
    // Attempt connection
    tcpSocket->Connect(*addr);
    std::this_thread::sleep_for(std::chrono::milliseconds(STANDARD_REFRESH_RATE_MS * 2));

    // Recieve and process any data from the server here!
    unsigned int bytesRecieved{ static_cast<unsigned int>(tcpSocket->Recieve(data, BUFFER_SIZE)) };
    if (bytesRecieved > 1)
    {
      std::string toPrint{ data, bytesRecieved };
      if (loops > 0)
        std::cout << "\n";
      std::cout << "[Server]: " << toPrint << '\n';
      break;
    }

    // Sleep and retry
    std::this_thread::sleep_for(std::chrono::milliseconds(STANDARD_REFRESH_RATE_MS * 2));
    std::cout << "\r[!]: Retrying connection to " << IPAddr << "..." << GetLoadingPattern(loops);
    ++loops;
  }


  // We can go!
  DEBUG_PRINT("Connection Established!");
  ClientGlobals::canPollDirectory = true;

  // Are we running this loop?
  bool isRunning = true;
  bool hasServer = false;

  // Primary Loop.
  while (isRunning)
  {
    // Buffer Buffer, one more than the size because I don't trust myself with basic math.
    char data[BUFFER_SIZE + 1]{ 0 };

    // Check if we have any keyboard input!
    // If we do, we should make sure we strip the ending newline or whatever we want to do it it,
    // then send it on its way to the server.
    std::string msg = SharedMessage();
    if (msg.length() > 0)
    {
      tcpSocket->Send(msg.c_str(), msg.length());
      SharedMessageClear();
    }

    // Recieve and process any data from the server here!
    unsigned int bytesRecieved{ static_cast<unsigned int>(tcpSocket->Recieve(data, BUFFER_SIZE)) };
    if (bytesRecieved > 1)
    {
      std::string toPrint{ data, bytesRecieved };
      std::cout << "[Server]: " << toPrint << '\n';
    }

    // Don't scream eternal, give it a rest for like 16ms, which is approximately 
    // the time between loops in a game running at 60fps.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  helperThread.join();
}



  ///////////////////////////////////////////
 // Global helper function implementation //
///////////////////////////////////////////
// Safeuly setsd the global shared message so threads can communicate.
void SharedMessage(std::string message)
{
  // Lock the global message queue
  std::lock_guard<std::mutex> lock(ClientGlobals::passableMessageMutex);
  ClientGlobals::passableMessage = message;
}

// Safely returns a copy of the global message to be sent between threads
std::string SharedMessage()
{
  std::lock_guard<std::mutex> lock(ClientGlobals::passableMessageMutex);
  return std::string(ClientGlobals::passableMessage.c_str());
}

// Safely clears out the message to be passed between threads
void SharedMessageClear()
{
  std::lock_guard<std::mutex> lock(ClientGlobals::passableMessageMutex);
  ClientGlobals::passableMessage.clear();
}

// Returns a character indicating a wrapped index into ASCII loading character set
char GetLoadingPattern(int index)
{
  const char pattern[]{ '_','~','^', '\'','^','~','_' };
  index = index % sizeof(pattern) / sizeof(char);
  return pattern[index];
}

// Gets the size of the specified file.
std::ifstream::pos_type GetFilesize(const char* filename)
{
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}