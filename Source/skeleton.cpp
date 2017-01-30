#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::mat3;

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;
vector<vec3> stars( 1000 );

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update( vector<vec3>& stars );
void Draw( vector<vec3>& stars);
void Interpolate( vec3 a, vec3 b, vector<vec3>& result );

int main( int argc, char* argv[] )
{

	for ( int i=0; i<stars.size(); ++i )
	{
		float randx = -1 + 2*(float(rand()) / float(RAND_MAX));	// random number between -1 and 1
		float randy = -1 + 2*(float(rand()) / float(RAND_MAX));	// random number between -1 and 1
		float randz =  float(rand()) / float(RAND_MAX); // random number between 0 and 1

		cout << stars[1].z << endl;
		stars[i].x = randx;
		stars[i].y = randy;
		stars[i].z = randz;
	}

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	while( NoQuitMessageSDL() )
	{
		Update( stars );
		Draw( stars );
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update( vector<vec3>& stars )
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

	for( int s=0; s<stars.size(); ++s )
	{
		float v = 0.0001;
		stars[s].z -= v*dt;
		if( stars[s].z <= 0 )
			stars[s].z += 1;
		if( stars[s].z > 1 )
			stars[s].z -= 1;
	}
}

void Draw( vector<vec3>& stars )
{
	SDL_FillRect( screen, 0, 0 );

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	vec3 topLeft(1,0,0); // red
	vec3 topRight(0,0,1); // blue
	vec3 bottomRight(0,1,0); // green
	vec3 bottomLeft(1,1,0); // yellow

	vector<vec3> leftSide( SCREEN_HEIGHT );
	vector<vec3> rightSide( SCREEN_HEIGHT );
	Interpolate( topLeft, bottomLeft, leftSide );
	Interpolate( topRight, bottomRight, rightSide );

	for( int y=0; y<SCREEN_HEIGHT; ++y )
	{
		vector<vec3> rowColors( SCREEN_WIDTH );	// Interpolate for each row
		Interpolate( leftSide[y], rightSide[y], rowColors );
		for( int x=0; x<SCREEN_WIDTH; ++x )
		{
			PutPixelSDL( screen, x, y, rowColors[x] );
		}
	}

	// float focal,centerx,centery;
	// focal = SCREEN_HEIGHT/2;
	// centerx = SCREEN_WIDTH/2;
	// centery = SCREEN_HEIGHT/2;
	// vec3 color ( 1.0, 1.0, 1.0 );
	//
	// for( size_t s=0; s<stars.size(); ++s )
	// {
	// 	vec3 color = 0.2f * vec3(1.0,1.0,1.0) / (stars[s].z*stars[s].z);
	// 	float u = focal*(stars[s].x/stars[s].z) + centerx;
	// 	float v = focal*(stars[s].y/stars[s].z) + centery;
		// PutPixelSDL( screen, u, v, color );
	// }

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

void Interpolate( vec3 a, vec3 b, vector<vec3>& result)
{
	float size,intervalx,intervaly,intervalz;
	size = result.size()-1;
	intervalx = (b.x - a.x)/size;
	intervaly = (b.y - a.y)/size;
	intervalz = (b.z - a.z)/size;

	for ( int i=0; i<result.size(); ++i)
	{
		result[i].x = a.x + intervalx*i;
		result[i].y = a.y + intervaly*i;
		result[i].z = a.z + intervalz*i;
	}
}
