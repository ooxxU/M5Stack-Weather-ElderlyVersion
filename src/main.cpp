#include <Arduino.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "display_ch.h"
#include "str.h"
#include <Ticker.h>


// Set the name and password of the wifi to be connected.  配置所连接wifi的名称和密码
//const char *ssid = "HW_OOXX";
const char *ssid = "XXX"; //modify your wifi ssid
const char *password = "XXX"; //modify your wifi PWD

const char *host = "api.seniverse.com";
const char *privateKey = "xxxxxx";  //modify your privateKey
const char *city = "xxxxx";   //modify your city
const char *city_code = "xxxxxx";  //modify your city code
const char *language = "en";

#define NTP1 "ntp1.aliyun.com"
#define NTP2 "ntp2.aliyun.com"
#define NTP3 "ntp3.aliyun.com"


const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};                //星期
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; //月份
String g_strDate = "";

struct WetherData
{
    //char city[32];
    char weather_day[32];
    char weather_night[32];
    char high[16];
    char low[16];
    char weather_code_day[16];
    char weather_code_night[16];
    //char humi[32];
};

DisplayCh displaych;

struct WetherData TodayWeatherData = {0};
struct WetherData TomorrowWetherData = {0};

bool bUpdateWeather = true;
Ticker ticker[3]; //声明Ticker对象


void setClock(String& strDate, String& strTime);
//void GetWeather(String& strHighTemp, String& strLowTemp, String& strDayWeather, String& strNightWeather, String& strCodeDay, String& strCodeNight);
void GetWeather(WetherData& TodayWetherDataParam, WetherData& TomorrowWetherDataParam);
char* GetWeatherCNCharByCode(int nCode);
void WeatherUpdateCallBack();


