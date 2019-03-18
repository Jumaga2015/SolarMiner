
float calculateSunrise(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings);
float calculateSunset (int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings);

void printSunrise(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings);
void printSunset(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings);

double calcJD(int year,int month,int day);
double calcSunriseUTC(double JD, double latitude, double longitude);
double calcSunsetUTC(double JD, double latitude, double longitude);