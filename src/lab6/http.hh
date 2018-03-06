
#ifndef http_hh
#define http_hh


#include "tcp.hh"
#include "job.hh"
#include "threads.hh"
#include "fs.hh"


class HTTPServer : public Job
{
public:
  HTTPServer(TCPSocket* theSocket); // wip constructor
  ~HTTPServer();
  char* extractString(char* thePosition, udword theLength);
  udword contentLength(char* theData, udword theLength);
  char* decodeBase64(char* theEncodedString);
  char* decodeForm(char* theEncodedForm);
  char* findPathName(char* str);
  char* findFileName(char* str);
  void  doit();
private:
  TCPSocket* mySocket;
  FileSystem* fs;
};


#endif
