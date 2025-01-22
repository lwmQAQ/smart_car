#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <HardwareSerial.h>
#include <Arduino.h>

#define ENA 13 // PWM 引脚
#define IN1 12
#define IN2 14

#define IN3 27
#define IN4 26
#define ENB 25
#define trigPin 5
#define echoPin 18
#define buzzerPin  4 // 蜂鸣器连接到 D4
const int thresholdDistance = 10; // 设定阈值距离为10cm
// PWM 设置
#define PWM_FREQ 1000 // PWM 频率 (1kHz)
#define PWM_RES 8     // PWM 分辨率 (8位, 0-255)

// PWM 通道
#define PWM_CHANNEL_A 0 // ENA 使用通道 0
#define PWM_CHANNEL_B 1 // ENB 使用通道 1

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
#define MAX_SPEED 255
// 定义服务UUID和特征UUID
#define SERVICE_UUID "00001800-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "f47ac10b-58cc-4372-a567-0e02b2c3d479"

void parseAndControl(std::string data);
void controlMotors(int x, int y);
void stopMotors();
// 解析蓝牙数据并控制小车
void parseAndControl(std::string data)
{
  int x = 0, y = 0;
  sscanf(data.c_str(), "x:%d;y:%d", &x, &y);

  // 控制小车运动
  controlMotors(x, y);
}

// 处理连接事件
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    Serial.println("设备已连接");
  }

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("设备已断开");
  }
};

// 自定义特征回调类
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    // 获取接收到的数据
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0)
    {
      Serial.print("收到数据: ");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
      }
      Serial.println();

      // 解析数据并控制小车
      parseAndControl(rxValue);
    }
  }
};

void controlMotors(int x, int y)
{
  // 如果x和y都为0，停止电机
  if (x == 0 && y == 0)
  {
    stopMotors();
  }
  else
  {
    // 将x和y映射到-1到1的范围
    float x_norm = x / 255.0;
    float y_norm = y / 255.0;

    // 计算斜边长度（即速度）
    float speed = sqrt(x_norm * x_norm + y_norm * y_norm);

    // 计算y轴值与斜边值的比值
    float ratio = y_norm / speed;

    // 根据x的正负决定哪个电机转得快
    if (x > 0)
    {
      // x为正，右电机转得快
      float left_speed = speed * ratio;  // 左轮速度为斜边速度乘以比值
      float right_speed = speed;  // 右轮速度为斜边速度

      // 映射到PWM值
      int left_pwm = left_speed * MAX_SPEED;
      int right_pwm = right_speed * MAX_SPEED;

      // 控制左电机（OUT1 和 OUT2）
      if (y_norm > 0)
      {                          // 前进
        digitalWrite(IN1, HIGH); // OUT1 = HIGH
        digitalWrite(IN2, LOW);  // OUT2 = LOW
        ledcWrite(PWM_CHANNEL_A, abs(left_pwm));
      }
      else if (y_norm < 0)
      {                          // 后退
        digitalWrite(IN1, LOW);  // OUT1 = LOW
        digitalWrite(IN2, HIGH); // OUT2 = HIGH
        ledcWrite(PWM_CHANNEL_A, abs(left_pwm));
      }
      else
      { // 停止
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        ledcWrite(PWM_CHANNEL_A, 0);
      }

      // 控制右电机（OUT3 和 OUT4）
      if (y_norm > 0)
      {                          // 前进
        digitalWrite(IN3, LOW);  // OUT3 = LOW
        digitalWrite(IN4, HIGH); // OUT4 = HIGH
        ledcWrite(PWM_CHANNEL_B, abs(right_pwm));
      }
      else if (y_norm < 0)
      {                          // 后退
        digitalWrite(IN3, HIGH); // OUT3 = HIGH
        digitalWrite(IN4, LOW);  // OUT4 = LOW
        ledcWrite(PWM_CHANNEL_B, abs(right_pwm));
      }
      else
      { // 停止
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        ledcWrite(PWM_CHANNEL_B, 0);
      }
    }
    else if (x < 0)
    {
      // x为负，左电机转得快
      float left_speed = speed;  // 左轮速度为斜边速度
      float right_speed = speed * ratio;  // 右轮速度为斜边速度乘以比值

      // 映射到PWM值
      int left_pwm = left_speed * MAX_SPEED;
      int right_pwm = right_speed * MAX_SPEED;

      // 控制左电机（OUT1 和 OUT2）
      if (y_norm > 0)
      {                          // 前进
        digitalWrite(IN1, HIGH); // OUT1 = HIGH
        digitalWrite(IN2, LOW);  // OUT2 = LOW
        ledcWrite(PWM_CHANNEL_A, abs(left_pwm));
      }
      else if (y_norm < 0)
      {                          // 后退
        digitalWrite(IN1, LOW);  // OUT1 = LOW
        digitalWrite(IN2, HIGH); // OUT2 = HIGH
        ledcWrite(PWM_CHANNEL_A, abs(left_pwm));
      }
      else
      { // 停止
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        ledcWrite(PWM_CHANNEL_A, 0);
      }

      // 控制右电机（OUT3 和 OUT4）
      if (y_norm > 0)
      {                          // 前进
        digitalWrite(IN3, LOW);  // OUT3 = LOW
        digitalWrite(IN4, HIGH); // OUT4 = HIGH
        ledcWrite(PWM_CHANNEL_B, abs(right_pwm));
      }
      else if (y_norm < 0)
      {                          // 后退
        digitalWrite(IN3, HIGH); // OUT3 = HIGH
        digitalWrite(IN4, LOW);  // OUT4 = LOW
        ledcWrite(PWM_CHANNEL_B, abs(right_pwm));
      }
      else
      { // 停止
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        ledcWrite(PWM_CHANNEL_B, 0);
      }
    }
  }
}


