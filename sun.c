
#include "sun.h"

#include <math.h>
#include <stdio.h>
#define PI 3.1415926
#define ZENITH -.83

float calculateSunrise(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings) {
    /*
    localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0
    */
    //1. first calculate the day of the year
    float N1 = floor(275 * month / 9);
    float N2 = floor((month + 9) / 12);
    float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    float N = N1 - (N2 * N3) + day - 30;

    //2. convert the longitude to hour value and calculate an approximate time
    float lngHour = lng / 15.0;
         
    float t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    //float t = N + ((18 - lngHour) / 24)   //if setting time is desired:

    //3. calculate the Sun's mean anomaly   
    float M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
    float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634,360.0);

    //5a. calculate the Sun's right ascension      
    float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)),360.0);

    //5b. right ascension value needs to be in the same quadrant as L   
    float Lquadrant  = floor( L/90) * 90;
    float RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours   
    RA = RA / 15;

    //6. calculate the Sun's declination
    float sinDec = 0.39782 * sin((PI/180)*L);
    float cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    float cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
    /*   
    if (cosH >  1) 
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    float H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    //float H = acos(cosH) //   if setting time is desired:      
    H = H / 15;

    //8. calculate local mean time of rising/setting      
    float T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
    float UT = fmod(T - lngHour,24.0);

    //10. convert UT value to local time zone of latitude/longitude
    return UT + localOffset + daylightSavings;
 }

float calculateSunset(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings) {
    /*
    localOffset will be <0 for western hemisphere and >0 for eastern hemisphere
    daylightSavings should be 1 if it is in effect during the summer otherwise it should be 0
    */
    //1. first calculate the day of the year
    float N1 = floor(275 * month / 9);
    float N2 = floor((month + 9) / 12);
    float N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    float N = N1 - (N2 * N3) + day - 30;

    //2. convert the longitude to hour value and calculate an approximate time
    float lngHour = lng / 15.0;
         
    //float t = N + ((6 - lngHour) / 24);   //if rising time is desired:
    float t = N + ((18 - lngHour) / 24);   //if setting time is desired:

    //3. calculate the Sun's mean anomaly   
    float M = (0.9856 * t) - 3.289;

    //4. calculate the Sun's true longitude
    float L = fmod(M + (1.916 * sin((PI/180)*M)) + (0.020 * sin(2 *(PI/180) * M)) + 282.634,360.0);

    //5a. calculate the Sun's right ascension      
    float RA = fmod(180/PI*atan(0.91764 * tan((PI/180)*L)),360.0);

    //5b. right ascension value needs to be in the same quadrant as L   
    float Lquadrant  = floor( L/90) * 90;
    float RAquadrant = floor(RA/90) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    //5c. right ascension value needs to be converted into hours   
    RA = RA / 15;

    //6. calculate the Sun's declination
    float sinDec = 0.39782 * sin((PI/180)*L);
    float cosDec = cos(asin(sinDec));

    //7a. calculate the Sun's local hour angle
    float cosH = (sin((PI/180)*ZENITH) - (sinDec * sin((PI/180)*lat))) / (cosDec * cos((PI/180)*lat));
    /*   
    if (cosH >  1) 
    the sun never rises on this location (on the specified date)
    if (cosH < -1)
    the sun never sets on this location (on the specified date)
    */

    //7b. finish calculating H and convert into hours
    //float H = 360 - (180/PI)*acos(cosH);   //   if if rising time is desired:
    float H = acos(cosH); //   if setting time is desired:      
    H = H / 15;

    //8. calculate local mean time of rising/setting      
    float T = H + RA - (0.06571 * t) - 6.622;

    //9. adjust back to UTC
    float UT = fmod(T - lngHour,24.0);

    //10. convert UT value to local time zone of latitude/longitude
    return UT + localOffset + daylightSavings;
 }

void printSunrise(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings){

    float localT = calculateSunrise(year, month, day, lat, lng, localOffset,daylightSavings);
    double hours;
    float minutes = modf(localT,&hours)*60;
    printf("%02.0f:%02.0f",hours,minutes);
}

void printSunset(int year,int month,int day,float lat, float lng,int localOffset, int daylightSavings){

    float localT = calculateSunset(year, month, day, lat, lng, localOffset,daylightSavings);
    double hours;
    float minutes = modf(localT,&hours)*60;
    printf("%02.0f:%02.0f",hours,minutes);
}


double calcSunEqOfCenter(double t);


/* Convert degree angle to radians */

double  degToRad(double angleDeg)
{
  return (M_PI * angleDeg / 180.0);
}

double radToDeg(double angleRad)
{
  return (180.0 * angleRad / M_PI);
}


double calcMeanObliquityOfEcliptic(double t)
{
  double seconds = 21.448 - t*(46.8150 + t*(0.00059 - t*(0.001813)));
  double e0 = 23.0 + (26.0 + (seconds/60.0))/60.0;

  return e0;              // in degrees
}


double calcGeomMeanLongSun(double t)
{


  double L = 280.46646 + t * (36000.76983 + 0.0003032 * t);
  while( (int) L >  360 )
    {
      L -= 360.0;

    }
  while(  L <  0)
    {
      L += 360.0;

    }


  return L;              // in degrees
}






