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

#include <common/OptionsScorched.h>
#include <common/Logger.h>
#include <engine/ScorchedContext.h>
#include <landscapedef/LandscapeTex.h>
#include <landscapedef/LandscapeDefn.h>
#include <landscapedef/LandscapeDefinitions.h>
#include <landscapedef/LandscapeOptions.h>
#include <landscapedef/LandscapeInclude.h>
#include <net/NetBufferPool.h>

OptionsScorched::OptionsScorched()
{
}

OptionsScorched::~OptionsScorched()
{
}

void OptionsScorched::updateLevelOptions(ScorchedContext &context, LandscapeDefinition &defn)
{
	// Get the current level data
	LandscapeTex *ltex = context.getLandscapes().getTex(defn.getTex());
	LandscapeDefn *ldefn = context.getLandscapes().getDefn(defn.getDefn());

	// Get all of the options specified in the current level
	std::map<std::string, XMLEntrySimpleType *> values;
	updateLevelOptions(ltex->texDefn.includes, values);
	updateLevelOptions(ldefn->texDefn.includes, values);

	// Iterate over the level and current options
	std::list<XMLEntry *> &levelOptions = getLevelOptions().getChildren();
	std::list<XMLEntry *> &mainoptions = getMainOptions().getChildren();
	std::list<XMLEntry *>::iterator levelitor;
	std::list<XMLEntry *>::iterator mainitor;
	for (levelitor = levelOptions.begin(), mainitor = mainoptions.begin();
		levelitor != levelOptions.end() && mainitor != mainoptions.end();
		++levelitor, ++mainitor)
	{
		XMLEntrySimpleType *mainEntry = (XMLEntrySimpleType *) (*mainitor);
		XMLEntrySimpleType *levelEntry = (XMLEntrySimpleType *) (*levelitor);

		// Get the current settings value that is in use
		XMLEntrySimpleType *currentEntry = mainEntry;
		if (hasLevelChangedValue(currentEntry->getName().c_str())) currentEntry = levelEntry;
		std::string oldValue = currentEntry->getValueAsString();

		// If this level entry has changed set its new value
		currentEntry = mainEntry;
		std::map<std::string, XMLEntrySimpleType *>::iterator findItor = 
			values.find(mainEntry->getName());
		if (findItor != values.end())
		{
			currentEntry = levelEntry;
			levelEntry->setValueFromString((*findItor).second->getValueAsString());
			changedOptionNames_.insert(mainEntry->getName());
		}
		else
		{
			// Reset the level entry
			changedOptionNames_.erase(mainEntry->getName());
		}

		// Find out the new settings value that is in use 
		std::string newValue = currentEntry->getValueAsString();

		// Log if the value has changed
		if (0 != strcmp(newValue.c_str(), oldValue.c_str()))
		{
			Logger::log(S3D::formatStringBuffer("Level option %s has been changed from %s to %s",
				mainEntry->getName(),
				oldValue.c_str(), newValue.c_str()));
		}
	}
}

void OptionsScorched::updateLevelOptions(std::vector<LandscapeInclude *> &options,
	std::map<std::string, XMLEntrySimpleType *> &values)
{
	// For each include
	std::vector<LandscapeInclude *>::iterator itor;
	for (itor = options.begin();
		itor != options.end();
		++itor)
	{
		LandscapeInclude *option = (*itor);

		std::list<LandscapeOptions *>::iterator oitor = option->options->getChildren().begin(),
			oend = option->options->getChildren().end();

		// For each set of options
		for (;oitor!=oend;++oend)
		{
			LandscapeOptions *optionType = (*oitor);

			// Add the list of options that were defined in the file
			std::list<std::string>::iterator citor = optionType->getChangedOptionNames().begin(),
				cend = optionType->getChangedOptionNames().end();
			for (;citor!=cend;++citor)
			{
				XMLEntrySimpleType *type = optionType->getEntryByName(*citor);
				if (type) values[*citor] = type;
			}
		}
	}
}

void OptionsScorched::updateChangeSet()
{
	NetBuffer *defaultBuffer = NetBufferPool::instance()->getFromPool();

	defaultBuffer->reset();
	mainOptions_.writeToBuffer(*defaultBuffer, true);
	NetBufferReader reader(*defaultBuffer);
	changedOptions_.readFromBuffer(reader, true);

	NetBufferPool::instance()->addToPool(defaultBuffer);
}

bool OptionsScorched::commitChanges()
{
	bool different = false;

	// Compare buffers
	std::list<XMLEntry *> &options = mainOptions_.getChildren();
	std::list<XMLEntry *> &otheroptions = changedOptions_.getChildren();
	std::list<XMLEntry *>::iterator itor;
	std::list<XMLEntry *>::iterator otheritor;
	for (itor=options.begin(), otheritor=otheroptions.begin();
		itor!=options.end() && otheritor!=otheroptions.end();
		++itor, ++otheritor)
	{
		XMLEntrySimpleType *entry = (XMLEntrySimpleType *) *itor;
		XMLEntrySimpleType *otherentry = (XMLEntrySimpleType *) *otheritor;

		DIALOG_ASSERT(entry->getName() == otherentry->getName());

		std::string str = entry->getValueAsString();
		std::string otherstr = otherentry->getValueAsString();
		if (str != otherstr)
		{
			if (!(entry->getData() & XMLEntry::eDataProtected) &&
				!(otherentry->getData() & XMLEntry::eDataProtected))
			{
				if (strlen(str.c_str()) < 20 && strlen(otherstr.c_str()) < 20)
				{
					Logger::log(S3D::formatStringBuffer("Option %s has been changed from %s to %s",
						entry->getName(), str.c_str(), otherstr.c_str()));
				}
				else
				{
					Logger::log(S3D::formatStringBuffer("Option %s has been changed.",
						entry->getName()));
				}
			}

			different = true;
			entry->setValueFromString(otherentry->getValueAsString());
		}
	}

	return different;
}

bool OptionsScorched::hasLevelChangedValue(const char *name)
{
	return changedOptionNames_.find(name) != changedOptionNames_.end();
}

XMLEntrySimpleType *OptionsScorched::getEntryByName(const std::string &name)
{
	if (hasLevelChangedValue(name.c_str()))
	{
		return levelOptions_.getEntryByName(name);
	}
	else
	{
		return mainOptions_.getEntryByName(name);
	}
}
