#include "Overkill.h"


bool analyzed;
bool analysis_just_finished;



void Overkill::onStart()
{
	Broodwar->setLocalSpeed(0);
	//BWAPI::Broodwar->printf("gl hf :)");

	//Broodwar->printf("The map is %s, a %d player map", Broodwar->mapName().c_str(), Broodwar->getStartLocations().size());
	// Enable some cheat flags
	Broodwar->enableFlag(Flag::UserInput);
	// Uncomment to enable complete map information
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	//read map information into BWTA so terrain analysis can be done in another thread
	BWTA::readMap();

	BWTA::analyze();

	BOOST_FOREACH(BWAPI::Unit* u, BWAPI::Broodwar->self()->getUnits())
	{
		if (u->getType().isResourceDepot())
			InformationManager::Instance().addOccupiedRegionsDetail(BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation()), BWAPI::Broodwar->self(), u);
	}

	analyzed = true;
	analysis_just_finished = true;



	show_bullets = false;
	show_visibility_data = false;
	show_paths = true;

}


void Overkill::onEnd(bool isWinner)
{
	if (isWinner)
	{
		//log win to file
	}
}


void Overkill::onFrame()
{
	if (Broodwar->isReplay())
		return;

	drawStats();

	TimerManager::Instance().startTimer(TimerManager::All);

	TimerManager::Instance().startTimer(TimerManager::Worker);
	WorkerManager::Instance().update();
	TimerManager::Instance().stopTimer(TimerManager::Worker);

	//self start location only available if the map has base locations
	if (analyzed)
	{
		drawTerrainData();

		TimerManager::Instance().startTimer(TimerManager::Information);
		InformationManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::Information);

		TimerManager::Instance().startTimer(TimerManager::Production);
		ProductionManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::Production);

		TimerManager::Instance().startTimer(TimerManager::Building);
		BuildingManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::Building);

		TimerManager::Instance().startTimer(TimerManager::Attack);
		AttackManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::Attack);

		TimerManager::Instance().startTimer(TimerManager::tactic);
		TacticManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::tactic);

		TimerManager::Instance().startTimer(TimerManager::Scout);
		ScoutManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::Scout);

		TimerManager::Instance().startTimer(TimerManager::strategy);
		StrategyManager::Instance().update();
		TimerManager::Instance().stopTimer(TimerManager::strategy);
	}

	TimerManager::Instance().stopTimer(TimerManager::All);

	TimerManager::Instance().displayTimers(490, 180);
	
	/*
	if (show_paths)
	{
		BuildingManager::Instance().addBuildingTask(BWAPI::UnitTypes::Zerg_Spawning_Pool, BWAPI::Broodwar->self()->getStartLocation());
		show_paths = false;
	}*/
}

void Overkill::onSendText(std::string text)
{
	if (text == "/show bullets")
	{
		show_bullets = !show_bullets;
	}
	else if (text == "/show players")
	{
		showPlayers();
	}
	else if (text == "/show forces")
	{
		showForces();
	}
	else if (text == "/show visibility")
	{
		show_visibility_data = !show_visibility_data;
	}
	else if (text == "/analyze")
	{
		if (analyzed == false)
		{
			Broodwar->printf("Analyzing map... this may take a minute");
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);
		}
	}
	else
	{
		Broodwar->printf("You typed '%s'!", text.c_str());
		Broodwar->sendText("%s", text.c_str());
	}
}


void Overkill::drawPaths()
{
	for (set<Unit*>::const_iterator it = Broodwar->self()->getUnits().begin(); it != Broodwar->self()->getUnits().end(); it++) 
	{
		if ((*it)->getType() != UnitTypes::Protoss_Interceptor) 
		{
			Position point1 = (*it)->getPosition();
			Position point2 = (*it)->getTargetPosition();

			Color color = Colors::Cyan;
			if ((*it)->getOrder() == Orders::AttackUnit) 
			{
				color = Colors::Red;
			}

			Broodwar->drawLine(CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), color);

			/*
			if ((*it)->getType().isWorker() && (*it)->getOrder() == Orders::PlaceBuilding)
			{
				int x = (*it)->getTargetPosition().x() - ((*it)->getBuildType().tileWidth()*TILE_SIZE) / 2;
				int y = (*it)->getTargetPosition().y() - ((*it)->getBuildType().tileHeight()*TILE_SIZE) / 2;

				Broodwar->drawBoxMap(x, y, x + (*it)->getBuildType().tileWidth()*TILE_SIZE, y + (*it)->getBuildType().tileHeight()*TILE_SIZE, Colors::Green);
			}*/
		}
	}
}


void Overkill::onNukeDetect(BWAPI::Position target)
{
	if (target != Positions::Unknown)
		Broodwar->printf("Nuclear Launch Detected at (%d,%d)", target.x(), target.y());
	else
		Broodwar->printf("Nuclear Launch Detected");
}

void Overkill::onUnitDiscover(BWAPI::Unit* unit)
{

}

