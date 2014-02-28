////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2013
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
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
////////////////////////////////////////////////////////////////////////////////

#include <engine/ScorchedContext.h>
#include <engine/ActionController.h>
#include <engine/Simulator.h>
#include <target/TargetContainer.h>
#include <target/TargetDamageCalc.h>
#include <target/TargetRenderer.h>
#include <target/TargetState.h>
#include <target/TargetSpace.h>
#include <target/TargetLife.h>
#include <tank/TankViewPoints.h>
#include <actions/Napalm.h>
#include <actions/CameraPositionAction.h>
#include <landscapemap/LandscapeMaps.h>
#include <landscapedef/LandscapeDefinition.h>
#include <landscapedef/LandscapeTex.h>
#include <weapons/AccessoryStore.h>
#include <common/Defines.h>
#include <common/OptionsScorched.h>

#ifndef S3D_SERVER
	#include <client/ScorchedClient.h>
	#include <uiactions/UINapalmRenderer.h>
#endif

static const int deformSize = 3;
static bool deformCreated = false;

#define XY_TO_UINT(x, y) ((((unsigned int) x) << 16) | (((unsigned int) y) & 0xffff))
#define XY2_TO_UINT(x, y) ((((unsigned int) x - x % 2) << 16) | (((unsigned int) y - y % 2) & 0xffff))
#define UINT_TO_X(pt) ((int)(pt >> 16))
#define UINT_TO_Y(pt) ((int)(pt & 0xffff))

Napalm::Napalm(int x, int y, WeaponNapalm *weapon, 
	WeaponFireContext &weaponContext) :
	Action(weaponContext.getInternalContext().getReferenced()),
	startX_(x), startY_(y), napalmTime_(0), 
	weapon_(weapon), 
	weaponContext_(weaponContext), 
	totalTime_(0), hurtTime_(0),
	counter_(0.1f, 0.1f), 
	particleSet_(0), vPoint_(0)
{
}

Napalm::~Napalm()
{
	if (vPoint_) vPoint_->decrementReference();
	while (!napalmPoints_.empty()) 
	{
		NapalmEntry *entry = napalmPoints_.front();
		delete entry;
		napalmPoints_.pop_front();
	}
}

void Napalm::init()
{
	edgePoints_.insert(XY_TO_UINT(startX_, startY_));
	weaponStepTime_ = weapon_->getStepTime(*context_);
	weaponNapalmTime_ = weapon_->getNapalmTime(*context_);
	weaponHurtStepTime_ = weapon_->getHurtStepTime(*context_);
	weaponNapalmHeight_ = weapon_->getNapalmHeight(*context_);
	weaponLandscapeErosion_ = weapon_->getLandscapeErosion(*context_);
	weaponHurtPerSecond_ = weapon_->getHurtPerSecond(*context_);

#ifndef S3D_SERVER
	if (!context_->getServerMode()) 
	{
		if (weapon_->getCameraTrack().getCameraTrack())
		{
			FixedVector position(fixed(startX_), fixed(startY_), context_->getLandscapeMaps().
				getGroundMaps().getHeight(startX_, startY_));
			vPoint_ = new TankViewPointProvider();
			vPoint_->setValues(position);
			vPoint_->incrementReference();

			CameraPositionAction *pos = weapon_->getCameraTrack().createPositionAction(
				weaponContext_.getPlayerId(),
				vPoint_,
				5, 5, true);
			context_->getActionController().addAction(pos);
		}

		setActionRender(new UINapalmRenderer(this));
	}
#endif // #ifndef S3D_SERVER
}

std::string Napalm::getActionDetails()
{
	return S3D::formatStringBuffer("%i,%i %s",
		startX_, startY_, weapon_->getParent()->getName());
}

