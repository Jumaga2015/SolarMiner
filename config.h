//
// This is the SolarMiner configuration header file
// by Jose Peral, japeralsoler@gmail.com
//

// Solar PV plant constants
// http://www.date-and-time.net/?calculate=true
#define LATITUDE         38.2699329      // North (Elche city)
#define LONGITUDE        -0.7125608      // East
#define LOCALOFFSET       1              // UK=0 (GMT +0), Spain=1 (GMT +1). localOffset will be <0 for western hemisphere and >0 for eastern hemisphere

// Regulation loop constants
#define NUM_READINGS                10   // Number of periods to average the regulation loop feedback signal.

#define CONSOLE_DISPLAY_TIME        5    // seconds
#define SOLAX_POLL_TIME             5   // to do not spam Solax Wifi API interface.
#define IFTTT_POST_TIME             3    // to do not spam IFTTT server.  
#define ALL_ONOFF_TIME              5    // Initial delay till

#define MIN_SOLAR_POWER             100  //W
#define MIN_PV_VOLTAGE              180  //V

#define SOLAX_REGULATION_THRESHOLD -80   //W
#define KILLING_THRESHOLD          -120  //W

// on screen event log
#define LOG_LINES 10
#define LINE_LEN  480

// Antminer API calls
// Set static IP´s for each miner and edit this strings acordingly.
// /dev/shm/ folder is a RAM memory parition, to preserve SD memory endurance.
#define M1_ALIAS              "AntminerS9"
#define M1_BOOT_TIME          170            // 170s for antminer S9 bitmain asicboost firm nov 2018.
#define M1_IP                 "192.168.0.201"
#define M1_UNITS              "GHS"
#define M1_SYSTEM_PING        "echo -n \"summary+devs\" | nc 192.168.0.201 4028 > /dev/shm/miner201_response.json 2> /dev/null"        
#define M1_PING_RES_PATH      "/dev/shm/miner201_response.json"
#define M1_RESET_TIME         200
  
#define M2_ALIAS              "AntminerL3"
#define M2_BOOT_TIME          150            
#define M2_IP                 "192.168.0.202"
#define M2_UNITS              "MHS"
#define M2_SYSTEM_PING        "echo -n \"summary+devs\" | nc 192.168.0.202 4028 > /dev/shm/miner202_response.json 2> /dev/null"        
#define M2_PING_RES_PATH      "/dev/shm/miner202_response.json"
#define M2_RESET_TIME         200
#define M2_ST1_SYS            "sshpass -p \"Password\" sudo scp /home/pi/solarminer/AntminerL3/cgminer_100.conf root@192.168.0.202:/config/cgminer.conf"
#define M2_ST2_SYS            "sshpass -p \"Password\" sudo scp /home/pi/solarminer/AntminerL3/cgminer_200.conf root@192.168.0.202:/config/cgminer.conf"
#define M2_ST3_SYS            "sshpass -p \"Password\" sudo scp /home/pi/solarminer/AntminerL3/cgminer_300.conf root@192.168.0.202:/config/cgminer.conf"
#define M2_ST4_SYS            "sshpass -p \"Password\" sudo scp /home/pi/solarminer/AntminerL3/cgminer_400.conf root@192.168.0.202:/config/cgminer.conf"
#define M2_RES_SYS            "sshpass -p \"Password\" sudo ssh root@192.168.0.202 /etc/init.d/cgminer.sh restart"

#define M3_ALIAS              "AntminerS9"
#define M3_BOOT_TIME          170            // 170s for antminer S9 bitmain asicboost firm nov 2018.
#define M3_IP                 "192.168.0.203"
#define M3_UNITS              "GHS"
#define M3_SYSTEM_PING        "echo -n \"summary+devs\" | nc 192.168.0.203 4028 > /dev/shm/miner203_response.json 2> /dev/null"        
#define M3_PING_RES_PATH      "/dev/shm/miner203_response.json"
#define M3_RESET_TIME         80


// IFTTT Loads API calls. 
// If (Web hooks) then (eWelink)
#define IFTTT_API_KEY        "KEY"

#define L1_ON_URL            "https://maker.ifttt.com/trigger/AntminerS9_202_on/with/key/"
#define L1_OFF_URL           "https://maker.ifttt.com/trigger/AntminerS9_202_off/with/key/"
#define L1_ST1_SYS           M2_ST1_SYS
#define L1_ST2_SYS           M2_ST2_SYS
#define L1_ST3_SYS           M2_ST3_SYS
#define L1_ST4_SYS           M2_ST4_SYS
#define L1_RES_SYS           M2_RES_SYS           
#define L1_BOOT_TIME         M2_BOOT_TIME
#define L1_RESET_TIME        M2_RESET_TIME
#define L1_POWER             -750
#define L1_ST1_POWER         -192    // (W). Frec = 100
#define L1_ST2_POWER         -343    // (W). Frec = 200    
#define L1_ST3_POWER         -499    // (W). Frec = 300
#define L1_ST4_POWER         -660    // (W). Frec = 400

#define L2_ON_URL            "https://maker.ifttt.com/trigger/AntminerS9_201_on/with/key/"
#define L2_OFF_URL           "https://maker.ifttt.com/trigger/AntminerS9_201_off/with/key/"
#define L2_BOOT_TIME         M1_BOOT_TIME
#define L2_RESET_TIME        M1_RESET_TIME
#define L2_POWER             -750
#define L2_ST1_POWER         0
#define L2_ST2_POWER         0
#define L2_ST3_POWER         0
#define L2_ST4_POWER         0

#define L3_ON_URL            "https://maker.ifttt.com/trigger/AntminerS9_203_on/with/key/"
#define L3_OFF_URL           "https://maker.ifttt.com/trigger/AntminerS9_203_off/with/key/"
#define L3_BOOT_TIME         M3_BOOT_TIME
#define L3_RESET_TIME        M3_RESET_TIME
#define L2_POWER             -750
#define L2_ST1_POWER         0
#define L2_ST2_POWER         0
#define L2_ST3_POWER         0
#define L2_ST4_POWER         0 


// Emoncms
#define EMONCMS_SERVER       "http://localhost:PORT/emoncms/input/post"
#define READ_WRITE           "KEY"
