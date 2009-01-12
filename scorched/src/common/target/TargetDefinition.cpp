////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2009
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

#include <weapons/AccessoryStore.h>
#include <common/RandomGenerator.h>
#ifndef S3D_SERVER
	#include <tankgraph/TargetRendererImplTarget.h>
#endif
#include <target/TargetDefinition.h>
#include <target/Target.h>
#include <target/TargetLife.h>
#include <target/TargetShield.h>
#include <target/TargetParachute.h>
#include <target/TargetState.h>
#include <3dsparse/ModelStore.h>
#include <3dsparse/Model.h>
#include <common/Defines.h>
#include <XML/XMLNode.h>

TargetDefinition::TargetDefinition() : 
	life_(1), boundingsphere_(true),
	size_(0, 0, 0), 
	modelscale_(fixed(true, 500)), modelscalediff_(0),
	modelrotation_(0), modelrotationsnap_(-1),
	driveovertodestroy_(false), flattendestroy_(false), border_(0), 
	displaydamage_(true), displayshadow_(true), displayhardwareshadow_(true),
	nodamageburn_(false), nocollision_(false), nofalling_(false),
	nofallingdamage_(false)
{
	shadow_.setDrawShadow(false);
}

TargetDefinition::~TargetDefinition()
{
}

bool TargetDefinition::readXML(XMLNode *node, const char *base)
{
	node->getNamedChild("name", name_, false);
	node->getNamedChild("life", life_, false);
	node->getNamedChild("shield", shield_, false);
	node->getNamedChild("parachute", parachute_, false);
	node->getNamedChild("boundingsphere", boundingsphere_, false);
	node->getNamedChild("nocollision", nocollision_, false);
	node->getNamedChild("nofalling", nofalling_, false);
	node->getNamedChild("nofallingdamage", nofallingdamage_, false);
	node->getNamedChild("nodamageburn", nodamageburn_, false);
	node->getNamedChild("displaydamage", displaydamage_, false);
	node->getNamedChild("displayshadow", displayshadow_, false);
	node->getNamedChild("displayhardwareshadow", displayhardwareshadow_, false);

	node->getNamedChild("size", size_, false);
	node->getNamedChild("modelscale", modelscale_, false);
	node->getNamedChild("modelscalediff", modelscalediff_, false);
	node->getNamedChild("modelrotation", modelrotation_, false);
	node->getNamedChild("modelrotationsnap", modelrotationsnap_, false);
	node->getNamedChild("modelbrightness", modelbrightness_, false);
	node->getNamedChild("border", border_, false);

	XMLNode *modelnode, *burntmodelnode;
	if (!node->getNamedChild("model", modelnode)) return false;
	if (!modelId_.initFromNode(base, modelnode)) return false;
	if (node->getNamedChild("modelburnt", burntmodelnode, false))
	{
		if (!modelburntId_.initFromNode(base, burntmodelnode)) return false;
	}
	else
	{
		modelnode->resurrectRemovedChildren();
		if (!modelburntId_.initFromNode(base, modelnode)) return false;
	}

	node->getNamedChild("flattendestroy", flattendestroy_, false);
	node->getNamedChild("driveovertodestroy", driveovertodestroy_, false);
	node->getNamedChild("removeaction", removeaction_, false);
	node->getNamedChild("burnaction", burnaction_, false);

	if (!shadow_.readXML(node, base)) return false;
	if (!groups_.readXML(node)) return false;

	return node->failChildren();
}

