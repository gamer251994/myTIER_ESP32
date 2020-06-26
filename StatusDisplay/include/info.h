typedef struct
{
    const char *snr;
    bool lock;
    double speed;
    double trip;
    double total;
    double consumption;
    int battery;
    bool led;
} eScooter;

typedef enum
{
    RIGHT_ALIGNMENT = 0,
    LEFT_ALIGNMENT,
    CENTER_ALIGNMENT,
} Text_alignment;

// Weather data
/*
class Weather
{
    double tempDay;
    double tempMin;
    double tempMax;
    double tempNight;
    double tempEve;
    double tempMorn;
    String main;
    String description;
    String icon;

public:
    double getTempDay() { return tempDay; }
    double getTempMin() { return tempMin; }
    double getTempMax() { return tempMax; }
    double getTempNight() { return tempNight; }
    double getTempEve() { return tempEve; }
    double getTempMorn() { return tempMorn; }
    String getMain() { return main; }
    String getDescription() { return description; }
    String getIcon() { return icon; }

    void setTempDay(double inTemp) { tempDay = inTemp; }
    void setTempMin(double inTempMin) { tempMin = inTempMin; }
    void setTempMax(double inTempMax) { tempMax = inTempMax; }
    void setTempNight(double inTempNight) { tempMax = inTempNight; }
    void setTempEve(double inTempEve) { tempMax = inTempEve; }
    void setTempMorn(double inTempMorn) { tempMax = inTempMorn; }
    void setMain(String inMain) { main = inMain; }
    void setDescription(String inDescription) { description = inDescription; }
    void setIcon(String inIcon) { icon = inIcon; }
};
*/
class OpenWeatherMap
{
    String httpResponse;
    long dateTime;
    double tempDay;
    double tempMin;
    double tempMax;
    double tempNight;
    double tempEve;
    double tempMorn;
    String main;
    String description;
    String icon;
    //Weather weather;

public:
    String getHttpResponse() { return httpResponse; }
    void setHttpResponse(String inHttpResponse) { httpResponse = inHttpResponse; }

    long getDateTime() { return dateTime; }
    void setDateTime(long inDateTime) { dateTime = inDateTime; }

    //Weather getWeather() { return weather; }
    //void setWeather(Weather inWeather) { weather = inWeather; }
    double getTempDay() { return tempDay; }
    double getTempMin() { return tempMin; }
    double getTempMax() { return tempMax; }
    double getTempNight() { return tempNight; }
    double getTempEve() { return tempEve; }
    double getTempMorn() { return tempMorn; }
    String getMain() { return main; }
    String getDescription() { return description; }
    String getIcon() { return icon; }

    void setTempDay(double inTempDay) { tempDay = inTempDay; }
    void setTempMin(double inTempMin) { tempMin = inTempMin; }
    void setTempMax(double inTempMax) { tempMax = inTempMax; }
    void setTempNight(double inTempNight) { tempMax = inTempNight; }
    void setTempEve(double inTempEve) { tempMax = inTempEve; }
    void setTempMorn(double inTempMorn) { tempMax = inTempMorn; }
    void setMain(String inMain) { main = inMain; }
    void setDescription(String inDescription) { description = inDescription; }
    void setIcon(String inIcon) { icon = inIcon; }

};

OpenWeatherMap parseCurrentJson(String httpResponse)
{
    OpenWeatherMap openWeatherMap;
    openWeatherMap.setHttpResponse(httpResponse);

    StaticJsonDocument<3500> root;
    auto error = deserializeJson(root, httpResponse);
    if (!error)
    {
        return openWeatherMap;
    }

    long dateTime = root["daily"][0]["dt"];
    openWeatherMap.setDateTime(dateTime);

    double weatherTempDay = root["daily"][0]["temp"]["day"];
    double weatherTempMin = root["daily"][0]["temp"]["min"];
    double weatherTempMax = root["daily"][0]["temp"]["max"];
    double weatherTempNight = root["daily"][0]["temp"]["night"];
    double weatherTempEve = root["daily"][0]["temp"]["eve"];
    double weatherTempMorn = root["daily"][0]["temp"]["morn"];
    String weatherMain = root["daily"][0]["weather"][0]["main"];
    String weatherDescription = root["daily"][0]["weather"][0]["description"];
    String weatherIcon = root["daily"][0]["weather"][0]["icon"];

    /*Weather weather;
    weather.setTempDay(weatherTempDay);
    weather.setTempMin(weatherTempMin);
    weather.setTempMax(weatherTempMax);
    weather.setTempNight(weatherTempNight);
    weather.setTempEve(weatherTempEve);
    weather.setTempMorn(weatherTempMorn);
    weather.setMain(weatherMain);
    weather.setDescription(weatherDescription);
    weather.setIcon(weatherIcon);
    openWeatherMap.setWeather(weather);
*/
    openWeatherMap.setTempDay(weatherTempDay);
    openWeatherMap.setTempMin(weatherTempMin);
    openWeatherMap.setTempMax(weatherTempMax);
    openWeatherMap.setTempNight(weatherTempNight);
    openWeatherMap.setTempEve(weatherTempEve);
    openWeatherMap.setTempMorn(weatherTempMorn);
    openWeatherMap.setMain(weatherMain);
    openWeatherMap.setDescription(weatherDescription);
    openWeatherMap.setIcon(weatherIcon);
    return openWeatherMap;
}