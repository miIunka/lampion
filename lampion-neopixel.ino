#include <Adafruit_NeoPixel.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define EFFECT_COUNT 7
#define BUTTON_DEBOUNCE_MS 500
#define PIN_BTN 3
#define PIN_DATA 2

Adafruit_NeoPixel strip(15, PIN_DATA, NEO_GRB + NEO_KHZ800);

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

void strip_fill_rows(RGBColor *columns) {
	for (u8 i = 0; i < 5; i++) {
		strip.setPixelColor(i, columns[i].r, columns[i].g, columns[i].b);
		strip.setPixelColor(9 - i, columns[i].r, columns[i].g, columns[i].b);
		strip.setPixelColor(10 + i, columns[i].r, columns[i].g, columns[i].b);
	}
}

void strip_fill_columns(RGBColor *columns) {
	for (u8 i = 0; i < 3; i++) {
		strip.fill(Adafruit_NeoPixel::Color(columns[i].r, columns[i].g, columns[i].b), i * 5, 5); // 5 LEDs per strip segment
	}
}

u8 effect_id = 0;
u32 button_press_time = 0;

//
// Effect 1 - Fading rainbow
// There are 6 predefined ("target") RGB colors and cross-fade between them.
// One color fills up the whole LED strip.
//

RGBColor effect_1_3_colors[] = {
	{255, 200, 0},
	{255, 0, 0},
	{255, 0, 200},
	{0, 0, 255},
	{0, 255, 255},
	{0, 255, 0}
};

u8 effect_1_3_color_count = sizeof(effect_1_3_colors) / sizeof(RGBColor);

struct {
	RGBColor current;
	RGBColor step;
	u16 steps_remaining;
	u8 color_target;
} effect_1_state = {
	.current = {0, 0, 0},
	.step = {0, 0, 0},
	.steps_remaining = 0,
	.color_target = effect_1_3_color_count - 1
};

void effect_1_update() {
	if (effect_1_state.steps_remaining == 0) {
		effect_1_state.color_target = (effect_1_state.color_target + 1) % effect_1_3_color_count;
		effect_1_state.steps_remaining = 240; // 240 steps = 60 Hz * 4 s
		effect_1_state.step = cross_fade_get_step(effect_1_state.current, effect_1_3_colors[effect_1_state.color_target], effect_1_state.steps_remaining);
		delay(1600);
	}

	color_add(&effect_1_state.current, effect_1_state.step);
	effect_1_state.steps_remaining--;

	strip.fill(Adafruit_NeoPixel::Color(effect_1_state.current.r, effect_1_state.current.g, effect_1_state.current.b));
	delay(16);
}

void setup() {
	Serial.begin(115200);
	digitalWrite(PIN_BTN, HIGH);
	attachInterrupt(digitalPinToInterrupt(PIN_BTN), switch_effect, FALLING);
	
	strip.begin();
	Serial.print("ready");
}

//
// Effect 2 - Cascade
// Loop of 6 colors lit up row by row from top to bottom.
// Each row is lightened up in a different color.
//

struct {
	u8 row;
	u8 color_target;
} effect_2_state = {
	.row = 0,
	.color_target = 0
};


void effect_2_update() {
	RGBColor rows[5];
	
	for (u8 i = 0; i < 5; i++) { // 5 LEDs (rows)
		if (i == effect_2_state.row) {
			rows[i] = effect_1_3_colors[effect_2_state.color_target];
		} else {
			rows[i] = {0, 0, 0}; // black
		}
	}
	
	effect_2_state.color_target = (effect_2_state.color_target + 1) % effect_1_3_color_count;
	effect_2_state.row = (effect_2_state.row + 1) % 5;
	
	strip_fill_rows(rows);
	delay(350);
}

//
// Effect 3 - Blink
// Loop of 6 colors. Each color is lit up for a specified time duration.
// One color fills up the whole LED strip.
//

struct {
	u8 color_target;
} effect_3_state = {
	.color_target = 0 // Starting with the first color in effect_1_3_colors
};

void effect_3_update() {
	RGBColor *col = effect_1_3_colors + effect_3_state.color_target;
	
	strip.fill(Adafruit_NeoPixel::Color(col->r, col->g, col->b));
	strip.show();
	delay(3000);
	strip.fill(0); // black
	strip.show();
	delay(1000);
	
	effect_3_state.color_target = (effect_3_state.color_target + 1) % effect_1_3_color_count;
}

//
// Effect 4 - Spin
// Loop of 6 colors lit by columns (there are 3 strip segments).
// Each column is lighten up in a different color.
//

struct {
	u8 column;
	u8 color_target;
} effect_4_state = {
	.column = 0,
	.color_target = 0
};


void effect_4_update() {
	RGBColor columns[3];
	
	for (u8 i = 0; i < 3; i++) {
		if (i == effect_4_state.column) {
			columns[i] = effect_1_3_colors[effect_4_state.color_target];
		} else {
			columns[i] = {0, 0, 0}; // black
		}
	}
	
	effect_4_state.color_target = (effect_4_state.color_target + 1) % effect_1_3_color_count;
	effect_4_state.column = (effect_4_state.column + 1) % 3;
	
	strip_fill_columns(columns);
	delay(350);
}

//
// Effect 5 - Knight Rider Cascade
// Loop of 6 colors. Each color is lighten up by row from bottom up and down two times
// and than changed to the next color in loop.
//

void loop() {
	switch (effect_id) {
		case 0:
			effect_1_update();
			break;
			
		case 1:
			effect_2_update();
			break;
		
		case 2:
			effect_3_update();
			break;
		
		case 3:
			effect_4_update();
			break;
	}
	
	strip.show();
}

void switch_effect() {
	u32 t = millis();

	if (t - button_press_time > BUTTON_DEBOUNCE_MS){
			effect_id = (effect_id + 1) % EFFECT_COUNT;
			button_press_time = t;
	}
}
