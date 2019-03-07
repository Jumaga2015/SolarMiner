//
// SolarMiner v1.1
//
// by Jose Peral, japeralsoler@gmail.com
// https://github.com/japeral/solarminer
//
//
// 2019-03-07 v1.4          Added smoother averager for controlled variable 'Imported power'.
// 2019-03-07 v1.3 release. Fine frequency control pushing cgminer.conf file to Antminer.
//
// 2019-01-29 v1.1 release. Emoncms datalogger integration.
//                          Solar Peak time/date
//                          Started / duration timers
//
// 2019-01-27 v1.0 release. Inverter: Solax-Boost-X1 
//                          Loads Control: Sonoff trought Webhooks+eWelink API calls
//                          Regulation loop: Bitmain Antminer S9
// Instructions:
// 1) Editing config.h file API calls.
// 2) Compile executing ./compile
// 3) Launch ./solarminer

#include <stdio.h>
#include <stdlib.h>        // system()
#include <unistd.h>        // sleep()
#include <time.h>          // usleep() duration in nanoseconds (check the man page #man time)
#include "sun.h"           // Sunset an Sunrise functions
#include "config_jose.h"

#include <string.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"


int    killing_threshold = KILLING_THRESHOLD;           // W
int   enabling_threshold = SOLAX_REGULATION_THRESHOLD;  // W  

int max_solar_power = 0;
int max_solar_at_load_reduction = 0;

// SOLAX STATE machine
enum{
  DEFAULT = 0,
  KILL_ALL_LOADS,
  LOAD_1_OFF_LOAD_2_OFF,
  ENGAGE_LOAD_1,
  LOAD_1_ON_LOAD_2_OFF,
  ENGAGE_LOAD_2,
  LOAD_1_ON_LOAD_2_ON, 
  KILL_LOAD_1,
  KILL_LOAD_2  
};
int loads_regulation_state_machine;

int load1_engage_counter=0;
int load2_engage_counter=0;
int load1_disengage_counter=0;
int load2_disengage_counter=0;

// Timers
time_t t;
struct tm *info;
time_t start_timer=0;
time_t elapsed_timer=0;
time_t solax_poll_timer=0;
time_t display_timer=0;
time_t loads_regulation_timer=0;
time_t loads_regulation_period=5;
time_t ifttt_post_timer=0;

// Solax variables
char  solax_fields[90][30] = {0};
int   solar_power=0;
int   pv1_voltage=0;
float pv1_current=0;
int   pv2_voltage=0;
float pv2_current=0;
int   house_power=0;     
int   miners_power=0;     
float today_solar_kwh=0;

//int   imported_power=0;

// Averaged Imported_power
//const  int numReadings=10;                    // Number of average samples.
#define NUM_READINGS  10
int imported_power_now = 0;                     // Current raw sample
int imported_power_readings[NUM_READINGS]={0};  // the readings from the analog input
int imported_power_index = 0;                   // the index of the current reading
int imported_power_total = 0;                   // the running total
int imported_power_avg = 0;                     // the average
//float fimported_power;


// Miner variables
char  miner_fields[90][30] = {0};
float miner201_hashrate=0;
float miner202_hashrate=0;
float miner203_hashrate=0;
unsigned char miner201_awake =0;
unsigned char miner202_awake =0;
unsigned char miner203_awake =0;

// Event log variables
char event[LOG_LINES][LINE_LEN] = {0};
int tail_line=0;
char log_time[25]={0}; // 2019/01/01-12:01:01
int log_full=0;
int head_line=0;
int no_event=0;
int test;

