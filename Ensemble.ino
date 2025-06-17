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
// ローカル変数    camelCase        data
// グローバル変数  g_camelCase      g_lastSpread
// 定数          PascalCase        Friction
// 列挙名（enum） PascalCase
// enumの内容    PascalCase
// ************************************************** **************************************************

const String SrcId = "Ensemble";
const String SrcVer = "1.0";

//#define Debug

#include <Midi.h>
#include <M5Unified.h>

#include "ball.h"  // 画像データ

// ************************************************** **************************************************
// Const

constexpr int Radius = 20;

// 物理パラメータ
const float AccelScale = 3.5f;  // 傾きによる加速 2.0〜5.0
const float SpringX = 0.03f;    // 中心へ戻すばねの強さ 0.01〜0.1
const float SpringY = 0.01f;    // 中心へ戻すばねの強さ 0.01〜0.1
const float Friction = 0.90f;   // 摩擦（速度の減衰） 0.85〜0.95

const int Pitch[2][7][7] = {  // Pitch[g_scale][g_spread][osc]
  { { 0, 0, 0, 0, 0, 0, 0 },
    { -5, -5, 0, 0, 0, 4, 4 },
    { -8, -8, -5, 0, 4, 7, 7 },
    { -12, -8, -5, 0, 4, 7, 12 },
    { -17, -12, -8, 0, 7, 12, 16 },
    { -20, -17, -8, 0, 7, 16, 19 },
    { -24, -17, -8, 0, 7, 16, 24 } },
  { { 0, 0, 0, 0, 0, 0, 0 },
    { -5, -5, 0, 0, 0, 3, 3 },
    { -9, -9, -5, 0, 3, 7, 7 },
    { -12, -9, -5, 0, 3, 7, 12 },
    { -17, -12, -9, 0, 7, 12, 15 },
    { -21, -17, -9, 0, 7, 15, 19 },
    { -24, -17, -9, 0, 7, 15, 24 } }
};

const int Fifth[2][7] = {
  { 60, 55, 62, 57, 64, 59, 65 },  // C4, G3, D4, A3, E4, B3, F4
  { 60, 55, 62, 56, 63, 58, 65 }   // C4, G3, D4, Ab3, Eb4, Bb3, F4
};

const char *FifthName[2][7] = {
  { "C", "G", "D", "A", "E", "B", "F" },
  { "Cm", "Gm", "Dm", "Abm", "Ebm", "Bbm", "Fm" }
};

// ************************************************** **************************************************
// Gloval

int g_screen_w, g_screen_h;
float g_x, g_y;            // ボールの位置
float g_vx = 0, g_vy = 0;  // ボールの速度

int g_scale = 0;  // 0:Major, 1:Minor
int g_spread = 0;
int g_lastSpread = 0;
int g_timbre = 0;
int g_lastTimbre = 0;
int g_pos = 0;
int g_note[7] = { 60, 60, 60, 60, 60, 60, 60 };

int g_next = 0;
int g_prev = 0;

int g_n = 0;

MidiBleServer midiServer("M5Stick");

// ************************************************** **************************************************
// dispText
// ************************************************** **************************************************

void dispText() {
  M5.Lcd.fillRect(0, 220, 135, 20, BLACK);
  M5.Lcd.setTextFont(&fonts::FreeMono12pt7b);
  M5.Display.setTextColor(g_scale ? MAGENTA : CYAN, BLACK);
  M5.Display.drawString(FifthName[g_scale][g_pos], 67, 228);
  M5.Display.drawString(FifthName[g_scale][(g_pos + 6) % 7], 20, 228);
  M5.Display.drawString(FifthName[g_scale][(g_pos + 1) % 7], 115, 228);
}

// ************************************************** **************************************************
// setup
// ************************************************** **************************************************

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);

  // Imuセンサーの確認
  if (M5.Imu.getType() == m5::imu_none) {
    M5.Display.println("IMU not found!");
    while (1) delay(100);
  }

  // MIDI準備
  midiServer.begin();
  midiServer.setDefaultSendingChannel(0);

  // 画面初期化
  M5.Display.setRotation(0);
  M5.Display.setSwapBytes(false);
  M5.Display.fillScreen(BLACK);

  M5.Lcd.setTextDatum(middle_center);
  M5.Lcd.setTextSize(1);

  // 描画サイズ
  g_screen_w = M5.Display.width();
  g_screen_h = M5.Display.height() - 20;

  // ボールの初期位置
  g_x = g_screen_w / 2.0f;
  g_y = g_screen_h - 40;

  dispText();
}

// ************************************************** **************************************************
// loop
// ************************************************** **************************************************