void Overkill::onUnitEvade(BWAPI::Unit* unit)
{

}

void Overkill::onUnitShow(BWAPI::Unit* unit)
{
	if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg || unit->getType() == BWAPI::UnitTypes::Zerg_Larva)
		return;

	TacticManager::Instance().onUnitShow(unit);
	InformationManager::Instance().onUnitShow(unit);
	//for initial 
	ScoutManager::Instance().onUnitShow(unit);
	WorkerManager::Instance().onUnitShow(unit);
	if (unit->getPlayer() == BWAPI::Broodwar->self() && unit->getType() == BWAPI::UnitTypes::Zerg_Zergling)
	{
		AttackManager::Instance().onUnitMorph(unit);
	}
}

void Overkill::onUnitHide(BWAPI::Unit* unit)
{
	
}

void Overkill::onUnitCreate(BWAPI::Unit* unit)
{

}

void Overkill::onUnitDestroy(BWAPI::Unit* unit)
{
	if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg || unit->getType() == BWAPI::UnitTypes::Zerg_Larva)
		return;
		

	InformationManager::Instance().onUnitDestroy(unit);
	if (unit->getPlayer() == BWAPI::Broodwar->self())
	{
		ProductionManager::Instance().onUnitDestroy(unit);
		WorkerManager::Instance().onUnitDestroy(unit);
		ScoutManager::Instance().onUnitDestroy(unit);
		if (!unit->getType().isBuilding() && !unit->getType().isWorker())
		{
			AttackManager::Instance().onUnitDestroy(unit);
			TacticManager::Instance().onUnitDestroy(unit);
		}
	}
	else if (unit->getPlayer() == BWAPI::Broodwar->enemy())
	{
		AttackManager::Instance().onEnemyUnitDestroy(unit);
	}
	
}

void Overkill::onUnitMorph(BWAPI::Unit* unit)
{
	if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg || unit->getType() == BWAPI::UnitTypes::Zerg_Larva)
		return;

	InformationManager::Instance().onUnitMorph(unit);
	if (unit->getPlayer() == BWAPI::Broodwar->self())
	{
		WorkerManager::Instance().onUnitMorph(unit);
		ScoutManager::Instance().onUnitMorph(unit);
		if (!unit->getType().isBuilding() && !unit->getType().isWorker() && unit->getType() != BWAPI::UnitTypes::Zerg_Overlord)
		{
			AttackManager::Instance().onUnitMorph(unit);
		}
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Lair)
		{
			BWAPI::Broodwar->printf("change strategy to Mid game!!!");
			StrategyManager::Instance().changeGameStage(Mid);
		}

		if (unit->getType() == BWAPI::UnitTypes::Zerg_Hive)
		{
			BWAPI::Broodwar->printf("change strategy to End game!!!");
			StrategyManager::Instance().changeGameStage(End);
		}
	}
}


void Overkill::onUnitComplete(BWAPI::Unit *unit)
{

}


DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	BOOST_FOREACH(BWAPI::Unit* u, BWAPI::Broodwar->self()->getUnits())
	{
		if (u->getType().isResourceDepot())
			InformationManager::Instance().addOccupiedRegionsDetail(BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation()), BWAPI::Broodwar->self(), u);
	}

	analyzed = true;
	analysis_just_finished = true;
	return 0;
}

void Overkill::drawStats()
{
	std::set<Unit*> myUnits = Broodwar->self()->getUnits();
	Broodwar->drawTextScreen(5, 0, "I have %d units:", myUnits.size());
	std::map<UnitType, int> unitTypeCounts;
	for (std::set<Unit*>::iterator i = myUnits.begin(); i != myUnits.end(); i++)
	{
		if (unitTypeCounts.find((*i)->getType()) == unitTypeCounts.end())
		{
			unitTypeCounts.insert(std::make_pair((*i)->getType(), 0));
		}
		unitTypeCounts.find((*i)->getType())->second++;
	}
	int line = 1;
	for (std::map<UnitType, int>::iterator i = unitTypeCounts.begin(); i != unitTypeCounts.end(); i++)
	{
		Broodwar->drawTextScreen(5, 16 * line, "- %d %ss", (*i).second, (*i).first.getName().c_str());
		line++;
	}
	// draw the map tile position at map
	Broodwar->drawTextScreen(Broodwar->getMousePosition().x() + 20, Broodwar->getMousePosition().y() + 20, "%d %d", (Broodwar->getScreenPosition().x() + Broodwar->getMousePosition().x()) / TILE_SIZE, (Broodwar->getScreenPosition().y() + Broodwar->getMousePosition().y()) / TILE_SIZE);
}

