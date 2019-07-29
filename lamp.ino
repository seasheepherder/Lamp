#include <NeoPixelAnimator.h>
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelBus.h>

#include <Ticker.h>
Ticker ticker;

uint16_t PressedTimer = 0;
uint16_t RepeatTimer = 0;
boolean KeyPressedFlag = false;
boolean KeyValidFlag = false;

#define TIME_30MS         30
#define TIME_200MS        200
#define TIME_1S           1000
#define TIME_5S           5000
#define TIME_KEY_REPEAT   1500

const int buttonPin = 0;     // the number of the pushbutton pin
// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
char LampMode = 0;
char ColorMode = 0;

const uint16_t PixelCount = 12; // make sure to set this to the number of pixels in your strip
const uint16_t PixelPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
const uint16_t AnimCount = 12; //
const uint16_t TailLength = 11; // length of the tail, must be shorter than PixelCount
const float MaxLightness = 0.2f; // max lightness at the head of the tail (0.5f is full bright)
uint8_t delta = 255 * MaxLightness;

boolean AnimationInit = false;
boolean LampOff = false;

HslColor hslcolor;
RgbColor rgbcolor;

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma
NeoPixelBus<NeoGrbFeature, NeoEsp8266AsyncUart1Ws2812Method> strip(PixelCount, PixelPin); // for esp8266 omit the pin

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

// what is stored for state is specific to the need, in this case, the colors.
// Basically what ever you need inside the animation update function
struct MyAnimationState
{
  RgbColor StartingColor;
  RgbColor EndingColor;
};

// one entry per pixel to match the animation timing manager
MyAnimationState animationState[PixelCount];
boolean fadeToColor = true;  // general purpose variable used to store effect state
const int led = 13;

void Keytimer() {
  PressedTimer++;
  RepeatTimer++;
}

void SetRandomSeed()
{
  uint32_t seed;

  // random works best with a seed that can use 31 bits
  // analogRead on a unconnected pin tends toward less than four bits
  seed = analogRead(0);
  delay(1);

  for (int shifts = 3; shifts < 31; shifts += 3)
  {
    seed ^= analogRead(0) << shifts;
    delay(1);
  }

  // Serial.println(seed);
  randomSeed(seed);
}

void LoopAnimUpdate(const AnimationParam& param)
{
  // wait for this animation to complete,
  // we are using it as a timer of sorts
  if (param.state == AnimationState_Completed)
  {
    // done, time to restart this position tracking animation/timer
    animations.RestartAnimation(param.index);

    // rotate the complete strip one pixel to the right on every update
    strip.RotateRight(1);
  }
}

// simple blend function
void BlendAnimUpdate(const AnimationParam& param)
{
  // this gets called for each animation on every time step
  // progress will start at 0.0 and end at 1.0
  // we use the blend function on the RgbColor to mix
  // color based on the progress given to us in the animation
  RgbColor updatedColor = RgbColor::LinearBlend(
                            animationState[param.index].StartingColor,
                            animationState[param.index].EndingColor,
                            param.progress);

  if ( LampMode == 3) {
    // apply the color to the strip
    strip.SetPixelColor(param.index, updatedColor);
  }
  else {
    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
      strip.SetPixelColor(pixel, updatedColor);
  }
}

void DrawTailPixels()
{
  // using Hsl as it makes it easy to pick from similiar saturated colors
  hslcolor = HslColor(rgbcolor);

  for (uint16_t index = 0; index < strip.PixelCount() ; index++)
  {
    float lightness = 0;

    if ( index <= TailLength )
      lightness = index * MaxLightness / TailLength;

    RgbColor color = HslColor(hslcolor.H, hslcolor.S, lightness);
    strip.SetPixelColor(index, colorGamma.Correct(color));
  }
}

void FadeInFadeOutRinseRepeat(float luminance)
{
  if (fadeToColor)
  {
    // Fade upto a random color
    // we use HslColor object as it allows us to easily pick a hue
    // with the same saturation and luminance so the colors picked
    // will have similiar overall brightness
    RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
    uint16_t time = random(1000, 2000);

    animationState[0].StartingColor = strip.GetPixelColor(0);
    animationState[0].EndingColor = target;

    animations.StartAnimation(0, time, BlendAnimUpdate);
  }
  else
  {
    // fade to black
    uint16_t time = random(600, 700);

    animationState[0].StartingColor = strip.GetPixelColor(0);
    animationState[0].EndingColor = RgbColor(0);

    animations.StartAnimation(0, time, BlendAnimUpdate);
  }

  // toggle to the next effect state
  fadeToColor = !fadeToColor;
}
void PickRandom(float luminance)
{
  // pick random count of pixels to animate
  uint16_t count = random(PixelCount);
  while (count > 0)
  {
    // pick a random pixel
    uint16_t pixel = random(PixelCount);

    // pick random time and random color
    // we use HslColor object as it allows us to easily pick a color
    // with the same saturation and luminance
    uint16_t time = random(100, 400);
    animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
    animationState[pixel].EndingColor = HslColor(random(360) / 360.0f, 1.0f, luminance);

    animations.StartAnimation(pixel, time, BlendAnimUpdate);

    count--;
  }
}

