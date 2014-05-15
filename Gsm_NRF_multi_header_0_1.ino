
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


#include<avr/sleep.h>
#include <GSM.h>
#include <String.h>

#include "nRF24L01.h"
#include "RF24.h"
#include <SPI.h>
// PIN Number
#define PINNUMBER ""

// APN data
#define GPRS_APN       "internet.com" // replace your GPRS APN
#define GPRS_LOGIN     "wapuser1"    // replace with your GPRS login
#define GPRS_PASSWORD  "wap" // replace with your GPRS password

#define BASEBROADCAST(x) (0xBB00000000LL + x)
#define RELAYBROADCAST(x) (0xAA00000000LL + x)
#define NODEACK(x) (0xCC00000000LL + x)

#define NODES 5

struct SENSOR{
  float temp;
  float humidity;
  float pressure;
};

struct HEADER{
  long type;
  long hops;
  long src;
  long ID;
  SENSOR sensor;
};

  HEADER header[NODES];        //used throughout

long cnt[10] = {};    //array used to store the last 10 header.IDs
byte cntID = 0;       //counter for header.ID array
int hdrcnt =0;  

String PostData ="";

// initialize the library instance
GSMClient client;
GPRS gprs;
GSM gsmAccess; 

RF24 radio(4,5);

// URL, path & port (for example: arduino.cc)
char server[] = "server.harvestrobotics.ca";
char path[] = "/index.html";
int port = 8000; // port 80 is the default for HTTP

char cmd[15];    // Nothing magical about 15, increase these if you need longer commands/parameters
char param1[32];
char param2[32];

boolean startRead = false; // is reading?
char inString[128]; // string for incoming serial data
int stringPos = 0; // string index counter
//int ledPin=13;

char strtemp[8];
char strhumid[8];
char strpress[8];

boolean notConnected = true;

void setup()
{  
  pinMode(13,OUTPUT); digitalWrite(13,LOW);  // I use this pin as GND for the LED.
  pinMode(12,OUTPUT); digitalWrite(12,LOW); 
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  randomSeed(analogRead(0));                        //here just for the testing bit on the bottom
  //setup radio
  radio.begin();
  radio.setRetries( 15,15 );
  radio.enableDynamicPayloads();
  radio.openReadingPipe( 1, RELAYBROADCAST(2) );    //relays send on this
  radio.openReadingPipe( 2, RELAYBROADCAST(1) );    //Nodes send on this
  radio.startListening();
  
 Serial.println( " Starting " );



  
}

void loop()
{    
    
  //Serial.println("waiting for radio.available()");  
    //check to see if we have anything available
   if ( radio.available() ){
      bool done = false;
      while ((!done) && (hdrcnt <= NODES)){
        
        //read whatever is available
        done = radio.read( &header[hdrcnt], radio.getDynamicPayloadSize() );
        Serial.print( "Got message from 0x" ); Serial.print( header[hdrcnt].src, HEX );Serial.print( " ID:" );Serial.print( header[hdrcnt].ID, HEX ); Serial.print( " Hops: " );Serial.println(header[hdrcnt].hops);
        }
        delay(20);    //wait a bit for node to switch to receiver
        Serial.print("Sending out ACK for ");Serial.print( " ID:" );Serial.print( header[hdrcnt].ID, HEX );Serial.print(" hdrcnt = " );Serial.println( hdrcnt );
        radio.stopListening();
        radio.openWritingPipe( NODEACK(1) );
        radio.write( &header[hdrcnt].ID, sizeof(header[hdrcnt].ID), true );    //send out ack with the id of our received message
        radio.startListening();
        delay(20);
        //this could be an original node broadcast or it could be another relay's tx.
        //they both get forwarded, DupID is used to stay out of an infinite loops of relays sending relays sending relays...
        //so far remembering the last ten has worked. this might not work on a larger scale. Needs more testing.
      
        // if there are incoming bytes available 
        // from the server, read them and print them:
        
        hdrcnt++ ; //increase the header count 
       
  
 
    }
  
  String concatString="[";   

    if (hdrcnt == NODES)
    {
      startConnection();
      concatData(concatString);
      Serial.print("concat string returned is "); Serial.println(concatString);
      Post(concatString);
      WaitForRequest();
      ParseReceivedRequest(); 
      PerformRequestedCommands();  
    //delay(100);
      hdrcnt =0; 
    }
  

  
  // if the server's disconnected, stop the client:
   //for(;;)
   //delay(10000);   
}

void concatData(String &PostData)
{
  //String PostData= "";
  Serial.println("Concating data");

  for( int i=0; i<NODES ;i++) {
      
      PostData= PostData+"{\"sensorId\":"+String(header[i].src,HEX);
      PostData=PostData+",";
      
      float ft_temp = header[i].sensor.temp;  
      float ft_humid = header[i].sensor.humidity; 
      float ft_press = header[i].sensor.pressure; 
      //Serial.print(ft_temp); Serial.print(ft_humid);Serial.println(ft_press);
      char tmp[20];
      //char tmp2[20];
      //char tmp3[20];
  
      PostData=PostData+"\"msgId";
      PostData=PostData+"\":";
      PostData=String(PostData + String(header[i].ID,HEX));
      PostData=PostData+",";
  
  
      dtostrf(ft_temp,1,2,tmp);  
    
      PostData=PostData+"\"Temp";
      PostData=PostData+"\":";
      PostData=String(PostData + String(tmp));
      PostData=PostData+",";

      dtostrf(ft_humid,1,2,tmp); 
  
      PostData=PostData+"\"Humidity";
      PostData=PostData+"\":";
      PostData=String(PostData + String(tmp));
      PostData=PostData+",";
    
      dtostrf(ft_press,1,2,tmp); 
   
      PostData=PostData+"\"Pressure";
      PostData=PostData+"\":";  
      PostData=String(PostData + String(tmp));
      //PostData=PostData+",";
      PostData=PostData+"}"; 
      //Serial.print("PostData = ");Serial.println(PostData);  
      
    }
     
  PostData=PostData+"]";  
   
  Serial.print("PostData  = ");Serial.println(PostData);
   
}
void Post(String concatString)
{
   Serial.println("Posting");  
  if (client.connect(server,port)) {
    Serial.println("connected");
    client.println("POST /index.html HTTP/1.1");
    client.println("Host: koshy");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json;");
    client.print("Content-Length: ");
    client.println(concatString.length());
    client.println();
    client.println(concatString);
  } else {
    Serial.println("connection failed");
  }
  
}

void WaitForRequest() // Sets buffer[] and bufferSize
{
  
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
    Serial.println("Not available and not connected");
    client.stop();
    closeConnection();
    //delay(5000);
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
 
 Serial.println("===============DONE=================");
 
}

 
bool DupID(long id)
{
  //this function keeps the last 10 header.IDs, then searches through them all
  //when cntID reaches 10, it rolls over to 0 again
  //if a match is found, it returns true, falst otherwise
  bool found = false;
  for (int i = 0; i < 10; i++)
    if (cnt [i] == id){
      found = true;
      break;}
  if (cntID < 10){cnt[cntID] = id;}
  else{cntID = 0;cnt[cntID] = id;}
  cntID++;
  return found;
}


void startConnection(){

  Serial.println("Starting web client ");


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
}



void closeConnection(){
  while(notConnected==false){
    if(gsmAccess.shutdown()){
      Serial.println("Shutting down GSM");
      delay(1000);
      //digitalWrite(3,LOW); // Disable the RX pin
      notConnected = true;

    }
    else{
      Serial.print("failed to shutdown");
      delay(1000);
    }
  }
}



