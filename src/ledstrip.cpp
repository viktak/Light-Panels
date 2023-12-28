#include <Arduino.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include "settings.h"

//? Pick one: WS2812B, SK6812
#define WS2812B

#ifdef WS2812B
struct RotatingPanelAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef SK6812
struct RotatingPanelAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef WS2812B
struct ChaserAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef SK6812
struct ChaserAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};
#endif

#ifdef WS2812B
struct SlowPanelsAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};
#endif

#ifdef SK6812
struct SlowPanelsAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
};
#endif

#ifdef WS2812B
struct FastSegmentsAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t Segment;
    bool dir;
};
#endif

#ifdef SK6812
struct FastSegmentsAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint16_t Segment;
    bool dir;
};
#endif

const uint16_t LEDS_PER_SEGMENT = 8;
const uint16_t SEGMENTS_PER_PANEL = 3; //  3: tirangle, 6: hexagon, etc.
const uint16_t NUMBER_OF_PANELS = 6;

const uint16_t LEDS_PER_PANEL = LEDS_PER_SEGMENT * SEGMENTS_PER_PANEL;
const uint16_t TOTAL_LEDS = LEDS_PER_PANEL * NUMBER_OF_PANELS;

const uint16_t AnimCount = TOTAL_LEDS;

const uint16_t PixelFadeDuration = 300;
// one second divide by the number of pixels = loop once a second
const uint16_t NextPixelMoveDuration = 2000 / TOTAL_LEDS; // how fast we move through the pixels

NeoGamma<NeoGammaTableMethod> colorGamma; // for any fade animations, best to correct gamma

#ifdef WS2812B
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(TOTAL_LEDS);
#endif
#ifdef SK6812
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(TOTAL_LEDS);
#endif

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

ChaserAnimationState chaserAnimationState[AnimCount];
SlowPanelsAnimationState slowPanelAnimationState[NUMBER_OF_PANELS];
FastSegmentsAnimationState fastSegmentsAnimationState;
RotatingPanelAnimationState rotatingPanelAnimationState[AnimCount];

uint16_t frontPixel = 0; // the front of the loop
RgbColor frontColor;     // the color at the front of the loop

void FadeOutAnimUpdate(const AnimationParam &param)
{
// this gets called for each animation on every time step
// progress will start at 0.0 and end at 1.0
// we use the blend function on the RgbColor to mix
// color based on the progress given to us in the animation
#ifdef WS2812B
    RgbColor updatedColor = RgbColor::LinearBlend(
        chaserAnimationState[param.index].StartingColor,
        chaserAnimationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(chaserAnimationState[param.index].IndexPixel,
                        colorGamma.Correct(updatedColor));
#endif

#ifdef SK6812
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        chaserAnimationState[param.index].StartingColor,
        chaserAnimationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(chaserAnimationState[param.index].IndexPixel,
                        colorGamma.Correct(updatedColor));
#endif
}

void ChaseAnimUpdate(const AnimationParam &param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // pick the next pixel inline to start animating
        //
        frontPixel = (frontPixel + 1) % TOTAL_LEDS; // increment and wrap
        if (frontPixel == 0)
        {
            // we looped, lets pick a new front color
            frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
        }

        uint16_t indexAnim;
        // do we have an animation available to use to animate the next front pixel?
        // if you see skipping, then either you are going to fast or need to increase
        // the number of animation channels
        if (animations.NextAvailableAnimation(&indexAnim, 1))
        {
            chaserAnimationState[indexAnim].StartingColor = frontColor;
            chaserAnimationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
            chaserAnimationState[indexAnim].IndexPixel = frontPixel;

            animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
        }
    }
}

void FadeOutPanelAnimUpdate(const AnimationParam &param)
{

#ifdef WS2812B
    RgbColor updatedColor = RgbColor::LinearBlend(
        rotatingPanelAnimationState[param.index].StartingColor,
        rotatingPanelAnimationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(rotatingPanelAnimationState[param.index].IndexPixel,
                        colorGamma.Correct(updatedColor));
#endif

#ifdef SK6812
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        rotatingPanelAnimationState[param.index].StartingColor,
        rotatingPanelAnimationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(rotatingPanelAnimationState[param.index].IndexPixel,
                        colorGamma.Correct(updatedColor));
#endif
}

void RotatingPanelAnimUpdate(const AnimationParam &param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // pick the next pixel inline to start animating
        frontPixel = (frontPixel + 1) % LEDS_PER_PANEL; // increment and wrap
        if (frontPixel == 0)
        {
            // we looped, lets pick a new front color
            frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
        }

        for (size_t i = 0; i < NUMBER_OF_PANELS; i++)
        {
            uint16_t indexAnim;
            if (animations.NextAvailableAnimation(&indexAnim, 1))
            {
                switch (settings::mode)
                {
                case settings::OPERATION_MODES::ROTATING_PANELS:
                {
                    rotatingPanelAnimationState[indexAnim].StartingColor = frontColor;
                    rotatingPanelAnimationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
                    rotatingPanelAnimationState[indexAnim].IndexPixel = frontPixel + i * LEDS_PER_PANEL;
                    break;
                }

                case settings::OPERATION_MODES::ROTATING_PANELS_INVERTED:
                {
                    rotatingPanelAnimationState[indexAnim].StartingColor = RgbColor(0, 0, 0);
                    rotatingPanelAnimationState[indexAnim].EndingColor = frontColor;
                    rotatingPanelAnimationState[indexAnim].IndexPixel = frontPixel + i * LEDS_PER_PANEL;
                    break;
                }

                default:
                    break;
                }

                animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutPanelAnimUpdate);
            }
        }
    }
}

