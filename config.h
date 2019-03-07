//
// This is the SolarMiner configuration header file v1.3
// by Jose Peral, japeralsoler@gmail.com
//

// Solar PV plant constants
// http://www.date-and-time.net/?calculate=true
#define LATITUDE          38.2699329      // North (Elche city)
#define LONGITUDE        -0.7125608      // East
#define LOCALOFFSET      1               // UK=0 (GMT +0), Spain=1 (GMT +1). localOffset will be <0 for western hemisphere and >0 for eastern hemisphere

// Regulation loop constants
#define CONSOLE_DISPLAY_TIME        5    // seconds
#define SOLAX_POLL_TIME             10   // to do not spam Solax Wifi API interface.
#define IFTTT_POST_TIME             3    // to do not spam IFTTT server.  
#define SOLAX_STABILIZATION_TIME    15   // 15s. Do not touch. It is parametrized from a Solax Boost X1 with 3KW PV array.
#define REENGAGE_STABILIZATION_TIME 400  // 300s. To filter small impoted power transitories and minimize the number of times the miner switches on/off

#define MIN_SOLAR_POWER             180  //W
#define MIN_PV_VOLTAGE              180  //V

#define SOLAX_REGULATION_THRESHOLD -90   //W
#define KILLING_THRESHOLD          -200  //
#define LOAD1_POWER                -750  //W
#define LOAD2_POWER                -750  //W

// on screen event log
#define LOG_LINES 10
#define LINE_LEN 480

// Antminer API calls
// Set static IP´s for each miner and edit this strings acordingly.
// /dev/shm/ folder is a RAM memory parition, to preserve SD memory endurance.

#define M1_ALIAS              "AntminerS9"
#define M1_BOOT_TIME          170            // 170s for antminer S9 bitmain asicboost firm nov 2018.
#define M1_IP                 "192.168.0.xxx"
#define M1_UNITS              "GHS"
#define M1_SYSTEM_PING        "echo -n \"summary+devs\" | nc 192.168.0.xxx 4028 > /dev/shm/miner_response.json 2> /dev/null"        
#define M1_PING_RES_PATH      "/dev/shm/miner201_response.json"

#define M2_ALIAS              "AntminerL3"
#define M2_BOOT_TIME          130            
#define M2_IP                 "192.168.0.yyy"
#define M2_UNITS              "MHS"
#define M2_SYSTEM_PING        "echo -n \"summary+devs\" | nc 192.168.0.yyy 4028 > /dev/shm/miner_response.json 2> /dev/null"        
#define M2_PING_RES_PATH      "/dev/shm/miner202_response.json"

// IFTTT Loads API calls. 
// If (Web hooks) then (eWelink)
#define IFTTT_API_KEY        "YOUR_IFTTT_API_KEY_HERE"

#define L1_URL_ENGAGE        "https://maker.ifttt.com/trigger/YOURSONOFF1_201_on/with/key/"
#define L1_URL_KILL          "https://maker.ifttt.com/trigger/YOURSONOFF1_201_off/with/key/"

#define L2_URL_ENGAGE        "https://maker.ifttt.com/trigger/YOURSONOFF2_202_on/with/key/"
#define L2_URL_KILL          "https://maker.ifttt.com/trigger/YOURSONOFF2_202_off/with/key/"

#define L3_URL_ENGAGE        "https://maker.ifttt.com/trigger/YOURSONOFF3_on/with/key/"
#define L3_URL_KILL          "https://maker.ifttt.com/trigger/YOURSONOFF3_203_off/with/key/"


// Emoncms
#define EMONCMS_SERVER       "YOUR_EMONCMS_SERVER_URL_HERE/emoncms/input/post"
#define READ_WRITE           "YOUR_EMONCMS_API_KEY_HERE"