void setClock(String& strDate, String& strTime)
{
  struct tm timeInfo; //声明一个结构体
  char szDate[30] = {0};
  char szTime[30] = {0};

  if (!getLocalTime(&timeInfo))
  { 
    //一定要加这个条件判断，否则内存溢出
    //Serial.println("Failed to obtain time");
    return;
  }
  //Serial.print(asctime(&timeInfo)); //默认打印格式：Mon Oct 25 11:13:29 2021
  //String date = WDAY_NAMES[timeInfo.tm_wday];
  //Serial.println(date.c_str());
  //sprintf_P(szDate, PSTR("%04d-%02d-%02d %s"), timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, WDAY_NAMES[timeInfo.tm_wday].c_str());
  sprintf_P(szDate, PSTR("%04d-%02d-%02d"), timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
  //sprintf_P(szTime, PSTR("%02d:%02d:%02d"), timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  sprintf_P(szTime, PSTR("%02d:%02d"), timeInfo.tm_hour, timeInfo.tm_min);

  strDate = szDate;
  strTime = szTime;
  return;
}

void GetWeather(WetherData& TodayWetherDataParam, WetherData& TomorrowWetherDataParam)
{

  WiFiClient client;

  if (!client.connect(host, 80))
  {
      Serial.println("Connect host failed!");
      ESP.restart();
      return;
  }

  Serial.println("host Conected!");

  //https://api.seniverse.com/v3/weather/daily.json?key=XXXXXX-KEY&loca&location=city_code
  //https://api.seniverse.com/v3/weather/daily.json?key=XXXXXX-KEY&loca&location=fuzhou
  String getUrl = "/v3/weather/daily.json?key=";
  getUrl += privateKey;
  getUrl += "&location=";
  //getUrl += city;
  getUrl += city_code;
  getUrl += "&language=";
  getUrl += language;
  client.print(String("GET ") + getUrl + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  Serial.println("Get send");
  Serial.println(getUrl);

  char endOfHeaders[] = "\r\n\r\n";
  bool ok = client.find(endOfHeaders);
  if (!ok)
  {
      Serial.println("No response or invalid response!");
  }
  Serial.println("Skip headers");

  String line="";

  line += client.readStringUntil('\n'); 

  Serial.println(line);

  DynamicJsonDocument doc(1400);

  DeserializationError error = deserializeJson(doc, line);
  if (error)
  {
      Serial.println("deserialize json failed");
      ESP.restart();
      return;
  }
  Serial.println("deserialize json success");

  // =================================================== Get Today Weather Data ===================================================
  //strcpy(weatherdata.city, doc["results"][0]["location"]["name"].as<const char*>());
  strcpy(TodayWetherDataParam.weather_day, doc["results"][0]["daily"][0]["text_day"].as<const char*>());
  strcpy(TodayWetherDataParam.weather_night, doc["results"][0]["daily"][0]["text_night"].as<const char*>());
  strcpy(TodayWetherDataParam.high, doc["results"][0]["daily"][0]["high"].as<const char*>());
  strcpy(TodayWetherDataParam.low, doc["results"][0]["daily"][0]["low"].as<const char*>());
  //strcpy(weatherdata.humi, doc["results"][0]["daily"][0]["humidity"].as<const char*>());
  strcpy(TodayWetherDataParam.weather_code_day, doc["results"][0]["daily"][0]["code_day"].as<const char*>());
  strcpy(TodayWetherDataParam.weather_code_night, doc["results"][0]["daily"][0]["code_night"].as<const char*>());

  // Serial.println("City:");
  // Serial.print(weatherdata.city);
  Serial.print("Today Day Weather: ");
  Serial.println(TodayWetherDataParam.weather_day);

  Serial.print("Today Night Weather: ");
  Serial.println(TodayWetherDataParam.weather_night);

  Serial.print("Today Temp High: ");
  Serial.println(TodayWetherDataParam.high);

  Serial.print("Today Temp Low: ");
  Serial.println(TodayWetherDataParam.low);

  Serial.print("Today weather_code_day: ");
  Serial.println(TodayWetherDataParam.weather_code_day);

  Serial.print("Today weather_code_night: ");
  Serial.println(TodayWetherDataParam.weather_code_night);

  // Serial.println("Today  humi");
  // Serial.println(Today weatherdata.humi);
  // =================================================== Get Today Weather Data ===================================================
  
  // =================================================== Get Tomorrow Weather Data ===================================================
  //strcpy(weatherdata.city, doc["results"][0]["location"]["name"].as<const char*>());
  strcpy(TomorrowWetherDataParam.weather_day, doc["results"][0]["daily"][1]["text_day"].as<const char*>());
  strcpy(TomorrowWetherDataParam.weather_night, doc["results"][0]["daily"][1]["text_night"].as<const char*>());
  strcpy(TomorrowWetherDataParam.high, doc["results"][0]["daily"][1]["high"].as<const char*>());
  strcpy(TomorrowWetherDataParam.low, doc["results"][0]["daily"][1]["low"].as<const char*>());
  //strcpy(weatherdata.humi, doc["results"][0]["daily"][0]["humidity"].as<const char*>());
  strcpy(TomorrowWetherDataParam.weather_code_day, doc["results"][0]["daily"][1]["code_day"].as<const char*>());
  strcpy(TomorrowWetherDataParam.weather_code_night, doc["results"][0]["daily"][1]["code_night"].as<const char*>());

  // Serial.println("City:");
  // Serial.print(weatherdata.city);
  Serial.print("Tomorrow Day Weather: ");
  Serial.println(TomorrowWetherDataParam.weather_day);

  Serial.print("Tomorrow Night Weather: ");
  Serial.println(TomorrowWetherDataParam.weather_night);

  Serial.print("Tomorrow Temp High: ");
  Serial.println(TomorrowWetherDataParam.high);

  Serial.print("Tomorrow Temp Low: ");
  Serial.println(TomorrowWetherDataParam.low);

  Serial.print("Tomorrow weather_code_day: ");
  Serial.println(TomorrowWetherDataParam.weather_code_day);

  Serial.print("Tomorrow weather_code_night: ");
  Serial.println(TomorrowWetherDataParam.weather_code_night);

  // Serial.println("Tomorrow  humi");
  // Serial.println(Tomorrow weatherdata.humi);
  // =================================================== Get Tomorrow Weather Data ===================================================

  Serial.println("read json success");
  Serial.println();
  Serial.println("closing connection");
  client.stop();
}

char* GetWeatherCNCharByCode(int nCode)
{
    if (nCode <= 3)
    {
      //晴天
      return strSunny;

    }else if(nCode > 3 && nCode <=8)
    {
      //多云
      return strCloudy;
      
    }else if(nCode == 9)
    {
      //阴天
      return strOvercast;

    }else if(nCode > 9 && nCode <= 12)
    {
      if (nCode == 10)
      {
        //阵雨
        return strShower;

      }else
      {
        //雷阵雨
        return strThundershower;

      }
    }else if(nCode == 13)
    {
      //小雨
      return strLightRain;
      
    }else if(nCode == 14)
    {
      //中雨
      return strModerateRain;

    }else if(nCode == 15)
    {
      //大雨
      return strHeavyRain;
      
    }else if(nCode >= 16 && nCode < 19)
    {
      //暴雨
      return strStorm;

    }else
    {
      return strUnkonw;
    }
      
}

void WeatherUpdateCallBack()
{
  
    Serial.println("WeatherUpdateCallBack");
    bUpdateWeather = true;
    
}

void setup()
{
  M5.begin();                                             // Init M5Core.  初始化M5Core
  displaych.loadHzk16();  //Load the Chinese character library (be sure to load before using the Chinese character library).  加载汉字库(务必在使用汉字库前加载)
  displaych.setTextColor(WHITE, BLACK); //Set the text color to white and the text background color to black (mandatory).  设置文字颜色为白色,文字背景颜色为黑色(必加)


  M5.Lcd.setTextSize(2);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      M5.Lcd.print(".");
  }
  M5.Lcd.println("\nWiFi connected");
  M5.lcd.print("IP address: ");
  M5.lcd.println(WiFi.localIP());
  
  delay(3000); 

  // M5.Lcd.clear();  

  
  configTime(8 * 3600, 0, NTP1, NTP2, NTP3);

  M5.Lcd.sleep(); 
  M5.Lcd.wakeup();
  M5.Lcd.clear();

  ticker[0].attach(30*60, WeatherUpdateCallBack);

  // M5.Speaker.begin(); //初始化扬声器
  // M5.Speaker.tone(661, 1000);    //设定喇叭以661Hz频率响1000ms
}



void loop()
{
  static bool bShow = true;
  String strDate = "";
  String strTime = "";
  setClock(strDate, strTime);

  int nCNSize = 48; //because  setTextSize 3 = 48 const
  int nCNPosY = 16; //frist CN start Y
  int nTextWidth = 0;
  int nCNTodayToTomorrowPosYSpace = 54; //今日和明日之间的Y间隔

  if(bUpdateWeather)
  {

    //拉取天气数据
    String strTodayHighTemp = "";
    String strTodayLowTemp = "";
    String strTodayDayWeather = "";
    String strTodayNightWeather = "";
    String strTodayCodeDay = "";
    String strTodayCodeNight = "";
    GetWeather(TodayWeatherData, TomorrowWetherData);

    strTodayDayWeather = TodayWeatherData.weather_day;
    strTodayNightWeather = TodayWeatherData.weather_night;
    strTodayHighTemp = TodayWeatherData.high;
    strTodayLowTemp = TodayWeatherData.low;
    strTodayCodeDay = TodayWeatherData.weather_code_day;
    strTodayCodeNight = TodayWeatherData.weather_code_night;

    if (strTodayHighTemp.isEmpty() || strTodayLowTemp.isEmpty() || strTodayDayWeather.isEmpty() || strTodayNightWeather.isEmpty() || strTodayCodeDay.isEmpty() || strTodayCodeNight.isEmpty())
    {
      ESP.restart();
      return;
    }
    
    String strTomorrowHighTemp = "";
    String strTomorrowLowTemp = "";
    String strTomorrowDayWeather = "";
    String strTomorrowNightWeather = "";
    String strTomorrowCodeDay = "";
    String strTomorrowCodeNight = "";

    strTomorrowDayWeather = TomorrowWetherData.weather_day;
    strTomorrowNightWeather = TomorrowWetherData.weather_night;
    strTomorrowHighTemp = TomorrowWetherData.high;
    strTomorrowLowTemp = TomorrowWetherData.low;
    strTomorrowCodeDay = TomorrowWetherData.weather_code_day;
    strTomorrowCodeNight = TomorrowWetherData.weather_code_night;

    if (strTomorrowHighTemp.isEmpty() || strTomorrowLowTemp.isEmpty() || strTomorrowDayWeather.isEmpty() || strTomorrowNightWeather.isEmpty() || strTomorrowCodeDay.isEmpty() || strTomorrowCodeNight.isEmpty())
    {
      ESP.restart();
      return;
    }

    //////////////////////////////////显示今日的处理///////////////////////////////////
    //显示今日
    displaych.setCursor(0,nCNPosY);
    displaych.setTextSize(3); //48*48
    displaych.setTextColor(GREEN, BLACK);
    displaych.writeHzk(strToday);
    //显示今日的最低最高温度
    //displaych.setCursor(48*2,48); //two cn char
    String strLowTemp = strTodayLowTemp;
    M5.Lcd.setTextSize(4); 
    int nLowTempPosX = nCNSize*2 + 16; //最低温的X位置 ，是两个中文字的大小加上间隔16, setTextSize=3的时候用displaych显示的中文是 48*48
    int nLowTempPosY = 0;
    M5.Lcd.drawRect(nLowTempPosX, nLowTempPosY, 320-48, 48*4+10, BLACK);//数字度数开始位置向后清空四行
    M5.Lcd.drawString(strLowTemp, nLowTempPosX, nLowTempPosY);
    nTextWidth = M5.Lcd.textWidth(strLowTemp,1);
    displaych.setTextSize(2); //32*32
    int nTempFlagPosX = nLowTempPosX + nTextWidth + 1; //度数X的位置
    displaych.setCursor(nTempFlagPosX, nLowTempPosY); 
    displaych.setTextColor(WHITE, BLACK);
    displaych.writeHzk(strTempFlag);
    //分隔符 - 
    String strSpliet = "-";
    M5.Lcd.setTextSize(4);
    int nSplietPosX = nTempFlagPosX + 32 + 5; //displaych -> setTextSize(2) => 32*32 ，32是度数的宽，5是间隔，分隔符X的位置
    int nSplietPosY = 0;
    M5.Lcd.drawString(strSpliet, nSplietPosX, nSplietPosY);
    //分隔符前后隔 5 个像素
    String strHighTemp = strTodayHighTemp;
    M5.Lcd.setTextSize(4); 
    int nHighTempPosX = nSplietPosX + 24 + 5; //最高温 分隔符前后隔5宽，需要多加5；也就是分隔符X的位置加上字体大小24加上5的间隔
    int nHighTempPosY = 0;
    M5.Lcd.drawString(strHighTemp, nHighTempPosX, nHighTempPosY);
    nTextWidth = M5.Lcd.textWidth(strHighTemp,1);
    displaych.setTextSize(2); //32*32
    nTempFlagPosX = nHighTempPosX + nTextWidth + 1; //度数X的位置
    displaych.setCursor(nTempFlagPosX, nHighTempPosY); 
    displaych.setTextColor(WHITE, BLACK);
    displaych.writeHzk(strTempFlag);

    //显示今日天气状况
    //白天天气
    int nDayWeatherPosX = nCNSize*2 + 16; //中文今日白天天气字符X位置
    int nDayWeatherTempPosY = 32 + 16; //32像素是 温度的像素高度， 16是间隔
    displaych.setCursor(nDayWeatherPosX, nDayWeatherTempPosY); //白天天气的位置
    displaych.setTextSize(2); //32*32
    int nCodeDay = atoi(strTodayCodeDay.c_str());
    displaych.writeHzk(GetWeatherCNCharByCode(nCodeDay));

    //转
    int nTrunFlagPosX = nDayWeatherPosX + 32*2 + 16; //天气都是定义成两个中文的，这里字体设置成2，所以一个中文是32, 16是间隔
    displaych.setCursor(nTrunFlagPosX, nDayWeatherTempPosY); //白天天气的位置
    displaych.setTextSize(2); //32*32
    displaych.writeHzk(strTrunFlag);

    //夜晚天气
    int nNightWeatherPosX = nTrunFlagPosX + 32 + 16; //中文晚上天气字符X位置，16是间隔，32是 "转" 的像素 32
    int nNightWeatherTempPosY = nDayWeatherTempPosY;
    displaych.setCursor(nNightWeatherPosX, nNightWeatherTempPosY); //夜晚天气的位置
    displaych.setTextSize(2); //32*32
    int nCodeNight = atoi(strTodayCodeNight.c_str());
    displaych.writeHzk(GetWeatherCNCharByCode(nCodeNight));

    //////////////////////////////////显示明日的处理///////////////////////////////////

    //左边显示明日
    //int nCNTodayToTomorrowPosYSpace = 54; //今日和明日之间的Y间隔
    int nCNTomorrowPosY = nCNPosY + nCNSize + nCNTodayToTomorrowPosYSpace;
    displaych.setCursor(0, nCNTomorrowPosY);
    displaych.setTextSize(3); //48*48
    displaych.setTextColor(GREEN, BLACK);
    displaych.writeHzk(strTomorrow);
    //displaych.setCursor(48*2,48); //two cn char
    
    //显示明日的最低最高温度
    //显示明日的最低温度
    strLowTemp = strTomorrowLowTemp;
    M5.Lcd.setTextSize(4); 
    nLowTempPosX = nCNSize*2 + 16; //最低温的X位置 ，是两个中文字的大小加上间隔16, setTextSize=3的时候用displaych显示的中文是 48*48
    nLowTempPosY = nCNTomorrowPosY - nCNPosY; //今日是从16的像素高度开始的，但是他的温度高度是从0像素开始的，所以这样为了保持一致，也要从0开始的话，就要减去16
    M5.Lcd.drawString(strLowTemp, nLowTempPosX, nLowTempPosY);
    nTextWidth = M5.Lcd.textWidth(strLowTemp,1);
    displaych.setTextSize(2); //32*32
    nTempFlagPosX = nLowTempPosX + nTextWidth + 1; //度数X的位置
    displaych.setCursor(nTempFlagPosX, nLowTempPosY); 
    displaych.setTextColor(WHITE, BLACK);
    displaych.writeHzk(strTempFlag); 
    //分隔符 - 
    strSpliet = "-";
    M5.Lcd.setTextSize(4);
    nSplietPosX = nTempFlagPosX + 32 + 5; //displaych -> setTextSize(2) => 32*32 ，32是度数的宽，5是间隔，分隔符X的位置
    nSplietPosY = nLowTempPosY;
    M5.Lcd.drawString(strSpliet, nSplietPosX, nSplietPosY);
    //分隔符前后隔 5 个像素
    //显示明日的最高温度
    strHighTemp = strTomorrowHighTemp;
    M5.Lcd.setTextSize(4); 
    nHighTempPosX = nSplietPosX + 24 + 5; //分隔符前后隔5宽，需要多加5；也就是分隔符X的位置加上字体大小24加上5的间隔
    nHighTempPosY = nLowTempPosY;
    M5.Lcd.drawString(strHighTemp, nHighTempPosX, nHighTempPosY);
    nTextWidth = M5.Lcd.textWidth(strHighTemp,1);
    displaych.setTextSize(2); //32*32
    nTempFlagPosX = nHighTempPosX + nTextWidth + 1; //度数X的位置
    displaych.setCursor(nTempFlagPosX, nHighTempPosY); 
    displaych.setTextColor(WHITE, BLACK);
    displaych.writeHzk(strTempFlag);

    //显示明日天气状况
    //白天天气
    nDayWeatherPosX = nCNSize*2 + 16; //中文白天天气字符X位置
    nDayWeatherTempPosY = nLowTempPosY + 32 + 16; //32像素是 温度的像素高度， 16是间隔， 起始位置是从nLowTempPosY开始
    displaych.setCursor(nDayWeatherPosX, nDayWeatherTempPosY); //白天天气的位置
    displaych.setTextSize(2); //32*32
    nCodeDay = atoi(strTomorrowCodeDay.c_str());
    displaych.writeHzk(GetWeatherCNCharByCode(nCodeDay));

    //转
    nTrunFlagPosX = nDayWeatherPosX + 32*2 + 16; //天气都是定义成两个中文的，这里字体设置成2，所以一个中文是32, 16是间隔
    displaych.setCursor(nTrunFlagPosX, nDayWeatherTempPosY); //白天天气的位置
    displaych.setTextSize(2); //32*32
    displaych.writeHzk(strTrunFlag);

    //夜晚天气
    nNightWeatherPosX = nTrunFlagPosX + 32 + 16; //中文晚上天气字符X位置，16是间隔，32是 "转" 的像素 32
    nNightWeatherTempPosY = nDayWeatherTempPosY;
    displaych.setCursor(nNightWeatherPosX, nNightWeatherTempPosY); //夜晚天气的位置
    displaych.setTextSize(2); //32*32
    nCodeNight = atoi(strTomorrowCodeNight.c_str());
    displaych.writeHzk(GetWeatherCNCharByCode(nCodeNight));

    bUpdateWeather = false;
  }

  /////////////////////////////////显示时间部分/////////////////////////////////////////
  
  //日期
  //String strDate = "2022-06-23";
  int nDatePosX = 0;
  int nDatePosY = nCNPosY + 48 + 48 + nCNTodayToTomorrowPosYSpace + 40; //左上角初始高度像素(今日上面的空间高度) + "今日"高度像素 + "明日"高度像素 + 间隔
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.drawString(strDate, nDatePosX, nDatePosY); //
  //时间
  //String strTime = "01:30";
  nTextWidth = M5.Lcd.textWidth(strDate,1);
  int nTimePosX = nTextWidth + 32;
  int nTimePosY = nDatePosY;
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.drawString(strTime, nTimePosX, nTimePosY); //

  M5.Lcd.setTextColor(WHITE, BLACK); //还原回去


  delay(1000);
  
}
