
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>


int value = 0;
#define pinin 5
long threshold = 100000;
String state= "Closed";
String previous_state=state;

/* ******** Ethernet Card Settings ******** */
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x23, 0x36 };

/* ******** NTP Server Settings ******** */
IPAddress timeServer(128,138,141,172);
const long timeZoneOffset = -28800L; 


char server[] = "smtpcorp.com";
int port = 2525;

EthernetClient client;



/* Syncs to NTP server every 15 seconds for testing,
   set to 1 hour or more to be reasonable */
unsigned int ntpSyncTime = 3600;       


/* ALTER THESE VARIABLES AT YOUR OWN RISK */
// local port to listen for UDP packets
unsigned int localPort = 8888;
// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE= 48;     
// Buffer to hold incoming and outgoing packets
byte packetBuffer[NTP_PACKET_SIZE]; 
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;                   
// Keeps track of how long ago we updated the NTP server
unsigned long ntpLastUpdate = 0;   
// Check last time clock displayed (Not in Production)
time_t prevDisplay = 0;           
//--------------------- TIME STUFF END




void setup(){
  pinMode(pinin,INPUT);
  Serial.begin(9600); 

   // Ethernet shield and NTP setup
   int i = 0;
   int DHCP = 0;
   DHCP = Ethernet.begin(mac);
   //Try to get dhcp settings 30 times before giving up
   while( DHCP == 0 && i < 30){
     delay(1000);
     DHCP = Ethernet.begin(mac);
     i++;
   }
   if(!DHCP){
    Serial.println("DHCP FAILED");
     for(;;); //Infinite loop because DHCP Failed
   }
   Serial.println("DHCP Success");
  
   //Try to get the date and time
   int trys=0;
   while(!getTimeAndDate() && trys<10) {
     trys++;
   }
    delay(2000);
}

// Do not alter this function, it is used by the system
int getTimeAndDate() {
   int flag=0;
   Udp.begin(localPort);
   sendNTPpacket(timeServer);
   delay(1000);
   if (Udp.parsePacket()){
     Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
     unsigned long highWord, lowWord, epoch;
     highWord = word(packetBuffer[40], packetBuffer[41]);
     lowWord = word(packetBuffer[42], packetBuffer[43]); 
     epoch = highWord << 16 | lowWord;
     epoch = epoch - 2208988800 + timeZoneOffset;
     flag=1;
     setTime(epoch);
     ntpLastUpdate = now();
   }
   return flag;
}

// Do not alter this function, it is used by the system
unsigned long sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;                 
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

// Clock display of the time and date (Basic)
void clockDisplay(){
  client.print(hour());
  printDigits(minute());
  printDigits(second());
  client.print(" ");
  client.print(month());
  client.print(" ");
  client.print(day());  
  client.print(" ");
  client.print(year());
  client.println();
}

// Utility function for clock display: prints preceding colon and leading 0
void printDigits(int digits){
  client.print(":");
  if(digits < 10)
    client.print('0');
  client.print(digits);
}



void loop(){
  
    // Update the time via NTP server as often as the time you set at the top
    if(now()-ntpLastUpdate > ntpSyncTime) {
      int trys=0;
      while(!getTimeAndDate() && trys<10){
        trys++;
      }
      if(trys<10){
        Serial.println("ntp server update success");
      }
      else{
        Serial.println("ntp server update failed");
      }
    }
  
  previous_state=state;
  if(checkstate()>threshold){
  state="Closed";
  }
  else{
  state="Open";
  }
  if(previous_state!=state){
  send_message(previous_state,state);
  }
  delay(1000);
}

long checkstate(){
int array[100];
long sum=0;
for(int i =0; i<100;i++){
array[i]=analogRead(pinin);
}
for(int i =0; i<100;i++){
sum+=array[i];
}
return sum;
}




void send_message(String prev, String now){

  
  
  if(sendEmail()) Serial.println(F("Email sent"));
  else Serial.println(F("Email failed"));
}




byte sendEmail()
{
  byte thisByte = 0;
  byte respCode;

  if(client.connect(server,port) == 1) {
    Serial.println(F("connected"));
  } else {
    Serial.println(F("connection failed"));
    return 0;
  }

  if(!eRcv()) return 0;

  Serial.println(F("Sending hello"));
// replace 1.2.3.4 with your Arduino's ip
  client.println("EHLO 1.2.3.4");
  if(!eRcv()) return 0;

  Serial.println(F("Sending auth login"));
  client.println("auth login");
  if(!eRcv()) return 0;

  Serial.println(F("Sending User"));
// Change to your base64 encoded user
  client.println("cm9zYXJpb2dhcmFnZWRvb3JAZ21haWwuY29t");

  if(!eRcv()) return 0;

  Serial.println(F("Sending Password"));
// change to your base64 encoded password
  client.println("RXByMTEyMyE=");

  if(!eRcv()) return 0;

// change to your email address (sender)
  Serial.println(F("Sending From"));
  client.println("MAIL From: <rosariogaragedoor@gmail.com>");
  if(!eRcv()) return 0;

// change to recipient address
  Serial.println(F("Sending To"));
  client.println("RCPT To: <ericmd2003@gmail.com>");
  if(!eRcv()) return 0;

  Serial.println(F("Sending DATA"));
  client.println("DATA");
  if(!eRcv()) return 0;

  Serial.println(F("Sending email"));

// change to recipient address
  client.println("To: <ericmd2003@gmail.com>");

// change to your address
  client.println("From: Garage <rosariogaragedoor@gmail.com>");

  client.println("Subject: Garage Door Movement\n");
//---------------------------BODY TODO


  String prompt="Door was "+previous_state+" but now "+state+" at ";
  
  client.print(prompt);
  clockDisplay();



  client.println();

  client.println(".");

  if(!eRcv()) return 0;

  Serial.println(F("Sending QUIT"));
  client.println("QUIT");
  if(!eRcv()) return 0;

  client.stop();

  Serial.println(F("disconnected"));

  return 1;
}

byte eRcv()
{
  byte respCode;
  byte thisByte;
  int loopCount = 0;

  while(!client.available()) {
    delay(1);
    loopCount++;

    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  respCode = client.peek();

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }

  if(respCode >= '4')
  {
    efail();
    return 0;  
  }

  return 1;
}


void efail()
{
  byte thisByte = 0;
  int loopCount = 0;

  client.println(F("QUIT"));

  while(!client.available()) {
    delay(1);
    loopCount++;

    // if nothing received for 10 seconds, timeout
    if(loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return;
    }
  }

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }

  client.stop();

  Serial.println(F("disconnected"));
}


