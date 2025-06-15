// ************************************************** **************************************************
// Ensemble v1.0
//
// 
// ************************************************** **************************************************
// <Hardware>
// M5StickC PLUS2
// ************************************************** **************************************************
//
// ************************************************** **************************************************
// ローカル変数    camelCase        dispPage
// グローバル変数  g_camelCase      g_dispPage
// 定数          UPPER_CASE        MAX_PAGE
// 列挙名（enum） PascalCase        PageType
// enumの内容    PascalCase        PagePlay
// ************************************************** **************************************************

const String SrcId = "Ensemble";
const String SrcVer = "1.0";

//#define Debug

#include <M5Unified.h>

#include "ball.h"  // 画像データ

// ************************************************** **************************************************
// Const

constexpr int RADIUS = 10;

// 物理パラメータ
const float ACCEL_SCALE = 3.5f;  // 傾きによる加速 2.0〜5.0
const float SPRING_X = 0.03f;   // 中心へ戻すばねの強さ 0.01〜0.1
const float SPRING_Y = 0.01f;   // 中心へ戻すばねの強さ 0.01〜0.1
const float FRICTION = 0.90f;    // 摩擦（速度の減衰） 0.85〜0.95

// ************************************************** **************************************************
// Gloval

int g_screen_w, g_screen_h;
float g_x, g_y;            // ボールの位置
float g_vx = 0, g_vy = 0;  // ボールの速度

// ************************************************** **************************************************
// setup
// ************************************************** **************************************************

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // 傾きセンサーの確認
  if (M5.Imu.getType() == m5::imu_none) {
    M5.Display.println("IMU not found!");
    while (1) delay(100);
  }

  M5.Display.setRotation(0);
  M5.Display.setSwapBytes(false);
  M5.Display.fillScreen(TFT_BLACK);

  M5.Lcd.setTextDatum(middle_center);
  M5.Lcd.setTextSize(1);

  g_screen_w = M5.Display.width();
  g_screen_h = M5.Display.height() - 20;

  g_x = g_screen_w / 2.0f;
  g_y = g_screen_h / 2.0f;

  M5.Lcd.setTextFont(&fonts::FreeMono12pt7b);
  M5.Display.drawString("GCE", 67, 228);
  M5.Lcd.setTextFont(&fonts::FreeMono9pt7b);
  M5.Display.drawString("EAC", 20, 229);
  M5.Display.drawString("BEG", 115, 229);
}

// ************************************************** **************************************************
// loop
// ************************************************** **************************************************

void loop() {
  if (M5.Imu.update()) {
    auto data = M5.Imu.getImuData();

    // 前の円を消去
    M5.Display.fillRect((int)g_x - 9, (int)g_y - 9, 18, 18, TFT_BLACK);

    // センサーによる加速度
    float ax = -data.accel.x * ACCEL_SCALE;
    float ay = data.accel.y * ACCEL_SCALE;

    // 中心への復元力
    float cx = g_screen_w / 2.0f;
    float cy = g_screen_h;
    //float cy = g_screen_h / 2.0f;
    float fx = -(g_x - cx) * SPRING_X;
    float fy = -(g_y - cy) * SPRING_Y;

    // 合成加速度 → 速度
    g_vx += ax + fx;
    g_vy += ay + fy;

    // 摩擦
    g_vx *= FRICTION;
    g_vy *= FRICTION;

    // 速度 → 位置
    g_x += g_vx;
    g_y += g_vy;

    // 画面外に出ないよう制限
    if (g_x < RADIUS) {
      g_x = RADIUS;
      g_vx = 0;
    }
    if (g_x > g_screen_w - RADIUS) {
      g_x = g_screen_w - RADIUS;
      g_vx = 0;
    }
    if (g_y < RADIUS) {
      g_y = RADIUS;
      g_vy = 0;
    }
    if (g_y > g_screen_h - RADIUS) {
      g_y = g_screen_h - RADIUS;
      g_vy = 0;
    }

    // 現在位置に円を描画
    M5.Display.pushImage((int)g_x - 9, (int)g_y - 9, imgWidth, imgHeight, img);
  }

  delay(16);  // 約60FPS
}

// ************************************************** **************************************************
