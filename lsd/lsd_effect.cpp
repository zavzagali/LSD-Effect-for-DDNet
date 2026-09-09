#include "lsd_effect.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

#include <engine/console.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

void CLsdEffect::OnConsoleInit()
{
    Console()->Register("lsd_toggle", "", CFGFLAG_CLIENT, ConToggle, this,
       "Toggle the LSD screen effect on/off");
    Console()->Register("lsd_speed", "?f", CFGFLAG_CLIENT, ConSpeed, this,
       "Set hue-cycle speed (default 1.0)");
    Console()->Register("lsd_intensity", "?f", CFGFLAG_CLIENT, ConIntensity, this,
       "Set tint overlay strength 0..1 (default 0.15)");
    Console()->Register("lsd_wobble", "?f", CFGFLAG_CLIENT, ConWobble, this,
       "Set zoom-breathing amount 0..1 (default 0.08)");
    Console()->Register("lsd_breathe", "?f", CFGFLAG_CLIENT, ConBreathe, this,
       "Set pattern breathing amount 0..1, independent of lsd_wobble (default 0.2)");
}

void CLsdEffect::OnShutdown()
{
    DestroyTexture();
}

void CLsdEffect::OnRender()
{
    if(!m_Enabled)
    {
       m_Time = 0.0f;
       m_WasEnabled = false;
       return;
    }

    if(!m_WasEnabled)
    {
        m_LastUpdateTime = -UPDATE_INTERVAL; 
        m_LastModeSwitch = 0.0f;
        m_TransitionProgress = 1.0f; 
        m_WasEnabled = true;
    }

    m_Time += Client()->RenderFrameTime();
    RenderTintOverlay();
}

float CLsdEffect::ZoomModifier() const
{
    if(!m_Enabled)
       return 1.0f;
    return 1.0f + std::sin(m_Time * m_WobbleSpeed * 2.0f * 3.14159265f) * m_WobbleAmount;
}

