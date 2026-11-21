#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <tuple>

//==============================================================================
// Realism helpers: procedural textures and lighting tricks shared by the panel
// and the LookAndFeel. Light always comes from the top-left.
//==============================================================================
namespace realism
{
    //--------------------------------------------------------------------------
    // Small float field utilities (wrap-around so textures tile seamlessly)
    struct Field
    {
        int w = 0, h = 0;
        std::vector<float> v;

        Field (int width, int height) : w (width), h (height), v ((size_t) (width * height), 0.0f) {}
        float& at (int x, int y)       { return v[(size_t) (((y % h + h) % h) * w + ((x % w + w) % w))]; }
        float  at (int x, int y) const { return v[(size_t) (((y % h + h) % h) * w + ((x % w + w) % w))]; }

        void randomise (juce::Random& r) { for (auto& f : v) f = r.nextFloat() - 0.5f; }

        void blurH (int radius)
        {
            std::vector<float> out (v.size());
            for (int y = 0; y < h; ++y)
            {
                float sum = 0.0f;
                for (int k = -radius; k <= radius; ++k) sum += at (k, y);
                for (int x = 0; x < w; ++x)
                {
                    out[(size_t) (y * w + x)] = sum / (float) (2 * radius + 1);
                    sum += at (x + radius + 1, y) - at (x - radius, y);
                }
            }
            v.swap (out);
        }

        void blurV (int radius)
        {
            std::vector<float> out (v.size());
            for (int x = 0; x < w; ++x)
            {
                float sum = 0.0f;
                for (int k = -radius; k <= radius; ++k) sum += at (x, k);
                for (int y = 0; y < h; ++y)
                {
                    out[(size_t) (y * w + x)] = sum / (float) (2 * radius + 1);
                    sum += at (x, y + radius + 1) - at (x, y - radius);
                }
            }
            v.swap (out);
        }

        void normalise()
        {
            double mean = 0.0, sq = 0.0;
            for (auto f : v) mean += f;
            mean /= (double) v.size();
            for (auto f : v) sq += (f - mean) * (f - mean);
            const auto sd = (float) std::sqrt (sq / (double) v.size()) + 1.0e-6f;
            for (auto& f : v) f = (f - (float) mean) / sd;
        }
    };

    inline juce::uint8 clampByte (float x) { return (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (x)); }

    inline juce::Colour shade (juce::Colour base, float delta)   // delta in 0..255 units
    {
        return juce::Colour (clampByte ((float) base.getRed()   + delta),
                             clampByte ((float) base.getGreen() + delta),
                             clampByte ((float) base.getBlue()  + delta));
    }

    //--------------------------------------------------------------------------
    // Pebbled leather tolex: blurred noise used as a height map, then embossed
    inline juce::Image makeTolex (int size, juce::Colour base, int seed, float strength = 1.0f, bool coarse = false)
    {
        juce::Random rnd (seed);
        Field grain (size, size), large (size, size);
        grain.randomise (rnd);
        grain.blurH (1); grain.blurV (1); grain.blurH (1); grain.blurV (1);
        if (coarse) { grain.blurH (2); grain.blurV (2); }   // bigger "levant" pebbles
        grain.normalise();
        large.randomise (rnd);
        large.blurH (6); large.blurV (6);
        large.normalise();

        juce::Image img (juce::Image::RGB, size, size, false);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float emboss = grain.at (x - 1, y - 1) - grain.at (x + 1, y + 1);   // top-left light
                const float d = (emboss * 9.0f + large.at (x, y) * 2.5f) * strength + (rnd.nextFloat() - 0.5f) * 4.0f;
                bd.setPixelColour (x, y, shade (base, d));
            }