void setup()
{
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);

  strip.Begin();
  strip.Show();
  SetRandomSeed();

  ticker.attach_ms(1, Keytimer);
}

void loop()
{
  // this is all that is needed to keep it running
  // and avoiding using delay() is always a good thing for
  // any timing related routines

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    if (KeyPressedFlag == false && KeyValidFlag == false) {
      PressedTimer = 0;
      RepeatTimer = 0;
      KeyPressedFlag = true;
      ticker.attach_ms(1, Keytimer);
      printf("KeyPressedFlag\r\n");
    }

    if (KeyPressedFlag == true) {
      if (PressedTimer >= TIME_1S && RepeatTimer >= TIME_KEY_REPEAT) {
        LampMode++;
        LampMode = LampMode % 4;
        AnimationInit = false;
        RepeatTimer = 0;
        printf("TIME_1S\r\n");
        printf("LampMode = %d\r\n", LampMode);
      }
    }
  }
  else {
    if ( KeyPressedFlag == true ) {
      KeyPressedFlag = false;
      KeyValidFlag = true;

      if (PressedTimer >= TIME_1S && PressedTimer < TIME_5S && RepeatTimer <= TIME_KEY_REPEAT) {
        KeyValidFlag = false;
        printf("RepeatTimer = %d\r\n", RepeatTimer);
      }
      
      printf("PressedTimer = %d\r\n", PressedTimer);
      ticker.detach();
    }
  }

  if (KeyValidFlag == true) {
    
    if (PressedTimer >= TIME_5S) {
      
      if ( LampOff == false ) {
        LampOff = true;
        rgbcolor = RgbColor(0, 0, 0);
        for (char i = 0; i < PixelCount; i++)
          strip.SetPixelColor(i, rgbcolor);
        strip.Show();
      }
      else {
        LampOff = false;
        AnimationInit = false;
      }
      printf("TIME_3S\r\n");
            
    }
    else if (PressedTimer >= TIME_1S) {
      LampMode++;
      LampMode = LampMode % 4;
      AnimationInit = false;
      printf("TIME_1S\r\n");
    }
    else if (PressedTimer >= TIME_200MS) {
      ColorMode++;
      ColorMode = ColorMode % 7;
      AnimationInit = false;
      
      if (LampOff == true)
        LampOff = false;     
         
      printf("TIME_200MS\r\n");
    }
    else if (PressedTimer >= TIME_30MS)  {
      printf("TIME_30MS\r\n");
    }
    
    KeyValidFlag = false;    
    PressedTimer = 0;
    RepeatTimer = 0;    
  }

  if ( LampOff == false ) {
    if ( AnimationInit == false ) {
      switch (ColorMode) {
        case 0:
          rgbcolor = RgbColor(255, 0, 0);
          break;
        case 1:
          rgbcolor = RgbColor(0, 255, 0);
          break;
        case 2:
          rgbcolor = RgbColor(0, 0, 255);
          break;
        case 3:
          rgbcolor = RgbColor(255, 255, 0);
          break;
        case 4:
          rgbcolor = RgbColor(255, 0, 255);
          break;
        case 5:
          rgbcolor = RgbColor(0, 255, 255);
          break;
        case 6:
          rgbcolor = RgbColor(255, 255, 255);
          break;
        default:
          rgbcolor = RgbColor(255, 0, 0);
          break;
      }
      rgbcolor.Darken(delta);

      switch (LampMode) {
        case 0:
          hslcolor = HslColor(rgbcolor);
          rgbcolor = HslColor(hslcolor.H, hslcolor.S, MaxLightness);
          for (char i = 0; i < PixelCount; i++)
            strip.SetPixelColor(i, colorGamma.Correct(rgbcolor));
          strip.Show();
          break;
        case 1:
          // Draw the tail that will be rotated through all the rest of the pixels
          DrawTailPixels();
          // we use the index 0 animation to time how often we rotate all the pixels
          animations.StartAnimation(0, 100, LoopAnimUpdate);
          break;
        case 2:
          FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
          break;
        case 3:
          PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
          break;
        default:
          break;
      }
      AnimationInit = true;
    } 

    if (animations.IsAnimating()) {
      animations.UpdateAnimations();
      strip.Show();
    }
    else {
      if ( LampMode == 2 )
        FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
      else if ( LampMode == 3 )
        PickRandom(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
      else {
        ;
      }
    }
    strip.Show();
  }

}