void CLsdEffect::HSVtoRGB(float h, float s, float v, float &r, float &g, float &b){
    float h6 = h * 6.0f;
    int i = (int)h6;
    float f = h6 - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch(i % 6)
    {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
}

void CLsdEffect::DestroyTexture()
{
    for(int i = 0; i < 2; i++)
    {
        if(m_TexValid[i])
        {
            Graphics()->UnloadTexture(&m_TintTexture[i]);
            m_TexValid[i] = false;
        }
    }
    std::vector<unsigned char>().swap(m_TexData);
}

void CLsdEffect::BuildLookupTables()
{
    if(m_TablesReady)
       return;

    for(int i = 0; i < TEX_SIZE; i++)
       m_AxisLut[i] = (i / (float)(TEX_SIZE - 1)) * 2.0f - 1.0f;

    m_RadiusLut.resize(TEX_SIZE * TEX_SIZE);
    m_AngleLut.resize(TEX_SIZE * TEX_SIZE);

    for(int y = 0; y < TEX_SIZE; y++)
    {
        for(int x = 0; x < TEX_SIZE; x++)
        {
            float DX = m_AxisLut[x];
            float DY = m_AxisLut[y]; 

            int Idx = y * TEX_SIZE + x;
            m_RadiusLut[Idx] = std::sqrt(DX * DX + DY * DY);
            m_AngleLut[Idx] = std::atan2(DY, DX);
        }
    }

    m_TablesReady = true;
}

void CLsdEffect::UpdateTintTexture()
{
    if(m_TexData.empty())
       m_TexData.assign(TEX_SIZE * TEX_SIZE * 4, 0);

    BuildLookupTables();

    if(m_Time - m_LastModeSwitch > 8.0f && m_TransitionProgress >= 1.0f)
    {
        do {
            m_NextEffectMode = std::rand() % 3;
        } while(m_NextEffectMode == m_CurrentMode);

        m_TransitionProgress = 0.0f;
        m_LastModeSwitch = m_Time;
    }

    if(m_TransitionProgress < 1.0f)
    {
        m_TransitionProgress += UPDATE_INTERVAL * 0.4f;
        if(m_TransitionProgress >= 1.0f)
        {
            m_TransitionProgress = 1.0f;
            m_CurrentMode = m_NextEffectMode;
        }
    }

    const float SpiralTimeTerm = m_Time * m_Speed * 2.5f; 
    const float BreathTimeTerm = m_Time * m_WobbleSpeed; 
    const float WaveP1TimeTerm = m_Time * m_Speed; 
    const float WaveP2TimeTerm = m_Time * m_Speed * 1.2f; 
    const float SatTimeTerm = 0.7f + 0.3f * std::sin(m_Time * 0.5f); 
    const float KaleidoTimeTerm = m_Time * m_Speed * 3.5f; 
    const float ValTimeTerm = 0.8f + 0.2f * std::abs(std::sin(m_Time * 2.0f)); 
    const float HueTimeOffset = m_Time * 0.05f; 
    const float BreatheTimeTerm = m_Time * m_BreatheSpeed * 2.0f * 3.14159265f; 

    auto ComputeMode = [&](int Mode, float DX, float DY, float Radius, float Angle,
       float &OutHue, float &OutSat, float &OutVal, float &OutAlpha, float &OutWave) {
        float Wave = 0.0f;
        OutSat = 0.9f;
        OutVal = 1.0f;

        if(Mode == 0)
        {
            float LogRadius = std::log(Radius + 0.1f);
            float Spiral = std::sin(LogRadius * 5.0f - Angle + SpiralTimeTerm);
            float Breathing = std::cos(Radius * 3.0f - BreathTimeTerm);
            Wave = (Spiral + Breathing) * 0.5f;
        }
        else if(Mode == 1)
        {
            float P1 = std::sin(DX * 4.0f + WaveP1TimeTerm);
            float P2 = std::sin(DY * 5.0f - WaveP2TimeTerm);
            float P3 = std::sin(std::sqrt((DX * 3.0f + m_Time) * (DX * 3.0f + m_Time) + (DY * 3.0f) * (DY * 3.0f)));
            Wave = (P1 + P2 + P3) / 3.0f;
            OutSat = SatTimeTerm;
        }
        else
        {
            float KAngle = std::abs(std::fmod(Angle, 3.14159265f / 3.0f) - (3.14159265f / 6.0f));
            Wave = std::sin(Radius * 10.0f + KAngle * 4.0f - KaleidoTimeTerm);
            OutVal = ValTimeTerm;
        }

        OutWave = Wave;
        OutHue = std::fmod(std::fmod(std::abs(Wave), 1.0f) + HueTimeOffset, 1.0f);
        OutAlpha = m_Intensity * (0.3f + 0.7f * (Wave * 0.5f + 0.5f));
    };

    const bool InTransition = m_TransitionProgress < 1.0f;
    const float Blend = InTransition ? (1.0f - std::cos(m_TransitionProgress * 3.14159265f)) * 0.5f : 0.0f;

    for(int y = 0; y < TEX_SIZE; y++)
    {
        for(int x = 0; x < TEX_SIZE; x++)
        {
            float DX = m_AxisLut[x];
            float DY = m_AxisLut[y];

            int LutIdx = y * TEX_SIZE + x;
            float Radius = m_RadiusLut[LutIdx];
            float Angle = m_AngleLut[LutIdx];

            float BreathePhase = Radius * 6.0f;
            float BreatheOffset = m_BreatheAmount * 0.5f * std::sin(BreatheTimeTerm - BreathePhase);
            float EffRadius = Radius + BreatheOffset;

            float HueCurr, SatCurr, ValCurr, AlphaCurr, WaveCurr;
            ComputeMode(m_CurrentMode, DX, DY, EffRadius, Angle, HueCurr, SatCurr, ValCurr, AlphaCurr, WaveCurr);

            float FinalR, FinalG, FinalB, FinalAlpha = AlphaCurr;
            HSVtoRGB(HueCurr, SatCurr, ValCurr, FinalR, FinalG, FinalB);

            if(InTransition)
            {
                float HueNext, SatNext, ValNext, AlphaNext, WaveNext;
                ComputeMode(m_NextEffectMode, DX, DY, EffRadius, Angle, HueNext, SatNext, ValNext, AlphaNext, WaveNext);

                float NextR, NextG, NextB;
                HSVtoRGB(HueNext, SatNext, ValNext, NextR, NextG, NextB);

                FinalR = FinalR * (1.0f - Blend) + NextR * Blend;
                FinalG = FinalG * (1.0f - Blend) + NextG * Blend;
                FinalB = FinalB * (1.0f - Blend) + NextB * Blend;
                FinalAlpha = AlphaCurr * (1.0f - Blend) + AlphaNext * Blend;
            }

            int Offset = (y * TEX_SIZE + x) * 4;
            m_TexData[Offset + 0] = (unsigned char)(std::clamp(FinalR, 0.0f, 1.0f) * 255.0f);
            m_TexData[Offset + 1] = (unsigned char)(std::clamp(FinalG, 0.0f, 1.0f) * 255.0f);
            m_TexData[Offset + 2] = (unsigned char)(std::clamp(FinalB, 0.0f, 1.0f) * 255.0f);
            m_TexData[Offset + 3] = (unsigned char)(std::clamp(FinalAlpha, 0.0f, 1.0f) * 255.0f);
        }
    }

    int TargetSlot = 1 - m_ActiveTexture;

    CImageInfo Image;
    Image.m_Width = TEX_SIZE;
    Image.m_Height = TEX_SIZE;
    Image.m_Format = CImageInfo::FORMAT_RGBA;
    Image.m_pData = m_TexData.data();

    if(m_TexValid[TargetSlot])
    {
        Graphics()->UnloadTexture(&m_TintTexture[TargetSlot]);
        m_TexValid[TargetSlot] = false;
    }

    m_TintTexture[TargetSlot] = Graphics()->LoadTextureRaw(Image, 0);
    m_TexValid[TargetSlot] = m_TintTexture[TargetSlot].IsValid();

    if(m_TexValid[TargetSlot])
       m_ActiveTexture = TargetSlot; 
}

void CLsdEffect::RenderTintOverlay()
{
    bool AnyTextureValid = m_TexValid[0] || m_TexValid[1];
    if(!AnyTextureValid || m_Time - m_LastUpdateTime >= UPDATE_INTERVAL)
    {
        UpdateTintTexture();
        m_LastUpdateTime = m_Time;
    }

    if(!m_TexValid[m_ActiveTexture])
       return; 

    float Width = Graphics()->ScreenWidth();
    float Height = Graphics()->ScreenHeight();

    Graphics()->MapScreenToSize(Width, Height);

    Graphics()->TextureSet(m_TintTexture[m_ActiveTexture]);
    Graphics()->BlendNormal();
    Graphics()->WrapClamp(); 

    Graphics()->QuadsBegin();
    Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f); 

    IGraphics::CQuadItem QuadItem(0, 0, Width, Height);
    Graphics()->QuadsDrawTL(&QuadItem, 1);

    Graphics()->QuadsEnd();
    Graphics()->WrapNormal(); 
}

void CLsdEffect::ConToggle(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    pSelf->m_Enabled = !pSelf->m_Enabled;
}

void CLsdEffect::ConSpeed(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_Speed = pResult->GetFloat(0);
}

void CLsdEffect::ConIntensity(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_Intensity = pResult->GetFloat(0);
}

void CLsdEffect::ConWobble(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_WobbleAmount = pResult->GetFloat(0);
}

void CLsdEffect::ConBreathe(IConsole::IResult *pResult, void *pUserData)
{
    CLsdEffect *pSelf = (CLsdEffect *)pUserData;
    if(pResult->NumArguments())
       pSelf->m_BreatheAmount = pResult->GetFloat(0);
}
