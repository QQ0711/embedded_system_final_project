#include <MFRC522.h>      //RC522
#include <U8g2lib.h>      //OLED
#include <Keypad.h>       //Keypad
#include <ESP32Servo.h>   //使用tone()來設定蜂鳴器、使用伺服馬達

#define SDA_PIN  5               //RC522 的 SDA 腳位
#define RST_PIN 27               //RC522 的 RST 腳位
#define buzzer_PIN 14            //蜂鳴器的腳位
#define photo_interrupt_PIN 26   //光遮斷器的腳位
#define ROW_NUM 4                //row的數量為4
#define COLUMN_NUM 3             //column的數量為3

MFRC522 rfid(SDA_PIN, RST_PIN);  //建立並初始化 MFRC522 物件

Servo myServo; //建立一個伺服馬達物件

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

int angle = 0;       //紀錄伺服馬達的角度
int sensorRead = 0;  //光遮斷器讀入的值
int in_counter = 0;  //投入錢幣的數量
int out_counter = 0; //取出錢幣的數量
int out_money = 0;   //匯出的金額

//數字鍵盤數字的設定
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte pin_rows[ROW_NUM] = {17, 16, 4, 0};   //數字鍵盤控制row的腳位
byte pin_column[COLUMN_NUM] = {2, 15, 13}; //數字鍵盤控制column的腳位

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password[3] = {"1111","2222","3333"};        //3個使用者的密碼
const String user_name[3] = {"apple","banana","orange"};  //3個使用者的名字
int balance[3] = {100, 200, 300};  //3個使用者的帳戶餘額
String input_password = "";        //儲存輸入的密碼  

//3個使用者的UID
const byte user_UID[3][4] = {{0xD5, 0XF7, 0X1C, 0X48},{0xF5, 0x3C, 0xE5, 0x41},{0x45, 0x5D, 0xC5, 0x47}};
//柯：0xD5, 0XF7, 0X1C, 0X48
//商：0xF5, 0x3C, 0xE5, 0x41
//蕭：0x45, 0x5D, 0xC5, 0x47

int ID = -1; //目前是哪一個使用者


//按'#'開始服務
void start() {
  u8g2.clearBuffer();
  delay(50);
  u8g2.drawStr(2, 10, "Please press '#' to start.");
  u8g2.sendBuffer();
  while(1){
    char key = keypad.getKey();
    if (key=='#') {
      do {
        ID = check_user_UID();
      }while (ID == -1);
      break;
    }
  }
}

//檢查密碼是否正確
bool check_password()
{
  if(ID == -1) return false;
  u8g2.clearBuffer();
  delay(10);
  u8g2.drawStr(2, 10, "Enter password:");
  u8g2.sendBuffer();
  input_password = "";
  int index = 90;
  while(1){
    char key = keypad.getKey(); //取得按鍵的值
    if (key) 
    {
      u8g2.drawStr(index, 10, "*");
      u8g2.sendBuffer();
      index += 5;
      if (key == '*') 
      {
        u8g2.clearBuffer();
        delay(50);
        u8g2.drawStr(2, 10, "Enter password:");
        u8g2.sendBuffer();
        index = 90;
        input_password = "";  //清空輸入的密碼
      } 
      else if (key == '#') 
      {
        if (password[ID] == input_password) 
        {
          u8g2.clearBuffer();
          delay(50);
          u8g2.drawStr(2, 10, "ACCESS GRANTED !");  //密碼正確
          u8g2.sendBuffer();
          delay(2000);
          return true;
  
        } else {
          u8g2.clearBuffer();
          delay(50);
          u8g2.drawStr(2, 10, "PASSWORD INCORRECT !");  //密碼錯誤
          u8g2.drawStr(2, 20, "WAIT...");
          u8g2.sendBuffer();
          delay(2000);
        }
        u8g2.clearBuffer();
        u8g2.drawStr(2, 10, "Enter password:");
        u8g2.sendBuffer();
        index = 90;
        input_password = "";   //清空輸入的密碼
      } 
      else 
      {
        input_password += key; //新增輸入的字元到字串
      }
    }
  }
  return false;
}

