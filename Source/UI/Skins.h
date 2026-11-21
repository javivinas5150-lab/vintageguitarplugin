#pragma once

#include <JuceHeader.h>

//==============================================================================
// Skins: complete material/colour sets for the whole rig.
// Index 0 is the default. All are original designs (no brand trade dress).
//==============================================================================
struct Skin
{
    juce::String name;
    juce::String ampModelLine;           // small print under the panel logo

    // Room
    juce::Colour wall, spotlight;
    juce::Colour woodTop, woodBottom, woodGrain, titleText;

    // Covering of the amp head and bottom strip
    enum class Covering { tolex, tweed } covering = Covering::tolex;
    juce::Colour covering1, covering2;   // tolex base | tweed light/dark yarn

    // Grille cloth (basket weave)
    juce::Colour grilleLight, grilleMid, grilleShadow, grilleBacking;
    juce::Colour piping;

    // Amp control panel
    juce::Colour panelTop, panelBottom, panelText, panelTrim;
    bool brushedPanel = false;           // brushed metal instead of painted
    juce::Colour knobCap, knobNumbers;
    bool chickenKnobs = false;            // white chicken-head amp knobs
    bool aluminiumKnobs = false;          // big spun-aluminium amp knob caps
    juce::Colour logo;                   // panel logo colour

    // Pedals
    juce::Colour vibeTop, vibeBottom, vibeText;
    juce::Colour goldTop, goldBottom, goldText;
    juce::Colour namTop, namBottom, namText;

    // Rack + display
    juce::Colour rackFace, rackAccent, rackTitle, display, displayGlass;

    // Bottom strip sub-panels
    juce::Colour subPanel;
};

