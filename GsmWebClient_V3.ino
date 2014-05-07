/*
  Web client
 
 This sketch connects to a website through a GSM shield. Specifically,
 this example downloads the URL "http://arduino.cc/asciilogo.txt" and 
 prints it to the Serial monitor.
 
 Circuit:
 * GSM shield attached to an Arduino
 * SIM card with a data plan
 
 created 8 Mar 2012
 by Tom Igoe
 
 http://arduino.cc/en/Tutorial/GSMExamplesWebClient
 
 */

// libraries
#include <GSM.h>
#include <String.h>
// PIN Number
#define PINNUMBER ""

// APN data
#define GPRS_APN       "internet.com" // replace your GPRS APN
#define GPRS_LOGIN     "wapuser1"    // replace with your GPRS login
#define GPRS_PASSWORD  "wap" // replace with your GPRS password

// initialize the library instance
GSMClient client;
GPRS gprs;
GSM gsmAccess; 

// URL, path & port (for example: arduino.cc)
char server[] = "server.harvestrobotics.ca";
char path[] = "/index.html";
int port = 8000; // port 80 is the default for HTTP

char cmd[15];    // Nothing magical about 15, increase these if you need longer commands/parameters
char param1[32];
char param2[32];

#define bufferMax 128
int bufferSize;
char buffer[bufferMax];

boolean startRead = false; // is reading?
char inString[128]; // string for incoming serial data
int stringPos = 0; // string index counter
//int ledPin=13;
void setup()
{  
  pinMode(13,OUTPUT); digitalWrite(13,LOW);  // I use this pin as GND for the LED.
  pinMode(12,OUTPUT); digitalWrite(12,LOW); 
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println("Starting Arduino web client.");
  // connection state
  boolean notConnected = true;

  // After starting the modem with GSM.begin()
  // attach the shield to the GPRS network with the APN, login and password
  while(notConnected)
  {
    if((gsmAccess.begin(PINNUMBER)==GSM_READY) &
      (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)==GPRS_READY))
      notConnected = false;
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }

  Serial.println("connecting...");
  //post();
}

void loop()
{
   
  // if there are incoming bytes available 
  // from the server, read them and print them:
   Post();
   WaitForRequest();
   ParseReceivedRequest(); 
   PerformRequestedCommands();
  
  // if the server's disconnected, stop the client:
   //for(;;)
   delay(60000);   
}



void Post()
{
  Serial.println("connecting...");
  String PostData="Data={\"sensorId\":1,";
  unsigned char i;
  for(i=0;i<6;i++)
  {
    PostData=PostData+"\"data-";
    PostData=String(PostData+i);
    PostData=PostData+"\":";
    PostData=String(PostData + String(analogRead(i)));
    if(i!=5)
      PostData=PostData+",";
  }
    PostData=PostData+"}";  
  Serial.println(PostData);
  if (client.connect(server,port)) {
    Serial.println("connected");
    client.println("POST /index.html HTTP/1.1");
    client.println("Host: koshy");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json;");
    client.print("Content-Length: ");
    client.println(PostData.length());
    client.println();
    client.println(PostData);
  } else {
    Serial.println("connection failed");
  }
}

void WaitForRequest() // Sets buffer[] and bufferSize
{
  bufferSize = 0;
  stringPos = 0;
  memset( &inString, 0, 128 ); //clear inString memory
 
  while(true) 
  { 
    if (client.available()) 
    {
      char c = client.read();
      Serial.print(c);  
      if (c == '<' ) 
      { //'<' is our begining character
        startRead = true; //Ready to start reading the part 
      }
      else if(startRead)
      {

        if(c != '>')
        { //'>' is our ending character
          inString[stringPos] = c;
          stringPos ++;
        }
        else
        {
          //got what we need here! We can disconnect now
          startRead = false;
          client.stop();
          client.flush();
          Serial.println("disconnecting.");
          Serial.println( inString);

          break;   
      }

      }
    }

  }
  
  
  if (!client.available() && !client.connected())
  {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    //Serial.print("infinite for loop");
    // do nothing forevermore:
    //for(;;)
      //;
  }
  
  //PrintNumber("bufferSize", bufferSize);
}

void ParseReceivedRequest()
{
  Serial.println("in ParseReceivedRequest");
  Serial.println(inString);
  
  //Received buffer contains "GET /cmd/param1/param2 HTTP/1.1".  Break it up.
  char* slash1;
  char* slash2;
  char* slash3;
  char* space2;
  
  slash1 = strstr(inString, "/") + 1; // Look for first slash
  slash2 = strstr(slash1, "/") + 1; // second slash
  slash3 = strstr(slash2, "/") + 1; // third slash
  space2 = strstr(slash2, " ") + 1; // space after second slash (in case there is no third slash)
  if (slash3 > space2) slash3=slash2;

  //Serial.print("slash1 ");Serial.println(slash1);
  //Serial.print("slash2 ");Serial.println(slash2);
  //Serial.print("slash3 ");Serial.println(slash3);
  //Serial.print("space2 ");Serial.println(space2);

  
  // strncpy does not automatically add terminating zero, but strncat does! So start with blank string and concatenate.
  cmd[0] = 0;
  param1[0] = 0;
  param2[0] = 0;
  strncat(cmd, slash1, slash2-slash1-1);
  strncat(param1, slash2, slash3-slash2-1);
  strncat(param2, slash3, space2-slash3-1);
  
  Serial.print("cmd ");Serial.println(cmd);
  Serial.print("param1 ");Serial.println(param1);
  Serial.print("param2 ");Serial.println(param2);

}

void PerformRequestedCommands()
{
  if ( strcmp(cmd,"digitalWrite") == 0 ) RemoteDigitalWrite();
  //if ( strcmp(cmd,"analogRead") == 0 ) RemoteAnalogRead();
}

void RemoteDigitalWrite()
{  int ledPin;
  Serial.println("Remote digital write");

  if (param1[1])
  { 
     Serial.println("Multi digit port ");
    int tens =  param1[0] - '0';
    int units = param1[1] - '0';
    Serial.print(tens);
    Serial.println(units);
    int ledPin =(tens*10) +units;
    Serial.print("ledPin"); Serial.println(ledPin);
    int ledState = param2[0] - '0'; // Param2 should be either 1 or 0
    digitalWrite(ledPin, ledState);
   
  }
  else
  { 
    Serial.println("single digit port "); 
    int ledPin = param1[0] - '0'; // Param1 should be one digit port
    Serial.print("ledPin"); Serial.println(ledPin); 
    int ledState = param2[0] - '0'; // Param2 should be either 1 or 0
    digitalWrite(ledPin, ledState);
     
  }
 
}

