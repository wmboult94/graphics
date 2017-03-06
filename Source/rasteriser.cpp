#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::mat3;
using glm::ivec2;


/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES AND STRUCTS                                                        */

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;

vector<Triangle> triangles;
vec3 currentColor;

struct Intersection
{
	glm::vec3 position;
	float distance;
	int triangleIndex;
};

float focal = SCREEN_HEIGHT;
float centerx = SCREEN_WIDTH/2;
float centery = SCREEN_HEIGHT/2;
vec3 cameraPos( 0, 0, -3.001 );
mat3 rotation;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw();
void VertexShader( const vec3& v, ivec2& p );
void DrawLineSDL( SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color );
void DrawPolygonEdges( const vector<vec3>& vertices );
void ComputePolygonRows( const vector<ivec2>& vertexPixels, vector<ivec2>& leftPixels, vector<ivec2>& rightPixels );
void DrawRows( const vector<ivec2>& leftPixels, const vector<ivec2>& rightPixels );
void DrawPolygon( const vector<vec3>& vertices );
void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result );
// bool ClosestIntersection( vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection );
// bool distanceCompare( const Intersection &x, const Intersection &y );

int main( int argc, char* argv[] )
{

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	LoadTestModel( triangles );

	/* -- Test ComputePolygonRows --
	// vector<ivec2> vertexPixels(3);
	// vertexPixels[0] = ivec2(10, 5);
	// vertexPixels[1] = ivec2( 5,10);
	// vertexPixels[2] = ivec2(15,15);
	// vector<ivec2> leftPixels;
	// vector<ivec2> rightPixels;
	// ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
	// for( int row=0; row<leftPixels.size(); ++row )
	// {
	// 	cout << "Start: ("
	// 	<< leftPixels[row].x << ","
	// 	<< leftPixels[row].y << "). "
	// 	<< "End: ("
	// 	<< rightPixels[row].x << ","
	// 	<< rightPixels[row].y << "). " << endl;
	// }
	------------------------------ */

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
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

}