double calcObliquityCorrection(double t)
{
  double e0 = calcMeanObliquityOfEcliptic(t);


  double omega = 125.04 - 1934.136 * t;
  double e = e0 + 0.00256 * cos(degToRad(omega));
  return e;               // in degrees
}

double calcEccentricityEarthOrbit(double t)
{
  double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
  return e;               // unitless
}

double calcGeomMeanAnomalySun(double t)
{
  double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
  return M;               // in degrees
}


double calcEquationOfTime(double t)
{


  double epsilon = calcObliquityCorrection(t);               
  double  l0 = calcGeomMeanLongSun(t);
  double e = calcEccentricityEarthOrbit(t);
  double m = calcGeomMeanAnomalySun(t);
  double y = tan(degToRad(epsilon)/2.0);
  y *= y;
  double sin2l0 = sin(2.0 * degToRad(l0));
  double sinm   = sin(degToRad(m));
  double cos2l0 = cos(2.0 * degToRad(l0));
  double sin4l0 = sin(4.0 * degToRad(l0));
  double sin2m  = sin(2.0 * degToRad(m));
  double Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0
				- 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;


  return radToDeg(Etime)*4.0;	// in minutes of time


	}


double calcTimeJulianCent(double jd)
{

  double T = ( jd - 2451545.0)/36525.0;
  return T;
}









double calcSunTrueLong(double t)
{
  double l0 = calcGeomMeanLongSun(t);
  double c = calcSunEqOfCenter(t);

  double O = l0 + c;
  return O;               // in degrees
}



double calcSunApparentLong(double t)
{
  double o = calcSunTrueLong(t);

  double  omega = 125.04 - 1934.136 * t;
  double  lambda = o - 0.00569 - 0.00478 * sin(degToRad(omega));
  return lambda;          // in degrees
}




double calcSunDeclination(double t)
{
  double e = calcObliquityCorrection(t);
  double lambda = calcSunApparentLong(t);

  double sint = sin(degToRad(e)) * sin(degToRad(lambda));
  double theta = radToDeg(asin(sint));
  return theta;           // in degrees
}


double calcHourAngleSunrise(double lat, double solarDec)
{
  double latRad = degToRad(lat);
  double sdRad  = degToRad(solarDec);



  double HA = (acos(cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad)));

  return HA;              // in radians
}

double calcHourAngleSunset(double lat, double solarDec)
{
  double latRad = degToRad(lat);
  double sdRad  = degToRad(solarDec);


  double HA = (acos(cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad)));

  return -HA;              // in radians
}


double calcJD(int year,int month,int day)
{
		if (month <= 2) {
			year -= 1;
			month += 12;
		}
		int A = floor(year/100);
		int B = 2 - A + floor(A/4);

		double JD = floor(365.25*(year + 4716)) + floor(30.6001*(month+1)) + day + B - 1524.5;
		return JD;
}



double calcJDFromJulianCent(double t)
{
  double JD = t * 36525.0 + 2451545.0;
  return JD;
}


double calcSunEqOfCenter(double t)
{
		double m = calcGeomMeanAnomalySun(t);

		double mrad = degToRad(m);
		double sinm = sin(mrad);
		double sin2m = sin(mrad+mrad);
		double sin3m = sin(mrad+mrad+mrad);

		double C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
		return C;		// in degrees
}






double calcSunriseUTC(double JD, double latitude, double longitude)
 {

	double t = calcTimeJulianCent(JD);

		// *** First pass to approximate sunrise


	double  eqTime = calcEquationOfTime(t);
	double  solarDec = calcSunDeclination(t);
	double  hourAngle = calcHourAngleSunrise(latitude, solarDec);
  double  delta = longitude - radToDeg(hourAngle);
	double  timeDiff = 4 * delta;	// in minutes of time	
	double  timeUTC = 720 + timeDiff - eqTime;	// in minutes	
  double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0); 


  eqTime = calcEquationOfTime(newt);
  solarDec = calcSunDeclination(newt);
		
		
	hourAngle = calcHourAngleSunrise(latitude, solarDec);
	delta = longitude - radToDeg(hourAngle);
	timeDiff = 4 * delta;
	timeUTC = 720 + timeDiff - eqTime; // in minutes



	return timeUTC;
}

double calcSunsetUTC(double JD, double latitude, double longitude)
{

    double t = calcTimeJulianCent(JD);

		// *** First pass to approximate sunset
    double  eqTime = calcEquationOfTime(t);
    double  solarDec = calcSunDeclination(t);
    double  hourAngle = calcHourAngleSunset(latitude, solarDec);
    double  delta = longitude - radToDeg(hourAngle);
    double  timeDiff = 4 * delta;	// in minutes of time	
    double  timeUTC = 720 + timeDiff - eqTime;	// in minutes	
    double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0); 


    eqTime = calcEquationOfTime(newt);
    solarDec = calcSunDeclination(newt);
		
		
		hourAngle = calcHourAngleSunset(latitude, solarDec);
		delta = longitude - radToDeg(hourAngle);
		timeDiff = 4 * delta;
		timeUTC = 720 + timeDiff - eqTime; // in minutes

		// printf("************ eqTime = %f  \nsolarDec = %f \ntimeUTC = %f\n\n",eqTime,solarDec,timeUTC);


		return timeUTC;
}