        return img;
    }

    //--------------------------------------------------------------------------
    // Lacquered tweed: diagonal twill of light and dark yarns, slub texture and
    // an amber lacquer tint. Tiles seamlessly when size is a multiple of 8.
    inline juce::Image makeTweed (int size, juce::Colour lightYarn, juce::Colour darkYarn, int seed)
    {
        juce::Random rnd (seed);
        Field slub (size, size);
        slub.randomise (rnd);
        slub.blurH (3); slub.blurV (1);
        slub.normalise();

        juce::Image img (juce::Image::RGB, size, size, false);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                // 2/2 twill: diagonal bands, each yarn 2 px wide
                const int d = ((x + y) / 2) & 3;
                const bool warpOnTop = (((x / 2) + (y / 2)) & 1) != 0;
                float t = (d < 2) ? 0.85f : 0.2f;                 // light vs dark diagonal
                t += warpOnTop ? 0.08f : -0.08f;                   // weave relief
                t += slub.at (x, y) * 0.12f + (rnd.nextFloat() - 0.5f) * 0.18f;

                // yarn shading: each 2 px yarn is lit on its upper-left pixel
                const float lit = ((x + y) & 1) == 0 ? 10.0f : -8.0f;
                auto c = darkYarn.interpolatedWith (lightYarn, juce::jlimit (0.0f, 1.0f, t));
                bd.setPixelColour (x, y, shade (c, lit));
            }

        // Amber lacquer glaze
        juce::Graphics g (img);
        g.fillAll (juce::Colour (0xffb07020).withAlpha (0.12f));
        return img;
    }

    //--------------------------------------------------------------------------
    // Brushed metal: long horizontal streaks + fine grain
    inline juce::Image makeBrushed (int w, int h, juce::Colour base, float amount, int seed)
    {
        juce::Random rnd (seed);
        Field streak (w, h);
        streak.randomise (rnd);
        streak.blurH (28);
        streak.normalise();

        juce::Image img (juce::Image::RGB, w, h, false);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < h; ++y)
        {
            const float rowTone = (rnd.nextFloat() - 0.5f) * 0.6f;
            for (int x = 0; x < w; ++x)
            {
                const float d = (streak.at (x, y) + rowTone + (rnd.nextFloat() - 0.5f) * 0.8f) * amount;
                bd.setPixelColour (x, y, shade (base, d));
            }
        }

        return img;
    }

    //--------------------------------------------------------------------------
    // Transparent fleck layer (powder coat / metallic paint / paper fibres)
    inline juce::Image makeSpeckle (int w, int h, float density, float maxAlpha, int seed)
    {
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Random rnd (seed);
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);

        const int count = (int) ((float) (w * h) * density);
        for (int i = 0; i < count; ++i)
        {
            const int x = rnd.nextInt (w), y = rnd.nextInt (h);
            const bool light = rnd.nextBool();
            const auto c = (light ? juce::Colours::white : juce::Colours::black).withAlpha (rnd.nextFloat() * maxAlpha);
            bd.setPixelColour (x, y, c);
        }

        return img;
    }

    //--------------------------------------------------------------------------
    // Fake gaussian shadow by stacking expanding, faint shapes (cheap enough for
    // knobs drawn every repaint)
    inline void softShadowEllipse (juce::Graphics& g, juce::Point<float> c, float radius, float spread,
                                   juce::Point<float> offset, float alpha)
    {
        constexpr int layers = 7;
        for (int i = layers; i >= 1; --i)
        {
            const float rr = radius + spread * (float) i / (float) layers;
            g.setColour (juce::Colours::black.withAlpha (alpha * 0.28f));
            g.fillEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c + offset * ((float) i / (float) layers)));
        }
    }

    // Real blurred drop shadow for a rounded rectangle (use when cached)
    inline void dropShadow (juce::Graphics& g, juce::Rectangle<float> r, float corner, int blur,
                            juce::Point<int> offset, float alpha)
    {
        juce::Path p;
        p.addRoundedRectangle (r, corner);
        juce::DropShadow (juce::Colours::black.withAlpha (alpha), blur, offset).drawForPath (g, p);
    }

    //--------------------------------------------------------------------------
    // Bevel: light on the top edge, dark on the bottom edge
    inline void bevel (juce::Graphics& g, juce::Rectangle<float> r, float corner, float width,
                       float lightAlpha, float darkAlpha)
    {
        juce::ColourGradient edge (juce::Colours::white.withAlpha (lightAlpha), r.getX(), r.getY(),
                                   juce::Colours::black.withAlpha (darkAlpha), r.getX(), r.getBottom(), false);
        edge.addColour (0.5, juce::Colours::transparentWhite);
        g.setGradientFill (edge);
        g.drawRoundedRectangle (r.reduced (width * 0.5f), juce::jmax (0.0f, corner - width * 0.5f), width);
    }

    // Recessed look: darkness falling in from the top and sides of an area
    inline void innerShadow (juce::Graphics& g, juce::Rectangle<float> r, float corner, float depth, float alpha)
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);

        constexpr int layers = 6;
        for (int i = 0; i < layers; ++i)
        {
            const float t = (float) i / (float) layers;
            g.setColour (juce::Colours::black.withAlpha (alpha * 0.3f * (1.0f - t)));
            g.drawRoundedRectangle (r.translated (0.0f, depth * 0.5f).reduced (depth * t), corner, depth / (float) layers * 1.6f);
        }
    }

    // Diagonal reflection band (glass / glossy paint)
    inline void sheen (juce::Graphics& g, juce::Rectangle<float> r, float corner, float alpha)
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);

        juce::Path band;
        band.startNewSubPath (r.getX(), r.getY());
        band.lineTo (r.getX() + r.getWidth() * 0.55f, r.getY());
        band.lineTo (r.getX() + r.getWidth() * 0.30f, r.getBottom());
        band.lineTo (r.getX(), r.getBottom());
        band.closeSubPath();

        juce::ColourGradient grad (juce::Colours::white.withAlpha (alpha), r.getX(), r.getY(),
                                   juce::Colours::white.withAlpha (0.0f), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (band);
    }

    //--------------------------------------------------------------------------
    // Spun / turned metal disc: anisotropic highlight that sweeps around the
    // centre, plus fine concentric machining rings
    inline void spunMetal (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour base, float contrast)
    {
        constexpr int wedges = 72;
        const float step = juce::MathConstants<float>::twoPi / (float) wedges;
        const float lightAngle = -0.785f;  // top-left (JUCE angles: 0 = up, clockwise)

        for (int i = 0; i < wedges; ++i)
        {
            const float a0 = (float) i * step, a1 = a0 + step * 1.08f;
            const float mid = a0 + step * 0.5f;
            const float k = std::cos (2.0f * (mid - lightAngle));          // two bright lobes
            juce::Path w;
            w.addPieSegment (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c), a0, a1, 0.0f);
            g.setColour (shade (base, k * contrast));
            g.fillPath (w);
        }

        g.setColour (juce::Colours::black.withAlpha (0.07f));
        for (float rr = radius * 0.15f; rr < radius; rr += juce::jmax (1.2f, radius * 0.07f))
            g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c), 0.6f);
    }

    // Small elliptical specular highlight (top-left of a dome or cap)
    inline void specular (juce::Graphics& g, juce::Point<float> c, float radius, float alpha)
    {
        const auto hc = c.translated (-radius * 0.35f, -radius * 0.42f);
        juce::ColourGradient hl (juce::Colours::white.withAlpha (alpha), hc,
                                 juce::Colours::white.withAlpha (0.0f), hc.translated (radius * 0.55f, 0.0f), true);
        g.setGradientFill (hl);
        g.fillEllipse (juce::Rectangle<float> (radius * 1.1f, radius * 0.8f).withCentre (hc));
    }

    // Vertical chrome gradient (for trims, logos, handles)
    inline juce::ColourGradient chrome (juce::Rectangle<float> r)
    {
        juce::ColourGradient grad (juce::Colour (0xfff7f8fa), r.getX(), r.getY(),
                                   juce::Colour (0xff6b7077), r.getX(), r.getBottom(), false);
        grad.addColour (0.42, juce::Colour (0xffb9bec5));
        grad.addColour (0.50, juce::Colour (0xff4b4f55));
        grad.addColour (0.62, juce::Colour (0xffd8dce1));
        return grad;
    }
}
