//
// SolarMiner v1.5
//
// by Jose Peral, japeralsoler@gmail.com
// https://github.com/japeral/solarminer
//
// 2019-03-13 v1.5          Added Fine frequency regulation for Load 1, Load 2 is fixed.
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

#define COLOR_WHITE   "\x1b[30m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BLACK   "\x1b[40m"


int    killing_threshold = KILLING_THRESHOLD;           // W
int   enabling_threshold = SOLAX_REGULATION_THRESHOLD;  // W  

int max_solar_power = 0;
int max_solar_at_load_reduction = 0;

// ON/OFF Regulation STATE machine.
/* 
enum{
  DEFAULT = 0,
  KILL_ALL_LS,
  L_1_OFF_L_2_OFF,
  ENGAGE_L_1,
  L_1_ON_L_2_OFF,
  ENGAGE_L_2,
  L_1_ON_L_2_ON, 
  KILL_L_1,
  KILL_L_2  
};*/

// 4 FREQUENCY STEPS for Load 1, FIXED for Load 2, Regulation STATE machine. 
// 
enum{
  DEFAULT = 0,
  SET_L1_OFF_L2_OFF,
      L1_OFF_L2_OFF,
        
  SET_L1_ON__L2_OFF,   
      L1_ON__L2_OFF,
      
  SET_L1_ST1_L2_OFF,
      L1_ST1_L2_OFF,
  SET_L1_ST2_L2_OFF,
      L1_ST2_L2_OFF,
  SET_L1_ST3_L2_OFF,
      L1_ST3_L2_OFF,
  SET_L1_ST4_L2_OFF,
      L1_ST4_L2_OFF,
      
  SET_L1_OFF_L2_ON,
      L1_OFF_L2_ON,
      
  SET_L1_ON__L2_ON,
      L1_ON__L2_ON,
      
  SET_L1_ST1_L2_ON,
      L1_ST1_L2_ON,
  SET_L1_ST2_L2_ON,
      L1_ST2_L2_ON,
  SET_L1_ST3_L2_ON,
      L1_ST3_L2_ON,
  SET_L1_ST4_L2_ON,
      L1_ST4_L2_ON
};
int loads_regulation_state_machine;
char regulation_message[30]={0};

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
time_t loads_regulation_period=ALL_ONOFF_TIME;
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
float solar_kwh_today=0;
float solar_kwh_total=0;

// Averaged imp_pwr
long imp_pwr_now = 0;                     // Current raw sample
long imp_pwr_readings[NUM_READINGS]={0};  // the readings from the analog input
long imp_pwr_index = 0;                   // the index of the current reading
long imp_pwr_total = 0;                   // the running total
long imp_pwr_avg = 0;                     // the average

// Miner variables
char  miner_fields[90][30] = {0};
float miner201_hr=0;
float miner202_hr=0;
float miner203_hr=0;
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

  loads_regulation_state_machine = SET_L1_OFF_L2_OFF; // Sync with IFTTT status.
  
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
      imp_pwr_now = atoi(solax_fields[14]);
      solar_kwh_today = atof(solax_fields[45]);
      solar_kwh_total = atof(solax_fields[46]);

      // Smooth Average of imp_pwr
      imp_pwr_total = imp_pwr_total - imp_pwr_readings[imp_pwr_index];    // Substract previous sample to total.
      imp_pwr_readings[imp_pwr_index] = imp_pwr_now; imp_pwr_index++;     // Store actual reading in readings array
      if(imp_pwr_index >= NUM_READINGS) imp_pwr_index = 0;                // Rollback index
      imp_pwr_total = imp_pwr_total + imp_pwr_now;
      imp_pwr_avg = imp_pwr_total / NUM_READINGS;                         // Use total total to calculate averaged value.
      

      // Solar peak detection.
      if(solar_power > max_solar_power){
        max_solar_power = solar_power;
      }
      
      // House power.
      house_power = -1*(abs(imp_pwr_now) + solar_power);
 
      // Miners poll & power.
      /*
      switch(loads_regulation_state_machine){
      case DEFAULT:      
      case L_1_OFF_L_2_OFF: miners_power = 0;                         
        break; 
      case L_1_ON_L_2_OFF:  miners_power = L1_POWER;    
        break;
      case L_1_ON_L_2_ON:   miners_power = L1_POWER + L2_POWER; 
        break;

      default:
        // No change in load power. 
        break; 
      }//switch */
      

      switch(loads_regulation_state_machine){
      case DEFAULT:      
      case L1_OFF_L2_OFF: miners_power = 0;                       break;
      case L1_ON__L2_OFF: miners_power = 0;                       break;
      case L1_ST1_L2_OFF: miners_power = L1_ST1_POWER;            break;
      case L1_ST2_L2_OFF: miners_power = L1_ST2_POWER;            break;
      case L1_ST3_L2_OFF: miners_power = L1_ST3_POWER;            break;
      case L1_ST4_L2_OFF: miners_power = L1_ST4_POWER;            break;
      case L1_OFF_L2_ON:  miners_power =                0;        break; 
      case L1_ON__L2_ON:  miners_power = L1_POWER     + L2_POWER; break;
      case L1_ST1_L2_ON:  miners_power = L1_ST1_POWER + L2_POWER; break; 
      case L1_ST2_L2_ON:  miners_power = L1_ST2_POWER + L2_POWER; break;  
      case L1_ST3_L2_ON:  miners_power = L1_ST3_POWER + L2_POWER; break; 
      case L1_ST4_L2_ON:  miners_power = L1_ST4_POWER + L2_POWER; break;  
      default:
        // No change in load power. 
        break; 
      }//switch */



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
          
          miner201_hr = atof(miner_fields[16]);       
        }  
