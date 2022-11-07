#include <Adafruit_NeoPixel.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define EFFECT_COUNT 7
#define BUTTON_DEBOUNCE_MS 500

typedef struct {
  float r;
  float g;
  float b;
} RGBColor;

RGBColor cross_fade_get_step(RGBColor c_from, RGBColor c_to, u16 steps) {
  RGBColor step;
  
  step.r = (c_to.r - c_from.r) / steps;
  step.g = (c_to.g - c_from.g) / steps;
  step.b = (c_to.b - c_from.b) / steps;

  return step;
}

void color_add(RGBColor *col, RGBColor step) {
  col->r += step.r;
  col->g += step.g;
  col->b += step.b;
}

Adafruit_NeoPixel strip(15, 2, NEO_GRB + NEO_KHZ800);

u8 effect_id = 0;
u32 button_press_time = 0;

//
// Effect 1 data
//

RGBColor effect_1_colors[] = {
  {255, 200, 0},
  {255, 0, 0},
  {255, 0, 200},
  {0, 0, 255},
  {0, 255, 255},
  {0, 255, 0}
};

u8 effect_1_color_count = sizeof(effect_1_colors) / sizeof(RGBColor);

struct {
  RGBColor current;
  RGBColor step;
  u16 steps_remaining;
  u8 color_target;
} effect_1_state = {
  .current = {0, 0, 0},
  .step = {0, 0, 0},
  .steps_remaining = 0,
  .color_target = effect_1_color_count - 1
};

void effect_1_update() {
  if (effect_1_state.steps_remaining == 0) {
    effect_1_state.color_target = (effect_1_state.color_target + 1) % effect_1_color_count;
    effect_1_state.steps_remaining = 240; // 240 steps = 60 Hz * 4 s
    effect_1_state.step = cross_fade_get_step(effect_1_state.current, effect_1_colors[effect_1_state.color_target], effect_1_state.steps_remaining);
    delay(1600);
  }

  color_add(&effect_1_state.current, effect_1_state.step);
  effect_1_state.steps_remaining--;

  strip.fill(Adafruit_NeoPixel::Color(effect_1_state.current.r, effect_1_state.current.g, effect_1_state.current.b));
  delay(16);
}

void setup() {
  Serial.begin(115200);
  digitalWrite(3, HIGH);
  attachInterrupt(digitalPinToInterrupt(3), switch_effect, FALLING);
  
  strip.begin();
  Serial.print("ready");

}

void loop() {
  if (effect_id == 0) {
    effect_1_update();
  }
  
 // strip.setPixelColor(0, 255, 0, 0);

  strip.show();
  
}

void switch_effect() {
  u32 t = millis();

  if (t - button_press_time > BUTTON_DEBOUNCE_MS){
      effect_id = (effect_id + 1) % EFFECT_COUNT;
      button_press_time = t;
  }
    

}
