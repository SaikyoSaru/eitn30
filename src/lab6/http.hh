
#ifndef http_hh
#define http_hh

#include "fs.hh"
#include "job.hh"
#include "tcp.hh"
#include "threads.hh"

class HTTPServer : public Job {
public:
  HTTPServer(TCPSocket *theSocket); // wip constructor
  ~HTTPServer();
  char *extractString(char *thePosition, udword theLength);
  udword contentLength(char *theData, udword theLength);
  char *decodeBase64(char *theEncodedString);
  char *decodeForm(char *theEncodedForm);
  char *findPathName(char *str);
  char *findFileName(char *str);
  bool authentication(char *header);
  void getRequest(char *header, udword aLength);
  void postRequest(char *header, udword aLength);
  void doit();

private:
  TCPSocket *mySocket;
  FileSystem *fs;
  byte *postBuffer;
  udword dataArrived;
  udword contLen;
  char *savedPath;
  char *savedFileName;
  bool done;
};

#endif