/*      }else{
          printf("201_reacheable removed\r\n");
//        fclose(file); // Close ping file.
          miner201_awake = 0;
          miner201_hr = 0;
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
          miner202_hr = atof(miner_fields[16]);       
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
          miner203_hr = atof(miner_fields[16]);       
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
      solar_power,imp_pwr_now,imp_pwr_avg,miners_power,miner201_hr,miner202_hr,miner203_hr,READ_WRITE);
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
      test=head_line+1; if(test>LOG_LINES){head_line=0; log_full=1;} 
      no_event=0;
      
      //   
      // DAY mode
      //
      if(solar_power > MIN_SOLAR_POWER){
      
        strcpy(regulation_message, "DAY - ");      
      
        //
        // Increase Load
        //       -0W                -90W             
        if( imp_pwr_avg > enabling_threshold ){
  
          strcat(regulation_message, COLOR_GREEN "INCREASE" COLOR_RESET);
                                                       
          switch(loads_regulation_state_machine){
          
          // All OFF. 
          case L1_OFF_L2_OFF:
            loads_regulation_state_machine=SET_L1_ON__L2_OFF,
            loads_regulation_period=L1_BOOT_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_OFF_L2_OFF -> SET_L1_ON__L2_OFF\r\n", log_time);
            break;
  
          // Miner 1 ON, and wait for SSH interface to be ready.
          case L1_ON__L2_OFF:  
            loads_regulation_state_machine=SET_L1_ST1_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_ON__L2_OFF -> SET_L1_ST1_L2_OFF\r\n", log_time);          
            break;
   
          // Load 1, 4 steps       
          case L1_ST1_L2_OFF:  // Change to STEP 1 frequency
            loads_regulation_state_machine=SET_L1_ST2_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_ST1_L2_OFF -> SET_L1_ST2_L2_OFF\r\n", log_time);          
            break;
  
          case L1_ST2_L2_OFF:  // Change to STEP 2 frequency
            loads_regulation_state_machine=SET_L1_ST3_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_ST2_L2_OFF -> SET_L1_ST3_L2_OFF\r\n", log_time);          
            break;
  
          case L1_ST3_L2_OFF:  // Change to STEP 3 frequency
            loads_regulation_state_machine=SET_L1_ST4_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_ST3_L2_OFF -> SET_L1_ST4_L2_OFF\r\n", log_time);          
            break;
  
          case L1_ST4_L2_OFF:  // Change to STEP 4 frequency
            loads_regulation_state_machine=SET_L1_OFF_L2_ON,
            loads_regulation_period=L2_BOOT_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (INC) L1_ST4_L2_OFF -> SET_L1_OFF_L2_ON\r\n", log_time);          
            break;
  
          // Miner 1 OFF, Miner 2 ON.
          case L1_OFF_L2_ON:    
            loads_regulation_state_machine=SET_L1_ON__L2_ON,
            loads_regulation_period=L1_BOOT_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (INC) L1_OFF_L2_ON -> SET_L1_ON__L2_ON\r\n", log_time);          
            break;
  
          // Miner 1 ON, and wait for SSH interface to be ready.
          case L1_ON__L2_ON:  
            loads_regulation_state_machine=SET_L1_ST1_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (INC) L1_ON__L2_ON -> SET_L1_ST1_L2_ON\r\n", log_time);          
            break;
            
          // Load 1, 4 steps       
          case L1_ST1_L2_ON:  // Change to STEP 1 frequency
            loads_regulation_state_machine=SET_L1_ST2_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (INC) L1_ST1_L2_ON -> SET_L1_ST2_L2_ON\r\n", log_time);          
            break;
  
          case L1_ST2_L2_ON:  // Change to STEP 2 frequency
            loads_regulation_state_machine=SET_L1_ST3_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //           
            snprintf(event[head_line], LINE_LEN,  "[%s] - (INC) L1_ST2_L2_ON -> SET_L1_ST3_L2_ON\r\n", log_time);          
            break;
  
          case L1_ST3_L2_ON:  // Change to STEP 3 frequency
            loads_regulation_state_machine=SET_L1_ST4_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (INC) L1_ST3_L2_ON -> SET_L1_ST4_L2_ON\r\n", log_time);          
            break;
  
          case L1_ST4_L2_ON:
            loads_regulation_period=ALL_ONOFF_TIME; //       
            no_event=1;
            break;
          
          default: 
            no_event=1; 
            break;        
          }
        }
  
        //
        // Reduce Load.
        //    -1000           -200               
        else if(imp_pwr_avg <= killing_threshold){

           strcat(regulation_message, COLOR_MAGENTA "DECREASE" COLOR_RESET);                    
           
           switch(loads_regulation_state_machine){
          
          // All OFF. 
          case L1_OFF_L2_OFF:
            loads_regulation_period=ALL_ONOFF_TIME; //       
            no_event=1;           
            break;
  
          // Miner 1 ON, and wait for SSH interface to be ready.
          case L1_ON__L2_OFF:  
            loads_regulation_state_machine=SET_L1_OFF_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_ON__L2_OFF -> SET_L1_OFF_L2_OFF\r\n", log_time);          
            break;
   
          // Load 1, 4 steps       
          case L1_ST1_L2_OFF:  // Change to STEP 1 frequency
            loads_regulation_state_machine=SET_L1_OFF_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_ST1_L2_OFF -> SET_L1_OFF_L2_OFF\r\n", log_time);          
            break;
  
          case L1_ST2_L2_OFF:  // Change to STEP 2 frequency
            loads_regulation_state_machine=SET_L1_ST1_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_ST2_L2_OFF -> SET_L1_ST1_L2_OFF\r\n", log_time);         
            break;
  
          case L1_ST3_L2_OFF:  // Change to STEP 3 frequency
            loads_regulation_state_machine=SET_L1_ST2_L2_OFF,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_ST3_L2_OFF -> SET_L1_ST2_L2_OFF\r\n", log_time);          
            break;
  
          case L1_ST4_L2_OFF:  // Change to STEP 4 frequency
            loads_regulation_state_machine=SET_L1_ST3_L2_OFF,
            loads_regulation_period=L2_BOOT_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_ST4_L2_OFF -> SET_L1_ST3_L2_OFF\r\n", log_time);          
            break;
  
  
          // Miner 1 OFF, Miner 2 ON.
          case L1_OFF_L2_ON:    
            loads_regulation_state_machine=SET_L1_ST4_L2_OFF,
            loads_regulation_period=L1_BOOT_TIME; //            
            snprintf(event[head_line], LINE_LEN, "[%s] - (DEC) L1_OFF_L2_ON -> SET_L1_ST4_L2_OFF\r\n", log_time);         
            break;
  
          // Miner 1 ON, and wait for SSH interface to be ready.
          case L1_ON__L2_ON:  
            loads_regulation_state_machine=SET_L1_OFF_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (DEC) L1_ON__L2_ON -> SET_L1_OFF_L2_ON\r\n", log_time);          
            break;
            
          // Load 1, 4 steps       
          case L1_ST1_L2_ON:  // Change to STEP 1 frequency
            loads_regulation_state_machine=SET_L1_OFF_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (DEC) L1_ST1_L2_ON -> SET_L1_OFF_L2_ON\r\n", log_time);          
            break;
  
          case L1_ST2_L2_ON:  // Change to STEP2 frequency
            loads_regulation_state_machine=SET_L1_ST1_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //           
            snprintf(event[head_line], LINE_LEN,  "[%s] - (DEC) L1_ST2_L2_ON -> SET_L1_ST1_L2_ON\r\n", log_time);          
            break;
  
          case L1_ST3_L2_ON:  // Change to STEP 3 frequency
            loads_regulation_state_machine=SET_L1_ST2_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (DEC) L1_ST3_L2_ON -> SET_L1_ST2_L2_ON\r\n", log_time);       
            break;
  
          case L1_ST4_L2_ON:
            loads_regulation_state_machine=SET_L1_ST3_L2_ON,
            loads_regulation_period=L1_RESET_TIME; //            
            snprintf(event[head_line], LINE_LEN,  "[%s] - (DEC) L1_ST4_L2_ON -> SET_L1_ST3_L2_ON\r\n", log_time);
            break;
          
          default: 
            no_event=1; 
            break;        
          }
        }// end decrease
        
        //
        // EMERGENCY_STOP
        //        
/*      else if(imp_pwr){
            strcat(regulation_message, "STOP!!");
            no_event =1;                      
        }
*/

        //
        // IDLE
        //        
        else{
          strcat(regulation_message, COLOR_CYAN "IDLE" COLOR_RESET);                
          no_event =1;
        } // end of day mode cases
                 
      }else{ // night mode
        strcpy(regulation_message, COLOR_BLACK "NIGHT-STANDYBY" COLOR_RESET);
        no_event =1;
      }// if(solar_power > MIN_SOLAR_POWER){
     
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
      
      case SET_L1_OFF_L2_OFF:
        snprintf(ifttt_apicall, 300, L1_ST1_SYS);                               system(ifttt_apicall);  // Before killing L1, preset ST1.
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);                      
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall); miner201_hr = 0; usleep(IFTTT_POST_TIME*1000);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall); miner202_hr = 0; 
        loads_regulation_state_machine=L1_OFF_L2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_OFF_L2_OFF Set\r\n", log_time);              
        break;
                   
      case SET_L1_ON__L2_OFF:   
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_ON_URL IFTTT_API_KEY);   system(ifttt_apicall); usleep(IFTTT_POST_TIME*1000);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);
        loads_regulation_state_machine=L1_ON__L2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ON__L2_OFF Set\r\n", log_time);      
        break;

      case SET_L1_ST1_L2_OFF:
        snprintf(ifttt_apicall, 300, L1_ST1_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);
        loads_regulation_state_machine=L1_ST1_L2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST1_L2_OFF Set\r\n", log_time);  
        break;

      case SET_L1_ST2_L2_OFF:
        snprintf(ifttt_apicall, 300, L1_ST2_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);
        loads_regulation_state_machine=L1_ST2_L2_OFF;
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST2_L2_OFF Set\r\n", log_time);  
        break;
        
      case SET_L1_ST3_L2_OFF:
        snprintf(ifttt_apicall, 300, L1_ST3_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST3_L2_OFF Set\r\n", log_time);  
        loads_regulation_state_machine=L1_ST3_L2_OFF;
        break;

      case SET_L1_ST4_L2_OFF:
        snprintf(ifttt_apicall, 300, L1_ST4_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_ON_URL IFTTT_API_KEY);   system(ifttt_apicall); usleep(IFTTT_POST_TIME*1000);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST4_L2_OFF Set\r\n", log_time);
        loads_regulation_state_machine=L1_ST4_L2_OFF;
        break;
        
      case SET_L1_OFF_L2_ON:
        snprintf(ifttt_apicall, 300, L1_ST1_SYS);                               system(ifttt_apicall);  // Before killing L1, preset ST1.
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);                
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall); usleep(IFTTT_POST_TIME*1000);
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_OFF_URL IFTTT_API_KEY);  system(ifttt_apicall);        
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_OFF_L2_ON Set\r\n", log_time);     
        loads_regulation_state_machine=L1_OFF_L2_ON; 
        break;


      case SET_L1_ON__L2_ON:
        snprintf(ifttt_apicall, 300, "curl -X POST "L1_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);  usleep(IFTTT_POST_TIME*1000);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ON__L2_ON Set\r\n", log_time);     
        loads_regulation_state_machine=L1_ON__L2_ON;  
        break;
        
      case SET_L1_ST1_L2_ON:
        snprintf(ifttt_apicall, 300, L1_ST1_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST1_L2_ON Set\r\n", log_time);
        loads_regulation_state_machine=L1_ST1_L2_ON;
        break;

      case SET_L1_ST2_L2_ON:
        snprintf(ifttt_apicall, 300, L1_ST2_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST2_L2_ON\r\n", log_time);
        loads_regulation_state_machine=L1_ST2_L2_ON;
        break;

      case SET_L1_ST3_L2_ON:
        snprintf(ifttt_apicall, 300, L1_ST3_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST3_L2_ON Set\r\n", log_time);
        loads_regulation_state_machine=L1_ST3_L2_ON;
        break;

      case SET_L1_ST4_L2_ON:
        snprintf(ifttt_apicall, 300, L1_ST4_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, L1_RES_SYS);                               system(ifttt_apicall);
        snprintf(ifttt_apicall, 300, "curl -X POST "L2_ON_URL IFTTT_API_KEY);   system(ifttt_apicall);
        snprintf(event[head_line], LINE_LEN, "[%s] - L1_ST4_L2_ON Set\r\n", log_time);
        loads_regulation_state_machine=L1_ST4_L2_ON;
        break;

  
      default:
        no_event = 1;
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
      printf(         ",I=%2.1f)->-/ Today/Total(%03.2f/%03.2f KWh)|\r\n", pv2_current, solar_kwh_today, solar_kwh_total);
      printf("                                                   |\r\n");
      printf("Imported power (");
      if     ( imp_pwr_avg > 0 )                    printf(COLOR_MAGENTA"%05d W)  --<----<----<----<----<--|\r\n" COLOR_RESET, imp_pwr_avg);
      else if( imp_pwr_avg > enabling_threshold )   printf(COLOR_GREEN  "%05d W)  -------------------------|\r\n" COLOR_RESET, imp_pwr_avg);
      else if( imp_pwr_avg > killing_threshold)     printf(COLOR_CYAN   "%05d W)  -->---->---->---->---->--|\r\n" COLOR_RESET, imp_pwr_avg);
      else                                             printf(COLOR_RED    "%05d W)  ==>====>====>====>====>==|\r\n" COLOR_RESET, imp_pwr_avg);

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
      printf(": %02.2f ", miner201_hr); printf(M1_UNITS); printf("\r\n");
      
      printf("                                                                   |--> ");
      if(miner202_awake==1) printf(M2_ALIAS COLOR_GREEN " " M2_IP COLOR_RESET);
      else                  printf(COLOR_RESET M2_IP);
      printf(": %02.2f ", miner202_hr); printf(M2_UNITS); printf("\r\n");
      
      printf("                                                                   \\--> ");
      if(miner203_awake==1) printf(M3_ALIAS COLOR_GREEN " " M3_IP COLOR_RESET);
      else                  printf(COLOR_RESET M3_IP);
      printf(": %02.2f ", miner203_hr); printf(M3_UNITS); printf("\r\n");
      
 
      printf("\r\n");
      
      printf("       Max solar peak ever seen since start = %04d W\r\n", max_solar_power);
      printf(" Max solar power before last load reduction = %04d W\r\n", max_solar_at_load_reduction); 
                 
      // Regulation loop
      printf("_____________________________________________________________________________________________________\r\n");
      printf("# Regulation loop # \r\n");
      printf("Time Since Last Regulation: %d s, Regulation period: %d s ", time(NULL)-loads_regulation_timer, loads_regulation_period);
      printf("NextRegulationAction:%s\r\n", regulation_message); 
      printf("Min Solar power to engage loads: %d W\r\n", MIN_SOLAR_POWER);  

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