void Napalm::simulate(fixed frameTime, bool &remove)
{
	// Add napalm for the period of the time interval
	// once the time interval has expired then start taking it away
	// Once all napalm has disapeared the simulation is over
	totalTime_ += frameTime;
	while (totalTime_ > weaponStepTime_)
	{
		totalTime_ -= weaponStepTime_;
		napalmTime_ += weaponStepTime_;
		if (napalmTime_ < weaponNapalmTime_)
		{
			// Still within the time period, add more napalm
			if (int(napalmPoints_.size()) < weapon_->getNumberOfParticles()) 
			{
				simulateAddStep();
			}

			// Check for the case where we land in water
			if (napalmPoints_.empty())
			{
				remove = true;
				break;
			}
		}
		else
		{
			// Not within the time period remove napalm
			if (!napalmPoints_.empty())
			{
				simulateRmStep();
			}
			else
			{
				remove = true;
				break;
			}
		}
	}

	// Calculate how much damage to make to the tanks
	hurtTime_ += frameTime;
	while (hurtTime_ > weaponHurtStepTime_)
	{
		hurtTime_ -= weaponHurtStepTime_;

		simulateDamage();
	}

	Action::simulate(frameTime, remove);
}

fixed Napalm::getHeight(int x, int y)
{
	LandscapeMaps *hmap = &context_->getLandscapeMaps();
	if (x < 0 || y < 0 ||
		x > hmap->getGroundMaps().getLandscapeWidth() ||
		y > hmap->getGroundMaps().getLandscapeHeight())
	{
		// The height at the sides of the landscape is huge
		// so we will never go there with the napalm
		return fixed::MAX_FIXED;
	}

	// Return the correct height the square + the
	// height of all the napalm on this square
	// the napalm builds up and get higher so
	// we can go over small bumps
	return hmap->getGroundMaps().getHeight(x, y) +
		hmap->getGroundMaps().getNapalmHeight(x, y);
}

void Napalm::simulateRmStep()
{
	int pset = napalmPoints_.front()->pset;
	while (!napalmPoints_.empty()) 
	{
		// Check if the entry should be removed
		NapalmEntry *entry = napalmPoints_.front();
		if (pset != entry->pset) break;

		// Remove the first napalm point from the list
		// and remove the height from the landscape
		napalmPoints_.pop_front();
		int x = entry->posX;
		int y = entry->posY;

#ifndef S3D_SERVER
	if (!context_->getServerMode()) 
	{
		((UINapalmRenderer *) renderer_)->removeEntry(entry);
	}
#endif
		delete entry;

		unsigned int pointsCount = XY2_TO_UINT(x, y);
		std::map<unsigned int, int>::iterator countItor =
			napalmPointsCount_.find(pointsCount);
		if (countItor != napalmPointsCount_.end())
		{
			countItor->second--;
			if (countItor->second == 0) napalmPointsCount_.erase(countItor);
		}

		context_->getLandscapeMaps().getGroundMaps().getNapalmHeight(x, y) -= weaponNapalmHeight_;
	}
}

void Napalm::simulateAddStep()
{
	particleSet_++;

	std::set<unsigned int> currentEdges = edgePoints_;
	edgePoints_.clear();

	std::set<unsigned int>::iterator itor;
	for (itor = currentEdges.begin();
		itor != currentEdges.end();
		++itor)
	{
		unsigned int currentEdge = *itor;
		int x = UINT_TO_X(currentEdge);
		int y = UINT_TO_Y(currentEdge);

		simulateAddEdge(x, y);
	}
}