int main(){		

  system("clear");

  loads_regulation_state_machine = KILL_ALL_LOADS; // Sync with IFTTT status.
  
  // Start time
  char start_time[50];  
  time(&start_timer); info=localtime(&start_timer); strftime(start_time,50,"%Y-%m-%d %H:%M:%S", info);
  start_timer = time (NULL);   

  // Logging files names preparation
  char logfile_name[50];
  char rawfile_name[50];  
  time(&t); info=localtime(&t); strftime(log_time,25,"%Y%m%d-%H%M%S", info);
  snprintf(logfile_name, 50, "logs/%s_events.log", log_time);
  snprintf(rawfile_name, 50, "logs/%s_rawdata.log", log_time);  

  FILE *file;
  FILE *raw_file;
  unsigned int i, j;

  while(1){
  
    usleep(500); // sleep for CPU saving

    //--------------------------------------------------------------------------
    // Data adquisition: Solax polling task
    //
    if( time(NULL) > (solax_poll_timer+SOLAX_POLL_TIME)){
      solax_poll_timer=time(NULL);
      system("curl -X POST 5.8.8.8:80/?optType=ReadRealTimeData > /dev/shm/solaxresponse.json 2> /dev/null");  // stdout to ram file, stderr to null
        
      // Response file open               
      file=fopen("/dev/shm/solaxresponse.json","r");
      
      // Open raw file in append mode
      raw_file=fopen(rawfile_name,"a+"); ; 

      // Comma response parser    
      for (j=0; j<81; j++){
        for (i = 0; i < 30; ++i){
          int c = getc(file);      
          fwrite((char*)&c, 1, 1, raw_file);

          if ( c == ',' || c == '}' || c == '[' ){
            solax_fields[j][i] = 0;
            break;
          }
          solax_fields[j][i] = c;
        }
      }
      fclose(file);
      fwrite("\r\n", 1, 2, raw_file);
      fclose(raw_file);

/*
     //Parser debugger      
     for(j=0; j<81; j++){
       if(solax_fields[j][0]!=0){      
         printf("%0d : %s\r\n", j, solax_fields[j]);
       }
     }
*/


      // Array to integer values conversion
      pv1_current = atof(solax_fields[4]);
      pv2_current = atof(solax_fields[5]);
      pv1_voltage = atoi(solax_fields[6]);
      pv2_voltage = atoi(solax_fields[7]);
      solar_power = atoi(solax_fields[10]);
      imported_power_now = atoi(solax_fields[14]);
      today_solar_kwh = atof(solax_fields[45]);

      // Smooth Average of Imported_power
      imported_power_total = imported_power_total - imported_power_readings[imported_power_index];    // Substract previous sample to total.
      imported_power_readings[imported_power_index] = imported_power_now; imported_power_index++;     // Store actual reading in readings array
      if(imported_power_index >= NUM_READINGS) imported_power_index = 0;                              // Rollback index
      imported_power_total = imported_power_total + imported_power_now;
      imported_power_avg = imported_power_total / NUM_READINGS;                                       // Use total total to calculate averaged value.
      

      // Solar peak detection.
      if(solar_power > max_solar_power){
        max_solar_power = solar_power;
      }
      
      // House power.
      house_power = -1*(abs(imported_power_now) + solar_power);
 
      // Miners poll & power.
      switch(loads_regulation_state_machine){
      case DEFAULT:      
      case LOAD_1_OFF_LOAD_2_OFF: miners_power = 0;                         
        break; 
      case LOAD_1_ON_LOAD_2_OFF:  miners_power = LOAD1_POWER;    
        break;
      case LOAD_1_ON_LOAD_2_ON:   miners_power = LOAD1_POWER + LOAD2_POWER; 
        break;
      default:
        // No change in load power. 
        break; 
      }//switch

      // Miner 1 polling
      //system("ping -c 1 192.168.0.201 &> /dev/null && echo success || echo fail");
//    system("rm /dev/shm/201_reacheable");
//    system("ping -c 1 192.168.0.201 &> /dev/null && touch /dev/shm/201_reacheable || rm /dev/shm/201_reacheable");
//    file=fopen("/dev/shm/201_reacheable","r");
//    printf("miner1 file %d\r\n", file);
//    if ( file<0 ){ // if ping success

//      printf("201_reacheable touched\r\n");      
//      fclose(file); // Close ping file.
        miner201_awake = 1;
        
        system(M1_SYSTEM_PING);            // Ping bmminer                      
        file=fopen(M1_PING_RES_PATH,"r");  // Miner 1 response parse

        // Comma response parser    
        if (file > 0){ 
          for (j=0; j<72; j++){
            for (i = 0; i < 30; ++i){
              int c = getc(file);
              if ( c == ',' || c == '='){
                miner_fields[j][i] = 0;
                break;
              }
              miner_fields[j][i] = c;
            }
          }
          fclose(file);
            
          //Parser debugger      
          //for(j=0; j<16; j++){
          //  if(miner_fields[j][0]!=0){      
          //    printf("%0d : %s\r\n", j, miner_fields[j]);
          //  }
          //}
          //usleep(2000);
          
          miner201_hashrate = atof(miner_fields[16]);       
        }  
/*      }else{
          printf("201_reacheable removed\r\n");
//        fclose(file); // Close ping file.
          miner201_awake = 0;
          miner201_hashrate = 0;
      }
*/
   
      // Miner 2 polling
        miner202_awake = 1;              
        system(M2_SYSTEM_PING);             // Ping bmminer
        file=fopen(M2_PING_RES_PATH,"r");   // Miner 1 response parse
        // Comma response parser    
        if (file > 0){ 
          for (j=0; j<72; j++){
            for (i = 0; i < 30; ++i){
              int c = getc(file);
              if ( c == ',' || c == '='){
                miner_fields[j][i] = 0;
                break;
              }
              miner_fields[j][i] = c;
            }
          }
          fclose(file);         
          miner202_hashrate = atof(miner_fields[16]);       
        }  

       // Miner 3 polling
        miner203_awake = 1;       
        system(M3_SYSTEM_PING);              // Ping bmminer               
        file=fopen(M3_PING_RES_PATH,"r");    // Miner 3 response parse
        // Comma response parser    
        if (file > 0){ 
          for (j=0; j<72; j++){
            for (i = 0; i < 30; ++i){
              int c = getc(file);
              if ( c == ',' || c == '='){
                miner_fields[j][i] = 0;
                break;
              }
              miner_fields[j][i] = c;
            }
          }
          fclose(file);
          miner203_hashrate = atof(miner_fields[16]);       
      }
 
      /*  // Debug event log.     
      time(&t); info=localtime(&t); strftime(log_time,25,"%Y/%m/%d-%H:%M:%S", info); 
      test=head_line+1; if(test>LOG_LINES){head_line=0; log_full=1;} no_event=0;
      snprintf(event[head_line], LINE_LEN, "[%s] - Data adquisition.\r\n", log_time);
      FILE *log_file; log_file=fopen(logfile_name,"a+"); fwrite(event[head_line], 1, strlen(event[head_line]), log_file); fclose(log_file);
      if (!no_event) head_line++; 
      */     
 
      // Emoncms logger data feed
      char emoncms_apicall[300]={0};
      snprintf(emoncms_apicall, 300, 
      "curl --data \"node=solarminer&json={solar_pwr:%d,imported_pwr:%d,imported_pwr_avg:%d,miners_pwr:%d,"
      M1_ALIAS"_201_"M1_UNITS":%.0f,"
      M2_ALIAS"_202_"M2_UNITS":%.0f,"
      M3_ALIAS"_203_"M3_UNITS":%.0f}&apikey=%s\" \""EMONCMS_SERVER"\"",
      solar_power,imported_power_now,imported_power_avg,miners_power,miner201_hashrate,miner202_hashrate,miner203_hashrate,READ_WRITE);
      //printf(emoncms_apicall); printf("\r\n");
      system(emoncms_apicall);
      
    }// end solax polling task
    //--------------------------------------------------------------------------



    //--------------------------------------------------------------------------
    // Load regulation task.
    //   
    if( time(NULL) > (loads_regulation_timer+loads_regulation_period) ){
      loads_regulation_timer=time(NULL);
      
      time(&t); info=localtime(&t); strftime(log_time,25,"%Y/%m/%d-%H:%M:%S", info); 
      test=head_line+1; if(test>LOG_LINES){head_line=0; log_full=1;} no_event=0;
                  
      // Increase Load
      //       -0W                -90W             
      if( (imported_power_avg > enabling_threshold) && (solar_power > MIN_SOLAR_POWER) ){
      
        loads_regulation_period=M1_BOOT_TIME; //
                                        
        switch(loads_regulation_state_machine){
        case LOAD_1_OFF_LOAD_2_OFF:
          loads_regulation_state_machine=ENGAGE_LOAD_1;
          loads_regulation_period=M1_BOOT_TIME;                   
          snprintf(event[head_line], LINE_LEN, 
          "[%s] - Increase.1OFF/2OFF.solar_power=%d.imported_power_avg=%d>enabling_threshold=%d.Engage 1.next loads_regulation_period=%d s\r\n",
          log_time,solar_power,imported_power_avg,enabling_threshold,loads_regulation_period);          
          break;
        case LOAD_1_ON_LOAD_2_OFF:
          loads_regulation_state_machine=ENGAGE_LOAD_2;
          loads_regulation_period=M2_BOOT_TIME;                   
          snprintf(event[head_line], LINE_LEN, 
          "[%s] - Increase.1ON/2OFF.solar_power=%d.imported_power_avg=%d>enabling_threshold=%d.Engage 2.next loads_regulation_period=%d s\r\n",
          log_time,solar_power,imported_power_avg,enabling_threshold,loads_regulation_period);          
          break;
        default:
//          snprintf(event[head_line], LINE_LEN, 
//          "[%s] - Increase.DEFAULT.solar_power=%d.imported_power_avg=%d>enabling_threshold=%d.No action.next loads_regulation_period=%d s\r\n",
//          log_time,solar_power,imported_power_avg,enabling_threshold,loads_regulation_period);
          no_event=1;
          break;        
        case LOAD_1_ON_LOAD_2_ON:       
//          snprintf(event[head_line], LINE_LEN, 
//          "[%s] - Increase.1ON/2ON.solar_power=%d.imported_power_avg=%d>enabling_threshold=%d.No action.next loads_regulation_period=%d s\r\n",
//          log_time,solar_power,imported_power_avg,enabling_threshold,loads_regulation_period);      
          no_event=1;    
          break;
        }
      }
             // Reduce Load.
             //    -1000           -200
      else if(imported_power_avg <= killing_threshold){   
      
        loads_regulation_period=REENGAGE_STABILIZATION_TIME;
        
        switch(loads_regulation_state_machine){
        default:
//          snprintf(event[head_line], LINE_LEN,
//          "[%s] - Decrease.DEFAULT.solar_power=%d.imported_power_avg=%d>killing_threshold=%d.No action.next loads_regulation_period=%d s\r\n",
//          log_time,solar_power,imported_power_avg,killing_threshold,loads_regulation_period);
          no_event=1;
          break;
        case LOAD_1_OFF_LOAD_2_OFF:
//          snprintf(event[head_line], LINE_LEN,
//          "[%s] - Decrease.1OFF/2OFF.solar_power=%d.imported_power_avg=%d>killing_threshold=%d.No action.next loads_regulation_period=%d s\r\n",
//          log_time,solar_power,imported_power_avg,killing_threshold,loads_regulation_period);
          no_event=1;        
          break; 
        case LOAD_1_ON_LOAD_2_OFF:
          loads_regulation_state_machine=KILL_LOAD_1;
          snprintf(event[head_line], LINE_LEN, 
          "[%s] - Decrease.1ON/2OFF.solar_power=%d.imported_power_avg=%d>killing_threshold=%d.Kill 1.next loads_regulation_period=%d s\r\n",
          log_time,solar_power,imported_power_avg,killing_threshold,loads_regulation_period);
          max_solar_at_load_reduction = solar_power;
          break;
        case LOAD_1_ON_LOAD_2_ON:
          loads_regulation_state_machine=KILL_LOAD_2;        
          snprintf(event[head_line], LINE_LEN,
          "[%s] - Decrease.1ON/2ON.solar_power=%d.imported_power_avg=%d>killing_threshold=%d.Kill 2.next loads_regulation_period=%d s\r\n",
          log_time,solar_power,imported_power_avg,killing_threshold,loads_regulation_period);
          max_solar_at_load_reduction = solar_power;
          break;      
        }//case                 
      }else{
        no_event =1;
      }// if
      
      // Write event[] into log_file
      if (!no_event){
        FILE *log_file; log_file=fopen(logfile_name,"a+"); fwrite(event[head_line], 1, strlen(event[head_line]), log_file); fclose(log_file);      
        head_line++; 
      } 
           
    }// if loads_regulation_decrease_timer
    //--------------------------------------------------------------------------    

    //--------------------------------------------------------------------------
    // IFTTT post regulation task
    //   
    if( time(NULL) > (ifttt_post_timer+IFTTT_POST_TIME) ){
      ifttt_post_timer=time(NULL);

      time(&t); info=localtime(&t); strftime(log_time,25,"%Y/%m/%d-%H:%M:%S", info); 
      test=head_line+1; if(test>LOG_LINES){head_line=0; log_full=1;} no_event = 0;

      char ifttt_apicall[300]={0};
              
      // Regulation outputs
      switch(loads_regulation_state_machine){
                   
      case ENGAGE_LOAD_1:   
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_URL_ENGAGE IFTTT_API_KEY);
        //printf(ifttt_apicall); printf("\r\n");
        system(ifttt_apicall);
        loads_regulation_state_machine = LOAD_1_ON_LOAD_2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - Load 1 engaged\r\n", log_time);          
        load1_engage_counter++;      
        break;
      
      case ENGAGE_LOAD_2:
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_URL_ENGAGE IFTTT_API_KEY);
        //printf(ifttt_apicall); printf("\r\n");
        system(ifttt_apicall);
        loads_regulation_state_machine = LOAD_1_ON_LOAD_2_ON;
        snprintf(event[head_line], LINE_LEN, "[%s] - Load 2 engaged\r\n", log_time);
        load2_engage_counter++;    
        break;
          
      case KILL_LOAD_1:
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_URL_KILL IFTTT_API_KEY);
        //printf(ifttt_apicall); printf("\r\n");
        system(ifttt_apicall);
        loads_regulation_state_machine = LOAD_1_OFF_LOAD_2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - Load 1 disengaged\r\n", log_time); 
        load1_disengage_counter++;
        miner201_hashrate=0;       
        break;
        
      case KILL_LOAD_2:
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_URL_KILL IFTTT_API_KEY);
        //printf(ifttt_apicall); printf("\r\n");
        system(ifttt_apicall);        
        loads_regulation_state_machine = LOAD_1_ON_LOAD_2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - Load 2 disengaged\r\n", log_time);
        load2_disengage_counter++;
        miner202_hashrate=0;
        break;
  
      default:
        no_event = 1;
        break;
      
      case KILL_ALL_LOADS:
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_URL_KILL IFTTT_API_KEY);
        printf(ifttt_apicall); printf("\r\n");         
        system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_URL_KILL IFTTT_API_KEY);
        printf(ifttt_apicall); printf("\r\n");         
        system(ifttt_apicall);
        loads_regulation_state_machine=LOAD_1_OFF_LOAD_2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - Load 1 & 2 disengaged\r\n", log_time);  
        miner201_hashrate = 0;
        miner202_hashrate = 0;      
        break;
      }// switch    
      
      if (!no_event){
        FILE *log_file; log_file=fopen(logfile_name,"a+"); fwrite(event[head_line], 1, strlen(event[head_line]), log_file); fclose(log_file);      
        head_line++; 
      }      
         
    }// if ifttt regulation task
    //--------------------------------------------------------------------------
       
    //--------------------------------------------------------------------------
    // Refresh display task
    //
    if( time(NULL) > (display_timer+CONSOLE_DISPLAY_TIME)){
      display_timer=time(NULL);
    	  
      system("clear");
      printf("\r\nSolar Miner by Jose Peral v1.0. Press CTRL+Z to STOP"); 

      // Elapsed time            
      elapsed_timer = time(NULL)-start_timer;
      int remainder, forDays = elapsed_timer / 86400; remainder = elapsed_timer % 86400; int forHours = remainder / 3600;
      remainder = elapsed_timer % 3600; int forMinutes = remainder / 60, forSeconds = remainder % 60;                   
      printf(" Started:[%s]-[%dday %02d:%02d:%02d]\r\n", start_time, forDays, forHours, forMinutes, forSeconds);
      
      printf("_____________________________________________________________________________________________________\r\n");
      printf("# Ambient conditions #\r\n");
      time_t now_timer; time(&now_timer); info=localtime(&now_timer);      
      printf("Date: %02d/%02d/%02d, Location: %.04fN %.04fE, ", info->tm_year+1900, info->tm_mon+1, info->tm_mday, LATITUDE, LONGITUDE);
      printf("Sunrise=");   printSunrise(info->tm_year+1900, info->tm_mon+1, info->tm_mday, LATITUDE, LONGITUDE, LOCALOFFSET, info->tm_isdst); 
      printf(" | Sunset="); printSunset (info->tm_year+1900, info->tm_mon+1, info->tm_mday, LATITUDE, LONGITUDE, LOCALOFFSET, info->tm_isdst); 
      float sunriseT = calculateSunrise(info->tm_year+1900, info->tm_mon+1, info->tm_mday, LATITUDE, LONGITUDE, LOCALOFFSET, info->tm_isdst);      
      float sunsetT = calculateSunset(info->tm_year+1900, info->tm_mon+1, info->tm_mday, LATITUDE, LONGITUDE, LOCALOFFSET, info->tm_isdst);
      printf(" | Sunshine=%fh\r\n", sunsetT-sunriseT);
      
      double julianday = calcJD(info->tm_year+1900,info->tm_mon+1,info->tm_mday);
      printf("JD=%.02f sunrise=%.02f sunset=%.02f\r\n", calcSunriseUTC(julianday, LATITUDE, LONGITUDE), calcSunsetUTC(julianday, LATITUDE, LONGITUDE));
      
      printf("\r\n");
       
      printf("_____________________________________________________________________________________________________\r\n");
      printf("# Power routing # \r\n");
      printf("Inverter detected: %s, %s, %s, %s\r\n", solax_fields[76], solax_fields[77], solax_fields[1], solax_fields[2]);
      printf("\r\n"); 
      
      printf("PV1 (V="); if (pv1_voltage > MIN_PV_VOLTAGE) printf(COLOR_GREEN "%03d" COLOR_RESET, pv1_voltage);
                         else                              printf(COLOR_RED "%03d" COLOR_RESET, pv1_voltage);
      printf(          ",I=%2.1f)->-\\\r\n", pv1_current);
      printf("                     |->- Solar Power ("); if (solar_power > 0) printf(COLOR_YELLOW "%05d" COLOR_RESET, solar_power);
                                                         else                 printf(COLOR_RESET  "%05d", solar_power);
      printf("W) ->-\\\r\n"); 
      printf("PV2 (V="); if (pv2_voltage > MIN_PV_VOLTAGE) printf(COLOR_GREEN "%03d" COLOR_RESET, pv2_voltage);
                         else                              printf(COLOR_RED "%03d" COLOR_RESET, pv2_voltage);
      printf(         ",I=%2.1f)->-/         Today ( %03.1f KWh)     |\r\n", pv2_current, today_solar_kwh);
      printf("                                                   |\r\n");
      printf("Imported power (");
      if     ( imported_power_avg > 0 )                    printf(COLOR_MAGENTA"%05d W)  --<----<----<----<----<--|\r\n" COLOR_RESET, imported_power_avg);
      else if( imported_power_avg > enabling_threshold )   printf(COLOR_GREEN  "%05d W)  -------------------------|\r\n" COLOR_RESET, imported_power_avg);
      else if( imported_power_avg > killing_threshold)     printf(COLOR_CYAN   "%05d W)  -->---->---->---->---->--|\r\n" COLOR_RESET, imported_power_avg);
      else                                             printf(COLOR_RED    "%05d W)  ==>====>====>====>====>==|\r\n" COLOR_RESET, imported_power_avg);

      printf("                                                   V\r\n");
      printf("                                                   |\r\n");
      printf("                         Total Power usage now (%05d W)\r\n", house_power);
      printf("                                                   |\r\n");
      printf("                                                   V\r\n");
      printf("                                                   |----> House  (%05d W)\r\n", house_power - miners_power);
      printf("                                                   \\----> Miners (%05d W)\r\n", miners_power);

      printf("                                                                   |--> ");
      if(miner201_awake==1) printf(M1_ALIAS COLOR_GREEN " " M1_IP COLOR_RESET);
      else                  printf(COLOR_RESET M1_IP);
      printf(": %02.2f ", miner201_hashrate); printf(M1_UNITS); printf("\r\n");
      
      printf("                                                                   |--> ");
      if(miner202_awake==1) printf(M2_ALIAS COLOR_GREEN " " M2_IP COLOR_RESET);
      else                  printf(COLOR_RESET M2_IP);
      printf(": %02.2f ", miner202_hashrate); printf(M2_UNITS); printf("\r\n");
      
      printf("                                                                   \\--> ");
      if(miner203_awake==1) printf(M3_ALIAS COLOR_GREEN " " M3_IP COLOR_RESET);
      else                  printf(COLOR_RESET M3_IP);
      printf(": %02.2f ", miner203_hashrate); printf(M3_UNITS); printf("\r\n");
      
 
      printf("\r\n");
      
      printf("       Max solar peak ever seen since start = %04d W\r\n", max_solar_power);
      printf(" Max solar power before last load reduction = %04d W\r\n", max_solar_at_load_reduction); 
      //printf("\r\n");
 
                 
      // Regulation loop
      printf("_____________________________________________________________________________________________________\r\n");
      printf("# Regulation loop # \r\n");
      printf("elapsed: %d s, period: %d s\r\n", time(NULL)-loads_regulation_timer, loads_regulation_period);
      printf("Min Solar power to engage loads: %d W\r\n", MIN_SOLAR_POWER);
      printf("Engaged:Disengaged > Load 1 %d:%d | Load 2 %d:%d\r\n", 
      load1_engage_counter,load1_disengage_counter,load2_engage_counter,load2_disengage_counter);

/*
      printf("Loads regulation State machine:");
      switch(loads_regulation_state_machine){
      case DEFAULT:               printf("[DEFAULT"); break;
      case KILL_ALL_LOADS:        printf("[KILL_ALL_LOADS"); break;
      case ENGAGE_LOAD_1:         printf("[ENGAGE_LOAD_1"); break;                           
      case ENGAGE_LOAD_2:         printf("[ENGAGE_LOAD_2"); break;
      case KILL_LOAD_1:           printf("[KILL_LOAD_1"); break;
      case KILL_LOAD_2:           printf("[KILL_LOAD_2"); break;                                 
      case LOAD_1_OFF_LOAD_2_OFF: printf("[LOAD_1="); printf(COLOR_RED "OFF" COLOR_RESET); printf(" | LOAD_2="); printf(COLOR_RED "OFF" COLOR_RESET); break;
      case LOAD_1_ON_LOAD_2_OFF:  printf("[LOAD_1="); printf(COLOR_GREEN "ON" COLOR_RESET); printf(" | LOAD_2="); printf(COLOR_RED "OFF" COLOR_RESET); break;
      case LOAD_1_ON_LOAD_2_ON:   printf("[LOAD_1="); printf(COLOR_GREEN "ON" COLOR_RESET); printf(" | LOAD_2="); printf(COLOR_GREEN "ON" COLOR_RESET); break;
      }
      printf("]\r\n");     
*/
          

      // Events log
      //printf("\r\n");
      printf("_____________________________________________________________________________________________________\r\n");      
      printf("# Recent event log # \r\n");      
      int i;
      if(log_full==0){
        for(i=0; i<LOG_LINES; i++){
          printf("%s",event[i]);
        }// for
      }else{
        for(i=(head_line); i<LOG_LINES; i++){
          printf("%s",event[i]);;
        }
        for(i=0; i<=(head_line-1); i++){
          printf("%s",event[i]);
        }// for
      }// if
    } // Display task
    //--------------------------------------------------------------------------
            
  } // while (1)
    
	return 0;
} // main()
