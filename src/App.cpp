#include "App.h"
#include <iostream>
#include <sstream>
#include "util.h"
#include "Enemy.h"

App::App(int argc, const char **argv):
	score(0)
{
	this->parseArgs(argc, argv);
	this->initOSL();
	
	uRandomInit(time(NULL));
	srand(time(NULL)); // In case I need it... I sure hope not.
	
	player = new Player(this, this->loadImagePNG("img/ship.png"), this->loadImagePNG("img/beam.png"));
	player->move(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	
	bgImage = this->loadImagePNG("img/bg.png");
	bgImage->x = 0;
	bgImage->y = 0;
	
	hpImage = this->loadImagePNG("img/heart.png", OSL_PF_4444);
	
	bigFont = oslLoadFontFile(FONT_PATH_BIG_SANS);
	smallFont = oslLoadFontFile(FONT_PATH_SMALL_SANS);
}

App::~App()
{
	
}

void App::parseArgs(int argc, const char **argv)
{
	// Extract the pathname from the executable name in argv[0]
	{
		int len = strrchr(argv[0], '/') - argv[0];
		char *cpy = strndup(argv[0], len);
		this->appDir = cpy;
		free(cpy);
	}
}

void App::initOSL()
{
	// -- OSLib
	// Init OSLib with default callbacks
	oslInit(0);
	// Don't crash if we can't load a resource
	//oslSetQuitOnLoadFailure(1);
	
	// -- Input
	// We want the D-Pad to repeat if you hold it down, makes it easier to scroll through stuff
	// Start repeating after 40 frames, then repeat every 10th
	oslSetKeyAutorepeatMask(OSL_KEYMASK_UP|OSL_KEYMASK_RIGHT|OSL_KEYMASK_DOWN|OSL_KEYMASK_LEFT);
	oslSetKeyAutorepeatInit(40);
	oslSetKeyAutorepeatInterval(10);
	// Map the analog stick to the D-Pad with 80/127 sensitivty
	// This is oslib's recommended default, considering the analog stick's dead zone
	oslSetKeyAnalogToDPad(80);
	
	// -- Graphics
	// Init graphics with 16bit (for now) and double buffering
	// Might bump it up to 32bit later if it looks too bad
	oslInitGfx(OSL_PF_5551, true);
	
	// -- Fonts
	oslIntraFontInit(INTRAFONT_CACHE_MED);
	
	// -- Synchronization
	oslSetFrameskip(1);
	oslSetMaxFrameskip(4);
}

void App::run()
{
	// True if the last frame was in late, in which case we should drop the next frame
	bool lastFrameWasLate = false;
	
	while(!osl_quit)
	{
		// Update state
		this->tick();
		
		// If the last frame was /not/ late, draw the next one
		// Otherwise, drop frames until we catch up
		if(!lastFrameWasLate)
			this->draw();
		
		// This takes care of timing and determining when to drop a frame
		oslEndFrame();
		lastFrameWasLate = oslSyncFrame();
	}
}

void App::tick()
{
	// Check the controller for new inputs; this updates osl_pad
	oslReadKeys();
	
	if(this->state == AppStatePlaying)
	{
		if(osl_pad.pressed.start)
			this->state = AppStatePaused;
		else
		{
			// Let objects update their states
			player->tick();
			std::deque<Enemy*>::iterator it = enemies.begin();
			while(it != enemies.end())
			{
				Enemy *enemy = *it;
				enemy->tick();
				
				if(enemy->x + enemy->width() < 0)
				{
					it = enemies.erase(it);
					delete enemy;
				}
				else ++it;
			}
			
			// Occasionally spawn enemies
			if(uRandomBool(kEnemySpawnRate))
			{
				Enemy *enemy = new Enemy(this, this->loadImagePNG("img/enemy1.png"), this->loadImagePNG("img/rocket.png"));
				enemy->move(SCREEN_WIDTH + enemy->width(), uRandomUIntBetween(0, SCREEN_HEIGHT - enemy->height()));
				enemy->putInMotion(-uRandomFloatBetween(kEnemyMinSpeed, kEnemyMaxSpeed), 0);
				enemies.push_back(enemy);
			}
		}
	}
	else if(this->state == AppStatePaused)
	{
		if(osl_pad.pressed.start)
			this->state = AppStatePlaying;
	}
	else if(this->state == AppStateGameOver)
	{
		
	}
}

void App::draw()
{
	// Prepare to draw
	oslStartDrawing();
	
	// Clear the buffer to prevent ghosting
	oslClearScreen(RGB(0, 0, 0));
	
	// Draw a background
	oslDrawImage(bgImage);
	
	// Let objects draw themselves
	for(std::deque<Enemy*>::iterator it = enemies.begin(); it != enemies.end(); it++)
		(*it)->draw();
	player->draw();
	
	// Draw HUD
	this->drawHUD();
	
	// Draw overlays for certain states
	if(state == AppStatePaused)
	{
		const char *msg1 = "PAUSED";
		const char *msg2 = "START = Resume";
		
		oslSetFont(this->bigFont);
		oslDrawString((SCREEN_WIDTH - oslGetStringWidth(msg1))/2, SCREEN_HEIGHT/2 - this->bigFont->charHeight, msg1);
		
		oslSetFont(this->smallFont);
		oslDrawString((SCREEN_WIDTH - oslGetStringWidth(msg2))/2, (SCREEN_HEIGHT/2) + 5, msg2);
	}
	else if(state == AppStateGameOver)
	{
		
	}
	
	// Release resources grabbed in oslStartDrawing()
	oslEndDrawing();
}

void App::drawHUD()
{
	oslSetFont(this->bigFont);
	
	// Score
	{
		std::stringstream scoreStream;
		scoreStream << score;
		std::string scoreString = scoreStream.str();
		oslDrawString(SCREEN_WIDTH - oslGetStringWidth(scoreString.c_str()) - 10, 10, scoreString.c_str());
	}
	
	// Lives
	{
		for(int i = 0; i < player->hp; i++)
		{
			int x = 5 + (15 + 10)*i;
			int y = 0;
			oslDrawImageXY(this->hpImage, x, y);
		}
	}
}

OSL_IMAGE* App::loadImagePNG(std::string filename, int format, int flags)
{
	std::map<std::string, OSL_IMAGE*>::iterator it = images.find(filename);
	if(it == images.end())
	{
		std::cout << "Loading image '" << filename << "'... (" << std::hex << flags << std::dec << ", " << format << ")" << std::endl;
		char *filenameCStrDup = strdup(filename.c_str());
		OSL_IMAGE *img = oslLoadImageFilePNG(filenameCStrDup, flags, format);
		free(filenameCStrDup);
		
		images.insert(std::pair<std::string, OSL_IMAGE*>(filename, img));
		return img;
	}
	
	return it->second;
}

void App::unloadImage(std::string filename)
{
	std::map<std::string, OSL_IMAGE*>::iterator it = images.find(filename);
	if(it != images.end())
	{
		std::cout << "Unloading image '" << filename << "'..." << std::endl;
		oslDeleteImage(it->second);
		images.erase(it);
	}
}

void App::unloadAllImages()
{
	std::cout << "Unloading all images... Total: " << images.size() << std::endl;
	for(std::map<std::string, OSL_IMAGE*>::iterator it = images.begin(); it != images.end(); it++)
	{
		std::cout << "-> " << it->first << std::endl;
		oslDeleteImage(it->second);
	}
	images.clear();
	std::cout << "Done." << std::endl;
}
