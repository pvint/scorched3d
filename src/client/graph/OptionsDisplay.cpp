////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2003
//
//    This file is part of Scorched3D.
//
//    Scorched3D is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Scorched3D is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Scorched3D; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

#include <graph/OptionsDisplay.h>
#include <common/Defines.h>
#include <stdio.h>

OptionsDisplay *OptionsDisplay::instance_ = 0;

OptionsDisplay *OptionsDisplay::instance()
{
	if (!instance_)
	{
		instance_ = new OptionsDisplay;
	}

	return instance_;
}

OptionsDisplay::OptionsDisplay() :
	detailTexture_(options_, "DetailTexture",
		"Use detail textures on the landscape."
		"Adds the texture seen when very close to the landscape."
		"Requires multi-texturing", RWAccess, true),
	hideFinalScore_(options_, "HideFinalScore",
		"Hide the score dialog that is automatically shown after shots", RWAccess | NoRestore, false),
	depricatedUniqueUserId_(options_, "UniqueUserId",
		"The unique string given by this client to any servers to uniquely identify itself."
		"Used for server stats and logging (confidentially)", RAccess | OptionEntry::DataDepricated | NoRestore, ""),
	hostDescription_(options_, "HostDescription",
		"The description of this host given to any servers for stats.", RAccess | NoRestore, ""),
	onlineUserIcon_(options_, "OnlineUserIcon",
		"The players icon, must be PNG 32x32.", RAccess | NoRestore, ""),
	buyTab_(options_, "BuyTab",
		"The default buy tab", RWAccess | NoRestore, ""),
	depricatedRoamVarianceStart_(options_, "RoamVarianceStart",
		"The minimum variance to allow", RWAccess | OptionEntry::DataDepricated, 2),
	depricatedRoamVarianceRamp_(options_, "RoamVarianceRamp",
		"The variance ramping for each distance unit", RWAccess | OptionEntry::DataDepricated, 10),
	depricatedRoamVarianceTank_(options_, "RoamVarianceTank",
		"The variance difference for ROAM areas with tanks on them", RWAccess | OptionEntry::DataDepricated, 50),
	onlineUserName_(options_, "OnlineUserName",
		"The player name that will be used for all online games.", RAccess | NoRestore, "Player"),
	onlineTankModel_(options_, "OnlineTankModel",
		"The tank model that will be used for all online games.", RAccess | NoRestore, ""),
	onlineColor_(options_, "OnlineColor",
		"The tank color that will be used for all online (non-team) games.", RAccess | NoRestore, Vector::getNullVector(), true),
	explosionParts_(options_, "ExplosionParts",
		"How many explosion clouds are drawn", RAccess, 8, 0, 10, 1),
	explosionSubParts_(options_, "ExplosionSubParts",
		"How many explosion sub clouds are drawn", RAccess, 4, 0, 5, 1),
	dialogSize_(options_, "DialogSize",
		"How large the on screen dialogs and menus are.", RAccess, 2, 0, 3, 1),
	tankModelSize_(options_, "TankModelSize",
		"The percentage size of the tank models", RWAccess, 100),
	depricatedMaxModelTriPercentage_(options_, "MaxModelTriPercentage",
		"", RAccess | OptionEntry::DataDepricated, 100, 50, 100, 1),
	explosionParticleMult_(options_, "ExplosionParticleMult",
		"The number of particles that each explosion will create (relative to explosion size)", RAccess, 20, 0, 100, 10),
	depricatedDayTime_(options_, "DayTime",
		"", RWAccess | OptionEntry::DataDepricated, 2),
	depricatedSunXYAng_(options_, "SunXYAng",
		"", RAccess | OptionEntry::DataDepricated, 110),
	depricatedSunYZAng_(options_, "SunYZAng",
		"", RAccess | OptionEntry::DataDepricated, 25),
	toolTipTime_(options_, "ToolTipTime", 
		"The milliseconds before showing a tool tip.", RWAccess, 100),
	toolTipSpeed_(options_, "ToolTipSpeed", 
		"The speed at which a tool tip will fade in.", RWAccess, 6),
	framesPerSecondLimit_(options_, "FramesPerSecondLimit",
		"The maximum frame rate that the game will run at", RWAccess, 250),
	brightness_(options_, "Brightness", 
		"The game screen brightness (gamma).", RAccess, 10, 3, 40, 1),
	fullScreen_(options_, "FullScreen", 
		"Run the game in a full screen mode.", RAccess | NoRestore, true),
	moreRes_(options_, "MoreRes",
		"Show more screen resolutions in the drop down.  By default only hardware supported modes are shown.", RAccess, false),
	depricatedFullClear_(options_, "FullClear", 
		"Completely clear the screen before drawing each frame", RWAccess | OptionEntry::DataDepricated, true),
	invertElevation_(options_, "InvertUpDownKeys",
		"Invert/reverse the tanks up/down elevation directions.", RWAccess | NoRestore, false),
	invertMouse_(options_, "InvertMouse",
		"Invert/reverse the mouses y-axis when rotating camera.", RWAccess | NoRestore, false),
	saveWindowPositions_(options_, "SaveWindowPositions",
		"Save the positions of all the onscreen windows.", RWAccess | NoRestore, true),
	swapYAxis_(options_, "SwapYAxis",
		"Remaps mouse pointer from top of window to the bottom and vice-versa (MAC/OSX)", RWAccess | NoRestore, false),
	sideScroll_(options_, "SideScroll",
		"Allows the user to scroll the viewport moving the mouse to the sides of the screen", RWAccess | NoRestore, true),
	storePlayerCamera_(options_, "StorePlayerCamera",
		"Stores the camera position for each player and resets to that position on thier turn", RWAccess | NoRestore, false),
	drawPlayerNames_(options_, "DrawPlayerNames",
		"Draw the names above the tanks", RWAccess, true),
	drawPlayerIcons_(options_, "DrawPlayerIcons",
		"Draw the icons above the tanks", RWAccess, true),
	smoothLines_(options_, "SmoothLines",
		"Smooth/AA the dialog lines", RWAccess, true),
	drawPlayerSight_(options_, "DrawPlayerSight",
		"Draw the aiming sight infront of the tanks", RWAccess, true),
	drawDistance_(options_, "DrawDistance",
		"The distance at which objects will be culled", RWAccess, 25000.0f),
	drawDistanceFade_(options_, "DrawDistanceFade",
		"The distance before the draw distance at which objects will be faded", RWAccess, 10000.0f),
	oldSightPosition_(options_, "OldSightPosition",
		"Draw the aiming sight aligned with the model and not the shot", RWAccess, false),
	drawPlayerColor_(options_, "DrawPlayerColor",
		"Draw the player color triangle over the tank", RWAccess, true),
	drawPlayerHealth_(options_, "DrawPlayerHealth",
		"Draw the health bars above the tank", RWAccess, true),
	depricatedFirstTimePlayed_(options_, "FirstTimePlayed",
		"Is this the first time the user has played Scorched3D", OptionEntry::DataDepricated, true),
	lastVersionPlayed_(options_, "LastVersionPlayed",
		"What was the last version of scorched3d played", RWAccess | NoRestore, ""),
	hideMenus_(options_, "HideMenus",
		"Should the menu title bars always be visible", RWAccess, false),
	noGLTexSubImage_(options_, "NoGLTexSubImage",
		"Do not use texture area replacing.  Required to be able to scorch the ground.", RAccess, false),
	noGLShaders_(options_, "NoGLShaders",
		"Do not use shaders.", RAccess, false),
	noPlanDraw_(options_, "NoPlanDraw", 
		"Do not show any drawings from other players on the plan window.", RWAccess, false),
	noFog_(options_, "NoFog", 
		"Do not use any fog extensions.", RWAccess, false),
	noGLExt_(options_, "NoGLExt", 
		"Do not use any OpenGL extensions.  Turn off to disable any extra OpenGL features.", RAccess, false),
	noGLMultiTex_(options_, "NoGLMultiTex", 
		"Only use one texture for all models and the landscape.", RAccess, false),
	noGLCompiledArrays_(options_, "NoGLCompiledArrays",
		"Do not compile vertex arrays.", RAccess, false),
	noGLEnvCombine_(options_, "NoGLEnvCombine",
		"Only use one texture on the landscape.", RAccess, false),
	noGLCubeMap_(options_, "NoGLCubeMap",
		"Draw the water without using cubemap relfections.", RAccess, false),
	noGLSphereMap_(options_, "NoGLSphereMap",
		"Draw the water without using sphere map relfections.", RAccess, false),
	noGLHardwareMipmaps_(options_, "NoGLHardwareMipmaps",
		"Generate texture LOD in software only.", RAccess, false),
	soundChannels_(options_, "SoundChannels", 
		"Number of sound channels to mix.", RAccess, 8),
	noSound_(options_, "NoSound", 
		"Do not play any sound.", RWAccess, false),
	noMusic_(options_, "NoMusic", 
		"Do not play any music.", RWAccess, false),
	noProgressBackdrop_(options_, "NoProgressBackdrop", 
		"Do capture a screen shot and use it as the progress backdrop", RWAccess, false),
	depricatedNoAmbientSound_(options_, "NoAmbientSound", 
		"Do not play any ambient sounds.", OptionEntry::DataDepricated, false),
	depricatedNoBoidSound_(options_, "NoBoidSound", 
		"Do not play any sounds from boids.", OptionEntry::DataDepricated, false), 
	noShadows_(options_, "NoShadows", 
		"Do not draw real-time shadows.", RWAccess, false), 
	noGLShadows_(options_, "NoGLShadows", 
		"Do not draw GL shadow map shadows.", RAccess, false), 
	noGLObjectShadows_(options_, "NoGLObjectShadows", 
		"Do not draw GL shadow map shadows for objects (trees/tanks).", RWAccess, true), 
	noSimulateParticles_(options_, "NoParticleSimulate", 
		"Do not use custom simulations for the  particles.", RWAccess, false), 
	noDrawParticles_(options_, "NoParticleDraw", 
		"Do not draw any particles.", RWAccess, false), 
	noTrees_(options_, "NoTrees",
		"Do not draw any trees", RWAccess, false),
	lowTreeDetail_(options_, "LowTreeDetail",
		"Only use low detail trees.  Faster.", RWAccess, false),
	depricatedNoWaves_(options_, "NoWaves",
		"Do not draw the moving shore waves.", OptionEntry::DataDepricated, false),
	noDepthSorting_(options_, "NoDepthSorting",
		"Do not depth sort sprites.", RWAccess, false),
	clientLogToFile_(options_, "ClientLogToFile",
		"Client logs to file", RAccess, false),
	clientLogState_(options_, "ClientLogState",
		"Client logs state (0 is off)", RAccess, 0),
	validateServerIp_(options_, "ValidateServerIp",
		"Checks if the server ip address matches the published address", RAccess, true),
	drawLines_(options_, "DrawLines", 
		"Do not fill in the landscape", DebugOnly | RWAccess, false),
	drawNormals_(options_, "DrawNormals", 
		"Show landscape normals on the landscape", RWAccess, false),
	drawGraphicalShadowMap_(options_, "DrawGraphicalShadowMap", 
		"Shows the depth map used for shadowing", RWAccess, false),
	drawCollisionGeoms_(options_, "DrawCollisionGeoms",
		"Show object collision geoms", DebugOnly | RWAccess, false),
	drawCollisionSpace_(options_, "DrawCollisionSpace",
		"Show object collision space", DebugOnly | RWAccess, false),
	drawBoundingSpheres_(options_, "DrawBoundingSpheres", 
		"Show landscape bounding spheres on the landscape", DebugOnly | RWAccess, false),
	depricatedDrawShipPaths_(options_, "DrawShipPaths", 
		"Show paths for the ships", OptionEntry::DataDepricated, false),
	frameTimer_(options_, "FrameTimer",
		"Show the current number of frames per second (FPS)", RWAccess | NoRestore, false),
	noSkins_(options_,"NoTankSkins", 
		"Do not texture the tank models.", RAccess, false),
	drawWater_(options_,"DrawWater", 
		"Draw the water", RWAccess, true),
	drawLandscape_(options_, "DrawLandscape",
		"Draw the landscape", DebugOnly | RWAccess, true),
	drawSurround_(options_, "DrawSurround",
		"Draw the surround", RWAccess, true),
	drawMovement_(options_, "DrawMovement",
		"Draw the movement paths", DebugOnly | RWAccess, false),
	noWaterMovement_(options_, "NoWaterMovement",
		"Draw simpler less detailed water", RWAccess, false),
	noWaterReflections_(options_, "NoWaterReflections",
		"Draw reflections in the water", RAccess, false),
	noWaterWaves_(options_, "NoWaterWaves",
		"Draw water waves (breakers)", RWAccess, false),
	noWaterLOD_(options_, "NoWaterLOD",
		"Draw water at minimum detail", RWAccess, false),
	noSkyLayers_(options_, "NoSkyLayers",
		"Only draw one sky layer.", RWAccess, false),
	noSkyMovement_(options_, "NoSkyMovement",
		"Do not animate the sky", RWAccess, false),
	depricatedNoROAM_(options_, "NoROAM", 
		"Do not use ROAM algorithm", RWAccess | OptionEntry::DataDepricated, false),
	depricatedNoBOIDS_(options_,"NoBOIDS", 
		"Do not use BOIDS", OptionEntry::DataDepricated, false),
	depricatedNoShips_(options_,"NoShips", 
		"Do not use ships", OptionEntry::DataDepricated, false),
	noPrecipitation_(options_, "NoPrecipitation",
		"Do not draw precipitation", RWAccess, false),
	depricatedNoTessalation_(options_,"NoTessalation", 
		"Do not use ROAM tessalation algorithm", OptionEntry::DataDepricated, false),
	noVBO_(options_,"NoVBO", 
		"Do not use Vertex Buffer Objects (if avaialable)", RWAccess, false),
	noWaterBuffers_(options_,"NoWaterBuffers", 
		"Do not use Vertex Buffers for water (if avaialable)", RAccess, false),
	depricatedNoCg_(options_,"NoCg", 
		"Do not use vertex or pixel shaders (if avaialable)", RWAccess | OptionEntry::DataDepricated, true),
	noModelLOD_(options_, "NoModelLOD",
		"Do not use LOD optimizations for models", RWAccess, true),
	noModelLighting_(options_, "NoModelLighting",
		"Do not use dynamic lighting calculations for models", RWAccess, false),
	useLandscapeTexture_(options_, "LandscapeTexture",
		"Texture the landscape", RWAccess, true),
	noLenseFlare_(options_,"NoLenseFlare", 
		"Do not show the full lense flare effect", RWAccess, true),
	softwareMouse_(options_,"SoftwareMouse", 
		"Use a software mouse pointer.  Useful if mouse clicks are not aligned.", RWAccess, false),
	depricatedUseHex_(options_,"UseHexidecimal", 
		"Show the tank angles and amounts in hex", RWAccess | OptionEntry::DataDepricated, false),
	soundVolume_(options_, "SoundVolume",
		"The master volume. Max = 128, Min = 0", RAccess | NoRestore, 128, 0, 128, 1),
	ambientSoundVolume_(options_, "AmbientSoundVolume",
		"The ambient sound effect volume. Max = 128, Min = 0", RAccess | NoRestore, 128, 0, 128, 1),
	musicVolume_(options_, "MusicVolume",
		"The music effect volume. Max = 128, Min = 0", RAccess | NoRestore, 128, 0, 128, 1),
	antiAlias_(options_, "AntiAlias",
		"Use anti aliasing", RAccess, 0, 0, 4, 1),
	texSize_(options_, "TexureSize", 
		"The texture details setting.  Lower is faster.", RAccess, 1, 0, 2, 1),
	bannerRowsDepricated_(options_, "BannerRows",
		"", RAccess | OptionEntry::DataDepricated, 6),
	tankDetail_(options_, "TankDetail", 
		"The tank details setting.  Lower is faster but shows less tank models.", RAccess, 2, 0, 2, 1),
	effectsDetail_(options_, "EffectsDetail",
		"The number of effects allowed at once.",  RAccess, 1, 0, 2, 1),
	screenWidth_(options_, "ScreenWidth", 
		"The window width to use (in pixels)", RAccess, 800),
	screenHeight_(options_, "ScreenHeight",
		"The window height to use (in pixels)", RAccess, 600),
	depthBufferBits_(options_, "DepthBufferBits",
		"The number of bits requested for the depth buffer", RAccess, 24),
	doubleBuffer_(options_, "DoubleBuffer",
		"Use double buffering", RAccess, true),
	colorComponentSize_(options_, "ColorComponentSize",
		"The number of bits to use for each of the RGBA components (0 = use default)", RAccess, 0),
	bitsPerPixel_(options_, "BitsPerPixel",
		"The number of bits per pixel to use for the display (0 = current display bbp)", RAccess, 0),
	showContextHelp_(options_, "ShowContextHelp",
		"Show in game help tooltips for items that have it", RWAccess, true),
	showContextInfo_(options_, "ShowContextInfo",
		"Show in game information tooltips for items that have it", RWAccess, true),
	sortAccessories_(options_, "SortAccessories",
		"Sort accessories alphabetically by name before displaying", RWAccess, false),
	focusPause_(options_, "FocusPause",
		"Pause the graphics display when mouse leaves window.", RWAccess, true)

