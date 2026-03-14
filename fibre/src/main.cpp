#include "Wire.h"

#include "RayTracingLayer.h"

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#if !defined(WR_DIST) || !defined(WR_PLATFORM_WINDOWS)

int main()
{
	WR_SETUP_LOG({ &std::cout }, { "fibre.log" }, "%c[%H:%M:%S]%c %m");

	wire::ApplicationDesc desc{};
	desc.WindowTitle = "fibre";
	desc.WindowWidth = 900;
	desc.WindowHeight = 600;

	wire::Application app(desc);
	app.pushLayer(new fibre::RayTracingLayer());

	app.run();

	return 0;
}

#else

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	WR_SETUP_LOG({ &std::cout }, { "fibre.log" }, "%c[%H:%M:%S]%c %m");

	wire::ApplicationDesc desc{};
	desc.WindowTitle = "fibre";
	desc.WindowWidth = 1280;
	desc.WindowHeight = 720;

	wire::Application app(desc);

	app.run();

	return 0;
}

#endif
