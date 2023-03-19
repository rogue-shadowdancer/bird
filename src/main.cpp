#include "esp32-hal-gpio.h"
#include "esp32-hal.h"
#include <Arduino.h>
#include <WiFi.h>
#include "../lib/jy60/REG.h"
#include "../lib/jy60/wit_c_sdk.h"


#define ACC_UPDATE		0x01
#define GYRO_UPDATE		0x02
#define ANGLE_UPDATE	0x04
#define MAG_UPDATE		0x08
#define READ_UPDATE		0x80
static volatile char s_cDataUpdate = 0, s_cCmd = 0xff; 

static void CmdProcess(void);
static void AutoScanSensor(void);
static void SensorUartSend(uint8_t *p_data, uint32_t uiSize);
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);
static void Delayms(uint16_t ucMs);
const uint32_t c_uiBaud[8] = {0,4800, 9600, 19200, 38400, 57600, 115200, 230400};

int i;
float fAcc[3], fGyro[3], fAngle[3];

const char *ssid = "DKU"; 
const char *password = "Duk3blu3!"; 
const char *server_ip = "10.200.84.164"; 
int server_port = 8001; 

WiFiUDP Udp;//声明UDP对象
int incomingByte = 0; // for incoming serial data


void CopeCmdData(unsigned char ucData)
{
	static unsigned char s_ucData[50], s_ucRxCnt = 0;
	
	s_ucData[s_ucRxCnt++] = ucData;
	if(s_ucRxCnt<3)return;										//Less than three data returned
	if(s_ucRxCnt >= 50) s_ucRxCnt = 0;
	if(s_ucRxCnt >= 3)
	{
		if((s_ucData[1] == '\r') && (s_ucData[2] == '\n'))
		{
			s_cCmd = s_ucData[0];
			memset(s_ucData,0,50);
			s_ucRxCnt = 0;
		}
		else 
		{
			s_ucData[0] = s_ucData[1];
			s_ucData[1] = s_ucData[2];
			s_ucRxCnt = 2;
			
		}
	}
}

void WIFI_connect()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
}

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println();
    WIFI_connect();
    Serial.println("WiFi connected!");

    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); //打印模块IP

    Serial2.begin(9600);

      // put your setup code here, to run once:
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(SensorUartSend);
    WitRegisterCallBack(SensorDataUpdata);
    WitDelayMsRegister(Delayms);
    Serial.print("\r\n********************** wit-motion normal example  ************************\r\n");
    AutoScanSensor();
}

void loop() {
  // put your main code here, to run repeatedly:
    /*将接受到的数据发送回去*/
    // Udp.beginPacket("10.200.84.164",8001);  //准备发送数据到目标IP和目标端口
    // Udp.print("receive data:");  //将数据receive data:放入发送的缓冲区
    // Udp.endPacket();  //向目标IP目标端口发送数据
    while (Serial.available() > 0) {
    // read the incoming byte
        incomingByte = Serial.read();
    };

    while (Serial2.available())
    {
      WitSerialDataIn(Serial2.read());
    }
    while (Serial.available()) 
    {
      CopeCmdData(Serial.read());
    }
    CmdProcess();
    if(s_cDataUpdate)
    {
        for(i = 0; i < 3; i++)
        {
            fAcc[i] = sReg[AX+i] / 32768.0f * 16.0f;
            fGyro[i] = sReg[GX+i] / 32768.0f * 2000.0f;
            fAngle[i] = sReg[Roll+i] / 32768.0f * 180.0f;
        }
        if(s_cDataUpdate & ACC_UPDATE)
        {
            Serial.print("acc:");
            Serial.print(fAcc[0], 3);
            Serial.print(" ");
            Serial.print(fAcc[1], 3);
            Serial.print(" ");
            Serial.print(fAcc[2], 3);
            Serial.print("\r\n");

            Udp.beginPacket(server_ip,server_port);  //准备发送数据到目标IP和目标端口
            // Udp.print("acc:");
            Udp.print("0x01");
            Udp.print(fAcc[0]);
            Udp.print(" ");
            Udp.print(fAcc[1]);
            Udp.print(" ");
            Udp.print(fAcc[2]);
            Udp.endPacket();  //向目标IP目标端口发送数据
            s_cDataUpdate &= ~ACC_UPDATE;
        }

        // if(s_cDataUpdate & GYRO_UPDATE)
        // {
        //     Serial.print("gyro:");
        //     Serial.print(fGyro[0], 1);
        //     Serial.print(" ");
        //     Serial.print(fGyro[1], 1);
        //     Serial.print(" ");
        //     Serial.print(fGyro[2], 1);
        //     Serial.print("\r\n");
        //     s_cDataUpdate &= ~GYRO_UPDATE;
        // }
        // if(s_cDataUpdate & ANGLE_UPDATE)
        // {
        //     Serial.print("angle:");
        //     Serial.print(fAngle[0], 3);
        //     Serial.print(" ");
        //     Serial.print(fAngle[1], 3);
        //     Serial.print(" ");
        //     Serial.print(fAngle[2], 3);
        //     Serial.print("\r\n");
        //     s_cDataUpdate &= ~ANGLE_UPDATE;
        // }
        // if(s_cDataUpdate & MAG_UPDATE)
        // {
        //     Serial.print("mag:");
        //     Serial.print(sReg[HX]);
        //     Serial.print(" ");
        //     Serial.print(sReg[HY]);
        //     Serial.print(" ");
        //     Serial.print(sReg[HZ]);
        //     Serial.print("\r\n");
        //     s_cDataUpdate &= ~MAG_UPDATE;
        // }
        s_cDataUpdate = 0;
    }

}