{
}

OptionsDisplay::~OptionsDisplay()
{	
}

bool OptionsDisplay::writeOptionsToFile()
{
	std::string path = S3D::getSettingsFile("display.xml");

	// Check the options files are writeable
	FILE *checkfile = fopen(path.c_str(), "a");
	if (!checkfile)
	{
		S3D::dialogMessage(
			"Scorched3D", S3D::formatStringBuffer(
			"Warning: Your display settings file (%s) cannot be\n"
			"written to.  Your settings will not be saved from one game to the next.\n\n"
			"To fix this problem correct the permissions for this file.",
			path.c_str()));
	}
	else fclose(checkfile);

	if (!OptionEntryHelper::writeToFile(options_, path)) return false;
	return true;
}

bool OptionsDisplay::readOptionsFromFile()
{
	std::string path = S3D::getSettingsFile("display.xml");

	if (!OptionEntryHelper::readFromFile(options_, path))
	{
		S3D::dialogMessage(
			"Scorched3D", S3D::formatStringBuffer(
			"Warning: Your display settings file (%s) cannot be\n"
			"read.  This may be because it was create by an out of date version of Scorched3D.\n"
			"If this is the case it can be safely deleted.",
			path.c_str()));
		return false;
	}

	return true;
}

void OptionsDisplay::loadDefaultValues()
{
        std::list<OptionEntry *>::iterator itor;
        for (itor = options_.begin();
                itor != options_.end();
                itor++)
	{
                OptionEntry *entry = (*itor);
		if (!(entry->getData() & NoRestore))
		{
			entry->setValueFromString(entry->getDefaultValueAsString());
		}
	}
}

void OptionsDisplay::loadSafeValues()
{
	std::string path = S3D::getDataFile("data/safedisplay.xml");
	OptionEntryHelper::readFromFile(options_, path);
}

void OptionsDisplay::loadMediumValues()
{
	std::string path = S3D::getDataFile("data/mediumdisplay.xml");
	OptionEntryHelper::readFromFile(options_, path);
}

void OptionsDisplay::loadFastestValues()
{
	std::string path = S3D::getDataFile("data/fastestdisplay.xml");
	OptionEntryHelper::readFromFile(options_, path);
}