void BlendAnimUpdate(const AnimationParam &param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation

#ifdef WS2812B
    for (size_t panel = 0; panel < NUMBER_OF_PANELS; panel++)
    {
        RgbColor updatedColor = RgbColor::LinearBlend(
            slowPanelAnimationState[panel].StartingColor,
            slowPanelAnimationState[panel].EndingColor,
            param.progress);

        // apply the color to the strip
        for (uint16_t pixel = 0; pixel < LEDS_PER_PANEL; pixel++)
        {
            strip.SetPixelColor(panel * LEDS_PER_PANEL + pixel, updatedColor);
        }
    }
#endif

#ifdef SK6812
    for (size_t panel = 0; panel < NUMBER_OF_PANELS; panel++)
    {
        RgbwColor updatedColor = RgbwColor::LinearBlend(
            slowPanelAnimationState[panel].StartingColor,
            slowPanelAnimationState[panel].EndingColor,
            param.progress);

        // apply the color to the strip
        for (uint16_t pixel = 0; pixel < LEDS_PER_PANEL; pixel++)
        {
            strip.SetPixelColor(panel * LEDS_PER_PANEL + pixel, updatedColor);
        }
    }
#endif
}

void FastSegmentsAnimUpdate(const AnimationParam &param)
{
#ifdef WS2812B
    RgbColor updatedColor = RgbColor::LinearBlend(
        fastSegmentsAnimationState.StartingColor,
        fastSegmentsAnimationState.EndingColor,
        param.progress);

    for (uint16_t pixel = 0; pixel < LEDS_PER_SEGMENT; pixel++)
    {
        strip.SetPixelColor(fastSegmentsAnimationState.Segment * LEDS_PER_SEGMENT + pixel, updatedColor);
    }
#endif

#ifdef SK6812
    RgbwColor updatedColor = RgbwColor::LinearBlend(
        fastSegmentsAnimationState.StartingColor,
        fastSegmentsAnimationState.EndingColor,
        param.progress);

    for (uint16_t pixel = 0; pixel < LEDS_PER_SEGMENT; pixel++)
    {
        strip.SetPixelColor(fastSegmentsAnimationState.Segment * LEDS_PER_SEGMENT + pixel, updatedColor);
    }
#endif
}

void StopAnimations()
{

    animations.StopAll();             //  Stop all animations
    strip.ClearTo(RgbColor(0, 0, 0)); //  Clear all pixels
    strip.Show();
}

void setupLedStrip()
{
    strip.Begin();
    for (size_t i = 0; i < TOTAL_LEDS; i++)
    {
        strip.ClearTo(RgbColor(0));
    }
    strip.Show();
}

void loopLedStrip()
{
    if (animations.IsAnimating())
    {
        animations.UpdateAnimations();
        strip.Show();
    }
    else
    {
        switch (settings::mode)
        {
        case settings::OPERATION_MODES::LED_CHASER:
        {
            animations.StartAnimation(0, NextPixelMoveDuration, ChaseAnimUpdate);
            break;
        }
        case settings::OPERATION_MODES::SLOW_PANELS:
        {
            for (size_t panel = 0; panel < NUMBER_OF_PANELS; panel++)
            {
                RgbColor target = HslColor(random(360) / 360.0f, 1.0f, .5f);

                slowPanelAnimationState[panel].StartingColor = strip.GetPixelColor(panel * LEDS_PER_PANEL);
                slowPanelAnimationState[panel].EndingColor = target;

                animations.StartAnimation(panel, 5000, BlendAnimUpdate);
            }
            break;
        }

        case settings::OPERATION_MODES::FAST_CHANGING_RANDOM_SEGMENTS:
        {
            if (fastSegmentsAnimationState.dir)
            {
                fastSegmentsAnimationState.Segment = random(NUMBER_OF_PANELS) * SEGMENTS_PER_PANEL + random(SEGMENTS_PER_PANEL);
                fastSegmentsAnimationState.StartingColor = RgbColor(0, 0, 0);
                fastSegmentsAnimationState.EndingColor = HslColor(random(360) / 360.0f, 1.0f, .5f);
            }
            else
            {
                fastSegmentsAnimationState.StartingColor = fastSegmentsAnimationState.EndingColor;
                fastSegmentsAnimationState.EndingColor = RgbColor(0, 0, 0);
            }

            animations.StartAnimation(0, 50, FastSegmentsAnimUpdate);
            fastSegmentsAnimationState.dir = !fastSegmentsAnimationState.dir;
            break;
        }
        case settings::OPERATION_MODES::ROTATING_PANELS:
        {
            animations.StartAnimation(0, NextPixelMoveDuration, RotatingPanelAnimUpdate);
            break;
        }
        case settings::OPERATION_MODES::ROTATING_PANELS_INVERTED:
        {

            animations.StartAnimation(0, NextPixelMoveDuration, RotatingPanelAnimUpdate);
            break;
        }
        default:
            break;
        }
    }
}
