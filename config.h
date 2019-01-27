//
// This is the SolarMiner configuration header file v1.0
// by Jose Peral, japeralsoler@gmail.com
//

// Solar PV plant constants
#define LATITUDE         38
#define LONGITUDE        -0.5
#define LOCALOFFSET      1
#define DAYLIGHTSAVINGS  0

// Regulation loop constants
#define SOLAX_POLL_TIME             10   // to do not spam Solax Wifi API interface.
#define IFTTT_POST_TIME             3    // to do not spam IFTTT server.  
#define MINER_BOOT_TIME             170  // 170s for antminer S9 bitmain asicboost firm nov 2019. Adjust acordingly if you have auto frec cal on.
#define SOLAX_STABILIZATION_TIME    15   // 15s. Do not touch. It is parametrized from a Solax Boost X1 with 3KW PV array.
#define REENGAGE_STABILIZATION_TIME 300  // 300s. To filter small impoted power transitories and minimize the number of times the miner switches on/off

#define MIN_SOLAR_POWER             10   //W
#define MIN_PV_VOLTAGE              200  //V

#define SOLAX_REGULATION_THRESHOLD -90   //W
#define KILLING_THRESHOLD          -200  //
#define LOAD1_POWER                -750  //W
#define LOAD2_POWER                -750  //W

// on screen event log
#define LOG_LINES 20
#define LINE_LEN 480

// Antminer API calls
// Set static IP´s for each miner and edit this strings acordingly.
// /dev/shm/ folder is a RAM memory parition, to preserve SD memory endurance.
#define MINER1_IP             "192.168.0.201"
#define PING_SYSTEM_MINER1    "echo -n \"summary+devs\" | nc 192.168.0.201 4028 > /dev/shm/miner201_response.json 2> /dev/null"        
#define PING_RESPONSE_MINER1  "/dev/shm/miner201_response.json"

#define MINER2_IP             "192.168.0.202"
#define PING_SYSTEM_MINER2    "echo -n \"summary+devs\" | nc 192.168.0.202 4028 > /dev/shm/miner202_response.json 2> /dev/null"        
#define PING_RESPONSE_MINER2  "/dev/shm/miner202_response.json"


// IFTTT API calls. 
// If (Web hooks) then (eWelink)
#define ENGAGE_SYSTEM_MINER1 "curl -X POST https://maker.ifttt.com/trigger/AntminerS9_201_on/with/key/YOUR_API_KEY_HERE"
#define KILL_SYSTEM_MINER1   "curl -X POST https://maker.ifttt.com/trigger/AntminerS9_201_off/with/key/YOUR_API_KEY_HERE"

#define ENGAGE_SYSTEM_MINER2 "curl -X POST https://maker.ifttt.com/trigger/AntminerS9_202_on/with/key/YOUR_API_KEY_HERE"
#define KILL_SYSTEM_MINER2   "curl -X POST https://maker.ifttt.com/trigger/AntminerS9_202_off/with/key/YOUR_API_KEY_HERE"