static void ShowHelp(void)
{
	Serial.print("\r\n************************	 WIT_SDK_DEMO	************************");
	Serial.print("\r\n************************          HELP           ************************\r\n");
	Serial.print("UART SEND:a\\r\\n   Acceleration calibration.\r\n");
	Serial.print("UART SEND:m\\r\\n   Magnetic field calibration,After calibration send:   e\\r\\n   to indicate the end\r\n");
	Serial.print("UART SEND:U\\r\\n   Bandwidth increase.\r\n");
	Serial.print("UART SEND:u\\r\\n   Bandwidth reduction.\r\n");
	Serial.print("UART SEND:B\\r\\n   Baud rate increased to 115200.\r\n");
	Serial.print("UART SEND:b\\r\\n   Baud rate reduction to 9600.\r\n");
	Serial.print("UART SEND:R\\r\\n   The return rate increases to 10Hz.\r\n");
    Serial.print("UART SEND:r\\r\\n   The return rate reduction to 1Hz.\r\n");
    Serial.print("UART SEND:C\\r\\n   Basic return content: acceleration, angular velocity, angle, magnetic field.\r\n");
    Serial.print("UART SEND:c\\r\\n   Return content: acceleration.\r\n");
    Serial.print("UART SEND:h\\r\\n   help.\r\n");
	Serial.print("******************************************************************************\r\n");
}

static void CmdProcess(void)
{
	switch(s_cCmd)
	{
		case 'a':	if(WitStartAccCali() != WIT_HAL_OK) Serial.print("\r\nSet AccCali Error\r\n");
			break;
		case 'm':	if(WitStartMagCali() != WIT_HAL_OK) Serial.print("\r\nSet MagCali Error\r\n");
			break;
		case 'e':	if(WitStopMagCali() != WIT_HAL_OK) Serial.print("\r\nSet MagCali Error\r\n");
			break;
		case 'u':	if(WitSetBandwidth(BANDWIDTH_5HZ) != WIT_HAL_OK) Serial.print("\r\nSet Bandwidth Error\r\n");
			break;
		case 'U':	if(WitSetBandwidth(BANDWIDTH_256HZ) != WIT_HAL_OK) Serial.print("\r\nSet Bandwidth Error\r\n");
			break;
		case 'B':	if(WitSetUartBaud(WIT_BAUD_115200) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else 
              {
                Serial2.begin(c_uiBaud[WIT_BAUD_115200]);
                Serial.print(" 115200 Baud rate modified successfully\r\n");
              }
			break;
		case 'b':	if(WitSetUartBaud(WIT_BAUD_9600) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else 
              {
                Serial2.begin(c_uiBaud[WIT_BAUD_9600]); 
                Serial.print(" 9600 Baud rate modified successfully\r\n");
              }
			break;
		case 'r': if(WitSetOutputRate(RRATE_1HZ) != WIT_HAL_OK)  Serial.print("\r\nSet Baud Error\r\n");
			        else Serial.print("\r\nSet Baud Success\r\n");
			break;
		case 'R':	if(WitSetOutputRate(RRATE_10HZ) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else Serial.print("\r\nSet Baud Success\r\n");
			break;
    case 'C': if(WitSetContent(RSW_ACC|RSW_GYRO|RSW_ANGLE|RSW_MAG) != WIT_HAL_OK) Serial.print("\r\nSet RSW Error\r\n");
      break;
    case 'c': if(WitSetContent(RSW_ACC) != WIT_HAL_OK) Serial.print("\r\nSet RSW Error\r\n");
      break;
		case 'h':	ShowHelp();
			break;
		default :break;
	}
	s_cCmd = 0xff;
}
static void SensorUartSend(uint8_t *p_data, uint32_t uiSize)
{
  Serial2.write(p_data, uiSize);
  Serial2.flush();
}
static void Delayms(uint16_t ucMs)
{
  delay(ucMs);
}
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
	int i;
    for(i = 0; i < uiRegNum; i++)
    {
        switch(uiReg)
        {
            case AZ:
				s_cDataUpdate |= ACC_UPDATE;
            break;
            case GZ:
				s_cDataUpdate |= GYRO_UPDATE;
            break;
            case HZ:
				s_cDataUpdate |= MAG_UPDATE;
            break;
            case Yaw:
				s_cDataUpdate |= ANGLE_UPDATE;
            break;
            default:
				s_cDataUpdate |= READ_UPDATE;
			break;
        }
		uiReg++;
    }
}

static void AutoScanSensor(void)
{
	int i, iRetry;
	
	for(i = 0; i < sizeof(c_uiBaud)/sizeof(c_uiBaud[0]); i++)
	{
		Serial2.begin(c_uiBaud[i]);
    Serial2.flush();
		iRetry = 2;
		s_cDataUpdate = 0;
		do
		{
			WitReadReg(AX, 3);
			delay(200);
      while (Serial2.available())
      {
        WitSerialDataIn(Serial2.read());
      }
			if(s_cDataUpdate != 0)
			{
				Serial.print(c_uiBaud[i]);
				Serial.print(" baud find sensor\r\n\r\n");
				ShowHelp();
				return ;
			}
			iRetry--;
		}while(iRetry);		
	}
	Serial.print("can not find sensor\r\n");
	Serial.print("please check your connection\r\n");
}