void Napalm::simulateAddEdge(int x, int y)
{
	// Get the height of this point
	fixed height = getHeight(x, y);

	if (!weapon_->getAllowUnderWater())
	{
		// Check napalm is under water 
		LandscapeTex &tex = *context_->getLandscapeMaps().getDefinitions().getTex();
		fixed waterHeight = tex.waterHeight.getValue();

		if (height < waterHeight) // Water height
		{
			// Perhaps we could add a boiling water sound at some point
			return;
		}
	} 

	// Add this current point to the napalm map
	RandomGenerator &random = context_->getSimulator().getRandomGenerator();
	int offset = (random.getRandFixed("Napalm") * 31).asInt();
	NapalmEntry *newEntry = new NapalmEntry(x, y, height, offset, particleSet_);
#ifndef S3D_SERVER
	if (!context_->getServerMode()) 
	{
		((UINapalmRenderer *) renderer_)->addEntry(newEntry);
	}
#endif
	napalmPoints_.push_back(newEntry);

	unsigned int pointsCount = XY2_TO_UINT(x, y);
	std::map<unsigned int, int>::iterator countItor =
		napalmPointsCount_.find(pointsCount);
	if (countItor == napalmPointsCount_.end())
	{
		napalmPointsCount_.insert(std::pair<unsigned int, int>(pointsCount, 1));
	}
	else
	{
		countItor->second++;
	}

#ifndef S3D_SERVER
	if (!context_->getServerMode())
	{

	}
#endif // #ifndef S3D_SERVER

	context_->getLandscapeMaps().getGroundMaps().getNapalmHeight(x, y) += weaponNapalmHeight_;

	// Calculate every time as the landscape may change
	// due to other actions
	fixed heightL = getHeight(x-1, y);
	fixed heightR = getHeight(x+1, y);
	fixed heightU = getHeight(x, y+1);
	fixed heightD = getHeight(x, y-1);

	if (weapon_->getSingleFlow()) 
	{
		fixed *heightLR = 0;
		int LR = 0;
		if (heightL < heightR)
		{
			heightLR = &heightL;
			LR = -1;
		}
		else if (heightL == heightR)
		{
			if (random.getRandUInt("Napalm") % 2 == 0)
			{
				heightLR = &heightL;
				LR = -1;
			}
			else
			{
				heightLR = &heightR;
				LR = +1;
			}
		}
		else
		{
			heightLR = &heightR;
			LR = +1;
		}

		fixed *heightUD = 0;
		int UD = 0;
		if (heightU < heightD)
		{
			heightUD = &heightU;	
			UD = +1;
		}
		else if (heightU == heightD)
		{
			if (random.getRandUInt("Napalm") % 2 == 0)
			{
				heightUD = &heightU;	
				UD = +1;
			}
			else
			{
				heightUD = &heightD;	
				UD = -1;
			}
		}
		else 
		{
			heightUD = &heightD;	
			UD = -1;
		}

		enum Direction
		{
			eUD,
			eLR,
			eNone
		} dir = eNone;
		if (*heightLR < *heightUD)
		{
			if (*heightLR < height) dir = eLR;
		}
		else if (*heightLR == *heightUD)
		{
			if (*heightLR < height) 
			{
				if (random.getRandUInt("Napalm") % 2 == 0)
				{
					dir = eUD;
				}
				else
				{
					dir = eLR;
				}				
			}
		}
		else
		{
			if (*heightUD < height) 
			{
				if (*heightLR < height) dir = eUD;
			}
		}

		switch (dir)
		{
		case eUD:
			edgePoints_.insert(XY_TO_UINT(x, y + UD));
			break;
		case eLR:
			edgePoints_.insert(XY_TO_UINT(x + LR, y));
			break;
		default:
			// None of the landscape is currently lower than the current point
			// Just wait, as this point will be now be covered in napalm
			// and may get higher and higher until it is
			edgePoints_.insert(XY_TO_UINT(x, y));
			break;
		}
	}
	else
	{
		int addedCount = 0;
		if (heightL < height)
		{
			// Move left
			addedCount++;
			edgePoints_.insert(XY_TO_UINT(x - 1, y));
		}
		if (heightR < height)
		{
			// Move right
			addedCount++;
			edgePoints_.insert(XY_TO_UINT(x + 1, y));
		}
		if (heightU < height)
		{
			// Move up
			addedCount++;
			edgePoints_.insert(XY_TO_UINT(x, y + 1));
		}
		if (heightD < height)
		{
			// Move down
			addedCount++;
			edgePoints_.insert(XY_TO_UINT(x, y - 1));
		}
		if (addedCount == 0)
		{
			// None of the landscape is currently lower than the current point
			// Just wait, as this point will be now be covered in napalm
			// and may get higher and higher until it is
			edgePoints_.insert(XY_TO_UINT(x, y));
		}
	}
}