//檢查卡片的UID是否有被紀錄
int check_user_UID()
{
  u8g2.clearBuffer();
  delay(10);
  u8g2.drawStr(2, 10, "Please put your card.");
  u8g2.sendBuffer();
  if (rfid.PICC_IsNewCardPresent()) { //是否感應到新的卡片
    if (rfid.PICC_ReadCardSerial()) { //讀取卡片的資料
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      
      //以十六進位輸出UID
      Serial.print("UID:");
      for (int i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
      }
      Serial.println();

      //確認卡片的UID是否有被紀錄，並回傳使用者的ID
      if (rfid.uid.uidByte[0] == user_UID[0][0] &&
          rfid.uid.uidByte[1] == user_UID[0][1] &&
          rfid.uid.uidByte[2] == user_UID[0][2] &&
          rfid.uid.uidByte[3] == user_UID[0][3] ) {
          return 0; //柯的ID
      } else if (rfid.uid.uidByte[0] == user_UID[1][0] &&
                 rfid.uid.uidByte[1] == user_UID[1][1] &&
                 rfid.uid.uidByte[2] == user_UID[1][2] &&
                 rfid.uid.uidByte[3] == user_UID[1][3] ) {
          return 1; //商的ID
      } else if (rfid.uid.uidByte[0] == user_UID[2][0] &&
                 rfid.uid.uidByte[1] == user_UID[2][1] &&
                 rfid.uid.uidByte[2] == user_UID[2][2] &&
                 rfid.uid.uidByte[3] == user_UID[2][3] ) {
          return 2; //蕭的ID
      }else {
          u8g2.clearBuffer();
          delay(50);
          u8g2.drawStr(2, 10, "Invalid card !");
          u8g2.drawStr(2, 20, "WAIT...");
          u8g2.sendBuffer();
          delay(2000);
          return -1;   
      }
      
      rfid.PICC_HaltA();      // 讓卡片進入停止模式
      rfid.PCD_StopCrypto1(); // 停止 PCD 上的加密 
    }
  }
  return -1;
}

//顯示餘額、交易選項
void show_basic_info(bool state)
{
  if(state){
    String msg1 = "name: " + user_name[ID];
    String msg2 = "balance: " + (String)balance[ID];
    String msg3 = "1: deposit";  //存款
    String msg4 = "2: withdraw"; //提款
    String msg5 = "3: transfer"; //轉帳
    u8g2.clearBuffer();
    delay(50);
    u8g2.drawStr(5, 10, msg1.c_str());
    u8g2.drawStr(5, 22, msg2.c_str());
    u8g2.drawStr(5, 34, msg3.c_str());
    u8g2.drawStr(5, 46, msg4.c_str());
    u8g2.drawStr(5, 58, msg5.c_str());
    u8g2.sendBuffer();
    select_service(); //選取要進行的交易
  }
}

//選取要進行的交易
void select_service(){
  while(1){
    char key = keypad.getKey(); //取得按鍵的值
    //如果有按按鍵
    if(key){
      if (key == '1') {      //存款
        deposit_money();
        break;
      }else if(key == '2'){  //提款
        withdraw_money();
        break;
      }else if(key == '3'){  //匯款
        transfer_money();
        break;
      }else{                 //如果按到沒有提供服務的編號，就會顯示錯誤訊息
         u8g2.clearBuffer();
         delay(10);
         u8g2.drawStr(2, 10, "Service not provide.");
         u8g2.drawStr(2, 25, "Please select again.");
         u8g2.sendBuffer();
         delay(5000);
         show_basic_info(true); //顯示餘額、交易選項，並再次選取要進行的交易
      } 
    }
  }
}

//判斷投入錢幣的數量
void photo_interrupt(){
  sensorRead = digitalRead(photo_interrupt_PIN); //讀取光遮斷器訊號並存入變數 sensorRead
  if(sensorRead == 1) {                          //當光被遮斷時，訊號為 "1"
    Serial.println("1");                         //在序列埠監視器印出 "1"
    in_counter++;
    tone(buzzer_PIN, 500, 1000);                 //蜂鳴器叫一聲，頻率 500 Hz，響聲長度為1秒
  }  
  else {                  //當光沒有被遮斷時，訊號為 "0"
    Serial.println("0");  //在序列埠監視器印出 "0"
    noTone(buzzer_PIN);   //蜂鳴器不叫
  }
  delay(10);
}

//存款
void deposit_money(){
  u8g2.clearBuffer();
  delay(50);
  u8g2.drawStr(2, 10, "Please insert coins...");
  u8g2.sendBuffer();
  in_counter = 0;
  String msg1 = "";
  String msg2 = "";
  while(1){
    char key = keypad.getKey(); //取得按鍵的值
    
    //完成投入錢幣的動作後，按'#'來結束存款
    if (key=='#') {
      u8g2.clearBuffer();
      balance[ID] += 10*in_counter;                    //改變帳戶餘額
      msg1 = "inserted amount: " + (String)in_counter; //顯示投入的錢幣數量
      msg2 = "your balance: " + (String)balance[ID];   //顯示存款後的餘額
      delay(500);
      u8g2.drawStr(2, 10, msg1.c_str());
      u8g2.drawStr(2, 20, msg2.c_str());
      u8g2.sendBuffer();
      break;
    }else{
      photo_interrupt();  //計算投入錢幣的數量
      u8g2.drawStr(2, 30, "Press '#' to finish."); 
      u8g2.sendBuffer();
    }
  }
  continue_transaction(); //判斷是否繼續交易
}

