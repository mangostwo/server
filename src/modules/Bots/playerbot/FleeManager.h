#pragma once

using namespace std;

class Player;

namespace ai
{
    class Engine;

	class FleePoint {
	public:
		FleePoint(PlayerbotAI* ai, float x, float y, float z) : ai(ai), sumDistance(0.0f), minDistance(0.0f) {
			this->x = x;
			this->y = y;
			this->z = z;
		}

	public:
		float x;
		float y;
		float z;

		float sumDistance;
		float minDistance;

	private:
		PlayerbotAI* ai;
	};

	class FleeManager
	{
	public:
		FleeManager(Player* bot, float maxAllowedDistance, float followAngle, bool forceMaxDistance = false) {
			this->bot = bot;
			this->maxAllowedDistance = maxAllowedDistance;
			this->followAngle = followAngle;
			this->forceMaxDistance = forceMaxDistance;
		}

	public:
		bool CalculateDestination(float* rx, float* ry, float* rz);
		bool isUseful();

	private:
		void calculatePossibleDestinations(list<FleePoint*> &points);
		void calculateDistanceToCreatures(FleePoint *point);
		void cleanup(list<FleePoint*> &points);
		FleePoint* selectOptimalDestination(list<FleePoint*> &points);
		bool isBetterThan(FleePoint* point, FleePoint* other);
		bool isTooCloseToEdge(float x, float y, float z, float angle);

	private:
		Player* bot;
		float maxAllowedDistance;
		float followAngle;
		bool forceMaxDistance;
	};

};
