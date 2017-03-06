#define _USE_MATH_DEFINES

#include <cmath>
#include <iostream>
#include <algorithm>
#include <math.h>
#include <glm/glm.hpp>
#include <X11/Xlib.h>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec3;
using glm::mat3;

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES AND STRUCTS                                                        */

const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 400;
SDL_Surface* screen;
int t;

vector<Triangle> triangles;

struct Intersection
{
	vec3 position;
	float distance;
	int triangleIndex;
};

vec3 cameraPos( 0.f, 0.f, -4.f );
vec3 ceilingLight( 0, -0.5, -0.7 );
vec3 ceilingLColor = 10.f * vec3( 1, 0.1, 0.1 );
vec3 secondLight( -0.99, 0, -0.25);
vec3 secondLColor = 10.f * vec3( 0.1, 1, 0.1);
vec3 thirdLight( 0.99, 0, -0.75);
vec3 thirdLColor = 10.f * vec3( 0.1, 0.1, 1);


vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );
float reflectVal = 0.4f;
const int GLOBDEPTH = 2;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw();
void Rotate( mat3& R, float yaw );
void Interpolate( vec3 a, vec3 b, vector<vec3>& result );
vec3 DirectLight( const Intersection& i, vec3 lightPos, vec3 lightColor );
vec3 ReflectedLight( vec3& rColor, const Intersection& i, int depth );
bool ClosestIntersection( vec3 start, vec3 dir, Intersection& closestIntersection );
// bool distanceCompare( const Intersection &x, const Intersection &y );

int main( int argc, char* argv[] )
{
	// mat3 R;
	// float yaw = 0.0;
	XInitThreads();

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	LoadTestModel( triangles );

	while( NoQuitMessageSDL() )
	{
		// Update();
		Draw();
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
	mat3 R;
	float yaw = 0.0;
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	// cout << "Render time: " << dt << " ms." << endl;

	Uint8* keystate = SDL_GetKeyState( 0 );
	if( keystate[SDLK_UP] )
	{
		cameraPos.z += 0.3;
	}
	if( keystate[SDLK_DOWN] )
	{
		cameraPos.z -= 0.3;
	}
	if( keystate[SDLK_LEFT] )
	{
		yaw += M_PI/15.f;
		Rotate( R, yaw );
		// cout << "x: " << cameraPos.x << "\ny: " << cameraPos.y<< "\nz: " << cameraPos.z << endl;
	}
	if( keystate[SDLK_RIGHT] )
	{
		yaw -= M_PI/15.f;
		Rotate( R, yaw );
		// cout << "x: " << cameraPos.x << "\ny: " << cameraPos.y<< "\nz: " << cameraPos.z << endl;
	}
}

void Rotate( mat3& R, float yaw )
{
	R[0].x = cos( yaw );
	R[0].z = - sin( yaw );
	R[2].x = sin( yaw );
	R[2].z = cos( yaw );

	cameraPos = R * cameraPos;
}

void Draw()
{
	SDL_FillRect( screen, 0, 0 );

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	Intersection closestIntersection;
	float focalLength,centerx,centery;
	vec3 d, color;
	// vec3 tempcol = vec3(0,0,0);
	// bool hasIntercept;
	focalLength = SCREEN_HEIGHT*1.5;
	centerx = SCREEN_WIDTH/2;
	centery = SCREEN_HEIGHT/2;

	#pragma omp parallel for
	for( int y=0; y<SCREEN_HEIGHT; ++y )
	{
		for( int x=0; x<SCREEN_WIDTH; ++x )
		{
			vec3 avColor( 0, 0, 0 );

			// shoot multiple rays for anti-alias
			for( int s = 0; s < 3; s++ )
			{
				for( int t = 0; t < 3; t++ )
				{
					d = vec3( x-centerx+s, y-centery+t, focalLength );
					// hasIntercept = ClosestIntersection( cameraPos, d, triangles, closestIntersection);
					if( ClosestIntersection( cameraPos, d, closestIntersection ) )
					{
						// vec3 triColor = triangles[closestIntersection.triangleIndex].color;
						color = triangles[closestIntersection.triangleIndex].color *
							( DirectLight( closestIntersection, ceilingLight, ceilingLColor ) +
								DirectLight( closestIntersection, secondLight, secondLColor ) +
								DirectLight( closestIntersection, thirdLight, thirdLColor ) +
								indirectLight );
						avColor += color;
					}
					else
						PutPixelSDL( screen, x, y, vec3(0,0,0) );
				}
			}
			avColor.x = avColor.x/9.f;
			avColor.y = avColor.y/9.f;
			avColor.z = avColor.z/9.f;
			PutPixelSDL( screen, x, y, color );
		}
	}


	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

vec3 DirectLight( const Intersection& i, vec3 lightPos, vec3 lightColor )
{
	vec3 r, l_pos, l_dir, normal, B, color;
	float l_dist;
	Intersection nearestObject;

	for( int s = 0; s < 5; s++ )
	{
		for( int t = 0; t < 5; t++ )
		{
			l_pos = vec3( lightPos.x + 0.001*s, lightPos.y, lightPos.z + 0.001*t );
			r = l_pos - i.position;
			l_dist = glm::length( r );
			l_dir = glm::normalize( r );
			normal = triangles[i.triangleIndex].normal;

			ClosestIntersection( i.position+l_dir*0.0001f, l_dir, nearestObject );
			if( nearestObject.distance < l_dist )
			{
				color = vec3( 0.0, 0.0, 0.0 );
				return color;
			}

			B = (lightColor / (float)(4*M_PI*pow(l_dist, 2))) / (float)25;
			color += B * std::max( glm::dot( normal, l_dir ), 0.f );
		}
	}
	return color;
}

// TODO: Get reflection working
vec3 ReflectedLight( vec3& rColor, const Intersection& i, int depth )
{
	vec3 I, N, R, Rdir;
	Intersection nearestObject;

	if ( depth <= GLOBDEPTH )
	{
		I = i.position;
		N = triangles[nearestObject.triangleIndex].normal;

		R = I - 2 * glm::dot( N, I ) * N;
		Rdir = glm::normalize( R );

		ClosestIntersection( R, Rdir, nearestObject );
		rColor = 0.3f * triangles[nearestObject.triangleIndex].color;
		return ReflectedLight( rColor, nearestObject, depth + 1 );
	}

	return 0.3f * rColor;
}

// bool distanceCompare( const Intersection &x, const Intersection &y )
// {
// 	return ( 0 < x.distance < y.distance);
// }

bool ClosestIntersection( vec3 start, vec3 dir, Intersection& closestIntersection )
{
	bool hasIntercept = false;
	closestIntersection.distance = numeric_limits<float>::max();

	for( int s=0; s<triangles.size(); ++s)
	{
		vec3 e1 = triangles[s].v1 - triangles[s].v0;
		vec3 e2 = triangles[s].v2 - triangles[s].v0;
		vec3 b = start - triangles[s].v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = inverse( A ) * b;
		// cout << s << ", ";
		if( (x.x > 0) && (x.y > 0) && (x.z > 0) && ((x.y + x.z) < 1) )	// x.x = t, x.y = u, x.z = v
		{
			if( closestIntersection.distance > x.x )
			{
				closestIntersection.position = start + x.x * dir;
				closestIntersection.distance = x.x;
				closestIntersection.triangleIndex = s;
				hasIntercept = true;
			}
		}
	}

	return hasIntercept;
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