void Draw()
{
	SDL_FillRect( screen, 0, 0 );

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	// vec3 color ( 1.0, 1.0, 1.0 );

	// vec3 color( 1, 1, 1 );
	// DrawLineSDL( screen, ivec2( 100, 300 ), ivec2( 350, 100 ), color );
	//
	// vec3 color2( 1, 0.1, 0.3 );
	// DrawLineSDL( screen, ivec2( 400, 250 ), ivec2( 450, 200 ), color2 );


	for( int s=0; s<triangles.size(); s++ )
	{
		vector<vec3> vertices(3);

		currentColor = triangles[s].color;

		vertices[0] = triangles[s].v0;
		vertices[1] = triangles[s].v1;
		vertices[2] = triangles[s].v2;

		DrawPolygon( vertices );

		// for( int v=0; v<3; v++ )
		// {
		// 	glm::ivec2 projPos;
		// 	VertexShader( vertices[v], projPos );
		// 	vec3 color( 1, 1, 1 );
		// 	PutPixelSDL( screen, projPos.x, projPos.y, color );
		// }
	}

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

void VertexShader( const vec3& v, ivec2& p )
{
	vec3 pos = ( v - cameraPos ) * rotation;
	p.x = focal*(pos.x/pos.z) + centerx;
	p.y = focal*(pos.y/pos.z) + centery;

}

void DrawLineSDL( SDL_Surface* surface, ivec2 a, ivec2 b, vec3 color )
{
	ivec2 delta = glm::abs( a - b );
	int pixels = glm::max( delta.x, delta.y ) + 1;

	vector<ivec2> line( pixels );
	Interpolate( a, b, line );

	for( int i=0; i<line.size(); i++ )
	{
		PutPixelSDL( surface, line[i].x, line[i].y, color );
	}
}

void DrawPolygonEdges( const vector<vec3>& vertices )
{
	int V = vertices.size();

	// Transform each vertex from 3D world position to 2D image position:
	vector<ivec2> projectedVertices( V );
	for( int i=0; i<V; ++i )
	{
		VertexShader( vertices[i], projectedVertices[i] );
	}

	// Loop over all vertices and draw the edge from it to the next vertex:
	for( int i=0; i<V; ++i )
	{
		int j = (i+1)%V; // The next vertex
		vec3 color( 1, 1, 1 );
		DrawLineSDL( screen, projectedVertices[i], projectedVertices[j], color );
	}

}

void ComputePolygonRows( const vector<ivec2>& vertexPixels, vector<ivec2>& leftPixels, vector<ivec2>& rightPixels )
{

	// 1. Find max and min y-value of the polygon
	// and compute the number of rows it occupies.
	vector<int> yvals;
	for( int s=0; s<vertexPixels.size(); s++)
	{
		yvals.push_back(vertexPixels[s].y);
	}

	int ymin = *min_element( yvals.begin(), yvals.end());
	int ymax = *max_element( yvals.begin(), yvals.end());

	int nrows = ymax - ymin + 1;

	// 2. Resize leftPixels and rightPixels
	// so that they have an element for each row.

	leftPixels.resize(nrows);
	rightPixels.resize(nrows);

	// 3. Initialize the x-coordinates in leftPixels
	// to some really large value and the x-coordinates
	// in rightPixels to some really small value.

	for( int s=0; s<nrows; s++ )
	{
		leftPixels[s].x = numeric_limits<int>::max();
		rightPixels[s].x = -numeric_limits<int>::max();
	}

	// 4. Loop through all edges of the polygon and use
	// linear interpolation to find the x-coordinate for
	// each row it occupies. Update the corresponding
	// values in rightPixels and leftPixels.

	vector< vector<ivec2> > edgeVals(vertexPixels.size());


	// Interpolate edge values
	int index = 0;
	for( int s=0; s<vertexPixels.size()-1; s++ )
	{
		for( int t=s+1; t<vertexPixels.size(); t++ )
		{
			edgeVals[index].resize( max( abs( vertexPixels[s].x - vertexPixels[t].x ), abs( vertexPixels[s].y - vertexPixels[t].y ) ) + 1 );
			Interpolate( vertexPixels[s], vertexPixels[t], edgeVals[index] );
			index++;
		}
	}

	// Get values of left and right pixels
	for( int s=0; s<edgeVals.size(); s++ )
	{
		for( int t=0; t<edgeVals[s].size(); t++ )
		{
			int i = edgeVals[s][t].y - ymin;
			// cout << "Edge no. " << s << ", Edge val: " << edgeVals[s][t].x << ", " << edgeVals[s][t].y << endl;

			if( edgeVals[s][t].x < leftPixels[i].x )
			{
				leftPixels[i].x = edgeVals[s][t].x;
				leftPixels[i].y = edgeVals[s][t].y;
			}

			if( edgeVals[s][t].x > rightPixels[i].x )
			{
				rightPixels[i].x = edgeVals[s][t].x;
				rightPixels[i].y = edgeVals[s][t].y;
			}
		}
	}

}

void DrawRows( const vector<ivec2>& leftPixels, const vector<ivec2>& rightPixels )
{

	for( int s=0; s<leftPixels.size(); s++)
	{
		DrawLineSDL( screen, leftPixels[s], rightPixels[s], currentColor );
	}

}

void DrawPolygon( const vector<vec3>& vertices )
{

	int V = vertices.size();
	vector<ivec2> vertexPixels( V );

	for( int i=0; i<V; ++i )
		VertexShader( vertices[i], vertexPixels[i] );

	vector<ivec2> leftPixels;
	vector<ivec2> rightPixels;
	ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
	DrawRows( leftPixels, rightPixels );

}

// TODO: Depth buffer part of lab sheet onwards

void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result )
{

	int N = result.size();
	vec2 step = vec2(b-a) / float(max(N-1,1));
	vec2 current( a );

	for( int i=0; i<N; ++i )
	{
		result[i] = current;
		current += step;
	}

}
