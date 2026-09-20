#ifndef GAME_CLIENT_COMPONENTS_LSD_LSD_EFFECT_H
#define GAME_CLIENT_COMPONENTS_LSD_LSD_EFFECT_H

#include <game/client/component.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <vector>

class CLsdEffect : public CComponent
{
	bool m_Enabled = false;
	bool m_WasEnabled = false; 
	float m_Time = 0.0f;

	float m_Speed = 1.0f;
	float m_Intensity = 0.15f;
	float m_WobbleAmount = 0.08f;
	float m_WobbleSpeed = 0.6f;

	float m_BreatheAmount = 0.2f;
	float m_BreatheSpeed = 0.35f;

	static const int TEX_SIZE = 96; 
	std::vector<unsigned char> m_TexData; 

	IGraphics::CTextureHandle m_TintTexture[2];
	bool m_TexValid[2] = {false, false};
	int m_ActiveTexture = 0; 

	float m_LastUpdateTime = 0.0f;
	static constexpr float UPDATE_INTERVAL = 1.0f / 20.0f; 

	float m_LastModeSwitch = 0.0f;
	int m_CurrentMode = 0;
	int m_NextEffectMode = 0;
	float m_TransitionProgress = 1.0f;

	bool m_TablesReady = false;
	float m_AxisLut[TEX_SIZE]; 
	std::vector<float> m_RadiusLut;
	std::vector<float> m_AngleLut;
	void BuildLookupTables();

	void RenderTintOverlay();
	void UpdateTintTexture();
	void DestroyTexture();

	void HSVtoRGB(float h, float s, float v, float &r, float &g, float &b);

	static void ConToggle(IConsole::IResult *pResult, void *pUserData);
	static void ConSpeed(IConsole::IResult *pResult, void *pUserData);
	static void ConIntensity(IConsole::IResult *pResult, void *pUserData);
	static void ConWobble(IConsole::IResult *pResult, void *pUserData);
	static void ConWobbleSpeed(IConsole::IResult *pResult, void *pUserData);
	static void ConBreathe(IConsole::IResult *pResult, void *pUserData);

public:
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnShutdown() override; 
	void OnRender() override;

	bool IsEnabled() const { return m_Enabled; }

	float ZoomModifier() const;
};

#endif