//輸入要提領或匯出的金額
int input_amount(){
  u8g2.clearBuffer();
  delay(50);
  u8g2.drawStr(2, 10, "Enter amount: ");
  u8g2.sendBuffer();
  String money = "";  //輸入的金額字串
  int value = 0;
  String msg1 = "";
  String msg2 = "";
  int index = 85; //輸入字元要在OLED上顯示的初始位置
  while(1){
    char key = keypad.getKey(); //取得案件的值
    msg2 = "";
    if (key) 
    {
      msg2 = key;
      u8g2.drawStr(index, 10, msg2.c_str());
      u8g2.sendBuffer();
      index += 7; //下一個輸入字元要顯示的位置
      Serial.print(key);
      if (key == '*')
      {
        money = "";        // 清空輸入的金額字串
        //再次輸入金額
        u8g2.clearBuffer();
        delay(50);
        u8g2.drawStr(2, 10, "Enter amount: ");
        u8g2.sendBuffer();
        index = 85;
      }
      else if (key == '#'){          //按'#'結束輸入金額的動作
        value = atoi(money.c_str()); //將金額字串轉成整數 
        if(value%10 == 0){           //輸入的金額必須是10的倍數才能進行提款或匯款
          if(value > balance[ID]){   //輸入的金額超過帳戶餘額
            u8g2.drawStr(2, 20, "Exceed balance amount!");
            u8g2.sendBuffer();
            index = 85; //初始化輸入字元要在OLED上顯示的初始位置
            delay(5000);
          }
          else{ //輸出要提領或匯出的金額
            msg1 = "the amount: " + (String)money; 
            u8g2.clearBuffer();
            delay(50);
            u8g2.drawStr(2, 10, msg1.c_str());
            u8g2.sendBuffer();
            return value;
          }
        }
        else{ //如果不是10的倍數，就會出現錯誤訊息
          delay(50);
          u8g2.drawStr(2, 20, "Not a multiple of 10 !");
          u8g2.sendBuffer();
          delay(5000);
        }
        u8g2.clearBuffer();
        delay(50);
        u8g2.drawStr(2, 10, "Enter amount: ");
        u8g2.sendBuffer();
        money = "";
        index = 85;
      }
      else money += key; //新增輸入的字元到金額字串中
    }
  }
  return 0;
}

//提款
void withdraw_money(){
  String msg1 = "";
  out_counter = input_amount()/10; //將取得的金額換算成錢幣數量
  //以錢幣數量作為打擊次數，進行將錢幣打出的動作
  for(int i = 0; i < out_counter; i++){
    if(angle == 20){
      angle = 160;
      myServo.write(angle);
      balance[ID] -= 10;   //每打一次就扣10元 
      delay(2000);
    }else{
      angle = 20;
      myServo.write(angle);
      balance[ID] -= 10; 
      delay(2000);
    }
  }
  msg1 = "your balance: " + (String)balance[ID]; 
  u8g2.drawStr(2, 20, msg1.c_str());  //顯示提款後的餘額
  u8g2.sendBuffer();
  continue_transaction();
}

//選擇要匯款的對象
int select_receicer(){
  u8g2.clearBuffer();
  delay(50);
  u8g2.drawStr(2, 10, "receiver ID: ");
  u8g2.sendBuffer();
  while(1){
    char key = keypad.getKey();
    if(key){
      u8g2.drawStr(70, 10, ((String)(key%48)).c_str()); //顯示輸入的ID
      u8g2.sendBuffer();
      
      
      if(48 <= key && key <= 50){ //如果輸入的ID存在，就回傳ID
        delay(2000);
        return key%48;
      }else{                      //如果輸入的ID不存在，就會輸出錯誤訊息
        u8g2.drawStr(2, 30, "Invaild ID!");
        u8g2.drawStr(2, 40, "Please enter again.");
        u8g2.sendBuffer();
        delay(5000);
        u8g2.clearBuffer();
        delay(50);
        u8g2.drawStr(2, 10, "receiver ID: ");
        u8g2.sendBuffer();
      }
    }
  }
  return -1;
}

//匯款
void transfer_money(){
  out_money = input_amount();               //取得要匯出的金額
  balance[ID] -= out_money;                 //根據匯出金額對匯出者扣款
  balance[select_receicer()] += out_money;  //根據匯出金額對接收者存款
  String msg1 = "your balance: " + (String)balance[ID];  
  delay(10);
  u8g2.drawStr(2, 20, msg1.c_str());  //顯示匯款後的餘額//顯示匯款後的餘額
  u8g2.sendBuffer();
  continue_transaction();
}

//判斷是否要繼續交易
void continue_transaction(){
  u8g2.drawStr(2, 35, "Service is end.");
  u8g2.drawStr(2, 48, "To continue ?");
  u8g2.drawStr(2, 61, "1: YES / others: NO");
  u8g2.sendBuffer();
  while(1){
    char key = keypad.getKey();
    if(key){
      if(key == '1'){
        show_basic_info(true); //顯示餘額、交易選項，並再次選取要進行的交易
        break;
      }else break;
    }
  }   
}

void setup() {
  Serial.begin(115200);
  Serial.println("START!");
  
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  //OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(5, 10, "Welcome !");
  u8g2.sendBuffer();
  delay(3000);
  
  pinMode(photo_interrupt_PIN, INPUT); //esp32 從 pin 26 讀入光遮斷器的訊號

  myServo.attach(32, 500, 2400);
  
}

void loop() {
  start(); //按'#'開始服務
  show_basic_info(check_password()); ＋
}
