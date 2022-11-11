#include <Adafruit_NeoPixel.h>
#include <stdlib.h>


#define u8 uint8_t
#define i8 int8_t
#define u16 uint16_t
#define i16 int16_t
#define u32 uint32_t
#define i32 int32_t
#define EFFECT_COUNT 5
#define BUTTON_DEBOUNCE_MS 500
#define PIN_BTN 3
#define PIN_DATA 2

u8 effect_id = 0;
u32 button_press_time = 0;

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

// returns (a - b) clamped to 0
float subtract_clamped(float a, float b) {
	if (b > a) {
		return 0.0;
	}
	
	return a - b;
}

void strip_shift_and_dim(RGBColor *pixels, u8 count) {
	// going from the top of the segment, take the lower color and dim it randomly
	for (u8 i = count - 1; i; i--) {
		u8 dim_by = 10 + rand() % 75;
		
		pixels[i].r = subtract_clamped(pixels[i - 1].r, dim_by);
		pixels[i].g = subtract_clamped(pixels[i - 1].g, dim_by);
		pixels[i].b = subtract_clamped(pixels[i - 1].b, dim_by);
	}
	
	// i can ignore pixels[0] because it will get painted over anyway
}

void set_random_pixel(RGBColor *pixel, u8 rmin, u8 rmax, u8 gmin, u8 gmax, u8 bmin, u8 bmax) {
	pixel->r = rmin + rand() % (rmax - rmin);
	pixel->g = gmin + rand() % (gmax - gmin);
	pixel->b = bmin + rand() % (bmax - bmin);
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

void strip_fill_copy(RGBColor *pixels, u8 from = 0, u8 count = 15, bool reverse = false) {
	for (u8 i = 0; i < count; i++) {
		u8 src_i = (reverse)? (count - i - 1) : i;
		strip.setPixelColor(from + i, pixels[src_i].r, pixels[src_i].g, pixels[src_i].b);
	}
}

//
// Effect 1 - Rainbow
// There are 6 predefined ("target") RGB colors and cross-fade between them.
// One color fills up the whole LED strip.
//

RGBColor effect_colors[] = {
	{255, 200, 0},
	{255, 0, 0},
	{255, 0, 200},
	{0, 0, 255},
	{0, 255, 255},
	{0, 255, 0}
};

u8 effect_color_count = sizeof(effect_colors) / sizeof(RGBColor);

struct {
	RGBColor current;
	RGBColor step;
	u16 steps_remaining;
	u8 color_target;
} effect_rainbow_state = {
	.current = {0, 0, 0},
	.step = {0, 0, 0},
	.steps_remaining = 0,
	.color_target = effect_color_count - 1
};

void effect_rainbow_update() {
	if (effect_rainbow_state.steps_remaining == 0) {
		effect_rainbow_state.color_target = (effect_rainbow_state.color_target + 1) % effect_color_count;
		effect_rainbow_state.steps_remaining = 240; // 240 steps = 60 Hz * 4 s
		effect_rainbow_state.step = cross_fade_get_step(effect_rainbow_state.current, effect_colors[effect_rainbow_state.color_target], effect_rainbow_state.steps_remaining);
		delay(1600);
	}

	color_add(&effect_rainbow_state.current, effect_rainbow_state.step);
	effect_rainbow_state.steps_remaining--;

	strip.fill(Adafruit_NeoPixel::Color(effect_rainbow_state.current.r, effect_rainbow_state.current.g, effect_rainbow_state.current.b));
	delay(16);
}

void setup() {
	Serial.begin(115200);
	digitalWrite(PIN_BTN, HIGH);
	attachInterrupt(digitalPinToInterrupt(PIN_BTN), switch_effect, FALLING);
	
	strip.begin();
	Serial.println("ready :)");
}

//
// Effect 2 - Cascade
// Loop of 6 colors lit up row by row from top to bottom.
// Each row is lightened up in a different color.
//

struct {
	u8 row;
	u8 color_target;
} effect_cascade_state = {
	.row = 0,
	.color_target = 0
};


void effect_cascade_update() {
	RGBColor rows[5];
	
	for (u8 i = 0; i < 5; i++) { // 5 LEDs (rows)
		if (i == effect_cascade_state.row) {
			rows[i] = effect_colors[effect_cascade_state.color_target];
		} else {
			rows[i] = {0, 0, 0}; // black
		}
	}
	
	effect_cascade_state.color_target = (effect_cascade_state.color_target + 1) % effect_color_count;
	effect_cascade_state.row = (effect_cascade_state.row + 1) % 5;
	
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
} effect_blink_state = {
	.color_target = 0 // Starting with the first color in effect_colors
};

void effect_blink_update() {
	RGBColor *col = effect_colors + effect_blink_state.color_target;
	
	strip.fill(Adafruit_NeoPixel::Color(col->r, col->g, col->b));
	strip.show();
	delay(3000);
	strip.fill(0); // black
	strip.show();
	delay(1000);
	
	effect_blink_state.color_target = (effect_blink_state.color_target + 1) % effect_color_count;
}

//
// Effect 4 - Spin
// Loop of 6 colors lit by columns (there are 3 strip segments).
// Each column is lighten up in a different color.
//

struct {
	u8 column;
	u8 color_target;
} effect_spin_state = {
	.column = 0,
	.color_target = 0
};


void effect_spin_update() {
	RGBColor columns[3];
	
	for (u8 i = 0; i < 3; i++) {
		if (i == effect_spin_state.column) {
			columns[i] = effect_colors[effect_spin_state.color_target];
		} else {
			columns[i] = {0, 0, 0}; // black
		}
	}
	
	effect_spin_state.color_target = (effect_spin_state.color_target + 1) % effect_color_count;
	effect_spin_state.column = (effect_spin_state.column + 1) % 3;
	
	strip_fill_columns(columns);
	delay(350);
}

//
// Effect 5 - Fire
// Random orangish colors are injected to the bottom of each segment and are propagated
// upwards while fading. Independently, brightness of the whole strip is varied smoothly.
//

struct {
	RGBColor seg_A[5]; //
	RGBColor seg_B[5]; // In all segments [0] is the bottom.
	RGBColor seg_C[5]; //
	u16 color_frames_remaining;      // one frame == approx 16 ms
	u16 brightness_frames_remaining; //
	i16 brightness_delta;
	i16 brightness;
} effect_fire_state; // all initialized to 0

void effect_fire_update() {
	// if no more color frames remain
	if (effect_fire_state.color_frames_remaining == 0) {
		// shift segments one row up while dimming each color a bit
		strip_shift_and_dim(effect_fire_state.seg_A, 5);
		strip_shift_and_dim(effect_fire_state.seg_B, 5);
		strip_shift_and_dim(effect_fire_state.seg_C, 5);
		
		// inject a new bright pixel to the bottom of each segment
		set_random_pixel(effect_fire_state.seg_A, 200, 255, 100, 180, 0, 20);
		set_random_pixel(effect_fire_state.seg_B, 200, 255, 100, 180, 0, 20);
		set_random_pixel(effect_fire_state.seg_C, 200, 255, 100, 180, 0, 20);
		
		// copy the bitmap into the strip
		strip_fill_copy(effect_fire_state.seg_A, 0, 5, true);
		strip_fill_copy(effect_fire_state.seg_B, 5, 5);
		strip_fill_copy(effect_fire_state.seg_C, 10, 5, true);
		
		// and finally determine when to do the next shift
		effect_fire_state.color_frames_remaining = 10 + rand() % 5;
	}
		
	// if no more brightness frames remain
	if (effect_fire_state.brightness_frames_remaining == 0) {
		// pick a new brightness
		i16 new_brightness = 40 + rand() % (256 - 40);
		
		// pick a new ramp duration
		effect_fire_state.brightness_frames_remaining = 10 + rand() % 20;
		
		// calculate the delta (step)
		effect_fire_state.brightness_delta = (new_brightness - effect_fire_state.brightness) / (i16)effect_fire_state.brightness_frames_remaining;
	}
	
	// apply the brightness interpolation step
	effect_fire_state.brightness += effect_fire_state.brightness_delta;
	strip.setBrightness(effect_fire_state.brightness);
	
	// and finally hold long enough to maintain roughly 60 FPS
	delay(10);
	
	effect_fire_state.color_frames_remaining--;
	effect_fire_state.brightness_frames_remaining--;
}

void loop() {
	switch (effect_id) {
		case 0:
			effect_fire_update();
			break;
			
		case 1:
			effect_rainbow_update();
			break;
		
		case 2:
			effect_cascade_update();
			break;
		
		case 3:
			effect_blink_update();
			break;
			
		case 4:
			effect_spin_update();
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
