#include "Enemy.h"

Enemy::Enemy(App *app, OSL_IMAGE *image, OSL_IMAGE *bulletImage):
	Thing(app, image, bulletImage),
	hp(1)
{
	
}

Enemy::~Enemy()
{
	
}