void Overkill::drawBullets()
{
	std::set<Bullet*> bullets = Broodwar->getBullets();
	for (std::set<Bullet*>::iterator i = bullets.begin(); i != bullets.end(); i++)
	{
		Position p = (*i)->getPosition();
		double velocityX = (*i)->getVelocityX();
		double velocityY = (*i)->getVelocityY();
		if ((*i)->getPlayer() == Broodwar->self())
		{
			Broodwar->drawLineMap(p.x(), p.y(), p.x() + (int)velocityX, p.y() + (int)velocityY, Colors::Green);
			Broodwar->drawTextMap(p.x(), p.y(), "\x07%s", (*i)->getType().getName().c_str());
		}
		else
		{
			Broodwar->drawLineMap(p.x(), p.y(), p.x() + (int)velocityX, p.y() + (int)velocityY, Colors::Red);
			Broodwar->drawTextMap(p.x(), p.y(), "\x06%s", (*i)->getType().getName().c_str());
		}
	}
}

void Overkill::drawVisibilityData()
{
	for (int x = 0; x<Broodwar->mapWidth(); x++)
	{
		for (int y = 0; y<Broodwar->mapHeight(); y++)
		{
			if (Broodwar->isExplored(x, y))
			{
				if (Broodwar->isVisible(x, y))
					Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Green);
				else
					Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Blue);
			}
			else
				Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Red);
		}
	}
}

void Overkill::drawTerrainData()
{
	Broodwar->drawTextScreen(5, 0, "\x04Starts: %d Bases: %d Enemy: %s Frames: %d (%d:%s%d) Map: %s", BWTA::getStartLocations().size(), BWTA::getBaseLocations().size(), Broodwar->enemy()->getRace().getName().c_str(), Broodwar->getFrameCount(), Broodwar->elapsedTime() / 60, (Broodwar->elapsedTime() % 60 < 10) ? "0" : "", Broodwar->elapsedTime() % 60, Broodwar->mapName().c_str());

	//we will iterate through all the base locations, and draw their outlines.
	for (std::set<BWTA::BaseLocation*>::const_iterator i = BWTA::getBaseLocations().begin(); i != BWTA::getBaseLocations().end(); i++)
	{
		TilePosition p = (*i)->getTilePosition();
		Position c = (*i)->getPosition();

		//draw outline of center location
		Broodwar->drawBox(CoordinateType::Map, p.x() * 32, p.y() * 32, p.x() * 32 + 4 * 32, p.y() * 32 + 3 * 32, Colors::Blue, false);

		//draw a circle at each mineral patch
		for (std::set<BWAPI::Unit*>::const_iterator j = (*i)->getStaticMinerals().begin(); j != (*i)->getStaticMinerals().end(); j++)
		{
			Position q = (*j)->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map, q.x(), q.y(), 30, Colors::Cyan, false);
		}

		//draw the outlines of vespene geysers
		for (std::set<BWAPI::Unit*>::const_iterator j = (*i)->getGeysers().begin(); j != (*i)->getGeysers().end(); j++)
		{
			TilePosition q = (*j)->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map, q.x() * 32, q.y() * 32, q.x() * 32 + 4 * 32, q.y() * 32 + 2 * 32, Colors::Orange, false);
		}

		//if this is an island expansion, draw a yellow circle around the base location
		if ((*i)->isIsland())
			Broodwar->drawCircle(CoordinateType::Map, c.x(), c.y(), 80, Colors::Yellow, false);
	}

	//we will iterate through all the regions and draw the polygon outline of it in green.
	for (std::set<BWTA::Region*>::const_iterator r = BWTA::getRegions().begin(); r != BWTA::getRegions().end(); r++)
	{
		BWTA::Polygon p = (*r)->getPolygon();
		for (int j = 0; j<(int)p.size(); j++)
		{
			Position point1 = p[j];
			Position point2 = p[(j + 1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), Colors::Green);
		}
	}

	//we will visualize the chokepoints with red lines
	for (std::set<BWTA::Region*>::const_iterator r = BWTA::getRegions().begin(); r != BWTA::getRegions().end(); r++)
	{
		for (std::set<BWTA::Chokepoint*>::const_iterator c = (*r)->getChokepoints().begin(); c != (*r)->getChokepoints().end(); c++)
		{
			Position point1 = (*c)->getSides().first;
			Position point2 = (*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map, point1.x(), point1.y(), point2.x(), point2.y(), Colors::Red);
		}
	}
}

void Overkill::showPlayers()
{
	std::set<Player*> players = Broodwar->getPlayers();
	for (std::set<Player*>::iterator i = players.begin(); i != players.end(); i++)
	{
		Broodwar->printf("Player [%d]: %s is in force: %s", (*i)->getID(), (*i)->getName().c_str(), (*i)->getForce()->getName().c_str());
	}
}

void Overkill::showForces()
{
	std::set<Force*> forces = Broodwar->getForces();
	for (std::set<Force*>::iterator i = forces.begin(); i != forces.end(); i++)
	{
		std::set<Player*> players = (*i)->getPlayers();
		Broodwar->printf("Force %s has the following players:", (*i)->getName().c_str());
		for (std::set<Player*>::iterator j = players.begin(); j != players.end(); j++)
		{
			Broodwar->printf("  - Player [%d]: %s", (*j)->getID(), (*j)->getName().c_str());
		}
	}
}