void loop() {
  if (M5.Imu.update()) {
    auto data = M5.Imu.getImuData();

    // 前の球を消去
    // M5.Display.fillRect((int)g_x - Radius, (int)g_y - Radius, imgWidth, imgHeight, TFT_BLACK);

    // センサーによる加速度
    float ax = -data.accel.x * AccelScale;
    float ay = data.accel.y * AccelScale;

    // 復元力の向かう位置
    float cx = g_screen_w / 2.0f;  // 中央
    float cy = g_screen_h;         // 手前

    float fx = -(g_x - cx) * SpringX;
    float fy = -(g_y - cy) * SpringY;

    // 合成加速度 → 速度
    g_vx += ax + fx;
    g_vy += ay + fy;

    // 摩擦
    g_vx *= Friction;
    g_vy *= Friction;

    // 前の球を消去
    M5.Display.fillRect((int)g_x - Radius, (int)g_y - Radius, imgWidth, imgHeight, TFT_BLACK);

    // 速度 → 位置
    g_x += g_vx;
    g_y += g_vy;

    // 画面外に出ないように制限
    if (g_x < Radius) {
      g_x = Radius;
      g_vx = 0;
    }
    if (g_x > g_screen_w - Radius) {
      g_x = g_screen_w - Radius;
      g_vx = 0;
    }
    if (g_y < Radius) {
      g_y = Radius;
      g_vy = 0;
    }
    if (g_y > g_screen_h - Radius) {
      g_y = g_screen_h - Radius;
      g_vy = 0;
    }

    // 現在位置に球を描画
    M5.Display.pushImage((int)g_x - Radius, (int)g_y - Radius, imgWidth, imgHeight, img);

    // 横方向の位置 20〜115 → -1,0,1    <20><32>|<32>|<31><20>
    if (g_x < 52) {
      g_timbre = -1;
    } else if (g_x < 84) {
      g_timbre = 0;
    } else {
      g_timbre = 1;
    }

    // 縦方向の位置 20〜200 → 0〜7
    g_spread = (200 - g_y) / 22.5;
    g_spread = (g_spread > 7) ? 7 : g_spread;


    if (g_timbre != g_lastTimbre || g_spread != g_lastSpread) {
      Serial.println("Note off");
      for (int i = 0; i < 7; i++) {
        // Note Off
        midiServer.noteOff(g_note[i], 0, 0);
      }

      g_lastTimbre = g_timbre;
      g_lastSpread = g_spread;

      if (g_spread > 0) {
        Serial.println("Note on timbre:" + String(g_timbre) + " spread:" + String(g_spread));

        int code = Fifth[g_scale][(g_pos + g_timbre + 7) % 7];
        for (int i = 0; i < 7; i++) {
          // Note On
          g_note[i] = Pitch[g_scale][g_spread - 1][i] + code;
          Serial.print(" note" + String(i) + ":" + String(g_note[i]));
          midiServer.noteOn(g_note[i], 100, 0);
        }
        Serial.println();

      }
    }

    // 右に大きく傾けて戻した時
    if (g_next == 0 && ax > 2.5) {
      g_next = 1;
    } else if (g_next == 1 && ax < 1.5) {
      // next code
      g_pos = (g_pos + 1) % 7;
      g_next = 0;

      dispText();
    }

    // 左に大きく傾けて戻した時
    if (g_prev == 0 && ax < -2.5) {
      g_prev = 1;
    } else if (g_prev == 1 && ax > -1.5) {
      // prev code
      g_pos = (g_pos + 6) % 7;
      g_prev = 0;

      dispText();
    }

    // 無音の時、全ての音のNote Offを送信
    if (g_spread == 0) {
      int i = g_n / 6;
      int j, k;
      switch (g_n % 6) {
        case 0:
          j = 3;
          k = g_pos;
          break;
        case 1:
          j = 3;
          k = (g_pos + 1) % 7;
          break;
        case 2:
          j = 3;
          k = (g_pos + 6) % 7;
          break;
        case 3:
          j = 6;
          k = g_pos;
          break;
        case 4:
          j = 6;
          k = (g_pos + 1) % 7;
          break;
        default:
          j = 6;
          k = (g_pos + 6) % 7;
          break;
      }
      midiServer.noteOff(Pitch[g_scale][j][i] + Fifth[g_scale][k], 0, 0);
      if (++g_n >= 42) {
        g_n = 0;
      }
    }
  }

  M5.update();

  // ボタンAを押した時
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button A");

    g_scale = 1 - g_scale;
    dispText();
  }

  /*
  if (M5.BtnB.wasPressed()) {
  }
  */

  // ボタンPowerを押した時
  if (M5.BtnPWR.wasClicked()) {
    Serial.println("Button Power");

    for (int j = 3; j <= 6; j=j+3) {
      for (int i = 0; i < 7; i++) {
        midiServer.noteOff(Pitch[g_scale][j][i] + Fifth[g_scale][g_pos], 0, 0);
        delay(10);
        midiServer.noteOff(Pitch[g_scale][j][i] + Fifth[g_scale][(g_pos+1)%7], 0, 0);
        delay(10);
        midiServer.noteOff(Pitch[g_scale][j][i] + Fifth[g_scale][(g_pos+6)%7], 0, 0);
        delay(10);
      }
    }
    midiServer.allNotesOff(0);
  }
  delay(16);
}

// ************************************************** **************************************************