Target *TargetDefinition::createTarget(unsigned int playerId,
	FixedVector &position,
	FixedVector &velocity,
	ScorchedContext &context,
	RandomGenerator &generator)
{
	Target *target = new Target(playerId, 
		name_, context);
	target->getLife().setBoundingSphere(boundingsphere_);

	fixed rotation = modelrotation_;
	if (modelrotationsnap_ > 0)
	{
		rotation = fixed((generator.getRandFixed() * 360).asInt() / 
			(modelrotationsnap_.asInt())) * modelrotationsnap_;
	}
	fixed finalModelScale = modelscale_;
	if (modelscalediff_ > 0)
	{
		finalModelScale += generator.getRandFixed() * modelscalediff_;
	}
	fixed finalBrightness = modelbrightness_;
	if (finalBrightness == -1)
	{
		finalBrightness = generator.getRandFixed() * fixed(true, 7000) + fixed(true, 3000);
	}

	FixedVector finalSize = size_;
	if (finalSize == FixedVector::getNullVector())
	{
		Model *model = ModelStore::instance()->loadModel(modelId_);
		Vector size = model->getMax() - model->getMin();
		finalSize = FixedVector::fromVector(size);
		finalSize *= finalModelScale;
	}

	target->getTargetState().setNoCollision(nocollision_);
	target->getTargetState().setDisplayDamage(displaydamage_);
	target->getTargetState().setDisplayShadow(displayshadow_);
	target->getTargetState().setDisplayHardwareShadow(displayhardwareshadow_);
	target->getTargetState().setNoDamageBurn(nodamageburn_);
	target->getTargetState().setNoFalling(nofalling_);
	target->getTargetState().setNoFallingDamage(nofallingdamage_);
	target->getTargetState().setDriveOverToDestroy(driveovertodestroy_);
	target->getTargetState().setFlattenDestroy(flattendestroy_);
	target->getLife().setMaxLife(life_);
	target->getLife().setSize(finalSize);
	target->getLife().setVelocity(velocity);
	target->getLife().setRotation(rotation);
	target->setBorder(border_);
	target->newGame();

	if (shield_.c_str()[0] && 0 != strcmp(shield_.c_str(), "none"))
	{
		Accessory *shield = context.getAccessoryStore().
			findByPrimaryAccessoryName(shield_.c_str());
		if (!shield)
		{
			S3D::dialogExit("Scorched3D",
				S3D::formatStringBuffer("Failed to find shield named \"%s\"",
				shield_.c_str()));
		}

		target->getShield().setCurrentShield(shield);
	}

	if (parachute_.c_str()[0] && 0 != strcmp(parachute_.c_str(), "none"))
	{
		Accessory *parachute = context.getAccessoryStore().
			findByPrimaryAccessoryName(parachute_.c_str());
		if (!parachute)
		{
			S3D::dialogExit("Scorched3D",
				S3D::formatStringBuffer("Failed to find parachute named \"%s\"",
				parachute_.c_str()));
		}

		target->getParachute().setCurrentParachute(parachute);
	}

	if (removeaction_.c_str()[0] && 0 != strcmp(removeaction_.c_str(), "none"))
	{
		Accessory *action = context.getAccessoryStore().
			findByPrimaryAccessoryName(removeaction_.c_str());		
		if (!action || action->getType() != AccessoryPart::AccessoryWeapon)
		{
			S3D::dialogExit("Scorched3D",
				S3D::formatStringBuffer("Failed to find death action \"%s\"",
				removeaction_.c_str()));
		}

		target->setDeathAction((Weapon *) action->getAction());
	}
	if (burnaction_.c_str()[0] && 0 != strcmp(burnaction_.c_str(), "none"))
	{
		Accessory *action = context.getAccessoryStore().
			findByPrimaryAccessoryName(burnaction_.c_str());		
		if (!action || action->getType() != AccessoryPart::AccessoryWeapon)
		{
			S3D::dialogExit("Scorched3D",
				S3D::formatStringBuffer("Failed to find burn action \"%s\"",
				burnaction_.c_str()));
		}

		target->setBurnAction((Weapon *) action->getAction());
	}

#ifndef S3D_SERVER
	if (!context.getServerMode())
	{
		target->setRenderer(
			new TargetRendererImplTarget(
				target, modelId_, modelburntId_,
				finalModelScale.asFloat(), finalBrightness.asFloat()));
	}
#endif // #ifndef S3D_SERVER

	target->getLife().setTargetPosition(position);
	groups_.addToGroups(context, &target->getGroup(), false);

	return target;
}