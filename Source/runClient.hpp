#pragma once
#include <mutex>
#include <atomic>

// Client-related Defines 
#define BUFFER_SIZE 1400 // Defines the maxiumum buffer size we're going to be using.
#define PORT 8004
#define STANDARD_REFRESH_RATE_MS 125



// Client-realted global values
struct ClientGlobals
{
  static std::string passableMessage;
  static std::mutex passableMessageMutex;
  static std::atomic<bool> canPollDirectory;
};

  ////////////////////////////////
 // Global function prototypes //
////////////////////////////////
void RunClient(const char*, const char *);

   ///////////////////////////////////////
 // Global helper function prototypes //
///////////////////////////////////////
void SharedMessage(std::string);
std::string SharedMessage();
void SharedMessageClear();
char GetLoadingPattern(int);
