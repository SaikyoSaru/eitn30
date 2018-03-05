

class HTTPServer
{
public:

  char* extractString(char* thePosition, udword theLength);
  udword contentLength(char* theData, udword theLength);
  char* decodeBase64(char* theEncodedString);
  char* decodeForm(char* theEncodedForm);
private:
};
