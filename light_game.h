#pragma once
#include "light.h"

class GameLight{
private:
	static AmbientLight m_ambientLight;
	static PointLight* m_pointLight;

public:
	static void Init();
	static void Finalize();
	static void Update();
	static void Draw();
};