void Napalm::simulateDamage()
{
	const int EffectRadius = weapon_->getEffectRadius();

	// Store how much each tank is damaged
	// Keep in a map so we don't need to create multiple
	// damage actions.  Now we only create one per tank
	std::map<unsigned int, fixed> TargetDamageCalc;

	// Add damage into the damage map for each napalm point that is near to
	// the tanks
	std::map<unsigned int, int>::iterator itor =
		napalmPointsCount_.begin();
	std::map<unsigned int, int>::iterator endItor = 
		napalmPointsCount_.end();
	for (;itor != endItor; ++itor)
	{
		unsigned int pointsCount = itor->first;
		fixed count = fixed(itor->second);
		int x = UINT_TO_X(pointsCount);
		int y = UINT_TO_Y(pointsCount);

		fixed height = context_->getLandscapeMaps().getGroundMaps().
			getHeight(x, y);
		FixedVector position(
			fixed(x), 
			fixed(y),
			height);

		if (weaponLandscapeErosion_ > 0)
		{
			DeformLandscape::deformLandscape(*context_, position, 
				1, true, weaponLandscapeErosion_, "");
			TargetDamageCalc::explosion(
				*context_, weapon_, weaponContext_, 
				position, 1, 0, true, false);
		}

		std::map<unsigned int, Target *> collisionTargets;
		context_->getTargetSpace().getCollisionSet(position, 
			fixed(EffectRadius), collisionTargets);
		std::map<unsigned int, Target *>::iterator itor;
		for (itor = collisionTargets.begin();
			itor != collisionTargets.end();
			++itor)
		{
			Target *target = (*itor).second;
			if (target->getAlive())
			{
				std::map<unsigned int, fixed>::iterator damageItor = 
					TargetDamageCalc.find(target->getPlayerId());
				if (damageItor == TargetDamageCalc.end())
				{
					TargetDamageCalc[target->getPlayerId()] = count * weaponHurtPerSecond_;
				}
				else
				{
					TargetDamageCalc[target->getPlayerId()] += count * weaponHurtPerSecond_;
				}
			}
		}
	}

	// Add all the damage to the tanks (if any)
	if (!TargetDamageCalc.empty())
	{
		std::map<unsigned int, fixed>::iterator damageItor;
		for (damageItor = TargetDamageCalc.begin();
			damageItor != TargetDamageCalc.end();
			++damageItor)
		{
			Target *target = 
				context_->getTargetContainer().getTargetById(damageItor->first);
			fixed damage = (*damageItor).second;

			// Set this target to burnt
			if (target->getRenderer() &&
				weapon_->getObjectDamage())
			{
				target->getRenderer()->targetBurnt();
			}

			// Add damage to the tank
			// If allowed for this target type (mainly for trees)
			if (!target->getTargetState().getNoDamageBurn())
			{
				if (burnedTargets_.find(target->getPlayerId()) == burnedTargets_.end()) 
				{
					burnedTargets_.insert(target->getPlayerId());
					addBurnAction(target);
				}

				// Do last as it may remove the target
				TargetDamageCalc::damageTarget(*context_, target->getPlayerId(), weapon_, 
					weaponContext_, damage, true, false, false);
			}
		}
		TargetDamageCalc.clear();
	}
}

void Napalm::addBurnAction(Target *target)
{
	Accessory *accessory = target->getBurnAction();
	if (accessory)
	{
		Weapon *weapon = (Weapon *) accessory->getAction();
		if (context_->getOptionsGame().getActionSyncCheck())
		{
			context_->getSimulator().addSyncCheck(
				S3D::formatStringBuffer("BurnAction: %u %s", 
					target->getPlayerId(),
					weapon->getParent()->getName()));
		}

		FixedVector position = target->getLife().getTargetPosition();
		FixedVector velocity;
		WeaponFireContext weaponContext(weaponContext_.getPlayerId(), 
			weaponContext_.getInternalContext().getSelectPositionX(), 
			weaponContext_.getInternalContext().getSelectPositionY(), 
			weaponContext_.getInternalContext().getVelocityVector(),
			weaponContext_.getInternalContext().getReferenced(), 
			false);
		weapon->fire(*context_, weaponContext, 
			position, velocity);
	}
}
