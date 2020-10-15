#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <Windows.h>

using namespace std;

int swprintf_s(char *buffer, size_t sizeOfBuffer, const char *format);

int main(){
    int ScreenSizeOfWidth = 120; /** Tamanho da tela **/
    int ScreenSizeOfHeight = 40;
    int MapDimensionsWidth = 16; /** Dimensões do mapa **/
    int MapDimensionsHeight = 16;

    float PlayerXPosition = 14.7f;  /** Posição inicial do jogador **/
    float PlayerYPosition = 5.09f;
    float PlayerRotation = 0.0f;
    float PlayerSizeOfFov = 3.14159f/4.0f; /** Campo de visão do jogador **/
    float PlayerFieldOfView = 16.0f;
    float PlayerWalkSpeed = 5.0f; /** Velocidade do jogador **/

	// Create Screen Buffer
	wchar_t *screen = new wchar_t[ScreenSizeOfWidth*ScreenSizeOfHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Create Map of world space # = wall block, . = space
	wstring map;
	map += L"#########.......";
	map += L"#...............";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	while (true){
		// We'll need time differential per frame to calculate modification
		// to movement speeds, to ensure consistant movement, as ray-tracing
		// is non-deterministic
		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();


		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			PlayerRotation -= (PlayerWalkSpeed * 0.75f) * fElapsedTime;

		// Handle CW Rotation
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			PlayerRotation += (PlayerWalkSpeed * 0.75f) * fElapsedTime;

		// Handle Forwards movement & collision
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			PlayerXPosition += sinf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			PlayerYPosition += cosf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			if (map.c_str()[(int)PlayerXPosition * MapDimensionsWidth + (int)PlayerYPosition] == '#')
			{
				PlayerXPosition -= sinf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
				PlayerYPosition -= cosf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			}
		}

		// Handle backwards movement & collision
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			PlayerXPosition -= sinf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			PlayerYPosition -= cosf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			if (map.c_str()[(int)PlayerXPosition * MapDimensionsWidth + (int)PlayerYPosition] == '#')
			{
				PlayerXPosition += sinf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
				PlayerYPosition += cosf(PlayerRotation) * PlayerWalkSpeed * fElapsedTime;;
			}
		}

		for (int x = 0; x < ScreenSizeOfWidth; x++)
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (PlayerRotation - PlayerSizeOfFov/2.0f) + ((float)x / (float)ScreenSizeOfWidth) * PlayerSizeOfFov;

			// Find distance to wall
			float fStepSize = 0.1f;		  // Increment size for ray casting, decrease to increase
			float fDistanceToWall = 0.0f; //                                      resolution

			bool bHitWall = false;		// Set when ray hits wall block
			bool bBoundary = false;		// Set when ray hits boundary between two wall blocks

			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			// Incrementally cast ray from player, along ray angle, testing for
			// intersection with a block
			while (!bHitWall && fDistanceToWall < PlayerFieldOfView)
			{
				fDistanceToWall += fStepSize;
				int nTestX = (int)(PlayerXPosition + fEyeX * fDistanceToWall);
				int nTestY = (int)(PlayerYPosition + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= MapDimensionsWidth || nTestY < 0 || nTestY >= MapDimensionsHeight)
				{
					bHitWall = true;			// Just set distance to maximum depth
					fDistanceToWall = PlayerFieldOfView;
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map.c_str()[nTestX * MapDimensionsWidth + nTestY] == '#')
					{
						// Ray has hit wall
						bHitWall = true;

						// To highlight tile boundaries, cast a ray from each corner
						// of the tile, to the player. The more coincident this ray
						// is to the rendering ray, the closer we are to a tile
						// boundary, which we'll shade to add detail to the walls
						vector<pair<float, float>> p;

						// Test each corner of hit tile, storing the distance from
						// the player, and the calculated dot product of the two rays
						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								// Angle of corner to eye
								float vy = (float)nTestY + ty - PlayerYPosition;
								float vx = (float)nTestX + tx - PlayerXPosition;
								float d = sqrt(vx*vx + vy*vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(make_pair(d, dot));
							}

						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first; });

						// First two/three are closest (we will never see all four)
						float fBound = 0.01;
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float)(ScreenSizeOfHeight/2.0) - ScreenSizeOfHeight / ((float)fDistanceToWall);
			int nFloor = ScreenSizeOfHeight - nCeiling;

			// Shader walls based on distance
			short nShade = ' ';
			if (fDistanceToWall <= PlayerFieldOfView / 4.0f)			nShade = 0x2588;	// Very close
			else if (fDistanceToWall < PlayerFieldOfView / 3.0f)		nShade = 0x2593;
			else if (fDistanceToWall < PlayerFieldOfView / 2.0f)		nShade = 0x2592;
			else if (fDistanceToWall < PlayerFieldOfView)				nShade = 0x2591;
			else											nShade = ' ';		// Too far away

			if (bBoundary)		nShade = ' '; // Black it out

			for (int y = 0; y < ScreenSizeOfHeight; y++)
			{
				// Each Row
				if(y <= nCeiling)
					screen[y*ScreenSizeOfWidth + x] = ' ';
				else if(y > nCeiling && y <= nFloor)
					screen[y*ScreenSizeOfWidth + x] = nShade;
				else // Floor
				{
					// Shade floor based on distance
					float b = 1.0f - (((float)y -ScreenSizeOfHeight/2.0f) / ((float)ScreenSizeOfHeight / 2.0f));
					if (b < 0.25)		nShade = '#';
					else if (b < 0.5)	nShade = 'x';
					else if (b < 0.75)	nShade = '.';
					else if (b < 0.9)	nShade = '-';
					else				nShade = ' ';
					screen[y*ScreenSizeOfWidth + x] = nShade;
				}
			}
		}

		// Display Stats
		//swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", PlayerXPosition, PlayerYPosition, PlayerRotation, 1.0f/fElapsedTime);

		// Display Map
		for (int nx = 0; nx < MapDimensionsWidth; nx++)
			for (int ny = 0; ny < MapDimensionsWidth; ny++)
			{
				screen[(ny+1)*ScreenSizeOfWidth + nx] = map[ny * MapDimensionsWidth + nx];
			}
		screen[((int)PlayerXPosition+1) * ScreenSizeOfWidth + (int)PlayerYPosition] = 'P';

		// Display Frame
		screen[ScreenSizeOfWidth * ScreenSizeOfHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, ScreenSizeOfWidth * ScreenSizeOfHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}