void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(PWM_CHANNEL_A, 0); // 停止左电机
  ledcWrite(PWM_CHANNEL_B, 0); // 停止右电机
}
void setup() {
  Serial.begin(115200); // 启动串口通信
  delay(500);           // 确保串口已启动

  // 设置引脚模式
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);  // 关闭蜂鸣器
  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // 配置 PWM
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RES);

  // 绑定 PWM 通道到引脚
  ledcAttachPin(ENA, PWM_CHANNEL_A);
  ledcAttachPin(ENB, PWM_CHANNEL_B);

  pinMode(trigPin, OUTPUT); // 设置 Trig 引脚为输出
  pinMode(echoPin, INPUT);  // 设置 Echo 引脚为输入

  // 初始化 BLE 设备
  BLEDevice::init("ESP32_BLE"); // 初始化 BLE 设备

  // 创建 BLE 服务器
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // 设置连接回调

  // 创建服务
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 创建特征
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_WRITE_NR // 如果需要无响应写入
  );

  pCharacteristic->addDescriptor(new BLE2902()); // 添加描述符
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks()); // 设置特征回调

  // 启动服务
  pService->start();

  // 获取蓝牙广播实例
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  
  // 设置扫描响应，允许广播更多的信息
  pAdvertising->setScanResponse(true);

  // 设置广播的设备名称（通过广告数据来设置）
  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x04);  // 设置广播标志
  advertisementData.setManufacturerData("ESP32_BLE"); // 设置设备名称（自定义内容）



  pAdvertising->setAdvertisementData(advertisementData);

  // 启动广播
  pAdvertising->start();

  Serial.println("等待BLE连接...");
}

void loop() {
  // 发送10微秒的高电平信号触发超声波
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 读取 Echo 引脚的高电平持续时间
  long duration = pulseIn(echoPin, HIGH);

  // 计算距离（声速为340m/s，时间单位为微秒）
  float distance = duration * 0.034 / 2;

  // 如果距离小于阈值，蜂鸣器响
  if (distance < thresholdDistance) {
    digitalWrite(buzzerPin, LOW); // 打开蜂鸣器
    stopMotors();
  } else {
    digitalWrite(buzzerPin, HIGH);  // 关闭蜂鸣器
  }

  delay(100); // 每100ms测量一次
}