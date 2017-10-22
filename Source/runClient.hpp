#pragma once
#include <mutex>
#include <atomic>

// Defines, etc.
#define BUFFER_SIZE 1400 // Defines the maxiumum buffer size we're going to be using.
#define PORT 8004
#define STANDARD_REFRESH_RATE_MS 125

// Globally passing things around because we can. Use a mutex.
struct ClientGlobals
{
  static std::string passableMessage;
  static std::mutex passableMessageMutex;
  static std::atomic<bool> canPollDirectory;
};

  ////////////////////////////////
 // Global Function Prototypes //
////////////////////////////////
void RunClient(const char*, const char *);
void SharedMessage(std::string message);
std::string SharedMessage();
void SharedMessageClear();
char GetLoadingPattern(int index);