namespace skins
{
    inline const std::vector<Skin>& all()
    {
        static const std::vector<Skin> list = []
        {
            std::vector<Skin> v;

            //------------------------------------------------------------------
            // 1. BLACKFACE - black tolex, wheat grille, black panel, blue rack
            {
                Skin s;
                s.name = "Blackface";             s.ampModelLine = "BLACK PANEL  '65";
                s.wall = juce::Colour (0xff15110f); s.spotlight = juce::Colour (0x40ffd8a0);
                s.woodTop = juce::Colour (0xff3a2b20); s.woodBottom = juce::Colour (0xff17100b);
                s.woodGrain = juce::Colour (0xffc89060); s.titleText = juce::Colour (0xffeadbb2);
                s.covering = Skin::Covering::tolex; s.covering1 = juce::Colour (0xff1b1b1d);
                s.grilleLight = juce::Colour (0xfff2dcaa); s.grilleMid = juce::Colour (0xffa88a5a);
                s.grilleShadow = juce::Colour (0xff160e08); s.grilleBacking = juce::Colour (0xff6e5438);
                s.piping = juce::Colour (0xfff3ecdc);
                s.panelTop = juce::Colour (0xff202024); s.panelBottom = juce::Colour (0xff060607);
                s.panelText = juce::Colour (0xffe9dfc6); s.panelTrim = juce::Colour (0xffb9b4a6);
                s.knobCap = juce::Colour (0xffeedfb8); s.knobNumbers = juce::Colour (0xfff2ece0);
                s.logo = juce::Colour (0xffe9dfc6);
                s.vibeTop = juce::Colour (0xff4a3a62); s.vibeBottom = juce::Colour (0xff241a33); s.vibeText = juce::Colour (0xffe9dfc6);
                s.goldTop = juce::Colour (0xffe4c46e); s.goldBottom = juce::Colour (0xffa9822f); s.goldText = juce::Colour (0xff1a1714);
                s.namTop = juce::Colour (0xff3f4a44); s.namBottom = juce::Colour (0xff1a201d); s.namText = juce::Colour (0xffe9dfc6);
                s.rackFace = juce::Colour (0xff101114); s.rackAccent = juce::Colour (0xff2ea6ff);
                s.rackTitle = juce::Colour (0xfff4f5f7); s.display = juce::Colour (0xffff3a2a); s.displayGlass = juce::Colour (0xff240a07);
                s.subPanel = juce::Colour (0xff1d1d21);
                v.push_back (s);
            }

            //------------------------------------------------------------------
            // 2. TWEED - lacquered tweed, brown/gold grille, brown panel, amber rack
            {
                Skin s;
                s.name = "Tweed";                 s.ampModelLine = "NARROW PANEL  '57";
                s.wall = juce::Colour (0xff1b130c); s.spotlight = juce::Colour (0x50ffc070);
                s.woodTop = juce::Colour (0xff5a3a1e); s.woodBottom = juce::Colour (0xff24160a);
                s.woodGrain = juce::Colour (0xffe0a860); s.titleText = juce::Colour (0xfff3dca8);
                s.covering = Skin::Covering::tweed;
                s.covering1 = juce::Colour (0xffd9b878); s.covering2 = juce::Colour (0xff5e3d1c);
                s.grilleLight = juce::Colour (0xffe2b86a); s.grilleMid = juce::Colour (0xff8a5e2a);
                s.grilleShadow = juce::Colour (0xff1a0e05); s.grilleBacking = juce::Colour (0xff4a2c12);
                s.piping = juce::Colour (0xff3a2210);
                s.panelTop = juce::Colour (0xff4a3220); s.panelBottom = juce::Colour (0xff22160c);
                s.panelText = juce::Colour (0xfff3e2bc); s.panelTrim = juce::Colour (0xffd8b878);
                s.knobCap = juce::Colour (0xff3a2414); s.knobNumbers = juce::Colour (0xfff3e2bc);
                s.logo = juce::Colour (0xfff6e6c0);
                s.vibeTop = juce::Colour (0xff7a2a22); s.vibeBottom = juce::Colour (0xff3a0f0b); s.vibeText = juce::Colour (0xfff3e2bc);
                s.goldTop = juce::Colour (0xffead08a); s.goldBottom = juce::Colour (0xffb08a3a); s.goldText = juce::Colour (0xff2a1a0c);
                s.namTop = juce::Colour (0xff35402a); s.namBottom = juce::Colour (0xff171d10); s.namText = juce::Colour (0xfff3e2bc);
                s.rackFace = juce::Colour (0xff2a2118); s.rackAccent = juce::Colour (0xffffb040);
                s.rackTitle = juce::Colour (0xfff6e6c0); s.display = juce::Colour (0xffffa62a); s.displayGlass = juce::Colour (0xff221405);
                s.subPanel = juce::Colour (0xff2a2016);
                v.push_back (s);
            }

            //------------------------------------------------------------------
            // 3. BLONDE - cream tolex, oxblood grille, brown panel, champagne rack
            {
                Skin s;
                s.name = "Blonde";                s.ampModelLine = "BROWN PANEL  '62";
                s.wall = juce::Colour (0xff17181b); s.spotlight = juce::Colour (0x48fff0d0);
                s.woodTop = juce::Colour (0xff4a3426); s.woodBottom = juce::Colour (0xff1e140d);
                s.woodGrain = juce::Colour (0xffd8a878); s.titleText = juce::Colour (0xfff4ead2);
                s.covering = Skin::Covering::tolex; s.covering1 = juce::Colour (0xffd9c9a3);
                s.grilleLight = juce::Colour (0xffb0584a); s.grilleMid = juce::Colour (0xff6e2a20);
                s.grilleShadow = juce::Colour (0xff160504); s.grilleBacking = juce::Colour (0xff3e120d);
                s.piping = juce::Colour (0xfff6efe0);
                s.panelTop = juce::Colour (0xff4a2e1e); s.panelBottom = juce::Colour (0xff23140b);
                s.panelText = juce::Colour (0xfff4e8cc); s.panelTrim = juce::Colour (0xffe6d6b0);
                s.knobCap = juce::Colour (0xfff0e6cc); s.knobNumbers = juce::Colour (0xfff4e8cc);
                s.logo = juce::Colour (0xfff4e8cc);
                s.vibeTop = juce::Colour (0xff2e5a66); s.vibeBottom = juce::Colour (0xff10262c); s.vibeText = juce::Colour (0xfff4ead2);
                s.goldTop = juce::Colour (0xffe6c878); s.goldBottom = juce::Colour (0xffa88434); s.goldText = juce::Colour (0xff1e1710);
                s.namTop = juce::Colour (0xff5a5048); s.namBottom = juce::Colour (0xff26211c); s.namText = juce::Colour (0xfff4ead2);
                s.rackFace = juce::Colour (0xff2c2a28); s.rackAccent = juce::Colour (0xffffd27a);
                s.rackTitle = juce::Colour (0xfff4ead2); s.display = juce::Colour (0xffff4632); s.displayGlass = juce::Colour (0xff240a07);
                s.subPanel = juce::Colour (0xff2a2622);
                v.push_back (s);
            }

            //------------------------------------------------------------------
            // 4. MIDNIGHT - blue-black tolex, silver grille, brushed silver
            //    panel, ice-blue studio rack
            {
                Skin s;
                s.name = "Midnight";              s.ampModelLine = "SILVER PANEL  '74";
                s.wall = juce::Colour (0xff0e1116); s.spotlight = juce::Colour (0x40a0c8ff);
                s.woodTop = juce::Colour (0xff23252b); s.woodBottom = juce::Colour (0xff0c0d10);
                s.woodGrain = juce::Colour (0xff8a9aae); s.titleText = juce::Colour (0xffdfe8f4);
                s.covering = Skin::Covering::tolex; s.covering1 = juce::Colour (0xff151922);
                s.grilleLight = juce::Colour (0xffd6dde6); s.grilleMid = juce::Colour (0xff7d8794);
                s.grilleShadow = juce::Colour (0xff07090c); s.grilleBacking = juce::Colour (0xff242a33);
                s.piping = juce::Colour (0xffc8ced6);
                s.panelTop = juce::Colour (0xffc4c9d0); s.panelBottom = juce::Colour (0xff8e949c);
                s.panelText = juce::Colour (0xff12161e); s.panelTrim = juce::Colour (0xff3a4250);
                s.brushedPanel = true;
                s.knobCap = juce::Colour (0xff0b0c0e); s.knobNumbers = juce::Colour (0xff12161e);
                s.logo = juce::Colour (0xff0f2a52);
                s.vibeTop = juce::Colour (0xff23345e); s.vibeBottom = juce::Colour (0xff0c1328); s.vibeText = juce::Colour (0xffdfe8f4);
                s.goldTop = juce::Colour (0xffdcc070); s.goldBottom = juce::Colour (0xff9a7a30); s.goldText = juce::Colour (0xff10141c);
                s.namTop = juce::Colour (0xff1f2a26); s.namBottom = juce::Colour (0xff0b100e); s.namText = juce::Colour (0xffdfe8f4);
                s.rackFace = juce::Colour (0xff15181e); s.rackAccent = juce::Colour (0xff5fd8ff);
                s.rackTitle = juce::Colour (0xffeef4fb); s.display = juce::Colour (0xff48d0ff); s.displayGlass = juce::Colour (0xff04141c);
                s.subPanel = juce::Colour (0xff181b21);
                v.push_back (s);
            }

            return v;
        }();
        return list;
    }

    inline const Skin& get (int index)
    {
        const auto& list = all();
        return list[(size_t) juce::jlimit (0, (int) list.size() - 1, index)];
    }
}
